/**
    ***************************************************************************
    * @file         AppSync.c
    * @author       ADI
    * @version      V1.0.0
    * @date         Oct-2015
    * @brief        HW sync layer
    *
    ***************************************************************************
    * @attention
    ***************************************************************************
*/
/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2019 Analog Devices Inc.                                      *
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
* This software is intended for use with the ADPD142 and derivative parts     *
* only                                                                        *
*                                                                             *
******************************************************************************/
/* Includes -----------------------------------------------------------------*/
#include <stdint.h>
#include <math.h>
#include "adxl362.h"
#include "adxl_dcfg.h"
#include "adxl_task.h"
#include "adpd400x_drv.h"
#include "app_sync.h"
#include "hw_if_config.h"
#include "app_common.h"
#include "printf.h"
#include "adpd400x_lib_common.h"
#include <adpd4000_task.h>
#include "sync_data_buffering.h"
#include "mw_ppg.h"
#include "us_tick.h"
#include "nrf_log.h"
#include "app_timer.h"
#include "adi_adpd_ssm.h"
#include "adi_adpd_m2m2.h"
#define HW_SYNC_MAX_WATERMARK   (16)

#define PPG_CHANNELS 2
#define ACC_CHANNELS 3
#define ADPD_TS_DATA_TYPE uint32_t
#define ADXL_TS_DATA_TYPE uint32_t
#define MODULE      ("AppSync.c")

#define APP_SYNC_INTERVAL            20 /* 20m sec. Interval */
/* -------------------------Public variables ---------------------------------*/

extern tAdiAdpdAppState oAppState;
#ifdef ENABLE_PPG_APP
extern volatile uint8_t gn_uc_hr_enable;
extern volatile uint16_t gnAdxlODR; //ADXL ODR
extern uint16_t gPrevSamplerate;
extern g_state_adxl_t  g_state_adxl;
volatile uint8_t gnAppSyncTimerStarted = 0;
extern volatile uint8_t gsOneTimeValueWhenReadAdxlData;
extern volatile uint8_t gsOneTimeValueWhenReadAdpdData;
extern uint32_t  Ppg_Slot;
extern uint16_t gn_uc_hr_slot;
extern tAdiAdpdSSmInst goAdiAdpdSSmInst;
/* -------------------------Public function prototype ------------------------*/
void DisplaySyncMode(SynchMode_t eSyncMode);

void SyncInit();
void SyncDeInit();
SyncErrorStatus DoSync(uint32_t *pPpgData, uint32_t nAdpdTs, uint16_t *pnAccelData, uint32_t nAccelTs);
uint32_t *GetSyncAdpdData();
int16_t *GetSyncAccelData();
uint16_t *GetSyncRawAccelData();
uint32_t GetSyncAdpdTs();
uint32_t GetSyncAccelTs();
uint32_t GetSyncRawAccelTs();
void HwSyncInit();
void HwSyncDeInit();

SyncErrorStatus HwSyncDoSync(uint32_t *pPpgData, uint32_t nAdpdTs, uint16_t *pnAccelData, uint32_t nAccelTs);
uint32_t *HwSyncGetSyncAdpdData();
int16_t *HwSyncGetSyncAccelData();
uint16_t *HwSyncGetSyncRawAccelData();
uint32_t HwSyncGetSyncAdpdTs();
uint32_t HwSyncGetSyncAccelTs();
uint32_t HwSyncGetSyncRawAccelTs();

APP_TIMER_DEF(m_app_sync_timer_id);     /**< Handler for repeated timer for app sync buffering. */

static void app_sync_timeout_handler(void * p_context);
static void app_sync_timer_start(uint8_t timer_interval);
static void app_sync_timer_stop(void);

extern void event_from_sync(void);

typedef enum {
    SYNC_SKIP = 0,
    SYNC_INIT,
    BUFFER_DATA,
    SYNC_DATA,
    ERR_SYNC
} SyncStates;

typedef struct {
    SyncStates eSyncState;
} SyncParameters;

extern int16_t g_accelExists;

static SynchMode_t geSyncMode = ADPD_ODCS_TRIGGERS;

