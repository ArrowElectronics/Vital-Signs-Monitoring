/**
******************************************************************************
* @file     PedometerLib.h
* @author   ADI
* @version  V1.0.0
* @date     17-January-2017
* @brief    Include file for the Pedometer M2M application
******************************************************************************
* @attention
******************************************************************************
*/
/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2017 Analog Devices Inc.                                      *
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
* This software is intended for use with the pedometer application only       *
*                                                                             *
*                                                                             *
******************************************************************************/
#ifndef PEDOMETERLIB_H
#define PEDOMETERLIB_H

#define PED_ALGO_MAJOR_VERSION     1
#define PED_ALGO_MINOR_VERSION     0
#define PED_ALGO_PATCH_VERSION     0
#define PED_ALGO_DATA_RATE         25

// Pedometer return status
typedef enum {
  PEDOMETER_SUCCESS = 0x00,
  PEDOMETER_ERROR
}PEDOMETER_STATUS_ENUM_t;

typedef struct {
    uint32_t nPedometerSteps;
    uint16_t nPedometerStatus;
    uint32_t nTimeStamp;
} PedometerResult_t;

typedef struct {
  char      aNameStr[40];
  uint16_t  nMajor;
  uint16_t  nMinor;
  uint16_t  nPatch;
} PedometerAlgoVersion_t;

PEDOMETER_STATUS_ENUM_t PedometerOpenStep(void);
PEDOMETER_STATUS_ENUM_t PedometerGetStep(PedometerResult_t *ped_result, int16_t *acceldata);
PEDOMETER_STATUS_ENUM_t PedometerCloseStep();
uint16_t PedometerGetDataRate(void);
void PedometerLibGetAlgorithmVendorAndVersion(PedometerAlgoVersion_t *nAlgoInfo);

#endif // PEDOMETERLIB_H
