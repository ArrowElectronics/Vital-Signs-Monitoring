/**
 * Copyright (c) 2017 - 2018, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
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
 * 5. Any software provided in binary form under this license must not be
 * reverse engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*---------------------------- Includes --------------------------------------*/
#include "app_timer.h"
#include "app_util_platform.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_bas.h"
#include "ble_hrs.h"
#include "ble_dis.h"
#include "ble_conn_params.h"
#include "ble_hci.h"
#include "ble_nus.h"
#include "bsp_btn_ble.h"
#include "nordic_common.h"
#include "nrf.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_freertos.h"
#include "nrf_sdh_soc.h"
#ifdef HIBERNATE_MD_EN
#include "power_manager.h"
#endif

#include "nrf_delay.h"
#include "nrf_drv_clock.h"
#include "nrf_drv_power.h"
#include "nrf_drv_usbd.h"
#include "nrf_gpio.h"
#include "nrf_log.h"
#include "nrf_pwr_mgmt.h"
#ifdef BLE_PEER_ENABLE
#include "peer_manager.h"
#include "peer_manager_handler.h"
#endif
/* FreeRTOS related */
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "timers.h"
#include "file_system_utils.h"

#include "adp5360.h"
#include "app_util.h"
#include "common_sensor_interface.h"
#include "display_app.h"
#include <adi_osal.h>
#include <app_cfg.h>
#include <ble_task.h>
#include "ble_services_m2m2_protocol.h"
#include "ppg_application_interface.h"
#include <hw_if_config.h>
#include <post_office.h>
#include <system_task.h>
#include <task_includes.h>
#ifdef BLE_CUSTOM_PROFILE1
#include "ble_cus1.h"
#include "ble_cus2.h"
#include "ble_cus3.h"
#endif

#ifdef DEBUG_PKT
#include <ecg_task.h>
#endif

/* For BLE Tx FreeRTOS Task Creation */
/* Create the stack for task */
uint8_t ga_ble_app_task_stack[APP_OS_CFG_BLE_TASK_STK_SIZE];
/* Create handler for task */
ADI_OSAL_THREAD_HANDLE gh_ble_app_task_handler;
/* Create task attributes variable */
ADI_OSAL_STATIC_THREAD_ATTR g_ble_app_task_attributes;
/* Create TCB for task */
StaticTask_t g_ble_app_task_tcb;
/* Create Queue Handler for task */
ADI_OSAL_QUEUE_HANDLE gh_ble_app_task_msg_queue = NULL;
/* Semaphore to block until ble_nus_data_send() is done &
 * BLE_NUS_EVT_TX_RDY received */
//ADI_OSAL_SEM_HANDLE g_ble_nus_evt_sem;
/* Semaphore to block and wakeup ble_tx_task */
ADI_OSAL_SEM_HANDLE g_ble_tx_task_evt_sem;

/* For BLE Services Sensor FreeRTOS Task Creation */
/* Create the stack for task */
uint8_t ga_ble_services_sensor_task_stack[APP_OS_CFG_BLE_SERVICES_SENSOR_TASK_STK_SIZE];
/* Create handler for task */
ADI_OSAL_THREAD_HANDLE gh_ble_services_sensor_task_handler;
/* Create task attributes variable */
ADI_OSAL_STATIC_THREAD_ATTR g_ble_services_sensor_task_attributes;
/* Create TCB for task */
StaticTask_t g_ble_services_sensor_task_tcb;
/* Create Queue Handler for task */
ADI_OSAL_QUEUE_HANDLE gh_ble_services_sensor_task_msg_queue = NULL;

/* Variable which controls enab/disable of ppg sensor init/deinit done for the
 BLE HR service */
static volatile uint8_t gn_ble_hr_service_sensor_enab = 0;
/* Variable which holds the ppg sensor init/deinit done status for the
 BLE HR serivce */
static uint8_t gn_ble_hr_service_sensor_init = 0;

#ifdef BLE_CUSTOM_PROFILE1
/* Variable which controls enab/disable of ecg sensor init/deinit done for the
 ECG Raw Sample service */
static volatile uint8_t gn_ble_ecg_raw_samp_service_sensor_enab = 0;
/* Variable which holds the ecg sensor init/deinit done status for the
 ECG Raw Sample serivce */
uint8_t gn_ble_ecg_raw_samp_service_sensor_init = 0;
/* Variable which controls sending of ECG/PPG Mode notification pkt for the
 Misc service */
static uint8_t gn_ble_ecg_ppg_mode_enab = 0;
/* Variable which controls sending of HR Algo Info notification pkt for the
 Algo Info service */
static uint8_t gn_ble_hr_algo_info_service_sensor_enab = 0;
/* Rate at which other BLE Custom Profile Notifications are to be sent */
/* Below count kept to make this rate as every 1sec
   ECG_APP_FREQUENCY / ECG_SAMPLE_COUNT */
#define MAX_BLE_CUST_PROFILE_NOTF_CNT (20)
/* Variable to count till MAX_BLE_CUST_PROFILE_NOTF_CNT is reached */
static uint8_t gn_ble_cust_notf_cnt = 0;
#endif

/* ble_nus_data_send() api at times return NRF_ERROR_RESOURCES, at the point
 * in which retry api usage needs to be done. Flag to hangle this condition */
volatile uint8_t gsErrResources = 0;
/* variable which keeps track of total msg pkts submitted to
 * ble_nus_data_send() api */
volatile uint32_t gTotalPktSubmit = 0;

/*---------------------------- Defines --------------------------------------*/
/* A tag identifying the SoftDevice BLE configuration. */
#define APP_BLE_CONN_CFG_TAG 1

/* Reply when unsupported features are requested. */
#define APP_FEATURE_NOT_SUPPORTED BLE_GATT_STATUS_ATTERR_APP_BEGIN + 2

/* Manufacturer. Will be passed to Device Information Service. */
#define MANUFACTURER_NAME                   "AnalogDevices"
#define FW_VERSION                          "1.0.1"
#define SERIAL_NUMBER                       "112233"
/* Name of device. Will be included in the advertising data. */
#define DEVICE_NAME "STUDYWATCH"
/* Total length: length of device name plus sizeof('_') plus mac address size
 */
#define DEVICE_NAME_LEN (10 + 1 + (2 * BLE_GAP_ADDR_LEN))
/* UUID type for the Nordic UART Service (vendor specific). */
#define NUS_SERVICE_UUID_TYPE BLE_UUID_TYPE_VENDOR_BEGIN

/* Application's BLE observer priority. You shouldn't need to modify this
 * value. */
#define APP_BLE_OBSERVER_PRIO 3

/* The advertising interval (in units of 0.625 ms. This value corresponds to
 * 625 ms). */
#define APP_ADV_INTERVAL 1000
/* The advertising duration (180 seconds) in units of 10 milliseconds. */
#define APP_ADV_DURATION 18000

/* Change it according to the spec of BLE. for stability */
#define MIN_CONN_INTERVAL MSEC_TO_UNITS(7.5, UNIT_1_25_MS)
/* Change it according to the spec of BLE. for stability */
#define MAX_CONN_INTERVAL MSEC_TO_UNITS(35, UNIT_1_25_MS)
/* Slave latency. */
#define SLAVE_LATENCY 0
/* Connection supervisory timeout (4 seconds). Supervision Timeout uses 10 ms
 * units. */
#define CONN_SUP_TIMEOUT MSEC_TO_UNITS(4000, UNIT_10_MS)
/* Time from initiating an event (connect or start of notification) to the
 * first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define FIRST_CONN_PARAMS_UPDATE_DELAY APP_TIMER_TICKS(5000)
/* Time between each call to sd_ble_gap_conn_param_update after the first call
 * (30 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY APP_TIMER_TICKS(30000)
/* Number of attempts before giving up the connection parameter negotiation.
 */
#define MAX_CONN_PARAMS_UPDATE_COUNT 3

/* Value used as error code on stack dump. Can be used to identify stack
 * location on stack unwind. */
#define DEAD_BEEF 0xDEADBEEF

/* No: of retries to do when ble_nus_send fails */
#define MAX_BLE_TX_RETRY_CNT 40

/* Battery level to ensure safe bootloader entry for DFU */
#define UPGRADE_BATTERY_LEVEL (30)

/* For enabling BLE Peer Manager */
#ifdef BLE_PEER_ENABLE
/* Perform bonding. */
#define SEC_PARAM_BOND 1
/* Man In The Middle protection not required. */
#define SEC_PARAM_MITM 0
/* LE Secure Connections not enabled. */
#define SEC_PARAM_LESC 0
/* Keypress notifications not enabled. */
#define SEC_PARAM_KEYPRESS 0
/* No I/O capabilities. */
#define SEC_PARAM_IO_CAPABILITIES BLE_GAP_IO_CAPS_DISPLAY_ONLY
/* Out Of Band data not available. */
#define SEC_PARAM_OOB 0
/* Minimum encryption key size. */
#define SEC_PARAM_MIN_KEY_SIZE 7
/* Maximum encryption key size. */
#define SEC_PARAM_MAX_KEY_SIZE 16

ble_gap_sec_params_t m_sec_params;
ble_gap_sec_keyset_t m_sec_keyset;
ble_gap_enc_key_t m_own_enc_key;
ble_gap_enc_key_t m_peer_enc_key;
#endif

/* BLE NUS service instance. */
BLE_NUS_DEF(m_nus, NRF_SDH_BLE_TOTAL_LINK_COUNT);
/* GATT module instance. */
NRF_BLE_GATT_DEF(m_gatt);
/* Advertising module instance. */
BLE_ADVERTISING_DEF(m_advertising);
/* Context for the Queued Write module.*/
NRF_BLE_QWR_DEF(m_qwr);
/* Battery service instance. */
BLE_BAS_DEF(m_bas);
/* Heart rate service instance. */
BLE_HRS_DEF(m_hrs);
#ifdef BLE_CUSTOM_PROFILE1
BLE_CUS1_DEF(m_CustomService1);
BLE_CUS2_DEF(m_CustomService2);
BLE_CUS3_DEF(m_CustomService3);
#endif

/* Handle of the current connection. */
static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID;

/* Variable holding current BLE connection status */
static volatile uint8_t gb_ble_status = BLE_DISCONNECTED;
/* Variable to initiate stream stop from all sensors when BLE connection
 * status is either BLE_DISCONNECTED or BLE_PORT_CLOSED */
static volatile bool gb_ble_force_stream_stop = false;
/* Maximum length of data (in bytes) that can be transmitted to the peer by
 * the Nordic UART service module. */
static uint16_t m_ble_nus_max_data_len = BLE_GATT_ATT_MTU_DEFAULT - 3;
/* Universally unique service identifier. */
#ifdef BLE_CUSTOM_PROFILE1
static ble_uuid_t m_adv_uuids_more[] = {
    {BLE_UUID_HEART_RATE_SERVICE, BLE_UUID_TYPE_BLE},
    {BLE_UUID_BATTERY_SERVICE, BLE_UUID_TYPE_BLE},
    {BLE_UUID_DEVICE_INFORMATION_SERVICE, BLE_UUID_TYPE_BLE},
    {BLE_UUID_ECG_RAW_SAMPLE_SERVICE, BLE_UUID_TYPE_VENDOR_BEGIN}
};
#else
static ble_uuid_t m_adv_uuids[] = {
    {BLE_UUID_NUS_SERVICE, NUS_SERVICE_UUID_TYPE},
    {BLE_UUID_HEART_RATE_SERVICE, BLE_UUID_TYPE_BLE},
    {BLE_UUID_BATTERY_SERVICE, BLE_UUID_TYPE_BLE},
    {BLE_UUID_DEVICE_INFORMATION_SERVICE, BLE_UUID_TYPE_BLE}
};
#endif

uint32_t gn_ble_bas_timer_cnt = 0;

/* variable which keeps track of total bytes submitted for BLE Tx */
uint32_t g_ble_tx_byte_cnt = 0;
/* variable which keeps track of total bytes received via BLE Rx */
uint32_t g_ble_rx_byte_cnt = 0;
/* To hold the BLE connect src address(CLI_BLE/DROID/IOS,etc) */
M2M2_ADDR_ENUM_t ble_pkt_src = M2M2_ADDR_UNDEFINED;

