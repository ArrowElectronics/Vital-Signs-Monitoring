/**
******************************************************************************
* @file     app_bcm.h
* @author   ADI
* @version  V1.0.0
* @date     10-October-2017
* @brief    Include file for the BIA application internal functions
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
* This software is intended for use with the BIA                              *
* only                                                                        *
*                                                                             *
******************************************************************************/
#ifndef _APP_BIA_H
#define _APP_BIA_H

#include <bia_application_interface.h>
#include <sensor_internal.h>
#define BIA_BUFFER_SIZE               4

typedef enum {
  BIA_ERROR = 0,
  BIA_SUCCESS = 1,
} BIA_ERROR_CODE_t;

typedef enum {
  BIA_LCFG_FS = 0,
  BIA_LCFG_ADC_PGA_GAIN,
  BIA_LCFG_PWR_MOD,
  BIA_LCFG_SIN_FREQ,
  BIA_DFT_NUM,
  BCM_FFM_DATA_RATE,
  BCM_ALGO_ENABLE,
  BCM_ALGO_HEIGHT,
  BCM_ALGO_WEIGHT,
  BCM_ALGO_AGE,
  BCM_ALGO_COEFF_CONFIG_ZERO,
  BCM_ALGO_COEFF_CONFIG_ONE,
  BCM_ALGO_COEFF_CONFIG_TWO,
  BCM_ALGO_COEFF_CONFIG_THREE,
  BCM_ALGO_COEFF_CONFIG_FOUR,
  BCM_ALGO_COEFF_CONFIG_FIVE,
  BCM_ALGO_COEFF_CONFIG_SIX,
  BCM_ALGO_COEFF_CONFIG_SEVEN,
  BIA_LCFG_MAX,
} BIA_LCFG_t;

typedef enum {
  BIA_DATA_NO_READY = 0,
  BIA_DATA_READY = 1,
} BIA_DATA_READY_t;

typedef struct _gStateBia_t {
  uint16_t      biaSubscriberCount;
  uint8_t       biaNSamples;
  uint16_t      biaSequenceCount;
  uint8_t       biaStartCount;
} _gStateBia_t;

typedef struct _gBiaImpedence_t {
  float       real;
  float       img;
  uint32_t    timestamp;   
} _gBiaImpedence_t;

typedef struct _gBiaData_t {
  _gBiaImpedence_t biaImpedence;
  uint32_t      biaTs;
  uint32_t      biaPrevTs;
  uint8_t       biaCounter;
} _gBiaData_t;

#endif  // _APP_BCM_H
