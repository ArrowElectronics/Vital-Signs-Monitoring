/*!
 *  \copyright Analog Devices
 * ****************************************************************************
 *
 * License Agreement
 *
 * Copyright (c) 2019 Analog Devices Inc.
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

/* Includes ------------------------------------------------------------------*/
#include <post_office.h>
#include <mail_box.h>
#include <task_includes.h>
#include <system_task.h>
#include <adpd4000_task.h>
#include <adxl_task.h>
#include <usbd_task.h>
#include <ble_task.h>
#include <file_system_task.h>
#include <watch_dog_task.h>
#ifdef ENABLE_PPG_APP
#include <ppg_application_task.h>
#include <sync_data_application_task.h>
#endif
#ifdef ENABLE_ECG_APP
#include <ecg_task.h>
#endif
#ifdef ENABLE_EDA_APP
#include <eda_application_task.h>
#endif
#ifdef ENABLE_BIA_APP
#include <bia_application_task.h>
#endif
#ifdef ENABLE_TEMPERATURE_APP
#include <temperature_task.h>
#endif
#ifdef ENABLE_PEDO_APP
#include <pedometer_task.h>
#endif
#ifdef ENABLE_SQI_APP
#include <sqi_task.h>
#endif
#ifdef ENABLE_WATCH_DISPLAY
#include <display_app.h>
#endif
#include <low_touch_task.h>
#include <touch_detect.h>
#ifdef USER0_CONFIG_APP
#include <user0_config_app_task.h>
#endif
#include <m2m2_core.h>
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

/* Private macros ------------------------------------------------------------*/
static void post_office_route(m2m2_hdr_t *p_msg);
static post_office_app_msg_send_func_t *get_routing_table_entry(M2M2_ADDR_ENUM_t addr);

static void PostOfficeTask(void *pArgument);
static void send_message_post_office_task(m2m2_hdr_t *p_pkt);

/* Private variables ---------------------------------------------------------*/
uint8_t PostOfficeTaskStack[APP_OS_CFG_POST_OFFICE_TASK_STK_SIZE];
ADI_OSAL_THREAD_HANDLE PostOfficeTaskHandler;
ADI_OSAL_STATIC_THREAD_ATTR PostOfficeTaskAttributes;
StaticTask_t PostOfficeTaskTcb;

ADI_OSAL_QUEUE_HANDLE  post_office_task_msg_queue = NULL;
ADI_OSAL_QUEUE_HANDLE  wdt_task_msg_queue = NULL;

ADI_OSAL_QUEUE_HANDLE   task_queue_list[APP_OS_CFG_MAX_TASKS] = {NULL};

#ifdef COLLISION_DEBUG
uint32_t gPOPingReqCnt =0, gPOPingRespCnt = 0;
#endif //COLLISION_DEBUG
#ifdef MEM_MGMT_DBG
msg_mgmt_info_t mem_create_arr, mem_free_arr[MEM_DBG_SIZE];
uint8_t gMemAllocIndx = 0, gMemFreeIndx = 0;
#endif //MEM_MGMT_DBG

/* API which updates the task queue list based on the task index*/
void update_task_queue_list (task_queue_index_t task_index, ADI_OSAL_QUEUE_HANDLE task_queue_handle)
{
  task_queue_list[task_index] =  task_queue_handle;
}


/* API which returns the task queue handler based on the task index*/
ADI_OSAL_QUEUE_HANDLE get_task_queue_handler( task_queue_index_t task_index)
{
  return task_queue_list[task_index];
}

/*!
 * @brief:  The master routing table for this Post Office.
 */
