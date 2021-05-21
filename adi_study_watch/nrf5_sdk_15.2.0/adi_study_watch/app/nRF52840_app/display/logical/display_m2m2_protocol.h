/**
****************************************************************************
* @file     display_m2m2_protocol.h
* @author   ADI
* @version  V0.1
* @date     11-Mar-2021
* @brief    This is the header file used by display task to issue m2m2 protocol.
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

#ifndef  _DISPLAY_M2M2_PROTOCOL_H_
#define  _DISPLAY_M2M2_PROTOCOL_H_

#include "stdint.h"
#include "m2m2_core.h"

#define M2M2_TIMEOUT_SHORT_WAIT (100)//ms
#define M2M2_TIMEOUT_PERIOD (5000)//ms
#define M2M2_TIMEOUT_LONG_WAIT (5000)//ms
#define M2M2_FS_TIMEOUT_PERIOD (10000u)

typedef union{
	uint8_t logging_all;
	struct{
		uint8_t ppg:1;
		uint8_t ecg:1;
		uint8_t reserved:6;
	}logging_t;

}logging_option_u;

enum{
	SENSOR_PPG = 0,
	SENSOR_ECG,
	SENSOR_ALL,
};

#define LOGGING_ALL_ENABLE ((0x01 << SENSOR_PPG)|(0x01 << SENSOR_ECG))
#define LOGGING_ALL_DISABLE (0x00)


extern logging_option_u logging_status_value;
extern logging_option_u logging_set_value;
extern logging_option_u logging_check_flag;

enum{
    M2M2_SUCCESS = 0,
    M2M2_WAIT_TIMEOUT,
    M2M2_STATUS_ERROR,
    M2M2_STATUS_DISABLE,
    M2M2_NO_MEMORY,
    M2M2_SEND_ERROR,
    M2M2_VALUE_OVERFLOW,
};

uint32_t m2m2_ecg_write_lcfg(uint8_t *status);
uint32_t m2m2_ppg_load_adpd_cfg(uint8_t *status);
uint32_t m2m2_ppg_clock_calibration(uint8_t *status);
uint32_t m2m2_set_ppg_configuration(uint8_t *status);
uint32_t m2m2_start_fs_log(uint8_t *status);
uint32_t m2m2_add_fs_subscribe(M2M2_ADDR_ENUM_t sub_addr,uint8_t *status);
uint32_t m2m2_subscribe_stream(M2M2_ADDR_ENUM_t dest_addr,M2M2_ADDR_ENUM_t sub_addr,uint8_t *status);
uint32_t m2m2_start_stream(M2M2_ADDR_ENUM_t dest_addr,uint8_t *status);
uint32_t m2m2_stop_fs_log(uint8_t *status);
uint32_t m2m2_remove_fs_subscribe(M2M2_ADDR_ENUM_t sub_addr,uint8_t *status);
uint32_t m2m2_stop_stream(M2M2_ADDR_ENUM_t dest_addr,uint8_t *status);
uint32_t m2m2_unsubscribe_stream(M2M2_ADDR_ENUM_t dest_addr,M2M2_ADDR_ENUM_t sub_addr,uint8_t *status);

uint32_t m2m2_check_sensor_status(M2M2_ADDR_ENUM_t dest_addr,uint8_t *status,uint8_t *subs_num,uint8_t *starts_num);
uint32_t m2m2_check_fs_status(uint8_t *status);
uint32_t m2m2_check_fs_sub_status(M2M2_ADDR_ENUM_t sub_addr,uint8_t *status);


uint32_t m2m2_set_fs_status(uint8_t en);
uint32_t m2m2_set_fs_subscribe(uint8_t sensor,uint8_t en);

uint32_t m2m2_set_stream_status(uint8_t sensor,uint8_t en);
uint32_t m2m2_memory_usage_get(uint8_t *percent);
uint32_t m2m2_check_logging_status(uint8_t *status);

#endif
