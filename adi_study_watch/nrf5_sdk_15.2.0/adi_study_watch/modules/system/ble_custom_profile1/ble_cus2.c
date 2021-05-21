/**
****************************************************************************
* @file     ble_cus2.c
* @author   ADI
* @version  V0.1
* @date     10-May-2021
* @brief    This is the source file for Misc Service module.
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
#ifdef BLE_CUSTOM_PROFILE1
#include "sdk_common.h"
#include "ble_cus2.h"
#include <string.h>
#include "ble_srv_common.h"
//#include "nrf_log.h"

#define EXPECTED_LEN_OF_RECEIVED_DATA (2)  //!< Expected length of received data, when the characteristics was written

/**@brief Function for handling the Connect event.
 *
 * @param  p_cus       Custom Service2 structure.
 * @param  p_ble_evt   Event received from the BLE stack.
 * @retval None
 */
static void on_connect(ble_cus2_t * p_cus, ble_evt_t const * p_ble_evt)
{
    p_cus->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
}

/**@brief Function for handling the Disconnect event.
 *
 * @param  p_cus       Custom Service2 structure.
 * @param  p_ble_evt   Event received from the BLE stack.
 * @retval None
 */
static void on_disconnect(ble_cus2_t * p_cus, ble_evt_t const * p_ble_evt)
{
    UNUSED_PARAMETER(p_ble_evt);
    p_cus->conn_handle = BLE_CONN_HANDLE_INVALID;
}

/**@brief Function for handling write events to the set_ref_time_char_handles of Misc Service
 *
 * @param  p_cus         Custom Service2 structure.
 * @param  p_evt_write   Write event received from the BLE stack.
 * @retval None
 */
static void on_set_ref_time_cccd_write(ble_cus2_t * p_cus, ble_gatts_evt_write_t const * p_evt_write)
{

    // CCCD written, update notification state
    if (p_cus->evt_handler != NULL)
    {
        ble_cus2_evt_t evt;
        evt.evt_type                  = BLE_CUS2_EVT_SET_REF_TIME;
        evt.p_set_ref_data = (set_ref_time_char_data_t *)p_evt_write->data;
        //NRF_LOG_INFO("Data;%x, Len:%d",p_evt_write->data, p_evt_write->len);

        p_cus->evt_handler(&evt);
    }
}

/**@brief Function for handling write events to the ecg_ppg_mode_char_handles of Misc Service
 *
 * @param  p_cus         Custom Service2 structure.
 * @param  p_evt_write   Write event received from the BLE stack.
 * @retval None
 */
static void on_ecg_ppg_mode_cccd_write(ble_cus2_t * p_cus, ble_gatts_evt_write_t const * p_evt_write)
{
    if (p_evt_write->len == EXPECTED_LEN_OF_RECEIVED_DATA)
    {
        // CCCD written, update notification state
        if (p_cus->evt_handler != NULL)
        {
            ble_cus2_evt_t evt;

            if (ble_srv_is_notification_enabled(p_evt_write->data))
            {
                evt.evt_type = BLE_CUS2_EVT_NOTIFICATION_ENABLED;
            }
            else
            {
                evt.evt_type = BLE_CUS2_EVT_NOTIFICATION_DISABLED;
            }

            p_cus->evt_handler(&evt);
        }
    }
}


/**@brief Function for handling the Write event from the SoftDevice.
 *
 * @param  p_cus       Custom Service2 structure.
 * @param  p_ble_evt   Event received from the BLE stack.
 * @retval None
 */
static void on_write(ble_cus2_t * p_cus, ble_evt_t const * p_ble_evt)
{
    ble_gatts_evt_write_t const * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

    if (p_evt_write->handle == p_cus->set_ref_time_char_handles.value_handle)
    {
        on_set_ref_time_cccd_write(p_cus, p_evt_write);
    }
    else if (p_evt_write->handle == p_cus->ecg_ppg_mode_char_handles.cccd_handle)
    {
        on_ecg_ppg_mode_cccd_write(p_cus, p_evt_write);
    }
}

