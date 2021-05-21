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
  AgcLog_Saturation = 4,
  AgcLog_Drift = 5,
  AgcLog_Loop1 = 6
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
extern INT_ERROR_CODE_t Adpd400xACOptDoOptimization(uint32_t* rData, uint8_t* status );
extern void AdpdMwLibAdjustRate(void);
extern void Adpd400xUtilSetLoop1Config(void);

/************************************************************
 Private Variables
 ************************************************************/
static uint32_t gsSampleCnt;
static uint8_t gsHrSampleCnt, gsNewSetting_SkipSampleNum;
static uint32_t gsHighMotionCounter, gsLowMotionCounter;
static uint8_t gsHighMotionTrue, gsLowMotionTrue;
static uint16_t gsMotionCheckCount;
static uint16_t gsSampleRate, gsDecimation;
// static uint8_t gsFloatModeUsed;
#if defined(AGC_FEATURE)
static Agc_State_t gsAgcStarted, gsPreAgcState;
#endif
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
static void SaturationAdjust(uint8_t);
static void SettingForHighMotion(void);

#if defined(AGC_FEATURE)
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
  AdpdMwLibSetMode(ADPDDrv_MODE_IDLE, ADPD400xDrv_SIZE_0, ADPD400xDrv_SIZE_0);
  SMStartInit();
  gAdpd400xPPGLibStatus.AFE_OpMode = ADPD400xLIB_AFE_OPMODE_NORMAL;
  Adpd400xStoreOpModeSetting();  // store intial settings
  AdpdMwLibSetMode(ADPD400xDrv_MODE_SAMPLE, ADPD400xDrv_SIZE_0, ADPD400xDrv_SIZE_32);
  // gsFloatModeUsed = 0;
#ifndef STATIC_AGC
  gAdpd400xPpgLibState = ADPDLIB_STAGE_START;
#else 
  gAdpd400xPpgLibState = ADPDLIB_STAGE_HEART_RATE_INIT;
#endif // STATIC_AGC
  gsSampleCnt = 0;
#if defined(AGC_FEATURE)
  gsAgcStarted = Agc_Stop_Normal;
  gsPreAgcState = Agc_Stop_Normal;
#endif
  gsChangeSetting = Flag_Clr;
  gsNewSetting_SkipSampleNum = 0;
  gsHighMotionCounter = 0;
  gsLowMotionCounter = 0;
  gsHighMotionTrue = 0;
  gsLowMotionTrue = 1;
#ifdef STATIC_AGC
  //gsAgcSampleCnt = 0;
  //gsAgcFlag = 0;
  //gsAgcCount = 0;
#endif // STATIC_AGC
  return IERR_SUCCESS;
}

/**
  * @brief De-initializes the state machine of the library
  * @retval SUCCESS = Initialization successful
  */
