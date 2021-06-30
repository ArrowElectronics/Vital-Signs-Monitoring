/**
***************************************************************************
* @addtogroup Tasks
* @{
* @file         ecg_application_task.c
* @author       ADI
* @version      V1.0.0
* @date         26-June-2019
* @brief        Source file contains ECG processing wrapper.
***************************************************************************
* @attention
***************************************************************************
*/
/*!
*  copyright Analog Devices
* ****************************************************************************
*
* License Agreement
*
* Copyright (c) 2020 Analog Devices Inc.
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
#include <includes.h>
#include <ecg_task.h>
#ifdef ENABLE_ECG_APP
#include <adpd4000_dcfg.h>
#include <power_manager.h>
#include "adi_ecg.h"
#include <rtc.h>
#include <stdint.h>
#include <string.h>
#include "rtc.h"
#ifdef VSM_MBOARD
#include "watch_vsm_motherboard_pin_config.h"
#elif PCBA
#include "watch_board_pcba_pin_config.h"
#else
#include "watch_board_evt_pin_config.h"
#endif
#include "nrf_drv_gpiote.h"
#include "us_tick.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#ifdef ECG_HR_ALGO
#include "rpeaks.h"
#endif
#ifdef BLE_CUSTOM_PROFILE1
#include "ble_cus1.h"
#endif
#include "lcd_driver.h"

/////////////////////////////////////////
#ifdef DCB
#include <adi_dcb_config.h>
#include <dcb_interface.h>
static volatile bool g_ecg_dcb_Present = false;
bool ecg_get_dcb_present_flag(void);
void ecg_set_dcb_present_flag(bool set_flag);
ECG_ERROR_CODE_t read_ecg_dcb(uint32_t *ecg_dcb_data, uint16_t *read_size);
ECG_ERROR_CODE_t write_ecg_dcb(uint32_t *ecg_dcb_data, uint16_t write_Size);
ECG_ERROR_CODE_t delete_ecg_dcb(void);
#endif
/////////////////////////////////////////
g_state_ecg_t g_state_ecg;

/* Buffer is reused in ECG,EDA,BCM applications to minmize RAM usage */
uint32_t AppBuff[APPBUFF_SIZE];
#ifdef EXTERNAL_TRIGGER_EDA	
uint8_t ecg_start_req=0;
#endif

#ifdef DEBUG_PKT
uint32_t g_ecg_pkt_cnt =0;
#endif
typedef m2m2_hdr_t *(app_cb_function_t)(m2m2_hdr_t *);

const char GIT_ECG_VERSION[] = "TEST ECG_VERSION STRING";
const uint8_t GIT_ECG_VERSION_LEN = sizeof(GIT_ECG_VERSION);
const char GIT_ECG_ALGO_VERSION[] = "ADI ECG HR;";
const uint8_t GIT_ECG_ALGO_VERSION_LEN = sizeof(GIT_ECG_ALGO_VERSION);
#ifdef BLE_CUSTOM_PROFILE1
static void packetize_ecg_raw_data_ble(
    int16_t *pEcgData, ecg_raw_sample_char_data_t *pPkt, uint32_t timestamp);
#else
static void packetize_ecg_raw_data(
    int16_t *p_data, ecg_app_stream_t *pPkt, uint32_t timestamp);
