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
#ifndef __HEARTRATEINTERNAL_H__
#define __HEARTRATEINTERNAL_H__

#include <stdio.h>
#include <errno.h>
#ifndef NDEBUG
#include <stdio.h>
#endif
#include "adpd400x_lib.h"
#include "timers.h"
#include "logging.h"
#include "adpd_err_code.h"

#ifdef __cplusplus /* If this is a C++ compiler, use C linkage */
extern "C" {
#endif

#define CBDEC(b, d) ((b) % (d))
#define CBINC(b, d) ((b + 1) % (d))

#define SKIPSAMPLES (3)
#define DEFAULT_TIMEOUT ((uint32_t)(-1))
#define DEFAULT_MINTIMEOUT (500);
#define MINMEASURETIME (50)
#define MAXMEASURETIME (100)


/*
Algorithm Selection
    Bit field - 0: diabled; 1: Enabled
    0 - Morphological
    1 - HRV Calculation
    2 - Lower Rate support
    3 - Combined B2B ( Morphological + Tracking)

 */
#define ENABLE_MORPHOLOGICAL ((uint32_t)(1 << 0)) /* 0x01 */
#define ENABLE_HRV ((uint32_t)(1 << 1)) /* 0x02 */
#define ENABLE_LOWERSAMPLERATE ((uint32_t)(1 << 2)) /* 0x04 */
#define ENABLE_COMBOB2B ((uint32_t)(1 << 3)) /* 0x08 */

extern uint16_t gAdpd400x_dataRateRatio;

typedef struct LibraryResult {
    int16_t HR;             /* 12.4 fixed pt format */
    int16_t confidence;     /* 6.10 fixed pt format */
    int16_t HR_Type;        /* 1- Track; 2- Spot; 3 - Indicates state when Spot and Track are close and Spot HR is stable; 0- No Algo */
    int16_t RRinterval;
    uint8_t Activity;
    uint16_t IsHrvValid;
    uint16_t Mt1;   /* 100000x */
    uint16_t Mt2;
    uint16_t Mt3;
} LibResult_t;

typedef enum {
    ACCELEROMETERSCALE    =  0,
    MORPHOLOGICALHR       =  1,
    SENSORDECIMATION      =  2,
    PPGFILTER             =  3,
    GETB2BVALUE           =  4,
    HRVCALCULATION        =  5,
    HRSELECTION           =  6
} HR_Alg_API_t;

// Utility

extern uint32_t Adpd400xMwLibGetCurrentTime();

extern uint32_t AdpdMwLibDeltaTime(uint32_t start, uint32_t current);
extern int32_t AdpdMwLibStartTimer(const uint32_t slot);
extern int32_t AdpdMwLibStopTimer(const uint32_t slot, uint32_t *result);
extern int32_t AdpdMwLibCancelTimer(const uint32_t slot);
extern void AdpdMwLibStartCycleCount();
extern uint32_t AdpdMwLibStopCycleCount();

extern int32_t AdpdMwLibLoadConfigFromArray(uint32_t *in);
extern int32_t AdpdMwLibReadConfigToArray(uint32_t *in);
extern int32_t AdpdMwLibLoadConfig(uint32_t value);

// extern void AdpdLibPrintf(char *module, char *str);
extern void AdpdLibGetRTC(char *str);
extern void AdpdLibGetRTCTs(char *str);
extern uint32_t AdpdLibGetTick();
extern void AdpdLibDelay(uint32_t delay);
extern void AdpdLibMcuSleep(void);
// Utility

ADPDLIB_ERROR_CODE_t Adpd400xAlgHRProcess(uint32_t nSlotSumA,
                                  uint32_t nSlotSumB,
                                  int16_t *acceldata,
                                  LibResult_t *result,
                                  HR_Log_Result_t *g_resultlog);
extern void Adpd400xAlgHrFrontEnd_Reset(void);
extern void Adpd400xHeartRateSignalAlgNewSetting(void);

#ifdef __cplusplus /* If this is a C++ compiler, end C linkage */
}
#endif

#endif /*__HEARTRATEINTERNAL_H__*/