/**@brief Function for handling BLE events the SoftDevice.
 *
 * @param  p_ble_evt     Event received from the BLE stack.
 * @param  p_context     Context.
 * @retval None
 */
void ble_cus2_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context)
{
    ble_cus2_t * p_cus = (ble_cus2_t *) p_context;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GATTS_EVT_WRITE:
            on_write(p_cus, p_ble_evt);
            break;

        case BLE_GAP_EVT_CONNECTED:
            on_connect(p_cus, p_ble_evt);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            on_disconnect(p_cus, p_ble_evt);
            break;

        default:
            // No implementation needed.
            break;
    }
}

/**@brief Function for initializing the Custom Service2.
 *
 * @param  p_cus       Custom Service1 structure. This structure will have to be supplied by
 *                     the application. It will be initialized by this function, and will later
 *                     be used to identify this particular service instance.
 * @param  p_cus_init  Information needed to initialize the service.
 *
 * @return NRF_SUCCESS on successful initialization of service,
 *         otherwise an error code from nrf_error.h.
 */
uint32_t ble_cus2_init(ble_cus2_t * p_cus, const ble_cus2_init_t * p_cus_init)
{
    if (p_cus == NULL || p_cus_init == NULL)
    {
        return NRF_ERROR_NULL;
    }

    uint32_t   err_code;
    ble_uuid_t ble_uuid;
    ble_add_char_params_t add_char_params;

    // Initialize service structure
    p_cus->conn_handle               = BLE_CONN_HANDLE_INVALID;
    p_cus->evt_handler               = p_cus_init->evt_handler;

    // Add Misc Service Base UUID
    ble_uuid128_t base_uuid = {MISC_SERVICE_UUID_BASE};
    err_code =  sd_ble_uuid_vs_add(&base_uuid, &p_cus->uuid_type);
    VERIFY_SUCCESS(err_code);

    ble_uuid.type = p_cus->uuid_type;
    ble_uuid.uuid = BLE_UUID_MISC_SERVICE;

    // Add the Misc Service
    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &p_cus->service_handle);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Add the Set Ref Time Characteristic.
    memset(&add_char_params, 0, sizeof(add_char_params));
    add_char_params.uuid                     = BLE_UUID_SET_REF_TIME_CHAR_UUID;
    add_char_params.uuid_type                = p_cus->uuid_type;
    add_char_params.max_len                  = BLE_SET_REF_TIME_CHAR_LEN;
    add_char_params.init_len                 = BLE_SET_REF_TIME_CHAR_LEN;
    add_char_params.is_var_len               = false;
    add_char_params.char_props.write         = 1;
    //add_char_params.char_props.write_wo_resp = 1;
    add_char_params.write_access             = p_cus_init->set_ref_time_wr_sec;
    add_char_params.cccd_write_access        = p_cus_init->set_ref_time_wr_sec;
    //add_char_params.read_access              = p_cus_init->set_ref_time_rd_sec;

    err_code = characteristic_add(p_cus->service_handle, &add_char_params, &(p_cus->set_ref_time_char_handles));
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Add the Device configuration Characteristic.
    memset(&add_char_params, 0, sizeof(add_char_params));
    add_char_params.uuid                     = BLE_UUID_DEVICE_CONFIGURATION_CHAR_UUID;
    add_char_params.uuid_type                = p_cus->uuid_type;
    add_char_params.max_len                  = BLE_DEV_CONFIGURATION_CHAR_LEN;
    add_char_params.init_len                 = BLE_DEV_CONFIGURATION_CHAR_LEN;
    add_char_params.p_init_value             = (uint8_t *)p_cus_init->p_dev_config_data;
    add_char_params.is_var_len               = false;
    add_char_params.char_props.read = 1;
    add_char_params.read_access     = p_cus_init->dev_config_rd_sec;

    err_code = characteristic_add(p_cus->service_handle, &add_char_params, &(p_cus->dev_configuration_char_handles));
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Add the BLE Profile Version String Characteristic.
    memset(&add_char_params, 0, sizeof(add_char_params));
    add_char_params.uuid                     = BLE_UUID_BLE_PROFILE_VER_STR_CHAR_UUID;
    add_char_params.uuid_type                = p_cus->uuid_type;
    add_char_params.max_len                  = strlen(BLE_PROFILE_VERSION_STR);
    add_char_params.init_len                 = strlen(BLE_PROFILE_VERSION_STR);
    add_char_params.p_init_value             = (uint8_t *)p_cus_init->p_ble_prof_ver_str;
    add_char_params.is_var_len               = false;
    add_char_params.char_props.read = 1;
    add_char_params.read_access     = p_cus_init->ble_prof_ver_str_rd_sec;

    err_code = characteristic_add(p_cus->service_handle, &add_char_params, &(p_cus->ble_prof_ver_str_char_handles));
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Add ECG PPG Mode characteristic
    memset(&add_char_params, 0, sizeof(add_char_params));

    add_char_params.uuid              = BLE_UUID_ECG_PPG_MODE_CHAR_UUID;
    add_char_params.max_len           = BLE_ECG_PPG_MODE_CHAR_LEN;
    add_char_params.init_len          = BLE_ECG_PPG_MODE_CHAR_LEN;
    add_char_params.p_init_value      = (uint8_t *)p_cus_init->p_mode;
    add_char_params.is_var_len        = true;
    add_char_params.char_props.read = 1;
    add_char_params.char_props.notify = 1;
    add_char_params.read_access     =   p_cus_init->ecg_ppg_mode_rd_sec;;
    add_char_params.cccd_write_access = p_cus_init->ecg_ppg_mode_wr_sec;

    err_code = characteristic_add(p_cus->service_handle, &add_char_params, &(p_cus->ecg_ppg_mode_char_handles));
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }
}

