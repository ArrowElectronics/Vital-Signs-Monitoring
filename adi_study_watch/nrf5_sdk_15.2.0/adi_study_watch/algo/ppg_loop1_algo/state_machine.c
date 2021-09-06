/*!
 *  \file StateMachine.c
 *
 * This source file is used to control the states through the library.
 *
 */
/*!
 *  \copyright Analog Devices
*******************************************************************************
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
* This software is intended for use with the ADPD and derivative parts    *
* only                                                                        *
*                                                                             *
******************************************************************************/

// Todo: change led current optimize to 75% for dark skin, 60% for light skin
// do integrator cross point optimization after Loop1

#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <math.h>
#include "adpd400x_lib.h"
#include "adpd400x_lib_common.h"
#include "heart_rate_internal.h"
#ifdef STATIC_AGC
//#include "agc.h"
#endif // STATIC_AGC

//================== LOG LEVELS=============================================//
#define NRF_LOG_MODULE_NAME PPG_LIB

#if PPG_LIB_CONFIG_LOG_ENABLED
#ifdef NRF_LOG_DEFAULT_LEVEL
#undef NRF_LOG_DEFAULT_LEVEL
#define NRF_LOG_DEFAULT_LEVEL  PPG_LIB_CONFIG_LOG_LEVEL
#endif
#define NRF_LOG_INFO_COLOR  PPG_LIB_CONFIG_INFO_COLOR
#define NRF_LOG_DEBUG_COLOR  PPG_LIB_CONFIG_DEBUG_COLOR
#else //PPG_LIB_CONFIG_LOG_ENABLED
#define NRF_LOG_LEVEL       0
#endif //PPG_LIB_CONFIG_LOG_ENABLED
#include "nrf_log.h"

#define MODULE ("StateMachine.c")

// #define DETECT_ON_TEST
#define SKIP_NUM        4   // Skip the first SKIP_NUM data for averaging

#ifndef TRUE
#define TRUE            1
#endif
#ifndef FALSE
#define FALSE           0
#endif

#define HR_ALGOR_PPG_LIMIT      600000      // ~(2^31-1 / 3200)
#define MAX_SIGNAL_LEVEL_FOR_PULSE_ADJ 600000
#define SINGLE_PULSE_SATURATION_VALUE 8000

/************************************************************
 Type Definitions
 ************************************************************/
#ifdef DETECTION_GREEN_LED
#define DetectObjectOnInit()        Adpd400xDetectObjectOnInitGreen()
#define DetectObjectOn(b)           Adpd400xDetectObjectOnGreen(b)
#define DetectObjectOnDeInit()      Adpd400xDetectObjectOnDeInitGreen()
#else
#define DetectObjectOnInit()        Adpd400xDetectObjectOnInitIR()
#define DetectObjectOn(b)           Adpd400xDetectObjectOnIR(b)
#define DetectObjectOnDeInit()      Adpd400xDetectObjectOnDeInitIR()
#endif

/************************************************************
 Public Variables
 ************************************************************/
Adpd400xLibConfig_t  *gAdpd400x_lcfg = 0;
uint16_t g_reg_base; //to hold the reg base offset according to the slot set in lcfg
ADPDLIB_STAGES_t gAdpd400xPpgLibState = ADPDLIB_STAGE_START;

uint32_t            gAdpd400xSlotBChOrg[2];
int16_t             gAdpd400xAdxlOrg[3];
uint32_t            gAdpd400xPpgTimeStamp;
AGCStat_t           gAdpd400xAGCStatInfo;
AlgoRawDataStruct_t gGetAlgoInfo;
uint16_t            gAdpd400xSlotAmode;

typedef enum {
  Flag_Clr = 0,
  Flag_Set = 1,
  Flag_Ack = 2
} Flag_State_t;

#if defined(AGC_FEATURE)
typedef enum {
  AgcLog_Start = 1,
  AgcLog_Stop_Normal = 2,
  AgcLog_Stop_Motion = 3,
  AgcLog_Adc_Saturation = 4,
  AgcLog_Afe_Saturation = 5,
  AgcLog_Loop1 = 6,
  AgcLog_AGC_recal = 7
} AgcLog_Indicator_t;

typedef enum {
  Agc_Start = 0,
  Agc_Stop_Normal = 1,
  Agc_Stop_Motion = 2
} Agc_State_t;
#endif
#ifdef STATIC_AGC
//extern agc_data_t agc_data[LED_SLOTS];
#endif // STATIC_AGC
extern uint16_t gnPulseWidth, gnAfeGainNum, gnBpfGainNum,gDVT2;
/************************************************************
 Extern function Prototype
 ************************************************************/
extern INT_ERROR_CODE_t Adpd400xheartRateInit();
extern INT_ERROR_CODE_t Adpd400xheartRate(uint32_t *slotData,
                           int16_t *acceldata,
                           int32_t timeStamp,
                           int32_t timeStampADPD,
                           LibResult_t *result);

extern void Adpd400xGetCtrInit(void);
extern void Adpd400xGetCtrDeInit(void);
extern INT_ERROR_CODE_t Adpd400xGetCtrValue(uint32_t *rawData);
extern void Adpd400xOptimizationInit_108(void);
extern void Adpd400xOptimizationDeInit_108(void);
extern INT_ERROR_CODE_t Adpd400xOptimizeSetting_108(uint32_t *rawData);
extern void AdpdMwLibAdjustRate(void);
extern void Adpd400xUtilSetLoop1Config(void);
#if defined(AGC_FEATURE) 
extern void Adpd400xACOptReset(uint8_t);
extern void Adpd400xMDResetTimers(void);
extern INT_ERROR_CODE_t Adpd400xACOptDoOptimization(uint32_t* rData, uint8_t* status );
extern INT_ERROR_CODE_t Adpd400xACOptInit(uint16_t);
//extern void ACOptDeInit(void);
#endif
/************************************************************
 Private Variables
 ************************************************************/
static uint32_t gsSampleCnt;
static uint16_t gRegInputs;
static uint8_t gsHrSampleCnt, gsNewSetting_SkipSampleNum;
#if defined(AGC_FEATURE) 
static uint32_t gsHighMotionCounter, gsLowMotionCounter;
static uint8_t gsHighMotionTrue, gsLowMotionTrue;
static uint16_t gsMotionCheckCount;
// static uint8_t gsFloatModeUsed;
static Agc_State_t gsAgcStarted, gsPreAgcState;
#endif
static uint16_t gsSampleRate, gsDecimation;
static Flag_State_t gsChangeSetting;
#ifdef STATIC_AGC
//static uint32_t gsAgcSampleCnt;
//static uint8_t gsAgcFlag = 0;
//static uint8_t gsAgcCount = 0;
#endif // STATIC_AGC
/************************************************************
 Private Function Prototypes
 ************************************************************/
static void SMStartInit(void);
static void SMEndDeInit(void);
#if defined(AGC_FEATURE)
static void SMDoAGCInit(void);
static void SMDoAGC(uint32_t *rAdpdData, int16_t *rAdxlData);
#endif
static void PostNewSettingSetUp(uint8_t);
static uint8_t SaturationAdjust(uint8_t,uint32_t);
static void SaturationAdjustCurrent(void);

#if defined(AGC_FEATURE)
static void SettingForHighMotion(void);
extern void Adpd400xMDCheckMotionInit();
extern INT_ERROR_CODE_t Adpd400xMDInstantCheck(int16_t *acceldata, uint8_t*);
extern INT_ERROR_CODE_t Adpd400xMDDurationCheck(void);
extern void Adpd400xMDPeriodicCheckStart(void);
#endif

/************************************************************
 Public Function Prototypes
 ************************************************************/
INT_ERROR_CODE_t Adpd400xStateMachineInit(void);
INT_ERROR_CODE_t Adpd400xStateMachineDeInit(void);
INT_ERROR_CODE_t Adpd400xStateMachine(LibResult_t *result,
                          uint32_t *slotData,
                          int16_t *acceldata,
                          TimeStamps_t timeStamp);