/* For BLE data throughput increase:
   From the Watch side, an increase in BLE data throughput happens when Watch
   receives sensor subscribe add from a Tool, following which Watch starts
   sending out data stream packets from the sensor at a particular data rate.
   Based on this, ble_nus_data_send() api can combine MAX_TX_PKT_COMB_CNT
   packets and send out, which reduces the chances of getting
   NRF_ERROR_RESOURCES
*/
/* Maximum No: of PO pkts to combine for sending out to ble nus */
#define MAX_TX_PKT_COMB_CNT 4
/* Minimum No: of PO pkts to combine for sending out to ble nus */
#define MIN_TX_PKT_COMB_CNT 1

/* Flag to decide whether to enable PO pkt combining or not, for BLE Tx */
static uint8_t tx_pkt_comb_stop = 0;
/* Value with which how many PO pkt need to be combined is decided */
static uint8_t tx_pkt_compare_val = MIN_TX_PKT_COMB_CNT;
/* Value with which MAX how many PO pkt need to be combined is decided,
 introduced to be able to use a m2m2 cmd to change what is being done in Fw */
static volatile uint8_t gn_max_tx_kt_comb_cnt = MAX_TX_PKT_COMB_CNT;
/* Combined pkt length from various PO pkt, based on tx_pkt_compare_val. This
 * is the no: of bytes which is sent via BLE Tx */
static uint16_t tx_pkt_len_comb = 0;
/* holds currently how many PO pkts have been combined */
static uint8_t tx_pkt_comb_cnt = 0;
/* to hold the total 'sensor SUB add' resp command from PO, to correspondingly
 * check for UNSUB condition */
static uint8_t sub_add_cnt = 0;
/* flag to check if total msg len in ble_data_array[] is going to exceed
 * BLE_NUS_MAX_DATA_LEN, even before MAX_TX_PKT_COMB_CNT is reached or not;
 * based on which msg needs to be sent */
static uint8_t tx_pkt_send_now = 0;

#ifdef CUST4_SM
/*Variables updated for debugging, BLE Turned On/Off state*/
uint8_t gn_turn_on = false;
uint8_t gn_turn_off = false;
#endif
static void advertising_start(void *p_context);

/* Enable BLE_THROUGHTPUT_DEBUG macro, to see in real time, the BLE throughput
   in SES Debug terminal, when BLE streaming is happening */
//#define BLE_THROUGHTPUT_DEBUG 1

#ifdef BLE_NUS_PROFILE_TIME_ENABLED
uint32_t get_micro_sec(void);
static uint32_t nUSec = 0;
static volatile uint32_t ble_nus_send_time = 0;
static volatile uint32_t max_ble_nus_send_time = 0,
                         min_ble_nus_send_time = 0xFFFFFFFF;
uint32_t gBleTaskMsgPostCnt=0, gBleTaskMsgPostCntFailed=0, gBleTaskMsgProcessCnt=0, gBleTaskMsgStreamDataCnt=0;
#endif
//#define BLE_NUS_PROFILE_TIME_ENABLED 1
#ifdef BLE_NUS_PROFILE_TIME_ENABLED
#include "us_tick.h"
#endif

#ifdef CUST4_SM
#include "user0_config_app_task.h"
extern bool usbd_get_cradle_disconnection_status();
#endif
/*!
 ****************************************************************************
 * @brief Function for assert macro callback.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product.
 * You need to analyze how your product is supposed to react in case of an
 * assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num    Line number of the failing ASSERT call.
 * @param[in] p_file_name File name of the failing ASSERT call.
 ******************************************************************************/