#endif
static m2m2_hdr_t *ecg_app_reg_access(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *ecg_app_stream_config(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *ecg_app_status(m2m2_hdr_t *p_pkt);
// static m2m2_hdr_t *ecg_app_load_cfg(m2m2_hdr_t *p_pkt);
// static m2m2_hdr_t *ecg_app_get_dcfg(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *ecg_app_get_version(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *ecg_app_decimation(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *ecg_app_get_algo_version(m2m2_hdr_t *p_pkt);
#ifdef DCB
static m2m2_hdr_t *ecg_app_set_dcb_lcfg(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *ecg_dcb_command_read_config(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *ecg_dcb_command_write_config(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *ecg_dcb_command_delete_config(m2m2_hdr_t *p_pkt);
#endif
static void fetch_ecg_data(void);
static void sensor_ecg_task(void *pArgument);
int32_t EcgInit(void);
static uint16_t gnEcgSequenceCount = 0;
extern void EcgFifoCallBack(void);
void Enable_ephyz_power(void);
static void InitCfg();
AD5940Err AppECGGetCfg(void *pCfg);

uint8_t sensor_ecg_task_stack[APP_OS_CFG_ECG_TASK_STK_SIZE];
ADI_OSAL_THREAD_HANDLE sensor_ecg_task_handler;
ADI_OSAL_STATIC_THREAD_ATTR sensor_ecg_task_attributes;
StaticTask_t ecgTaskTcb;
ADI_OSAL_SEM_HANDLE ecg_task_evt_sem;
AppECGCfg_Type AppECGCfg;
extern AD5950_APP_ENUM_t gnAd5940App;
uint8_t send_packets=0;
void send_key_value(uint8_t  k_value);
#if DEBUG_ECG
extern uint32_t time_stamps_deviated[50];
#endif
/*!
 * @brief:  The dictionary type for mapping addresses to callback handlers.
 */
typedef struct _app_routing_table_entry_t {
  uint8_t command;
  app_cb_function_t *cb_handler;
} app_routing_table_entry_t;

app_routing_table_entry_t ecg_app_routing_table[] = {
    {M2M2_APP_COMMON_CMD_STREAM_START_REQ, ecg_app_stream_config},
    {M2M2_APP_COMMON_CMD_STREAM_STOP_REQ, ecg_app_stream_config},
    {M2M2_APP_COMMON_CMD_STREAM_SUBSCRIBE_REQ, ecg_app_stream_config},
    {M2M2_APP_COMMON_CMD_STREAM_UNSUBSCRIBE_REQ, ecg_app_stream_config},
    {M2M2_APP_COMMON_CMD_SENSOR_STATUS_QUERY_REQ, ecg_app_status},
    {M2M2_APP_COMMON_CMD_READ_LCFG_REQ, ecg_app_reg_access},
    {M2M2_APP_COMMON_CMD_WRITE_LCFG_REQ, ecg_app_reg_access},
    {M2M2_SENSOR_COMMON_CMD_GET_STREAM_DEC_FACTOR_REQ, ecg_app_decimation},
    {M2M2_SENSOR_COMMON_CMD_SET_STREAM_DEC_FACTOR_REQ, ecg_app_decimation},
    {M2M2_APP_COMMON_CMD_GET_VERSION_REQ, ecg_app_get_version},
    {M2M2_ECG_APP_CMD_GET_ALGO_VENDOR_VERSION_REQ, ecg_app_get_algo_version},
#ifdef DCB
    {M2M2_APP_COMMON_CMD_SET_LCFG_REQ, ecg_app_set_dcb_lcfg},
    {M2M2_DCB_COMMAND_READ_CONFIG_REQ, ecg_dcb_command_read_config},
    {M2M2_DCB_COMMAND_WRITE_CONFIG_REQ, ecg_dcb_command_write_config},
    {M2M2_DCB_COMMAND_ERASE_CONFIG_REQ, ecg_dcb_command_delete_config},
#endif
};
#define ECG_APP_ROUTING_TBL_SZ                                                 \
  (sizeof(ecg_app_routing_table) / sizeof(ecg_app_routing_table[0]))

ADI_OSAL_QUEUE_HANDLE ecg_task_msg_queue = NULL;

/*!
 ****************************************************************************
 *@brief       Ecg Task initialization
 *@param       None
 *@return      None
 ******************************************************************************/
void ad5940_ecg_task_init(void) {
  /* Initialize app state */
  g_state_ecg.num_subs = 0;
  g_state_ecg.num_starts = 0;
  g_state_ecg.leads_on_samplecount = 0;
  g_state_ecg.leads_off_samplecount = 0;
  /* Default behaviour is to send every packet */
  g_state_ecg.decimation_factor = 1;
  g_state_ecg.decimation_nsamples = 0;
  g_state_ecg.data_pkt_seq_num = 0;

  ADI_OSAL_STATUS eOsStatus = ADI_OSAL_SUCCESS;
  sensor_ecg_task_attributes.pThreadFunc = sensor_ecg_task;
  sensor_ecg_task_attributes.nPriority = APP_OS_CFG_ECG_TASK_PRIO;
  sensor_ecg_task_attributes.pStackBase = &sensor_ecg_task_stack[0];
  sensor_ecg_task_attributes.nStackSize = APP_OS_CFG_ECG_TASK_STK_SIZE;
  sensor_ecg_task_attributes.pTaskAttrParam = NULL;
  sensor_ecg_task_attributes.szThreadName = "ECG Sensor";

  sensor_ecg_task_attributes.pThreadTcb = &ecgTaskTcb;
  eOsStatus = adi_osal_MsgQueueCreate(&ecg_task_msg_queue, NULL, 5);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
    Debug_Handler();
  } else {
    update_task_queue_list(APP_OS_CFG_ECG_TASK_INDEX, ecg_task_msg_queue);
  }
  eOsStatus = adi_osal_ThreadCreateStatic(
      &sensor_ecg_task_handler, &sensor_ecg_task_attributes);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
    Debug_Handler();
  }
  adi_osal_SemCreate(&ecg_task_evt_sem, 0);
}

/*!
 ****************************************************************************
 *@brief      Called by the post office to send messages to this application
 *@param      p_pkt: packet of type m2m2_hdr_t
 *@return     None
 ******************************************************************************/
void send_message_ad5940_ecg_task(m2m2_hdr_t *p_pkt) {
  ADI_OSAL_STATUS osal_result;
  osal_result = adi_osal_MsgQueuePost(ecg_task_msg_queue, p_pkt);
  if (osal_result != ADI_OSAL_SUCCESS)
    post_office_consume_msg(p_pkt);
  adi_osal_SemPost(ecg_task_evt_sem);
}

void ad5940_ecg_start(void);
void ad5940_fetch_data(void);
//#define ECG_POLLING
/*!
 ****************************************************************************
 *@brief      Ecg Task
 *@param      None
 *@return     None
 ******************************************************************************/
extern uint32_t gnAd5940TimeCurVal;
extern uint8_t gnAd5940TimeIrqSet, gnAd5940TimeIrqSet1;
extern volatile uint8_t gnAd5940DataReady;
static void sensor_ecg_task(void *pArgument) {
  m2m2_hdr_t *p_in_pkt = NULL;
  m2m2_hdr_t *p_out_pkt = NULL;
  ADI_OSAL_STATUS err;
  post_office_add_mailbox(M2M2_ADDR_MED_ECG, M2M2_ADDR_MED_ECG_STREAM);

#ifdef ECG_POLLING
  adp5360_enable_ldo(ECG_LDO,true);
  ad5940_ecg_start();
  ad5940_fetch_data();
#endif

  while (1) {
    adi_osal_SemPend(ecg_task_evt_sem, ADI_OSAL_TIMEOUT_FOREVER);
    p_in_pkt =  post_office_get(ADI_OSAL_TIMEOUT_NONE, APP_OS_CFG_ECG_TASK_INDEX);
    if (p_in_pkt == NULL) {
      /* No m2m2 messages to process, so fetch some data from the device */
      if(g_state_ecg.num_starts != 0) { /* Fetch data from device only if the device is in active state(initialized properly)*/
      /* Now there should be fifo data ; number of samples to read is being set in
        ecg app which FIFO threshold variable*/

          /* set ISR flags high */
          gnAd5940DataReady = 1;
          gnAd5940TimeIrqSet = 1;
          gnAd5940TimeIrqSet1 = 1;
          fetch_ecg_data();
        }
        else
        {
          /* false interrupt trigger */
        }
    } else {
      /* Got an m2m2 message from the queue, process it */
      PYLD_CST(p_in_pkt, _m2m2_app_common_cmd_t, p_in_cmd);
      /* Look up the appropriate function to call in the function table */
      for (int i = 0; i < ECG_APP_ROUTING_TBL_SZ; i++) {
        if (ecg_app_routing_table[i].command == p_in_cmd->command) {
          p_out_pkt = ecg_app_routing_table[i].cb_handler(p_in_pkt);
          break;
        }
      }
      post_office_consume_msg(p_in_pkt);
      if (p_out_pkt != NULL) {
        post_office_send(p_out_pkt, &err);
      }
    }
  }
}

/*!
 ****************************************************************************
 *@brief      Reset ecg packetization
 *@param      None
 *@return     None
 ******************************************************************************/
static void reset_ecg_packetization(void) {
  g_state_ecg.ecg_pktizer.packet_nsamples = 0;
  g_state_ecg.leads_on_samplecount = 0;
  g_state_ecg.leads_off_samplecount = 0;
}

#ifdef DEBUG_ECG
static uint32_t pkt_count = 0;
static float ecg_odr = 0, time_elapsed_for_ecg = 0, ecg_start_time = 0;
#endif

/*!
 ****************************************************************************
 *@brief      Fetch ecg data
 *@param      None
 *@return     None
 ******************************************************************************/
static void fetch_ecg_data(void) {
  ADI_OSAL_STATUS err;
  static uint32_t ecgData = 0;
  uint32_t ecgTS = 0;
  int8_t status = 0;
#ifdef BLE_CUSTOM_PROFILE1
  static ecg_raw_sample_char_data_t pkt_ble;
#else
  static ecg_app_stream_t pkt;
#endif

  g_state_ecg.ecg_pktizer.p_pkt = NULL;
#ifdef ECG_HR_ALGO
  uint16_t nEcgQRS = 0;
  AppECGCfg_Type *pCfg;
  AppECGGetCfg(&pCfg);
#endif

  M2M2_SENSOR_ECG_APP_INFO_BITSET_ENUM_t ecg_info = M2M2_SENSOR_ECG_APP_INFO_BITSET_LEADSOFF;
  ad5940_read_ecg_data_to_buffer();
  status = ad5950_buff_get(&ecgData, &ecgTS);
  while (status == AD5940Drv_SUCCESS) {
#ifdef ECG_HR_ALGO
    //Current ECG HR Alog supports only upto 200Hz
    if(pCfg->ECGODR <= 200 )
    {
      nEcgQRS = QRSdetector(ecgData,pCfg->ECGODR, 3);//ECG HR Algorithm
      if (nEcgQRS) {
        g_state_ecg.ecgHR = (pCfg->ECGODR * 60)/nEcgQRS;
      }
    }
    else
      g_state_ecg.ecgHR = 0; //Dummy HR
#endif
    g_state_ecg.decimation_nsamples++;
    if (g_state_ecg.decimation_nsamples >= g_state_ecg.decimation_factor) {
      g_state_ecg.decimation_nsamples = 0;
#ifdef BLE_CUSTOM_PROFILE1
      /* If the electrode status LOD GPIO = 1, ie not touched
          put ecg Data as INVALID */
      if (nrf_gpio_pin_read(AD8233_LOD_PIN)) {
        ecgData = 0xFFFF;//Leads Off Sample Value
      }
      packetize_ecg_raw_data_ble((int16_t *)&ecgData, &pkt_ble, ecgTS);
      if (g_state_ecg.ecg_pktizer.packet_nsamples >=
          g_state_ecg.ecg_pktizer.packet_max_nsamples) {
#else
        if(AppECGCfg.PacketizationEn) {
        /* set the electrode status = 0 not touched */
          if (nrf_gpio_pin_read(AD8233_LOD_PIN)) {
            send_packets = 0;
            ecg_info = M2M2_SENSOR_ECG_APP_INFO_BITSET_LEADSOFF;
            g_state_ecg.leads_on_samplecount = 0;
            if(g_state_ecg.leads_off_samplecount <= MAX_SAMPLES_LEADS_OFF) {
              g_state_ecg.leads_off_samplecount++;
            }
          }else {
            /* set the electrode status = 1 touched */
            send_packets = 1;
            ecg_info = M2M2_SENSOR_ECG_APP_INFO_BITSET_LEADSON;
            if(g_state_ecg.leads_on_samplecount <= MAX_SAMPLES_LEADS_ON) {
              if(g_state_ecg.leads_on_samplecount == 0) {
                g_state_ecg.ecg_pktizer.packet_nsamples = 0;// when leadson starts after leadsoff,the packetization has to start from first sample
              }else if(g_state_ecg.leads_on_samplecount == MAX_SAMPLES_LEADS_ON) {
                g_state_ecg.leads_off_samplecount = 0;// Clear leadsoff sample count when actual leadson happened
              }
              g_state_ecg.leads_on_samplecount++;
            }
          }
        }else {
          g_state_ecg.leads_on_samplecount = MAX_SAMPLES_LEADS_ON;// For No Leadson/off case ,send packet always.
          send_packets = 1;
        }
        if(((send_packets == 1) && (g_state_ecg.leads_on_samplecount >= MAX_SAMPLES_LEADS_ON)) 
            || ((g_state_ecg.leads_off_samplecount <= MAX_SAMPLES_LEADS_OFF) && (send_packets == 0))) {
          packetize_ecg_raw_data((int16_t *)&ecgData, &pkt, ecgTS);
          if (g_state_ecg.ecg_pktizer.packet_nsamples >
            g_state_ecg.ecg_pktizer.packet_max_nsamples) {
#endif
            adi_osal_EnterCriticalRegion();
#ifdef BLE_CUSTOM_PROFILE1
        //Create memory for m2m2 header + ecg_raw_sample_char_data_t + uint16_t HR
        g_state_ecg.ecg_pktizer.p_pkt = post_office_create_msg(sizeof(ecg_raw_sample_char_data_t) + sizeof(uint16_t) + M2M2_HEADER_SZ);
#else
        g_state_ecg.ecg_pktizer.p_pkt = post_office_create_msg(sizeof(ecg_app_stream_t) + M2M2_HEADER_SZ);
#endif
        if (g_state_ecg.ecg_pktizer.p_pkt != NULL) {
#ifdef DEBUG_PKT
            g_ecg_pkt_cnt++;
#endif
#ifdef BLE_CUSTOM_PROFILE1
          PYLD_CST(g_state_ecg.ecg_pktizer.p_pkt, ecg_raw_sample_char_data_t, p_payload_ptr_ble);
#else
          PYLD_CST(g_state_ecg.ecg_pktizer.p_pkt, ecg_app_stream_t, p_payload_ptr);
#endif
          g_state_ecg.ecg_pktizer.p_pkt->src = M2M2_ADDR_MED_ECG;
          g_state_ecg.ecg_pktizer.p_pkt->dest = M2M2_ADDR_MED_ECG_STREAM;
#ifdef BLE_CUSTOM_PROFILE1
          memcpy(p_payload_ptr_ble, &pkt_ble, sizeof(ecg_raw_sample_char_data_t));
          p_payload_ptr_ble->seq_number = gnEcgSequenceCount++;

          //Extract address from the PO pkt, to copy the ECG HR
          uint16_t *ptr = &(p_payload_ptr_ble->ecg_samp_data[ECG_SAMPLE_COUNT]);
          ptr += 1; //Advance to the interested mem location to copy ECG HR
          *ptr = g_state_ecg.ecgHR;
          //NRF_LOG_INFO("ECG HR:%d",g_state_ecg.ecgHR);
          //NRF_LOG_HEXDUMP_INFO(&pkt_ble,sizeof(ecg_raw_sample_char_data_t));
#else
          memcpy(&p_payload_ptr->command, &pkt, sizeof(ecg_app_stream_t));
          p_payload_ptr->command = M2M2_SENSOR_COMMON_CMD_STREAM_DATA;
          p_payload_ptr->status = 0x00;
          p_payload_ptr->datatype = M2M2_SENSOR_ECG_SPORT;
          p_payload_ptr->ecg_info = ecg_info;
#ifdef ECG_HR_ALGO
          p_payload_ptr->HR = g_state_ecg.ecgHR;
#else
          /* Dummy HR , Need to include ECG ALGO */
          p_payload_ptr->HR = 0;
#endif //ECG_HR_ALGO
          p_payload_ptr->sequence_num = gnEcgSequenceCount++;

#endif//BLE_CUSTOM_PROFILE1

#ifdef DEBUG_PKT
          post_office_msg_cnt(g_state_ecg.ecg_pktizer.p_pkt);
#endif
          post_office_send(g_state_ecg.ecg_pktizer.p_pkt, &err);
#ifdef DEBUG_ECG
          pkt_count++;
          time_elapsed_for_ecg = MCU_HAL_GetTick() - ecg_start_time;
          ecg_odr = ((pkt_count * 11 * 1000) / time_elapsed_for_ecg);
          NRF_LOG_INFO("TIME ELAPSED = " NRF_LOG_FLOAT_MARKER,
          NRF_LOG_FLOAT(time_elapsed_for_ecg));
          NRF_LOG_INFO("START TIME = " NRF_LOG_FLOAT_MARKER,
          NRF_LOG_FLOAT(ecg_start_time));
          NRF_LOG_INFO("ECG MEASURED ODR = " NRF_LOG_FLOAT_MARKER,
          NRF_LOG_FLOAT(ecg_odr));
#endif
          g_state_ecg.ecg_pktizer.packet_nsamples = 0;
          g_state_ecg.ecg_pktizer.packet_max_nsamples = 0;
          g_state_ecg.ecg_pktizer.p_pkt = NULL;
          } //if (g_state_ecg.ecg_pktizer.p_pkt != NULL)
	adi_osal_ExitCriticalRegion(); /* exiting critical region even if
                                          mem_alloc fails*/
#ifndef BLE_CUSTOM_PROFILE1
      } //if(send_packets == 1)
#endif
     }
    } /* if (g_state_ecg.decimation_nsamples >= g_state_ecg.decimation_factor)
       */
    status = ad5950_buff_get(&ecgData, &ecgTS);
  } /* while */
}

/*!
 ****************************************************************************
 *@brief      Packetize ecg data
 *@param      pEcgData: pointer to the ecg data
 *@param      pPkt: pointer to the packet structure
 *@param      timestamp: timestamp of the packet
 *@return     None
 ******************************************************************************/
#ifdef BLE_CUSTOM_PROFILE1
static void packetize_ecg_raw_data_ble(
    int16_t *pEcgData, ecg_raw_sample_char_data_t *pPkt, uint32_t timestamp) {
  if (g_state_ecg.ecg_pktizer.packet_nsamples == 0) {
    g_state_ecg.ecg_pktizer.packet_max_nsamples = (sizeof(pPkt->ecg_samp_data) / sizeof(pPkt->ecg_samp_data[0]));
    pPkt->version = 1;
    /* Converting TS in ticks to ms resolution */
    timestamp = timestamp >> 5;
    //NRF_LOG_INFO("TS:%d",timestamp);
    /* Converting to Big Endian as required */
    pPkt->rel_timestamp = BYTE_SWAP_32(timestamp);
    uint16_t temp = *pEcgData;
    /* Converting to Big Endian as required */
    temp = BYTE_SWAP_16(temp);
    pPkt->ecg_samp_data[0] = temp;
    g_state_ecg.ecg_pktizer.packet_nsamples++;
  } else if (g_state_ecg.ecg_pktizer.packet_nsamples <= g_state_ecg.ecg_pktizer.packet_max_nsamples-1) {
    /* one packet = ECG_SAMPLE_COUNT = 25 samples
     * all samples are added here */
    uint16_t i = g_state_ecg.ecg_pktizer.packet_nsamples;
    uint16_t temp = *pEcgData;
    /* Converting to Big Endian as required */
    temp = BYTE_SWAP_16(temp);
    pPkt->ecg_samp_data[i] = temp;
    g_state_ecg.ecg_pktizer.packet_nsamples++;
  }
  g_state_ecg.ecg_pktizer.prev_ts = timestamp;
}
#endif
static void packetize_ecg_raw_data(
    int16_t *pEcgData, ecg_app_stream_t *pPkt, uint32_t timestamp) {
  if (g_state_ecg.ecg_pktizer.packet_nsamples == 0) {
    g_state_ecg.ecg_pktizer.packet_max_nsamples = (sizeof(pPkt->ecg_data) / sizeof(pPkt->ecg_data[0]));
    pPkt->timestamp = timestamp;
    pPkt->firstecgdata = *pEcgData;
    //pPkt->HR = nEcgHR;
    g_state_ecg.ecg_pktizer.packet_nsamples++;
  } else if (g_state_ecg.ecg_pktizer.packet_nsamples <= g_state_ecg.ecg_pktizer.packet_max_nsamples) {
    /* one packet = 11 samples
     * first sample is added in abovee check
     * remaining 10 samples is added here */
    uint16_t i = g_state_ecg.ecg_pktizer.packet_nsamples - 1;
    if (timestamp < g_state_ecg.ecg_pktizer
                        .prev_ts) { /* handle day roll-over after 24hrs */
      /* MAX_RTC_TICKS_FOR_24_HOUR:- Max RTC Count value returned by
       * get_sensor_timestamp() after 24hrs. Adding that value to have
       * correct TS value during day roll-over */
      pPkt->ecg_data[i].timestamp = MAX_RTC_TICKS_FOR_24_HOUR + timestamp -
                                    g_state_ecg.ecg_pktizer.prev_ts;
    } else {
      pPkt->ecg_data[i].timestamp = timestamp - g_state_ecg.ecg_pktizer.prev_ts;
    }
    pPkt->ecg_data[i].ecgdata = *pEcgData;
    g_state_ecg.ecg_pktizer.packet_nsamples++;
  }
  g_state_ecg.ecg_pktizer.prev_ts = timestamp;
}
/*!
 ****************************************************************************
 *@brief      Get ecg application version
 *@param      pPkt: pointer to the packet structure
 *@return     m2m2_hdr_t
 ******************************************************************************/
static m2m2_hdr_t *ecg_app_get_version(m2m2_hdr_t *p_pkt) {
  PKT_MALLOC(p_resp_pkt, m2m2_app_common_version_t, 0);
  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_app_common_version_t, p_resp_payload);

    p_resp_payload->command = M2M2_APP_COMMON_CMD_GET_VERSION_RESP;
    p_resp_payload->major = 0x03;
    p_resp_payload->minor = 0x04;
    p_resp_payload->patch = 0x03;
    memcpy(&p_resp_payload->verstr[0], "ECG_App", 8);
    memcpy(&p_resp_payload->str[0], &GIT_ECG_VERSION, GIT_ECG_VERSION_LEN);
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  }
  return p_resp_pkt;
}

/*!
 ****************************************************************************
 *@brief      Get ecg algorithm version (currently not included)
 *@param      pPkt: pointer to the packet structure
 *@return     m2m2_hdr_t
 ******************************************************************************/
static m2m2_hdr_t *ecg_app_get_algo_version(m2m2_hdr_t *p_pkt) {
  PKT_MALLOC(p_resp_pkt, m2m2_app_common_version_t, 0);
  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_app_common_version_t, p_resp_payload);

    p_resp_payload->command = M2M2_ECG_APP_CMD_GET_ALGO_VENDOR_VERSION_RESP;
    p_resp_payload->major = 0x03;
    p_resp_payload->minor = 0x04;
    p_resp_payload->patch = 0x03;
    memcpy(&p_resp_payload->verstr[0], "-HC", 4);
    memcpy(&p_resp_payload->str[0], &GIT_ECG_ALGO_VERSION,
        GIT_ECG_ALGO_VERSION_LEN);
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  }
  return p_resp_pkt;
}

