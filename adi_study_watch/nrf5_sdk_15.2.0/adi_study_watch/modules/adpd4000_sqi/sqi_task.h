/**
******************************************************************************
* @file     sqi_task.h
* @author   ADI
* @version  V1.0.0
* @date     17-January-2017
* @brief    Include file for the sqi application internal functions
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
* This software is intended for use with the sqi application only       *
*                                                                             *
*                                                                             *
******************************************************************************/
#ifndef _APP_SQI_H
#define _APP_SQI_H

#include <sqi_application_interface.h>
#include <common_sensor_interface.h>
#include <sensor_internal.h>
#include <app_cfg.h>

typedef enum {
  SQI_SUCCESS = 0,
  SQI_BUFFERING,
  SQI_NULL_PTR_ERROR,
  SQI_DIV_BY_ZERO_ERROR,
  SQI_ERROR
} SQI_ERROR_CODE_t;

typedef enum {
  SQI_LCFG_SQIALGOTYPE = 0,
  SQI_LCFG_MAX,
} SQI_LCFG_t;

typedef enum {
  SQI_BUFF_SUCCESS = 0,
  SQI_BUFF_IN_PROGRESS,
  SQI_BUFF_ERROR
} SQI_BUFF_RET_t;

void sqi_app_task_init(void);
void send_message_sqi_app_task(m2m2_hdr_t *p_pkt);
void send_sqi_app_data(uint32_t *padpdData, uint32_t ts);
#endif  
