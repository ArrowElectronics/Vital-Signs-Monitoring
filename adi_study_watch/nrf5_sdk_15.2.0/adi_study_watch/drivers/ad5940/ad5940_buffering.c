/**
***************************************************************************
* @file         ad5940_buffering.c
* @author       ADI
* @version
* @date         26-Jun-2019
* @brief        Layer to read the AD5940 data
*
***************************************************************************
* @attention
***************************************************************************
*/
/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2020 Analog Devices Inc.                                      *
* All rights reserved.                                                        *
*                                                                             *
* This source code is intended for the recipient only under the guidelines of *
* the non-disclosure agreement with Analog Devices Inc.                       *
*                                                                             *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,    *
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE *
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      *
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING     *
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER         *
* DEALINGS IN THE SOFTWARE.                                                   *
*                                                                             *
*                                                                             *
* This software is intended for use with the AD5950 and derivative parts     *
* only                                                                        *
*                                                                             *
******************************************************************************/
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "ad5940.h"
#include <hw_if_config.h>
#include "sensor_ad5940.h"
#include "eda_application_task.h"
#ifdef ENABLE_ECG_APP
#include "adi_ecg.h"
#endif
#ifdef ENABLE_BCM_APP
#include "bcm_application_task.h"
#endif
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "us_tick.h"
#include "rtc.h"
#ifdef VSM_MBOARD
#include "watch_vsm_motherboard_pin_config.h"
#elif PCBA
#include "watch_board_pcba_pin_config.h"
#else
#include "watch_board_evt_pin_config.h"
#endif
#include "nrf_drv_gpiote.h"
/************************************************* Private macros ***********************************************************/

#define AD5940_RX_BUFFER_SIZE            20

/****** Driver related declarations start ********/
#define WATERMARK_VALUE 16

/************************************************* Structures ***********************************************************/


/*!
 *  @internal
    @struct Rx_Buffer_Def_Struct
    @brief Received data's Ring Buffer Structure
    @var Rx_Buffer_Def_Struct::DataValue
    Data contained in the current buffer
    @var Rx_Buffer_Def_Struct::Next
    Pointer to next buffer structure
 */
typedef struct Ad5940_Rx_Buffer_Def_Struct {
    uint32_t nDataValue;
    uint32_t nTimeStamp;
    struct Ad5940_Rx_Buffer_Def_Struct* Next;
} AD5940_RX_BUFFER_DEF;


/************************************************* Private variables ***********************************************************/
uint16_t gnAd5940TimeGap = 0;
#if DEBUG_ECG
uint16_t gnAd5940TimeGap1 = 10;
uint16_t gnAd5940TimeGap2 = 10;
#endif
uint8_t gnAd5940NumberOfEcgData = 0;
uint8_t bMeasureVolt = 1;
uint8_t gnAd5940TimeIrqSet = 0, gnAd5940TimeIrqSet1 = 0;
uint32_t gn_DivErr_Cnt = 0, gn_ecg_fetch_data_cnt = 0;
uint32_t gn_ad5940_isr_cnt = 0;
uint32_t gn_eda_fetch_data_cnt = 0;
uint32_t BcmFifoCount;

/****** Driver related declarations start ********/
int32_t anRegReadAd5940Data[WATERMARK_VALUE] = {0};
AD5940_RX_BUFFER_DEF Ad5940RxBuff[AD5940_RX_BUFFER_SIZE];
AD5940_RX_BUFFER_DEF *Ad5940WtBufferPtr, *Ad5940RdBufferPtr;
volatile uint8_t gnAd5940DataReady;
uint32_t gnAd5940TimeCurVal = 0, gnAd5940TimePreVal = 0;
uint32_t gnAd5940TimeCurVal_copy = 0;
uint32_t gnAd5940TimeCurValInMicroSec = 0;
uint32_t gnAd5940TimePrevVal = 0;
uint32_t FifoCount;
AD5950_APP_ENUM_t gnAd5940App = AD5940_APP_NONE;
#ifdef DEBUG_EDA
uint32_t eda_tick=0,eda_tick_prev=0;
uint32_t eda_load_time_diff=0;
#endif
#ifdef DEBUG_BCM
uint32_t bcm_tick=0,bcm_tick_prev=0,bcm_load_time_diff=0;
#endif
static uint8_t FifoOverflow = 0;
static uint32_t fifo_cnt=0;
uint32_t nTcv = 0;
static bool day_roll_over = false; //flag to track day roll-over

