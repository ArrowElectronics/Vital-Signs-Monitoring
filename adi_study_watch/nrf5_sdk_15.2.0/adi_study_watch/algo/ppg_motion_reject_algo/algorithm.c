/**
***************************************************************************
* @file         algorithm.c
* @author       ADI
* @version      V1.0.0
* @date         14-May-2014
* @brief        Sample showing how to use the HRM Library.
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
* This software is intended for use with the ADPD and derivative parts     *
* only                                                                        *
*                                                                             *
******************************************************************************/
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include "AdpdLib.h"
#include "HeartRateInternal.h"

int16_t AlgHRInit() {
    return 0;
}

ADPDLIB_ERROR_CODE_t AlgHRProcess(uint32_t R,
                                  uint32_t IR,
                                  int16_t *acceldata,
                                  LibResult_t *result,
                                  HR_Log_Result_t *g_resultlog) {
    static uint32_t i = 0;
    i+=16;

    if (i >= 1600)
        i = 16;

    result->HR = 1600 + i;
    g_resultlog->HR_SELECTED = result->HR;

    return ADPDLIB_ERR_SUCCESS_WITH_RESULT;
}

void AlgHrFrontEnd_Reset()  {
  return;
}

#if MCU == M3
uint32_t get_algorithm_version(void) {
    return 0;
}
#endif
