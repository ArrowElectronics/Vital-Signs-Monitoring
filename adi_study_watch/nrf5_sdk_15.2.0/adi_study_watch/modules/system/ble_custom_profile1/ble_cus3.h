/**
****************************************************************************
* @file     ble_cus3.h
* @author   ADI
* @version  V0.1
* @date     10-May-2021
* @brief    This is the header file for Algorithm Info Service module.
*
* @details  This module implements the Algorithm Info Service with Heart Rate Algorithm
*           Information characteristic.
*
*          If enabled, notification of the Heart Rate Algorithm Information
*          characteristic is performed
*          when the application calls ble_hr_algo_info_send().
*
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
/**
 * Copyright (c) 2012 - 2018, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#ifndef BLE_CUS3_H__
#define BLE_CUS3_H__

/* This code belongs in ble_cus3.h*/
#include <stdint.h>
#include <stdbool.h>
#include "sdk_config.h"
#include "ble.h"
#include "ble_srv_common.h"
#include "nrf_sdh_ble.h"
#include "nrf_ble_gatt.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The UUID is given as the sixteen octets of a UUID are represented as 32 hexadecimal (base 16) digits,
 * displayed in five groups separated by hyphens, in the form 8-4-4-4-12.
 * The 16 octets are given in big-endian, while we use the little-endian representation in our SDK.
 * Thus, we must reverse the byte-ordering when we define our UUID base in the ble_cus.h, as shown below.
 */

/**
 * Algo Info Service
 * UUID: 3fb80C00-92e7-4865-9a00-14f06845bb77
 *
 * Characteristic                    UUID                           Properties
 * ---------------------------------------------------------------------------
 * HR Algo Info           | 3fb80C01-92e7-4865-9a00-14f06845bb77  |  Notify
 */

 //!< Algo Info Service UUID - 3fb80C00-92e7-4865-9a00-14f06845bb77
#define ALGO_INFO_SERVICE_UUID_BASE         {0x77, 0xBB, 0x45, 0x68, 0xF0, 0x14, 0x00, 0x9A, \
                                          0x65, 0x48, 0xE7, 0x92, 0x00, 0x00, 0xB8, 0x3F}

#define BLE_UUID_ALGO_INFO_SERVICE                0x0C00  //!< The UUID of the Algo Info Service.
#define BLE_UUID_HR_ALGO_INFO_CHAR_UUID           0x0C01  //!< The UUID of the HR Algo Info Service.

/**
 * @brief Custome Service3 event type.
 */
typedef enum
{
    BLE_CUS3_EVT_NOTIFICATION_ENABLED,    //!< HR Algo Info notification enabled event.
    BLE_CUS3_EVT_NOTIFICATION_DISABLED    //!< HR Algo Info notification disabled event.
} ble_cus3_evt_type_t;

/**
 * @brief Custom Service3 event.
 */
typedef struct
{
    ble_cus3_evt_type_t evt_type;     //!< Type of event.
} ble_cus3_evt_t;

/**
 * @brief   Macro for defining a ble_cus3 instance.
 *
 * @param   _name   Name of the instance.
 * @hideinitializer
 */
#define BLE_CUS3_DEF(_name)                                                                         \
static ble_cus3_t _name;                                                                            \
NRF_SDH_BLE_OBSERVER(_name ## _obs,                                                                 \
                     BLE_NUS_BLE_OBSERVER_PRIO,                                                     \
                     ble_cus3_on_ble_evt, &_name)

 //!< Forward declaration of the ble_cus3_t type.
typedef struct ble_cus3_s ble_cus3_t;

/**
 * @brief Custom Service3 event handler type.
 */
typedef void (* ble_cus3_evt_handler_t) (ble_cus3_evt_t * p_evt);

/**
 * Explicitly enforce struct packing so that the nested structs and unions are laid out
 * as expected.
 */
#if defined __CC_ARM || defined __IAR_SYSTEMS_ICC__ || __clang__ || defined _MSC_VER || defined __GNUC__
// DELIBERATELY BLANK
#else
#error "WARNING! Your compiler might not support '#pragma pack(1)'! \
  You must add an equivalent compiler directive to the file generator!"
#endif  // defined __CC_ARM || defined __IAR_SYSTEMS_ICC__ || __clang__ || defined _MSC_VER || defined __GNUC__
#pragma pack(1)

