/**
******************************************************************************
* @file     app_ecg.h
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
#ifndef _APP_ECG_H
#define _APP_ECG_H

#include <stdint.h>
typedef enum {
  ECG_DATA_NO_READY = 0,
  ECG_DATA_READY = 1,
} ECG_DATA_READY_t;

typedef enum {
  ECG_LCFG_FS = 0,
  ECG_LCFG_ADC_PGA_GAIN,
  ECG_LCFG_AFE_PWR_MOD,
  ECG_LCFG_MAX,
} ECG_LCFG_t;

typedef struct EcgData_t {
  uint16_t EcgData;
  uint32_t EcgTS;
} EcgData_t;

typedef enum {
  ECG_ERROR = 0,
  ECG_SUCCESS = 1,
} ECG_ERROR_CODE_t;


typedef enum {
  ECG_TYPE_SPORT = 0x01,
} ECG_TYPE_t;

void EcgGetEvent();
ECG_DATA_READY_t EcgReadData(EcgData_t *nEcgData, uint8_t *nsamples);
ECG_ERROR_CODE_t EcgAppInit(void);
ECG_ERROR_CODE_t EcgAppDeInit(void);
int32_t EcgInit(void);
ECG_ERROR_CODE_t EcgReadLCFG(uint8_t index, uint16_t *value);
ECG_ERROR_CODE_t EcgWriteLCFG(uint8_t field, uint16_t value);
#endif  // _APP_ECG_H
