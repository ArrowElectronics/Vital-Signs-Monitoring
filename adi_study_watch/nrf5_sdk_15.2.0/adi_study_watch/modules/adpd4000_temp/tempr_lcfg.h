/**
****************************************************************************
* @file     tempr_lcfg.h
* @author   ADI
* @version  V0.1
* @date     08-Sept-2021
* @brief    Temperature application LCFG settings.
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
#ifndef _TEMPERATURE_LCFG_H_
#define _TEMPERATURE_LCFG_H_
#include <stdio.h>
#include <stdint.h>
#include <temperature_task.h>
#include <adi_dcb_config.h>
#include <dcb_interface.h>
#include "nrf_log.h"

#define MAX_NUM_THERMISTORS           5u      /*! Maximum number of thermistors supported */
#define NUM_ENTRIES_T_I_LUT           21u     /*! Number of entries in LUT */
#define T_I_LUT_STEP_SIZE             5u      /*! LUT step size */
#define TEMPERATURE_SCALING_FACTOR    1000u   /*! Scaling factor for temperature value */
#define TEMPERATURE_MAX_VALUE         65535u  /*! Maximum temperature value supported */

/* Enums to indicate the presence of thermistor connected to different slots of ADPD4K*/
typedef enum
{
  E_THERMISTOR_SLOT_A = 0x00,
  E_THERMISTOR_SLOT_B = 0x01,
  E_THERMISTOR_SLOT_C = 0x02,
  E_THERMISTOR_SLOT_D = 0x03,
  E_CAL_RES_SLOT_E = 0x04,
  E_THERMISTOR_SLOT_F = 0x05,
  E_THERMISTOR_SLOT_G = 0x06,
  E_THERMISTOR_SLOT_H = 0x07,
  E_THERMISTOR_SLOT_I = 0x08,
  E_THERMISTOR_SLOT_J = 0x09,
  E_THERMISTOR_SLOT_K = 0x0A,
  E_THERMISTOR_SLOT_L = 0x0B,
  E_MAX_THERMISTOR_SLOT,
}e_thermistor_slots_t;

/* Library Configuration structure for Temperature Application */
typedef struct
{
  uint32_t sample_period;        /*! sampling period of temperature stream in seconds */
  uint32_t slots_selected;       /*! bit mask for (DCBA) slots selected */

/*! An array of Temperature - Impedance Characteristic Curve Look Up Table 
    which captures Thermistor Impedance for temperature ranging from 0 to 100 
    degree celcius in steps of 5 degree celcius  */  
  
  uint32_t T_I_curve_LUT[MAX_NUM_THERMISTORS][NUM_ENTRIES_T_I_LUT];  
}temperature_lcfg_t;

typedef enum
{
  TEMPERATURE_SUCCESS = 0x00,
  TEMPERATURE_ERROR = 0x01,
}TEMPERATURE_ERROR_CODE_t;

typedef enum {
  TEMPERATUER_LCFG_SAMPLE_PERIOD = 0,
  TEMPERATUER_LCFG_SLOTS_SELECTED,
  TEMPERATUER_LCFG_LUT_0,
  TEMPERATUER_LCFG_LUT_1,
  TEMPERATUER_LCFG_LUT_2,
  TEMPERATUER_LCFG_LUT_3,
  TEMPERATUER_LCFG_LUT_4,
  TEMPERATUER_LCFG_MAX,
} TEMPERATUER_LCFG_t;

m2m2_hdr_t *tempr_dcb_command_write_config(m2m2_hdr_t *p_pkt);
m2m2_hdr_t *tempr_dcb_command_read_config(m2m2_hdr_t *p_pkt);
m2m2_hdr_t *tempr_dcb_command_delete_config(m2m2_hdr_t *p_pkt);
m2m2_hdr_t *tempr_app_set_dcb_lcfg(m2m2_hdr_t *p_pkt);
m2m2_hdr_t *temperature_app_lcfg_access(m2m2_hdr_t *p_pkt);
TEMPERATURE_ERROR_CODE_t read_tempr_dcb(uint32_t *tempr_dcb_data, uint16_t *read_size);
TEMPERATURE_ERROR_CODE_t write_tempr_dcb(uint32_t *tempr_dcb_data, uint16_t write_Size);
TEMPERATURE_ERROR_CODE_t delete_tempr_dcb(void);
void tempr_update_dcb_present_flag(void);
bool tempr_get_dcb_present_flag(void);
void load_tempr_lcfg_from_dcb(void);
#endif