void assert_nrf_callback(uint16_t line_num, const uint8_t *p_file_name) {
  app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

/*!
 ****************************************************************************
 * @brief Function to change max tx pkt combine count value being used in Fw
 *
 * @details This function is to be used by iOS app which wasnt working with higher
 * MTU coming when using MAX_TX_PKT_COMB_CNT=4 from Fw
 *
 * @param[in] max_tx_kt_comb_cnt   value to be set gn_max_tx_kt_comb_cnt to
 * @param[out] 0-> success | 1-> failure
 ******************************************************************************/
uint8_t set_max_tx_pkt_comb_cnt(uint8_t max_tx_kt_comb_cnt) {
    if(max_tx_kt_comb_cnt==0 || max_tx_kt_comb_cnt>4)
      return 1;
    gn_max_tx_kt_comb_cnt = max_tx_kt_comb_cnt;
    return 0;
}

/*!
 ****************************************************************************
 * @brief Function to get the max tx pkt combine count value being used in Fw
 *
 * @details This function is to be used by iOS app which wasnt working with higher
 * MTU coming when using MAX_TX_PKT_COMB_CNT=4 from Fw
 *
 * @param[in] None
 * @param[out] value being set in Fw currently
 ******************************************************************************/
uint8_t get_max_tx_pkt_comb_cnt() {
    return gn_max_tx_kt_comb_cnt;
}

#ifdef CUST4_SM
/*!
 ****************************************************************************
 * @brief Function to turn ON BLE
 *
 * @details This function is to be used by user0 config app state machine
 *          framework to turn BLE ON, when required. This turns on the BLE
 *          advertising.
 * @param[in]  None
 * @param[out] None
 ******************************************************************************/
void turn_on_BLE() {
  /*For debugging*/
  gn_turn_on = true;
  gn_turn_off = false;

  advertising_start(NULL);
}

/*!
 ****************************************************************************
 * @brief Function to turn OFF BLE
 *
 * @details This function is to be used by user0 config app state machine
 *          framework to turn BLE OFF, when required. This disconnects from the
 *          Central if already connected and stops BLE advertising
 * @param[in]  None
 * @param[out] None
 ******************************************************************************/
void turn_off_BLE() {
  /*For debugging*/
  gn_turn_off = true;
  gn_turn_on = false;

  ble_disconnect_and_unbond();
  sd_ble_gap_adv_stop(m_advertising.adv_handle);
}
#endif

/**@brief Function for performing battery measurement and updating the Battery Level characteristic
 *        in Battery Service.
 */
void battery_level_update(void)
{
    ret_code_t err_code;
    uint8_t  battery_level;

    gn_ble_bas_timer_cnt++;

    BATTERY_STATUS_t bat_status;
    if (Adp5360_get_battery_details(&bat_status) == ADP5360_SUCCESS) {
      battery_level = bat_status.level;
    }
    else
      battery_level = 0;

    err_code = ble_bas_battery_level_update(&m_bas, battery_level, BLE_CONN_HANDLE_ALL);
    if ((err_code != NRF_SUCCESS) &&
        (err_code != NRF_ERROR_INVALID_STATE) &&
        (err_code != NRF_ERROR_RESOURCES) &&
        (err_code != NRF_ERROR_BUSY) &&
        (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
       )
    {
        APP_ERROR_HANDLER(err_code);
    }
}

/*!
 ****************************************************************************
 * @brief Function for the GAP initialization.
 *
 * @details This function sets up all the necessary GAP (Generic Access Profile)
 * parameters of the device. It also sets the permissions and appearance.
 ******************************************************************************/
static void gap_params_init(void) {
  uint32_t err_code;
  ble_gap_conn_params_t gap_conn_params;
  ble_gap_conn_sec_mode_t sec_mode;
  uint8_t device_name[DEVICE_NAME_LEN] = {'\0'};

  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

  ble_gap_addr_t ble_addr_t;
  sd_ble_gap_addr_get(&ble_addr_t);

#ifdef BLE_CUSTOM_PROFILE1
  sprintf((char *)device_name, "ADI_%X",
      ble_addr_t.addr[0]);
#else
  sprintf((char *)device_name, "%s_%X%X%X%X%X%X", DEVICE_NAME,
      ble_addr_t.addr[5], ble_addr_t.addr[4], ble_addr_t.addr[3],
      ble_addr_t.addr[2], ble_addr_t.addr[1], ble_addr_t.addr[0]);
#endif

  err_code = sd_ble_gap_device_name_set(&sec_mode,
      //(const uint8_t *) DEVICE_NAME,
      // strlen(DEVICE_NAME));
      device_name, strlen((char *)device_name));
  APP_ERROR_CHECK(err_code);

  memset(&gap_conn_params, 0, sizeof(gap_conn_params));

  gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
  gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
  gap_conn_params.slave_latency = SLAVE_LATENCY;
  gap_conn_params.conn_sup_timeout = CONN_SUP_TIMEOUT;

  err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
  APP_ERROR_CHECK(err_code);
}

/*< Variable to extract the pkt length from pkt header, for BLE Rx message, for
 * PO msg creation */
static m2m2_hdr_t *gp_ble_rx_pkt_hdr = NULL;
/*< Debug variable to check if PO msg creation fails */
static uint32_t gn_ble_pkt_mem_fail = 0;
/*< Variable which gets correspondingly incremented for the variable
 * gTotalPktSubmit- which counts the data buffer submitted to
 * ble_nus_data_send() */
volatile static uint32_t g_ble_tx_rdy_cnt = 0;
/*!
 ****************************************************************************
 * @brief Function for handling the data from the Nordic UART Service.
 *
 * @details This function processes the data received from the Nordic UART BLE
 * Service and sends it to the USBD CDC ACM module.
 *
 * @param[in] p_evt Nordic UART Service event.
 ******************************************************************************/
static void nus_data_handler(ble_nus_evt_t *p_evt) {

  if (p_evt->type == BLE_NUS_EVT_RX_DATA) {
    uint16_t pkt_len;
    m2m2_hdr_t *p_in_pkt = NULL;
    ADI_OSAL_STATUS err;

    NRF_LOG_INFO("Received data from BLE NUS. Routing to PO.Len=%d",
        p_evt->params.rx_data.length);
    NRF_LOG_HEXDUMP_INFO(
        p_evt->params.rx_data.p_data, p_evt->params.rx_data.length);

    /* Send data to post office */
    gp_ble_rx_pkt_hdr = (m2m2_hdr_t *)p_evt->params.rx_data.p_data;
    pkt_len = BYTE_SWAP_16(gp_ble_rx_pkt_hdr->length);
    // pkt_len = p_evt->params.rx_data.length;
    if (pkt_len > BLE_NUS_MAX_DATA_LEN) {
      NRF_LOG_INFO("Received pkt_len greater than %d", BLE_NUS_MAX_DATA_LEN);
      return;
    }
    g_ble_rx_byte_cnt += pkt_len;

    p_in_pkt = post_office_create_msg(pkt_len);
    if (p_in_pkt == NULL) {
      gn_ble_pkt_mem_fail++;
      return;
    }

    memcpy((uint8_t *)p_in_pkt, p_evt->params.rx_data.p_data, pkt_len);
    /* swap from network byte order to little endian */
    p_in_pkt->src = (M2M2_ADDR_ENUM_t)BYTE_SWAP_16(p_in_pkt->src);
    p_in_pkt->dest = (M2M2_ADDR_ENUM_t)BYTE_SWAP_16(p_in_pkt->dest);
    p_in_pkt->length = BYTE_SWAP_16(p_in_pkt->length);
    p_in_pkt->checksum = BYTE_SWAP_16(p_in_pkt->checksum);

    /* Find out Tool used for BLE connection, to handle
     M2M2_PM_SYS_COMMAND_FORCE_STREAM_STOP_REQ upon BLE Disconnection or Port
     Close */
    if (M2M2_ADDR_UNDEFINED == ble_pkt_src)
      ble_pkt_src = p_in_pkt->src;

    /* Check for valid m2m2 source addresses */
    if ((p_in_pkt->src == M2M2_ADDR_APP_CLI_BLE) ||
        (p_in_pkt->src == M2M2_ADDR_APP_WT) ||
        (p_in_pkt->src == M2M2_ADDR_APP_DROID) ||
        (p_in_pkt->src == M2M2_ADDR_APP_IOS) ||
        (p_in_pkt->src == M2M2_ADDR_APP_IOS_STREAM)) {
      // if(M2M2_ADDR_UNDEFINED == ble_pkt_src)
      //  ble_pkt_src = p_in_pkt->src;
      post_office_send(p_in_pkt, &err);
    } else {
      NRF_LOG_INFO("BLE RX: received pkt from incorrect src, consuming msg");
      post_office_consume_msg(p_in_pkt);
    }
  } else if (p_evt->type == BLE_NUS_EVT_TX_RDY) {
    g_ble_tx_rdy_cnt++;
    if (gsErrResources) {
      gsErrResources = 0;
      // adi_osal_SemPost(g_ble_nus_evt_sem);
    }
    //adi_osal_SemPost(g_ble_nus_evt_sem);
  } else if (BLE_NUS_EVT_COMM_STARTED == p_evt->type) {
    gb_ble_status = BLE_PORT_OPENED;
    gb_ble_force_stream_stop = false;
    adi_osal_SemPost(g_ble_tx_task_evt_sem);
  } else if (BLE_NUS_EVT_COMM_STOPPED == p_evt->type) {
    gb_ble_status = BLE_PORT_CLOSED;
    gb_ble_force_stream_stop = true;
	adi_osal_SemPost(g_ble_tx_task_evt_sem);
  }
}

#ifdef BLE_CUSTOM_PROFILE1
/*!
 ****************************************************************************
 * @brief Function for handling the data from the Custom Service1.
 *
 * @details This function processes the data received from the Custom Service1
 *
 * @param[in] p_evt CUS1 event.
 ******************************************************************************/
static void cus1_evt_handler(ble_cus1_evt_t *p_evt) {
  uint32_t err_code;
  uint8_t status;

  if (p_evt->evt_type == BLE_CUS1_EVT_NOTIFICATION_ENABLED) {

    NRF_LOG_INFO("Received enab ECG Raw Sample Service");
    gn_ble_ecg_raw_samp_service_sensor_enab = 1;
    adi_osal_ThreadResumeFromISR(gh_ble_services_sensor_task_handler);

  } else if (p_evt->evt_type == BLE_CUS1_EVT_NOTIFICATION_DISABLED) {
    NRF_LOG_INFO("Received disab ECG Raw Sample Service");
    gn_ble_ecg_raw_samp_service_sensor_enab = 0;
    adi_osal_ThreadResumeFromISR(gh_ble_services_sensor_task_handler);
  }
}

/*!
 ****************************************************************************
 * @brief Function for handling the data from the Custom Service2.
 *
 * @details This function processes the data received from the Custom Service2
 *
 * @param[in] p_evt CUS2 event.
 ******************************************************************************/
static void cus2_evt_handler(ble_cus2_evt_t *p_evt) {
  uint32_t err_code;
  uint8_t status;

  if (p_evt->evt_type == BLE_CUS2_EVT_NOTIFICATION_ENABLED) {

    NRF_LOG_INFO("Received enable ECG/PPG Mode notification");
    gn_ble_ecg_ppg_mode_enab = 1;
  } else if (p_evt->evt_type == BLE_CUS2_EVT_NOTIFICATION_DISABLED) {
    NRF_LOG_INFO("Received disable ECG/PPG Mode notification");
    gn_ble_ecg_ppg_mode_enab = 0;
  }
  else if (p_evt->evt_type == BLE_CUS2_EVT_SET_REF_TIME) {
    uint32_t temp;
    /* Converting to Big Endian as required */
    temp = BYTE_SWAP_32(p_evt->p_set_ref_data->unix_time);
    NRF_LOG_INFO("Received Set Ref Time:%x",temp);
    rtc_timestamp_set(temp);
    rtc_timezone_set(0);
#ifdef ENABLE_WATCH_DISPLAY
    send_private_type_value(DIS_REFRESH_SIGNAL);
#endif
  }
}

/*!
 ****************************************************************************
 * @brief Function for handling the data from the Custom Service3.
 *
 * @details This function processes the data received from the Custom Service3
 *
 * @param[in] p_evt CUS3 event.
 ******************************************************************************/
static void cus3_evt_handler(ble_cus3_evt_t *p_evt) {
  uint32_t err_code;
  uint8_t status;

  if (p_evt->evt_type == BLE_CUS3_EVT_NOTIFICATION_ENABLED) {

    NRF_LOG_INFO("Received enab HR Algo Info Service");
    gn_ble_hr_algo_info_service_sensor_enab = 1;
    //adi_osal_ThreadResumeFromISR(gh_ble_services_sensor_task_handler);

  } else if (p_evt->evt_type == BLE_CUS3_EVT_NOTIFICATION_DISABLED) {
    NRF_LOG_INFO("Received disab HR Algo Info Service");
    gn_ble_hr_algo_info_service_sensor_enab = 0;
    //adi_osal_ThreadResumeFromISR(gh_ble_services_sensor_task_handler);
  }
}
#endif

/*!
 ****************************************************************************
 *@brief Function for handling Queued Write Module errors.
 *
 * @details A pointer to this function will be passed to each service which may need to inform the
 *          application about an error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 ******************************************************************************/
static void nrf_qwr_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}

/*!
 ****************************************************************************
 * @brief Function for initializing services that will be used by the
 * application.
 ******************************************************************************/
static void services_init(void) {
  uint32_t err_code;
  ble_hrs_init_t     hrs_init;
  ble_bas_init_t     bas_init;
  ble_dis_init_t     dis_init;
  nrf_ble_qwr_init_t qwr_init = {0};
  uint8_t            body_sensor_location;
#ifdef BLE_CUSTOM_PROFILE1
  ble_cus1_init_t     cus1_init;
  ble_cus2_init_t     cus2_init;
  ble_cus3_init_t     cus3_init;
#else
  ble_nus_init_t nus_init;

  memset(&nus_init, 0, sizeof(nus_init));

  nus_init.data_handler = nus_data_handler;

  err_code = ble_nus_init(&m_nus, &nus_init);
  APP_ERROR_CHECK(err_code);
#endif

  // Initialize Queued Write Module.
  qwr_init.error_handler = nrf_qwr_error_handler;

  err_code = nrf_ble_qwr_init(&m_qwr, &qwr_init);
  APP_ERROR_CHECK(err_code);

  // Initialize Heart Rate Service.
  body_sensor_location = BLE_HRS_BODY_SENSOR_LOCATION_WRIST;

  memset(&hrs_init, 0, sizeof(hrs_init));

  hrs_init.evt_handler                 = NULL;
  hrs_init.is_sensor_contact_supported = true;
  hrs_init.p_body_sensor_location      = &body_sensor_location;

  // Here the sec level for the Heart Rate Service can be changed/increased.
  hrs_init.hrm_cccd_wr_sec = SEC_OPEN;
  hrs_init.bsl_rd_sec      = SEC_OPEN;

  err_code = ble_hrs_init(&m_hrs, &hrs_init);
  APP_ERROR_CHECK(err_code);

  // Initialize Battery Service.
  memset(&bas_init, 0, sizeof(bas_init));

  // Here the sec level for the Battery Service can be changed/increased.
  bas_init.bl_rd_sec        = SEC_OPEN;
  bas_init.bl_cccd_wr_sec   = SEC_OPEN;
  bas_init.bl_report_rd_sec = SEC_OPEN;

  bas_init.evt_handler          = NULL;
  bas_init.support_notification = true;
  bas_init.p_report_ref         = NULL;
  bas_init.initial_batt_level   = 100;

  err_code = ble_bas_init(&m_bas, &bas_init);
  APP_ERROR_CHECK(err_code);

  // Initialize Device Information Service.
  memset(&dis_init, 0, sizeof(dis_init));

  ble_srv_ascii_to_utf8(&dis_init.manufact_name_str, (char *)MANUFACTURER_NAME);
  ble_srv_ascii_to_utf8(&dis_init.fw_rev_str, (char *)FW_VERSION);
  ble_srv_ascii_to_utf8(&dis_init.serial_num_str, (char *)SERIAL_NUMBER);

  dis_init.dis_char_rd_sec = SEC_OPEN;

  err_code = ble_dis_init(&dis_init);
  APP_ERROR_CHECK(err_code);

#ifdef BLE_CUSTOM_PROFILE1
  /***************************************************************************/
  // Initialize CUS Service1 init structure to zero.
  memset(&cus1_init, 0, sizeof(cus1_init));

  cus1_init.evt_handler = cus1_evt_handler;
  /*
    ECG Range:
    Voltage at electrodes are expected to be
    within 3 mVpp Range

    16-bit, 800 kSPS ADC
  */
  /* Specifying range in uV */
  int16_t range_max = ECG_RANGE_MAX, range_min = ECG_RANGE_MIN;
  /* Converting to Big Endian as required */
  range_max = BYTE_SWAP_16(range_max);
  range_min = BYTE_SWAP_16(range_min);
  hw_configuration_char_data_t hw_config_data = {1, range_max, range_min, ADC_RESOLUTION};
  cus1_init.p_hw_config_data      = &hw_config_data;
  /* Here the sec level for the Custom Service1 can be changed/increased. */
  cus1_init.ecg_raw_samp_cccd_wr_sec = SEC_OPEN;
  cus1_init.hw_config_rd_sec         = SEC_OPEN;

  err_code = ble_cus1_init(&m_CustomService1, &cus1_init);
  APP_ERROR_CHECK(err_code);

  /***************************************************************************/
  /* Initialize CUS Service2 init structure to zero. */
  memset(&cus2_init, 0, sizeof(cus2_init));

  /* Converting to Big Endian as required */
  int16_t ecg_freq = ECG_APP_FREQUENCY;
  ecg_freq = BYTE_SWAP_16(ecg_freq);
  cus2_init.evt_handler = cus2_evt_handler;
  dev_configuration_char_data_t dev_config_data = {1, ecg_freq, 0, 0};
  cus2_init.p_dev_config_data      = &dev_config_data;

  uint8_t ble_prof_ver_str[strlen(BLE_PROFILE_VERSION_STR)];
  strcpy(ble_prof_ver_str, BLE_PROFILE_VERSION_STR);
  cus2_init.p_ble_prof_ver_str = ble_prof_ver_str;

  ECG_PPG_MODE_ENUM_t mode_val = ECG_MODE;
  ecg_ppg_mode_char_data_t mode = {1, mode_val };
  cus2_init.p_mode = &mode;

  // Here the sec level for the Custom Service2 can be changed/increased.
  cus2_init.set_ref_time_wr_sec       = SEC_OPEN;
  cus2_init.set_ref_time_rd_sec       = SEC_OPEN;
  cus2_init.dev_config_rd_sec         = SEC_OPEN;
  cus2_init.ble_prof_ver_str_rd_sec   = SEC_OPEN;
  cus2_init.ecg_ppg_mode_wr_sec       = SEC_OPEN;
  cus2_init.ecg_ppg_mode_rd_sec       = SEC_OPEN;

  err_code = ble_cus2_init(&m_CustomService2, &cus2_init);
  APP_ERROR_CHECK(err_code);

  /***************************************************************************/
  // Initialize CUS Service3 init structure to zero.
  memset(&cus3_init, 0, sizeof(cus3_init));

  cus3_init.evt_handler = cus3_evt_handler;
  hr_algo_info_char_data_t hr_algo_info_data = {1, ECG_HR_SOURCE, ECG_ESTIMATED_HR, 0}; //TODO: Check & Fill
  cus3_init.p_hr_algo_info_data      = &hr_algo_info_data;
  // Here the sec level for the Custom Service3 can be changed/increased.
  cus3_init.hr_algo_info_cccd_wr_sec = SEC_OPEN;

  err_code = ble_cus3_init(&m_CustomService3, &cus3_init);
  APP_ERROR_CHECK(err_code);
  /***************************************************************************/
#endif
}

/** @brief   ppg sensor control flag status for HR Service in BLE
 * @details  Return the gn_ble_hr_service_sensor_enab flag value
 * @param    None
 * @retval   0  --> ppg sensor control flag enabled
 *           1 --> ppg sensor control flag disabled
 */
int get_ble_hr_service_status() {
  return gn_ble_hr_service_sensor_enab;
}

/** @brief   Set the ppg sensor enable/disable for BLE HR Service
 * @details  Set the gn_ble_hr_service_sensor_enab flag value
 * @param    enab --> 0/1
 * @retval   0  --> flag set success 1 --> flag set failed
 */
int set_ble_hr_service_sensor(bool enab) {
  if (gb_ble_status == BLE_CONNECTED)
  {
    gn_ble_hr_service_sensor_enab = enab;
    vTaskResume((TaskHandle_t)gh_ble_services_sensor_task_handler);
    return 0;
  }
  return 1;
}

/*!
 ****************************************************************************
 * @brief  BLE Services Sensor task - handle post office messages to control ble tx
 * activities
 * @param  pArgument not used
 * @return None
 ******************************************************************************/
static void ble_services_sensor_task(void *arg) {
  m2m2_hdr_t *p_in_pkt = NULL;
  uint16_t msg_len;
  uint32_t err_code;
  uint8_t status;

  while (1) {

    vTaskSuspend(NULL);

    if (gb_ble_status == BLE_CONNECTED) {

      /*While connected to BLE, if gn_ble_hr_service_sensor_enab is set to 1,
        do ppg sensor init */
      if(gn_ble_hr_service_sensor_enab && !gn_ble_hr_service_sensor_init)
      {
        err_code = ble_services_sensor_m2m2_subscribe_stream(M2M2_ADDR_MED_PPG,M2M2_ADDR_MED_PPG_STREAM,&status);
        APP_ERROR_CHECK(err_code);
        if(M2M2_APP_COMMON_STATUS_SUBSCRIBER_ADDED != status)
        {
            NRF_LOG_INFO("subscribe rppg fail,status = %d",status);
            return;
        }
        ble_services_sensor_m2m2_set_stream_status(SENSOR_PPG,1);
        gn_ble_hr_service_sensor_init = 1;
        continue;
      }

      /*While connected to BLE, if gn_ble_hr_service_sensor_enab is set to 0,
        do ppg sensor de-init */
      else if(!gn_ble_hr_service_sensor_enab && gn_ble_hr_service_sensor_init)
      {
          err_code = ble_services_sensor_m2m2_unsubscribe_stream(M2M2_ADDR_MED_PPG,M2M2_ADDR_MED_PPG_STREAM,&status);
          APP_ERROR_CHECK(err_code);
          if((M2M2_APP_COMMON_STATUS_SUBSCRIBER_REMOVED != status)
              &&(M2M2_APP_COMMON_STATUS_SUBSCRIBER_COUNT_DECREMENT != status))
          {
              NRF_LOG_INFO("unsubscribe stream fail,status = %d",status);
              //return;//even unsubscribe failure, the page need jump.
          }
          ble_services_sensor_m2m2_set_stream_status(SENSOR_PPG,0);
          gn_ble_hr_service_sensor_init = 0;
          continue;
      }

#ifdef BLE_CUSTOM_PROFILE1
      /*While connected to BLE, if gn_ble_ecg_raw_samp_service_sensor_enab is set to 1,
        do ecg sensor init */
      if(gn_ble_ecg_raw_samp_service_sensor_enab && !gn_ble_ecg_raw_samp_service_sensor_init)
      {
        //Give Start ECG cmds
        err_code = ble_services_sensor_m2m2_subscribe_stream(M2M2_ADDR_MED_ECG,M2M2_ADDR_MED_ECG_STREAM,&status);
        APP_ERROR_CHECK(err_code);
        if(M2M2_APP_COMMON_STATUS_SUBSCRIBER_ADDED != status)
        {
            NRF_LOG_INFO("subscribe rppg fail,status = %d",status);
            return;
        }
        err_code = ble_services_sensor_m2m2_set_stream_status(SENSOR_ECG,1);
        APP_ERROR_CHECK(err_code);
        gn_ble_ecg_raw_samp_service_sensor_init = 1;

        //reset variable value, for comparison with MAX_BLE_CUST_PROFILE_NOTF_CNT
        gn_ble_cust_notf_cnt = 0;

        //Send notification pkt for ECG/PPG Mode chage
        if(gn_ble_ecg_ppg_mode_enab)
        {
          ECG_PPG_MODE_ENUM_t mode_val = ECG_MODE;
          ecg_ppg_mode_char_data_t mode = {1, mode_val };
          ble_ecg_ppg_mode_send(&m_CustomService2, mode);
        }

        continue;
      }

      /*While connected to BLE, if gn_ble_ecg_raw_samp_service_sensor_enab is set to 0,
        do ecg sensor de-init */
      else if(!gn_ble_ecg_raw_samp_service_sensor_enab && gn_ble_ecg_raw_samp_service_sensor_init)
      {
        //Give Stop ECG cmds
        err_code = ble_services_sensor_m2m2_unsubscribe_stream(M2M2_ADDR_MED_ECG,M2M2_ADDR_MED_ECG_STREAM,&status);
        APP_ERROR_CHECK(err_code);
        if((M2M2_APP_COMMON_STATUS_SUBSCRIBER_REMOVED != status)
            &&(M2M2_APP_COMMON_STATUS_SUBSCRIBER_COUNT_DECREMENT != status))
        {
            NRF_LOG_INFO("unsubscribe stream fail,status = %d",status);
            //return;//even unsubscribe failure, the page need jump.
        }
        err_code = ble_services_sensor_m2m2_set_stream_status(SENSOR_ECG,0);
        APP_ERROR_CHECK(err_code);
        gn_ble_ecg_raw_samp_service_sensor_init = 0;

        //Send notification pkt for ECG/PPG Mode chage
        if(gn_ble_ecg_ppg_mode_enab)
        {
          ECG_PPG_MODE_ENUM_t mode_val = NOT_SAMPLING;
          ecg_ppg_mode_char_data_t mode = {1, mode_val };
          ble_ecg_ppg_mode_send(&m_CustomService2, mode);
        }
        continue;
      }

#endif

      p_in_pkt =post_office_get(ADI_OSAL_TIMEOUT_FOREVER, APP_OS_CFG_BLE_SERVICES_SENSOR_TASK_INDEX);
      if (p_in_pkt == NULL) {
        /* No m2m2 messages to process, so fetch some data from the device. */
        continue;
      } else {
        /* We got an m2m2 message from the queue, process it. */
        PYLD_CST(p_in_pkt, _m2m2_app_common_cmd_t,p_in_cmd); /* to extract the
                                      stream data pkts from PO */
        msg_len = p_in_pkt->length;

        if(p_in_pkt->src==M2M2_ADDR_MED_PPG_STREAM)
        {
            ppg_app_hr_debug_stream_t *payload = NULL;
            payload = (ppg_app_hr_debug_stream_t *)&p_in_pkt->data[0];
            if(payload->command == M2M2_SENSOR_COMMON_CMD_STREAM_DATA)
            {
                err_code = ble_hrs_heart_rate_measurement_send(&m_hrs, (payload->hr>>4));
                if ((err_code != NRF_SUCCESS) &&
                    (err_code != NRF_ERROR_INVALID_STATE) &&
                    (err_code != NRF_ERROR_RESOURCES) &&
                    (err_code != NRF_ERROR_BUSY) &&
                    (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
                   )
                {
                    APP_ERROR_HANDLER(err_code);
                }
                //Update about sensor contact detected
                //ble_hrs_sensor_contact_detected_update(&m_hrs, sensor_contact_detected);
                //Update RR interval
                //ble_hrs_rr_interval_add(&m_hrs, rr_interval);
            }
        }

#ifdef BLE_CUSTOM_PROFILE1
        //Check for SRC from ECG Stream
        else if(p_in_pkt->src==M2M2_ADDR_MED_ECG_STREAM)
        {
            /* Extract ecg raw sample notification pkt to be sent */
            ecg_raw_sample_char_data_t payload;

            memcpy(&payload, (uint8_t *)&p_in_pkt->data[0], \
                             sizeof(ecg_raw_sample_char_data_t));

            //Send ECG Raw Sample Notification
            err_code = ble_raw_ecg_sample_send(&m_CustomService1, payload);
            if ((err_code != NRF_SUCCESS) &&
                (err_code != NRF_ERROR_INVALID_STATE) &&
                (err_code != NRF_ERROR_RESOURCES) &&
                (err_code != NRF_ERROR_BUSY) &&
                (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
               )
            {
                APP_ERROR_HANDLER(err_code);
            }

            if(++gn_ble_cust_notf_cnt >= MAX_BLE_CUST_PROFILE_NOTF_CNT )
            {
              //reset variable value, for comparison with MAX_BLE_CUST_PROFILE_NOTF_CNT
              gn_ble_cust_notf_cnt = 0;

              /* Extract ecg HR notification pkt to be sent */
              uint16_t ecg_hr;
              uint16_t *ptr = (uint16_t *) ((uint8_t *)&(p_in_pkt->data[0]) + sizeof(ecg_raw_sample_char_data_t));

              ptr += 1;
              memcpy(&ecg_hr, (uint8_t *)ptr, sizeof(uint16_t));

              //Send HR notification
              err_code = ble_hrs_heart_rate_measurement_send(&m_hrs, ecg_hr);
              if ((err_code != NRF_SUCCESS) &&
                  (err_code != NRF_ERROR_INVALID_STATE) &&
                  (err_code != NRF_ERROR_RESOURCES) &&
                  (err_code != NRF_ERROR_BUSY) &&
                  (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
                 )
              {
                  APP_ERROR_HANDLER(err_code);
              }

              //Send HR Algo Info Notification, if enabled
              if(gn_ble_hr_algo_info_service_sensor_enab)
              {
                hr_algo_info_char_data_t hr_algo_info_notf_pkt = {1, ECG_HR_SOURCE, ECG_ESTIMATED_HR, 0};
                err_code = ble_hr_algo_info_send(&m_CustomService3, hr_algo_info_notf_pkt);
                if ((err_code != NRF_SUCCESS) &&
                      (err_code != NRF_ERROR_INVALID_STATE) &&
                      (err_code != NRF_ERROR_RESOURCES) &&
                      (err_code != NRF_ERROR_BUSY) &&
                      (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
                     )
                  {
                      APP_ERROR_HANDLER(err_code);
                  }
              }
            }//gn_ble_cust_notf_cnt check
        }
#endif
        /* Swap the header around so that the length can be properly freed. */
        p_in_pkt->length = msg_len;
        post_office_consume_msg(p_in_pkt);
      }
    } // End of if condition for ble connection ON
    else {
        if(gn_ble_hr_service_sensor_enab && gn_ble_hr_service_sensor_init)
        {
          err_code = ble_services_sensor_m2m2_unsubscribe_stream(M2M2_ADDR_MED_PPG,M2M2_ADDR_MED_PPG_STREAM,&status);
          APP_ERROR_CHECK(err_code);
          if((M2M2_APP_COMMON_STATUS_SUBSCRIBER_REMOVED != status)
              &&(M2M2_APP_COMMON_STATUS_SUBSCRIBER_COUNT_DECREMENT != status))
          {
              NRF_LOG_INFO("unsubscribe stream fail,status = %d",status);
              //return;//even unsubscribe failure, the page need jump.
          }
          ble_services_sensor_m2m2_set_stream_status(SENSOR_PPG,0);
          gn_ble_hr_service_sensor_enab = 0;
          gn_ble_hr_service_sensor_init = 0;
        }

        p_in_pkt = post_office_get(ADI_OSAL_TIMEOUT_FOREVER, APP_OS_CFG_BLE_SERVICES_SENSOR_TASK_INDEX);
        if (p_in_pkt != NULL)
            post_office_consume_msg(p_in_pkt);
#ifdef BLE_CUSTOM_PROFILE1
        if(gn_ble_ecg_raw_samp_service_sensor_enab && gn_ble_ecg_raw_samp_service_sensor_init)
        {
          err_code = ble_services_sensor_m2m2_unsubscribe_stream(M2M2_ADDR_MED_ECG,M2M2_ADDR_MED_ECG_STREAM,&status);
          APP_ERROR_CHECK(err_code);
          if((M2M2_APP_COMMON_STATUS_SUBSCRIBER_REMOVED != status)
              &&(M2M2_APP_COMMON_STATUS_SUBSCRIBER_COUNT_DECREMENT != status))
          {
              NRF_LOG_INFO("unsubscribe stream fail,status = %d",status);
              //return;//even unsubscribe failure, the page need jump.
          }
          ble_services_sensor_m2m2_set_stream_status(SENSOR_ECG,0);
          gn_ble_ecg_raw_samp_service_sensor_enab = 0;
          gn_ble_ecg_raw_samp_service_sensor_init = 0;
        }
#endif
    }
  }//end of while(1)
}

void send_message_ble_services_sensor_task(m2m2_hdr_t *p_pkt) {
  ADI_OSAL_STATUS osal_result;
  osal_result = adi_osal_MsgQueuePost(gh_ble_services_sensor_task_msg_queue,p_pkt);
  if (osal_result != ADI_OSAL_SUCCESS)
    post_office_consume_msg(p_pkt);
  vTaskResume((TaskHandle_t)gh_ble_services_sensor_task_handler);
}

/*!
 ****************************************************************************
 *@brief Function for initializing the task which handles and configures
 *       sensors required by the BLE HR service
 *
 * @details Initialize the task which handles data from Heart Rate sensor for HRS
 ******************************************************************************/
void ble_services_sensor_task_init()
{
  ADI_OSAL_STATUS eOsStatus;

  /* Create ble services sensor thread */
  g_ble_services_sensor_task_attributes.pThreadFunc = ble_services_sensor_task;
  g_ble_services_sensor_task_attributes.nPriority = APP_OS_CFG_BLE_SERVICES_SENSOR_TASK_PRIO;
  g_ble_services_sensor_task_attributes.pStackBase = &ga_ble_services_sensor_task_stack[0];
  g_ble_services_sensor_task_attributes.nStackSize = APP_OS_CFG_BLE_SERVICES_SENSOR_TASK_STK_SIZE;
  g_ble_services_sensor_task_attributes.pTaskAttrParam = NULL;
  g_ble_services_sensor_task_attributes.szThreadName = "ble_services_sensor_task";
  g_ble_services_sensor_task_attributes.pThreadTcb = &g_ble_services_sensor_task_tcb;
  eOsStatus = adi_osal_MsgQueueCreate(&gh_ble_services_sensor_task_msg_queue, NULL,
      60); /* Incease this value as the ble buffer. */
  if (eOsStatus != ADI_OSAL_SUCCESS) {
    Debug_Handler();
  } else {
    update_task_queue_list(
        APP_OS_CFG_BLE_SERVICES_SENSOR_TASK_INDEX, gh_ble_services_sensor_task_msg_queue);
  }

  eOsStatus = adi_osal_ThreadCreateStatic(
      &gh_ble_services_sensor_task_handler, &g_ble_services_sensor_task_attributes);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
    Debug_Handler();
  }
}

/*!
 ****************************************************************************
 * @brief Function for handling errors from the Connection Parameters module.
 *
 * @param[in] nrf_error  Error code containing information about what went
 * wrong.
 ******************************************************************************/
static void conn_params_error_handler(uint32_t nrf_error) {
  APP_ERROR_HANDLER(nrf_error);
}

/*!
 ****************************************************************************
 * @brief Function for initializing the Connection Parameters module.
 ******************************************************************************/
static void conn_params_init(void) {
  uint32_t err_code;
  ble_conn_params_init_t cp_init;

  memset(&cp_init, 0, sizeof(cp_init));

  cp_init.p_conn_params = NULL;
  cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
  cp_init.next_conn_params_update_delay = NEXT_CONN_PARAMS_UPDATE_DELAY;
  cp_init.max_conn_params_update_count = MAX_CONN_PARAMS_UPDATE_COUNT;
  cp_init.start_on_notify_cccd_handle = BLE_GATT_HANDLE_INVALID;
  cp_init.disconnect_on_fail = true;
  cp_init.evt_handler = NULL;
  cp_init.error_handler = conn_params_error_handler;

  err_code = ble_conn_params_init(&cp_init);
  APP_ERROR_CHECK(err_code);
}

#if 0
/*!
 ****************************************************************************
 * @brief Function for putting the chip into sleep mode.
 *
 * @note This function does not return.
 ******************************************************************************/
static void sleep_mode_enter(void)
{
    uint32_t err_code = bsp_indication_set(BSP_INDICATE_IDLE);
    APP_ERROR_CHECK(err_code);

    // Prepare wakeup buttons.
    err_code = bsp_btn_ble_sleep_mode_prepare();
    APP_ERROR_CHECK(err_code);

    // Go to system-off mode (this function will not return; wakeup will cause a
reset). err_code = sd_power_system_off(); APP_ERROR_CHECK(err_code);
}
#endif

/*!
 ****************************************************************************
 * @brief Function for starting advertising.
 *****************************************************************************/
static void advertising_start(void *p_context) {
  UNUSED_PARAMETER(p_context);

#ifdef CUST4_SM
  if(!gn_turn_off)
  {
#endif
  uint32_t err_code = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);
  APP_ERROR_CHECK(err_code);
#ifdef CUST4_SM
  }
#endif
}

/*!
 ****************************************************************************
 * @brief Function for handling advertising events.
 *
 * @details This function is called for advertising events which are passed to
 * the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 ******************************************************************************/
static void on_adv_evt(ble_adv_evt_t ble_adv_evt) {
  switch (ble_adv_evt) {
  case BLE_ADV_EVT_FAST:
    break;
  case BLE_ADV_EVT_IDLE:
#ifdef CUST4_SM
    //Check curr state and do re-adv only for intermittent state
    if(get_user0_config_app_state() == STATE_INTERMITTENT_MONITORING)
    {
      NRF_LOG_INFO("Advertising timeout, entering sleep state.")
      user0_config_app_enter_state_sleep();
    }
    else
    {
#endif
      NRF_LOG_INFO("Advertising timeout, restarting.")
      advertising_start(NULL);
#ifdef CUST4_SM
    }
#endif
    break;
  default:
    break;
  }
}

#ifdef BLE_PEER_ENABLE
#include "display_app.h"
static uint8_t peer_password[6] = {0};
void ble_peer_password_get(uint8_t *passwd) {
  if (NULL != passwd) {
    memcpy(passwd, peer_password, 6);
  }
}
#endif

/*!
 ****************************************************************************
 * @brief Function for handling BLE events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 * @param[in]   p_context   Unused.
 ******************************************************************************/
static void ble_evt_handler(ble_evt_t const *p_ble_evt, void *p_context) {
  uint32_t err_code;
#ifdef BLE_PEER_ENABLE
  pm_handler_secure_on_connection(p_ble_evt);
#endif
  switch (p_ble_evt->header.evt_id) {
  case BLE_GAP_EVT_CONNECTED:
    NRF_LOG_INFO("BLE NUS connected");
    m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
#ifdef HIBERNATE_MD_EN
    hibernate_md_clear(HIB_MD_BLE_DISCONNECT_EVT);
    hibernate_mode_entry();
#endif
    gb_ble_status = BLE_CONNECTED;
    gb_ble_force_stream_stop = false;
    /* Reset the flags for TX PKT Combining */
    /* flag to decide whether to enable tx packet combining or not */
    tx_pkt_comb_stop = 0;
    /* combined tx pkt length which is sent out */
    tx_pkt_len_comb = 0;
    /* value with which how many pkt need to be combined is to decided */
    tx_pkt_compare_val = MIN_TX_PKT_COMB_CNT;
    /* holds currently how many pkts have been combined */
    tx_pkt_comb_cnt = 0;
    gn_max_tx_kt_comb_cnt = MAX_TX_PKT_COMB_CNT;
    sub_add_cnt = 0;
    gn_ble_bas_timer_cnt = 0;
    err_code = nrf_ble_qwr_conn_handle_assign(&m_qwr, m_conn_handle);
    APP_ERROR_CHECK(err_code);

#ifdef CUST4_SM
     //Watch connected to tool and its connected to cradle for charging
     //if(get_user0_config_app_state() == STATE_START &&
     if(get_user0_config_app_state() == STATE_ADMIT_STANDBY &&
        !usbd_get_cradle_disconnection_status())
     {
        user0_config_app_enter_state_admit_standby();
     }
     //Watch connected to tool and its disconnected from cradle
     else if(get_user0_config_app_state() == STATE_ADMIT_STANDBY &&
             usbd_get_cradle_disconnection_status())
     {
        user0_config_app_enter_state_start_monitoring();
     }
#endif

#if 0
    //Update to use 2MBPS PHY
    NRF_LOG_DEBUG("PHY update request.");
    ble_gap_phys_t const phys = {
        .rx_phys = BLE_GAP_PHY_2MBPS,
        .tx_phys = BLE_GAP_PHY_2MBPS,
    };
    err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
    APP_ERROR_CHECK(err_code);
#endif

    break;

  case BLE_GAP_EVT_DISCONNECTED:
    NRF_LOG_INFO("BLE NUS disconnected");
    NRF_LOG_INFO("disconnect reason %d", p_ble_evt->evt.gap_evt.params.disconnected.reason);
    m_conn_handle = BLE_CONN_HANDLE_INVALID;
    gb_ble_status = BLE_DISCONNECTED;
#ifdef HIBERNATE_MD_EN
    hibernate_md_set(HIB_MD_BLE_DISCONNECT_EVT);
    hibernate_mode_entry();
#endif
    gb_ble_force_stream_stop = true;
    adi_osal_SemPost(g_ble_tx_task_evt_sem);
#ifdef BLE_PEER_ENABLE
    dis_page_back();
#endif
    sd_ble_gap_adv_stop(m_advertising.adv_handle);
    advertising_start(NULL);
    gsErrResources = 0;
    adi_osal_ThreadResumeFromISR(gh_ble_services_sensor_task_handler);
#ifdef CUST4_SM
     //Unexpected BLE disconnection, when in STATE_ADMIT_STANDBY
     /*Do nothing*/
     //Central gave BLE disconnection, when in STATE_ADMIT_STANDBY
     /*Do nothing*/

     /*Unexpected BLE disconnection, when in STATE_START_MONITORING,
     STATE_INTERMITTENT_MONITORING*/
     /*Stay in same state*/
     //p_ble_evt->evt.gap_evt.params.disconnected.reason == BLE_HCI_CONNECTION_TIMEOUT


     /*Central gave BLE disconnection, when in STATE_START_MONITORING or
     STATE_INTERMITTENT_MONITORING */
     if( p_ble_evt->evt.gap_evt.params.disconnected.reason == BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION
     && (get_user0_config_app_state() == STATE_START_MONITORING
     || get_user0_config_app_state() == STATE_INTERMITTENT_MONITORING) )
     {
        user0_config_app_enter_state_sleep();
     }

#endif
    break;

  case BLE_GAP_EVT_PHY_UPDATE_REQUEST: {
    NRF_LOG_DEBUG("PHY update request.");
    ble_gap_phys_t const phys = {
        .rx_phys = BLE_GAP_PHY_2MBPS,
        .tx_phys = BLE_GAP_PHY_2MBPS,
    };
    err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
    APP_ERROR_CHECK(err_code);
  } break;

  case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
#ifndef BLE_PEER_ENABLE
    /* Pairing not supported. */
    err_code = sd_ble_gap_sec_params_reply(m_conn_handle, BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP, NULL, NULL);
    APP_ERROR_CHECK(err_code);
#endif
    break;
#ifdef BLE_PEER_ENABLE
  case BLE_GAP_EVT_PASSKEY_DISPLAY: {
#ifdef ENABLE_WATCH_DISPLAY
    send_global_type_value(DIS_BLE_PEER_REQUEST);
#endif
    memcpy(peer_password, p_ble_evt->evt.gap_evt.params.passkey_display.passkey,
        6);
    NRF_LOG_HEXDUMP_INFO(peer_password, 6);
  } break;
#endif
  case BLE_GAP_EVT_DATA_LENGTH_UPDATE_REQUEST: {
    ble_gap_data_length_params_t dl_params;

    /* Clearing the struct will effectively set members to @ref
       BLE_GAP_DATA_LENGTH_AUTO. */
    memset(&dl_params, 0, sizeof(ble_gap_data_length_params_t));
    err_code = sd_ble_gap_data_length_update(
        p_ble_evt->evt.gap_evt.conn_handle, &dl_params, NULL);
    APP_ERROR_CHECK(err_code);
  } break;

  case BLE_GATTS_EVT_SYS_ATTR_MISSING:
    /* No system attributes have been stored. */
    err_code = sd_ble_gatts_sys_attr_set(m_conn_handle, NULL, 0, 0);
    APP_ERROR_CHECK(err_code);
    break;

  case BLE_GATTC_EVT_TIMEOUT:
    /* Disconnect on GATT Client timeout event. */
    err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
        BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
    APP_ERROR_CHECK(err_code);
    break;

  case BLE_GATTS_EVT_TIMEOUT:
    /* Disconnect on GATT Server timeout event. */
    err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
        BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
    APP_ERROR_CHECK(err_code);
    break;

  case BLE_EVT_USER_MEM_REQUEST:
    err_code =
        sd_ble_user_mem_reply(p_ble_evt->evt.gattc_evt.conn_handle, NULL);
    APP_ERROR_CHECK(err_code);
    break;

  case BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST: {
    ble_gatts_evt_rw_authorize_request_t req;
    ble_gatts_rw_authorize_reply_params_t auth_reply;

    req = p_ble_evt->evt.gatts_evt.params.authorize_request;

    if (req.type != BLE_GATTS_AUTHORIZE_TYPE_INVALID) {
      if ((req.request.write.op == BLE_GATTS_OP_PREP_WRITE_REQ) ||
          (req.request.write.op == BLE_GATTS_OP_EXEC_WRITE_REQ_NOW) ||
          (req.request.write.op == BLE_GATTS_OP_EXEC_WRITE_REQ_CANCEL)) {
        if (req.type == BLE_GATTS_AUTHORIZE_TYPE_WRITE) {
          auth_reply.type = BLE_GATTS_AUTHORIZE_TYPE_WRITE;
        } else {
          auth_reply.type = BLE_GATTS_AUTHORIZE_TYPE_READ;
        }
        auth_reply.params.write.gatt_status = APP_FEATURE_NOT_SUPPORTED;
        err_code = sd_ble_gatts_rw_authorize_reply(
            p_ble_evt->evt.gatts_evt.conn_handle, &auth_reply);
        APP_ERROR_CHECK(err_code);
      }
    }
  } break; // BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST
  case BLE_GAP_EVT_AUTH_STATUS: {
    NRF_LOG_INFO("BLE_GAP_EVT_AUTH_STATUS: status=0x%x bond=0x%x lv4: %d "
                 "kdist_own:0x%x kdist_peer:0x%x",
        p_ble_evt->evt.gap_evt.params.auth_status.auth_status,
        p_ble_evt->evt.gap_evt.params.auth_status.bonded,
        p_ble_evt->evt.gap_evt.params.auth_status.sm1_levels.lv4,
        *((uint8_t *)&p_ble_evt->evt.gap_evt.params.auth_status.kdist_own),
        *((uint8_t *)&p_ble_evt->evt.gap_evt.params.auth_status.kdist_peer));
  } break;
  case BLE_GAP_EVT_CONN_PARAM_UPDATE: {
    NRF_LOG_DEBUG("Received BLE_GAP_EVT_CONN_PARAM_UPDATE");

    ble_gap_conn_params_t const *p_conn =
        &p_ble_evt->evt.gap_evt.params.conn_param_update.conn_params;

    NRF_LOG_INFO("max_conn_interval: %d", p_conn->max_conn_interval);
    NRF_LOG_INFO("min_conn_interval: %d", p_conn->min_conn_interval);
    NRF_LOG_INFO("slave_latency: %d", p_conn->slave_latency);
    NRF_LOG_INFO("conn_sup_timeout: %d", p_conn->conn_sup_timeout);
  } break;
  case BLE_GAP_EVT_DATA_LENGTH_UPDATE: {
    NRF_LOG_DEBUG(
        "Received BLE_GAP_EVT_DATA_LENGTH_UPDATE (%u, max_rx_time %u).",
        p_ble_evt->evt.gap_evt.params.data_length_update.effective_params
            .max_rx_octets,
        p_ble_evt->evt.gap_evt.params.data_length_update.effective_params
            .max_rx_time_us);
  } break;
  case BLE_GAP_EVT_PHY_UPDATE: {
    NRF_LOG_DEBUG("Received BLE_GAP_EVT_PHY_UPDATE (RX:%d, TX:%d, status:%d)",
        p_ble_evt->evt.gap_evt.params.phy_update.rx_phy,
        p_ble_evt->evt.gap_evt.params.phy_update.tx_phy,
        p_ble_evt->evt.gap_evt.params.phy_update.status);
  } break;
  default:
    /* No implementation needed. */
    break;
  }
}
#ifdef BLE_PEER_ENABLE
/*!
 ****************************************************************************
 *@brief Function for handling Peer Manager events.
 *
 * @param[in] p_evt  Peer Manager event.
 ******************************************************************************/
static void pm_evt_handler(pm_evt_t const *p_evt) {
  ret_code_t err_code;
  bool is_indication_enabled;

  pm_handler_on_pm_evt(p_evt);
  pm_handler_disconnect_on_sec_failure(p_evt);
  pm_handler_flash_clean(p_evt);
  // NRF_LOG_INFO("pm_evt_handler,evt_id = %d",p_evt->evt_id);
  switch (p_evt->evt_id) {
  case PM_EVT_CONN_SEC_SUCCEEDED:
  case PM_EVT_CONN_SEC_FAILED:

    dis_page_back();
    break;

  case PM_EVT_PEERS_DELETE_SUCCEEDED:
    advertising_start(NULL);
    break;
  case PM_EVT_CONN_SEC_CONFIG_REQ: {
    /* Reject pairing request from an already bonded peer. */
    pm_conn_sec_config_t conn_sec_config = {.allow_repairing = true};
    pm_conn_sec_config_reply(p_evt->conn_handle, &conn_sec_config);
  } break;

  default:
    break;
  }
}

/*!
 ****************************************************************************
 *@brief Function for the Peer Manager initialization.
 ******************************************************************************/
static void peer_manager_init(void) {
  ble_gap_sec_params_t sec_param;
  ret_code_t err_code;

  err_code = pm_init();
  APP_ERROR_CHECK(err_code);

  memset(&m_sec_params, 0, sizeof(m_sec_params));

  // Security parameters to be used for all security procedures.
  m_sec_params.bond = SEC_PARAM_BOND;
  m_sec_params.mitm = SEC_PARAM_MITM;
  m_sec_params.lesc = SEC_PARAM_LESC;
  m_sec_params.keypress = SEC_PARAM_KEYPRESS;
  m_sec_params.io_caps = SEC_PARAM_IO_CAPABILITIES;
  m_sec_params.oob = SEC_PARAM_OOB;
  m_sec_params.min_key_size = SEC_PARAM_MIN_KEY_SIZE;
  m_sec_params.max_key_size = SEC_PARAM_MAX_KEY_SIZE;
  m_sec_params.kdist_own.enc = 1;
  m_sec_params.kdist_own.id = 1;
  m_sec_params.kdist_peer.enc = 1;
  m_sec_params.kdist_peer.id = 1;

  err_code = pm_sec_params_set(&m_sec_params);
  APP_ERROR_CHECK(err_code);

  err_code = pm_register(pm_evt_handler);
  APP_ERROR_CHECK(err_code);
}
#endif
/*!
 ****************************************************************************
 * @brief Function for the SoftDevice initialization.
 *
 * @details This function initializes the SoftDevice and the BLE event
 * interrupt.
 ******************************************************************************/
static void ble_stack_init(void) {
  ret_code_t err_code;

  err_code = nrf_sdh_enable_request();
  APP_ERROR_CHECK(err_code);

  /* Configure the BLE stack using the default settings.
   Fetch the start address of the application RAM. */
  uint32_t ram_start = 0;
  err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
  APP_ERROR_CHECK(err_code);

  /* Increase the internal BLE Tx buffer queue size*/
  ble_cfg_t ble_cfg;
  memset(&ble_cfg, 0, sizeof ble_cfg);
  ble_cfg.conn_cfg.conn_cfg_tag = APP_BLE_CONN_CFG_TAG;
  ble_cfg.conn_cfg.params.gatts_conn_cfg.hvn_tx_queue_size = 50;
  err_code = sd_ble_cfg_set(BLE_CONN_CFG_GATTS, &ble_cfg, ram_start);
  APP_ERROR_CHECK(err_code);

#ifdef BLE_CUSTOM_PROFILE1
  //memset(&ble_cfg, 0, sizeof(ble_cfg));
  //// Set the ATT table size.
  //ble_cfg.gatts_cfg.attr_tab_size.attr_tab_size = 2000;
  //err_code = sd_ble_cfg_set(BLE_GATTS_CFG_ATTR_TAB_SIZE, &ble_cfg, ram_start);
#endif

  /* Enable BLE stack. */
  err_code = nrf_sdh_ble_enable(&ram_start);
  APP_ERROR_CHECK(err_code);

  /* Register a handler for BLE events. */
  NRF_SDH_BLE_OBSERVER(
      m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
}

/*!
 ****************************************************************************
 * @brief Function for handling events from the GATT library.
 ******************************************************************************/
void gatt_evt_handler(nrf_ble_gatt_t *p_gatt, nrf_ble_gatt_evt_t const *p_evt) {
  if ((m_conn_handle == p_evt->conn_handle) &&
      (p_evt->evt_id == NRF_BLE_GATT_EVT_ATT_MTU_UPDATED)) {
    m_ble_nus_max_data_len =
        p_evt->params.att_mtu_effective - OPCODE_LENGTH - HANDLE_LENGTH;
    NRF_LOG_INFO("Data len is set to 0x%X(%d)", m_ble_nus_max_data_len,
        m_ble_nus_max_data_len);
  }
  NRF_LOG_DEBUG("ATT MTU exchange completed. central 0x%x peripheral 0x%x",
      p_gatt->att_mtu_desired_central, p_gatt->att_mtu_desired_periph);
  UNUSED_PARAMETER(m_ble_nus_max_data_len);
}

/*!
 ****************************************************************************
 * @brief Function for initializing the GATT library.
 ******************************************************************************/
void gatt_init(void) {
  ret_code_t err_code;

  err_code = nrf_ble_gatt_init(&m_gatt, gatt_evt_handler);
  APP_ERROR_CHECK(err_code);

  err_code =
      nrf_ble_gatt_att_mtu_periph_set(&m_gatt, NRF_SDH_BLE_GATT_MAX_MTU_SIZE);
  APP_ERROR_CHECK(err_code);
}

#if 0
/*!
 ****************************************************************************
 * @brief Function for handling events from the BSP module.
 *
 * @param[in]   event   Event generated by button press.
 ******************************************************************************/
void bsp_event_handler(bsp_event_t event)
{
    uint32_t err_code;
    switch (event)
    {
        case BSP_EVENT_SLEEP:
            //sleep_mode_enter();
            break;

        case BSP_EVENT_DISCONNECT:
            err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            if (err_code != NRF_ERROR_INVALID_STATE)
            {
                APP_ERROR_CHECK(err_code);
            }
            break;

        case BSP_EVENT_WHITELIST_OFF:
            if (m_conn_handle == BLE_CONN_HANDLE_INVALID)
            {
                err_code = ble_advertising_restart_without_whitelist(&m_advertising);
                if (err_code != NRF_ERROR_INVALID_STATE)
                {
                    APP_ERROR_CHECK(err_code);
                }
            }
            break;

        default:
            break;
    }
}
#endif

#ifdef CUST4_SM
/*!
 ****************************************************************************
 * @brief Function to change the BLE advertising duration
 *
 * @details This function is to be used by user0 config app state machine
 *          framework to change the BLE advertising duration as put in user0
 *          config DCB. APIs turn_off_BLE() , turn_on_BLE() is to be used before
 *          and after calling this function.
 * @param[in]  new_adv_duration: New advertising duration to be changed to, in secs
 * @param[out] None
 ******************************************************************************/
void change_ble_adv_duration(uint32_t new_adv_duration)
{
  new_adv_duration *= 100; // Expressing in units of 10 milliseconds.
  m_advertising.adv_modes_config.ble_adv_fast_timeout = new_adv_duration;
}
#endif

/*!
 ****************************************************************************
 * @brief Function for initializing the Advertising functionality.
 ******************************************************************************/
static void advertising_init(void) {
  uint32_t err_code;
  ble_advertising_init_t init;

  memset(&init, 0, sizeof(init));

  init.advdata.name_type = BLE_ADVDATA_FULL_NAME;
  init.advdata.include_appearance = false;
  init.advdata.flags = BLE_GAP_ADV_FLAGS_LE_ONLY_LIMITED_DISC_MODE;

#ifdef BLE_CUSTOM_PROFILE1
  init.srdata.uuids_more_available.uuid_cnt =
       sizeof(m_adv_uuids_more) / sizeof(m_adv_uuids_more[0]);
  init.srdata.uuids_more_available.p_uuids = m_adv_uuids_more;
#else
  init.srdata.uuids_complete.uuid_cnt =
      sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
  init.srdata.uuids_complete.p_uuids = m_adv_uuids;
#endif

  init.config.ble_adv_fast_enabled = true;
  init.config.ble_adv_on_disconnect_disabled = false;
  init.config.ble_adv_fast_interval = APP_ADV_INTERVAL;
  init.config.ble_adv_fast_timeout = APP_ADV_DURATION;

  init.evt_handler = on_adv_evt;

  err_code = ble_advertising_init(&m_advertising, &init);
  APP_ERROR_CHECK(err_code);

  ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);

  /* To increase the TX power*/
  sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_ADV, 0, 4);
  /* To increase the TX power*/
  sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_CONN, 0, 4);
}