/*!
 ****************************************************************************
 *@brief      Load ecg application configuration (currently not included)
 *@param      pPkt: pointer to the packet structure
 *@return     m2m2_hdr_t
 ******************************************************************************/
/*
static m2m2_hdr_t *ecg_app_load_cfg(m2m2_hdr_t *p_pkt) {
  PYLD_CST(p_pkt, m2m2_sensor_ecg_resp_t, p_in_payload);
  PKT_MALLOC(p_resp_pkt, m2m2_sensor_ecg_resp_t, 0);
  PYLD_CST(p_resp_pkt, m2m2_sensor_ecg_resp_t, p_resp_payload);
  p_resp_payload->status = M2M2_APP_COMMON_STATUS_ERROR;
  uint16_t  device_id = p_in_payload->deviceid;

  if (g_state_ecg.num_starts == 0) {
    if (!load_ecg_dcfg(device_id)) {
      p_resp_payload->status = M2M2_APP_COMMON_STATUS_OK;
    } else {
      p_resp_payload->status = M2M2_APP_COMMON_STATUS_ERROR;
    }
  }
  p_resp_payload->command = M2M2_SENSOR_ECG_COMMAND_LOAD_CFG_RESP;
  p_resp_pkt->src = p_pkt->dest;
  p_resp_pkt->dest = p_pkt->src;
  return p_resp_pkt;
}*/

