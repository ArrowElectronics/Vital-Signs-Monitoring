/**
****************************************************************************
* @file     power_manager.h
* @author   ADI
* @version  V0.1
* @date     10-Mar-2020
* @brief    This header file used to implement power manager functions.
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
*   or more patent holders.  This license does not release you from the
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

#ifndef _POWER_MANAGER_H_
#define _POWER_MANAGER_H_
#include "stdbool.h"
#include "adi_types.h"

#define FS_LDO 1
#define OPT_DEV_LDO 2
#define ECG_LDO 3

#ifdef HIBERNATE_MD_EN
/* Hibernate Mode Entry-Exit event conditions */
/* 5 EVTs can result in Hib. Mode entry */
typedef enum {
  HIB_MD_USB_DISCONNECT_EVT     = (1<<0),
  HIB_MD_BLE_DISCONNECT_EVT     = (1<<1),
  HIB_MD_FS_LOG_STOP_EVT        = (1<<2),
  HIB_MD_NO_KEY_PRESS_EVT       = (1<<3),
  HIB_MD_DISP_INACTIVE_EVT      = (1<<4),
  HIB_MD_INVALID_EVT            = (0),
}HIB_MD_EVT_T;

void hibernate_md_set(HIB_MD_EVT_T evt);
void hibernate_md_clear(HIB_MD_EVT_T evt);
void hibernate_md_timers_init(void);
void hibernate_mode_entry();
#endif

void pmu_port_init(void);
_Bool adp5360_is_ldo_enable(int PIN);
void adp5360_enable_ldo(int PIN,bool en);
/*****************************************************************************
 * Function      : enter_poweroff_mode
 * Description   : Enter power off mode
 * Input         : void
 * Output        : None
 * Return        :
 * Others        :
 * Record
 * 1.Date        : 20200624
 *   Modification: Created function

*****************************************************************************/
void enter_poweroff_mode(void);

uint8_t set_hib_mode_control(uint8_t hib_mode_cntrl);
uint8_t get_hib_mode_control();
#endif //_POWER_MANAGER_H_