#ifdef BLE_THROUGHTPUT_DEBUG
static float throughput_kbps;

#define BLE_DATA_TX_INTERVAL                                                   \
  1000                         /* BLE Data transmitted interval 1000 (ms).  \  \
                                */
APP_TIMER_DEF(m_ble_timer_id); /* Handler for repeated timer for ble tx. */

/*!
 ****************************************************************************
 * @brief Function for handling the BLE DATA TX timer time-out.
 *
 * @details This function will be called each time the ble data transmit timer
 * expires.
 *
 ******************************************************************************/
static void ble_data_tx_timeout_handler(void *p_context) {
  if (m_conn_handle != BLE_CONN_HANDLE_INVALID) {
    uint32_t curr_bit_count;
    static uint32_t prev_bit_count = 0;
    uint32_t bit_count;

    curr_bit_count = ((g_ble_rx_byte_cnt + g_ble_tx_byte_cnt) *
                      8); // converting bytes to bits
    bit_count = curr_bit_count - prev_bit_count;
    throughput_kbps = ((bit_count / (BLE_DATA_TX_INTERVAL / 1000.f)) / 1000.f);
    throughput_kbps /= 8; // Converting to kilo bytes/sec

    NRF_LOG_INFO("BLE Data Rate in kB/s:" NRF_LOG_FLOAT_MARKER,
        NRF_LOG_FLOAT(throughput_kbps));

    prev_bit_count = curr_bit_count;
  }
}

