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
#ifndef __ADPDERRCODE_H__
#define __ADPDERRCODE_H__

#ifdef __cplusplus /* If this is a C++ compiler, use C linkage */
extern "C" {
#endif

#ifndef RELEASE_PRODUCTION
#define _BUF_SIZE (256)
extern char _SBZ[_BUF_SIZE];
#endif  // RELEASE_PRODUCTION

#ifdef NDEBUG
#define debug(M, ...)
#else
//#define debug(N, M, ...)  {SBZ[0] = 0; snprintf(SBZ, _BUF_SIZE, ":I %d: " M "\r\n", __LINE__, ##__VA_ARGS__); AdpdLibPrintf(N, SBZ);}
#define debug(N, M, ...)  {_SBZ[0] = 0; \
            snprintf(_SBZ, _BUF_SIZE, ":I %d: " M "\r\n", __LINE__, ##__VA_ARGS__); \
            adi_printf("%s %s", N, _SBZ);}
#endif

typedef enum {
    IERR_FAIL                       =  2,
    IERR_SUCCESS_WITH_RESULT        =  1,
    IERR_SUCCESS                    =  0,
    IERR_ON_SENSOR                  = -1,
    IERR_DEVICE_ON_MOTION           = -2,
    IERR_OFF_SENSOR                 = -3,
    IERR_HARD_PRESSURE              = -4,
    IERR_DEVICE_ON_TABLE            = -5,
    IERR_TIMEOUT_WAITING_FOR_PERSON = -6,
    IERR_SATURATED                  = -7,
    IERR_DATA_DRIFTED               = -8,
    IERR_AGC_ADJUSTED               = -9,
    IERR_IN_PROGRESS                = -10,
    IERR_TIMEOUT                    = -11,
    IERR_HRM_TIMEOUT                = -12,
    IERR_ADC_SATURATION             = -13,// Due to pulse 
    IERR_AFE_SATURATION             = -14,// no variance 
    IERR_ALGO_INPUT_OVERFLOW        = -15,// algo input limit exceeded
    IERR_STATIC_AGC_ADJUSTED        = -16,
    // IERR_IN_PROGRESS                = -100,
    // IERR_TIMEOUT                    = -101,
    IERR_NULL_ERROR                 = -102,
    IERR_DONE                       = -103
} INT_ERROR_CODE_t;

#ifdef __cplusplus /* If this is a C++ compiler, end C linkage */
}
#endif

#endif /*__ADPDERRCODE_H__*/