post_office_routing_table_entry_t m2m2_routing_table[] = {
  {M2M2_ADDR_UNDEFINED,           NULL}, //0
  {M2M2_ADDR_POST_OFFICE,         send_message_post_office_task},//1
  {M2M2_ADDR_EXTERNAL,            send_message_usbd_tx_task},//2
  {M2M2_ADDR_APP_CLI,             send_message_usbd_tx_task},//3
  {M2M2_ADDR_APP_CLI_BLE,         send_message_ble_tx_task},//4
  {M2M2_ADDR_APP_DROID,           send_message_ble_tx_task},//5
  {M2M2_ADDR_APP_IOS,             send_message_ble_tx_task},//6
  {M2M2_ADDR_APP_WT,              send_message_usbd_tx_task},//7
  {M2M2_ADDR_BLE_SERVICES_SENSOR, send_message_ble_services_sensor_task},//8
  {M2M2_ADDR_SENSOR_AD7156,       send_message_top_touch_task},//9
#ifdef USE_FS
  {M2M2_ADDR_SYS_FS,               send_message_file_system_task},//10
#endif
#ifdef ENABLE_WATCH_DISPLAY
  {M2M2_ADDR_DISPLAY,               send_message_display_task},//11
#endif
#ifdef LOW_TOUCH_FEATURE
  {M2M2_ADDR_APP_LT_APP,            send_message_lt_task},//12
#endif
  {M2M2_ADDR_SYS_PM,                send_message_system_task},//13
#ifdef ENABLE_PPG_APP
  {M2M2_ADDR_MED_PPG,               send_message_ppg_application_task},//14
#endif
#ifdef ENABLE_EDA_APP
  {M2M2_ADDR_MED_EDA,               send_message_ad5940_eda_task},//15
#endif
#ifdef ENABLE_ECG_APP
  {M2M2_ADDR_MED_ECG,               send_message_ad5940_ecg_task},//16
#endif
#ifdef ENABLE_TEMPERATURE_APP
  {M2M2_ADDR_MED_TEMPERATURE,       send_message_temperature_app_task},//17
#endif
#ifdef ENABLE_BIA_APP
  {M2M2_ADDR_MED_BIA,               send_message_ad5940_bia_task},
#endif
#ifdef ENABLE_PEDO_APP
  {M2M2_ADDR_MED_PED,               send_message_pedometer_app_task},
#endif
  {M2M2_ADDR_SENSOR_ADXL,           send_message_adxl_task},//20
  {M2M2_ADDR_SENSOR_ADPD4000,       send_message_adpd4000_task},//21
#ifdef ENABLE_SQI_APP
  {M2M2_ADDR_MED_SQI,               send_message_sqi_app_task},//22
#endif
#ifdef USER0_CONFIG_APP
  {M2M2_ADDR_USER0_CONFIG_APP,      send_message_user0_config_app},//23
#endif
//  //  M2M2_ADDR_HIGHEST
//  //  M2M2_ADDR_GLOBAL
};

//  Number of entries in the routing table.
#define PO_ROUTING_TABLE_SIZE (sizeof(m2m2_routing_table)/sizeof(m2m2_routing_table[0]))

/*!
  *@brief     Gets size of post office routing table
  *@param     pointer in which size of routing table will be updated
  *@return    None
 */
void get_routing_table_size(uint8_t *pSize) {
    *pSize = PO_ROUTING_TABLE_SIZE;
}

#ifdef DEBUG_PKT
typedef struct {
uint32_t cnt_created;
uint32_t cnt_consumed;
}po_msg_info;

typedef enum {
ADPD4k,
TEMP_APP,
ADXL_APP,
PPG_APP,
SYNC_PPG_APP,
AGC_APP,
HRV_APP,
ECG_APP,
EDA_APP,
BCM_APP,
TOTAL_APPS
}app_index;

po_msg_info po_apps[TOTAL_APPS];
#endif

void post_office_task_init(void) {

#ifdef DEBUG_PKT
  memset(po_apps, 0, sizeof(po_apps));
#endif

  ADI_OSAL_STATUS eOsStatus;

  /* Create post office thread */
  PostOfficeTaskAttributes.pThreadFunc = PostOfficeTask;
  PostOfficeTaskAttributes.nPriority = APP_OS_CFG_POST_OFFICE_TASK_PRIO;
  PostOfficeTaskAttributes.pStackBase = &PostOfficeTaskStack[0];
  PostOfficeTaskAttributes.nStackSize = APP_OS_CFG_POST_OFFICE_TASK_STK_SIZE;
  PostOfficeTaskAttributes.pTaskAttrParam = NULL;
  PostOfficeTaskAttributes.szThreadName = "PostOffice";
  PostOfficeTaskAttributes.pThreadTcb = &PostOfficeTaskTcb;
  eOsStatus = adi_osal_MsgQueueCreate(&post_office_task_msg_queue,NULL,
                                    125);//TODO: Need to review the Queue size based on multiple streams that will be started from CLI
  if (eOsStatus != ADI_OSAL_SUCCESS) {
      Debug_Handler();
  }
  else
  {
    update_task_queue_list(APP_OS_CFG_POST_OFFICE_TASK_INDEX,post_office_task_msg_queue);
  }
  eOsStatus = adi_osal_ThreadCreateStatic(&PostOfficeTaskHandler,
                                    &PostOfficeTaskAttributes);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
      Debug_Handler();
  }

}