/**
  * @brief Initializes the state machine of the library
  * @retval SUCCESS = Initialization successful
  */
INT_ERROR_CODE_t Adpd400xStateMachineInit() {
  uint16_t nPulseRegVal,nTempReg,nDevId;
  Adpd400xDrvRegRead(ADPD400x_REG_CHIP_ID, &nDevId);
  /* Check if its DVT2 chip */
  gDVT2 = (nDevId != 0xC0) ? 1 : 0;
  g_reg_base = log2(gAdpd400x_lcfg->targetSlots) * SLOT_REG_OFFSET;
  AdpdMwLibSetMode(ADPDDrv_MODE_IDLE, ADPD400xDrv_SIZE_0, ADPD400xDrv_SIZE_0);
  if((gAdpd400x_lcfg->targetChs & BITM_TARGET_CH) == TARGET_CH3){
   AdpdDrvRegRead(ADPD400x_REG_INPUTS_A + g_reg_base,&gRegInputs);
   nTempReg = ((gRegInputs & 0xFFF0) | 0x0007);/*! IN1 and IN2 to channel 1 as single ended input */
   AdpdDrvRegWrite(ADPD400x_REG_INPUTS_A + g_reg_base,nTempReg); 
  }

  /* set initial LED*/
  uint8_t nPulseVal = gAdpd400x_lcfg->initialLedPulse & BITM_COUNTS_A_NUM_REPEAT_A;
  AdpdDrvRegRead(ADPD400x_REG_COUNTS_A + g_reg_base, &nPulseRegVal);
  nPulseRegVal = (nPulseRegVal & (~BITM_COUNTS_A_NUM_REPEAT_A)) | nPulseVal;
  AdpdDrvRegWrite(ADPD400x_REG_COUNTS_A + g_reg_base, nPulseRegVal);
  SMStartInit();
  gAdpd400xPPGLibStatus.AFE_OpMode = ADPD400xLIB_AFE_OPMODE_NORMAL; 
  Adpd400xStoreOpModeSetting();  // store intial settings
  AdpdMwLibSetMode(ADPD400xDrv_MODE_SAMPLE, ADPD400xDrv_SIZE_0, ADPD400xDrv_SIZE_32);
#ifndef STATIC_AGC
  gAdpd400xPpgLibState = ADPDLIB_STAGE_START;
#else 
  gAdpd400xPpgLibState = ADPDLIB_STAGE_HEART_RATE_INIT;
#endif // STATIC_AGC
  gsSampleCnt = 0;
#if defined(AGC_FEATURE)
  gsAgcStarted = Agc_Stop_Normal;
  gsPreAgcState = Agc_Stop_Normal;
  gsChangeSetting = Flag_Clr;
  gsNewSetting_SkipSampleNum = 0;
  gsHighMotionCounter = 0;
  gsLowMotionCounter = 0;
  gsHighMotionTrue = 0;
  gsLowMotionTrue = 1;
#endif
  memset(&gAdpd400xAGCStatInfo,0,sizeof(gAdpd400xAGCStatInfo));
  return IERR_SUCCESS;
}

/**
  * @brief De-initializes the state machine of the library
  * @retval SUCCESS = Initialization successful
  */
INT_ERROR_CODE_t Adpd400xStateMachineDeInit() {
    uint16_t nTemp,nRegTrim,nTsCtrl;
#ifndef STATIC_AGC
    gAdpd400xPpgLibState = ADPDLIB_STAGE_START;
#else 
    gAdpd400xPpgLibState = ADPDLIB_STAGE_HEART_RATE_INIT;
#endif // STATIC_AGC
    g_reg_base = log2(gAdpd400x_lcfg->targetSlots) * SLOT_REG_OFFSET;
    Adpd400xDrvRegRead(ADPD400x_REG_OPMODE, &nTemp);
    AdpdMwLibSetMode(ADPDDrv_MODE_IDLE, ADPD400xDrv_SIZE_0, ADPD400xDrv_SIZE_0);
    if((gAdpd400x_lcfg->targetChs & BITM_TARGET_CH) == 3){//restore on sum mode
      AdpdDrvRegWrite(ADPD400x_REG_INPUTS_A + g_reg_base,gRegInputs);
    }
    if (gAdpd400x_lcfg->featureSelect & DYNAMIC_AGC_EN) //Restore Sample Rate if changed by Dynamic AGC
    {
      PpgLibSetSampleRate(gAdpd400xOptmVal.sampleRate,gAdpd400xOptmVal.decimation,gAdpd400x_lcfg->targetSlots);
    }
    if(gDVT2){ 
      AdpdDrvRegRead(ADPD400x_REG_AFE_TRIM_A + g_reg_base, &nRegTrim);
      nRegTrim &= ~(BITM_AFE_TRIM_X_TIA_CEIL_DETECT_EN_X);
      AdpdDrvRegWrite(ADPD400x_REG_AFE_TRIM_A + g_reg_base, nRegTrim);

        /*Disable Subsampling*/ 
      AdpdDrvRegRead(ADPD400x_REG_TS_CTRL_A + g_reg_base, &nTsCtrl);
      nTsCtrl &= ~(BITM_TS_CTRL_A_SUBSAMPLE_EN_A);
      AdpdDrvRegWrite(ADPD400x_REG_TS_CTRL_A + g_reg_base, nTsCtrl);

    }
    if(nTemp & 0x0001){ // put back to sample mode only if its running previously
      AdpdMwLibSetMode(ADPD400xDrv_MODE_SAMPLE, ADPD400xDrv_SIZE_0, ADPD400xDrv_SIZE_32);
    }
    return IERR_SUCCESS;
}

/**
  * @brief Main State Machine of the library
  * @param result  structure holding results of library processing
  * @param slotData pointer to an array of uint32_t that contains the slotB
  *                input data for algorithm processing
  * @param acceldata pointer to an array of int16_t that contains the data
  *                from the 3 axes of acclerometer for algorithm processing
  * @param timeStamp structure holding timestamp of Adpd/Adxl and Algo calling instant
  * @retval indicating success in processing the raw data or
  *         if it is in progress processing the data
  */
