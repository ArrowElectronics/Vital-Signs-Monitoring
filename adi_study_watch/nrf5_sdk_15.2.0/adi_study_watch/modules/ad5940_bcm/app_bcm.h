/**
******************************************************************************
* @file     app_bcm.h
* @author   ADI
* @version  V1.0.0
* @date     10-October-2017
* @brief    Include file for the BCM application internal functions
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
* This software is intended for use with the BCM                              *
* only                                                                        *
*                                                                             *
******************************************************************************/
#ifndef _APP_BCM_H
#define _APP_BCM_H

#include <bcm_application_interface.h>
#include <sensor_internal.h>

#define BCM_BUFFER_SIZE               4

typedef enum {
  BCM_ERROR = 0,
  BCM_SUCCESS = 1,
} BCM_ERROR_CODE_t;

typedef enum {
  BCM_LCFG_FS = 0,
  BCM_LCFG_ADC_PGA_GAIN,
  BCM_LCFG_PWR_MOD,
  BCM_LCFG_SIN_FREQ,
  BCM_DFT_NUM,
  BCM_LCFG_MAX,
} BCM_LCFG_t;

typedef enum {
  BCM_DATA_NO_READY = 0,
  BCM_DATA_READY = 1,
} BCM_DATA_READY_t;

typedef struct _gStateBcm_t {
  uint16_t      bcmSubscriberCount;
  uint8_t       bcmNSamples;
  uint16_t      bcmSequenceCount;
  uint8_t       bcmStartCount;
} _gStateBcm_t;

typedef struct _gBcmImpedence_t {
  int32_t       real;
  int32_t       img;
} _gBcmImpedence_t;

typedef struct _gBcmData_t {
  _gBcmImpedence_t bcmImpedence;
  uint32_t      bcmTs;
  uint32_t      bcmPrevTs;
  uint8_t       bcmCounter;
} _gBcmData_t;


void bcm_application(m2m2_hdr_t *p_pkt);
void BcmGetEvent();


BCM_ERROR_CODE_t BCMapp_Init(void);
uint8_t BCMapp_DeInit(void);
uint8_t BCMapp_SetDftNum(uint32_t nDftNum);
uint8_t BCMapp_getNumberOfData();
uint8_t BCMapp_CheckSignal_Range(int32_t real,int32_t img);
uint8_t BCMapp_CheckSignal_Stability(int32_t real,int32_t img);
void SweepGetNext(void);
#endif  // _APP_BCM_H
