/**
****************************************************************************
* @file     ble_cus1.h
* @author   ADI
* @version  V0.1
* @date     10-May-2021
* @brief    This is the header file for ECG Raw Data Sample Service module.
*
* @details This module implements the ECG Raw Data Service with Hw Configuration
*          and ECG Raw Sample Service characteristics.
*
*          If enabled, notification of the ECG Raw Sample characteristic is performed
*          when the application calls ble_raw_ecg_sample_send().
*
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
#ifndef BLE_CUS1_H__
#define BLE_CUS1_H__

/* This code belongs in ble_cus1.h*/
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
 * ECG Raw Sample Service
 * UUID: 3fb80100-92e7-4865-9a00-14f06845bb77
 *
 * Characteristic                    UUID                           Properties
 * ---------------------------------------------------------------------------
 * ECG Raw Samples        | 3fb80103-92e7-4865-9a00-14f06845bb77  |  Notify
 * Hardware Configuration | 3fb80104-92e7-4865-9a00-14f06845bb77  |  Read
 */

/**
 * ECG Raw Sample Service UUID - 3fb80100-92e7-4865-9a00-14f06845bb77
 */
#define ECG_RAW_SAMPLE_SERVICE_UUID_BASE         {0x77, 0xBB, 0x45, 0x68, 0xF0, 0x14, 0x00, 0x9A, \
                                          0x65, 0x48, 0xE7, 0x92, 0x00, 0x00, 0xB8, 0x3F}

#define BLE_UUID_ECG_RAW_SAMPLE_SERVICE                0x0100  //!< The UUID of the ECG RAW Sample Service.
#define BLE_UUID_ECG_RAW_SAMPLES_CHAR_UUID             0x0103  //!< The UUID of ecg raw samples char
#define BLE_UUID_HW_CONFIGURATION_CHAR_UUID            0x0104  //!< The UUID of hardware configuration char

/**
 * @brief Custome Service1 event type.
 */
typedef enum
{
    BLE_CUS1_EVT_NOTIFICATION_ENABLED,   //!< The UUID of ECG Raw Sample value notification enabled event.
    BLE_CUS1_EVT_NOTIFICATION_DISABLED   //!< The UUID of ECG Raw Sample value notification disabled event.
} ble_cus1_evt_type_t;

/**
 * @brief Custom Service1 event.
 */
typedef struct
{
    ble_cus1_evt_type_t evt_type;    //!<  Type of event.
} ble_cus1_evt_t;

/**
 * @brief   Macro for defining a ble_cus1 instance.
 *
 * @param   _name   Name of the instance.
 * @hideinitializer
 */
#define BLE_CUS1_DEF(_name)                                                                         \
static ble_cus1_t _name;                                                                            \
NRF_SDH_BLE_OBSERVER(_name ## _obs,                                                                 \
                     BLE_NUS_BLE_OBSERVER_PRIO,                                                     \
                     ble_cus1_on_ble_evt, &_name)

//!< Forward declaration of the ble_cus1_t type.
typedef struct ble_cus1_s ble_cus1_t;

/**
 * @brief Custom Service1 event handler type.
 */
typedef void (* ble_cus1_evt_handler_t) (ble_cus1_evt_t * p_evt);

#define ECG_APP_FREQUENCY    (500) //!< ECG freq to be used
#define ECG_SAMPLE_COUNT     (25)  //!< No: of ECG samples to be part of 1 ECG notification pkt

/**
 * Values for Hardware characteristics
 * Values below are as required by the custom profile
 */
#define ECG_RANGE_MAX  (12230) //!< Max ECG value in uV
#define ECG_RANGE_MIN  (2630)  //!< Min ECG value in uV
#define ADC_RESOLUTION (15)    //!< Equivalent ADC resolution

/**
 * Explicitly enforce struct packing so that the nested structs and unions are laid out
 *  as expected.
 */
#if defined __CC_ARM || defined __IAR_SYSTEMS_ICC__ || __clang__ || defined _MSC_VER || defined __GNUC__
// DELIBERATELY BLANK
#else
#error "WARNING! Your compiler might not support '#pragma pack(1)'! \
  You must add an equivalent compiler directive to the file generator!"
#endif  // defined __CC_ARM || defined __IAR_SYSTEMS_ICC__ || __clang__ || defined _MSC_VER || defined __GNUC__
#pragma pack(1)

/**
 * @brief Data structure holding ECG Raw sample characteristics
 */
typedef struct ecg_raw_sample_char_data {
  uint8_t   version;                        //!< version as required in the characteristics
  uint32_t  rel_timestamp;                  //!< relative timestamp
  uint8_t   seq_number;                     //!< sequence number
  uint16_t  ecg_samp_data[ECG_SAMPLE_COUNT];//!< Array of len = ECG_SAMPLE_COUNT storing ECG sample data
}ecg_raw_sample_char_data_t;