/*!
 ****************************************************************************
 *@brief      Get ecg application status (subscribers and start requests)
 *@param      pPkt: pointer to the packet structure
 *@return     m2m2_hdr_t
 ******************************************************************************/
static m2m2_hdr_t *ecg_app_status(m2m2_hdr_t *p_pkt) {
  PKT_MALLOC(p_resp_pkt, m2m2_app_common_status_t, 0);
  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_app_common_status_t, p_resp_payload);

    p_resp_payload->command = M2M2_APP_COMMON_CMD_SENSOR_STATUS_QUERY_RESP;
    p_resp_payload->status = M2M2_APP_COMMON_STATUS_ERROR;

    if (g_state_ecg.num_starts == 0) {
      p_resp_payload->status = M2M2_APP_COMMON_STATUS_STREAM_STOPPED;
    } else {
      p_resp_payload->status = M2M2_APP_COMMON_STATUS_STREAM_STARTED;
    }
    p_resp_payload->stream = M2M2_ADDR_MED_ECG_STREAM;
    p_resp_payload->num_subscribers = g_state_ecg.num_subs;
    p_resp_payload->num_start_reqs = g_state_ecg.num_starts;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  }
  return p_resp_pkt;
}

/*!
 ****************************************************************************
 *@brief      Ecg stream START/STOP/SUBSCRIBE/UNSUBSCRIBE options
 *@param      pPkt: pointer to the packet structure
 *@return     m2m2_hdr_t
 ******************************************************************************/