/*!
 ****************************************************************************
 *@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module. This creates
 ******************************************************************************/
static void ble_timer_init(void) {
  // Create timers
  ret_code_t err_code;
  err_code = app_timer_create(
      &m_ble_timer_id, APP_TIMER_MODE_REPEATED, ble_data_tx_timeout_handler);

  APP_ERROR_CHECK(err_code);
}

/*!
 ****************************************************************************
 * @brief   Function for starting application timers.
 * @details Timers are run after the scheduler has started.
 ******************************************************************************/
static void ble_timer_start(void) {
  // Start repeated timer
  ret_code_t err_code = app_timer_start(
      m_ble_timer_id, APP_TIMER_TICKS(BLE_DATA_TX_INTERVAL), NULL);
  APP_ERROR_CHECK(err_code);
}

/*!
 ****************************************************************************
 * @brief   Function for stopping application timers.
 ******************************************************************************/
static void ble_timer_stop(void) {
  // Stop the repeated timer
  ret_code_t err_code = app_timer_stop(m_ble_timer_id);
  APP_ERROR_CHECK(err_code);
}
#endif


/** @brief   Get the BLE NUS service status
 * @details  Return the gb_ble_status variable value
 * @param    None
 * @retval   BLE_DISCONNECTED = 0x00 --> disconnected from Central
 *           BLE_CONNECTED = 0x01   -->  connected to Central
 *           BLE_PORT_OPENED = 0x02 -->  NUS start
 *           BLE_PORT_CLOSED = 0x03 -->  NUS stop
 */
