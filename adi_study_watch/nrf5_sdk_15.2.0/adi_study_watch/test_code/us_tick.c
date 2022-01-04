/********************************************************************************

*/
/*!
*  copyright Analog Devices
* ****************************************************************************
*
* License Agreement
*
* Copyright (c) 2021 Analog Devices Inc.
* All rights reserved.
*
* This source code is intended for the recipient only under the guidelines of
* the non-disclosure agreement with Analog Devices Inc.
*

* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER  LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING  FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER  DEALINGS IN THE SOFTWARE.

 ********************************************************************************
 * File Name     : us_tick.c
 * Date          : 15-06-2021
 * Description   : .Offer a micro second precision counter.
 * Version       : 1.0
*************************************************************************************************************/

#include "us_tick.h"

#include "nrf_log.h"
#include "nrf.h"
#include "nrf_drv_timer.h"
#include "Adxl362.h"
#if NRF_LOG_ENABLED
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#endif
#include <rtc.h>
#include "ad5940.h"
#include "ecg_task.h"
#include "hw_if_config.h"
#ifdef EXTERNAL_TRIGGER_EDA
#include "eda_application_task.h"
#endif

typedef enum
{
  E_ADXL_RUNNING = 0x01,
  E_ADPD_RUNNING,
  E_ADPD_ADXL_RUNNING,
  E_AD5940_RUNNING,
  E_AD5940_ADXL_RUNNING,
  E_AD5940_ADPD_RUNNING,
  E_AD5940_ADPD_ADXL_RUNNING,
}E_AD5940_ADPD_ADXL_STATUS_t;

#if defined(PROFILE_TIME_ENABLED) || defined(BLE_NUS_PROFILE_TIME_ENABLED) || defined(MEASURE_BLE_ADV_TIME)


const nrfx_timer_t TIMER_INC_MICRO_SEC = NRFX_TIMER_INSTANCE(4);

static void inc_micro_sec(nrf_timer_event_t event_type, void* p_context)
{
}
/*****************************************************************************
 * Function      : get_micro_sec
 * Description   : get micro second time
 * Input         : void
 * Output        : None
 * Return        :
 * Others        :
 * Record
 * 1.Date        : 20200720
 *   Modification: Created function

*****************************************************************************/
uint32_t get_micro_sec(void)
{
    return nrfx_timer_capture(&TIMER_INC_MICRO_SEC,NRF_TIMER_CC_CHANNEL0);
}

/*****************************************************************************
 * Function      : us_timer_init
 * Description   : micro second timer initialization
 * Input         : void
 * Output        : None
 * Return        :
 * Others        :
 * Record
 * 1.Date        : 20200720
 *   Modification: Created function

*****************************************************************************/
uint32_t us_timer_init(void)
{
    uint32_t err_code = NRF_SUCCESS;

    nrfx_timer_config_t timer_cfg  = NRFX_TIMER_DEFAULT_CONFIG;

       timer_cfg.frequency          = (nrf_timer_frequency_t)NRF_TIMER_FREQ_1MHz;
       timer_cfg.mode               =  (nrf_timer_mode_t)NRF_TIMER_MODE_TIMER;
       timer_cfg.bit_width          = (nrf_timer_bit_width_t)NRF_TIMER_BIT_WIDTH_32;
       timer_cfg.interrupt_priority = APP_IRQ_PRIORITY_LOWEST;

     err_code = nrfx_timer_init(&TIMER_INC_MICRO_SEC,&timer_cfg,inc_micro_sec);
     if(NRF_SUCCESS != err_code)
    {
        NRF_LOG_INFO("nrf_drv_timer_init failue,err_code = %d",err_code);
        return err_code;
    }
    nrfx_timer_enable(&TIMER_INC_MICRO_SEC);
    return 0;
}

void us_timer_deinit(void)
{
   nrfx_timer_uninit(&TIMER_INC_MICRO_SEC);
}

void us_timer_reset(void)
{
     // de init timer structure
      nrfx_timer_clear(&TIMER_INC_MICRO_SEC);
}
#endif

#define ADXL_EXT_TRIGGER
#ifdef ADXL_EXT_TRIGGER

#define MAX_ADXL_ODR_INDEX    5

const nrf_drv_timer_t TIMER_EXT_TRIGGER = NRF_DRV_TIMER_INSTANCE(1);

void ppg_adjust_adpd_ext_trigger(uint16_t nOdr);
extern void Ad5940FifoCallBack(void);

volatile uint32_t gTriggerTimerCnt = 0, gAdpdTimerCnt= 0, gAdxlTimerCnt = 0;
volatile uint8_t gAdxlExtTimerRunning = 0;
volatile uint8_t gAdpdRunning = 0, gAdxlRunning = 0, gTimerRunning = 0;
volatile uint8_t gResetTriggerPulseStates = 0;
volatile uint8_t gAd5940Running = 0;
volatile uint32_t gad5940TriggerTimerCnt = 0, gad5940TriggerPulseCnt = 0;
uint8_t gResetAd5940TriggerPulse = 0;