INT_ERROR_CODE_t Adpd400xStateMachine(LibResult_t *result,
                          uint32_t *slotData,
                          int16_t *acceldata,
                          TimeStamps_t timeStamp) {
  INT_ERROR_CODE_t errorCode = IERR_SUCCESS;
  INT_ERROR_CODE_t ret = IERR_SUCCESS;
  uint32_t dc_mean, dc_varianceCh1,dc_varianceCh2;
  uint8_t gotAverage, isSaturated = 0;
    
  uint16_t nslotDetectionCh1 = 0,nslotDetectionCh2 = 0,nTsCtrl = 0,nCh2Enable = 0;
  uint32_t nppgData = 0;

  // log entrance data
  memcpy(gAdpd400xSlotBChOrg, slotData, sizeof(gAdpd400xSlotBChOrg));
  memcpy(gAdpd400xAdxlOrg, acceldata, sizeof(gAdpd400xAdxlOrg));
  gAdpd400xPpgTimeStamp = timeStamp.tsADPD;
  // g_resultlog.SUM_SLOTB = slotBCh[0] + slotBCh[1] + slotBCh[2] + slotBCh[3];
 
  if (gAdpd400xPpgLibState == ADPDLIB_STAGE_HEART_RATE) {
    memset(&gGetAlgoInfo, 0, sizeof(gGetAlgoInfo));
    result->HR = 0;
    result->confidence = 0;
    result->HR_Type = 0;
    result->RRinterval = 0;
    gotAverage = 0;

    if (gsNewSetting_SkipSampleNum != 0)  {
      gsNewSetting_SkipSampleNum--;
            // debug(MODULE, "@@ skip ts=%u, data=%x", gAdpd400xPpgTimeStamp, slotBCh[0]+slotBCh[1]);
      if (gsNewSetting_SkipSampleNum == 0)  {
        Adpd400xUtilGetMeanVarInit(0, 1);   // start new Average calculation
      }
      return ret;   // skip few samples after setting change
    }
    /* check if ch2 is enabled */
    g_reg_base = log2(gAdpd400x_lcfg->targetSlots) * SLOT_REG_OFFSET; 
    Adpd400xDrvRegRead(ADPD400x_REG_TS_CTRL_A + g_reg_base, &nTsCtrl);
    nCh2Enable = (nTsCtrl & BITM_TS_CTRL_A_CH2_EN_A) >> BITP_TS_CTRL_A_CH2_EN_A;
    if(gDVT2){
      nppgData = slotData[0];
      isSaturated = 0;
      Adpd400xDrvRegRead(ADPD400x_REG_INT_STATUS_TC1, &nslotDetectionCh1);
      if((nslotDetectionCh1 >> (int)log2(gAdpd400x_lcfg->targetSlots))== 1){
        isSaturated = 1;
      } 
      if(nCh2Enable && ((gAdpd400x_lcfg->targetChs & BITM_TARGET_CH) != TARGET_CH3)){ // In sum mode or ch2 disable case, saturation check not needed
        Adpd400xDrvRegRead(ADPD400x_REG_INT_STATUS_TC2, &nslotDetectionCh2);
        if((nslotDetectionCh2 >> (int)log2(gAdpd400x_lcfg->targetSlots))== 1){
          isSaturated = 1;
        }
      }
      if(isSaturated){
        if (gAdpd400x_lcfg->featureSelect & DYNAMIC_AGC_EN){ // Saturation check only available for dynamic AGC        
          SaturationAdjust(0,nppgData);
          Adpd400xLibUpdateAGCStateInfo(ADPD400xLIB_AGCLOG_AFE_SATURATION);      // Saturation indication
        }else{
          SaturationAdjustCurrent();//reduce current by gAdpd400x_lcfg->satAdjustPercentForStaticAgc %
          Adpd400xLibUpdateAGCStateInfo(ADPD400xLIB_AGCLOG_AFE_SATURATION);          
        }
        ret = IERR_AFE_SATURATION;
        PostNewSettingSetUp(1);
        Adpd400xDrvRegWrite(ADPD400x_REG_INT_STATUS_TC1, nslotDetectionCh1);//clear the detection bit ch1 register
        if((gAdpd400x_lcfg->targetChs & BITM_TARGET_CH) != TARGET_CH3){
        Adpd400xDrvRegWrite(ADPD400x_REG_INT_STATUS_TC2, nslotDetectionCh2);//clear the detection bit ch2 register
        }
        return ret;
      }
    }
    if (Adpd400xUtilGetMeanVar(slotData, &dc_mean, &dc_varianceCh1, &dc_varianceCh2) == IERR_SUCCESS)  {
      gotAverage = 1;
      isSaturated = 0;
      Adpd400xUtilGetMeanVarInit(0, 1);
      if (gAdpd400x_lcfg->featureSelect & DYNAMIC_AGC_EN)
      {
        // check saturation
        // or: dc_mean / channel num + dc_offset > 16k * puls num
        if (dc_mean > HR_ALGOR_PPG_LIMIT)  {   // HR PPG input limit
          SaturationAdjust(1,dc_mean); /* changes pulse count, agc pkt is sent */
          isSaturated = 1;
          ret = IERR_ADC_SATURATION;
        } 
        if (isSaturated == 1)  {
          PostNewSettingSetUp(1);
  #if defined(AGC_FEATURE)
          Adpd400xLibUpdateAGCStateInfo(ADPD400xLIB_AGCLOG_ADC_SATURATION);      // Saturation indication
  #endif        
          NRF_LOG_DEBUG("@@ saturated DC=%u, varch1=%u,varch2=%u, LED=%x, Pulse=%x \r\n",
            dc_mean, dc_varianceCh1,dc_varianceCh2,\
            gAdpd400xAGCStatInfo.setting[1], gAdpd400xAGCStatInfo.setting[3]);
          return ret;
        }
      }
      if(!gDVT2){//for DVT1 watch TIA saturation register not available, using variance method for saturation check   
        if(dc_varianceCh1 == 0){
          isSaturated = 1;
        }
        if((dc_varianceCh2 == 0) && nCh2Enable && ((gAdpd400x_lcfg->targetChs & BITM_TARGET_CH) != TARGET_CH3)){ // In sum mode or ch2 disable case, saturation check not needed
          isSaturated = 1;
        }
        if (isSaturated == 1){
          if (gAdpd400x_lcfg->featureSelect & DYNAMIC_AGC_EN){ // Saturation check only available for dynamic AGC        
            SaturationAdjust(0,dc_mean);
            Adpd400xLibUpdateAGCStateInfo(ADPD400xLIB_AGCLOG_AFE_SATURATION);       // Saturation indication
          }else{
            SaturationAdjustCurrent();//reduce current by gAdpd400x_lcfg->satAdjustPercentForStaticAgc %
            Adpd400xLibUpdateAGCStateInfo(ADPD400xLIB_AGCLOG_AFE_SATURATION);           
          }
          ret = IERR_AFE_SATURATION;
          PostNewSettingSetUp(1);
          return ret;
        }
      }
      
      // after new setting, the next mean is used as detect off level
      if (gsChangeSetting == Flag_Ack)  {   // 1st packet of new setting
        gsChangeSetting = Flag_Clr;
        // set new detect DC Level
        gAdpd400xDetectVal.curDcLevelG = dc_mean;
        //debug(MODULE, "@@ New detect level=%u", dc_mean);
		NRF_LOG_INFO("@@ New detect level=%u \r\n",dc_mean);
        //Adpd400xDetectObjectOffInit();
      }
    }
#if 0
    if ((gAdpd400x_lcfg->featureSelect & DETECT_OFF_EN) != 0 && gotAverage == 1) {
      if (Adpd400xDetectObjectOFF(dc_mean, dc_varianceCh1) == IERR_SUCCESS) {
        gAdpd400xPpgLibState = ADPDLIB_STAGE_DETECT_OFF;
        // SMDataLogging(slotBCh);
        ret = IERR_OFF_SENSOR;
        //debug(MODULE, "@@ Off detected level=%u", dc_mean);
		NRF_LOG_INFO("@@ Off detect level=%u \r\n",dc_mean);
        return ret;
      }
    }
#endif

#if defined(AGC_FEATURE)
    if (gAdpd400x_lcfg->featureSelect & DYNAMIC_AGC_EN) {
      SMDoAGC(slotData, acceldata);
      if (gsAgcStarted == Agc_Start && gsPreAgcState != Agc_Start)  {
        gAdpd400xAGCStatInfo.setting[0] = AgcLog_Start;         // first started
        gAdpd400xAGCStatInfo.mts[0] = 0xFFFF;// signal quality not captured
        gAdpd400xAGCStatInfo.setting[9] = 0x0000; // no power change
        Adpd400xACOptReset(0); //reset signal metrics buffer
        //debug(MODULE, "@@ AGC started, LED current=%x", gAdpd400xAGCStatInfo.setting[1]);
		NRF_LOG_DEBUG("@@ AGC started, LED current=%x \r\n",gAdpd400xAGCStatInfo.setting[1]);
      }
      if (gsAgcStarted == Agc_Stop_Normal && gsPreAgcState == Agc_Start) {
        gAdpd400xAGCStatInfo.setting[0] = AgcLog_Stop_Normal;   // first stop
        //debug(MODULE, "@@ AGC stopped, new LED current=%x", gAdpd400xAGCStatInfo.setting[1]);
		NRF_LOG_DEBUG("@@ AGC stopped, new LED current=%x \r\n",gAdpd400xAGCStatInfo.setting[1]);
      }
      if (gsAgcStarted == Agc_Stop_Motion && gsPreAgcState == Agc_Start) {
        gAdpd400xAGCStatInfo.setting[0] = AgcLog_Stop_Motion;   // stop when motion
        gAdpd400xAGCStatInfo.mts[0] = 0xFFFF;// signal quality not captured
        gAdpd400xAGCStatInfo.setting[9] = 0x0000; // no power change
        // skip 3 data here? No need
        //debug(MODULE, "@@ AGC stopped due to motion");
		NRF_LOG_DEBUG("@@ AGC stopped due to motion");
      }
      gsPreAgcState = gsAgcStarted;
      gAdpd400xAGCStatInfo.timestamp = AdpdLibGetSensorTimeStamp();
      if (gsChangeSetting == Flag_Set)  {
        ret = IERR_AGC_ADJUSTED;
        gsChangeSetting = Flag_Ack;
        return ret;
      }
    }
#endif
    if ( ((gAdpd400x_lcfg->featureSelect & HR_ALGO_EN) != 0) && (gAdpd400x_lcfg->hrmInputRate == 50) ) {
      errorCode = Adpd400xheartRate(slotData,
                            acceldata,
                            timeStamp.tsAlgorithmCall,
                            timeStamp.tsADPD,
                            result);
    } else {
      errorCode = IERR_SUCCESS_WITH_RESULT;
      //Fixed HR value given, also when STATIC_AGC_EN is selected and hrmInputRate is made 100Hz or 500Hz
      result->HR = 1408; /*heart rate 88 in fixed point format*/
    }

    gsHrSampleCnt++;
    if ((errorCode == IERR_HRM_TIMEOUT) || (errorCode == IERR_ALGO_INPUT_OVERFLOW)){
           return errorCode;
    }
    /*heart rate range 39 to 223 in fixed point format*/
    if (errorCode == IERR_SUCCESS_WITH_RESULT && \
      result->HR > 624 && result->HR < 3568) {
#if defined(ADUCM3029) || defined(WITH_LIBRARY)
      if ((gsHrSampleCnt&0x7F) == 0)
        //debug(MODULE, "### HR: %d", result->HR/16);
	    NRF_LOG_INFO("### HR: %d", result->HR/16);
#else
        //debug(MODULE, "### HR: %f", result->HR);
	    NRF_LOG_INFO("### HR: %d", result->HR);
#endif //ADUCM3029
        ret = IERR_SUCCESS_WITH_RESULT;
    }
    // SMDataLogging(slotBCh);
    return ret;
  }


  result->HR_Type = -1;
  if (gAdpd400xPpgLibState == ADPDLIB_STAGE_START) {
    SMEndDeInit();          // clear up from previous failure
#ifndef STATIC_AGC
    gAdpd400xPpgLibState = ADPDLIB_STAGE_DARKOFFSET_CALIBRATION;
#else 
    gAdpd400xPpgLibState = ADPDLIB_STAGE_HEART_RATE_INIT;
#endif // STATIC_AGC
    gsSampleCnt = 0;
  }

  if (gAdpd400xPpgLibState == ADPDLIB_STAGE_DARKOFFSET_CALIBRATION) {
    if (gsSampleCnt++ == 0)
      Adpd400xLibGetDarkOffsetInit();

    if (Adpd400xLibGetDarkOffset(slotData, 0) == ADPDLIB_ERR_SUCCESS_WITH_RESULT) {
      // set dark offset for each channel regardless used or not
      Adpd400xLibSetDarkOffset();

      // set offset value depends on its usage and Channel selection
      Adpd400xSetChannelFilter(gAdpd400x_lcfg->targetSlots, -1);

      if ((gAdpd400x_lcfg->featureSelect & DETECT_ON_EN) == 1)  {
        gAdpd400xPpgLibState = ADPDLIB_STAGE_DETECT_PERSON;
      } else {
        gAdpd400xPpgLibState = ADPDLIB_STAGE_GETCTR;
      }
      gsSampleCnt = 0;
    }

    ret = IERR_SUCCESS;
    return ret;
  }

  if (gAdpd400xPpgLibState == ADPDLIB_STAGE_DETECT_PERSON) {
    if (gsSampleCnt++ == 0)  {
      DetectObjectOnInit();
    }

    errorCode = DetectObjectOn(slotData);
    if (errorCode == IERR_TIMEOUT) {
      //debug(MODULE, "Timeout waiting for subject");
	  NRF_LOG_DEBUG("Timeout waiting for subject");
      ret = IERR_SUCCESS_WITH_RESULT;
      result->HR = IERR_TIMEOUT_WAITING_FOR_PERSON;
      gAdpd400xPpgLibState = ADPDLIB_STAGE_START;
    } else if (errorCode == IERR_OFF_SENSOR) {
      //debug(MODULE, "Object Off sensor");
	  NRF_LOG_DEBUG("Object Off sensor");
      ret = IERR_SUCCESS_WITH_RESULT;
      result->HR = ADPDLIB_ERR_OFF_SENSOR;
      gAdpd400xPpgLibState = ADPDLIB_STAGE_START;
    } else if (errorCode == IERR_SUCCESS) {
      gsSampleCnt = 0;
      gAdpd400xPpgLibState = ADPDLIB_STAGE_GETCTR;
      //debug(MODULE, "Stage changed to LED cal init");
	  NRF_LOG_DEBUG("Stage changed to LED cal init");

      ret = IERR_SUCCESS;
    } else if (errorCode == IERR_SATURATED) {
      //debug(MODULE, "Signal Saturated.");
      NRF_LOG_DEBUG("Signal Saturated.");

      result->HR = IERR_SATURATED;
      ret = IERR_SUCCESS_WITH_RESULT;
    } else if (errorCode == IERR_DEVICE_ON_MOTION) {
      //debug(MODULE, "Device is not stable.");
      NRF_LOG_DEBUG("Device is not stable.");

      result->HR = IERR_DEVICE_ON_MOTION;
      ret = IERR_SUCCESS_WITH_RESULT;
    } else {    // IERR_IN_PROGRESS
      ret = IERR_SUCCESS;
    }

    if ( gAdpd400xPpgLibState != ADPDLIB_STAGE_DETECT_PERSON )
      DetectObjectOnDeInit();

    // SMDataLogging(slotBCh);
    return ret;
  }
#if 0
  if (gAdpd400xPpgLibState == ADPDLIB_STAGE_GETCTR)  {
    if (gsSampleCnt++ == 0) {
      Adpd400xGetCtrInit();
    }
    errorCode = Adpd400xGetCtrValue(slotData);
    if (errorCode == IERR_IN_PROGRESS) {
      ret = IERR_SUCCESS;
      NRF_LOG_INFO("Get CTR value Pass!!\r\n");
      // SMDataLogging(slotBCh, acceldata, timeStamp);
      return ret;
    }

    if (errorCode == IERR_FAIL) {
      //debug(MODULE, "Get CTR value Fail!!\r\n");
      NRF_LOG_INFO("Get CTR value Fail!!\r\n");
      // should not fail (something wrong if saturates with 25K gain).
      // ret = IERR_FAIL;
      // return ret;
    }
    Adpd400xGetCtrDeInit();
    if ((gAdpd400x_lcfg->featureSelect & OPTIMIZE_EN) != 0) {
      gAdpd400xPpgLibState = ADPDLIB_STAGE_OPTIMIZATION;
    } else {
      gAdpd400xPpgLibState = ADPDLIB_STAGE_END_CALIBRATION;
    }
    gsSampleCnt = 0;
  }

  // find Loop1 setting, then decide float mode
  if (gAdpd400xPpgLibState == ADPDLIB_STAGE_OPTIMIZATION) {
    if (gsSampleCnt++ == 0)
      Adpd400xOptimizationInit_108();

    errorCode = Adpd400xOptimizeSetting_108(slotData);
    if (errorCode == IERR_IN_PROGRESS) {
      ret = IERR_SUCCESS;
      // SMDataLogging(slotBCh, acceldata, timeStamp);
      return ret;
    }
    if (errorCode == IERR_SUCCESS) {  // Cal Done
      // debug(MODULE, "Optimization PASS\r\n");
      //adi_printf("Optimization PASS\r\n");
      NRF_LOG_INFO("Optimization PASS\r\n");
      Adpd400xStoreOpModeSetting();  // save Loop1 Setting
      gAdpd400xPpgLibState = ADPDLIB_STAGE_END_CALIBRATION;
      if ((gAdpd400x_lcfg->featureSelect & FLOATMODE_EN) != 0) {
        // Required PD_q * 70% > PD_q with max LED, use float mode
        if (gAdpd400xOptmVal.SelectedMode == 1)
          gAdpd400xPpgLibState = ADPDLIB_STAGE_FLOAT_MODE;
      }
    } else {    // Fail
      NRF_LOG_INFO("Optimization FAIL\r\n");
      gAdpd400xPpgLibState = ADPDLIB_STAGE_START;
    }

    Adpd400xOptimizationDeInit_108();
    /*debug(MODULE, "Opt Result: Reg105=%0x Reg106=%0x Reg107=%0x Reg104=%0x", \
          gAdpd400xOptmVal.ledB_Cur, gAdpd400xOptmVal.ledB_Trim, \
          gAdpd400xOptmVal.ledB_Pulse, gAdpd400xOptmVal.tiaB_Gain);*/
    NRF_LOG_DEBUG("Opt Result: Reg105=%0x Reg106=%0x Reg107=%0x Reg104=%0x\r\n", \
           gAdpd400xOptmVal.ledB_Cur, gAdpd400xOptmVal.ledB_Trim, \
           gAdpd400xOptmVal.ledB_Pulse, gAdpd400xOptmVal.tiaB_Gain);

    // initialize loop2 setting.
    gAdpd400xOptmVal.ledB_Cur2 = gAdpd400xOptmVal.ledB_Cur;
    gAdpd400xOptmVal.ledB_Trim2 = gAdpd400xOptmVal.ledB_Trim;
    gAdpd400xOptmVal.ledB_Pulse2 = gAdpd400xOptmVal.ledB_Pulse;
    gAdpd400xOptmVal.tiaB_Gain2 = gAdpd400xOptmVal.tiaB_Gain;
    gAdpd400xOptmVal.sampleRate2 = gAdpd400xOptmVal.sampleRate;
    gAdpd400xOptmVal.ledB_CurVal2 = gAdpd400xOptmVal.ledB_CurVal;

    gsSampleCnt = 0;
    // SMDataLogging(slotBCh, acceldata, timeStamp);
    ret = IERR_SUCCESS;
    return ret;
  }
#endif
  // use Loop1 Normal mode if float mode fail
  if (gAdpd400xPpgLibState == ADPDLIB_STAGE_FLOAT_MODE) {
#if 0
    if (gsSampleCnt++ == 0) {
      FloatModeInit();
    }
    errorCode = Adpd400xSetFloatMode(slotData);
    if (errorCode == IERR_IN_PROGRESS) {
      return ret;
    }
    if (errorCode == IERR_SUCCESS) {
      AdpdDrvRegRead(REG_LED1_DRV, &gAdpd400xOptmVal.ledB_Cur);
      AdpdDrvRegRead(REG_LED_TRIM, &gAdpd400xOptmVal.ledB_Trim);
      AdpdDrvRegRead(REG_PULSE_PERIOD_B, &gAdpd400xOptmVal.ledB_Pulse);
      AdpdDrvRegRead(REG_AFE_TRIM_B, &gAdpd400xOptmVal.tiaB_Gain );
      AdpdDrvRegRead(0x3F, &gAdpd400xOptmVal.ledB_FltWid );
      gAdpd400xPPGLibStatus.AFE_OpMode = ADPDLIB_AFE_OPMODE_FLOAT;
      // store float mode setting
      Adpd400xStoreOpModeSetting();
    } else {
      Adpd400xFloatModeDeInit();  // return back to normal mode
      gAdpd400xPPGLibStatus.AFE_OpMode = ADPDLIB_AFE_OPMODE_NORMAL;
      debug(MODULE, "FLOAT MODE FAIL");
    }
#endif
    gsSampleCnt = 0;
    gAdpd400xPpgLibState = ADPDLIB_STAGE_END_CALIBRATION;
    return IERR_SUCCESS;
  }

  if (gAdpd400xPpgLibState == ADPDLIB_STAGE_END_CALIBRATION) {
    if (gsSampleCnt++ == 0) {
      if ((gAdpd400x_lcfg->featureSelect & DETECT_ON_EN) == 0) {
        gsSampleCnt = 0;
        gAdpd400xPpgLibState = ADPDLIB_STAGE_HEART_RATE_INIT;
        NRF_LOG_INFO("ADPD Lib Stage End Calibration\r\n");
        // SMDataLogging(slotBCh, acceldata, timeStamp);
        return ret;
      }
#ifdef DETECT_ON_TEST
      AdpdLibDelay(2000);
#endif
      DetectObjectOnInit();
    }
    errorCode = DetectObjectOn(slotData);
    if (errorCode == IERR_SUCCESS) {
      //debug(MODULE, "!!!Device is ###ON###");
      NRF_LOG_INFO("!!!Device is ###ON Wrist###\r\n");

      gAdpd400xPpgLibState = ADPDLIB_STAGE_HEART_RATE_INIT;
      gsSampleCnt = 0;
      ret = IERR_SUCCESS;
    } else if (errorCode == IERR_IN_PROGRESS) {
      ret = IERR_SUCCESS;
    } else {
      //debug(MODULE, "!!!Device was on but now is OFF wrist!!!");
       NRF_LOG_INFO("!!!Device was on but now is OFF wrist!!!\r\n");

      ret = IERR_SUCCESS_WITH_RESULT;
      result->HR = ADPDLIB_ERR_OFF_SENSOR;
      gAdpd400xPpgLibState = ADPDLIB_STAGE_START;
    }
    if (gAdpd400xPpgLibState != ADPDLIB_STAGE_END_CALIBRATION)
      DetectObjectOnDeInit();

#ifdef DETECT_ON_TEST
    if (errorCode != IERR_IN_PROGRESS) {
      debug(MODULE, "************** Detection test done ***************");

      AdpdLibDelay(3000);
      //debug(MODULE, "------- New Detection Starting -------");
	  NRF_LOG_INFO("!!!Device was on but now is OFF wrist!!!\r\n");

      gAdpd400xPpgLibState = ADPDLIB_STAGE_START;
      ret = IERR_SUCCESS;
    }
#endif
    // SMDataLogging(slotBCh, acceldata, timeStamp);
    return ret;
  }

  if (gAdpd400xPpgLibState == ADPDLIB_STAGE_HEART_RATE_INIT) {
#ifndef STATIC_AGC
    if (gsSampleCnt == 0)  {
      // restore LED setting
      AdpdMwLibSetMode(ADPDDrv_MODE_IDLE,
                       ADPD400xDrv_SIZE_0,
                       ADPD400xDrv_SIZE_0);
      Adpd400xApplyOpModeSetting();
      PpgLibSetMode(ADPD400xDrv_MODE_SAMPLE,
                    (ADPDDrv_Operation_Slot_t)gSlotAmode,
                    (ADPDDrv_Operation_Slot_t)(gAdpd400x_lcfg->deviceMode));
      Adpd400xUtilGetMeanVarInit(0, 0);     // Get mean for detect OFF
      gAdpd400xDetectVal.curDcLevelG = 0;
    }

    gsSampleCnt++;
    if (gsSampleCnt < SKIP_NUM) {   // skip the first few samples
      return ret;
    } else if (gsSampleCnt < SKIP_NUM + 100) {
      if (Adpd400xUtilGetMeanVar(slotData, &dc_mean, &dc_variance) == IERR_SUCCESS)  {
        Adpd400xUtilGetMeanVarInit(0, 1);
        gAdpd400xDetectVal.curDcLevelG = dc_mean;
        gsSampleCnt = SKIP_NUM + 100;
        debug(MODULE, "Dectect OFF DC level:%u", gAdpd400xDetectVal.curDcLevelG);
      }
      return ret;
    }

    //Adpd400xDetectObjectOffInit();
    // LedDriftInit();
#if defined(AGC_FEATURE)    
    if ((gAdpd400x_lcfg->featureSelect & DYNAMIC_AGC_EN) != 0) {
      Adpd400xMDCheckMotionInit();
    }
#endif    
    // todo: AmbientCheckInit();
    Adpd400xheartRateInit();
#if defined(AGC_FEATURE)    
    if ((gAdpd400x_lcfg->featureSelect & DYNAMIC_AGC_EN) != 0) {
      SMDoAGCInit();
      gAdpd400xAGCStatInfo.mts[0] = 0xFFFF;
      gAdpd400xAGCStatInfo.setting[1] = gAdpd400xOptmVal.ledB_Cur;
      gAdpd400xAGCStatInfo.setting[2] = gAdpd400xOptmVal.ledB_Trim;
      gAdpd400xAGCStatInfo.setting[3] = gAdpd400xOptmVal.ledB_Pulse;
      gAdpd400xAGCStatInfo.setting[4] = gAdpd400xOptmVal.tiaB_Gain;
      gAdpd400xAGCStatInfo.setting[5] = gAdpd400xOptmVal.sampleRate;
      gAdpd400xAGCStatInfo.setting[6] = g_Adpd400xchannelNums;
      gAdpd400xAGCStatInfo.setting[8] = gAdpd400xPPGLibStatus.CtrValue;
      gAdpd400xAGCStatInfo.timestamp = AdpdLibGetSensorTimeStamp();
    }
#endif    
    /*debug(MODULE, "LED12:%x, LED34:%x, Pulse:%x\r\n",  \
      gOptmVal.ledB_Cur, gOptmVal.ledB_Trim, gOptmVal.ledB_Pulse);*/
    NRF_LOG_DEBUG("LED12:%x, LED34:%x, Pulse:%x\r\n",  \
      gOptmVal.ledB_Cur, gOptmVal.ledB_Trim, gOptmVal.ledB_Pulse);
    gAdpd400xPpgLibState = ADPDLIB_STAGE_HEART_RATE;
    gsSampleCnt = 0;
#else //STATIC_AGC
#if 0
    if ((gAdpd400x_lcfg->featureSelect & STATIC_AGC_EN) != 0)
    {
      if(gsAgcFlag == 0){
        agc_init(gAdpd400x_lcfg->hrmInputRate);
        gsAgcFlag = 1;
      }
      
      if(gsAgcCount++ < 30) {
        // do nothing
      } else if (gsAgcSampleCnt < SAMPLE_AVG_NUM) {
          agc_data[LED_SLOTS-1].ch1[gsAgcSampleCnt] = *slotData;
          agc_data[LED_SLOTS-1].ch2[gsAgcSampleCnt] = *(slotData+1);
          //agc_data[LED_SLOTS-1].ch2[gsAgcSampleCnt] = 0;
          gsAgcSampleCnt++;
          //return ret;
      } else if (gsAgcSampleCnt >= SAMPLE_AVG_NUM) {
          /*if(agc_data_process() == 1) {
            gAdpd400xPpgLibState = ADPDLIB_STAGE_HEART_RATE;
          }*/
          agc_data_process();
          agc_deinit();
          gAdpd400xPpgLibState = ADPDLIB_STAGE_HEART_RATE;
          gsAgcCount = 0;
          gsAgcSampleCnt = 0;
          gsSampleCnt = 0;
          gsAgcFlag = 0;
      }
    }
    else
    {
      gAdpd400xPpgLibState = ADPDLIB_STAGE_HEART_RATE;
      //Write default dcfg values to LED and TIA register
      // Slot F LED 1A Green
      Adpd400xDrvRegWrite(ADPD400x_REG_LED_POW12_F, 0x0002);

      // Slot F TIA gain
      Adpd400xDrvRegWrite(ADPD400x_REG_AFE_TRIM_F, 0xE3C1);
    }
#endif
    Adpd400xheartRateInit();
#if defined(AGC_FEATURE)
     if(gPpg_agc_done)
     {
      /* Adpd400xDetectObjectOffInit(); */
      if ((gAdpd400x_lcfg->featureSelect & DYNAMIC_AGC_EN) != 0) {
       Adpd400xMDCheckMotionInit();
      }

      if ((gAdpd400x_lcfg->featureSelect & DYNAMIC_AGC_EN) != 0) {
        SMDoAGCInit();
      } 
     }
     else
     {
       return ret;
     }
#endif //AGC_FEATURE
#endif // STATIC_AGC 
      uint16_t ledCurrent,ledTrim,tiaGain;
      g_reg_base = log2(gAdpd400x_lcfg->targetSlots) * SLOT_REG_OFFSET;
      Adpd400xDrvRegRead(ADPD400x_REG_LED_POW12_A + g_reg_base, &ledCurrent);
      gAdpd400xOptmVal.ledB_Cur = ledCurrent;
      if((ledCurrent & BITM_LED_POW12_X_LED_CURRENT1_X) != 0)
        gAdpd400xOptmVal.ledB_CurVal = ledCurrent;

      Adpd400xDrvRegRead(ADPD400x_REG_LED_POW34_A + g_reg_base, &ledTrim);
      if((ledTrim & BITM_LED_POW34_X_LED_CURRENT3_X) != 0)
        gAdpd400xOptmVal.ledB_Trim = ledTrim;

      Adpd400xDrvRegRead(ADPD400x_REG_AFE_TRIM_A + g_reg_base, &tiaGain);
      gAdpd400xOptmVal.tiaB_Gain = tiaGain;
      if(gDVT2){
        g_reg_base = log2(gAdpd400x_lcfg->targetSlots) * SLOT_REG_OFFSET;
        AdpdMwLibSetMode(ADPDDrv_MODE_IDLE, ADPD400xDrv_SIZE_0, ADPD400xDrv_SIZE_0);
        /*Enable TIA Saturation check*/ 
        AdpdDrvRegRead(ADPD400x_REG_AFE_TRIM_A + g_reg_base, &tiaGain);
        tiaGain |= (BITM_AFE_TRIM_X_TIA_CEIL_DETECT_EN_X);
        AdpdDrvRegWrite(ADPD400x_REG_AFE_TRIM_A + g_reg_base, tiaGain);
        gAdpd400xOptmVal.tiaB_Gain = tiaGain;
        /*Enable Subsampling*/ 
        AdpdDrvRegRead(ADPD400x_REG_TS_CTRL_A + g_reg_base, &nTsCtrl);
        nTsCtrl |= (BITM_TS_CTRL_A_SUBSAMPLE_EN_A);
        AdpdDrvRegWrite(ADPD400x_REG_TS_CTRL_A + g_reg_base, nTsCtrl);
        AdpdMwLibSetMode(ADPD400xDrv_MODE_SAMPLE, ADPD400xDrv_SIZE_0, ADPD400xDrv_SIZE_32);
      }
      gAdpd400xAGCStatInfo.mts[0] = 0xFFFF;
      gAdpd400xAGCStatInfo.setting[1] = gAdpd400xOptmVal.ledB_Cur;
      gAdpd400xAGCStatInfo.setting[2] = gAdpd400xOptmVal.ledB_Trim;
      gAdpd400xAGCStatInfo.setting[3] = gAdpd400xOptmVal.ledB_Pulse;
      gAdpd400xAGCStatInfo.setting[4] = gAdpd400xOptmVal.tiaB_Gain;
      gAdpd400xAGCStatInfo.setting[5] = gAdpd400xOptmVal.sampleRate;
          // initialize loop2 setting.
      gAdpd400xOptmVal.ledB_Cur2 = gAdpd400xOptmVal.ledB_Cur;
      gAdpd400xOptmVal.ledB_Trim2 = gAdpd400xOptmVal.ledB_Trim;
      gAdpd400xOptmVal.ledB_Pulse2 = gAdpd400xOptmVal.ledB_Pulse;
      gAdpd400xOptmVal.tiaB_Gain2 = gAdpd400xOptmVal.tiaB_Gain;
      gAdpd400xOptmVal.sampleRate2 = gAdpd400xOptmVal.sampleRate;
      gAdpd400xOptmVal.ledB_CurVal2 = gAdpd400xOptmVal.ledB_CurVal;

      gAdpd400xPpgLibState = ADPDLIB_STAGE_HEART_RATE;
      gsSampleCnt = 0;
      gsNewSetting_SkipSampleNum = 3;// For meanvariance init
    }

  // Offsensor: back to ADPDLIB_STAGE_START stage.
  if (gAdpd400xPpgLibState == ADPDLIB_STAGE_DETECT_OFF) {
    ret = IERR_SUCCESS_WITH_RESULT;
    result->HR = IERR_OFF_SENSOR;
    gAdpd400xPpgLibState = ADPDLIB_STAGE_START;
    //debug(MODULE, "######## OFF WRIST");
	NRF_LOG_DEBUG("######## OFF WRIST\r\n");
  }

  return ret;
}


