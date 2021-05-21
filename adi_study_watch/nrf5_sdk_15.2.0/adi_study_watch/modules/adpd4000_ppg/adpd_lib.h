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
#ifndef __ADPDLIB_H__
#define __ADPDLIB_H__

#include <stdint.h>

#ifdef __cplusplus /* If this is a C++ compiler, use C linkage */
extern "C" {
#endif

#define HRMADPDLIBVERSIONNUMBER   (0x00040000)
#define HRMALGORITHMVERSIONNUMBER (0x00010200) // 1.2.0

/* Macro */
/*
    Bit field - 0: do not skip; 1: skip
    0 - Detection
    1 - LED Calibraiton
    2 - DOC Calibration
    3 - Heart Rate
    4 - Object Detection
 */
#define SKIP_DETECT_ON          ((uint16_t)(1 << 0)) /* 0x01 */
#define SKIP_DETECT_OFF         ((uint16_t)(1 << 1)) /* 0x02 */
// #define SKIP_PROXIMITY       ((uint16_t)(1 << 2)) /* 0x04 */
// #define SKIP_DARKOFFSET      ((uint16_t)(1 << 3)) /* 0x08 */
#define SKIP_LED_CAL            ((uint16_t)(1 << 4)) /* 0x10 */
#define SKIP_SAMPLERATEADJUST   ((uint32_t)(1 << 5)) /* 0x20*/
#define SKIP_FLOATMODE          ((uint32_t)(1 << 6)) /* 0x40*/
#define SKIP_AGC_FEATURE        ((uint16_t)(1 << 7)) /* 0x80 */
#define SKIP_OPTIMIZE_SETTING   ((uint16_t)(1 << 8)) /* 0x100 */
#define SKIP_MEDIAN_FILTER      ((uint16_t)(1 << 9)) /* 0x200 */
// #define SKIP_DRIFT_ADJUSTMENT   ((uint16_t)(1 << 10)) /* 0x400 */
#define SKIP_AMBIENT_CHECK      ((uint16_t)(1 << 11)) /* 0x800 */
#define SKIP_HR                 ((uint16_t)(1 << 14)) /* 0x4000 */
#define SKIP_ALL_CAL            ((uint16_t)(1 << 15)) /* 0x8000 */

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

typedef struct LibraryResultX {
    int16_t HR;             /* 12.4 fixed pt format */
    int16_t confidence;     /* 6.10 fixed pt format */
    int16_t HR_Type;        /* 1- Track; 2- Spot; 0- No Algo */
    int16_t Activity;
    int16_t StepCount;
    int16_t RRinterval;
    uint16_t IsHrvValid;
} LibResultX_t;

typedef struct timestamp {
    uint32_t tsADPD;
    uint32_t tsADXL;
    uint32_t tsAlgorithmCall;
} TimeStamps_t;

typedef struct agcStruct {
    uint32_t  timestamp;
    uint16_t  mts[6];
    uint16_t  setting[10];
} AGCStat_t;

/* New structure for LCFG parameters */
typedef struct AdpdLibConfig {
/*  AdpdLib2 specific parameters  */
/*  New elements to be added at the end of the structure  */
    uint16_t partNum;
    uint8_t  targetChs;
    uint16_t devicemode;
    uint16_t skipstate;
    uint16_t drTime;
    uint32_t res32_1;
    uint16_t hrmInputRate;
    uint8_t  syncmode;

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

    uint8_t  driftPercent;
    uint8_t  driftInterval;

    uint16_t maxSamplingRate;
    uint8_t  saturateAdjustPercent;
    uint16_t ledMaxCurrent;
    uint8_t  maxPulseNum;
    uint8_t  floatModeCtr;
    uint8_t  ledB_Vol;
    uint8_t  dcLevelPercentA;
    uint16_t modIndex;
    uint32_t motionThreshold;
    uint32_t motionCheckPeriod;
    uint32_t motionThresholdHigh;
    uint32_t motionCheckPeriodHigh;

    uint8_t  ctrTh;
    uint16_t mt2Th;
    uint16_t mt3Th;

    uint16_t ambientChk;
    uint16_t ambientTh;

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

} AdpdLibConfig_t;

typedef enum {
    ADPDLIB_STAGE_START = 1,
    ADPDLIB_STAGE_DETECT_INIT = 2,
    ADPDLIB_STAGE_DARKOFFSET_CALIBRATION = 3,
    ADPDLIB_STAGE_PROXIMITY_DETECT = 4,
    ADPDLIB_STAGE_DETECT_PERSON = 5,
    ADPDLIB_STAGE_HEART_RATE_INIT = 6,
    ADPDLIB_STAGE_HEART_RATE = 7,
    ADPDLIB_STAGE_LEDCAL_INIT = 8,
    ADPDLIB_STAGE_LEDCAL = 9,
    ADPDLIB_STAGE_GETCTR = 10,
    ADPDLIB_STAGE_FLOAT_MODE = 11,
    ADPDLIB_STAGE_OPTIMIZATION = 12,
    ADPDLIB_STAGE_SAMPLE_RATE_OPT = 13,
    ADPDLIB_STAGE_OPT_SELECT = 14,
    ADPDLIB_STAGE_END_CALIBRATION = 15,
    ADPDLIB_STAGE_DETECT_OFF = 20
} ADPDLIB_STAGES_t;

typedef enum {
    ADPDLIB_AFE_OPMODE_NORMAL = 1,
    ADPDLIB_AFE_OPMODE_FLOAT = 2
} AdpdLib_AFEMODE_t;

typedef struct AdpdLibStat {
  AdpdLib_AFEMODE_t AFE_OpMode;
  uint16_t CtrValue;
  uint16_t Others;
} AdpdLibStat_t;

/* External Variables -------------------------------------------------------*/

/* External Functions -------------------------------------------------------*/
extern uint32_t AdpdLibGetVersion();
extern uint32_t AdpdLibGetAlgorithmVersion();
extern uint8_t AdpdLibGetAlgorithmType();
extern uint8_t AdpdLibGetAlgorithmVendorAndVersion(uint8_t *nAlgoInfo);
extern ADPDLIB_ERROR_CODE_t AdpdLibOpenHr();
extern ADPDLIB_ERROR_CODE_t AdpdLibCloseHr();
extern void AdpdLibSetConfig(AdpdLibConfig_t *lcfg, uint32_t *dcfg);
extern ADPDLIB_ERROR_CODE_t AdpdLibGetHr(LibResultX_t *result,
                                  uint32_t *slotBCh,
                                  int16_t *acceldata,
                                  TimeStamps_t timeStamp);
extern int32_t AdpdLibGetMode(uint16_t *mode);
extern void AdpdLibSetFlag5Threshold(float);
extern void AdpdLibSetInitialTimeout(uint32_t);
extern void AdpdLibSetFinalTimeout(uint32_t);

extern void AdpdLibSetOnSettleCnt(uint8_t);
extern void AdpdLibSetOffSettleCnt(uint8_t);
extern void AdpdLibSetTriggerOnLevel(uint32_t);
extern void AdpdLibSetTriggerOnAirLevel(uint32_t);
extern void AdpdLibSetTriggerOnStablizeVR(uint32_t);
extern void AdpdLibSetTriggerOffCal(uint32_t);
extern void AdpdLibSetOffTriggerPercent(uint32_t);
extern void AdpdLibSetTriggerOffStablizeVR(uint32_t);
extern void AdpdLibSetTriggerLevel(uint32_t);

extern void AdpdLibSetTransitionFlag(uint8_t);
extern void AdpdLibSetSNRDisplayHRThreshold(float);
extern void AdpdLibSetFlagSNRSpotThreshold(float);
extern void AdpdLibSetSNRB2BThreshold(float);
extern void AdpdLibDelay(uint32_t delay);
extern uint32_t AdpdLibGetTick();
extern void adi_printf (uint8_t *FmtStrg, ...);
extern int32_t AdpdLibTestAlg(LibResultX_t *result,
                                uint32_t *slotA,
                                uint32_t *slotB,
                                int16_t *acceldata);
extern void AdpdLibGetLibStat_AFE_OP_MODE(AdpdLib_AFEMODE_t *afe_mode);
extern void AdpdLibGetLibStat_CTR_Value(uint16_t *ctr_val);
extern void AdpdLibGetLibStat_AGC_SIGM(uint16_t *sig_val);
extern uint8_t AdpdLibGetState(void);
extern ADPDLIB_ERROR_CODE_t AdpdLibGetStateInfo(uint8_t, uint16_t*);
extern void AdpdLibGetAgcState(AGCStat_t*);
extern uint8_t AdpdLibGetDetectOnValues(uint32_t *val,
                      uint32_t *valAir,
                      uint32_t *var);
extern void AdpdLibAdjestAmbient(void);  // adjuest for float mode Ambient.
extern void AdpdLibSyncInit();
extern int16_t *AdpdLibGetSyncAccel();
extern uint32_t AdpdLibGetSyncAccelTs();
extern uint32_t *AdpdLibGetSyncAdpd();
extern uint32_t AdpdLibGetSyncAdpdTs();
extern int32_t AdpdLibGetB2bType();
extern uint8_t AdpdLibGetSpotFlag();
extern int32_t AdpdLibDoSyncV1(uint32_t    *slotACh,
             uint32_t    *slotBCh,
             uint32_t     tsADPD,
             int16_t     *acceldata,
             uint32_t     tsAccel);

extern void AdpdLibGetDarkOffsetInit(void);
extern ADPDLIB_ERROR_CODE_t AdpdLibGetDarkOffset(uint32_t*, uint16_t*);
extern void AdpdLibSetDarkOffset(void);
extern void AdpdLibApplyLCFG(AdpdLibConfig_t *lcfg);

extern void AdpdSetMode_RegCB(int16_t (*pfADPDSetMode)(uint8_t));
extern void PpgSetMode_RegCB(int16_t (*pfPpgSetMode)(uint8_t));
extern void AdpdSetSlot_RegCB(int16_t (*pfADPDSetSlot)(uint8_t, uint8_t));
extern void PpgLibSetSampleRate_RegCB(int8_t (*)(uint16_t, uint16_t));
extern void PpgLibGetSampleRate_RegCB(int8_t (*)(uint16_t*, uint16_t*));

#ifdef __cplusplus /* If this is a C++ compiler, end C linkage */
}
#endif

#endif /*__ADPDLIB_H__*/