/************************************************* External variables ***********************************************************/
#ifdef ENABLE_ECG_APP
extern ADI_OSAL_SEM_HANDLE   ecg_task_evt_sem;
extern AppECGCfg_Type AppECGCfg;
#endif
#ifdef ENABLE_EDA_APP
extern ADI_OSAL_SEM_HANDLE   eda_task_evt_sem;
extern g_state_eda_t g_state_eda;
extern AppEDACfg_Type AppEDACfg;
#endif
#ifdef ENABLE_BCM_APP
extern ADI_OSAL_SEM_HANDLE   bcm_task_evt_sem;
extern AppBIACfg_Type AppBIACfg;
#endif

/************************************************* Function declarations ***********************************************************/
uint16_t ad5950_buff_get(uint32_t *rdData, uint32_t *time);
uint16_t ad5950_buff_put(int32_t *pRegReadData, uint32_t nTcv);
uint8_t getAd5940DataReady();
void ResetTimeGapAd5940();



/*!
  ****************************************************************************
  *@brief      Clear AD5940 soft buffer
  *@param      None
  *@return     None
******************************************************************************/
void ClearDataBufferAd5940() {
    Ad5940WtBufferPtr = &Ad5940RxBuff[0];
    Ad5940RdBufferPtr = &Ad5940RxBuff[0];
    gnAd5940TimePrevVal = 0;
}

/*!
  ****************************************************************************
  *@brief      Reset time gap when FIFO is cleared
  *@param      None
  *@return     None
******************************************************************************/
void ResetTimeGapAd5940() {
   gnAd5940TimePrevVal = 0;
   gnAd5940TimeGap = 10;
}


/*!
  ****************************************************************************
  *@brief       Create a Ring Buffer to store received data.
  *@param      None
  *@return     None
******************************************************************************/
void Ad5940RxBufferInit(void) {
    uint8_t aLoopCount;
    /* clear soft buffer */
    ClearDataBufferAd5940();
    gnAd5940NumberOfEcgData = 0;
    /* create ring buffer */
    for (aLoopCount = 0; aLoopCount < AD5940_RX_BUFFER_SIZE; aLoopCount++) {
        Ad5940RxBuff[aLoopCount].Next = &Ad5940RxBuff[aLoopCount + 1];
    }
    /* address of last value in buffer linked to first to
      form ring buffer */
    Ad5940RxBuff[AD5940_RX_BUFFER_SIZE - 1].Next = &Ad5940RxBuff[0];
}

/*!
  ****************************************************************************
  *@brief      Get AD5940 Interrupt ready flag
  *@param      None
  *@return     None
******************************************************************************/
uint8_t getAd5940DataReady(){

  return gnAd5940DataReady;

}

/*!
  ****************************************************************************
  *@brief      Get AD5940 Interrupt count which is required for debug purposes
  *@param      None
  *@return     None
******************************************************************************/
uint32_t Ad5940DrvGetDebugInfo() {
  return gn_ad5940_isr_cnt;
}

#ifdef ENABLE_EDA_APP
/*!
  ****************************************************************************
  *@brief      Get FIFO Count, if FIFO Overflow return failure, else return
               success
  *@param      *fifo_sample_cnt : Pointer to variable FIFO Count
  *@return     1 ( Failure ) /0 ( Success )
******************************************************************************/
uint8_t get_fifostat(uint32_t *fifo_sample_cnt) {
  if(g_state_eda.num_starts > 0)
  {
    if(FifoOverflow == 1) {
      *fifo_sample_cnt = fifo_cnt;
      return 1;
   }
   else {
      *fifo_sample_cnt = fifo_cnt;
      return 0;
   }
  }
  else  {
      *fifo_sample_cnt = 0;
      return 0;
  }
}
#endif