/**
  * @brief State Machine Initialization
  * @param None
  * @retval None
  */
static void SMStartInit() {
  // UtilRecordGeneralSetting(); fs,
  Adpd400xUtilRecordCriticalSetting();
  gAdpd400xOptmVal.ledB_Cur = gSlotOri.led12_Cur;
  gAdpd400xOptmVal.ledB_Trim = gSlotOri.led34_Cur;
  gAdpd400xOptmVal.ledB_Pulse = gSlotOri.led_Pulse;
  gAdpd400xOptmVal.tiaB_Gain = gSlotOri.tia_Gain;

  PpgLibGetSampleRate(&gsSampleRate, &gsDecimation,gAdpd400x_lcfg->targetSlots);  // backup sampling rate
  gAdpd400xOptmVal.sampleRate = gsSampleRate;
  gAdpd400xOptmVal.decimation = gsDecimation;

  gAdpd400xPPGLibStatus.AFE_OpMode = ADPD400xLIB_AFE_OPMODE_NORMAL;
  gAdpd400xOptmVal.SelectedLoop = 1;
}

/**
  * @brief State Machine De-initialization
  * @param None
  * @retval None
  */
static void SMEndDeInit() {
  Adpd400xUtilRestoreCriticalSetting();
  // PpgLibSetSampleRate(gsSampleRate, gsDecimation,gAdpd400x_lcfg->targetSlots);    // restore sampling rate
}

