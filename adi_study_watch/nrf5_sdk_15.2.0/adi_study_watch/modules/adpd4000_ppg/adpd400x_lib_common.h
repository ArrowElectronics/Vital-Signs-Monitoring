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

/******************************************************************************/
/* Common header file for across all adpdlib functional related definitions */
/* Unlike adpd_lib.h, definition here are not expose to upper application    */

#ifndef __ADPDLIBCOMMON_H__
#define __ADPDLIBCOMMON_H__

#ifdef __cplusplus /* If this is a C++ compiler, use C linkage */
extern "C" {
#endif

#include "adpd400x_lib.h"
#include "adpd_err_code.h"
#include "adpd400x_reg.h"
#include "adpd400x_drv.h"
//#include "printf.h"

//#define BUF_SIZE (80)
//char _SBZ_[BUF_SIZE];
//#define ADPDDrv_Operation_Mode_t   ADPD400xDrv_Operation_Mode_t
#define ADPDDrv_Operation_Slot_t   ADPD400xDrv_FIFO_SIZE_t

//#define AdpdMwLibSetMode(a,b,c)    AdpdSetSlotCb(b,c); AdpdSetModeCB(a);  // optimize state w/o adxl
//#define AdpdMwLibSetMode(a, b ,c)   AdpdSetModeCB(a);  // optimize state w/o adxl
#define AdpdMwLibSetMode(a, b ,c)   PpgAdpd400xSetModeCB(a);
//#define PpgLibSetMode(a,b,c)        AdpdSetSlotCb(b,c); PpgSetModeCB(a);   // hrm state with adxl
#define PpgLibSetMode(a,b,c)        PpgAdpd400xSetModeCB(a);   // hrm state with adxl

#define PpgLibSetSampleRate(a,b)    PpgAdpd400xLibSetSampleRateCb(a, b);
#define PpgLibGetSampleRate(a,b)    PpgAdpd400xLibGetSampleRateCb(a, b);

#define PpgGetRegAddr(a,b)          log2(a) * 0x20 + b;

#define AdpdDrvRegWrite(a,b)        Adpd400xDrvRegWrite(a,b)
#define AdpdDrvRegRead(a,b)         Adpd400xDrvRegRead(a,b)
#define ADPDDrv_MODE_SAMPLE         ADPD400xDrv_MODE_SAMPLE
#define ADPDDrv_MODE_IDLE           ADPD400xDrv_MODE_IDLE

//#endif

//#define debug(M, ...)  {_SBZ_[0] = 0; \
//                        snprintf(_SBZ_, BUF_SIZE, ##__VA_ARGS__); \
//                        adi_printf("%s", _SBZ_);}

#ifndef ABSDIFF
#define ABSDIFF(a, b)   ((a>b) ? (a-b): (b-a))
#endif

// Original slot setting  //
typedef struct _adpdClSlotOri_t {
  uint16_t fs_low;              // fs Reg0D
  uint16_t fs_high;             // fs Reg0E
  uint16_t tia_Gain;            // Reg 104
  uint16_t led12_Cur;           // Reg 105
  uint16_t led34_Cur;           // Reg 106
  uint16_t led_Pulse;           // Reg 107
  uint16_t led_Width;           // Reg 109
  uint16_t adc_Offset1;         // Reg 10E
  uint16_t adc_Offset2;         // Reg 10F
  uint16_t decimate_val;        // Reg 112
  } AdpdClSlotOri_t;
extern AdpdClSlotOri_t gSlotOri;

// Led Calibration //
typedef struct _adpdOptimization_t {
  uint16_t ledB_Cur;
  uint16_t ledB_Trim;
  uint16_t ledB_Pulse;
  uint16_t tiaB_Gain;
  uint16_t ledB_CurVal;
  uint16_t ledB_FltWid;
  uint16_t sampleRate;
  uint16_t decimation;
  uint16_t darkOffset[4];
  uint16_t ledB_Cur2;
  uint16_t ledB_Trim2;
  uint16_t ledB_Pulse2;
  uint16_t tiaB_Gain2;
  uint16_t ledB_CurVal2;
  uint16_t sampleRate2;
  uint16_t decimation2;
  uint8_t  sigQuality;
  uint16_t Mt1;
  uint16_t Mt2;
  uint16_t Mt3;
  uint8_t SelectedLoop;
  uint8_t SelectedMode;
} AdpdOptimization_t;
extern AdpdOptimization_t gAdpd400xOptmVal;

// PPGLib status related
extern Adpd400xLibStat_t gAdpd400xPPGLibStatus;

// Detection related
typedef struct _adpd400x_detection_t {
  uint32_t detectOnValue;
  uint32_t detectOnAirLevel;
  uint32_t detectOnVariance;
  uint16_t proxOnLevel;
  uint32_t curDcLevelIR;
  uint32_t curDcLevelG;
} Adpd400xDetection_t;
extern Adpd400xDetection_t gAdpd400xDetectVal;

// Power related
typedef struct _adpdPowerUsage_t {
  uint16_t ledB_PW;
  uint16_t asic_PW;
  uint16_t total_PW;
} AdpdPowerUsage_t;
extern AdpdPowerUsage_t gAdpd400xPowerVal;

// Motion related
#define HIGH_MOTION     2
#define LOW_MOTION      1
#define NO_MOTION       0

typedef enum {
    MwErrPass = 0x0000,
    MwErrFail,
    MwErrProcessing,

    // LED Error Code
    MwErrLedAOutofRange = 0x0100,
    MwErrLedATrimOutofRange = 0x0200,
    MwErrLedBOutofRange = 0x0400,
    MwErrLedBTrimOutofRange = 0x0800
} MwLIB_ErrCode_t;

typedef struct _adpdClConfig_t {
  uint16_t addr;
  uint16_t value;
} AdpdClConfig_t;

/* External Variables -------------------------------------------------------*/
extern Adpd400xLibConfig_t *gAdpd400x_lcfg;
extern ADPDLIB_STAGES_t gAdpd400xPpgLibState;
extern uint32_t gAdpd400xSlotBChOrg[4], gAdpd400xSlotAChOrg[4];
extern int16_t gAdpd400xAdxlOrg[3];

extern uint32_t gAdpd400xPpgTimeStamp;
extern uint16_t gSlotAmode;
extern AGCStat_t gAdpd400xAGCStatInfo;
extern AdpdClConfig_t gAdpd400xFloatModeCfg[];
extern AdpdClConfig_t gAdpd400xNormalModeCfg[];
extern uint16_t gAdpd400xOpModeUsedReg[];

//////////////
extern uint16_t g_Adpd400xdeviceMode;
extern uint8_t g_Adpd400xchannelNums;
extern uint32_t gDcMean, gDcVariance;
extern uint32_t gnAdpd400xTempData[];       // Temporary buffer

/* External Functions -------------------------------------------------------*/
extern void Adpd400xSetModeCB(ADPD400xDrv_Operation_Mode_t);
extern void PpgAdpd400xSetModeCB(ADPD400xDrv_Operation_Mode_t);
extern void Adpd400xSetSlotCb(ADPD400xDrv_Operation_Mode_t, ADPD400xDrv_Operation_Mode_t);
extern void PpgAdpd400xLibSetSampleRateCb(uint16_t rate, uint16_t decimation);
extern void PpgAdpd400xLibGetSampleRateCb(uint16_t* rate, uint16_t* decimation);
extern void Adpd400xSetChannelFilter(uint16_t slotX_ch1, int16_t usedChValue);
extern void Adpd400xUtilRecordCriticalSetting(void);
extern void Adpd400xUtilRestoreCriticalSetting(void);
extern uint16_t Adpd400xUtilGetPulseNum(uint8_t slotNum);
extern INT_ERROR_CODE_t Adpd400xUtilSetPulseNum(uint8_t, uint16_t, uint8_t);

// external function prototype
extern INT_ERROR_CODE_t Adpd400xStateMachineInit(void);
extern INT_ERROR_CODE_t Adpd400xStateMachineDeInit(void);

// from detectOn.c //
extern void Adpd400xDetectObjectOnInitIR(void);
extern void Adpd400xDetectObjectOnDeInitIR(void);
extern INT_ERROR_CODE_t DetectObjectOnIR(uint32_t *rawA);

extern void Adpd400xDetectObjectOnInitGreen(void);
extern void Adpd400xDetectObjectOnDeInitGreen();
extern INT_ERROR_CODE_t Adpd400xDetectObjectOnGreen(uint32_t *rawB);

extern void DetectProximityInit(void);
extern void DetectProximityDeInit(void);
extern INT_ERROR_CODE_t DetectProximity(void);

// from FloatMode.c //
extern void FloatModeInit(void);
extern INT_ERROR_CODE_t Adpd400xSetFloatMode(uint32_t *);
extern void Adpd400xFloatModeDeInit(void);
extern void Adpd400xFloatModeAdjestAmbient(void);

// from LEDCalibration.c //
extern INT_ERROR_CODE_t AdpdMwLibDoLedCalInit(void);
extern INT_ERROR_CODE_t AdpdMwLibDoLedCal(uint32_t*);
extern void AdpdMwLibDoLedCalDeInit(void);

extern INT_ERROR_CODE_t AdpdMwLibDoLedCalInitDim(void);
extern INT_ERROR_CODE_t AdpdMwLibDoLedCalDim(uint32_t *rawB);

// from detectOff.c //
extern void Adpd400xDetectObjectOffInit(void);
extern INT_ERROR_CODE_t Adpd400xDetectObjectOFF(uint32_t mean_val, uint32_t var);

// functions from util.c //
extern uint32_t Adpd400xMwLibGetCurrentTime(void);
extern uint32_t AdpdMwLibDeltaTime(uint32_t start, uint32_t current);
extern void Adpd400xUtilGetMeanVarInit(uint8_t avg_size, uint8_t get_var);
extern INT_ERROR_CODE_t Adpd400xUtilGetMeanVar(uint32_t*, uint32_t*, uint32_t*);
extern uint16_t Adpd400xUtilGetCurrentFromSlot(uint8_t slotNum);
extern uint16_t Adpd400xUtilGetCurrentFromReg(uint16_t reg12, uint16_t reg34);
extern uint8_t Adpd400xUtilGetCurrentRegValue(uint16_t cur, uint16_t* cR, uint16_t* fR);
extern void Adpd400xUtilSetLoop1Config(void);
extern void Adpd400xStoreOpModeSetting(void);
extern void Adpd400xApplyOpModeSetting(void);

// functions from other
extern void Adpd400xMDResetTimers(void);
/////////////////////

#ifdef __cplusplus /* If this is a C++ compiler, end C linkage */
}
#endif

#endif /*__ADPDLIBCOMMON_H__*/