/*!
  ****************************************************************************
  *@brief      AD5940 FIFO Call back called when AD5940 ISR occurs
  *@param      None
  *@return     None
******************************************************************************/
void Ad5940FifoCallBack(void) {
    /* Read current time */
    gnAd5940TimeCurVal =  get_sensor_time_stamp();
#ifdef PROFILE_TIME_ENABLED
    gnAd5940TimeCurValInMicroSec = get_micro_sec();
#endif
    /* set ISR flag high */
    gnAd5940DataReady = 1;
    gnAd5940TimeIrqSet = 1;
    gnAd5940TimeIrqSet1 = 1;
    /* Increment isr count */
    gn_ad5940_isr_cnt++;

    switch(gnAd5940App){
#ifdef ENABLE_ECG_APP
      /* Case 1: ECG Interrupt */
      case AD5940_APP_ECG:
                /* post ecg semaphore */
                adi_osal_SemPost(ecg_task_evt_sem);
      break;
#endif
#ifdef ENABLE_EDA_APP
      case AD5940_APP_EDA:
      /* Case 2: EDA Interrupt */
#ifdef DEBUG_EDA
                /* timing obtained to know difference between successive EDA Interrupts */
                eda_tick = MCU_HAL_GetTick();
                eda_load_time_diff = eda_tick - eda_tick_prev;
                eda_tick_prev = eda_tick;
                NRF_LOG_INFO("Time diff b/n EDA Interrupts = %d",eda_load_time_diff);
#endif
                /* post eda semaphore */
                adi_osal_SemPost(eda_task_evt_sem);
      break;
#endif
#ifdef ENABLE_BCM_APP
      case AD5940_APP_BCM:
      /* Case 3: BCM Interrupt */
#ifdef DEBUG_BCM
                /* timing obtained to know difference between successive BCM Interrupts */
                bcm_tick = MCU_HAL_GetTick();
                bcm_load_time_diff = bcm_tick - bcm_tick_prev;
                bcm_tick_prev = bcm_tick;
                NRF_LOG_INFO("Time diff b/n BCM Interrupts = %d",bcm_load_time_diff);
#endif
                /* post bcm semaphore */
                adi_osal_SemPost(bcm_task_evt_sem);
      break;
#endif
      default:
      break;
     }
}


/*!
  ****************************************************************************
  *@brief      Move data from SPI FIFO to this Ring Buffer. If WtPtr reach RdPtr,
               host's reading is too slow.
  *@param      *pRegReadData: Pointer to data being read out from FIFO
  *             tcv Timestamp of the oldest data in the FIFO
  *@return     int16_t A 16-bit integer: 0 ( success );  -1 ( failure )
******************************************************************************/
uint16_t ad5950_buff_put(int32_t *pRegReadData, uint32_t nTcv) {
    uint32_t nTimeStamp;
    /* when read = write pointer , return error */
    if (Ad5940WtBufferPtr->Next == Ad5940RdBufferPtr) {
        return AD5940Drv_ERROR;
    }
    /* calculate time stamp based on roll over */
    if (gnAd5940TimeIrqSet == 1 && gnAd5940TimeIrqSet1 == 0) {
        gnAd5940TimeIrqSet = 0;
        nTimeStamp = (uint32_t)(nTcv % MAX_RTC_TICKS_FOR_24_HOUR);
    } else {
        nTimeStamp = (uint32_t)((gnAd5940TimePreVal + gnAd5940TimeGap) % MAX_RTC_TICKS_FOR_24_HOUR);
    }
    /* assign current time stamp to previous value for next iteration  */
    gnAd5940TimePreVal = nTimeStamp;
    /* assign pointers to data and time stamp of Ring buffer */
    Ad5940WtBufferPtr->nTimeStamp = nTimeStamp;
    Ad5940WtBufferPtr->nDataValue = *pRegReadData;
    /* Increment write pointer to point to next node */
    Ad5940WtBufferPtr = Ad5940WtBufferPtr->Next;
    gnAd5940NumberOfEcgData++;
    return AD5940Drv_SUCCESS;
}