#if defined(AGC_FEATURE)
static void SMDoAGCInit()  {
  Adpd400xACOptInit(gAdpd400x_lcfg->partNum); /* partNum val? */
  gsMotionCheckCount = gAdpd400x_lcfg->hrmInputRate * gAdpd400x_lcfg->motionCheckPeriodHigh;
}

static void SMDoAGC(uint32_t *rAdpdData, int16_t *rAdxlData)  {
  uint8_t status;
  uint8_t motionLevel;
  // check motion
  if (Adpd400xMDInstantCheck(rAdxlData, &motionLevel) == IERR_FAIL) {
    gsAgcStarted = Agc_Stop_Motion;   // to stop the AGC segment.

    if (motionLevel == HIGH_MOTION)  {   // high motion
      if (++gsHighMotionCounter >= gsMotionCheckCount)
        gsHighMotionTrue = 1;
      gsLowMotionCounter = 0;
    } else {
      if (++gsLowMotionCounter >= gsMotionCheckCount) {
        gsLowMotionTrue = 1;
        gsHighMotionTrue = 0;
      }
      gsHighMotionCounter = 0;
    }

    if (gsHighMotionTrue && gsLowMotionTrue)  {
      SettingForHighMotion();   // set to high power
      gsHighMotionTrue = 0;
      gsLowMotionTrue = 0;
      gsHighMotionCounter = 0;
      gsLowMotionCounter = 0;
    }
    return;                 // return when there's motion
  } else {
    // reset counter if no motion
    gsHighMotionCounter = 0;
    if (gsLowMotionCounter < 0xFFFF)
      gsLowMotionCounter++;
  }

  // check if time is N seconds and resting for M seconds
  if (Adpd400xMDDurationCheck() == IERR_SUCCESS)  {   // within optimize period
    // do AC optimization analysis
    gsChangeSetting = Flag_Clr;
    gsAgcStarted = Agc_Start;
    if (Adpd400xACOptDoOptimization(rAdpdData, &status) == IERR_SUCCESS)  {
      if (status == 1)  {
        PostNewSettingSetUp(1);
        gAdpd400xAGCStatInfo.setting[9] = 0xEEEE;               // power changed
      } else {
        PostNewSettingSetUp(0);
        gAdpd400xAGCStatInfo.setting[9] = 0x0000;               // power not changed
      }
      gAdpd400xOptmVal.SelectedLoop = 2;
      gAdpd400xAGCStatInfo.setting[1] = gAdpd400xOptmVal.ledB_Cur2;
      gAdpd400xAGCStatInfo.setting[2] = gAdpd400xOptmVal.ledB_Trim2;
      gAdpd400xAGCStatInfo.setting[3] = gAdpd400xOptmVal.ledB_Pulse2;
      gAdpd400xAGCStatInfo.setting[4] = gAdpd400xOptmVal.tiaB_Gain;
      gAdpd400xAGCStatInfo.setting[5] = gAdpd400xOptmVal.sampleRate2;

      gAdpd400xAGCStatInfo.mts[0] = gAdpd400xOptmVal.sigQuality;
      gAdpd400xAGCStatInfo.mts[1] = gAdpd400xOptmVal.Mt1;
      gAdpd400xAGCStatInfo.mts[2] = gAdpd400xOptmVal.Mt2;
      gAdpd400xAGCStatInfo.mts[3] = gAdpd400xOptmVal.Mt3;
    }
  }
  return;
}
#endif