static m2m2_hdr_t *ecg_app_stream_config(m2m2_hdr_t *p_pkt) {
  AppECGCfg_Type *pCfg;
  AppECGGetCfg(&pCfg);
  M2M2_APP_COMMON_STATUS_ENUM_t status = M2M2_APP_COMMON_STATUS_ERROR;
  M2M2_APP_COMMON_CMD_ENUM_t command;
  PYLD_CST(p_pkt, m2m2_app_common_sub_op_t, p_in_payload);
  PKT_MALLOC(p_resp_pkt, m2m2_app_common_sub_op_t, 0);
  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_app_common_sub_op_t, p_resp_payload);
    switch (p_in_payload->command) {
    case M2M2_APP_COMMON_CMD_STREAM_START_REQ:
      if (g_state_ecg.num_starts == 0) {
        gnAd5940App = AD5940_APP_ECG;
#ifdef ECG_HR_ALGO
        QRSdetectorInit();
#endif
#ifdef EXTERNAL_TRIGGER_EDA	
        /* flag is used to indicate ecg start req has been recieved */
        ecg_start_req=1;
#endif
        reset_ecg_packetization();
        //lcd_disp_off(); //Turn Off LCD Display to check ECG signal quality
        if (EcgAppInit() == ECG_SUCCESS) {
          g_state_ecg.num_starts = 1;
#ifdef DEBUG_ECG
          ecg_start_time = MCU_HAL_GetTick();
          pkt_count = 0;
#endif
          status = M2M2_APP_COMMON_STATUS_STREAM_STARTED;
        } else {
          status = M2M2_APP_COMMON_STATUS_STREAM_NOT_STARTED;
        }
      } else {
        g_state_ecg.num_starts++;
        status = M2M2_APP_COMMON_STATUS_STREAM_IN_PROGRESS;
      }
      command = M2M2_APP_COMMON_CMD_STREAM_START_RESP;
      break;
    case M2M2_APP_COMMON_CMD_STREAM_STOP_REQ:
      if (g_state_ecg.num_starts == 0) {
        status = M2M2_APP_COMMON_STATUS_STREAM_STOPPED;
      } else if (g_state_ecg.num_starts == 1) {
#ifdef EXTERNAL_TRIGGER_EDA	
        /* flag is used to indicate ecg stop req has been recieved */
         ecg_start_req=0;
#endif
        //lcd_disp_on(); //Turn ON LCD Display, once ECG is stopped
        //send_key_value(0xFF); //Send dummy key_value to update the current page
        if (EcgAppDeInit()) {
          g_state_ecg.num_starts = 0;
          status = M2M2_APP_COMMON_STATUS_STREAM_STOPPED;
        } else {
          g_state_ecg.num_starts = 1;
          status = M2M2_APP_COMMON_STATUS_ERROR;
        }
      } else {
        g_state_ecg.num_starts--;
        status = M2M2_APP_COMMON_STATUS_STREAM_COUNT_DECREMENT;
      }
#if DEBUG_ECG
      NRF_LOG_INFO("\n *** printing deviated timer ts ****\n");
      for(int i=0;i<50;i++){
        NRF_LOG_INFO("%d",time_stamps_deviated[i]);
      }
#endif
      command = M2M2_APP_COMMON_CMD_STREAM_STOP_RESP;
      break;
    case M2M2_APP_COMMON_CMD_STREAM_SUBSCRIBE_REQ:
      g_state_ecg.num_subs++;
      post_office_setup_subscriber(
          M2M2_ADDR_MED_ECG, M2M2_ADDR_MED_ECG_STREAM, p_pkt->src, true);
      status = M2M2_APP_COMMON_STATUS_SUBSCRIBER_ADDED;
      command = M2M2_APP_COMMON_CMD_STREAM_SUBSCRIBE_RESP;
      break;
    case M2M2_APP_COMMON_CMD_STREAM_UNSUBSCRIBE_REQ:
      if (g_state_ecg.num_subs <= 1) {
        g_state_ecg.num_subs = 0;
        status = M2M2_APP_COMMON_STATUS_SUBSCRIBER_REMOVED;
      } else {
        g_state_ecg.num_subs--;
        status = M2M2_APP_COMMON_STATUS_SUBSCRIBER_COUNT_DECREMENT;
      }
      post_office_setup_subscriber(
          M2M2_ADDR_MED_ECG, M2M2_ADDR_MED_ECG_STREAM, p_pkt->src, false);
      command = M2M2_APP_COMMON_CMD_STREAM_UNSUBSCRIBE_RESP;
      break;
    default:
      /* Something has gone horribly wrong */
      post_office_consume_msg(p_resp_pkt);
      return NULL;
    }
    p_resp_payload->command = command;
    p_resp_payload->status = status;
    p_resp_payload->stream = p_in_payload->stream;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  } // if(NULL != p_resp_pkt)
  return p_resp_pkt;
}

/*!
 ****************************************************************************
 *@brief      Ecg stream Read/write configuration options
 *@param      pPkt: pointer to the packet structure
 *@return     m2m2_hdr_t
 ******************************************************************************/