/*!
  ****************************************************************************
  *@brief      Read Ring buffer which holds data and time stamp
  *@param      *rxData: data read from ring buffer
  *@param      *time: timestamp attached to data when it was written
  *@return     int16_t A 16-bit integer: 0 ( success );  -1 ( failure )
******************************************************************************/
uint16_t ad5950_buff_get(uint32_t *rxData, uint32_t *time) {

    if (Ad5940RdBufferPtr == Ad5940WtBufferPtr) {
        /* Return error as there is no data in soft FIFO */
        return AD5940Drv_ERROR;
    }

    /* data read from ring buffer */
    *rxData = Ad5940RdBufferPtr->nDataValue;

    /* read time stamp attached to written data from ring buffer
        if attached time is valid */
    if (time != 0) {
        *time = Ad5940RdBufferPtr->nTimeStamp;
    }

    /* Increment read pointer to point it to next node */
    Ad5940RdBufferPtr = Ad5940RdBufferPtr->Next;
    gnAd5940NumberOfEcgData--;
    return AD5940Drv_SUCCESS;
}
/*!
  ****************************************************************************
  *@brief      Read ECG data from the AD5940.
  *@param      None
  *@return     int8_t A 8-bit integer: 0 ( success );  -1 ( failure )
******************************************************************************/
#if DEBUG_ECG
uint16_t fifo_time1=0,fifo_time2=0,fifo_time_diff=0;
uint16_t fifo_read_flag=0;
uint16_t IncrementalFifolevel=0;
uint16_t time_interval=0;
uint16_t div_factor=0;
extern uint16_t fifo_threshold_not_four;
extern uint8_t debug_flag;
extern uint32_t gad5940TriggerPulseCnt;
struct trigger_fifo_level{
uint32_t cur_tick;
uint32_t prev_tick;
uint32_t tick_diff;
uint32_t trigger_cnt;
uint32_t fifo_cnt;
};

struct trigger_fifo_level trig_fifo_arr[250];
uint32_t ntrig_index = 0;
#endif

