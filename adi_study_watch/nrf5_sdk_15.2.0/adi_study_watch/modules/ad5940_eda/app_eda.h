/**
******************************************************************************
* @file     app_eda.h
* @author   ADI
* @version  V1.0.0
* @date     30-August-2016
* @brief    Include file for the ECG application internal functions
******************************************************************************
* @attention
******************************************************************************
*/
/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2016 Analog Devices Inc.                                      *
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
* This software is intended for use with the ECG                              *
* only                                                                        *
*                                                                             *
******************************************************************************/
#ifndef _APP_EDA_H
#define _APP_EDA_H
#include <stdint.h>
#include <eda_application_interface.h>

#define EDA_ARRAY_SIZE 4
typedef enum {
  EDA_DATA_NO_READY = 0,
  EDA_DATA_READY = 1,
} EDA_DATA_READY_t;

typedef enum {
  EDA_LCFG_FS = 0,
  EDA_LCFG_ADC_PGA_GAIN,
  EDA_LCFG_DFT_NUM,
  EDA_LCFG_MAX,
} EDA_LCFG_t;

typedef struct _gEdaImpedence_t {
  float_t       real;
  float_t       img;
} _gEdaImpedence_t;

typedef struct _gEdaAd5940Data_t {
  _gEdaImpedence_t edaImpedence;
  uint32_t      edaTs;
  uint32_t      edaPrevTs;
  uint8_t       edaCounter;
} _gEdaAd5940Data_t;


typedef enum {
  EDA_ERROR = 0,
  EDA_SUCCESS = 1,
} EDA_ERROR_CODE_t;



EDA_ERROR_CODE_t EDAAppDeInit();
EDA_ERROR_CODE_t EDAAppInit();
EDA_ERROR_CODE_t EDAReadLCFG(uint8_t index, uint16_t *value);
EDA_ERROR_CODE_t EDAWriteLCFG(uint8_t field, uint16_t value);

#endif  // _APP_ECG_H