/*!
  *@brief     The post office's message routing handler.
  *           This function is called whenever the post office sends a message to itself.
  *@param     p_pkt The message to send
  *@return    None
 */
static void send_message_post_office_task(m2m2_hdr_t *p_pkt) {
//  ADI_OSAL_QUEUE_HANDLE nMsgHandler;
//  nMsgHandler = get_task_queue_handler(APP_OS_CFG_POST_OFFICE_TASK_INDEX);
  ADI_OSAL_STATUS osal_result;
  osal_result = adi_osal_MsgQueuePost(post_office_task_msg_queue,p_pkt);
  if (osal_result != ADI_OSAL_SUCCESS)
    post_office_consume_msg(p_pkt);
}


/*!
  *@brief     Fetch the destination task pointer based on an address.
  *@param     addr The address whose pointer should be retrieved
  *@return    A task pointer for the task with address addr
 */
static post_office_app_msg_send_func_t *get_routing_table_entry(M2M2_ADDR_ENUM_t addr) {
  for (int i = 0; i < PO_ROUTING_TABLE_SIZE; i++) {
    if (m2m2_routing_table[i].address == addr) {
      return m2m2_routing_table[i].p_func;
    }
  }
  return NULL;
}

/*!
  *@brief    Gets a Application address that corresponds to nElement place of routing table
  *@return   returns a Application address that sits in nElement place of routing table
 */
M2M2_ADDR_ENUM_t get_routing_table_element(uint8_t nElement) {
    if (m2m2_routing_table[nElement].address != NULL) {
        if(m2m2_routing_table[nElement].p_func != NULL) {
            return m2m2_routing_table[nElement].address;
        }
    }
  return NULL;
}

/*!
  *@brief     Gets a Application index in routing table based on m2m2 address
  *@return   returns a Application index that sits in routing table based on m2m2 address
 */
uint8_t get_routing_table_index(M2M2_ADDR_ENUM_t m2m2_address) {
    uint8_t i;

    for(i=1;i<PO_ROUTING_TABLE_SIZE;i++) {
      if (m2m2_routing_table[i].address == m2m2_address)  {
        return i; //index matching to m2m2_address returned
      }
    }
    return 0; //no m2m2 address found
}

//static m2m2_hdr_t *pkt_hdr;

/*!
  *@brief       The entry point for the Post Office task.
  *
  * Runs in a loop waiting for uC/OS messages to route.
  *@return      None.
 */
static void PostOfficeTask(void *pArgument) {



  mailbox_list_init(DEFAULT_MAX_NUM_MAILBOXES);
  while (1) {
    post_office_app_msg_send_func_t *p_msg_send_func;
    m2m2_hdr_t  *p_msg;
    // wait for a message
    if (adi_osal_MsgQueuePend(post_office_task_msg_queue,(void **)&p_msg,ADI_OSAL_TIMEOUT_FOREVER) != ADI_OSAL_SUCCESS)
      continue;
    else
    {
//      pkt_hdr = (m2m2_hdr_t *)p_msg;
//      msg_len = BYTE_SWAP_16(pkt_hdr->length);
    }
    // Check the message status

    // See if the message is for the Post Office
    switch (p_msg->dest) {
      case M2M2_ADDR_UNDEFINED:
        // The sender didn't set the destination! Drop the message
        post_office_consume_msg(p_msg);
        continue;
      case M2M2_ADDR_POST_OFFICE: {
        // This is a post office config packet, handle it
        post_office_config_t *cfg = (post_office_config_t*) &(p_msg->data[0]);
        switch (cfg->cmd) {
          case POST_OFFICE_CFG_CMD_ADD_MAILBOX: {
            add_mailbox(cfg->box,
                        DEFAULT_MAX_MAILBOX_SUBSCRIBER_NUM);
            break;
          } case POST_OFFICE_CFG_CMD_REMOVE_MAILBOX: {
            remove_mailbox(cfg->box);
            break;
          } case POST_OFFICE_CFG_CMD_MAILBOX_SUBSCRIBE: {
            mailbox_add_sub(cfg->box, cfg->sub);
            break;
          } case POST_OFFICE_CFG_CMD_MAILBOX_UNSUBSCRIBE: {
            mailbox_remove_sub(cfg->box, cfg->sub);
            break;
          }default :
          // This is an erroneous message, handle it somehow
            break;
        }
        post_office_consume_msg(p_msg);
        break;
      } case M2M2_ADDR_GLOBAL: {
        // This is a global broadcast message, send a copy to every task
        for (uint8_t i = 0; i < PO_ROUTING_TABLE_SIZE; i++) {
          p_msg_send_func = m2m2_routing_table[i].p_func;
          if (p_msg_send_func != NULL) {
            m2m2_hdr_t *p_m2m2_;
            p_m2m2_ = post_office_create_msg(p_msg->length);
            if (p_m2m2_ != NULL) {
              p_msg->dest = m2m2_routing_table[i].address;
              memcpy(p_m2m2_, (void *)p_msg, p_msg->length);
              p_m2m2_->src = p_msg->src;
              p_msg_send_func(p_m2m2_);
            }
          }
        }
        post_office_consume_msg(p_msg);
        break;
      }
      default: {
#ifdef COLLISION_DEBUG
      if((p_msg->dest == M2M2_ADDR_SYS_PM) && (p_msg->length == 14))
        gPOPingReqCnt++;
      if((p_msg->dest == M2M2_ADDR_APP_CLI) && (p_msg->length == 14))
        gPOPingRespCnt++;
#endif //COLLISION_DEBUG
        // This is a regular message, route it appropriately
        post_office_route(p_msg);
        break;
      }
    }
  }
}