static m2m2_hdr_t *ecg_app_reg_access(m2m2_hdr_t *p_pkt) {
  M2M2_APP_COMMON_STATUS_ENUM_t status = M2M2_APP_COMMON_STATUS_ERROR;
  PYLD_CST(p_pkt, ecg_app_lcfg_op_hdr_t, p_in_payload);
  /* Allocate a response packet with space for the correct number of operations
   */
  PKT_MALLOC(p_resp_pkt, ecg_app_lcfg_op_hdr_t,
      p_in_payload->num_ops * sizeof(p_in_payload->ops[0]));
  if (NULL != p_resp_pkt) {
    PYLD_CST(p_resp_pkt, ecg_app_lcfg_op_hdr_t, p_resp_payload);
    uint16_t reg_data = 0;

    switch (p_in_payload->command) {
    case M2M2_APP_COMMON_CMD_READ_LCFG_REQ:
      for (int i = 0; i < p_in_payload->num_ops; i++) {
        if (EcgReadLCFG(p_in_payload->ops[i].field, &reg_data) == ECG_SUCCESS) {
          status = M2M2_APP_COMMON_STATUS_OK;
        } else {
          status = M2M2_APP_COMMON_STATUS_ERROR;
        }
        p_resp_payload->ops[i].field = p_in_payload->ops[i].field;
        p_resp_payload->ops[i].value = reg_data;
      }
      p_resp_payload->command =
          (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_APP_COMMON_CMD_READ_LCFG_RESP;
      break;
    case M2M2_APP_COMMON_CMD_WRITE_LCFG_REQ:
      for (int i = 0; i < p_in_payload->num_ops; i++) {
        if (EcgWriteLCFG(p_in_payload->ops[i].field,
                p_in_payload->ops[i].value) == ECG_SUCCESS) {
          status = M2M2_APP_COMMON_STATUS_OK;
        } else {
          status = M2M2_APP_COMMON_STATUS_ERROR;
        }
        p_resp_payload->ops[i].field = p_in_payload->ops[i].field;
        p_resp_payload->ops[i].value = p_in_payload->ops[i].value;
      }
      p_resp_payload->command =
          (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_APP_COMMON_CMD_WRITE_LCFG_RESP;
      break;
    default:
      /* Something has gone horribly wrong. */
      post_office_consume_msg(p_resp_pkt);
      return NULL;
    }
    p_resp_pkt->dest = p_pkt->src;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_payload->num_ops = p_in_payload->num_ops;
    p_resp_payload->status = status;
  }
  return p_resp_pkt;
}

/*!
 ****************************************************************************
 *@brief      Ecg stream set decimation factor
 *@param      pPkt: pointer to the packet structure
 *@return     m2m2_hdr_t
 ******************************************************************************/
static m2m2_hdr_t *ecg_app_decimation(m2m2_hdr_t *p_pkt) {
  M2M2_APP_COMMON_STATUS_ENUM_t status = M2M2_APP_COMMON_STATUS_OK;
  PYLD_CST(p_pkt, m2m2_sensor_common_decimate_stream_t, p_in_payload);
  PKT_MALLOC(p_resp_pkt, m2m2_sensor_common_decimate_stream_t, 0);
  if (NULL != p_resp_pkt) {
    PYLD_CST(p_resp_pkt, m2m2_sensor_common_decimate_stream_t, p_resp_payload);

    switch (p_in_payload->command) {
    case M2M2_SENSOR_COMMON_CMD_GET_STREAM_DEC_FACTOR_REQ:
      p_resp_payload->command =
          M2M2_SENSOR_COMMON_CMD_GET_STREAM_DEC_FACTOR_RESP;
      break;
    case M2M2_SENSOR_COMMON_CMD_SET_STREAM_DEC_FACTOR_REQ:
      g_state_ecg.decimation_factor = p_in_payload->dec_factor;
      if (g_state_ecg.decimation_factor == 0) {
        g_state_ecg.decimation_factor = 1;
      }
      p_resp_payload->command =
          M2M2_SENSOR_COMMON_CMD_SET_STREAM_DEC_FACTOR_RESP;
      break;
    }
    p_resp_payload->status = status;
    p_resp_payload->dec_factor = g_state_ecg.decimation_factor;
    p_resp_payload->stream = p_in_payload->stream;
    p_resp_pkt->dest = p_pkt->src;
    p_resp_pkt->src = p_pkt->dest;
  }
  return p_resp_pkt;
}

/*****************************************************************************************************************************************************/


/*
  Application configuration structure. Specified by user from template.
  The variables are usable in this whole application.
  It includes basic configuration for sequencer generator
  and application related parameters
*/
volatile int16_t ecg_user_applied_odr = 0;
volatile int16_t ecg_user_applied_adcPgaGain = 0;
volatile int16_t user_applied_ecg_pwr_mod = 0;
volatile int16_t user_applied_packetization_enable = 0;

/*!
 ****************************************************************************
 *@brief      Initialize configurations for ecg application
 *@param      None
 *@return     None
 ******************************************************************************/
static void InitCfg() {
  AppECGCfg.bParaChanged = bFALSE;
  AppECGCfg.bBioElecBoard = bTRUE;
  AppECGCfg.SeqStartAddr = 0;
  AppECGCfg.MaxSeqLen = 512;
  AppECGCfg.SeqStartAddrCal = 0;
  AppECGCfg.MaxSeqLenCal = 512;
  AppECGCfg.NumOfData = -1;
  AppECGCfg.FifoThresh = 2;
  AppECGCfg.LfoscClkFreq = 32000.0;
  AppECGCfg.SysClkFreq = 16000000.0;
  AppECGCfg.AdcClkFreq = 16000000.0;

  if (!user_applied_ecg_pwr_mod) {
    AppECGCfg.PwrMod = AFEPWR_LP;
  } else {
    user_applied_ecg_pwr_mod = 0;
  }

  if (!user_applied_packetization_enable) {
    AppECGCfg.PacketizationEn = 1;/* enabled by default */
  } else {
    user_applied_packetization_enable = 0;
  }

  AppECGCfg.ADCSinc3Osr = ADCSINC3OSR_2;
  AppECGCfg.ADCSinc2Osr = ADCSINC2OSR_667;
  AppECGCfg.ECGInited = bFALSE;
  AppECGCfg.StopRequired = bFALSE;
  AppECGCfg.bRunning = bFALSE;
  AppECGCfg.FifoDataCount = 0;
}


/*!
 ****************************************************************************
 * @brief  Function to enable/disable Sport mode
 * @param  en: 1/0 to enable/disable switch
 * @return None
 *****************************************************************************/

void HAL_GPIO_ECG_Sport_Enable(uint8_t en) {
 uint32_t nRegVal = 0;
  if (0 != en) {
    /* Gp0-->INT0 Gp1-->SLEEPDEEP Gp2-->SYNC */
    nRegVal = AD5940_ReadReg(REG_AGPIO_GP0CON);
    nRegVal |= (GP5_GPIO) | (GP4_GPIO) | (GP3_GPIO);
    AD5940_WriteReg(REG_AGPIO_GP0CON, nRegVal);
    nRegVal = AD5940_ReadReg(REG_AGPIO_GP0CON);
    nRegVal |= (AGPIO_Pin5) | (AGPIO_Pin4) | (AGPIO_Pin3);
    /* Gp0/1/2 Output enable */
    AD5940_WriteReg(REG_AGPIO_GP0OEN,nRegVal);
    AD5940_WriteReg(REG_AGPIO_GP0SET, AGPIO_Pin4);
    /* Enable Fast restore operation from AD8233 */
    AD5940_WriteReg(REG_AGPIO_GP0SET, AGPIO_Pin5);
    AD5940_WriteReg(REG_AGPIO_GP0CLR, AGPIO_Pin3);
    nrf_gpio_cfg_input(AD8233_LOD_PIN, NRF_GPIO_PIN_NOPULL);
  } else {
    /* Gp0 Output enable */
    AD5940_WriteReg(REG_AGPIO_GP0OEN, (AGPIO_Pin4));
    /* 8233_SW_EN_1V8 */
    AD5940_WriteReg(REG_AGPIO_GP0CLR, AGPIO_Pin4);
  }
}
#endif

/*!
 ****************************************************************************
 * @brief  Function to control the GPIO, which enables/disables
 *          the DG2502 switch that enables AD5940 - eda application
 * @param  sw_enable: 1/0 to enable/disable switch
 * @return None
 *****************************************************************************/
void DG2502_SW_control_AD5940(uint8_t sw_enable) {
  if (sw_enable) {
    /* Gp0 Output enable */
    AD5940_WriteReg(REG_AGPIO_GP0OEN, (AGPIO_Pin3));
    /* 5940_SW_EN_1V8 */
    AD5940_WriteReg(REG_AGPIO_GP0SET, AGPIO_Pin3);
  } else {
    /* Gp0 Output enable */
    AD5940_WriteReg(REG_AGPIO_GP0OEN, (AGPIO_Pin3));
    /* 5940_SW_EN_1V8 */
    AD5940_WriteReg(REG_AGPIO_GP0CLR, AGPIO_Pin3);
  }
}

#ifdef ENABLE_ECG_APP
/*!
 ****************************************************************************
 * @brief  Function to control the GPIO, which enables/disables
 *          the DG2502 switch that connects ECG electrodes
 *          to AD5940 - eda application
 * @param  sw_enable: 1/0 to enable/disable switch
 * @return None
 *****************************************************************************/
void DG2502_SW_control_AD8233(uint8_t sw_enable) {
  if (sw_enable) {
    /* Gp0 Output enable */
    AD5940_WriteReg(REG_AGPIO_GP0OEN, (AGPIO_Pin4));
    // 8233_SW_EN_1V8
    AD5940_WriteReg(REG_AGPIO_GP0SET, AGPIO_Pin4);
  } else {
    /* Gp0 Output enable */
    AD5940_WriteReg(REG_AGPIO_GP0OEN, (AGPIO_Pin4));
    // 8233_SW_EN_1V8
    AD5940_WriteReg(REG_AGPIO_GP0CLR, AGPIO_Pin4);
  }
}

/*!
 ****************************************************************************
 * @brief  Ecg App initialization
 * @param  None
 * @return ECG_ERROR_CODE_t
 *****************************************************************************/
ECG_ERROR_CODE_t EcgAppInit() {

  /* switch off other switches */
  // DG2502_SW_control_AD5940(false);
  DG2502_SW_control_ADPD4000(false);
  ECG_ERROR_CODE_t retVal = ECG_ERROR;

    adp5360_enable_ldo(ECG_LDO,true);
    InitCfg();
    ClearDataBufferAd5940();
    ad5940_ecg_start();
    user_applied_packetization_enable=0;/* re init every time so that by default packetization is enabled for every start */
  /* GPIO for ECG mode and electrodes */
  /* Enable Sport AD8233 device */
  HAL_GPIO_ECG_Sport_Enable(1);
  retVal = ECG_SUCCESS;

  return retVal;
}

/*!
 ****************************************************************************
 * @brief  Ecg App Deinitialization
 * @param  None
 * @return ECG_ERROR_CODE_t: Success/Error
 *****************************************************************************/
ECG_ERROR_CODE_t EcgAppDeInit() {
  //if (AppECGCtrl(APPCTRL_STOPSYNC, 0) != 0) {
  //  return ECG_ERROR;
  //}
  AppECGCfg_Type *pCfg;

  AppECGGetCfg(&pCfg);

  disable_ad5940_ext_trigger(pCfg->ECGODR);
  /* Disable Sport AD8233 device */
  HAL_GPIO_ECG_Sport_Enable(0);

  /* de init power */
  adp5360_enable_ldo(ECG_LDO, false);
  return ECG_SUCCESS;
}

/*!
 ****************************************************************************
 * @brief    Example of how to write an LCFG parameter
 * @param    LCFG field that has to be written
 * @param    Value to be written
 * @retval   ECG_ERROR_CODE_t
 *****************************************************************************/
#ifdef DEBUG_ECG
 uint32_t ecgodr;
#endif
ECG_ERROR_CODE_t EcgWriteLCFG(uint8_t field, uint16_t value) {
  AppECGCfg_Type *pCfg;
  AppECGGetCfg(&pCfg);
  if (field < ECG_LCFG_MAX) {
    switch (field) {
    case ECG_LCFG_FS:
      pCfg->ECGODR = value;
#ifdef DEBUG_ECG
      ecgodr = pCfg->ECGODR;
#endif
      ecg_user_applied_odr = 1;
      break;
    case ECG_LCFG_ADC_PGA_GAIN:
      pCfg->AdcPgaGain = value;
      ecg_user_applied_adcPgaGain = 1;
      break;
    case ECG_LCFG_AFE_PWR_MOD:
      pCfg->PwrMod = value;
      user_applied_ecg_pwr_mod = 1;
      break;
    case ECG_LCFG_PACKETIZATION_ENABLE:
      pCfg->PacketizationEn = value;
      user_applied_packetization_enable = 1;
      reset_ecg_packetization();// start packetization from first sample and clear leadson/off samples.
      break;
    }
    return ECG_SUCCESS;
  }
  return ECG_ERROR;
}

/*!
 ****************************************************************************
 * @brief    Read LCFG parameter
 * @param    LCFG field
 * @param    Returned corresponding LCFG value
 * @retval   ECG_ERROR_CODE_t
 *****************************************************************************/
ECG_ERROR_CODE_t EcgReadLCFG(uint8_t index, uint16_t *value) {
  AppECGCfg_Type *pCfg;
  AppECGGetCfg(&pCfg);
  if (index < ECG_LCFG_MAX) {
    switch (index) {
    case ECG_LCFG_FS:
      *value = (uint16_t)(pCfg->ECGODR);
      break;
    case ECG_LCFG_ADC_PGA_GAIN:
      *value = pCfg->AdcPgaGain;
      break;
    case ECG_LCFG_AFE_PWR_MOD:
      *value = pCfg->PwrMod;
      break;
   case ECG_LCFG_PACKETIZATION_ENABLE:
      *value = pCfg->PacketizationEn;
      break;
    }
    return ECG_SUCCESS;
  }
  return ECG_ERROR;
}

///////////////////////////////////////////////////////////////////
#ifdef DCB

/*!
 ****************************************************************************
 * @brief    Gets the entire ecg DCB configuration written in flash
 * @param    ecg_dcb_data: pointer to dcb struct variable,
 *            in size of data in Double Word (32-bits)
 * @param    read_size: The size of the record to be returned to the user
 * @retval   ECG_ERROR_CODE_t: Status
 *****************************************************************************/
ECG_ERROR_CODE_t read_ecg_dcb(uint32_t *ecg_dcb_data, uint16_t *read_size) {
  ECG_ERROR_CODE_t dcb_status = ECG_ERROR;

  if (adi_dcb_read_from_fds(ADI_DCB_ECG_BLOCK_IDX, ecg_dcb_data, read_size) ==
      DEF_OK) {
    dcb_status = ECG_SUCCESS;
  }
  return dcb_status;
}

/*!
 ****************************************************************************
 * @brief    Sets the entire ecg DCB configuration in flash
 * @param    ecg_dcb_data: pointer to dcb struct variable,
 *            in size of data in Double Word (32-bits)
 * @param    write_Size: The size of the record to be written
 * @retval   ECG_ERROR_CODE_t: Status
 *****************************************************************************/
ECG_ERROR_CODE_t write_ecg_dcb(uint32_t *ecg_dcb_data, uint16_t write_Size) {
  ECG_ERROR_CODE_t dcb_status = ECG_ERROR;

  if (adi_dcb_write_to_fds(ADI_DCB_ECG_BLOCK_IDX, ecg_dcb_data, write_Size) ==
      DEF_OK) {
    dcb_status = ECG_SUCCESS;
  }
  return dcb_status;
}

/*!
 ****************************************************************************
 * @brief    Delete the entire ecg DCB configuration in flash
 * @param    None
 * @retval   ECG_ERROR_CODE_t: Status
 *****************************************************************************/
ECG_ERROR_CODE_t delete_ecg_dcb(void) {
  ECG_ERROR_CODE_t dcb_status = ECG_ERROR;

  if (adi_dcb_delete_fds_settings(ADI_DCB_ECG_BLOCK_IDX) == DEF_OK) {
    dcb_status = ECG_SUCCESS;
  }
  return dcb_status;
}

/*!
 ****************************************************************************
 * @brief    Set the dcb present flag
 * @param    set_flag: flag to set presence of dcb in flash
 * @retval   None
 *****************************************************************************/
void ecg_set_dcb_present_flag(bool set_flag) {
  g_ecg_dcb_Present = set_flag;
  NRF_LOG_INFO("ECG DCB present set: %s",
      (g_ecg_dcb_Present == true ? "TRUE" : "FALSE"));
}

/*!
 ****************************************************************************
 * @brief    Get whether the dcb is present in flash
 * @param    None
 * @retval   bool: TRUE: dcb present, FALSE: dcb absent
 *****************************************************************************/
bool ecg_get_dcb_present_flag(void) {
  NRF_LOG_INFO(
      "ECG DCB present: %s", (g_ecg_dcb_Present == true ? "TRUE" : "FALSE"));
  return g_ecg_dcb_Present;
}

/*!
 ****************************************************************************
 * @brief    Update the global dcb presence flag in flash
 * @param    None
 * @retval   None
 *****************************************************************************/
void ecg_update_dcb_present_flag(void) {
  g_ecg_dcb_Present = adi_dcb_check_fds_entry(ADI_DCB_ECG_BLOCK_IDX);
  NRF_LOG_INFO("Updated. ECG DCB present: %s",
      (g_ecg_dcb_Present == true ? "TRUE" : "FALSE"));
}

/*!
 ****************************************************************************
 * @brief    Update the lcfg from dcb
 * @param    p_pkt: pointer to packet
 * @retval   m2m2_hdr_t
 *****************************************************************************/
static m2m2_hdr_t *ecg_app_set_dcb_lcfg(m2m2_hdr_t *p_pkt) {
  M2M2_APP_COMMON_STATUS_ENUM_t status = M2M2_APP_COMMON_STATUS_ERROR;
  PKT_MALLOC(p_resp_pkt, ecg_app_dcb_lcfg_t, 0);
  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, ecg_app_dcb_lcfg_t, p_resp_payload);

    if (ecg_get_dcb_present_flag() == true) {
      uint32_t ecg_dcb[MAXECGDCBSIZE] = {'\0'};
      uint16_t size = MAXECGDCBSIZE;
      if (read_ecg_dcb(ecg_dcb, &size) == ECG_SUCCESS) {
        for (uint8_t i = 0; i < size; i++) {
          /*
           each ecg_dcb array element stores field and value together in total 4
           bytes. field -> 1 Byte value -> 2 Bytes undefined -> 1 Byte for ex-
           if ecg_dcb[i] = 0x00ABCDEF field = 0xAB Value = 0xCDEF
           not_used/undefined -> 0x00
         */
          /* doing right shift and then & operation to get field */
          uint8_t field = (ecg_dcb[i] >> 16) & 0xFF;
          /* doing & operation to get value */
          uint16_t value = ecg_dcb[i] & 0xFFFF;

          if (EcgWriteLCFG(field, value) == ECG_SUCCESS) {
            status = M2M2_APP_COMMON_STATUS_OK;
          } else {
            status = M2M2_APP_COMMON_STATUS_ERROR;
          }
        }
      } else {
        status = M2M2_APP_COMMON_STATUS_ERROR;
      }
    } else {
      status =
          M2M2_APP_COMMON_STATUS_ERROR; // Since this function is for setting
                                        // dcb lcfg for ecg, and because this is
                                        // the case when dcb lcfg is not present,
                                        // therefore error status given.
    }

    p_resp_payload->status = status;
    p_resp_payload->command = M2M2_APP_COMMON_CMD_SET_LCFG_RESP;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  }
  return p_resp_pkt;
}