uint8_t gAdxlFreqSelected = 2;
uint8_t gsTrigTimerStartCnt = 0;

uint8_t gExtTriggerFreqIndex = 2;
uint8_t gAdpdFreqIndex = 2, gAdpdDecFactor = 1;
uint8_t gAdxlFreqIndex = 2, gAdxlDecFactor = 1;

#ifdef ENABLE_ECG_APP
uint8_t gAd5940FreqIndex = 2, gAd5940DecFactor = 1;
#endif

#ifdef EXTERNAL_TRIGGER_EDA
extern uint8_t ecg_start_req;
extern uint8_t eda_start_req;
#endif


#ifdef DEBUG_TIMER_TICKS
uint32_t gaTrigTimings[100];
uint16_t gArrIndex = 0;

uint32_t gaTrigDiff[100];
uint16_t gDiffIndex = 0;
uint32_t gTimestampCp  = 0;
#endif

#ifdef DEBUG_ECG
uint32_t timer_start=0,timer_end=0,timer_diff=0,time_gap_based_on_odr;
extern uint32_t ecgodr;
uint32_t time_stamps_deviated[50]={0};
uint32_t ts_cnt=0;
uint16_t fifo_level_timer=0;
uint16_t Fifotimer_count=0;
uint32_t ecg_tick=0,ecg_tick_prev=0,ecg_load_time_diff=0;
#endif

extern uint32_t gn_ad5940_isr_cnt;
extern uint32_t gnAd5940TimeCurVal;
extern ADI_OSAL_SEM_HANDLE   ecg_task_evt_sem;
#ifdef EXTERNAL_TRIGGER_EDA
extern ADI_OSAL_SEM_HANDLE eda_task_evt_sem;
#endif

#ifdef ECG_POLLING
struct gTrigInfo{
uint32_t gaTrigTime[100];
uint32_t gaTrigCnt[100];
uint16_t nIndex;
}TrigArr;
#endif



/*!
                              ADPD ODR: Index to Frequency(Hz) to Timer Ticks(16Mhz clk ticks) Mapping Table
  ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  | Index     |   0      |   1    |   2   |    3     |   4     |   5     |   6     |   7    |   8     |   9    |   10   |   11   |  12   |   13   |  14   |   15   |   16   |
  ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  | Freq(Hz)  |   12.5   |   25   |   50  |    100   |   200   |   250   |   300   |  400   |   500   |  600   |   700  |   800  |  900  |   1000 |  1500 |  1600  |  2000  |
  ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  | Period(ms)|    80    |   40   |   20  |    10    |    5    |    4    |   3.33  |   2.5  |    2    |  1.66  |  1.428 |  1.25  |  1.11 |    1   |0.66666| 0.625  |   0.5  |
  ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  | Pulse     |    40    |   20   |   10  |    5     |   2.5   |    2    |  1.66   |  1.25  |    1    |  0.83  |  0.714 |  0.625 | 0.555 |   0.5  |0.33333| 0.3125 |  0.25  |
  | width(ms) |          |        |       |          |         |         |         |        |         |        |        |        |       |        |       |        |        |
  ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------

            Frequency to Ticks Conversion assuming 16Mhz clk and Prescaler value of 0.
                  # Ticks = Pulse Width (ms)  * 16000(ticks per ms)
*/

#ifdef EXTERNAL_TRIGGER_EDA
#define TRIGGER_AD5940_FREQ_STEPS 17
uint16_t gaAd5940TriggerFreq[TRIGGER_AD5940_FREQ_STEPS]={4,8,12,16,30,50,100,200,250,300,400,500,600,700,800,900,1000};
uint32_t gaExtAd5940TriggerTicks[TRIGGER_AD5940_FREQ_STEPS] = {2000000,100000,666640,500000,\
                                                            266640,320000,160000,80000,\
                                                            40000,32000,26640,20000,\
                                                            13280,11360,10000,8800,8000};
#else
#define TRIGGER_FREQ_STEPS  17u
uint16_t gaExtTriggerFreq[TRIGGER_FREQ_STEPS]={12,25,50,100,200,250,300,400,500,600,700,800,900,1000,1500,1600,2000};
uint32_t gaExtTriggerTicks[TRIGGER_FREQ_STEPS] = {666667,320000,160000,80000,40000,32000,26667,20000,\
								16000,13333,11429,10000,8889,8000,5333,5000,4000};
#endif //EXTERNAL_TRIGGER_EDA

/*!
 ***********************************************************************************
 *@brief      Maps the ADPD frequency(ODR) to index in the gaExtTriggerFreq look up
              table. This is required to compare the frequency indices of other
              sensor streams running which is used to configure the timer interrupt.
 *@param      nOdr: ODR of the ADPD to which index needs to be returned
 *@return     Returns the index corresponds to the frequency/nOdr requested
 ***********************************************************************************/
