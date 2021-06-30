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
#ifndef __OPTIMIZATION_H__
#define __OPTIMIZATION_H__

#include <stdlib.h> /* defines NULL */
#include <string.h> /* memset */
#include <stdint.h>

#define SAMPLE_RATE 50

#ifdef __cplusplus /* If this is a C++ compiler, use C linkage */
extern "C" {
#endif

/* convert from 12.4 fract to float */
#define FIXED16Q4_TO_FLOAT(x) (((float)(x))/16.0f)
/* convert from 6.10 fract to float */
#define FIXED16Q10_TO_FLOAT(x) (((float)(x))/1024.0f)
/* convert from 12.20 fract to float */
#define FIXED12Q20_TO_FLOAT(x) (((float)(x))/1048576.0f)
/* convert from 2.30 fract to float */
#define FIXED2Q30_TO_FLOAT(x) (((float)(x))/(float)(1<<30))
/* convert from float to 12.20 fract */
#define FLOAT_TO_FIXED12Q20(x) (int)((float)(x)*1048576.0f)
/* Convert from float to 6.10 fract */
#define FLOAT_TO_FIXED6Q10(x) (int)((float)(x)*(1<<10))
/* convert from float to 10.6 fract */
#define FLOAT_TO_FIXED10Q6(x) (int16_t)((x)*(float)(1<<6))
/* convert from float to 2.14 fract */
#define FLOAT_TO_FIXED2Q14(x) (int16_t)((x)*(float)(1<<14))

typedef struct signalMetrics {
    int32_t Mt1;
    int32_t Mt2;
    int32_t Mt3;
} SignalMetrics_t;

typedef enum {
    SignalQt_Lowest = 0,
    SignalQt_Bad,
    SignalQt_Good = 5,
    SignalQt_Great,
    SignalQt_Excellent,
    SignalQt_Highest = 10,
} SignalQt_t;

typedef enum {
    SMrtc_Error = -1,
    SMrtc_Pass = 0,
    SMrtc_InProgress,
    SMrtc_PreCondition_NonCausal, //waiting for noncausal samples
    SMrtc_PreCondition_Causal, //waiting for causal samples
} SM_RetCode_t;

typedef enum {
  ACTIVITY_FAILED = -1,
  ACTIVITY_SUCCESS = 0,
  ACTIVITY_INPROCESS = 1
} Activity_RetCode_t;

SM_RetCode_t GetSignalMetrics(uint32_t*, SignalMetrics_t*, SignalQt_t*);
SM_RetCode_t Preprocess_Metrics_Init(uint16_t, uint16_t, uint16_t);
void Preprocess_Metrics_Reset(void);

void Preprocess_Activity_Init(uint16_t sample_rate);
void Preprocess_Activity_Reset(void);
Activity_RetCode_t Preprocess_Activity_Run(int32_t accelx,
                            int32_t accely,
                            int32_t accelz,
                            int32_t *activity);

#ifdef __cplusplus /* If this is a C++ compiler, end C linkage */
}
#endif

#endif /*__ADPDLIB_H__*/
