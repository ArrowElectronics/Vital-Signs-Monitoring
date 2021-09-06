/**
****************************************************************************
* @file     display_app.h
* @author   ADI
* @version  V0.1
* @date     11-Mar-2021
* @brief    This is the header file used to do display task handler
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

#ifndef _DISPLAY_APP_H_
#define _DISPLAY_APP_H_

#include "stdint.h"
#include <post_office.h>

typedef struct{
    void (*display_func)(void);
    void (*key_handle)(uint8_t key_value);
    void (*signal_handle)(uint8_t signal_value);
    void (*m2m2_protocol_handle)(void * ptr);
    uint8_t page_type;
}PAGE_HANDLE;

typedef enum{
    DIS_STATIC_PAGE = 0,
    DIS_DYNAMIC_PAGE,
}DISPLAY_PAGE_TYPE;

typedef enum{
    DIS_KEY_SIGNAL = 0,
    DIS_PRIVATE_SIGNAL,
    DIS_GLOBLE_SIGNAL
}DISPLAY_SIGNAL_TYPE;

typedef enum{
    DIS_REFRESH_SIGNAL = 0,
}DIS_PRIVATE_SIGNAL_TYPE;

typedef enum{
    DIS_RESET_SIGNAL = 0,
#ifdef BLE_PEER_ENABLE
    DIS_BLE_PEER_REQUEST = 1,
#endif
#ifdef TEMP_PROTECT_FUNC_EN
    DIS_TMP_LOW_ALARM = 3,/* temp low alarm signal*/
    DIS_TMP_HIGH_ALARM = 4,/* temp high alarm signal*/
#endif
    DIS_VOLTAGE_LOW_ALARM = 5,/* voltage low alarm signal*/
    DIS_MAX_FILE_ALARM = 6,
    DIS_NO_BATTERY_ALARM = 7,
    DIS_LOW_TOUCH_ALARM = 8,
}DIS_GLOBLE_SIGNAL_TYPE;

typedef struct{
    uint8_t signal_type;
    uint8_t signal_value;
}display_signal_t;


/*all page handle */
extern const PAGE_HANDLE page_power_on;
extern const PAGE_HANDLE page_hr;
extern const PAGE_HANDLE page_setting;
extern const PAGE_HANDLE page_memory_usage;
extern const PAGE_HANDLE page_battery_level;
extern const PAGE_HANDLE page_blue_address;
extern const PAGE_HANDLE page_brightness_switch;
extern const PAGE_HANDLE page_power_off;
extern const PAGE_HANDLE page_power_off_confirm;
extern const PAGE_HANDLE page_shipment_mode;
extern const PAGE_HANDLE page_shipment_mode_confirm;
extern const PAGE_HANDLE page_ble_hr_service;

extern const PAGE_HANDLE page_waveform;
extern const PAGE_HANDLE page_ppg_title;
extern const PAGE_HANDLE page_ppg;
extern const PAGE_HANDLE page_ecg_title;
extern const PAGE_HANDLE page_ecg;
extern const PAGE_HANDLE page_ecg_measure_guide;
extern const PAGE_HANDLE page_skin_temp_title;
extern const PAGE_HANDLE page_skin_temp;

#ifdef CUST4_SM
extern const PAGE_HANDLE page_watch_id;
#endif
extern const PAGE_HANDLE page_menu;
extern const PAGE_HANDLE page_task_info;
extern const PAGE_HANDLE page_LT_mode2_log_enable;

extern const PAGE_HANDLE page_bpm;

extern const PAGE_HANDLE page_low_touch_config;
extern const PAGE_HANDLE page_logging_status;
extern const PAGE_HANDLE page_low_touch_enable;
extern const PAGE_HANDLE page_low_touch_read_cap;
extern const PAGE_HANDLE page_low_touch_logging;
extern const PAGE_HANDLE page_low_touch_lt_app_lcfg;
extern const PAGE_HANDLE page_low_touch_trigger_mode;
#ifdef BLE_PEER_ENABLE
extern const PAGE_HANDLE page_ble_peer_request;
extern const PAGE_HANDLE page_ble_peer_key;
#endif
#ifdef TEMP_PROTECT_FUNC_EN
extern const PAGE_HANDLE page_tmp_low_alarm;
extern const PAGE_HANDLE page_tmp_high_alarm;
#endif
extern const PAGE_HANDLE page_low_voltage_alarm;
extern const PAGE_HANDLE page_no_battery_error;
extern const PAGE_HANDLE page_low_touch_alarm;
extern const PAGE_HANDLE page_max_file_alarm;

extern const PAGE_HANDLE page_about;
extern const PAGE_HANDLE page_firmware_version;
extern const PAGE_HANDLE page_fcc_info;
extern const PAGE_HANDLE page_trade_mark;
extern const PAGE_HANDLE page_manufacture_date;
extern const PAGE_HANDLE page_made_country;
extern const PAGE_HANDLE page_device_name;

void send_message_display_task(m2m2_hdr_t *p_pkt);
void display_app_init(void);
void dis_page_jump(const PAGE_HANDLE *page);
void dis_page_back(void);
void dis_dynamic_refresh(uint16_t refresh_ms);
void dis_refresh_reset(uint16_t refresh_ms);
void send_global_type_value(uint8_t sub_type);
void send_private_type_value(uint8_t value);
//void fds_rtc_init(void);
#endif