uint8_t get_ble_nus_status() {
  return gb_ble_status;
}

/*!
 ****************************************************************************
 * @brief  Reset the msg queue of BLE NUS transmit task
 * @param  None
 * @return None
 ******************************************************************************/
void ble_tx_msg_queue_reset()
{
  xQueueReset( gh_ble_app_task_msg_queue );
}

#ifdef DEBUG_PKT
uint32_t present_ecg_packet_number=0,max_retry_sending_ecg=0;
uint32_t total_ecg_stream_recv=0,ecg_stream_packet=0;
#endif
#ifdef ADPD_SEM_CORRUPTION_DEBUG
 uint16_t ble_stop_resp_rx=0;
 uint16_t ble_packet_freed_count=0;
#endif
/*!
 ****************************************************************************
 * @brief  BLE NUS transmit task - handle post office messages to control ble tx
 * activities
 * @param  pArgument not used
 * @return None
 ******************************************************************************/
static void ble_tx_task(void *arg) {
  m2m2_hdr_t *p_in_pkt = NULL;
  uint16_t msg_len;
  uint32_t err_code;
  static uint8_t ble_data_array[BLE_NUS_MAX_DATA_LEN];
  /* To hold no: of retries to do when ble_nus_send fails with resource err */
  static volatile uint8_t tx_retry_cnt = 0;

  while (1) {
    adi_osal_SemPend(g_ble_tx_task_evt_sem, ADI_OSAL_TIMEOUT_FOREVER);
    p_in_pkt =post_office_get(ADI_OSAL_TIMEOUT_NONE, APP_OS_CFG_BLE_TASK_INDEX);
    if (gb_ble_status == BLE_PORT_OPENED) {
      if (p_in_pkt == NULL) {
        /* No m2m2 messages to process, so fetch some data from the device. */
        continue;
      } else {
        /* We got an m2m2 message from the queue, process it. */
        PYLD_CST(p_in_pkt, _m2m2_app_common_cmd_t,p_in_cmd); /* to extract the stream start/stop response pkt from PO
                        */
        tx_retry_cnt = 0;
        msg_len = p_in_pkt->length;
#ifdef DEBUG_PKT
        if(p_in_pkt->length == (sizeof(ecg_app_stream_t) + M2M2_HEADER_SZ) && p_in_pkt->src == (M2M2_ADDR_ENUM_t)(M2M2_ADDR_MED_ECG_STREAM))
        {
          total_ecg_stream_recv++;
          PYLD_CST(p_in_pkt, ecg_app_stream_t, p_payload_ptr);
          present_ecg_packet_number = p_payload_ptr->sequence_num;
          ecg_stream_packet=1;
        }
#endif
#ifdef BLE_NUS_PROFILE_TIME_ENABLED
        if ((p_in_cmd->command == (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_SENSOR_COMMON_CMD_STREAM_DATA))
           gBleTaskMsgStreamDataCnt++;
#endif

        if (msg_len > BLE_NUS_MAX_DATA_LEN) {
          NRF_LOG_INFO("Got BLE Tx pkt_len greater than %d from %x, skipping..",
              BLE_NUS_MAX_DATA_LEN, p_in_pkt->src);
          post_office_consume_msg(p_in_pkt);
          continue;
          // ASSERT(msg_len<=BLE_NUS_MAX_DATA_LEN);
        }

        /* For SQI application, pkt combining need not be handled for 'sub rsqi
           remove', since sqi application streams come only after ADPD is
           started
           And ADPD application is started only after SQI application is
           started, otherwise initial ADPD samples would be missed

           For Battery Streams, pkt combining need not be done */
        if (p_in_cmd->command == M2M2_APP_COMMON_CMD_STREAM_UNSUBSCRIBE_RESP)
        {
          /* to extract the stream address from unsub response pkt from PO */
          PYLD_CST(p_in_pkt, m2m2_app_common_sub_op_t,p_in_unsub_cmd);
          if( p_in_unsub_cmd->stream != M2M2_ADDR_MED_SQI_STREAM &&
              p_in_unsub_cmd->stream != M2M2_ADDR_SYS_BATT_STREAM) {
            NRF_LOG_INFO("BLE Tx: Got sensor sub remove resp from PO");
            if (sub_add_cnt > 0)
              /* Handled one stream SUB-UNSUB resp pair */
              sub_add_cnt--;
            /* No more subscription for streaming active */
            if (sub_add_cnt == 0) {
              tx_pkt_comb_stop = 1;
              NRF_LOG_INFO("BLE Tx: Stopping Pkt combining");
            }
          }
        }
        if (tx_pkt_comb_stop) {
          if (!(tx_pkt_comb_cnt % tx_pkt_compare_val)) {
            // tx_pkt_comb_cnt = 0;
            tx_pkt_comb_stop = 0;
            /* Stop pkt combining with min count, since data rate is going to
             * decrease henceforth */
            tx_pkt_compare_val = MIN_TX_PKT_COMB_CNT;
            NRF_LOG_INFO("BLE Tx: resetting pkt combining");
          }
        }

        p_in_pkt->src = (M2M2_ADDR_ENUM_t)BYTE_SWAP_16(p_in_pkt->src);
        p_in_pkt->dest = (M2M2_ADDR_ENUM_t)BYTE_SWAP_16(p_in_pkt->dest);
        p_in_pkt->length = BYTE_SWAP_16(p_in_pkt->length);
        p_in_pkt->checksum = BYTE_SWAP_16(p_in_pkt->checksum);

        if (msg_len + tx_pkt_len_comb > BLE_NUS_MAX_DATA_LEN)
          tx_pkt_send_now = 1;

        if (!tx_pkt_send_now) {
          memcpy(ble_data_array + tx_pkt_len_comb, p_in_pkt, msg_len);
          /* update the combined pkt len in ble_data_array */
          tx_pkt_len_comb += msg_len;
          /* update the count of pkts in ble_data_array */
          tx_pkt_comb_cnt++;
        }
#ifdef ADPD_SEM_CORRUPTION_DEBUG
        if(p_in_cmd->command == M2M2_APP_COMMON_CMD_STREAM_STOP_RESP){
          ble_stop_resp_rx += 1;
        }
#endif
        /* After copying the data pkt to ble_data_array, send it over ble using
           ble_nus_data_send() either if tx_pkt_comb_cnt has reached the
           tx_pkt_compare_val OR if its a RESP pkt to some other request other
           than M2M2_SENSOR_COMMON_CMD_STREAM_DATA which needs to be send out
           before it reaches the MAX_TX_PKT_COMB_CNT OR when tx_pkt_send_now
           flag is true, which happens total msg len in ble_data_array[] is
           going to exceed BLE_NUS_MAX_DATA_LEN, even before MAX_TX_PKT_COMB_CNT
           is reached or not; based on which msg needs to be sent
        */
        if ((p_in_cmd->command != (M2M2_APP_COMMON_CMD_ENUM_t)
                                      M2M2_SENSOR_COMMON_CMD_STREAM_DATA) ||
            (tx_pkt_comb_cnt == tx_pkt_compare_val) || tx_pkt_send_now) {
          // NRF_LOG_INFO("BLE Tx: Ready to send data over BLE
          // NUS.Len=%d",msg_len); NRF_LOG_HEXDUMP_INFO(ble_data_array,
          // msg_len);
          do {
#ifdef BLE_NUS_PROFILE_TIME_ENABLED
            nUSec = get_micro_sec();
#endif
            err_code = ble_nus_data_send(
                &m_nus, ble_data_array, &tx_pkt_len_comb, m_conn_handle);
#ifdef BLE_NUS_PROFILE_TIME_ENABLED
            ble_nus_send_time = get_micro_sec() - nUSec;
            max_ble_nus_send_time = (ble_nus_send_time > max_ble_nus_send_time)
                                        ? ble_nus_send_time
                                        : max_ble_nus_send_time;
            min_ble_nus_send_time = (ble_nus_send_time < min_ble_nus_send_time)
                                        ? ble_nus_send_time
                                        : min_ble_nus_send_time;
#endif

            if ((err_code != NRF_ERROR_INVALID_STATE) &&
                (err_code != NRF_ERROR_RESOURCES) &&
                (err_code != NRF_ERROR_NOT_FOUND)) {
              APP_ERROR_CHECK(err_code);
            }
            if (err_code == NRF_ERROR_RESOURCES) {
              gsErrResources = 1;
              // adi_osal_ThreadSleep(15);//15*40=600ms. to prevent sometime the
              // BLE signal unstable lead to data transmit fail.
              // adi_osal_SemPend(g_ble_nus_evt_sem, 1000);
              //                  nrf_pwr_mgmt_run();

            } else
              gTotalPktSubmit++;
            if (tx_retry_cnt++ == MAX_BLE_TX_RETRY_CNT ||
                err_code == NRF_ERROR_INVALID_STATE) {
              NRF_LOG_INFO("BLE Tx:retry count Max err_code:%d", err_code);
              /* No point in having BLE connection after BLE Tx fails, do BLE
                 Disconnect */
              err_code = sd_ble_gap_disconnect(
                  m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
              if (err_code != NRF_ERROR_INVALID_STATE) {
                APP_ERROR_CHECK(err_code);
              }

              break;
            }
          } while (err_code == NRF_ERROR_RESOURCES && gsErrResources);
#ifdef DEBUG_PKT
          if(ecg_stream_packet==1)
          {
             if(max_retry_sending_ecg<tx_retry_cnt)
                max_retry_sending_ecg = tx_retry_cnt;
             ecg_stream_packet=0;
          }
#endif
          //if (!gsErrResources)
          //  adi_osal_SemPend(g_ble_nus_evt_sem, ADI_OSAL_TIMEOUT_FOREVER);
          g_ble_tx_byte_cnt += tx_pkt_len_comb;
          /* reset */
          tx_pkt_len_comb = 0;
          /* reset */
          tx_pkt_comb_cnt = 0;
        } // end of if condition

        if (tx_pkt_send_now) {
          memcpy(ble_data_array + tx_pkt_len_comb, p_in_pkt, msg_len);
          /* update the combined pkt len in ble_data_array */
          tx_pkt_len_comb += msg_len;
          /* update the count of pkts in ble_data_array */
          tx_pkt_comb_cnt++;
          tx_pkt_send_now = 0;
        }

        /* For SQI application, pkt combining need not be handled for 'sub rsqi
           add', since sqi application streams come only after ADPD is started
           And ADPD application is started only after SQI application is
           started, otherwise initial ADPD samples would be missed

           For Battery Streams, pkt combining need not be done */
        if (p_in_cmd->command == M2M2_APP_COMMON_CMD_STREAM_SUBSCRIBE_RESP)
        {
          /* to extract the stream address from sub response pkt from PO */
          PYLD_CST(p_in_pkt, m2m2_app_common_sub_op_t,p_in_sub_cmd);
          if( p_in_sub_cmd->stream != M2M2_ADDR_MED_SQI_STREAM &&
              p_in_sub_cmd->stream != M2M2_ADDR_SYS_BATT_STREAM ) {
            NRF_LOG_INFO("BLE Tx: Got sensor sub add resp from PO");
            /* Start pkt combining with max count, since
             data rate is going to increase henceforth */
            //tx_pkt_compare_val = MAX_TX_PKT_COMB_CNT;
            tx_pkt_compare_val = gn_max_tx_kt_comb_cnt;
            sub_add_cnt++;
          }
        }

        /* Swap the header around so that the length can be properly freed. */
        p_in_pkt->length = msg_len;
        post_office_consume_msg(p_in_pkt);
#ifdef BLE_NUS_PROFILE_TIME_ENABLED
        gBleTaskMsgProcessCnt++;
#endif
#ifdef ADPD_SEM_CORRUPTION_DEBUG
  if(p_in_cmd->command == M2M2_APP_COMMON_CMD_STREAM_STOP_RESP){
        ble_packet_freed_count += 1;
        }
#endif
      }
    } // End of if condition for ble connection ON
    else if ((gb_ble_status == BLE_PORT_CLOSED) ||
             (gb_ble_status == BLE_DISCONNECTED)) {
      if (gb_ble_force_stream_stop) {
        ble_tx_msg_queue_reset();
        /* Force-stop Sensor streaming that wasn't stopped */
        m2m2_hdr_t *req_pkt = NULL;
        ADI_OSAL_STATUS err;

        req_pkt = post_office_create_msg(
            M2M2_HEADER_SZ + sizeof(m2m2_pm_force_stream_stop_cmd_t));
        if (req_pkt != NULL) {

          m2m2_pm_force_stream_stop_cmd_t *force_stream_stop_req =
              (m2m2_pm_force_stream_stop_cmd_t *)&req_pkt->data[0];

          /* send REQ packet */
          req_pkt->src = ble_pkt_src;
          req_pkt->dest = M2M2_ADDR_SYS_PM;

          force_stream_stop_req->command =
              M2M2_PM_SYS_COMMAND_FORCE_STREAM_STOP_REQ;
          NRF_LOG_INFO("Sending Force stream stop cmd from BLE");
          post_office_send(req_pkt, &err);
          ble_pkt_src = M2M2_ADDR_UNDEFINED;
          gb_ble_force_stream_stop = false;
        }
      }
      if(p_in_pkt != NULL)
          post_office_consume_msg(p_in_pkt);
    }
  }
}

/*!
 ****************************************************************************
 * @brief BLE Application task init function.
 ******************************************************************************/
void ble_application_task_init(void) {
  ADI_OSAL_STATUS eOsStatus;

  /* Create ble transmit thread */
  g_ble_app_task_attributes.pThreadFunc = ble_tx_task;
  g_ble_app_task_attributes.nPriority = APP_OS_CFG_BLE_TASK_PRIO;
  g_ble_app_task_attributes.pStackBase = &ga_ble_app_task_stack[0];
  g_ble_app_task_attributes.nStackSize = APP_OS_CFG_BLE_TASK_STK_SIZE;
  g_ble_app_task_attributes.pTaskAttrParam = NULL;
  g_ble_app_task_attributes.szThreadName = "ble_application_task";
  g_ble_app_task_attributes.pThreadTcb = &g_ble_app_task_tcb;
  eOsStatus = adi_osal_MsgQueueCreate(&gh_ble_app_task_msg_queue, NULL,
      60); /* Incease this value as the ble buffer. */
  if (eOsStatus != ADI_OSAL_SUCCESS) {
    Debug_Handler();
  } else {
    update_task_queue_list(
        APP_OS_CFG_BLE_TASK_INDEX, gh_ble_app_task_msg_queue);
  }

  eOsStatus = adi_osal_ThreadCreateStatic(
      &gh_ble_app_task_handler, &g_ble_app_task_attributes);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
    Debug_Handler();
  }

  /*eOsStatus = adi_osal_SemCreate(&g_ble_nus_evt_sem, 0U);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
    Debug_Handler();
  }*/
  eOsStatus = adi_osal_SemCreate(&g_ble_tx_task_evt_sem, 0U);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
    Debug_Handler();
  }

#ifdef BLE_THROUGHTPUT_DEBUG
  ble_timer_init();
#endif
#ifdef BLE_NUS_PROFILE_TIME_ENABLED
  us_timer_init();
#endif
  ble_stack_init();

  // Initialize timer module.
  ret_code_t err_code = app_timer_init();
  APP_ERROR_CHECK(err_code);

  gap_params_init();
  gatt_init();
  services_init();
  advertising_init();
  conn_params_init();
#ifdef BLE_THROUGHTPUT_DEBUG
  ble_timer_start();
#endif
#ifdef BLE_PEER_ENABLE
  peer_manager_init();
#endif
  //application_timers_start();

  ble_opt_t opt;

  memset(&opt, 0x00, sizeof(opt));
  opt.common_opt.conn_evt_ext.enable = 1;

  err_code = sd_ble_opt_set(BLE_COMMON_OPT_CONN_EVT_EXT, &opt);
  APP_ERROR_CHECK(err_code);
#ifdef BLE_PEER_ENABLE
  bool erase_bonds = false;
  // Create a FreeRTOS task for the BLE stack.
  // The task will run advertising_start() before entering its loop.
  nrf_sdh_freertos_init(advertising_start, &erase_bonds);
#else
  nrf_sdh_freertos_init(advertising_start, NULL);
#endif
  // Activate deep sleep mode.
  // SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

  /*for (;;)
  {
     APP_ERROR_HANDLER(NRF_ERROR_FORBIDDEN);
  }*/
}