uint8_t map_adpd_freq_to_index(uint16_t nOdr)
{
  int i;
  for(i=0; i<TRIGGER_FREQ_STEPS; i++)
  {
    if(gaExtTriggerFreq[i] >= nOdr)
    {
      return i;
    }
  }
  if(i==TRIGGER_FREQ_STEPS)
    return 2; /*! Wrong ODR chosen, set ODR to 50Hz */
}

/*!
 ************************************************************************************
 *@brief      Maps the ADXL frequency(ODR) index to the frequency index supported by
              the ADXL362. This is required to compare the frequency indices of other
              sensor streams running which is used to configure the timer interrupt.
 *@param      nOdrIndex: ODR index of the ADXL
 *@return     Returns the supported adxl frequency index for the ODR index requested
 ************************************************************************************/
uint8_t map_adxl_freq_to_index(uint8_t nOdrIndex)
{
  /*! Offset of +2 added because of 250,300Hz in between 200 and 400Hz in Ticks Conversion Table*/
  return(( nOdrIndex>=MAX_ADXL_ODR_INDEX)?MAX_ADXL_ODR_INDEX+2:nOdrIndex);
}

/*!
 ********************************************************************************
 *@brief      Timer event handler to handle ADPD,ADXL,AD5940 external trigger.
              Events are generated from Nordic Internal timer and the external
              trigger pins connected to ADPD,ADXL,AD5940 are driven from here
 *@param      event_type: Event type of type 'nrf_timer_event_t'
              p_context: pointer to context and is defined according to Nordic
              timer event call back
 *@return     None
 ********************************************************************************/