/*!
  *@brief       Send a message to the Post Office.
  *@param pkt   The packet to be sent.
  *@param err   Pointer to a variable that will contain the return status.
  *@return      None.
  */
void post_office_send(m2m2_hdr_t *pkt, ADI_OSAL_STATUS *err) {
  ADI_OSAL_STATUS osal_result;
  osal_result = adi_osal_MsgQueuePost(post_office_task_msg_queue,pkt);
  if (osal_result != ADI_OSAL_SUCCESS)
    post_office_consume_msg(pkt);
}

/*!
  *@brief         Block while waiting for a message from the Post Office.
  *@param timeout Time to spend waiting for a message, in ticks.
  *@return        A pointer to the message retrieved from the Post Office. Null if no message was received.
 */
m2m2_hdr_t *post_office_get(ADI_OSAL_TICKS timeout, task_queue_index_t nIndex) {
  m2m2_hdr_t *p_msg;
  ADI_OSAL_QUEUE_HANDLE nMsgHandler;
  nMsgHandler = get_task_queue_handler(nIndex);
  if (adi_osal_MsgQueuePend(nMsgHandler,(void **)&p_msg,timeout) != ADI_OSAL_SUCCESS)
      return NULL;
  else
  {
      return p_msg;
  }
}

/*!
  *@brief     Route a message to the appropriate task message receive function
  *@param     p_pkt   The packet to be routed
  *@return    None.
 */

static void post_office_route(m2m2_hdr_t *p_msg) {
  post_office_app_msg_send_func_t *p_msg_send_func = NULL;

  p_msg_send_func = get_routing_table_entry(p_msg->dest);
  if (p_msg_send_func == NULL) {
    // The destination is a local mailbox, find the list of recipients
    send_to_mailbox(p_msg);
  } else {
    p_msg_send_func(p_msg);
  }
}

/*!
  *@brief         Create message packet used by the Post Office.
  *@param length  length of message packet requested.
  *@return        A pointer to retrieved buffer used for mail to post office. Null if no message is available.
 */
static volatile int32_t mem_fails = 0;
m2m2_hdr_t *post_office_create_msg(uint16_t length) {
  m2m2_hdr_t *p_msg = NULL;
  if (length != 0 && length < SZ_MEM_BLK_TYPE5) {
    if (memory_block_get((void **)&p_msg, length) == MEM_MANAGER_ERR_SUCCESS) {
      p_msg->length = length;
      ASSERT(p_msg!=NULL);
      return p_msg;
    }
  }
  mem_fails++;
  return NULL;
}