/**
 * @brief Data structure holding Hardware configuration characteristics
 */
typedef struct hw_configuration_char_data {
  uint8_t  version;              //!< version as required in the characteristics
  int16_t  ecg_range_max;        //!< Max ECG Value
  int16_t  ecg_range_min;        //!< Min ECG Value
  uint8_t  adc_resolution;       //!< ADC Resolutions
}hw_configuration_char_data_t;

//!<  Reset struct packing outside
#pragma pack()

#define BLE_ECG_RAW_SAMPLES_CHAR_LEN         sizeof(ecg_raw_sample_char_data_t)     //!<  Maximum length of the ECG_RAW_SAMPLES Characteristic (in bytes).
#define BLE_HW_CONFIGURATION_CHAR_LEN        sizeof(hw_configuration_char_data_t)   //!<  Maximum length of the HW_CONFIGURATION Characteristic (in bytes).

/**
 * @brief Custom Service1 init structure. This contains all options and data needed for
 *        initialization of the service.
 */
typedef struct
{
    ble_cus1_evt_handler_t        evt_handler;                     //!< Event handler to be called for handling events in the Custom Service1.
    hw_configuration_char_data_t * p_hw_config_data;               //!< Initial custom value
    security_req_t               ecg_raw_samp_cccd_wr_sec;         //!< Security requirement for writing the ECG Raw Samples characteristic CCCD.
    security_req_t               hw_config_rd_sec;                 //!< Security requirement for reading the HW Configuration characteristic value.
} ble_cus1_init_t;

/**
 * @brief Custom Service1 structure. This contains various status information for the service.
 */
struct ble_cus1_s
{
    ble_cus1_evt_handler_t         evt_handler;                    //!<  Event handler to be called for handling events in the Custom Service1.
    uint16_t                       service_handle;                 //!<  Handle of Custom Service (as provided by the BLE stack).
    ble_gatts_char_handles_t       ecg_raw_sample_char_handles;    //!<  Handles related to the ECG_RAW_SAMPLES characteristic.
    ble_gatts_char_handles_t       hw_configuration_char_handles;  //!<  Handles related to the HW_CONFIGURATION characteristic.
    uint16_t                       conn_handle;                    //!<  Handle of the current connection (as provided by the BLE stack, is BLE_CONN_HANDLE_INVALID if not in a connection).
    uint8_t                        uuid_type;                      //!< UUID type
};

/**
 * @brief Function for initializing the Custom Service1.
 *
 * @param   p_cus       Custom Service1 structure. This structure will have to be supplied by
 *                      the application. It will be initialized by this function, and will later
 *                      be used to identify this particular service instance.
 * @param   p_cus_init  Information needed to initialize the service.
 *
 * @return  NRF_SUCCESS on successful initialization of service,
 *          otherwise an error code from nrf_error.h.
 */
uint32_t ble_cus1_init(ble_cus1_t * p_cus, ble_cus1_init_t const * p_cus_init);

/**
 * @brief Function for handling the GATT module's events.
 *
 * @details Handles all events from the GATT module of interest to the Custom Service1.
 *
 * @param   p_cus       Custom Service1 structure.
 * @param   p_gatt_evt  Event received from the GATT module.
 * @retval None
 */
void ble_cus1_on_gatt_evt(ble_cus1_t * p_cus, nrf_ble_gatt_evt_t const * p_gatt_evt);


/**
 * @brief Function for handling the Application's BLE Stack events.
 *
 * @details Handles all events from the BLE stack of interest to the Heart Rate Service.
 *
 * @param   p_ble_evt   Event received from the BLE stack.
 * @param   p_context   ECG raw sample Service structure.
 * @retval None
 */
void ble_cus1_on_ble_evt( ble_evt_t const * p_ble_evt, void * p_context);


/**
 * @brief Function for sending ecg raw samples measurement if notification has been enabled.
 *
 * @details The application calls this function after having performed a ecg measurement.
 *          If notification has been enabled, the ecg raw samples data is sent to
 *          the client.
 *
 * @param   p_cus                    Custom Service1 structure.
 * @param   ecg_raw_samp_notf_pkt    ECG Raw Sample Notification Pkt
 *
 * @return  NRF_SUCCESS on successful initialization of service,
 *          otherwise an error code from nrf_error.h.
 */
uint32_t ble_raw_ecg_sample_send(ble_cus1_t * p_cus, ecg_raw_sample_char_data_t ecg_raw_samp_notf_pkt);

#ifdef __cplusplus
}
#endif

#endif // BLE_CUS1_H__

/** @} */