void ext_trigger_timer_event_handler(nrf_timer_event_t event_type, void* p_context)
{
    E_AD5940_ADPD_ADXL_STATUS_t e_ad5940_adpd_adxl_status;
    switch (event_type)
    {
        case NRF_TIMER_EVENT_COMPARE0:
            e_ad5940_adpd_adxl_status = (gAd5940Running << 2) | (gAdpdRunning << 1) | gAdxlRunning;
            switch(e_ad5940_adpd_adxl_status)
            {
              case E_ADXL_RUNNING:
                    invert_adxl_trigger_signal();
                    gAdxlTimerCnt++;
                    break;
              case E_ADPD_RUNNING:
                    invert_adpd_trigger_signal();
                    gAdpdTimerCnt++;
                    break;
              case E_ADPD_ADXL_RUNNING:
                    gTriggerTimerCnt++;
                    if(gResetTriggerPulseStates)
                    {
                      reset_trigger_pulses();
                      gResetTriggerPulseStates = 0;
                    }
                    else
                    {
                      if((gTriggerTimerCnt%gAdxlDecFactor) == 0)
                      {
                        invert_adxl_trigger_signal();
                      }
                      if((gTriggerTimerCnt%gAdpdDecFactor) == 0)
                      {
                        invert_adpd_trigger_signal();
                      }
                    }
                    break;
              case E_AD5940_RUNNING:
                    gad5940TriggerTimerCnt++;
                    if(gResetAd5940TriggerPulse)
                    {
                      gResetAd5940TriggerPulse = 0;
                      gad5940TriggerTimerCnt = 1;
                      gad5940TriggerPulseCnt = 1;
                      reset_ad5940_trigger_signal(0);  /* set the trigger signal low*/
                    }
                    else
                    {
                      if((gad5940TriggerTimerCnt%gAd5940DecFactor) == 0)
                      {
                        invert_ad5940_trigger_signal();
                        gad5940TriggerPulseCnt++;
                        if(((gad5940TriggerPulseCnt) % FIFO_THRESHOLD_TRIGGERS_CNT) == 0)
                        {
                          /* Now there should be fifo data ; number of samples to read is being set in
                          ecg app which FIFO threshold variable*/
                          /* Read current time */
                          gnAd5940TimeCurVal =  get_sensor_time_stamp();
                          /* Increment isr count */
                          gn_ad5940_isr_cnt++;
#ifdef EXTERNAL_TRIGGER_EDA
                          if(ecg_start_req== 1){
                            /*Wake up the ecg task to read the samples from FIFO*/
                            adi_osal_SemPost(ecg_task_evt_sem);
                          }
                          else if(eda_start_req== 1){
                            /*Wake up the ecg task to read the samples from FIFO*/
                            adi_osal_SemPost(eda_task_evt_sem);
                          }
#else
                          adi_osal_SemPost(ecg_task_evt_sem);
#endif
                        }
                       }
                     }
                    break;
              case E_AD5940_ADXL_RUNNING:
                    gad5940TriggerTimerCnt++;
                    gTriggerTimerCnt++;
                    if(gResetTriggerPulseStates)
                    {
                      gResetTriggerPulseStates = 0;
                      if(gResetAd5940TriggerPulse)  /*if AD5940 is started recently then only reset the pulse cnt*/
                      {
                        gResetAd5940TriggerPulse = 0;
                        gad5940TriggerTimerCnt = 1;
                        gad5940TriggerPulseCnt = 1;
                      }
                      else
                      {
                        if((gad5940TriggerPulseCnt % 2) == 0) /*If current gpio trig state is 1; increment the pulse cnt*/
                            gad5940TriggerPulseCnt++;
                      }
                      gTriggerTimerCnt = 1;
                      reset_ad5940_trigger_signal(0);  /* set the trigger signal low*/
                      reset_adxl_trigger_signal(0); /*set adxl trigger pulse low*/
                    }
                    else
                    {
                      if((gTriggerTimerCnt%gAdxlDecFactor) == 0)
                      {
                        invert_adxl_trigger_signal();
                      }
                      if((gad5940TriggerTimerCnt%gAd5940DecFactor) == 0)
                      {
                        invert_ad5940_trigger_signal();
                        gad5940TriggerPulseCnt++;
                        if(((gad5940TriggerPulseCnt) % FIFO_THRESHOLD_TRIGGERS_CNT) == 0)
                        {
                          /* Now there should be fifo data ; number of samples to read is being set in
                          ecg app which FIFO threshold variable*/
                          /* Read current time */
                          gnAd5940TimeCurVal =  get_sensor_time_stamp();
                          /* Increment isr count */
                          gn_ad5940_isr_cnt++;
                          adi_osal_SemPost(ecg_task_evt_sem);
                        }
                       }
                     }
                    break;
              case E_AD5940_ADPD_RUNNING:
                    gad5940TriggerTimerCnt++;
                    gTriggerTimerCnt++;
                    if(gResetTriggerPulseStates)
                    {
                      gResetTriggerPulseStates = 0;
                      if(gResetAd5940TriggerPulse)  /*if AD5940 is started recently then only reset the pulse cnt*/
                      {
                        gResetAd5940TriggerPulse = 0;
                        gad5940TriggerTimerCnt = 1;
                        gad5940TriggerPulseCnt = 1;
                      }
                      else
                      {
                        if((gad5940TriggerPulseCnt % 2) == 0) /*If current gpio trig state is 1; increment the pulse cnt*/
                            gad5940TriggerPulseCnt++;
                      }
                      gTriggerTimerCnt = 1;
                      reset_ad5940_trigger_signal(0);  /* set the trigger signal low*/
                      reset_adpd_trigger_signal(0); /*set adxl trigger pulse low*/
                    }
                    else
                    {
                      if((gTriggerTimerCnt%gAdpdDecFactor) == 0)
                      {
                        invert_adpd_trigger_signal();
                      }
                      if((gad5940TriggerTimerCnt%gAd5940DecFactor) == 0)
                      {
                        invert_ad5940_trigger_signal();
                        gad5940TriggerPulseCnt++;
                        if(((gad5940TriggerPulseCnt) % FIFO_THRESHOLD_TRIGGERS_CNT) == 0)
                        {
                          /* Now there should be fifo data ; number of samples to read is being set in
                          ecg app which FIFO threshold variable*/
                          /* Read current time */
                          gnAd5940TimeCurVal =  get_sensor_time_stamp();
                          /* Increment isr count */
                          gn_ad5940_isr_cnt++;
                          adi_osal_SemPost(ecg_task_evt_sem);
                        }
                       }
                     }
                    break;
              case E_AD5940_ADPD_ADXL_RUNNING:
                    gad5940TriggerTimerCnt++;
                    gTriggerTimerCnt++;
                    if(gResetTriggerPulseStates)
                    {
                      gResetTriggerPulseStates = 0;
                      if(gResetAd5940TriggerPulse)  /*if AD5940 is started recently then only reset the pulse cnt*/
                      {
                        gResetAd5940TriggerPulse = 0;
                        gad5940TriggerTimerCnt = 1;
                        gad5940TriggerPulseCnt = 1;
                      }
                      else
                      {
                        if((gad5940TriggerPulseCnt % 2) == 0) /*If current gpio trig state is 1; increment the pulse cnt*/
                            gad5940TriggerPulseCnt++;
                      }
                      gTriggerTimerCnt = 1;
                      reset_ad5940_trigger_signal(0);  /* set the trigger signal low*/
                      reset_adxl_trigger_signal(0); /*set adxl trigger pulse low*/
                      reset_adpd_trigger_signal(0); /*set adxl trigger pulse low*/
                    }
                    else
                    {
                      if((gTriggerTimerCnt%gAdxlDecFactor) == 0)
                      {
                        invert_adxl_trigger_signal();
                      }
                      if((gTriggerTimerCnt%gAdpdDecFactor) == 0)
                      {
                        invert_adpd_trigger_signal();
                      }
                      if((gad5940TriggerTimerCnt%gAd5940DecFactor) == 0)
                      {
                        invert_ad5940_trigger_signal();
                        gad5940TriggerPulseCnt++;
                        if(((gad5940TriggerPulseCnt) % FIFO_THRESHOLD_TRIGGERS_CNT) == 0)
                        {
                          /* Now there should be fifo data ; number of samples to read is being set in
                          ecg app which FIFO threshold variable*/
                          /* Read current time */
                          gnAd5940TimeCurVal =  get_sensor_time_stamp();
                          /* Increment isr count */
                          gn_ad5940_isr_cnt++;
                          adi_osal_SemPost(ecg_task_evt_sem);
                        }
                       }
                     }
                    break;
              default:
                    break;
            }
            break;

        default:
            //Do nothing.
            break;
    }
}