#ifdef ENABLE_ECG_APP
int8_t ad5940_read_ecg_data_to_buffer() {

  int32_t *pRegReadData;
  uint32_t Fifolevel;
  uint32_t nCount = 0;

  pRegReadData = &anRegReadAd5940Data[0];

  /* if ISR has occured */
  if(gnAd5940DataReady)  {
    /* clear ISR flag */
    gnAd5940DataReady = 0;
    /* Wakup AFE by read register, read 10 times at most */
    if(AD5940_WakeUp(10) > 10)  {
      /* Wakeup Failed */
      return AD5940ERR_WAKEUP;
    }
    /* We are operating registers, so we don't allow AFE enter sleep mode which is done in our sequencer */
    AD5940_SleepKeyCtrlS(SLPKEY_LOCK);

    /* incrment fetch count variable */
    gn_ecg_fetch_data_cnt++;
    /* If FIFO threshold flag is set high in AD5940 register field, read FIFO data */
    if(AD5940_INTCTestFlag(AFEINTC_0, AFEINTSRC_DATAFIFOTHRESH) == bTRUE) {
#if DEBUG_ECG
      fifo_time2 = MCU_HAL_GetTick();
      fifo_time_diff = fifo_time2- fifo_time1;
      fifo_time1 = fifo_time2;
#endif
      /* Now there should be fifo data ; number of samples to read is being set in
      ecg app which FIFO threshold variable*/
      Fifolevel = AD5940_FIFOGetCnt();
      NRFX_ASSERT(Fifolevel <= ECG_WATER_MARK_LEVEL);
#if DEBUG_ECG
       Fifolevel1=Fifolevel;
       trig_fifo_arr[ntrig_index].cur_tick = gnAd5940TimeCurVal;
       trig_fifo_arr[ntrig_index].prev_tick = gnAd5940TimePrevVal;
       trig_fifo_arr[ntrig_index].tick_diff = gnAd5940TimeCurVal - gnAd5940TimePrevVal;
       trig_fifo_arr[ntrig_index].trigger_cnt = gad5940TriggerPulseCnt;
       trig_fifo_arr[ntrig_index].fifo_cnt = Fifolevel;

       if(++ntrig_index >= 250)
          ntrig_index = 0;
#endif

#if DEBUG_ECG
      fifo_read_flag = 0;
      if(Fifolevel != 4){
        fifo_count_deviated[deviated_cnt++] = Fifolevel;
      }
      if(deviated_cnt == 50){
        deviated_cnt=0;
        memset(fifo_count_deviated,0,50);
      }
       IncrementalFifolevel += Fifolevel;
#endif
      AD5940_FIFORd(pRegReadData, Fifolevel);
      AD5940_INTCClrFlag(AFEINTSRC_DATAFIFOTHRESH);
      /* If there is need to do AFE re-configure, do it here when AFE is in active state */
      AppECGRegModify(pRegReadData, &Fifolevel);
      /* Allow AFE to enter sleep mode. AFE will stay at active mode until sequencer trigger sleep */
      AD5940_SleepKeyCtrlS(SLPKEY_UNLOCK);
      /* Read fifo data */
      AppECGDataProcess(pRegReadData,&Fifolevel);
      /* Process data */
      if (gnAd5940TimePrevVal != 0) {
      /* this happens in roll over when current time value is less than previous,
        handle roll over of time stamp */
        if(gnAd5940TimeCurVal < gnAd5940TimePrevVal)  {
          /* MAX_RTC_TICKS_FOR_24_HOUR:- Max RTC Count value returned by get_sensor_timestamp() after 24hrs.
          Adding that value to have correct TimeGap value during day roll-over */
          gnAd5940TimeGap = (uint16_t)((MAX_RTC_TICKS_FOR_24_HOUR + gnAd5940TimeCurVal - gnAd5940TimePrevVal) / Fifolevel);
          day_roll_over = true;
        }
        /* calculate time gap based on current time value - previous value */
        else  {
          gnAd5940TimeGap = (uint16_t)(((gnAd5940TimeCurVal - gnAd5940TimePrevVal) + (Fifolevel/2)) / Fifolevel);
        }
      }
      /* occurs when first time Interrupt data is read out , calculate time gap
        based on number of tick of RTC */
      else  {
        /* 32KHz is ticks resolution of RTC */
         gnAd5940TimeGap = (uint16_t)((1000.0/AppECGCfg.ECGODR) * RTC_TICKS_PER_MILLI_SEC);
      }
#if DEBUG_ECG
      if(Fifolevel != 4){
          time_interval = (gnAd5940TimeCurVal - gnAd5940TimePrevVal);
          div_factor = Fifolevel;
          if(debug_flag!=0){
            gnAd5940TimeGap1 = (uint16_t)((gnAd5940TimeCurVal_copy - gnAd5940TimePrevVal))/Fifolevel;
            gnAd5940TimeGap2 = (uint16_t)((gnAd5940TimeCurVal - gnAd5940TimePrevVal))/Fifolevel;
            Fifolevel1=Fifolevel;
            debug_flag=0;
          }
      }
#endif
      /* assign previous time value with current time value for next iteration */
      gnAd5940TimePrevVal = gnAd5940TimeCurVal;

      /* current time where data is processed */
      nTcv =  get_sensor_time_stamp();
      /* Normally, data processed time > interrupt occured time , hence update
      current time value by difference between current time when data is processed and
      time corresponding to interrupt occured */
      /*if ((nTcv > gnAd5940TimeCurVal) && (gnAd5940TimeGap != 0))  {
        //gnAd5940TimeCurVal += nTcv - gnAd5940TimeCurVal;
      }*/
      /* If current data processed time < Interrupt occured time, this could be case of
       roll over, update current time based on data processes time */
      if((nTcv < gnAd5940TimeCurVal) && (gnAd5940TimeGap != 0))  {
        gnAd5940TimeCurVal = nTcv;
        day_roll_over = true;
      }
      /* else, increment error count */
      else  {
        gn_DivErr_Cnt++;
      }
      /* If roll over flag is set */
      if(day_roll_over)   {
      /* MAX_RTC_TICKS_FOR_24_HOUR:- Max RTC Count value returned by get_sensor_timestamp() after 24hrs.
        Adding that value to have correct sample_interval during day roll-over */
        nTcv = (uint32_t)(MAX_RTC_TICKS_FOR_24_HOUR + gnAd5940TimeCurVal - ((Fifolevel * gnAd5940TimeGap) - gnAd5940TimeGap));
        /* Reset roll over flag */
        day_roll_over = false;
      }
      else  {
        nTcv = (uint32_t)(gnAd5940TimeCurVal - ((Fifolevel * gnAd5940TimeGap) - gnAd5940TimeGap));
      }

      gnAd5940TimeIrqSet1 = 0;
      /* put read data from fifo to ring buffer */
      for (nCount = 0; nCount < Fifolevel; nCount++) {
        ad5950_buff_put(&pRegReadData[nCount], nTcv);
      }
    }
  }
  return AD5940ERR_OK;
}
#endif