static volatile uint16_t gnAdxlSYNC_BUFFERING_get = 0, gnAdpdSYNC_BUFFERING_get = 0;
static volatile uint16_t gnAdxlSYNC_BUFFERING_put = 0, gnAdpdSYNC_BUFFERING_put = 0;
static volatile uint8_t gsOneAdpdDataSetRdy = 0, gsOneAdxlDataSetRdy = 0;

void DisplaySyncMode(SynchMode_t eSyncMode) {
    switch (eSyncMode){
    case ADPD_ODCS_TRIGGERS:
        debug(MODULE," ************** HW Sync -- ADPD Output Data Cycle Signal triggers ADXL \r\n");
        break;
    case NO_HW_SYNCH:
    default:
        debug(MODULE," ************** SW SYNC \r\n");
        eSyncMode = NO_HW_SYNCH;
        break;
    }
}

void SyncInit() {
    switch(geSyncMode) {
    case ADPD_ODCS_TRIGGERS:
        HwSyncInit();
        break;
    default:
        break;
    }

}

void SyncDeInit() {

    switch(geSyncMode) {
    case ADPD_ODCS_TRIGGERS:
        if(gn_uc_hr_enable)
          app_sync_timer_stop();
        HwSyncDeInit();
        break;
    default:
        break;
    }
}

SyncErrorStatus DoSync(uint32_t *pPpgData, uint32_t nAdpdTs, uint16_t *pnAccelData, uint32_t nAccelTs) {
    SyncErrorStatus nStatus;

    nStatus = HwSyncDoSync(pPpgData, nAdpdTs,
                      pnAccelData, nAccelTs);

    return nStatus;
}

uint32_t *GetSyncAdpdData() {
    return HwSyncGetSyncAdpdData();
}

int16_t *GetSyncAccelData() {
    return HwSyncGetSyncAccelData();
}

uint16_t *GetSyncRawAccelData() {
    return HwSyncGetSyncRawAccelData();
}

uint32_t GetSyncAdpdTs() {
    return HwSyncGetSyncAdpdTs();
}

uint32_t GetSyncAccelTs() {
    return HwSyncGetSyncAccelTs();
}

uint32_t GetSyncRawAccelTs() {
    return HwSyncGetSyncRawAccelTs();
}
/******************************************************************************/

typedef struct  AdpdDataBufStruct {
	uint32_t nDataValue[PPG_CHANNELS];
	uint32_t nTimeStamp;
    struct AdpdDataBufStruct* Next;
} AdpdDataBuf;

typedef struct  AdxlDataBufStruct {
	int16_t nDataValue[ACC_CHANNELS];
	uint32_t nTimeStamp;
    struct AdxlDataBufStruct* Next;
} AdxlDataBuf;

typedef struct  AdxlRawDataBufStruct {
	uint16_t nDataValue[ACC_CHANNELS];
	uint32_t nTimeStamp;
} AdxlRawDataBuf;

static AdpdDataBuf oHwSyncAdpdData;
static AdxlDataBuf oHwSyncAdxlData;
static AdxlRawDataBuf oHwSyncAdxlRawData;


static SyncParameters oHwSync = {SYNC_SKIP};

void SyncClearDataBuffer(){
  SyncDataClear(8);
}
#endif

  /*! Enables the external sync mode on GPIO3 pin*/
void enable_ext_syncmode()
{
  uint16_t nRegVal;
  adi_adpddrv_RegRead(0x0026,&nRegVal);
  nRegVal = nRegVal | 0x0007;
  adi_adpddrv_RegWrite(0x0026, nRegVal);  /*!Enable ext-sync mode with GPIO3 pin*/
  adi_adpddrv_RegRead(0x0022,&nRegVal);
  nRegVal = nRegVal | 0x0200;
  adi_adpddrv_RegWrite(0x0022, nRegVal);  /*! Configure GPIO3 as input pin */
}

  /*! Disables the external sync mode on GPIO3 pin*/
void disable_ext_syncmode()
{
  uint16_t nRegVal;
  adi_adpddrv_RegRead(0x0026,&nRegVal);
  nRegVal = nRegVal & 0xFFF8;
  adi_adpddrv_RegWrite(0x0026, nRegVal);   /*! Disable ext sync mode*/
  adi_adpddrv_RegRead(0x0022,&nRegVal);
  nRegVal = nRegVal & 0xF1FF;             /*!Clear GPIO_PIN_CFG3 of GPIO_CFG ([11:9] of 0x0022)*/
  adi_adpddrv_RegWrite(0x0022, nRegVal);   /*! Tristate the GPIO3 input pin */
}