INT_ERROR_CODE_t Adpd400xStateMachineDeInit() {
    // if (gPpgLibState == ADPDLIB_STAGE_PROXIMITY_DETECT)
    //    DetectProximityDeInit();        // if stops at prximity stage.
    gAdpd400xPpgLibState = ADPDLIB_STAGE_START;

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
  uint32_t dc_mean, dc_variance;
  uint8_t gotAverage, isSaturated = 0;

  // log entrance data
  memcpy(gAdpd400xSlotBChOrg, slotData, sizeof(gAdpd400xSlotBChOrg));
  memcpy(gAdpd400xAdxlOrg, acceldata, sizeof(gAdpd400xAdxlOrg));
  gAdpd400xPpgTimeStamp = timeStamp.tsADPD;
  // g_resultlog.SUM_SLOTB = slotBCh[0] + slotBCh[1] + slotBCh[2] + slotBCh[3];
 
  if (gAdpd400xPpgLibState == ADPDLIB_STAGE_HEART_RATE) {
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

    #if 0
        // if in float mode, check Ambient bigDark bit
        if ((gAdpd400x_lcfg->featureSelect & SKIP_AMBIENT_CHECK) == 0)  {
          if (gAdpd400xPPGLibStatus.AFE_OpMode == ADPDLIB_AFE_OPMODE_FLOAT) {
            if (AdpdCheckAmbientIRQ() == 1)  {    // chk ambient. It is auto clear
              // Idle->clear buffer->adjust->notify algorithm->op mode
              Adpd400xFloatModeAdjestAmbient();
              PostNewSettingSetUp(1);
              AdpdCheckAmbientIRQ();    // clear irq again
            }
          } else {
            // in Normal mode, use connection mode every 30s to check ambient
          }
        }
    #endif
    if (Adpd400xUtilGetMeanVar(slotData, &dc_mean, &dc_variance) == IERR_SUCCESS)  {
      gotAverage = 1;
      Adpd400xUtilGetMeanVarInit(0, 1);
      // check saturation
      // or: dc_mean / channel num + dc_offset > 16k * puls num
      if (dc_mean > HR_ALGOR_PPG_LIMIT)  {   // HR PPG input limit
        SaturationAdjust(1);
        isSaturated = 1;
      } else if (dc_variance == 0)   {
        SaturationAdjust(0);
        isSaturated = 1;
      }
      if (isSaturated == 1)  {
        PostNewSettingSetUp(1);
#if defined(AGC_FEATURE)        
        gAdpd400xAGCStatInfo.setting[0] = AgcLog_Saturation;      // Saturation indication
#endif        
        ret = IERR_SATURATED;
        /*debug(MODULE, "@@ saturated DC=%u, var=%u, LED=%x, Puls=%x", \
              dc_mean, dc_variance, \
              gAdpd400xAGCStatInfo.setting[1], gAdpd400xAGCStatInfo.setting[3]);*/
	NRF_LOG_DEBUG("@@ saturated DC=%u, var=%u, LED=%x, Pulse=%x \r\n",
          dc_mean, dc_variance,\
          gAdpd400xAGCStatInfo.setting[1], gAdpd400xAGCStatInfo.setting[3]);
        return ret;
      }

      // after new setting, the next mean is used as detect off level
      if (gsChangeSetting == Flag_Ack)  {   // 1st packet of new setting
        gsChangeSetting = Flag_Clr;
        // set new detect DC Level
        gAdpd400xDetectVal.curDcLevelG = dc_mean;
        //debug(MODULE, "@@ New detect level=%u", dc_mean);
		NRF_LOG_INFO("@@ New detect level=%u \r\n",dc_mean);
        Adpd400xDetectObjectOffInit();
      }
    }

    if ((gAdpd400x_lcfg->featureSelect & DETECT_OFF_EN) != 0 && gotAverage == 1) {
      if (Adpd400xDetectObjectOFF(dc_mean, dc_variance) == IERR_SUCCESS) {
        gAdpd400xPpgLibState = ADPDLIB_STAGE_DETECT_OFF;
        // SMDataLogging(slotBCh);
        ret = IERR_OFF_SENSOR;
        //debug(MODULE, "@@ Off detected level=%u", dc_mean);
		NRF_LOG_INFO("@@ Off detect level=%u \r\n",dc_mean);
        return ret;
      }
    }

#if defined(AGC_FEATURE)
    if (gAdpd400x_lcfg->featureSelect & AGC_EN) {
      SMDoAGC(slotData, acceldata);
      if (gsAgcStarted == Agc_Start && gsPreAgcState != Agc_Start)  {
        gAdpd400xAGCStatInfo.setting[0] = AgcLog_Start;         // first started
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
        // skip 3 data here? No need
        //debug(MODULE, "@@ AGC stopped due to motion");
		NRF_LOG_DEBUG("@@ AGC stopped due to motion");
      }
      gsPreAgcState = gsAgcStarted;
      gAdpd400xAGCStatInfo.timestamp = timeStamp.tsADPD;
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
    // if (gsFloatModeUsed == 1)  {
    // FloatModeDeInit();  // clear up from previous
    // gsFloatModeUsed = 0;
    // }
    SMEndDeInit();          // clear up from previous failure
    gsSampleCnt = 0;
    gAdpd400xPpgLibState = ADPDLIB_STAGE_DARKOFFSET_CALIBRATION;

    if (gAdpd400x_lcfg->featureSelect == HR_ALGO_EN) {
      gAdpd400xPpgLibState = ADPDLIB_STAGE_HEART_RATE_INIT;
    }
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
    // SMDataLogging(slotBCh);
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

    Adpd400xDetectObjectOffInit();
    // LedDriftInit();
#if defined(AGC_FEATURE)    
    Adpd400xMDCheckMotionInit();
#endif    
    // todo: AmbientCheckInit();
    Adpd400xheartRateInit();
#if defined(AGC_FEATURE)    
    if ((gAdpd400x_lcfg->featureSelect & AGC_EN) != 0) {
      SMDoAGCInit();
      gAdpd400xAGCStatInfo.mts[0] = 0xFFFF;
      gAdpd400xAGCStatInfo.setting[1] = gAdpd400xOptmVal.ledB_Cur;
      gAdpd400xAGCStatInfo.setting[2] = gAdpd400xOptmVal.ledB_Trim;
      gAdpd400xAGCStatInfo.setting[3] = gAdpd400xOptmVal.ledB_Pulse;
      gAdpd400xAGCStatInfo.setting[4] = gAdpd400xOptmVal.tiaB_Gain;
      gAdpd400xAGCStatInfo.setting[5] = gAdpd400xOptmVal.sampleRate;
      gAdpd400xAGCStatInfo.setting[6] = g_Adpd400xchannelNums;
      gAdpd400xAGCStatInfo.setting[8] = gAdpd400xPPGLibStatus.CtrValue;
      gAdpd400xAGCStatInfo.timestamp = timeStamp.tsADPD;
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
#endif // STATIC_AGC
    gAdpd400xPpgLibState = ADPDLIB_STAGE_HEART_RATE;
    gsSampleCnt = 0;
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

  PpgLibGetSampleRate(&gsSampleRate, &gsDecimation);  // backup sampling rate
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
  // PpgLibSetSampleRate(gsSampleRate, gsDecimation);    // restore sampling rate
}

#if defined(AGC_FEATURE)
static void SMDoAGCInit()  {
  // Todo: ACOptInit(g_lcfg->partNum);
  // find the ODR
  /* gsHighMotionCheckCount = gOptmVal.sampleRate / gOptmVal.decimation \
                           * g_lcfg->motionCheckPeriodHigh;*/
  gsMotionCheckCount = gAdpd400x_lcfg->hrmInputRate * gAdpd400x_lcfg->motionCheckPeriodHigh;

  // PpgLibGetSampleRate(&sampleRate, &decimation);
}

static void SMDoAGC(uint32_t *rAdpdData, int16_t *rAdxlData)  {
  uint8_t status;
  uint8_t motionLevel;
#if 0
  if (gAdpd400xPPGLibStatus.CtrValue < gAdpd400x_lcfg->ctrTh)
    return;   // use high power setting from loop1
#endif // 0
  // check motion
  if (Adpd400xMDInstantCheck(rAdxlData, &motionLevel) == IERR_FAIL) {
    gsAgcStarted = Agc_Stop_Motion;   // to stop the AGC segment.

    if (motionLevel == HIGH_MOTION)  {   // high motion
      if (++gsHighMotionCounter >= gsMotionCheckCount)
        gsHighMotionTrue = 1;
      gsLowMotionCounter = 0;
    } else {
      if (++gsLowMotionCounter >= gsMotionCheckCount)
        gsLowMotionTrue = 1;
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
  if ((gAdpd400x_lcfg->featureSelect & AGC_EN) != 0)  {
    Adpd400xMDResetTimers();
    Adpd400xMDPeriodicCheckStart();     // no need to wait for 90s
    gsAgcStarted = Agc_Stop_Normal;
  }
#endif  
}

// 1 = adc_saturation, 0 = afe_saturation
static void SaturationAdjust(uint8_t adc_saturation)  {
  uint16_t ledCurrent, ledPulses, ledReg12, ledReg34;
  double tempDouble;
  
  g_reg_base = log2(gAdpd400x_lcfg->targetSlots) * 0x20;

  //ledPulses = Adpd400xUtilGetPulseNum(PPG_SLOTA);
  ledPulses = Adpd400xUtilGetPulseNum(gAdpd400x_lcfg->targetSlots);
  if (adc_saturation == 1)  {    // ADC saturation, adjust pulses
    AdpdMwLibSetMode(ADPDDrv_MODE_IDLE, ADPD400xDrv_SIZE_0, ADPD400xDrv_SIZE_0);
    // reduce to targetDcPercent
    ledPulses = (uint16_t)((ledPulses * gAdpd400x_lcfg->targetDcPercent + 50) / 100);
    //Adpd400xUtilSetPulseNum(PPG_SLOTA, ledPulses, 1);
    Adpd400xUtilSetPulseNum(gAdpd400x_lcfg->targetSlots, ledPulses, 1);
    PpgLibSetMode(ADPD400xDrv_MODE_SAMPLE, \
                  ADPD400xDrv_SIZE_0, \
                  (ADPDDrv_Operation_Slot_t)(gAdpd400x_lcfg->deviceMode));
    gAdpd400xOptmVal.ledB_Pulse2 = ledPulses;
    gAdpd400xOptmVal.SelectedLoop = 2;  // dont change loop1 setting
    gAdpd400xAGCStatInfo.setting[3] = gAdpd400xOptmVal.ledB_Pulse2;
    return;
  }

  // saturation in afe
  //ledCurrent = Adpd400xUtilGetCurrentFromSlot(PPG_SLOTA);
  ledCurrent = Adpd400xUtilGetCurrentFromSlot(gAdpd400x_lcfg->targetSlots);
  ledCurrent *= gAdpd400x_lcfg->targetDcPercent;
  ledCurrent /= 100;  // Adjust to valid range

#if 0   // add pulses to makeup the same performance lost from less led current
  tempDouble = gAdpd400x_lcfg->targetDcPercent * gAdpd400x_lcfg->targetDcPercent;
  tempDouble = 10000 / tempDouble;
  ledPulses *= tempDouble;   // gain back the same SNR by therory
#else   // add pulses by the same percentage
  tempDouble = 200 - gAdpd400x_lcfg->targetDcPercent;
  ledPulses = (uint16_t)(((ledPulses * tempDouble) + 50) / 100); // add pulses
#endif

  //gAdpd400xOptmVal.ledB_Pulse2 = Adpd400xUtilGetPulseNum(PPG_SLOTA);
  gAdpd400xOptmVal.ledB_Pulse2 = Adpd400xUtilGetPulseNum(gAdpd400x_lcfg->targetSlots);

  AdpdMwLibSetMode(ADPDDrv_MODE_IDLE, ADPD400xDrv_SIZE_0, ADPD400xDrv_SIZE_0);
  Adpd400xUtilGetCurrentRegValue(ledCurrent, &ledReg12, &ledReg34);
  AdpdDrvRegWrite(g_reg_base + ADPD400x_REG_LED_POW12_A, ledReg12);
  AdpdDrvRegWrite(g_reg_base + ADPD400x_REG_LED_POW34_A, ledReg34);
  // limitation on pulses (timing, and even number etc).
  //Adpd400xUtilSetPulseNum(PPG_SLOTA, ledPulses, 1);
  Adpd400xUtilSetPulseNum(gAdpd400x_lcfg->targetSlots, ledPulses, 1);

  PpgLibSetMode(ADPD400xDrv_MODE_SAMPLE, \
                ADPD400xDrv_SIZE_0, \
                (ADPDDrv_Operation_Slot_t)(gAdpd400x_lcfg->deviceMode));

  gAdpd400xOptmVal.ledB_Cur2 = ledReg12;
  gAdpd400xOptmVal.ledB_Trim2 = ledReg34;
  gAdpd400xOptmVal.ledB_CurVal2 = Adpd400xUtilGetCurrentFromReg(ledReg12, ledReg34);
  gAdpd400xOptmVal.SelectedLoop = 2;
  gAdpd400xAGCStatInfo.setting[1] = gAdpd400xOptmVal.ledB_Cur2;
  gAdpd400xAGCStatInfo.setting[2] = gAdpd400xOptmVal.ledB_Trim2;
}

static void SettingForHighMotion()  {
  uint32_t dcPower, acPower;

  // if in loop2 and previous highMotion is false
  if (gAdpd400xOptmVal.SelectedLoop == 2)  {
    // check current setting power compare to loop1
    dcPower = gAdpd400xOptmVal.ledB_CurVal * (gAdpd400xOptmVal.ledB_Pulse>>8);
    dcPower *= gAdpd400xOptmVal.sampleRate;
    acPower = gAdpd400xOptmVal.ledB_CurVal2 * (gAdpd400xOptmVal.ledB_Pulse2>>8);
    acPower *= gAdpd400xOptmVal.sampleRate2;

    if (dcPower > acPower)  {   // loop1 > loop2
      AdpdMwLibSetMode(ADPDDrv_MODE_IDLE, ADPD400xDrv_SIZE_0, ADPD400xDrv_SIZE_0);
      Adpd400xUtilSetLoop1Config();     // Apply Loop1 optimum setting
      AdpdMwLibSetMode(ADPD400xDrv_MODE_SAMPLE,
                      ADPD400xDrv_SIZE_0,
                      (ADPDDrv_Operation_Slot_t)(gAdpd400x_lcfg->deviceMode));
      PostNewSettingSetUp(1);
#if defined(AGC_FEATURE)      
      gAdpd400xAGCStatInfo.setting[1] = gAdpd400xOptmVal.ledB_Cur;
      gAdpd400xAGCStatInfo.setting[2] = gAdpd400xOptmVal.ledB_Trim;
      gAdpd400xAGCStatInfo.setting[3] = gAdpd400xOptmVal.ledB_Pulse;
      gAdpd400xAGCStatInfo.setting[4] = gAdpd400xOptmVal.tiaB_Gain;
      gAdpd400xAGCStatInfo.setting[5] = gAdpd400xOptmVal.sampleRate;

      gAdpd400xAGCStatInfo.mts[0] = 0xFF;
      gAdpd400xAGCStatInfo.mts[1] = 0;
      gAdpd400xAGCStatInfo.mts[2] = 0;
      gAdpd400xAGCStatInfo.mts[3] = 0;
      gAdpd400xAGCStatInfo.setting[0] = AgcLog_Loop1;      // Switch to loop1 indication
#endif      
      //adi_printf("\nHighMotion change Power!!");
	  NRF_LOG_INFO("HighMotion change Power!!\r\n");
    } else {
      //adi_printf("\nHighMotion, Keep current Power!!");
	  NRF_LOG_INFO("HighMotion, Keep current Power!!\r\n");
    }
    // else do nothing
  }
  return;
}