#ifdef DEBUG_PKT
void post_office_msg_cnt(m2m2_hdr_t *p_msg)
{
      if(p_msg->length == (sizeof(m2m2_sensor_adpd4000_data_stream_t) + M2M2_HEADER_SZ) && (p_msg->src >= M2M2_ADDR_SENSOR_ADPD4000))
      {
      po_apps[ADPD4k].cnt_created++;
      }
      else if(p_msg->length == (sizeof(temperature_app_stream_t) + M2M2_HEADER_SZ) && p_msg->src == M2M2_ADDR_MED_TEMPERATURE && p_msg->dest == M2M2_ADDR_MED_TEMPERATURE_STREAM)
      {
      po_apps[TEMP_APP].cnt_created++;
      }
      else if(p_msg->length == (sizeof(ppg_app_hr_debug_stream_t) + M2M2_HEADER_SZ) && p_msg->src == M2M2_ADDR_MED_PPG && p_msg->dest == M2M2_ADDR_MED_PPG_STREAM)
      {
      po_apps[PPG_APP].cnt_created++;
      }
      else if(p_msg->length == (sizeof(adpd_adxl_sync_data_stream_t) + M2M2_HEADER_SZ) && p_msg->src == M2M2_ADDR_MED_PPG && p_msg->dest == M2M2_ADDR_MED_SYNC_ADPD_ADXL_STREAM)
      {
      po_apps[SYNC_PPG_APP].cnt_created++;
      }
      else if(p_msg->length == (sizeof(ppg_app_agc_info_t) + M2M2_HEADER_SZ) && p_msg->src == M2M2_ADDR_MED_PPG && p_msg->dest == M2M2_ADDR_SYS_AGC_STREAM)
      {
      po_apps[AGC_APP].cnt_created++;
      }
      else if(p_msg->length == (sizeof(ppg_app_hrv_info_t) + M2M2_HEADER_SZ) && p_msg->src == M2M2_ADDR_MED_PPG && p_msg->dest == M2M2_ADDR_SYS_HRV_STREAM)
      {
      po_apps[HRV_APP].cnt_created++;
      }
      else if(p_msg->length == (sizeof(m2m2_sensor_adxl_data_no_compress_stream_t) + M2M2_HEADER_SZ) && p_msg->src == M2M2_ADDR_SENSOR_ADXL && p_msg->dest == M2M2_ADDR_SENSOR_ADXL_STREAM)
      {
      po_apps[ADXL_APP].cnt_created++;
      }
      else if(p_msg->length == (sizeof(ecg_app_stream_t) + M2M2_HEADER_SZ) && p_msg->src == M2M2_ADDR_MED_ECG && p_msg->dest == M2M2_ADDR_MED_ECG_STREAM)
      {
      po_apps[ECG_APP].cnt_created++;
      }
      else if(p_msg->length == (sizeof(eda_app_stream_t) + M2M2_HEADER_SZ) && p_msg->src == M2M2_ADDR_MED_EDA)
      {
      po_apps[EDA_APP].cnt_created++;
      }
      else if(p_msg->length == (sizeof(bcm_app_stream_t) + M2M2_HEADER_SZ) && p_msg->src == M2M2_ADDR_MED_BIA)
      {
      po_apps[BCM_APP].cnt_created++;
      }
}
#endif

/*!
  *@brief         Consume/free message packet used by the Post Office.
  *@param pmsg    Pointer to message packet to be consumed
  *@param length  length of message packet to be consumed
 */
