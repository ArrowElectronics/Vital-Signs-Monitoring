/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2021 Analog Devices Inc.                                      *
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
* This software is intended for use with the ADPD4x00 and derivative parts    *
* only                                                                        *
*                                                                             *
* HRMADPDLIBVERSIONNUMBER   (0x00010000)                                      *
* 2021/05/13: ADPDlib with Dynamic AGC support for HRM application            *
* HRMADPDLIBVERSIONNUMBER   (0x00010001)                                      *
* 2021/06/09:Added Support in library for AGC stream info update through      *
*            Adpd400xUpdateAGCInfoSettings API, it will be used for           *
*            AGC recalibration and saturation                                 *
******************************************************************************/
#ifndef __ADPD400xLIB_H__
#define __ADPD400xLIB_H__

#include <stdint.h>
#include "sdk_config.h"
#include "nrf_log_ctrl.h"

#ifdef __cplusplus /* If this is a C++ compiler, use C linkage */
extern "C" {
#endif


#define HRMPPGLIBVERSIONNUMBER      (0x00010001) // 1.0.1
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
#define DETECT_ON_BIT_P         0
#define DETECT_ON_EN            ((uint16_t)(1 << DETECT_ON_BIT_P))    /* 0x01 */

#define DETECT_OFF_BIT_P        1
#define DETECT_OFF_EN           ((uint16_t)(1 << DETECT_OFF_BIT_P))    /* 0x02 */

#define DYNAMIC_AGC_BIT_P       8
#define DYNAMIC_AGC_EN          ((uint16_t)(1 << DYNAMIC_AGC_BIT_P))    /* 0x100 */

#define STATIC_AGC_BIT_P        9
#define STATIC_AGC_EN           ((uint16_t)(1 << STATIC_AGC_BIT_P))    /* 0x200 */

#define HR_ALGO_BIT_P           12
#define HR_ALGO_EN              ((uint16_t)(1 << HR_ALGO_BIT_P))   /* 0x1000 */

/* Target Channel input options*/
#define TARGET_CH1 1 /* ch1-> PD1 ch2-> PD2 ,ch1 data will be fed to HRM*/
#define TARGET_CH2 2 /* ch1-> PD1 ch2-> PD2 ,ch2 data will be fed to HRM*/
#define TARGET_CH3 3 /* ch1-> PD1+PD2-> ,ch1 data will be fed to HRM (Analog Sum)*/
#define TARGET_CH4 4 /* ch1-> ch1 >> sift factor + ch2 >> shift factor ,ch1 data will be fed to HRM (Digital Sum)*/ 

/*Maximum Gain settings for ch1 and ch2*/
#define CH1_GAIN_MAX_REG_VAL 4
#define CH2_GAIN_MAX_REG_VAL 32

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
    ADPDLIB_ERR_HRM_TIMEOUT                = -11,
    ADPDLIB_ERR_ADC_SATURATION             = -13,// Due to pulse 
    ADPDLIB_ERR_AFE_SATURATION             = -14,// no variance 
    ADPDLIB_ERR_ALGO_INPUT_OVERFLOW        = -15,// algo input limit exceeded 
    ADPDLIB_ERR_STATIC_AGC_ADJUSTED        = -16,
    // Code < 100 should be the same as AdpdLibCommon.h //
    ADPDLIB_ERR_SYNC_ERR                   = -100,
    ADPDLIB_ERR_SYNC_BUFFERING             = -101,
    ADPDLIB_ERR_SYNC_OUTOFSYNC             = -102
} ADPDLIB_ERROR_CODE_t;

/* enum similar to what we have in adi_vsm_hrv.h */
typedef enum {
ADPDLIB_HRV_PPG_B2B_VALID_RR = 1,
ADPDLIB_HRV_PPG_B2B_GAP = 8
} ADPDLIB_PPG_HRV_FLAG_t; 