static void PostNewSettingSetUp(uint8_t newSetting)   {
  if (newSetting == 1)  {       // with new power setting
    // Signal Algorithm
    Adpd400xHeartRateSignalAlgNewSetting();
    gsChangeSetting = Flag_Set;
    gsNewSetting_SkipSampleNum = 3;
  }
#if defined(AGC_FEATURE)
  if ((gAdpd400x_lcfg->featureSelect & DYNAMIC_AGC_EN) != 0)  {
    Adpd400xMDResetTimers();
    Adpd400xMDPeriodicCheckStart();     // no need to wait for 90s
    gsAgcStarted = Agc_Stop_Normal;
  }
#endif  
}

// 1 = adc_saturation, 0 = afe_saturation
static uint8_t SaturationAdjust(uint8_t adc_saturation,uint32_t dc_mean)  {
  uint16_t ledCurrent, ledPulses, ledReg12, ledReg34, temp,sampleRate, decimateVal;
  double tempDouble;
  uint8_t retVal, ori_pulse_value;
  
  PpgLibGetSampleRate(&sampleRate, &decimateVal,gAdpd400x_lcfg->targetSlots);  // backup sampling rate
  g_reg_base = log2(gAdpd400x_lcfg->targetSlots) * SLOT_REG_OFFSET;

  AdpdDrvRegRead((ADPD400x_REG_COUNTS_A  + g_reg_base), &temp);
  ledPulses = (temp&0xFF);
  ori_pulse_value = ledPulses;
  if (adc_saturation == 1)  {    // ADC saturation, adjust pulses
    AdpdMwLibSetMode(ADPDDrv_MODE_IDLE, ADPD400xDrv_SIZE_0, ADPD400xDrv_SIZE_0);

    ledPulses = ledPulses * gAdpd400x_lcfg->targetDcPercent / 100; 

    retVal = ADPDLibPostPulseDecreaseAdjust(&ledPulses,&ori_pulse_value,&sampleRate, &decimateVal, &dc_mean);
    
    temp = (temp & 0xFF00) | (ledPulses);
    AdpdMwLibSetMode(ADPDDrv_MODE_IDLE, ADPD400xDrv_SIZE_0, ADPD400xDrv_SIZE_0);
    AdpdDrvRegWrite((ADPD400x_REG_COUNTS_A  + g_reg_base), temp);
    PpgLibSetSampleRate(sampleRate, decimateVal,gAdpd400x_lcfg->targetSlots);

    PpgLibSetMode(ADPD400xDrv_MODE_SAMPLE, \
                  ADPD400xDrv_SIZE_0, \
                  (ADPDDrv_Operation_Slot_t)(gAdpd400x_lcfg->deviceMode));
    gAdpd400xOptmVal.ledB_Pulse2 = temp;
    gAdpd400xOptmVal.SelectedLoop = 2;  // dont change loop1 setting
    gAdpd400xAGCStatInfo.setting[3] = gAdpd400xOptmVal.ledB_Pulse2;
    return 1;
  }

  ledCurrent = Adpd400xUtilGetCurrentFromSlot(gAdpd400x_lcfg->targetSlots);
  ledCurrent *= gAdpd400x_lcfg->targetDcPercent;
  ledCurrent /= 100;  // Adjust to valid range


  tempDouble = 200 - gAdpd400x_lcfg->targetDcPercent;
  ledPulses = (uint16_t)(ledPulses * tempDouble / 100); // add pulses

  retVal = ADPDLibPostPulseIncreaseAdjust(&ledPulses,&ori_pulse_value,&sampleRate, &decimateVal,&dc_mean);

  // if the ledPulses increment is too high as indicated by return value of 9, 
  //  then revert to original pulse
  if(retVal == IERR_FAIL) {
    ledPulses = (temp & 0xFF);
  }
  temp = (temp & 0xFF00) | (ledPulses);
  gAdpd400xOptmVal.ledB_Pulse2 =temp;
  
  AdpdMwLibSetMode(ADPDDrv_MODE_IDLE, ADPD400xDrv_SIZE_0, ADPD400xDrv_SIZE_0);
  Adpd400xUtilGetCurrentRegValue(ledCurrent, &ledReg12, &ledReg34);
  AdpdDrvRegWrite(g_reg_base + ADPD400x_REG_LED_POW12_A, ledReg12);
  AdpdDrvRegWrite(g_reg_base + ADPD400x_REG_LED_POW34_A, ledReg34);
  AdpdDrvRegWrite((ADPD400x_REG_COUNTS_A  + g_reg_base), temp);
  PpgLibSetSampleRate(sampleRate, decimateVal,gAdpd400x_lcfg->targetSlots)
  PpgLibSetMode(ADPD400xDrv_MODE_SAMPLE, \
                ADPD400xDrv_SIZE_0, \
                (ADPDDrv_Operation_Slot_t)(gAdpd400x_lcfg->deviceMode));

  gAdpd400xOptmVal.ledB_Cur2 = ledReg12;
  gAdpd400xOptmVal.ledB_Trim2 = ledReg34;
  gAdpd400xOptmVal.ledB_CurVal2 = Adpd400xUtilGetCurrentFromReg(ledReg12, ledReg34);
  gAdpd400xOptmVal.SelectedLoop = 2;
  gAdpd400xAGCStatInfo.setting[1] = gAdpd400xOptmVal.ledB_Cur2;
  gAdpd400xAGCStatInfo.setting[2] = gAdpd400xOptmVal.ledB_Trim2;
  return 1;
}