/*!
 ********************************************************************************
 *@brief      Enables the timer with period based on the nOdrIndex requested.
              After this timer handler get triggered based on the period
              configured and is used for triggering pulses to different sensors.
 *@param      uint8_t nOdrIndex: ODR index with which timer has to be configured
 *@return     None
 ********************************************************************************/
void  enable_trigger_timer(uint8_t nOdrIndex)
{
    uint32_t time_ticks;
    uint32_t err_code = NRF_SUCCESS;

    //nrf_drv_timer_config_t timer_cfg = NRF_DRV_TIMER_DEFAULT_CONFIG;
    nrf_drv_timer_config_t timer_cfg = {                                                                                    \
    .frequency          = (nrf_timer_frequency_t)NRFX_TIMER_DEFAULT_CONFIG_FREQUENCY,\
    .mode               = (nrf_timer_mode_t)NRFX_TIMER_DEFAULT_CONFIG_MODE,          \
    .bit_width          = (nrf_timer_bit_width_t)NRFX_TIMER_DEFAULT_CONFIG_BIT_WIDTH,\
    .interrupt_priority = APP_IRQ_PRIORITY_HIGHEST,                    \
    .p_context          = NULL                                                       \
    };
    err_code = nrf_drv_timer_init(&TIMER_EXT_TRIGGER, &timer_cfg, ext_trigger_timer_event_handler);
    APP_ERROR_CHECK(err_code);

    time_ticks = gaExtTriggerTicks[nOdrIndex];
    gExtTriggerFreqIndex = nOdrIndex;

    /*! Update the decimation factor with new timer frequency*/
    gAdpdDecFactor = gaExtTriggerFreq[nOdrIndex]/gaExtTriggerFreq[gAdpdFreqIndex];//1 << (nOdrIndex - gAdpdFreqIndex);
    gAdxlDecFactor = gaExtTriggerFreq[nOdrIndex]/gaExtTriggerFreq[gAdxlFreqIndex];//1 << (nOdrIndex - gAdxlFreqIndex);
    gAd5940DecFactor = gaExtTriggerFreq[nOdrIndex]/gaExtTriggerFreq[gAd5940FreqIndex];

    nrf_drv_timer_extended_compare(&TIMER_EXT_TRIGGER, NRF_TIMER_CC_CHANNEL0, time_ticks,\
                                  NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);
    nrf_drv_timer_enable(&TIMER_EXT_TRIGGER);
    gTimerRunning = 1;
}

/*!
 ********************************************************************************
 *@brief      Restart the timer with period based on the nOdrIndex requested.
              It stops the currently running timer and restart the timer with
              new nOdrIndex requested.
 *@param      uint8_t nOdrIndex: ODR index with which timer has to be restarted
 *@return     None
 ********************************************************************************/
void restart_trigger_timer(uint8_t nOdrIndex)
{
    uint32_t time_ticks;
    uint32_t err_code = NRF_SUCCESS;

    nrf_drv_timer_uninit(&TIMER_EXT_TRIGGER);    /*! Stop the Timer */

    //nrf_drv_timer_config_t timer_cfg = NRF_DRV_TIMER_DEFAULT_CONFIG;
    nrf_drv_timer_config_t timer_cfg = {                                                                                    \
    .frequency          = (nrf_timer_frequency_t)NRFX_TIMER_DEFAULT_CONFIG_FREQUENCY,\
    .mode               = (nrf_timer_mode_t)NRFX_TIMER_DEFAULT_CONFIG_MODE,          \
    .bit_width          = (nrf_timer_bit_width_t)NRFX_TIMER_DEFAULT_CONFIG_BIT_WIDTH,\
    .interrupt_priority = APP_IRQ_PRIORITY_HIGHEST,                    \
    .p_context          = NULL                                                       \
    };
    err_code = nrf_drv_timer_init(&TIMER_EXT_TRIGGER, &timer_cfg, ext_trigger_timer_event_handler);
    APP_ERROR_CHECK(err_code);

    time_ticks = gaExtTriggerTicks[nOdrIndex];    /*! Restart the timer with new frequency */
    gExtTriggerFreqIndex = nOdrIndex;

    /*! Update the decimation factor with new timer frequency*/
    gAdpdDecFactor = gaExtTriggerFreq[nOdrIndex]/gaExtTriggerFreq[gAdpdFreqIndex];//1 << (nOdrIndex - gAdpdFreqIndex);
    gAdxlDecFactor = gaExtTriggerFreq[nOdrIndex]/gaExtTriggerFreq[gAdxlFreqIndex];//1 << (nOdrIndex - gAdxlFreqIndex);
    gAd5940DecFactor = gaExtTriggerFreq[nOdrIndex]/gaExtTriggerFreq[gAd5940FreqIndex];
    nrf_drv_timer_extended_compare(
         &TIMER_EXT_TRIGGER, NRF_TIMER_CC_CHANNEL0, time_ticks, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);
    nrf_drv_timer_enable(&TIMER_EXT_TRIGGER);
}