typedef struct LibraryAdpd400xResultX {
    int16_t HR;             /* 12.4 fixed pt format */
    int16_t confidence;     /* 6.10 fixed pt format */
    int16_t HR_Type;        /* 1- Track; 2- Spot; 0- No Algo */
    int16_t Activity;
    int16_t StepCount;
    int16_t RRinterval;
    uint16_t IsHrvValid;
    uint16_t RMSSD;
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

typedef struct AlgoRawDataStruct {
  int32_t PPG_algo;  
  int32_t X_algo;
  int32_t Y_algo;
  int32_t Z_algo;
} AlgoRawDataStruct_t;

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

    uint32_t staticAgcRecalTime;
    uint16_t Res16_1;
    uint8_t  Res8_1;
    uint16_t Res16_2;
    uint16_t Res16_3;
    uint32_t Res32_2;
    uint16_t initialLedPulse;

    uint8_t  Res8_2;
    uint8_t  Res8_3;
    uint32_t Res32_3;
    uint16_t rmssdSampleWindow;
    uint8_t  Res8_4;
    uint16_t Res16_4;
    uint16_t Res16_5;

    uint16_t maxSamplingRate;
    uint8_t  targetDcPercent;
    uint16_t maxLedCurrent;
    uint8_t  maxPulseNum;
    uint8_t  satAdjustPercentForStaticAgc;
    uint8_t  Res8_5;
    uint16_t InitialCurrentTiaGain;
    uint32_t motionThreshold;
    uint32_t motionCheckPeriod;
    uint32_t motionThresholdHigh;
    uint32_t motionCheckPeriodHigh;

    uint8_t  Res8_6;
    uint16_t Res16_6;
    uint16_t Res16_7;

    uint16_t sqiLowPowerThreshold;
    uint16_t sqiHighPowerThreshold;

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

/*Enum for AGC stream info - setting[0] field*/
typedef enum {
  ADPD400xLIB_AGCLOG_START = 1,//!< Indicates AGC started for collecting samples
  ADPD400xLIB_AGCLOG_STOP_NORMAL = 2,//!< Indicates  AGC settings applied
  ADPD400xLIB_AGCLOG_STOP_MOTION = 3,//!< Indicates AGC stopped due to motion in between collecting the samples
  ADPD400xLIB_AGCLOG_ADC_SATURATION = 4,//!< Indicates ADPD signal DC level exceeds 600000
  ADPD400xLIB_AGCLOG_AFE_SATURATION = 5,//!< Indicates ADPD has AFE saturation
  ADPD400xLIB_AGCLOG_LOOP1 = 6,//!< Indicates staticAGC(loop1) settings applied during high motion
  ADPD400xLIB_AGCLOG_STATIC_AGC_FIRST_CAL = 7,//!< Indicates Initial staticAGC calibration settings applied
  ADPD400xLIB_AGCLOG_STATIC_AGC_RECAL = 8//!< Indicates staticAGC Re-calibration settings applied
} ADPD400xLIB_AGCLOG_Indicator_t;

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
extern void Adpd400xLibGetLibStat_CTR_Value(uint16_t *ctr_val);
extern void Adpd400xLibGetLibStat_AGC_SIGM(uint16_t *sig_val);
extern uint8_t Adpd400xLibGetState(void);
extern ADPDLIB_ERROR_CODE_t Adpd400xLibGetStateInfo(uint8_t, uint16_t*);
extern void Adpd400xLibGetAgcState(AGCStat_t*);
extern void Adpd400xLibUpdateAGCStateInfo(ADPD400xLIB_AGCLOG_Indicator_t agcindicator);
extern void Adpd400xLibGetAlgoRawData(AlgoRawDataStruct_t*);
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
extern void AdpdLibGetAgcState(AGCStat_t* agcInfo);

extern void Adpd400xSetMode_RegCB(int16_t (*pfADPDSetMode)(uint8_t));
extern void PpgAdpd400xSetMode_RegCB(int16_t (*pfPpgSetMode)(uint8_t));
extern void Adpd400xSetSlot_RegCB(int16_t (*pfADPDSetSlot)(uint8_t, uint8_t));
extern void PpgAdpd400xLibSetSampleRate_RegCB(int8_t (*)(uint16_t, uint16_t, uint16_t));
extern void PpgAdpd400xLibGetSampleRate_RegCB(int8_t (*)(uint16_t*, uint16_t*, uint16_t));

#ifdef __cplusplus /* If this is a C++ compiler, end C linkage */
}
#endif

#endif /*__ADPDLIB_H__*/