/**
 * @brief Data structure holding HR Algo Info characteristics
 */
typedef struct hr_algo_info_char_data {
  uint8_t   version;          //!< version as required in the characteristics
  uint8_t   hr_source;        //!< To show the Source of HR - ECG/PPG
  uint8_t   ecg_accurate_hr;  //!< To show if ECG HR is accurate/estimated
  uint8_t   ppg_hr_confidence;//!< Percentage based confidence number
}hr_algo_info_char_data_t;

#define ECG_HR_SOURCE       1 //!< Value to show HR source as ECG
#define ECG_ACCURATE_HR     1 //!< Value to show ECG HR is accurate
#define ECG_ESTIMATED_HR    0 //!< Value to show ECG HR is estimated

 //!< Reset struct packing outside
#pragma pack()

#define BLE_HR_ALGO_INFO_CHAR_LEN         sizeof(hr_algo_info_char_data_t)     //!<  Maximum length of the HR Algo Info Characteristic (in bytes).

/**
 * @brief Custom Service3 init structure. This contains all options and data needed for
 *        initialization of the service.
 */
typedef struct
{
    ble_cus3_evt_handler_t        evt_handler;                      //!< Event handler to be called for handling events in the Custom Service3.
    hr_algo_info_char_data_t *    p_hr_algo_info_data;              //!< Initial custom value
    security_req_t                hr_algo_info_cccd_wr_sec;         //!< Security requirement for writing the HR Algo Info characteristic CCCD.
} ble_cus3_init_t;

/**
 * @brief Custom Service3 structure. This contains various status information for the service.
 */
struct ble_cus3_s
{
    ble_cus3_evt_handler_t         evt_handler;                     //!< Event handler to be called for handling events in the Custom Service3.
    uint16_t                       service_handle;                  //!< Handle of Custom Service (as provided by the BLE stack).
    ble_gatts_char_handles_t       hr_algo_info_char_handles;       //!< Handles related to the hr_algo_info_char_data_t characteristic.
    uint16_t                       conn_handle;                     //!< Handle of the current connection (as provided by the BLE stack, is BLE_CONN_HANDLE_INVALID if not in a connection).
    uint8_t                        uuid_type;
};

/**
 * @brief Function for initializing the Custom Service3.
 *
 * @param  p_cus       Custom Service3 structure. This structure will have to be supplied by
 *                     the application. It will be initialized by this function, and will later
 *                     be used to identify this particular service instance.
 * @param  p_cus_init  Information needed to initialize the service.
 *
 * @return  NRF_SUCCESS on successful initialization of service,
 *          otherwise an error code from nrf_error.h.
 */
uint32_t ble_cus3_init(ble_cus3_t * p_cus, ble_cus3_init_t const * p_cus_init);

/**
 * @brief Function for handling the GATT module's events.
 *
 * @details Handles all events from the GATT module of interest to the Custom Service3.
 *
 * @param   p_cus       Custom Service3 structure.
 * @param   p_gatt_evt  Event received from the GATT module.
 * @retval None
 */
void ble_cus3_on_gatt_evt(ble_cus3_t * p_cus, nrf_ble_gatt_evt_t const * p_gatt_evt);


/**
 * @brief Function for handling the Application's BLE Stack events.
 *
 * @details Handles all events from the BLE stack of interest to the Heart Rate Service.
 *
 * @param   p_ble_evt   Event received from the BLE stack.
 * @param   p_context   Algo Info Service structure.
 * @retval None
 */
void ble_cus3_on_ble_evt( ble_evt_t const * p_ble_evt, void * p_context);


/**
 * @brief Function for sending HR algo Info measurement if notification has been enabled.
 *
 * @details The application calls this function after having performed a ecg measurement.
 *          If notification has been enabled, the ecg raw samples data is sent to
 *          the client.
 *
 * @param   p_cus                         Custom Service3 structure.
 * @param   hr_algo_info_notf_pkt         HR Algo Info Notification Pkt
 *
 * @return  NRF_SUCCESS on successful initialization of service,
 *          otherwise an error code from nrf_error.h.
 */
uint32_t ble_hr_algo_info_send(ble_cus3_t * p_cus, hr_algo_info_char_data_t hr_algo_info_notf_pkt);

#ifdef __cplusplus
}
#endif

#endif // BLE_CUS3_H__

/** @} */
