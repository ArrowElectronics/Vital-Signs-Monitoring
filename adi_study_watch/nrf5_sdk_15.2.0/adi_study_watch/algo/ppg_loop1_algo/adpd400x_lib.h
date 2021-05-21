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
* This software is intended for use with the ADUX1020 and derivative parts    *
* only                                                                        *
*                                                                             *
******************************************************************************/
#ifndef __ADPD400xLIB_H__
#define __ADPD400xLIB_H__

#include <stdint.h>
#include "sdk_config.h"
#include "nrf_log_ctrl.h"

#ifdef __cplusplus /* If this is a C++ compiler, use C linkage */
extern "C" {
#endif

#if 0
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
//NRF_LOG_MODULE_REGISTER ();
//==========================================================================//
#endif

#define HRMPPGLIBVERSIONNUMBER      (0x00000100)
#define HRMALGORITHMVERSIONNUMBER   (0x00010200) // 1.2.0

/* Macro */
/*
    Bit field - 0: do not skip; 1: skip
    0 - Detection
    1 - LED Calibraiton
    2 - DOC Calibration
    3 - Heart Rate
    4 - Object Detection
 */
#define DETECT_ON_EN            ((uint16_t)(1 << 0))    /* 0x01 */
#define DETECT_OFF_EN           ((uint16_t)(1 << 1))    /* 0x02 */

#define OPTIMIZE_EN             ((uint16_t)(1 << 4))    /* 0x10 */
#define FLOATMODE_EN            ((uint32_t)(1 << 6))    /* 0x40*/

#define AGC_EN                  ((uint16_t)(1 << 8))    /* 0x100 */
#define STATIC_AGC_EN           ((uint16_t)(1 << 9))    /* 0x200 */

#define HR_ALGO_EN              ((uint16_t)(1 << 12))   /* 0x1000 */

typedef enum {
    ADPDLIB_ERR_FAIL                       =  2,
    ADPDLIB_ERR_SUCCESS_WITH_RESULT        =  1,
    ADPDLIB_ERR_SUCCESS                    =  0,
    ADPDLIB_ERR_ON_SENSOR                  = -1,
    ADPDLIB_ERR_DEVICE_ON_MOTION           = -2,
    ADPDLIB_ERR_OFF_SENSOR                 = -3,
    ADPDLIB_ERR_DEVICE_ON_TABLE            = -5,
    ADPDLIB_ERR_TIMEOUT_WAITING_FOR_PERSON = -6,
    ADPDLIB_ERR_SATURATED                  = -7,
    ADPDLIB_ERR_DATA_DRIFTED               = -8,
    ADPDLIB_ERR_ADJUSTED                   = -9,
    ADPDLIB_ERR_IN_PROGRESS                = -10,
    // Code < 100 should be the same as AdpdLibCommon.h //
    ADPDLIB_ERR_SYNC_ERR                   = -100,
    ADPDLIB_ERR_SYNC_BUFFERING             = -101,
    ADPDLIB_ERR_SYNC_OUTOFSYNC             = -102
} ADPDLIB_ERROR_CODE_t;

typedef struct LibraryAdpd400xResultX {
    int16_t HR;             /* 12.4 fixed pt format */
    int16_t confidence;     /* 6.10 fixed pt format */
    int16_t HR_Type;        /* 1- Track; 2- Spot; 0- No Algo */
    int16_t Activity;
    int16_t StepCount;
    int16_t RRinterval;
    uint16_t IsHrvValid;
} LibResultX_t;

typedef struct Adpd400xtimestamp {
    uint32_t tsADPD;
    uint32_t tsADXL;
    uint32_t tsAlgorithmCall;
} TimeStamps_t;

typedef struct Adpd400xagcStruct {
    uint32_t  timestamp;
    uint16_t  mts[6];
    uint16_t  setting[10];
} AGCStat_t;

/* New structure for LCFG parameters */
typedef struct Adpd400xLibConfig {
/*  AdpdLib2 specific parameters  */
/*  New elements to be added at the end of the structure  */
    uint16_t partNum;
    uint16_t targetSlots;
    uint8_t  targetChs;
    uint16_t deviceMode;
    uint16_t featureSelect;
    uint16_t drTime;
    uint32_t DutyCycle;
    uint16_t hrmInputRate; //If STATIC_AGC_EN flag is set, this becomes the ADPD frequency to be set
    uint8_t  syncMode;

    uint32_t proximityRate;
    uint16_t proximityTimeout;
    uint16_t proximityOnLevel;

    uint32_t detectionRate;
    uint16_t detectOntimeout;
    uint8_t  detectOnSettlingCnt;
    uint16_t triggerOnLevel;
    uint16_t triggerOnAirLevel;
    uint32_t triggerOnStablizeVR;
    uint16_t res16_2;

    uint8_t  detectOffSettlingCnt;
    uint8_t  triggerOffPercentage;
    uint32_t triggerOffStablizeVR;
    uint16_t res16_3;
    uint8_t  res8_1;

    uint16_t maxSamplingRate;
    uint8_t  targetDcPercent;
    uint16_t maxLedCurrent;
    uint8_t  maxPulseNum;
    uint8_t  floatModeCtr;
    uint8_t  ledG_Voltage;
    uint16_t modIndex;
    uint32_t motionThreshold;
    uint32_t motionCheckPeriod;
    uint32_t motionThresholdHigh;
    uint32_t motionCheckPeriodHigh;

    //uint8_t  ctrTh;
    uint16_t mt2Th;
    uint16_t mt3Th;

    uint16_t ambientChk;
    uint16_t ambientTh;         // 1-40

    /*ADI algo parameters*/
    int16_t  spotalgosamplerate;
    int16_t  spotalgodecimation;
    int16_t  mindifftrackSpot;
    int16_t  initialconfidencethreshold;
    uint32_t ppgscale;
    int16_t  accelscale;
    uint8_t  spotstabilitycount;

    int16_t  spothrtimeoutsecs;
    int16_t  zeroorderholdnumsamples;
    int16_t  trackalgosamplerate;
    int16_t  trackhrtimeoutsecs;
    uint32_t spotwindowlength;
    uint32_t trackerminheartratebpm;
    uint8_t  hrvEnable;

} Adpd400xLibConfig_t;

typedef enum {
    ADPDLIB_STAGE_START = 1,
    // ADPDLIB_STAGE_DETECT_INIT = 2,
    ADPDLIB_STAGE_DARKOFFSET_CALIBRATION = 3,
    ADPDLIB_STAGE_PROXIMITY_DETECT = 4,
    ADPDLIB_STAGE_DETECT_PERSON = 5,
    ADPDLIB_STAGE_HEART_RATE_INIT = 6,
    ADPDLIB_STAGE_HEART_RATE = 7,
    ADPDLIB_STAGE_GETCTR = 10,
    ADPDLIB_STAGE_FLOAT_MODE = 11,
    ADPDLIB_STAGE_OPTIMIZATION = 12,
    ADPDLIB_STAGE_END_CALIBRATION = 15,
    ADPDLIB_STAGE_DETECT_OFF = 20
} ADPDLIB_STAGES_t;

typedef enum {
    ADPD400xLIB_AFE_OPMODE_NORMAL = 1,
    ADPD400xLIB_AFE_OPMODE_FLOAT = 2
} Adpd400xLib_AFEMODE_t;

typedef struct Adpd400xLibStat {
  Adpd400xLib_AFEMODE_t AFE_OpMode;
  uint16_t CtrValue;
  uint16_t Others;
} Adpd400xLibStat_t;

/* External Variables -------------------------------------------------------*/

/* External Functions -------------------------------------------------------*/
extern uint32_t Adpd400xLibGetVersion();
extern uint32_t Adpd400xLibGetAlgorithmVersion();
extern uint8_t Adpd400xLibGetAlgorithmType();
extern uint8_t Adpd400xLibGetAlgorithmVendorAndVersion(uint8_t *nAlgoInfo);
extern ADPDLIB_ERROR_CODE_t Adpd400xLibOpenHr();
extern ADPDLIB_ERROR_CODE_t Adpd400xLibCloseHr();
extern void Adpd400xLibSetConfig(Adpd400xLibConfig_t *lcfg, uint32_t *dcfg);
extern ADPDLIB_ERROR_CODE_t Adpd400xLibGetHr(LibResultX_t *result,
                                  uint32_t *slotData,
                                  int16_t *acceldata,
                                  TimeStamps_t timeStamp);
extern int32_t Adpd400xLibGetMode(uint16_t *mode);
extern void Adpd400xLibSetFlag5Threshold(float);
extern void Adpd400xLibSetInitialTimeout(uint32_t);
extern void Adpd400xLibSetFinalTimeout(uint32_t);

extern void Adpd400xLibSetOnSettleCnt(uint8_t);
extern void Adpd400xLibSetOffSettleCnt(uint8_t);
extern void Adpd400xLibSetTriggerOnLevel(uint32_t);
extern void Adpd400xLibSetTriggerOnAirLevel(uint32_t);
extern void Adpd400xLibSetTriggerOnStablizeVR(uint32_t);
extern void Adpd400xLibSetTriggerOffCal(uint32_t);
extern void Adpd400xLibSetOffTriggerPercent(uint32_t);
extern void Adpd400xLibSetTriggerOffStablizeVR(uint32_t);
extern void Adpd400xLibSetTriggerLevel(uint32_t);

extern void Adpd400xLibSetTransitionFlag(uint8_t);
extern void Adpd400xLibSetSNRDisplayHRThreshold(float);
extern void Adpd400xLibSetFlagSNRSpotThreshold(float);
extern void Adpd400xLibSetSNRB2BThreshold(float);
extern void Adpd400xLibDelay(uint32_t delay);
extern uint32_t AdpdLibGetTick();
extern void adi_printf (uint8_t *FmtStrg, ...);
extern int32_t Adpd400xLibTestAlg(LibResultX_t *result,
                                uint32_t *slotA,
                                uint32_t *slotB,
                                int16_t *acceldata);
extern void Adpd400xLibGetLibStat_AFE_OP_MODE(Adpd400xLib_AFEMODE_t *afe_mode);
extern void Adpd400xLibGetLibStat_CTR_Value(uint16_t *ctr_val);
extern void Adpd400xLibGetLibStat_AGC_SIGM(uint16_t *sig_val);
extern uint8_t Adpd400xLibGetState(void);
extern ADPDLIB_ERROR_CODE_t Adpd400xLibGetStateInfo(uint8_t, uint16_t*);
extern void Adpd400xLibGetAgcState(AGCStat_t*);
extern uint8_t Adpd400xLibGetDetectOnValues(uint32_t *val,
                      uint32_t *valAir,
                      uint32_t *var);
extern void Adpd400xLibAdjestAmbient(void);  // adjuest for float mode Ambient.
extern void Adpd400xLibSyncInit();
extern int16_t *AdpdLibGetSyncAccel();
extern uint32_t AdpdLibGetSyncAccelTs();
extern uint32_t *AdpdLibGetSyncAdpd();
extern uint32_t AdpdLibGetSyncAdpdTs();
extern int32_t AdpdLibDoSyncV1(uint32_t    *slotACh,
             uint32_t    *slotData,
             uint32_t     tsADPD,
             int16_t     *acceldata,
             uint32_t     tsAccel);

extern void Adpd400xLibGetDarkOffsetInit(void);
extern ADPDLIB_ERROR_CODE_t Adpd400xLibGetDarkOffset(uint32_t*, uint16_t*);
extern void Adpd400xLibSetDarkOffset(void);
extern void Adpd400xLibApplyLCFG(Adpd400xLibConfig_t *lcfg);

extern void Adpd400xSetMode_RegCB(int16_t (*pfADPDSetMode)(uint8_t));
extern void PpgAdpd400xSetMode_RegCB(int16_t (*pfPpgSetMode)(uint8_t));
extern void Adpd400xSetSlot_RegCB(int16_t (*pfADPDSetSlot)(uint8_t, uint8_t));
extern void PpgAdpd400xLibSetSampleRate_RegCB(int8_t (*)(uint16_t, uint16_t, uint8_t));
extern void PpgAdpd400xLibGetSampleRate_RegCB(int8_t (*)(uint16_t*, uint16_t*, uint8_t));

#ifdef __cplusplus /* If this is a C++ compiler, end C linkage */
}
#endif

#endif /*__ADPDLIB_H__*/
