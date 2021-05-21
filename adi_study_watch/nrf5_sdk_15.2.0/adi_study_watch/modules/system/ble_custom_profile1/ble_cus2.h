/**
****************************************************************************
* @file     ble_cus2.h
* @author   ADI
* @version  V0.1
* @date     10-May-2021
* @brief    This is the header file for Misc Service module.
*
* @details This module implements the Misc Service with Set Reference Time,
*          Device Configuration, BLE Profile Version String and
*          ECG/PPG Mode characteristics.
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
#ifndef BLE_CUS2_H__
#define BLE_CUS2_H__

/* This code belongs in ble_cus2.h*/
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
 * Misc Service
 * UUID: 3fb80B00-92e7-4865-9a00-14f06845bb77
 *
 * Characteristic                    UUID                           Properties
 * -------------------------------------------------------------------------------------
 * Set Reference Time            | 3fb80B01-92e7-4865-9a00-14f06845bb77  |  Write
 * Device Configuration          | 3fb80B04-92e7-4865-9a00-14f06845bb77  |  Read
 * BLE Profile Version String    | 3fb80B05-92e7-4865-9a00-14f06845bb77  |  Read
 * ECG/PPG Mode                  | 3fb80B06-92e7-4865-9a00-14f06845bb77  |  Read, Notify
 */

/**
 * Misc Service UUID - 3fb80B00-92e7-4865-9a00-14f06845bb77
 */
#define MISC_SERVICE_UUID_BASE         {0x77, 0xBB, 0x45, 0x68, 0xF0, 0x14, 0x00, 0x9A, \
                                          0x65, 0x48, 0xE7, 0x92, 0x00, 0x00, 0xB8, 0x3F}

#define BLE_UUID_MISC_SERVICE                       0x0B00  //!< The UUID of the Misc Service.

#define BLE_UUID_SET_REF_TIME_CHAR_UUID             0x0B01  //!< The UUID of set referance time char
#define BLE_UUID_DEVICE_CONFIGURATION_CHAR_UUID     0x0B04  //!< The UUID of device configuration char
#define BLE_UUID_BLE_PROFILE_VER_STR_CHAR_UUID      0x0B05  //!< The UUID of ble profile version string char
#define BLE_UUID_ECG_PPG_MODE_CHAR_UUID             0x0B06  //!< The UUID of ecg/ppg mode char

/**
 * @brief Custome Service2 event type.
 */
typedef enum
{
    BLE_CUS2_EVT_NOTIFICATION_ENABLED,    //!< ECG/PPG Mode notification enabled event.
    BLE_CUS2_EVT_NOTIFICATION_DISABLED,   //!< ECG/PPG Mode notification disabled event.
    BLE_CUS2_EVT_SET_REF_TIME             //!< Set Referance Time event.
} ble_cus2_evt_type_t;

/**
 * @brief   Macro for defining a ble_cus2 instance.
 *
 * @param   _name   Name of the instance.
 * @hideinitializer
 */
#define BLE_CUS2_DEF(_name)                                                                         \
static ble_cus2_t _name;                                                                            \
NRF_SDH_BLE_OBSERVER(_name ## _obs,                                                                 \
                     BLE_NUS_BLE_OBSERVER_PRIO,                                                     \
                     ble_cus2_on_ble_evt, &_name)

//!< Forward declaration of the ble_cus2_t type.
typedef struct ble_cus2_s ble_cus2_t;

/*
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
 * @brief Data structure holding Set Referance Time characteristics
 */
typedef struct set_ref_time_char_data {
  uint8_t   version;    //!< version as required in the characteristics
  uint32_t  unix_time;  //!< Timestamp to be set the Watch time to
}set_ref_time_char_data_t;

/**
 * @brief Data structure holding Device configuration characteristics
 */
typedef struct dev_configuration_char_data {
  uint8_t   version;        //!< version as required in the characteristics
  uint16_t  ecg_samp_rate;  //!< ECG sampling rate
  uint16_t  ppg_samp_rate;  //!< PPG sampling rate
  uint16_t  acc_samp_rate;  //!< Accelerometer sampling rate
}dev_configuration_char_data_t;

#define BLE_PROFILE_VERSION_STR "1.0.0"

/**
 * @brief Data structure holding ECG/PPG mode characteristics
 */
typedef struct ecg_ppg_mode_char_data {
  uint8_t  version;       //!< version as required in the characteristics
  uint8_t  ecg_ppg_mode;  //!< ECG / PPG mode as in ECG_PPG_MODE_ENUM_t
}ecg_ppg_mode_char_data_t;

typedef enum ECG_PPG_MODE_ENUM_t{
  ECG_MODE = 0,      //!< 0 - ECG Mode
  PPG_MODE,          //!< 1 - PPG Mode
  MIXED_MODE,        //!< 2 - Mixed Mode (ECG and PPG)
  NOT_SAMPLING,      //!< 3 - Not Sampling
  CARRYING_HW_TEST,  //!< 4 - Carrying out a HW Test
}ECG_PPG_MODE_ENUM_t;

//!< Reset struct packing outside
#pragma pack()