#ifdef ENABLE_EDA_APP
/*!
  ****************************************************************************
  *@brief      Read EDA data from the AD5940.
  *@param      *pBuff: point to an array in which the data will be placed
  *@param      *pCount:FIFO count
  *@return     int8_t A 8-bit integer: 0 ( success );  -1 ( failure )
******************************************************************************/
AD5940Err AppEDAISR(void *pBuff, uint32_t *pCount)  {
  uint32_t nCount;
  int32_t *pBuffer;

  if(AppEDACfg.EDAInited == bFALSE)
    return AD5940ERR_APPERROR;
  /* if ISR has occured */
  if(gnAd5940DataReady)  {
     /* clear ISR flag */
    gnAd5940DataReady = 0;

    /* Wakeup AFE by read register, read 10 times at most */
    if(AD5940_WakeUp(10) > 10)  {
    /* Wakeup Failed */
     return AD5940ERR_WAKEUP;
    }
#ifndef EXTERNAL_TRIGGER_EDA
    /* We are operating registers, so we don't allow AFE enter sleep mode which is done in our sequencer */
    AD5940_SleepKeyCtrlS(SLPKEY_LOCK);  /* Don't enter hibernate */
#endif
    *pCount = 0;

    /* increment fetch count variable */
    gn_eda_fetch_data_cnt++;

    /* If FIFO threshold flag is set high in AD5940 register field, read FIFO data */
    if(AD5940_INTCTestFlag(AFEINTC_0, AFEINTSRC_DATAFIFOTHRESH) == bTRUE) {
      /* Now there should be fifo data ; number of samples to read is being set in
      eda app which FIFO threshold variable*/
      FifoCount = (AD5940_FIFOGetCnt()/2)*2;

      if(FifoCount > AppEDACfg.FifoThresh)  {
        /* check for FIFO Overflow */
        if(AD5940_INTCTestFlag(AFEINTC_0, AFEINTSRC_DATAFIFOOF) == bTRUE) {
          /* set fifo over flow flag */
          FifoOverflow = 1;
          /* read fifo count */
          fifo_cnt = AD5940_FIFOGetCnt();
        }
      }
      /* read fifo */
      AD5940_FIFORd((int32_t *)pBuff, FifoCount);
      /* clear fifo threshold */
      AD5940_INTCClrFlag(AFEINTSRC_DATAFIFOTHRESH);
       /* If there is need to do AFE re-configure, do it here when AFE is in active state */
      AppEDARegModify(pBuff, &FifoCount);
#ifndef EXTERNAL_TRIGGER_EDA
      /* Manually put AFE back to hibernate mode. This operation only takes effect when register value is ACTIVE previously */
      AD5940_EnterSleepS();
      /* Don't enter hibernate */
      AD5940_SleepKeyCtrlS(SLPKEY_UNLOCK);  /* Don't enter hibernate */
#endif
      /* Process data */
      AppEDADataProcess((int32_t*)pBuff,&FifoCount);
      *pCount = FifoCount;

      /* Process data and timw */
      if(FifoCount != 0){
        if (gnAd5940TimePrevVal != 0) {
        /* this happens in roll over when current time value is less than previous,
        handle roll over of time stamp */
          if(gnAd5940TimeCurVal < gnAd5940TimePrevVal)  {
          /* MAX_RTC_TICKS_FOR_24_HOUR:- Max RTC Count value returned by get_sensor_timestamp() after 24hrs.
          Adding that value to have correct TimeGap value during day roll-over */
          gnAd5940TimeGap = (uint16_t)((MAX_RTC_TICKS_FOR_24_HOUR + gnAd5940TimeCurVal - gnAd5940TimePrevVal) / FifoCount);
          day_roll_over = true;
        }
         /* calculate time gap based on current time value - previous value */
        else  {
          gnAd5940TimeGap = (uint16_t)((gnAd5940TimeCurVal - gnAd5940TimePrevVal) / FifoCount);
        }
       }
       /* occurs when first time Interrupt data is read out , calculate time gap
        based on number of tick of RTC */
       else {
           /* 32KHz is ticks resolution of RTC */
          gnAd5940TimeGap = (uint16_t)((1000.0/AppEDACfg.EDAODR) * RTC_TICKS_PER_MILLI_SEC);//32KHz ticks resolution
       }
      }

      /* assign previous time value with current time value for next iteration */
      gnAd5940TimePrevVal = gnAd5940TimeCurVal;
       /* current time where data is processed */
      nTcv =  get_sensor_time_stamp();
       /* Normally, data processed time > interrupt occured time , hence update
      current time value by difference between current time when data is processed and
      time corresponding to interrupt occured */
      if ((nTcv > gnAd5940TimeCurVal) && (gnAd5940TimeGap != 0))  {
//        gnAd5940TimeCurVal += nTcv - gnAd5940TimeCurVal;
      }
      /* If current data processed time < Interrupt occured time, this could be case of
       roll over, update current time based on data processes time */
      else if((nTcv < gnAd5940TimeCurVal) && (gnAd5940TimeGap != 0))  {
        gnAd5940TimeCurVal = nTcv;
        day_roll_over = true;
      }
      /* else, increment error count */
      else  {
        gn_DivErr_Cnt++;
      }
       /* If roll over flag is set */
      if(day_roll_over) {
      /* MAX_RTC_TICKS_FOR_24_HOUR:- Max RTC Count value returned by get_sensor_timestamp() after 24hrs.
        Adding that value to have correct sample_interval during day roll-over */
        nTcv = (uint32_t)(MAX_RTC_TICKS_FOR_24_HOUR + gnAd5940TimeCurVal - ((FifoCount * gnAd5940TimeGap) - gnAd5940TimeGap));
        /* Reset roll over flag */
        day_roll_over = false;
      }
      else  {
        nTcv = (uint32_t)(gnAd5940TimeCurVal - ((FifoCount * gnAd5940TimeGap) - gnAd5940TimeGap));
      }

      gnAd5940TimeIrqSet1 = 0;

       pBuffer = (int32_t *)pBuff;
       /* put read data from fifo to ring buffer */
       for (nCount = 0; nCount < FifoCount; nCount++) {
          //NRF_LOG_DEBUG("pRegReadData[%d]=%u",nCount,pRegReadData[nCount]);
          ad5950_buff_put((int32_t*)&pBuffer[nCount], nTcv);
        }
      }
    }
  return AD5940ERR_OK;
}
#endif