/*!
 ****************************************************************************
 * @brief  Function to send PO pkts to BLE transmit task
 * @param  p_pkt m2m2 packet to be send to BLE Tx task
 * @return None
 ******************************************************************************/
void send_message_ble_tx_task(m2m2_hdr_t *p_pkt) {
  ADI_OSAL_STATUS osal_result;
  osal_result = adi_osal_MsgQueuePost(gh_ble_app_task_msg_queue, p_pkt);
  if (osal_result != ADI_OSAL_SUCCESS)
  {
    post_office_consume_msg(p_pkt);
#ifdef BLE_NUS_PROFILE_TIME_ENABLED
    gBleTaskMsgPostCntFailed++;
#endif
  }
#ifdef BLE_NUS_PROFILE_TIME_ENABLED
  else
    gBleTaskMsgPostCnt++;
#endif
  adi_osal_SemPost(g_ble_tx_task_evt_sem);
}

/*!
 ****************************************************************************
 * @brief Function to do softdevice BLE disconnect
 ******************************************************************************/
static void disconnect(uint16_t conn_handle, void *p_context) {
  UNUSED_PARAMETER(p_context);

  ret_code_t err_code = sd_ble_gap_disconnect(
      conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
  if (err_code != NRF_SUCCESS) {
    NRF_LOG_WARNING(
        "Failed to disconnect connection. Connection handle: %d Error: %d",
        conn_handle, err_code);
  } else {
    NRF_LOG_DEBUG("Disconnected connection handle %d", conn_handle);
  }
}

/*!
 ****************************************************************************
 * @brief Function to handle current BLE connection state, before exiting from
 * Watch application, to enter bootloader
 ******************************************************************************/
void ble_enter_bootloader_prepare(void) {
  /* Prevent device from advertising on disconnect. */
  ble_adv_modes_config_t config;
  memset(&config, 0, sizeof(ble_adv_modes_config_t));

  config.ble_adv_fast_enabled = true;
  config.ble_adv_fast_interval = APP_ADV_INTERVAL;
  config.ble_adv_fast_timeout = APP_ADV_DURATION;
  config.ble_adv_on_disconnect_disabled = true;
  ble_advertising_modes_config_set(&m_advertising, &config);

  /* Disconnect all other bonded devices that currently are connected.
     This is required to receive a service changed indication
     on bootup after a successful (or aborted) Device Firmware Update.
  */
  uint32_t conn_count = ble_conn_state_for_each_connected(disconnect, NULL);
  NRF_LOG_INFO("Disconnected %d links.", conn_count);
}

/*!
 ****************************************************************************
 * @brief Function to check and enter bootloader from Watch app, for DFU
 ******************************************************************************/
uint32_t enter_bootloader_and_restart(void) {
  uint32_t err_code;
  uint8_t bat_soc;
  /* if logging is in progress , close file */
  if(UpdateFileInfo() == true){
    NRF_LOG_INFO("Success file close ");
  }
  else {
    /* failure */
    NRF_LOG_INFO("Error file close");
  }
  Adp5360_getBatSoC(&bat_soc);
  if ((bat_soc < UPGRADE_BATTERY_LEVEL) &&
      (0 != Adp5360_pgood_pin_status_get())) {
#ifdef ENABLE_WATCH_DISPLAY
    send_global_type_value(DIS_VOLTAGE_LOW_ALARM);
#endif
    return NRF_ERROR_FORBIDDEN;
  }
  rtc_timestamp_store(320);
#ifdef BLE_THROUGHTPUT_DEBUG
  ble_timer_stop();
#endif
#ifdef USE_ADP5360_WDT
  Adp5350_wdt_set(ADP5360_DISABLE);
#endif
  NRF_LOG_DEBUG("In ble_dfu_buttonless_bootloader_start_finalize\r\n");
  ble_enter_bootloader_prepare();

  err_code = sd_power_gpregret_clr(0, 0xffffffff);
  VERIFY_SUCCESS(err_code);

  err_code = sd_power_gpregret_set(0, 1);
  VERIFY_SUCCESS(err_code);

  /* Signal that DFU mode is to be enter to the power management module */
  nrf_pwr_mgmt_shutdown(NRF_PWR_MGMT_SHUTDOWN_GOTO_DFU);

  return NRF_SUCCESS;
}

/*!
 ****************************************************************************
 * @brief Function to do disable softdevice
 ******************************************************************************/
void ble_disable_softdevice(void) {

  ret_code_t err_code;
  /* Clean up connection parameter negotiation before e.g., disabling SoftDevice
   * @see
   * https://infocenter.nordicsemi.com/topic/com.nordic.infocenter.sdk5.v15.3.0/group__ble__conn__params.html
   */

  /* Stop connection parameter update negotiation.*/
  err_code = ble_conn_params_stop();
  if (err_code != NRF_SUCCESS) {
    /* Procedure failed. Take recovery action.*/
    NRF_LOG_ERROR("Unable to stop conn param negotiation");
  }

  /* Disable SoftDevice */
  nrf_sdh_suspend();
  err_code = nrf_sdh_disable_request();
  APP_ERROR_CHECK(err_code);
}

/*!
 ****************************************************************************
 * @brief Function to do BLE disconnect and PM unbond
 ******************************************************************************/
void ble_disconnect_and_unbond(void) {
  ret_code_t err_code;

#ifdef BLE_THROUGHTPUT_DEBUG
  ble_timer_stop();
#endif
  /* 1. Disconnect */
  if (m_conn_handle != BLE_CONN_HANDLE_INVALID) {
    err_code = sd_ble_gap_disconnect(
        m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
    APP_ERROR_CHECK(err_code);
    //while (m_conn_handle != BLE_CONN_HANDLE_INVALID)
    //  ;
  } else {
    NRF_LOG_INFO("Attempting to disconnect while not connected.");
  }

  /* 2. Stop advertising (restart could happen but it's not handled in this
   function) */
  //    sd_ble_gap_adv_stop(m_advertising.adv_handle);

#ifdef BLE_PEER_ENABLE
  /* 3. Unbond */
  err_code = pm_peers_delete();
  APP_ERROR_CHECK(err_code);
#endif
}