/*!
 ********************************************************************************
 *@brief      Disables the timer with period based on the nOdrIndex requested.
              If there are other streams running, it configures the timer with
              minimum highest frequency required to keep other streams running.
              If no stream requires timer running, it completely stops the timer
 *@param      uint8_t nOdrIndex: ODR index with which timer has to be disabled
 *@return     None
 ********************************************************************************/

void disable_trigger_timer(uint8_t nOdrIndex)
{
  if(gsTrigTimerStartCnt == 1)
  {
    gsTrigTimerStartCnt = 0;
    nrf_drv_timer_uninit(&TIMER_EXT_TRIGGER);  /*! stop timer if only one stream is running*/
    /*! Reset all variables to default values*/
    gTriggerTimerCnt = 0;
    gTimerRunning = 0;
    gad5940TriggerTimerCnt = 0;
    gAdxlFreqIndex = 2;
    gAdpdFreqIndex = 2;
    gAd5940FreqIndex = 2;
    gExtTriggerFreqIndex = 2;
    gAdpdDecFactor = 1;
    gAdxlDecFactor = 1;
    gAd5940DecFactor = 1;
  }
  else
  {
    gsTrigTimerStartCnt--;
    /*! If other stream runs at lower freqeuncy; then restart the timer with lower frequency required*/
    if(nOdrIndex == gExtTriggerFreqIndex)
    {
      E_AD5940_ADPD_ADXL_STATUS_t e_ad5940_adpd_adxl_status;
      e_ad5940_adpd_adxl_status = (gAd5940Running << 2) | (gAdpdRunning << 1) | gAdxlRunning;
      switch(e_ad5940_adpd_adxl_status)
      {
        case E_ADPD_ADXL_RUNNING:
                if(nOdrIndex == gAdpdFreqIndex)
                {
                   restart_trigger_timer(gAdxlFreqIndex);
                   gAdxlDecFactor = 1;
                }
                else
                {
                   restart_trigger_timer(gAdpdFreqIndex);
                   gAdpdDecFactor = 1;
                }
              break;
        case E_AD5940_ADXL_RUNNING:
                if(nOdrIndex == gAd5940FreqIndex)
                {
                   restart_trigger_timer(gAdxlFreqIndex);
                   gAdxlDecFactor = 1;
                }
                else
                {
                   restart_trigger_timer(gAd5940FreqIndex);
                   gAd5940DecFactor = 1;
                }
              break;
        case E_AD5940_ADPD_RUNNING:
                if(nOdrIndex == gAd5940FreqIndex)
                {
                   restart_trigger_timer(gAdpdFreqIndex);
                   gAdpdDecFactor = 1;
                }
                else
                {
                   restart_trigger_timer(gAd5940FreqIndex);
                   gAd5940DecFactor = 1;
                }
              break;
        case E_AD5940_ADPD_ADXL_RUNNING:
                if(nOdrIndex == gAd5940FreqIndex)
                {
                  if(gAdpdFreqIndex > gAdxlFreqIndex)
                  {
                     restart_trigger_timer(gAdpdFreqIndex);
                     gAdpdDecFactor = 1;
                  }
                  else
                  {
                     restart_trigger_timer(gAdxlFreqIndex);
                     gAdxlDecFactor = 1;
                  }
                }
                else if(nOdrIndex == gAdpdFreqIndex)
                {
                  if(gAd5940FreqIndex > gAdxlFreqIndex)
                  {
                     restart_trigger_timer(gAd5940FreqIndex);
                     gAd5940DecFactor = 1;
                  }
                  else
                  {
                     restart_trigger_timer(gAdxlFreqIndex);
                     gAdxlDecFactor = 1;
                  }
                }
                else if(nOdrIndex == gAdxlFreqIndex)
                {
                  if(gAd5940FreqIndex > gAdpdFreqIndex)
                  {
                     restart_trigger_timer(gAd5940FreqIndex);
                     gAd5940DecFactor = 1;
                  }
                  else
                  {
                     restart_trigger_timer(gAdpdFreqIndex);
                     gAdpdDecFactor = 1;
                  }
                }
              break;
        default:
              break;
      }
    }
  }
}