/**@brief Function for sending ECG/PPG mode changes if notification has been enabled.
 *
 * @details The application calls this function after having performed a ecg measurement.
 *          If notification has been enabled, the ecg raw samples data is sent to
 *          the client.
 *
 * @param   p_cus                        Custom Service2 structure.
 * @param   ecg_ppg_mode_notf_pkt         ECG Raw Sample Notification Pkt
 *
 * @return  NRF_SUCCESS on successful initialization of service,
 *          otherwise an error code from nrf_error.h.
 */
uint32_t ble_ecg_ppg_mode_send(ble_cus2_t * p_cus, ecg_ppg_mode_char_data_t ecg_ppg_mode_notf_pkt)
{
    uint32_t err_code;

    // Send value if connected and notifying
    if (p_cus->conn_handle != BLE_CONN_HANDLE_INVALID)
    {
        uint16_t               hvx_len, len;
        ble_gatts_hvx_params_t hvx_params;

        len = sizeof(ecg_ppg_mode_char_data_t);
        hvx_len = len;

        memset(&hvx_params, 0, sizeof(hvx_params));

        hvx_params.handle = p_cus->ecg_ppg_mode_char_handles.value_handle;
        hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
        hvx_params.offset = 0;
        hvx_params.p_len  = &hvx_len;
        hvx_params.p_data = (uint8_t *)&ecg_ppg_mode_notf_pkt;

        err_code = sd_ble_gatts_hvx(p_cus->conn_handle, &hvx_params);
        if ((err_code == NRF_SUCCESS) && (hvx_len != len))
        {
            err_code = NRF_ERROR_DATA_SIZE;
        }
    }
    else
    {
        err_code = NRF_ERROR_INVALID_STATE;
    }

    return err_code;
}
#endif//BLE_CUSTOM_PROFILE1