void post_office_consume_msg(m2m2_hdr_t *p_msg) {
#ifdef DEBUG_PKT
      if(p_msg->length == (sizeof(m2m2_sensor_adpd4000_data_stream_t) + M2M2_HEADER_SZ) && (p_msg->src >= (M2M2_ADDR_ENUM_t)BYTE_SWAP_16(M2M2_ADDR_SENSOR_ADPD_STREAM1) && p_msg->src <= (M2M2_ADDR_ENUM_t)BYTE_SWAP_16(M2M2_ADDR_SENSOR_ADPD_STREAM12)))
      {
      po_apps[ADPD4k].cnt_consumed++;
      }
      else if(p_msg->length == (sizeof(temperature_app_stream_t) + M2M2_HEADER_SZ) && p_msg->src == (M2M2_ADDR_ENUM_t)BYTE_SWAP_16(M2M2_ADDR_MED_TEMPERATURE_STREAM))
      {
      po_apps[TEMP_APP].cnt_consumed++;
      }
      else if(p_msg->length == (sizeof(adpd_adxl_sync_data_stream_t) + M2M2_HEADER_SZ) && p_msg->src == (M2M2_ADDR_ENUM_t)BYTE_SWAP_16(M2M2_ADDR_MED_SYNC_ADPD_ADXL_STREAM))
      {
      po_apps[SYNC_PPG_APP].cnt_consumed++;
      }
      else if(p_msg->length == (sizeof(ppg_app_hr_debug_stream_t) + M2M2_HEADER_SZ) && p_msg->src == (M2M2_ADDR_ENUM_t)BYTE_SWAP_16(M2M2_ADDR_MED_PPG_STREAM))
      {
      po_apps[PPG_APP].cnt_consumed++;
      }
      else if(p_msg->length == (sizeof(ppg_app_hrv_info_t) + M2M2_HEADER_SZ) && p_msg->src == (M2M2_ADDR_ENUM_t)BYTE_SWAP_16(M2M2_ADDR_SYS_HRV_STREAM))
      {
      po_apps[HRV_APP].cnt_consumed++;
      }
      else if(p_msg->length == (sizeof(ppg_app_agc_info_t) + M2M2_HEADER_SZ) && p_msg->src == (M2M2_ADDR_ENUM_t)BYTE_SWAP_16(M2M2_ADDR_SYS_AGC_STREAM))
      {
      po_apps[AGC_APP].cnt_consumed++;
      }
      else if(p_msg->length == (sizeof(m2m2_sensor_adxl_data_no_compress_stream_t) + M2M2_HEADER_SZ) && (p_msg->src == (M2M2_ADDR_ENUM_t)BYTE_SWAP_16(M2M2_ADDR_SENSOR_ADXL_STREAM) || p_msg->src == M2M2_ADDR_SENSOR_ADXL_STREAM))
      {
      po_apps[ADXL_APP].cnt_consumed++;
      }
      else if(p_msg->length == (sizeof(ecg_app_stream_t) + M2M2_HEADER_SZ) && p_msg->src == (M2M2_ADDR_ENUM_t)BYTE_SWAP_16(M2M2_ADDR_MED_ECG_STREAM))
      {
      po_apps[ECG_APP].cnt_consumed++;
      }
      else if(p_msg->length == (sizeof(eda_app_stream_t) + M2M2_HEADER_SZ) && p_msg->src == (M2M2_ADDR_ENUM_t)BYTE_SWAP_16(M2M2_ADDR_MED_EDA_STREAM))
      {
      po_apps[EDA_APP].cnt_consumed++;
      }
      else if(p_msg->length == (sizeof(bcm_app_stream_t) + M2M2_HEADER_SZ) && p_msg->src == (M2M2_ADDR_ENUM_t)BYTE_SWAP_16(M2M2_ADDR_MED_BIA_STREAM))
      {
      po_apps[BCM_APP].cnt_consumed++;
      }
#endif
#ifdef MEM_MGMT_DBG
      mem_free_arr[gMemFreeIndx].pAddr = (uint8_t *) p_msg;
      mem_free_arr[gMemFreeIndx].src = p_msg->src;
      mem_free_arr[gMemFreeIndx].dst = p_msg->dest;
      mem_free_arr[gMemFreeIndx].size = p_msg->length;
      mem_free_arr[gMemFreeIndx].tick = get_sensor_time_stamp();
      if(++gMemFreeIndx >= MEM_DBG_SIZE)
          gMemFreeIndx = 0;
#endif //MEM_MGMT_DBG
      memory_block_free(p_msg, p_msg->length);
}


/*!
  *@brief       Send a message to all of the subscribers of a mailbox.
  *@param p_msg  A pointer to the message to be sent.
  *@return    None.
 */