/*!
 ********************************************************************************
 *@brief      Enables triggering ADXL sensor with ODR based on nOdrIndex.
              If timer is not running, it starts timer with ODR requested.
              If timer is running at higher frequency, it adjusts the decimation
              factor for adxl stream, otherwise it restart the timer with new
              frequency requested and adjust other stream's decimation factor.
              It also configures the GPIO pin required to trigger the sensor.
 *@param      uint8_t nOdrIndex: ODR index with which ADXL has to be triggered
 *@return     None
 ********************************************************************************/
void enable_adxl_ext_trigger(uint8_t nOdrIndex)
{
    uint32_t time_ticks;
    uint32_t err_code = NRF_SUCCESS;
    enable_adxl_trigger_pin();
    nOdrIndex = map_adxl_freq_to_index(nOdrIndex);
    if((nOdrIndex > gExtTriggerFreqIndex) || (!gTimerRunning))
    {
      //restart timer; adjust the dec factors
      gAdxlFreqIndex = nOdrIndex;
      if(gTimerRunning)
        restart_trigger_timer(nOdrIndex);
      else
        enable_trigger_timer(nOdrIndex);

    }
    else
    {
      gAdxlFreqIndex = nOdrIndex;
      gAdxlDecFactor = gaExtTriggerFreq[gExtTriggerFreqIndex]/gaExtTriggerFreq[nOdrIndex];//1 << (nOdrIndex - gAdxlFreqIndex);
    }
    gsTrigTimerStartCnt++;
    gAdxlExtTimerRunning = 1;
    gResetTriggerPulseStates = 1;
    gAdxlRunning = 1;
}

/*!
 ********************************************************************************
 *@brief      Disables triggering ADXL sensor with frequency based on nOdrIndex.
              nOdr parameter used in stopping/restarting the timer with new
              configurations depending upon other sensors enabled.
              It also disables the GPIO pin required to trigger the sensor.
 *@param      uint8_t nOdrIndex: ODR index with which ADXL has to be disabled
 *@return     None
 ********************************************************************************/
void disable_adxl_ext_trigger(uint8_t nOdrIndex)
{
    nOdrIndex = map_adxl_freq_to_index(nOdrIndex);
    disable_trigger_timer(nOdrIndex);
    gAdxlRunning = 0;
    disable_adxl_trigger_pin();
    gAdxlExtTimerRunning = 0;
}


/*!
 ********************************************************************************
 *@brief      Configures teh ADXL trigger frequency with nFreq.
              It disables the adxl with older trigger frequency and restarts
              with new frequency nFreq.
 *@param      uint8_t nFreq: Frequency with which ADXL has to be triggered
 *@return     None
 ********************************************************************************/
void set_adxl_trigger_freq(uint8_t nFreq)
{
  nFreq = map_adxl_freq_to_index(nFreq);
  gAdxlFreqSelected = nFreq;
  if(gAdxlRunning && (nFreq != gAdxlFreqIndex))
  {
    /*
      If Timer is currently running, first stop the timer.
      Then restart the timer with new Frequency requested
    */
    disable_adxl_ext_trigger(gAdxlFreqIndex);
//    AdxlDrvExtSampleMode(PRM_EXT_SAMPLE_DISABLE);
//    AdxlDrvSetOperationMode(PRM_MEASURE_STANDBY_MODE);
//
//    AdxlDrvSetOperationMode(PRM_MEASURE_MEASURE_MODE);
//    AdxlDrvExtSampleMode(PRM_EXT_SAMPLE_ENABLE);
    gAdxlFreqIndex = nFreq;
    enable_adxl_ext_trigger(nFreq);
  }
}


/*!
 ********************************************************************************
 *@brief      Enables triggering ADPD sensor with frequency based on nOdr.
              If timer is not running, it starts timer with ODR requested.
              If timer is running at higher frequency, it adjusts the decimation
              factor for adpd stream, otherwise it restart the timer with new
              nOdr requested and adjust other stream's decimation factor.
              It also configures the GPIO pin required to trigger the sensor.
 *@param      uint8_t nOdr: ODR with which ADPD has to be triggered
 *@return     None
 ********************************************************************************/
void enable_adpd_ext_trigger(uint16_t nOdr)
{
    uint8_t nOdrIndex;
    enable_adpd_trigger_pin();
    nOdrIndex = map_adpd_freq_to_index(nOdr);
    if((nOdrIndex > gExtTriggerFreqIndex) || (!gTimerRunning))
    {
      //restart timer; adjust the dec factors
      gAdpdFreqIndex = nOdrIndex;
      if(gTimerRunning)
        restart_trigger_timer(nOdrIndex);
      else
        enable_trigger_timer(nOdrIndex);
    }
    else
    {
      gAdpdFreqIndex = nOdrIndex;
      gAdpdDecFactor = gaExtTriggerFreq[gExtTriggerFreqIndex]/gaExtTriggerFreq[nOdrIndex];//1 << (nOdrIndex - gAdpdFreqIndex);
    }
    gsTrigTimerStartCnt++;
    gResetTriggerPulseStates = 1;
    gAdpdRunning = 1;
}