uint16_t g_adpd_odr =0, g_adxl_odr=0;
uint16_t get_adpd_odr(void)
{
    uint16_t temp16;
    //uint16_t dec_reg_data;
    uint32_t sampleFrq, lfOSC;
    uint16_t adpd_odr;
    adi_adpddrv_RegRead(ADPD400x_REG_SYS_CTL, &temp16);
    temp16 &= BITM_SYS_CTL_LFOSC_SEL;
    temp16 >>= BITP_SYS_CTL_LFOSC_SEL;
    if (temp16 == 1)
      lfOSC = 1000000;  // 1M clock
    else
      lfOSC = 32000;    // 32k clock
    adi_adpddrv_RegRead32B(ADPD400x_REG_TS_FREQ, &sampleFrq);
    adpd_odr = (uint16_t) (lfOSC / sampleFrq);

//    adi_adpddrv_RegRead(0x01B2, &dec_reg_data);    // TODO: hardcoded for slot F, need to make it generic
//    adpd_odr = (sampleFrq/(dec_reg_data+1));
    return adpd_odr;
}

#ifdef ENABLE_PPG_APP
void HwSyncInit(){
  uint16_t nFifoWatermark, samplingRate, decimateVal;

  if(!gn_uc_hr_enable)
  {
    nFifoWatermark = 1;
    uint8_t slot = (uint8_t)log2(Ppg_Slot);

    GetAdpdClOutputRate(&samplingRate, &decimateVal, Ppg_Slot);
    gPrevSamplerate = samplingRate;// Initial sample rate for PPG
  adi_adpdssm_SetParameter(ADPD400x_WATERMARKING, 0, nFifoWatermark);
    /***************************************************************************
     1. GPIO0 set as Inverted O/P, becuase the Host platform configured for
        falling edge IRQ Bit[2:0] as '3'
     2. GPIO1 set as Inverted O/P to trigger the ADXL for Sync. Bit[5:3] as '3'
    ***************************************************************************/
#ifdef ADPD_TRG_ADXL
    adi_adpddrv_RegWrite(0x0022, 0x001B);
#else
/*! Disable gpio2(to avoid conflicts with triggers from MCU) and enable only gpio1 as output*/
    adi_adpddrv_RegWrite(0x0022, 0x0003);
#endif
    /***************************************************************************
      1. Map GPIO0 for INTX. Bit[7:0] as '2'
      2. Map GPIO1 for Data Cycle. Bit[15:8] as '5f' this may be get vary
    ***************************************************************************/
#ifdef ADPD_TRG_ADXL
#ifdef SLOT_SELECT
    uint16_t regVal;
    uint32_t slotVal = 0;
    if(MwPPG_ReadLCFG(1, &slotVal) == PPG_SUCCESS)
    {
    regVal = (0x5002 | (((int)log2(slotVal)) << 8));   //get the register value for 0x0023 reg. based on the slot
    }
    adi_adpddrv_RegWrite(0x0023,regVal); // Output data cycle occured
#else
    adi_adpddrv_RegWrite(0x0023,0x5502); // Output data cycle occured
#endif
#else
    adi_adpddrv_RegWrite(0x0023,0x0002); // gpio0 as interrupt out pin
    
#endif
  adi_adpdssm_slotSetup(slot, 1, 0x04 , 3);
#ifdef SLOT_SELECT
    uint8_t ADPDODR = 50; /* 50Hz */
#else
    uint8_t ADPDODR = (uint8_t)(samplingRate/decimateVal);
#endif
    g_accelExists = AdxlDrvOpenDriver(ADPDODR, nFifoWatermark);
#ifndef ADPD_TRG_ADXL
    enable_ext_syncmode();
    if (oAppState.nNumberOfStart  == 0)
    {
      /*Enable ADPD device triggering only if it is not enabled by any other streams*/
      g_adpd_odr = get_adpd_odr();
      //enable_ext_syncmode();
      enable_adpd_ext_trigger(g_adpd_odr);
    }
    uint8_t adxl_odr;
    adxl_odr = (GetFilterAdxl362() & 0x07);
    enable_adxl_ext_trigger(adxl_odr);
#endif
    AdxlDrvExtSampleMode(PRM_EXT_SAMPLE_ENABLE);
  }//if(!gn_uc_hr_enable)
  else
  {
    gnAdxlSYNC_BUFFERING_get = 0, gnAdpdSYNC_BUFFERING_get = 0;
    gnAdxlSYNC_BUFFERING_put = 0, gnAdpdSYNC_BUFFERING_put = 0;
    gsOneAdpdDataSetRdy = 0, gsOneAdxlDataSetRdy = 0;
  }
  oHwSync.eSyncState = SYNC_INIT;
  //oHwSync.eSyncState = SYNC_SKIP; //To skip S/w Sync Buffering

  SyncDataBufferInit(8);
}