static void SaturationAdjustCurrent()  {
  uint16_t ledCurrent,ledReg12, ledReg34;

  g_reg_base = log2(gAdpd400x_lcfg->targetSlots) * SLOT_REG_OFFSET;
  
  ledCurrent = Adpd400xUtilGetCurrentFromSlot(gAdpd400x_lcfg->targetSlots);
  ledCurrent *= gAdpd400x_lcfg->satAdjustPercentForStaticAgc;
  ledCurrent /= 100;  // Adjust to valid range
  if(ledCurrent != 0){
  AdpdMwLibSetMode(ADPDDrv_MODE_IDLE, ADPD400xDrv_SIZE_0, ADPD400xDrv_SIZE_0);
  Adpd400xUtilGetCurrentRegValue(ledCurrent, &ledReg12, &ledReg34);
  AdpdDrvRegWrite(g_reg_base + ADPD400x_REG_LED_POW12_A, ledReg12);
  AdpdDrvRegWrite(g_reg_base + ADPD400x_REG_LED_POW34_A, ledReg34);
  PpgLibSetMode(ADPD400xDrv_MODE_SAMPLE, \
                ADPD400xDrv_SIZE_0, \
                (ADPDDrv_Operation_Slot_t)(gAdpd400x_lcfg->deviceMode));
  //Update for debug info
  gAdpd400xOptmVal.ledB_Cur2 = ledReg12;
  gAdpd400xOptmVal.ledB_Trim2 = ledReg34;
  gAdpd400xOptmVal.ledB_CurVal2 = Adpd400xUtilGetCurrentFromReg(ledReg12, ledReg34);
  gAdpd400xOptmVal.SelectedLoop = 2;
  }
}