static void send_to_mailbox(m2m2_hdr_t  *p_msg) {
  M2M2_ADDR_ENUM_t *sub_list = NULL;
  uint8_t                 num_subs = 0;
  uint8_t                 subs_list_size = 0;
  m2m2_hdr_t              *p_m2m2_ = NULL;

  if (p_msg == NULL) {
    return;
  }
  // Set the source address to the mailbox's address so that subscribers know which
  // mailbox the message is from.
  p_msg->src = p_msg->dest;
  sub_list = get_subscriber_list(p_msg->dest, &num_subs, &subs_list_size);

  if (num_subs == 0) {
    // This mailbox has no subscribers, swallow this packet
    post_office_consume_msg(p_msg);
    return;
  }

  // Don't bother allocating a new message if there's only one subscriber
  if (num_subs == 1) {
    // Need to check all entries in the sub list in case there are some empty subscriber entries before a non-empty entry.
    for (uint8_t i = 0; i < subs_list_size; i++) {
      if (sub_list[i] != M2M2_ADDR_UNDEFINED) {
        p_msg->dest = sub_list[i];
        post_office_route(p_msg);
        // The num_subs == 1, so we're done with this mailbox.
        return;
      }
    }
  }
  // If there's more than one sub, then send them each a copy of the message.
  for (uint8_t i = 0; i < subs_list_size; i++) {
    // Skip "empty" subscriber entries that are denoted by M2M2_ADDR_UNDEFINED
    if (sub_list[i] != M2M2_ADDR_UNDEFINED) {
      p_m2m2_ = post_office_create_msg(p_msg->length);
      adi_osal_EnterCriticalRegion();
      if (p_m2m2_ != NULL) {
        memcpy(p_m2m2_, (void *)p_msg, p_msg->length);
        p_m2m2_->dest = sub_list[i];
        post_office_route(p_m2m2_);
      }
      adi_osal_ExitCriticalRegion();
    }
  }
  post_office_consume_msg(p_msg);
}

/*!
  *@brief       Convenience function to construct and send a request to add a new mailbox in the post office.
  *@param     requester_addr  The address of the application that is adding the mailbox
  *@param     mailbox_addr    The address of the mailbox to which will be created
  *@return    None.
 */
void post_office_add_mailbox(M2M2_ADDR_ENUM_t requester_addr, M2M2_ADDR_ENUM_t mailbox_addr) {
  ADI_OSAL_STATUS err;

  PKT_MALLOC(p_pkt, post_office_config_t, 0);
  PYLD_CST(p_pkt, post_office_config_t, p_pkt_payload);

  p_pkt->src = requester_addr;
  p_pkt->dest = M2M2_ADDR_POST_OFFICE;

  p_pkt_payload->box = mailbox_addr;
  p_pkt_payload->cmd = POST_OFFICE_CFG_CMD_ADD_MAILBOX;
  post_office_send(p_pkt, &err);
}

/*!
  *@brief       Convenience function to remove a created mailbox in the post office.
  *@param     requester_addr  The address of the application that is adding the mailbox
  *@param     mailbox_addr    The address of the mailbox to which will be created
  *@return    None.
 */
void post_office_remove_mailbox(M2M2_ADDR_ENUM_t requester_addr, M2M2_ADDR_ENUM_t mailbox_addr) {
  ADI_OSAL_STATUS err;

  PKT_MALLOC(p_pkt, post_office_config_t, 0);
  PYLD_CST(p_pkt, post_office_config_t, p_pkt_payload);

  p_pkt->src = requester_addr;
  p_pkt->dest = M2M2_ADDR_POST_OFFICE;

  p_pkt_payload->box = mailbox_addr;
  p_pkt_payload->cmd = POST_OFFICE_CFG_CMD_REMOVE_MAILBOX;
  post_office_send(p_pkt, &err);
}

/*!
  *@brief       Convenience function to construct and send a request to add a subscriber to a post office mailbox.
  *@param     requester_addr  The address of the application that is adding a subscriber
  *@param     mailbox_addr    The address of the mailbox to which the subscriber will be added
  *@param     sub_addr        The address of the subscriber that will be added to the mailbox
  *@param     add             Flag specifying whether the subscriber is to be added or removed. 1 to add, 0 to remove.
  *@return    None.
 */
void post_office_setup_subscriber(M2M2_ADDR_ENUM_t requester_addr,
                                  M2M2_ADDR_ENUM_t mailbox_addr,
                                  M2M2_ADDR_ENUM_t sub_addr,
                                  uint8_t add) {
  ADI_OSAL_STATUS err;

  PKT_MALLOC(p_pkt, post_office_config_t, 0);
  PYLD_CST(p_pkt, post_office_config_t, p_pkt_payload);

  p_pkt->dest = M2M2_ADDR_POST_OFFICE;
  p_pkt->src = requester_addr;

  if (add == 1) {
    p_pkt_payload->cmd = POST_OFFICE_CFG_CMD_MAILBOX_SUBSCRIBE;
  } else {
    p_pkt_payload->cmd = POST_OFFICE_CFG_CMD_MAILBOX_UNSUBSCRIBE;
  }

  p_pkt_payload->sub = sub_addr;
  p_pkt_payload->box = mailbox_addr;
  post_office_send(p_pkt, &err);
}
