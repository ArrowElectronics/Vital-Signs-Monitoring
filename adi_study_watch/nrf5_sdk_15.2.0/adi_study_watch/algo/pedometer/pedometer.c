/*
    ****************************************************************************
    * @file     Pedometer.c
    * @author   ADI
    * @version  V1.0
    * @date     18-January-2017
    ****************************************************************************
    * @attention
    ****************************************************************************
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

#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <inttypes.h>
#include "pedometer_lib.h"
#include "hip_algorithm.h"
#include "adxl362.h"
#include "math.h"


/* pedometer related variables */
static struct _pedometer_lib_control_t {
  uint8_t nIndex25Hz;
  uint8_t nDecFactor;
  uint8_t nLog2DecFactor;
  int32_t nXData;
  int32_t nYData;
  int32_t nZData;
} gsPedometerLibControl = {0, 0, 0, 0, 0,0};

PEDOMETER_STATUS_ENUM_t PedometerGetStep(PedometerResult_t *pedometer_result,
                         int16_t *acceldata) {

    int32_t nNumSteps;
    int16_t nAlgoStatus;
    PEDOMETER_STATUS_ENUM_t nReturn = PEDOMETER_SUCCESS;

    gsPedometerLibControl.nXData += acceldata[0];
    gsPedometerLibControl.nYData += acceldata[1];
    gsPedometerLibControl.nZData += acceldata[2];
    /* Pedometer needs data at 25Hz, so downsampling by 4 is required */
    if (++gsPedometerLibControl.nIndex25Hz >= gsPedometerLibControl.nDecFactor) {
        gsPedometerLibControl.nIndex25Hz = 0;
        gsPedometerLibControl.nXData >>= gsPedometerLibControl.nLog2DecFactor;
        gsPedometerLibControl.nYData >>= gsPedometerLibControl.nLog2DecFactor;
        gsPedometerLibControl.nZData >>= gsPedometerLibControl.nLog2DecFactor;


        // This function calculates number of steps by using X, Y and Z
        //   values of accelerometer
        // algoStatus is a variable to know the status of the algorithm,
        nNumSteps = StepAlgorithm_hip(&nAlgoStatus,
                                 (int16_t)gsPedometerLibControl.nXData,
                                 (int16_t)gsPedometerLibControl.nYData,
                                 (int16_t)gsPedometerLibControl.nZData);
        gsPedometerLibControl.nXData = gsPedometerLibControl.nYData = gsPedometerLibControl.nZData = 0;

        pedometer_result->nPedometerSteps = nNumSteps;
        pedometer_result->nPedometerStatus = nAlgoStatus;
    }
    return nReturn;
}

PEDOMETER_STATUS_ENUM_t PedometerOpenStep(void) {
    // Initialize the pedometer algorithm parameters
    InitAlgorithmParameters_hip(0);
    // reset the buffers
    gsPedometerLibControl.nIndex25Hz = 0;
    gsPedometerLibControl.nXData = 0;
    gsPedometerLibControl.nYData = 0;
    gsPedometerLibControl.nZData = 0;
    return PEDOMETER_SUCCESS;
}

PEDOMETER_STATUS_ENUM_t PedometerCloseStep() {
   return PEDOMETER_SUCCESS;
}

/**
* @brief    PedometerGetDataRate function will set PED DecFactor  and  return ADXL ODR
* @param    None
* @retval   Pedometer ODR
*/
uint16_t PedometerGetDataRate(void) {

  uint16_t nPedometerODR = 0;

  // This driver call needs to be replaced with an m2m2 message as soon as the PS is running full m2m2
  AdxlDrvGetParameter(ADXL_ODRRATE, &nPedometerODR);
  nPedometerODR = (((1 << nPedometerODR) * 25 ) >> 1);
  // The equation has to be modified for diff accelerometers. but mostly all ADI
  //   acceleromers will have similar register settings. And to support more than
  //  one accelerometer, a middleware layer has to be written
  if (nPedometerODR > 400) {
    nPedometerODR = 400;  // as per ADXL data sheet page 33, Table 17
  }
    gsPedometerLibControl.nDecFactor = nPedometerODR /PED_ALGO_DATA_RATE;
    gsPedometerLibControl.nLog2DecFactor = (uint8_t)log2(gsPedometerLibControl.nDecFactor);
    return nPedometerODR;
}