#if defined(AGC_FEATURE) 
static void SettingForHighMotion()  {
  uint32_t dcPower, acPower;

  // if in loop2 and previous highMotion is false
  if (gAdpd400xOptmVal.SelectedLoop == 2)  {
    // check current setting power compare to loop1
    dcPower = gAdpd400xOptmVal.ledB_CurVal * (gAdpd400xOptmVal.ledB_Pulse & (0xFF));
    dcPower *= gAdpd400xOptmVal.sampleRate;
    acPower = gAdpd400xOptmVal.ledB_CurVal2 * (gAdpd400xOptmVal.ledB_Pulse2 & (0xFF));
    acPower *= gAdpd400xOptmVal.sampleRate2;

    if (dcPower > acPower)  {   // loop1 > loop2
      AdpdMwLibSetMode(ADPDDrv_MODE_IDLE, ADPD400xDrv_SIZE_0, ADPD400xDrv_SIZE_0);
      Adpd400xUtilSetLoop1Config();     // Apply Loop1 optimum setting
      AdpdMwLibSetMode(ADPD400xDrv_MODE_SAMPLE,
                      ADPD400xDrv_SIZE_0,
                      (ADPDDrv_Operation_Slot_t)(gAdpd400x_lcfg->deviceMode));
      PostNewSettingSetUp(1);   
      gAdpd400xAGCStatInfo.setting[1] = gAdpd400xOptmVal.ledB_Cur;
      gAdpd400xAGCStatInfo.setting[2] = gAdpd400xOptmVal.ledB_Trim;
      gAdpd400xAGCStatInfo.setting[3] = gAdpd400xOptmVal.ledB_Pulse;
      gAdpd400xAGCStatInfo.setting[4] = gAdpd400xOptmVal.tiaB_Gain;
      gAdpd400xAGCStatInfo.setting[5] = gAdpd400xOptmVal.sampleRate;

      gAdpd400xAGCStatInfo.mts[0] = 0xFFFF;
      gAdpd400xAGCStatInfo.mts[1] = 0;
      gAdpd400xAGCStatInfo.mts[2] = 0;
      gAdpd400xAGCStatInfo.mts[3] = 0;
      gAdpd400xAGCStatInfo.setting[0] = AgcLog_Loop1;      // Switch to loop1 indication
      gAdpd400xAGCStatInfo.setting[9] = 0xEEEE;               // power changed
	  NRF_LOG_INFO("HighMotion change Power!!\r\n");
    } else {
	  NRF_LOG_INFO("HighMotion, Keep current Power!!\r\n");
    }
    // else do nothing
  }
  return;
}
#endif