void HwSyncDeInit(){
  adi_adpddrv_RegWrite(0x0023,0x0302); // Enable the INTX & INTY
  if(!gn_uc_hr_enable)
  {
#ifndef ADPD_TRG_ADXL
    if(1 == oAppState.nNumberOfStart)
    {
      adi_adpddrv_RegWrite(0x0022, 0x0003); // Set GPIO0 as INTX
      disable_ext_syncmode();
      g_adpd_odr = get_adpd_odr();
      disable_adpd_ext_trigger(g_adpd_odr);
    }
    uint8_t adxl_odr;
    adxl_odr = (GetFilterAdxl362() & 0x07);
    disable_adxl_ext_trigger(adxl_odr);
#endif
    AdxlDrvExtSampleMode(PRM_EXT_SAMPLE_DISABLE);
    GPIO_IRQ_ADPD_Enable();
  }
  oHwSync.eSyncState = SYNC_SKIP;
  memset(&oHwSyncAdxlData, 0, sizeof(AdxlDataBuf));
}

SyncErrorStatus HwSyncDoSync(uint32_t *pPpgData, uint32_t nAdpdTs, uint16_t *pnAccelData, uint32_t nAccelTs) {
CIRC_BUFF_STATUS_t  status = CIRC_BUFF_STATUS_ERROR;
    if (oHwSync.eSyncState == SYNC_SKIP) {
      if (pnAccelData != NULL) {
          //for (int16_t nIndex = 0; nIndex < ACC_CHANNELS; nIndex++) {
              //oHwSyncAdxlData.nDataValue[nIndex] = pnAccelData[nIndex];
          //}
          //oHwSyncAdxlData.nTimeStamp = nAccelTs;
          oHwSyncAdxlRawData.nDataValue[0] = pnAccelData[0];
          oHwSyncAdxlRawData.nDataValue[1] = pnAccelData[1];
          oHwSyncAdxlRawData.nDataValue[2] = pnAccelData[2];
          oHwSyncAdxlRawData.nTimeStamp = nAccelTs;
          oHwSyncAdxlData.nDataValue[0] = (oHwSyncAdxlRawData.nDataValue[0] & 0x3FFF) | ((oHwSyncAdxlRawData.nDataValue[0] & 0x3000) << 2);
          oHwSyncAdxlData.nDataValue[1] = (oHwSyncAdxlRawData.nDataValue[1] & 0x3FFF) | ((oHwSyncAdxlRawData.nDataValue[1] & 0x3000) << 2);
          oHwSyncAdxlData.nDataValue[2] = (oHwSyncAdxlRawData.nDataValue[2] & 0x3FFF) | ((oHwSyncAdxlRawData.nDataValue[2] & 0x3000) << 2);
          oHwSyncAdxlData.nTimeStamp = oHwSyncAdxlRawData.nTimeStamp;
      }
      if (pPpgData != NULL) {
          for (int16_t nIndex = 0; nIndex < PPG_CHANNELS; nIndex++) {
              oHwSyncAdpdData.nDataValue[nIndex] = pPpgData[nIndex];
          }
          oHwSyncAdpdData.nTimeStamp = nAdpdTs;
          return SYNC_SUCCESS;
      }
      return SYNC_ERROR;
    } else {
      if (pPpgData != NULL) {
           /*Copy Adpd Data to index 0*/
           status = sync_adpd_buff_put(pPpgData, &nAdpdTs);
           if (status != CIRC_BUFF_STATUS_OK) {
             gnAdpdSYNC_BUFFERING_put++;
             return SYNC_BUFFERING;
           }
           if(!gsOneAdpdDataSetRdy){
              gsOneAdpdDataSetRdy = 1;
              if(g_state_adxl.num_starts == 0){// This is the case for ADPD without ADXL in UCHR, zero adxl data will be passed for HRM 
                 gsOneTimeValueWhenReadAdxlData = 1;
              }
           }
      }

      if (pnAccelData != NULL) {
           /*Copy Adpd Data to index 0*/
           status = sync_adxl_buff_put(pnAccelData, &nAccelTs);
           if (status != CIRC_BUFF_STATUS_OK) {
             gnAdxlSYNC_BUFFERING_put++;
             return SYNC_BUFFERING;
           }
           if(!gsOneAdxlDataSetRdy)
            gsOneAdxlDataSetRdy = 1;
      }

      if(!gn_uc_hr_enable)
      {
        status = sync_adpd_buff_get(oHwSyncAdpdData.nDataValue, &oHwSyncAdpdData.nTimeStamp);
        if (status != CIRC_BUFF_STATUS_OK) {
          return SYNC_BUFFERING;
        }
        status = sync_adxl_buff_get(oHwSyncAdxlRawData.nDataValue, &oHwSyncAdxlRawData.nTimeStamp);
        if (status != CIRC_BUFF_STATUS_OK) {
          return SYNC_BUFFERING;
        }
        int16_t pnData[3] = {oHwSyncAdxlRawData.nDataValue[0], oHwSyncAdxlRawData.nDataValue[1], oHwSyncAdxlRawData.nDataValue[2]};
        oHwSyncAdxlData.nDataValue[0] = (pnData[0] & 0x3FFF) | ((pnData[0] & 0x3000) << 2);
        oHwSyncAdxlData.nDataValue[1] = (pnData[1] & 0x3FFF) | ((pnData[1] & 0x3000) << 2);
        oHwSyncAdxlData.nDataValue[2] = (pnData[2] & 0x3FFF) | ((pnData[2] & 0x3000) << 2);
        oHwSyncAdxlData.nTimeStamp = oHwSyncAdxlRawData.nTimeStamp;

        return SYNC_SUCCESS;
      }//if(!gn_uc_hr_enable)
    }

    if(gn_uc_hr_enable && gsOneAdpdDataSetRdy && gsOneAdxlDataSetRdy &&
       gsOneTimeValueWhenReadAdpdData && gsOneTimeValueWhenReadAdxlData && !gnAppSyncTimerStarted)
    {
      static volatile uint16_t timer_interval;
      timer_interval = app_sync_timer_interval();
      //timer_interval = APP_SYNC_INTERVAL;
      if(timer_interval != 0)
      {
        app_sync_timer_stop();
        app_sync_timer_start(timer_interval);
        gnAppSyncTimerStarted = 1;
        gsOneAdpdDataSetRdy = 0;
        gsOneAdxlDataSetRdy = 0;
      }
    }

    return SYNC_BUFFERING;
}

