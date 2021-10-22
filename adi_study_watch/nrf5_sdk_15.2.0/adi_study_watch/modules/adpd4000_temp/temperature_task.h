/**
****************************************************************************
* @file     temperature_task.h
* @author   ADI
* @version  V0.1
* @date     04-Dec-2019
* @brief    This is the header file used to measure temperature from adpd4100
****************************************************************************
* @attention
******************************************************************************
* Copyright (c) 2021 Analog Devices, Inc.  All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
* - Redistributions of source code must retain the above copyright notice, this
*   list of conditions and the following disclaimer.
* - Redistributions in binary form must reproduce the above copyright notice,
*   this list of conditions and the following disclaimer in the documentation
*   and/or other materials provided with the distribution.
* - Modified versions of the software must be conspicuously marked as such.
* - This software is licensed solely and exclusively for use with
*   processors/products manufactured by or for Analog Devices, Inc.
* - This software may not be combined or merged with other code in any manner
*   that would cause the software to become subject to terms and conditions
*   which differ from those listed here.
* - Neither the name of Analog Devices, Inc. nor the names of its contributors
*   may be used to endorse or promote products derived from this software
*   without specific prior written permission.
* - The use of this software may or may not infringe the patent rights of one
**   or more patent holders.  This license does not release you from the
*   requirement that you obtain separate licenses from these patent holders to
*   use this software.
*
* THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES, INC. AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
* NONINFRINGEMENT, TITLE, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL ANALOG DEVICES, INC. OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, PUNITIVE OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, DAMAGES ARISING OUT OF
* CLAIMS OF INTELLECTUAL PROPERTY RIGHTS INFRINGEMENT; PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
******************************************************************************/
#ifndef __TEMPERATURE_APP_TASK__H
#define __TEMPERATURE_APP_TASK__H
#include <temperature_application_interface.h>
#include <common_sensor_interface.h>
#include <task_includes.h>
#include <post_office.h>
#include <adpd400x_drv.h>
#include <adpd4000_dcfg.h>
#include <app_cfg.h>

/*! structure to hold slot information for a temperature stream*/
typedef struct
{
  uint16_t num_subs;  /*! number of subscriber for a slot */
  uint16_t seq_num;   /*! sequence number of a packet */
  uint16_t tempr;     /*! temperature measured in a slot */
  uint32_t res;       /*! resistance measured in a slot */
}tempr_slot_info_t;

void temperature_app_task_init(void);
void send_message_temperature_app_task(m2m2_hdr_t *p_pkt);
#endif // __TEMPERATURE_APP_TASK__H