#define BLE_SET_REF_TIME_CHAR_LEN          sizeof(set_ref_time_char_data_t)         //!< Maximum length of the Set Ref Time Characteristic (in bytes).
#define BLE_DEV_CONFIGURATION_CHAR_LEN     sizeof(dev_configuration_char_data_t)    //!< Maximum length of the Device CONFIGURATION Characteristic (in bytes).
#define BLE_ECG_PPG_MODE_CHAR_LEN          sizeof(ecg_ppg_mode_char_data_t)         //!< Maximum length of the ECG/PPG Mode Characteristic (in bytes).

/**
 * @brief Custom Service2 event.
 */
typedef struct
{
    ble_cus2_evt_type_t evt_type;     //!< Type of event.
    set_ref_time_char_data_t    * p_set_ref_data;
} ble_cus2_evt_t;

/**
 * @brief Custom Service2 event handler type.
 */
typedef void (* ble_cus2_evt_handler_t) (ble_cus2_evt_t * p_evt);

/**
 * @brief Custom Service2 init structure. This contains all options and data needed for
 *        initialization of the service.
 */
typedef struct
{
    ble_cus2_evt_handler_t        evt_handler;                      //!< Event handler to be called for handling events in the Custom Service2.
    dev_configuration_char_data_t * p_dev_config_data;              //!< Initial custom value */
    uint8_t *                    p_ble_prof_ver_str;                //!< Pointer to Init value of BLE profile version string to be used
    ecg_ppg_mode_char_data_t *   p_mode;                            //!< Init value for ECG/PPG mode
    security_req_t               set_ref_time_wr_sec;               //!< Security requirement for writing the Set Ref Time characteristic CCCD.
    security_req_t               set_ref_time_rd_sec;               //!< Security requirement for reading the Set Ref Time characteristic CCCD.
    security_req_t               dev_config_rd_sec;                 //!< Security requirement for reading the Device Configuration characteristic value.
    security_req_t               ble_prof_ver_str_rd_sec;           //!< Security requirement for reading the BLE PROFILE Version String characteristic value.
    security_req_t               ecg_ppg_mode_wr_sec;               //!< Security requirement for writing the ECG/PPG Mode characteristic value.
    security_req_t               ecg_ppg_mode_rd_sec;               //!< Security requirement for reading the ECG/PPG Mode characteristic value.
} ble_cus2_init_t;

/**
 * @brief Custom Service2 structure. This contains various status information for the service.
 */
struct ble_cus2_s
{
    ble_cus2_evt_handler_t         evt_handler;                     //!< Event handler to be called for handling events in the Custom Service2.
    uint16_t                       service_handle;                  //!< Handle of Custom Service (as provided by the BLE stack).
    ble_gatts_char_handles_t       set_ref_time_char_handles;       //!< Handles related to the Set Ref Time characteristic.
    ble_gatts_char_handles_t       dev_configuration_char_handles;  //!< Handles related to the Device CONFIGURATION characteristic.
    ble_gatts_char_handles_t       ble_prof_ver_str_char_handles;   //!< Handles related to the ble_prof_ver_str characteristic.
    ble_gatts_char_handles_t       ecg_ppg_mode_char_handles;       //!< Handles related to the ecg_ppg_mode characteristic.
    uint16_t                       conn_handle;                     //!< Handle of the current connection (as provided by the BLE stack, is BLE_CONN_HANDLE_INVALID if not in a connection).
    uint8_t                        uuid_type;                       //!< UUID type
};

/**
 * @brief Function for initializing the Custom Service2.
 *
 * @param  p_cus       Custom Service2 structure. This structure will have to be supplied by
 *                     the application. It will be initialized by this function, and will later
 *                     be used to identify this particular service instance.
 * @param  p_cus_init  Information needed to initialize the service.
 *
 * @return  NRF_SUCCESS on successful initialization of service,
 *          otherwise an error code from nrf_error.h.
 */
uint32_t ble_cus2_init(ble_cus2_t * p_cus, ble_cus2_init_t const * p_cus_init);

/**
 * @brief Function for handling the GATT module's events.
 *
 * @details Handles all events from the GATT module of interest to the Custom Service1.
 *
 * @param   p_cus      Custom Service2 structure.
 * @param   p_gatt_evt  Event received from the GATT module.
 * @retval None
 */
void ble_cus2_on_gatt_evt(ble_cus2_t * p_cus, nrf_ble_gatt_evt_t const * p_gatt_evt);


/**
 * @brief Function for handling the Application's BLE Stack events.
 *
 * @details Handles all events from the BLE stack of interest to the Misc Service.
 *
 * @param   p_ble_evt   Event received from the BLE stack.
 * @param   p_context   Misc Service structure.
 * @retval None
 */
void ble_cus2_on_ble_evt( ble_evt_t const * p_ble_evt, void * p_context);


/**@brief Function for sending ecg_ppg_mode if notification has been enabled.
 *
 * @details The application calls this function after changing ecg_ppg_mode
 *
 * @param   p_cus                     Custom Service2 structure.
 * @param   ecg_ppg_mode_notf_pkt     ECG ppg mode Notification Pkt
 *
 * @return  NRF_SUCCESS on successful initialization of service,
 *          otherwise an error code from nrf_error.h.
 */
uint32_t ble_ecg_ppg_mode_send(ble_cus2_t * p_cus, ecg_ppg_mode_char_data_t ecg_ppg_mode_notf_pkt);

#ifdef __cplusplus
}
#endif

#endif // BLE_CUS2_H__

/** @} */