uint8_t app_sync_timer_interval()
{
  uint8_t timer_interval;
  uint8_t slot = gn_uc_hr_slot - 1;
  if(goAdiAdpdSSmInst.oAdpdSlotInst.aSlotInfo[slot].nOutputDataRate == 0)
    return 0;
  //Make timer run at lower ODR
  /* ADPD ODR < ADXL ODR */
  if(goAdiAdpdSSmInst.oAdpdSlotInst.aSlotInfo[slot].nOutputDataRate < gnAdxlODR){
    timer_interval = (1000/goAdiAdpdSSmInst.oAdpdSlotInst.aSlotInfo[slot].nOutputDataRate); //interval in ms
  }else{
    if(gnAdxlODR == 0){ // if ADPD alone running for UCHR ,ADXL ODR will be zero here.In that case using ADPD ODR.
      timer_interval = (1000/goAdiAdpdSSmInst.oAdpdSlotInst.aSlotInfo[slot].nOutputDataRate); //interval in ms
    }else{
      timer_interval = (1000/gnAdxlODR); //interval in ms
    }
  }
  NRF_LOG_INFO("App Sync Timer:%d ms", timer_interval);
  return timer_interval;
}


/**@brief Function for the Timer initialization.
*
* @details Initializes the timer module. This creates and starts application timers.
*
* @param[in]  None
*
* @return     None
*/
void app_sync_timer_init(void)
{
    ret_code_t err_code;

    /*! Create timers */
    err_code =  app_timer_create(&m_app_sync_timer_id, APP_TIMER_MODE_REPEATED, app_sync_timeout_handler);

    APP_ERROR_CHECK(err_code);
}