/*!
 ********************************************************************************
 *@brief      Function to update the ADPD/ADXL trigger frequency at run time for
              PPG application. This will be called from Library through callback
              PpgAdpd400xSetModeCB().
 *@param      uint8_t nOdr: ODR with which ADPD has to be triggered
 *@return     None
 ********************************************************************************/
void ppg_adjust_adpd_ext_trigger(uint16_t nOdr)
{
    uint8_t nOdrIndex;
   if(gAdpdRunning) {
      nOdrIndex = map_adpd_freq_to_index(nOdr);
      gAdpdFreqIndex = nOdrIndex;
      if(nOdrIndex > gExtTriggerFreqIndex)
      {
        //restart timer; adjust the dec factors
        restart_trigger_timer(nOdrIndex);
      }
      else
      {
        gAdpdDecFactor = gaExtTriggerFreq[gExtTriggerFreqIndex]/gaExtTriggerFreq[nOdrIndex];
        gAdxlDecFactor = gaExtTriggerFreq[gExtTriggerFreqIndex]/gaExtTriggerFreq[gAdxlFreqIndex];
        gAd5940DecFactor = gaExtTriggerFreq[gExtTriggerFreqIndex]/gaExtTriggerFreq[gAd5940FreqIndex];
      }
   }
}

/*!
 ********************************************************************************
 *@brief      Disables triggering ADPD sensor with frequency based on nOdr.
              nOdr parameter used in stopping/restarting the timer with new
              configurations depending upon other sensors enabled.
              It also disables the GPIO pin required to trigger the sensor.
 *@param      uint8_t nOdr: ODR with which ADPD has to be disabled
 *@return     None
 ********************************************************************************/
void disable_adpd_ext_trigger(uint16_t nOdr)
{
    uint8_t nOdrIndex;
    nOdrIndex = map_adpd_freq_to_index(nOdr);
    disable_trigger_timer(nOdrIndex);
    gAdpdRunning = 0;
    disable_adpd_trigger_pin();
}
#endif //ADXL_EXT_TRIGGER

#ifdef ENABLE_ECG_APP
/*!
 ********************************************************************************
 *@brief      Enables triggering AD5940 sensor with frequency based on nOdr.
              If timer is not running, it starts timer with ODR requested.
              If timer is running at higher frequency, it adjusts the decimation
              factor for AD5940 stream, otherwise it restart the timer with new
              nOdr requested and adjust other stream's decimation factor.
              It also configures the GPIO pin required to trigger the sensor.
 *@param      uint8_t nOdr: ODR with which AD5940 has to be triggered
 *@return     None
 ********************************************************************************/
void enable_ad5940_ext_trigger(uint16_t nOdr)
{
    uint8_t nOdrIndex;
    enable_ad5940_trigger_pin();
    reset_ad5940_trigger_signal(0); /* set the trigger signal low*/
    nOdrIndex = map_adpd_freq_to_index(nOdr);
#ifdef EXTERNAL_TRIGGER_EDA
    NRF_LOG_INFO("ODR=%d,ODR Index=%d",nOdr,nOdrIndex);
    if(ecg_start_req== 1){
      if((nOdrIndex == 0)||(nOdrIndex == 1)){
        nOdrIndex = ((nOdrIndex == 0)? (nOdrIndex+2):(nOdrIndex+1));
       }
     }
     else if(eda_start_req == 1){
        if(nOdrIndex > 4){
          return; /* eda doesnt support greater than 30 Hz*/
        }
     }
#endif
    if((nOdrIndex > gExtTriggerFreqIndex) || (!gTimerRunning))
    {
      //restart timer; adjust the dec factors
      gAd5940FreqIndex = nOdrIndex;
      if(gTimerRunning)
        restart_trigger_timer(nOdrIndex);
      else
        enable_trigger_timer(nOdrIndex);
    }
    else
    {
      gAd5940FreqIndex = nOdrIndex;
      gAd5940DecFactor = gaExtTriggerFreq[gExtTriggerFreqIndex]/gaExtTriggerFreq[nOdrIndex];//1 << (nOdrIndex - gAdpdFreqIndex);
    }
    gsTrigTimerStartCnt++;
    gResetTriggerPulseStates = 1;
    gResetAd5940TriggerPulse = 1;
    gAd5940Running = 1;
}

/*!
 ********************************************************************************
 *@brief      Disables triggering AD5940 sensor with frequency based on nOdr.
              nOdr parameter used in stopping/restarting the timer with new
              configurations depending upon other sensors enabled.
              It also disables the GPIO pin required to trigger the sensor.
 *@param      uint8_t nOdr: ODR with which AD5940 has to be disabled
 *@return     None
 ********************************************************************************/
void disable_ad5940_ext_trigger(uint16_t nOdr)
{
    uint8_t nOdrIndex;
    nOdrIndex = map_adpd_freq_to_index(nOdr);
    disable_trigger_timer(nOdrIndex);
    gAd5940Running = 0;
    disable_ad5940_trigger_pin();
}

#endif //ENABLE_ECG_APP