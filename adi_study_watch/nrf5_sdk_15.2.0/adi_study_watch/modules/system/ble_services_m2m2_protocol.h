/**
***************************************************************************
* @file         ble_services_m2m2_protocol.h
* @author       ADI
* @version      V1.0.0
* @date         15-Feb-2021
***************************************************************************
* @attention
***************************************************************************
*/
/*!
*  \copyright Analog Devices
* ****************************************************************************
*
* License Agreement
*
* Copyright (c) 2021 Analog Devices Inc.
* All rights reserved.
*
* This source code is intended for the recipient only under the guidelines of
* the non-disclosure agreement with Analog Devices Inc.
*

* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER  LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING  FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER  DEALINGS IN THE SOFTWARE.
* ****************************************************************************
*/
#ifndef  _BLE_SERVICES_M2M2_PROTOCOL_H_
#define  _BLE_SERVICES_M2M2_PROTOCOL_H_

#include "stdint.h"
#include "m2m2_core.h"

#define M2M2_TIMEOUT_SHORT_WAIT (100)//ms
#define M2M2_TIMEOUT_PERIOD (5000)//ms
#define M2M2_TIMEOUT_LONG_WAIT (5000)//ms
#define M2M2_FS_TIMEOUT_PERIOD (10000u)

enum{
	SENSOR_PPG = 0,
	SENSOR_ECG,
	SENSOR_ALL,
};

enum{
    M2M2_SUCCESS = 0,
    M2M2_WAIT_TIMEOUT,
    M2M2_STATUS_ERROR,
    M2M2_STATUS_DISABLE,
    M2M2_NO_MEMORY,
    M2M2_SEND_ERROR,
    M2M2_VALUE_OVERFLOW,
};

//uint32_t ble_services_sensor_m2m2_ecg_write_lcfg(uint8_t *status);
uint32_t ble_services_sensor_m2m2_ppg_load_adpd_cfg(uint8_t *status);
uint32_t ble_services_sensor_m2m2_ppg_clock_calibration(uint8_t *status);
uint32_t ble_services_sensor_m2m2_set_ppg_configuration(uint8_t *status);
uint32_t ble_services_sensor_m2m2_subscribe_stream(M2M2_ADDR_ENUM_t dest_addr,M2M2_ADDR_ENUM_t sub_addr,uint8_t *status);
uint32_t ble_services_sensor_m2m2_start_stream(M2M2_ADDR_ENUM_t dest_addr,uint8_t *status);
uint32_t ble_services_sensor_m2m2_stop_stream(M2M2_ADDR_ENUM_t dest_addr,uint8_t *status);
uint32_t ble_services_sensor_m2m2_unsubscribe_stream(M2M2_ADDR_ENUM_t dest_addr,M2M2_ADDR_ENUM_t sub_addr,uint8_t *status);

uint32_t ble_services_sensor_m2m2_check_sensor_status(M2M2_ADDR_ENUM_t dest_addr,uint8_t *status,uint8_t *subs_num,uint8_t *starts_num);
uint32_t ble_services_sensor_m2m2_set_stream_status(uint8_t sensor,uint8_t en);
#endif