/**@brief   Function for starting application timers.
* @details Timers are run after the scheduler has started.
*
* @param[in]  None
*
* @return     None
*/
static void app_sync_timer_start(uint8_t timer_interval)
{
    /*! Start repeated timer */
    ret_code_t err_code = app_timer_start(m_app_sync_timer_id, APP_TIMER_TICKS(timer_interval), NULL);
    APP_ERROR_CHECK(err_code);
}

/**@brief   Function for stopping the application timers.
*
* @param[in]  None
*
* @return     None
*/
static void app_sync_timer_stop(void)
{
    /*! Stop the repeated timer */
    ret_code_t err_code = app_timer_stop(m_app_sync_timer_id);
    APP_ERROR_CHECK(err_code);
}

/**@brief   Callback Function for application timer events
*
* @param[in]  p_context: pointer to the callback function arguments
*
* @return     None
*/
static void app_sync_timeout_handler(void * p_context)
{
  CIRC_BUFF_STATUS_t  status = CIRC_BUFF_STATUS_ERROR;

  /* ADPD ODR < ADXL ODR */
  if(goAdiAdpdSSmInst.oAdpdSlotInst.aSlotInfo[gn_uc_hr_slot - 1].nOutputDataRate < gnAdxlODR )  
  {
    status = sync_adxl_buff_get(oHwSyncAdxlRawData.nDataValue, &oHwSyncAdxlRawData.nTimeStamp);
    if (status != CIRC_BUFF_STATUS_OK) {
        gnAdxlSYNC_BUFFERING_get++;
      return;
    }

    status = sync_adpd_buff_get(oHwSyncAdpdData.nDataValue, &oHwSyncAdpdData.nTimeStamp);
    if (status != CIRC_BUFF_STATUS_OK) {
        gnAdpdSYNC_BUFFERING_get++;
      return;
    }
  }

  /* ADPD ODR >= ADXL ODR */
  else
  {
    gsOneAdpdDataSetRdy = gsOneAdxlDataSetRdy = 0;
    status = sync_adpd_buff_get(oHwSyncAdpdData.nDataValue, &oHwSyncAdpdData.nTimeStamp);
    if (status != CIRC_BUFF_STATUS_OK) {
        gnAdpdSYNC_BUFFERING_get++;
      return;
    }

    status = sync_adxl_buff_get(oHwSyncAdxlRawData.nDataValue, &oHwSyncAdxlRawData.nTimeStamp);
    if (status != CIRC_BUFF_STATUS_OK) {
        gnAdxlSYNC_BUFFERING_get++;
      return;
    }
  }

  int16_t pnData[3] = {oHwSyncAdxlRawData.nDataValue[0], oHwSyncAdxlRawData.nDataValue[1], oHwSyncAdxlRawData.nDataValue[2]};
  oHwSyncAdxlData.nDataValue[0] = (pnData[0] & 0x3FFF) | ((pnData[0] & 0x3000) << 2);
  oHwSyncAdxlData.nDataValue[1] = (pnData[1] & 0x3FFF) | ((pnData[1] & 0x3000) << 2);
  oHwSyncAdxlData.nDataValue[2] = (pnData[2] & 0x3FFF) | ((pnData[2] & 0x3000) << 2);
  oHwSyncAdxlData.nTimeStamp = oHwSyncAdxlRawData.nTimeStamp;

  event_from_sync();
}

uint32_t *HwSyncGetSyncAdpdData() {
    return oHwSyncAdpdData.nDataValue;
}
int16_t *HwSyncGetSyncAccelData() {
    return oHwSyncAdxlData.nDataValue;
}
uint16_t *HwSyncGetSyncRawAccelData() {
    return oHwSyncAdxlRawData.nDataValue;
}
uint32_t HwSyncGetSyncAdpdTs() {
    return oHwSyncAdpdData.nTimeStamp;
}
uint32_t HwSyncGetSyncAccelTs() {
    return oHwSyncAdxlData.nTimeStamp;
}
uint32_t HwSyncGetSyncRawAccelTs() {
    return oHwSyncAdxlRawData.nTimeStamp;
}
#endif