#ifdef ENABLE_BCM_APP
/*!
  ****************************************************************************
  *@brief      Read BCM data from the AD5940.
  *@param     None
  *@return     int8_t A 8-bit integer: 0 ( success );  -1 ( failure )
******************************************************************************/
int8_t ad5940_read_bcm_data_to_buffer() {
  int32_t *pRegReadData;
  uint32_t nCount = 0;

  pRegReadData = &anRegReadAd5940Data[0];

  /* if ISR has occured */
  if(gnAd5940DataReady)  {
  /* clear ISR flag */
    gnAd5940DataReady = 0;
   /* Wakeup AFE by read register, read 10 times at most */
  if(AD5940_WakeUp(10) > 10) {
   /* Wakeup Failed */
    return AD5940ERR_WAKEUP;
  }
  /* We are operating registers, so we don't allow AFE enter sleep mode which is done in our sequencer */
  AD5940_SleepKeyCtrlS(SLPKEY_LOCK);

  if(AD5940_INTCTestFlag(AFEINTC_0, AFEINTSRC_DATAFIFOTHRESH) == bTRUE) {
    /* Now there should be 4 data in FIFO */
    BcmFifoCount = (AD5940_FIFOGetCnt()/4)*4;
     /* Now there should be fifo data ; number of samples to read is being set in
      eda app which FIFO threshold variable*/
    AD5940_FIFORd(pRegReadData, BcmFifoCount);
    /* clear fifo interrupt */
    AD5940_INTCClrFlag(AFEINTSRC_DATAFIFOTHRESH);
    /* If there is need to do AFE re-configure, do it here when AFE is in active state */
    AppBIARegModify(pRegReadData, &BcmFifoCount);
    /* Manually put AFE back to hibernate mode. */
    AD5940_EnterSleepS();
    /* Allow AFE to enter hibernate mode */
    AD5940_SleepKeyCtrlS(SLPKEY_UNLOCK);
    /* Process data */
    AppBIADataProcess((int32_t*)pRegReadData,&BcmFifoCount);

    /* Process data */
    if (gnAd5940TimePrevVal != 0) {
        /* this happens in roll over when current time value is less than previous,
        handle roll over of time stamp */
       if(gnAd5940TimeCurVal < gnAd5940TimePrevVal) {
          /* MAX_RTC_TICKS_FOR_24_HOUR:- Max RTC Count value returned by get_sensor_timestamp() after 24hrs.
          Adding that value to have correct TimeGap value during day roll-over */
          gnAd5940TimeGap = (uint16_t)((MAX_RTC_TICKS_FOR_24_HOUR + gnAd5940TimeCurVal - gnAd5940TimePrevVal) / BcmFifoCount);
          day_roll_over = true;
        }
         /* calculate time gap based on current time value - previous value */
        else  {
          gnAd5940TimeGap = (uint16_t)((gnAd5940TimeCurVal - gnAd5940TimePrevVal) / BcmFifoCount);
        }
     }
     /* occurs when first time Interrupt data is read out , calculate time gap
        based on number of tick of RTC */
     else {
       /* 32KHz is ticks resolution of RTC */
       gnAd5940TimeGap = (uint16_t)((1000.0/AppBIACfg.BiaODR) * RTC_TICKS_PER_MILLI_SEC);//32KHz ticks resolution
     }
     /* assign previous time value with current time value for next iteration */
    gnAd5940TimePrevVal = gnAd5940TimeCurVal;
    /* current time where data is processed */
    nTcv =  get_sensor_time_stamp();

    if ((nTcv > gnAd5940TimeCurVal) && (gnAd5940TimeGap != 0))  {
//       gnAd5940TimeCurVal += nTcv - gnAd5940TimeCurVal;
    }
     /* Normally, data processed time > interrupt occured time , hence update
      current time value by difference between current time when data is processed and
      time corresponding to interrupt occured */
    else if((nTcv < gnAd5940TimeCurVal) && (gnAd5940TimeGap != 0))  {
       gnAd5940TimeCurVal = nTcv;
       day_roll_over = true;
    }
    /* else, increment error count */
    else
       gn_DivErr_Cnt++;
    /* If roll over flag is set */
    if(day_roll_over) {
      /* MAX_RTC_TICKS_FOR_24_HOUR:- Max RTC Count value returned by get_sensor_timestamp() after 24hrs.
        Adding that value to have correct TS during day roll-over */
      nTcv = (uint32_t)(MAX_RTC_TICKS_FOR_24_HOUR + gnAd5940TimeCurVal - ((BcmFifoCount * gnAd5940TimeGap) - gnAd5940TimeGap));
      /* Reset roll over flag */
      day_roll_over = false;
    }
    else  {
      nTcv = (uint32_t)(gnAd5940TimeCurVal - ((BcmFifoCount * gnAd5940TimeGap) - gnAd5940TimeGap));
    }

    gnAd5940TimeIrqSet1 = 0;
    /* put read data from fifo to ring buffer */
    for (nCount = 0; nCount < BcmFifoCount; nCount++) {
        ad5950_buff_put(&pRegReadData[nCount], nTcv);
    }
  }
 }
  return AD5940ERR_OK;
}
#endif

/*!
  ****************************************************************************
  *@brief     Ad5940 initialization
  *@param     None
  *@return    None
******************************************************************************/
void Ad5940Init() {
  Ad5940RxBufferInit();

   /* configure reset pin of ad5940 */
  nrf_gpio_cfg_output(AD5940_RESET_PIN);
  nrf_gpio_pin_set(AD5940_RESET_PIN);
}
