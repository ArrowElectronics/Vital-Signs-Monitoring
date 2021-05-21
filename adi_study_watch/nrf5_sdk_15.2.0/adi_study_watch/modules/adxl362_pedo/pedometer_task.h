/**
******************************************************************************
* @file     app_pedometer.h
* @author   ADI
* @version  V1.0.0
* @date     17-January-2017
* @brief    Include file for the pedometer application internal functions
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
#ifndef _APP_PEDOMETER_H
#define _APP_PEDOMETER_H

#include <pedometer_application_interface.h>
#include <common_sensor_interface.h>
#include <sensor_internal.h>
#include <app_cfg.h>

typedef enum {
  PED_ERROR = 0,
  PED_SUCCESS = 1,
} PED_ERROR_CODE_t;

typedef enum {
  PED_LCFG_PEDALGOTYPE = 0,
  PED_LCFG_MAX,
} PED_LCFG_t;

typedef enum {
  PED_NO_ALGO = 0,
  PED_ADI_ALGO = 1,
} PED_ALGO_TYPE_t;

typedef struct ped_cfg_t {
  uint8_t pedalgotype;
} ped_cfg_t;

void pedometer_app_task_init(void);
void send_message_pedometer_app_task(m2m2_hdr_t *p_pkt);
void send_pedometer_app_data(int16_t *adxlData, uint32_t ts);
#endif  // _APP_PEDOMETER_H