static m2m2_hdr_t *ecg_dcb_command_read_config(m2m2_hdr_t *p_pkt) {
  static uint16_t r_size = 0;
  uint32_t dcbdata[MAXECGDCBSIZE];
  M2M2_DCB_STATUS_ENUM_t status = M2M2_DCB_STATUS_ERR_NOT_CHKD;

  PKT_MALLOC(p_resp_pkt, m2m2_dcb_ecg_data_t, 0);
  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_dcb_ecg_data_t, p_resp_payload);

    r_size = MAXECGDCBSIZE;
    if (read_ecg_dcb(&dcbdata[0], &r_size) == ECG_SUCCESS) {
      for (int i = 0; i < r_size; i++)
        p_resp_payload->dcbdata[i] = dcbdata[i];
      p_resp_payload->size = (r_size);
      status = M2M2_DCB_STATUS_OK;
    } else {
      p_resp_payload->size = 0;
      status = M2M2_DCB_STATUS_ERR_ARGS;
    }

    p_resp_payload->status = status;
    p_resp_payload->command = M2M2_DCB_COMMAND_READ_CONFIG_RESP;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  }
  return p_resp_pkt;
}

static m2m2_hdr_t *ecg_dcb_command_write_config(m2m2_hdr_t *p_pkt) {
  M2M2_DCB_STATUS_ENUM_t status = M2M2_DCB_STATUS_ERR_NOT_CHKD;
  uint32_t dcbdata[MAXECGDCBSIZE];

  /* Declare a pointer to access the input packet payload */
  PYLD_CST(p_pkt, m2m2_dcb_ecg_data_t, p_in_payload);
  PKT_MALLOC(p_resp_pkt, m2m2_dcb_ecg_data_t, 0);
  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_dcb_ecg_data_t, p_resp_payload);

    for (int i = 0; i < p_in_payload->size; i++)
      dcbdata[i] = p_in_payload->dcbdata[i];
    if (write_ecg_dcb(&dcbdata[0], p_in_payload->size) == ECG_SUCCESS) {
      ecg_set_dcb_present_flag(true);
      status = M2M2_DCB_STATUS_OK;
    } else {
      status = M2M2_DCB_STATUS_ERR_ARGS;
    }

    p_resp_payload->status = status;
    p_resp_payload->command = M2M2_DCB_COMMAND_WRITE_CONFIG_RESP;
    p_resp_payload->size = 0;
    for(uint16_t i=0; i< MAXECGDCBSIZE; i++)
      p_resp_payload->dcbdata[i] = 0;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  }
  return p_resp_pkt;
}

static m2m2_hdr_t *ecg_dcb_command_delete_config(m2m2_hdr_t *p_pkt) {
  M2M2_DCB_STATUS_ENUM_t status = M2M2_DCB_STATUS_ERR_NOT_CHKD;
  PKT_MALLOC(p_resp_pkt, m2m2_dcb_ecg_data_t, 0);
  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_dcb_ecg_data_t, p_resp_payload);

    if (delete_ecg_dcb() == ECG_SUCCESS) {
      ecg_set_dcb_present_flag(false);
      status = M2M2_DCB_STATUS_OK;
    } else {
      status = M2M2_DCB_STATUS_ERR_ARGS;
    }

    p_resp_payload->status = status;
    p_resp_payload->command = M2M2_DCB_COMMAND_ERASE_CONFIG_RESP;
    p_resp_payload->size = 0;
    for(uint16_t i=0; i< MAXECGDCBSIZE; i++)
      p_resp_payload->dcbdata[i] = 0;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  }
  return p_resp_pkt;
}
#endif //DCB

#endif //ENABLE_ECG_APP
