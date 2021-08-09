/**
 ***************************************************************************
 * @addtogroup Tasks
 * @{
 * @file         usbd_task.c
 * @author       ADI
 * @version      V1.0.0
 * @date         16-May-2019
 * @brief        Source file contains comms task for wearable device framework.
 ***************************************************************************
 ***************************************************************************
 */
/*!
 *  \copyright Analog Devices
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

/*---------------------------- Includes --------------------------------------*/
#include <common_application_interface.h>
#include <includes.h>
#include <system_task.h>
#include <task_includes.h>
#include <usbd_task.h>
#ifdef HIBERNATE_MD_EN
#include <power_manager.h>
#endif

#include "app_timer.h"
#include "app_uart.h"
#include "app_util_platform.h"
#include "nordic_common.h"
#include "nrf.h"

#include "nrf_delay.h"
#include "nrf_drv_clock.h"
#include "nrf_drv_power.h"
#include "nrf_drv_usbd.h"
#include "nrf_gpio.h"
#include "nrf_log.h"

#include "app_error.h"
#include "app_usbd.h"
#include "app_usbd_cdc_acm.h"
#include "app_usbd_core.h"
#include "app_usbd_serial_num.h"
#include "app_usbd_string_desc.h"

#include "app_util.h"
#include <adi_osal.h>
#include <app_cfg.h>
#include <hw_if_config.h>
#include <post_office.h>
#include <task_includes.h>

#include "ble_nus.h"
#include "sdk_config.h"

#include "ad7156_dcfg.h"
#include "adpd4000_dcfg.h"
#include "adxl_dcfg.h"
#include "dcb_general_block.h"
#include "lt_app_lcfg_block.h"
#include "mw_ppg.h"
#include "semphr.h"
#include "task.h"
#include "us_tick.h"
#include "rtc.h"
#include "fds_drv.h"
#include <FreeRTOS.h>

/*------------------------ Private Function Prototype -----------------------*/
static void usbd_tx_task(void *pArgument);
static void usbd_application_task(void *pArgument);
static void cdc_acm_user_ev_handler(
    app_usbd_class_inst_t const *p_inst, app_usbd_cdc_acm_user_event_t event);

uint8_t get_file_download_chunk_count();

/*------------------------ Public Function Prototype -----------------------*/
#ifdef ENABLE_ECG_APP
extern bool ecg_update_dcb_present_flag(void);
#endif
extern bool eda_update_dcb_present_flag(void);

/* enum to hold possible PHY for wired communication */
typedef enum {
  M2M2_PHY_INVALID,
  M2M2_PHY_LOCAL,
  M2M2_PHY_UART,
  M2M2_PHY_SPI,
  M2M2_PHY_USB,
  M2M2_PHY_BLE,
} M2M2_PHY_ENUM_t;

/* If an application is not found in the routing table, this default PHY is
 * used. */
#define M2M2_PHY_DEFAULT M2M2_PHY_USB

/**
 * @brief:  The data structure used to define routing table entry in the Post
 * Office.
 */
typedef struct _usbd_routing_table_entry {
  M2M2_ADDR_ENUM_t address;
  M2M2_PHY_ENUM_t phy;
} usbd_routing_table_entry;

// TODO: Should we keep it?
const usbd_routing_table_entry usbd_routing_table[] = {
    {M2M2_ADDR_UNDEFINED, M2M2_PHY_INVALID},
    {M2M2_ADDR_EXTERNAL, M2M2_PHY_USB},
    {M2M2_ADDR_APP_CLI, M2M2_PHY_USB},
    {M2M2_ADDR_APP_WT, M2M2_PHY_USB},
    {M2M2_ADDR_APP_DROID, M2M2_PHY_BLE},
    {M2M2_ADDR_APP_IOS, M2M2_PHY_BLE},
};
#define USBD_ROUTING_TABLE_SIZE                                                \
  (sizeof(usbd_routing_table) / sizeof(usbd_routing_table[0]))

#ifdef COLLISION_DEBUG
uint32_t gPingRespCnt = 0, gPingReqCnt = 0;
#endif // COLLISION_DEBUG

/* For USBD Tx FreeRTOS Task Creation */
/* Create the stack for task */
uint8_t ga_usbd_tx_task_stack[APP_OS_CFG_USBD_TX_TASK_STK_SIZE];
/* Create handler for task */
ADI_OSAL_THREAD_HANDLE gh_usbd_tx_task_handler;
/* Create task attributes variable */
ADI_OSAL_STATIC_THREAD_ATTR g_usbd_tx_task_attributes;
/* Create TCB for task */
StaticTask_t g_usbd_tx_task_tcb;
/* Create Queue Handler for task */
ADI_OSAL_QUEUE_HANDLE gh_usbd_tx_task_msg_queue = NULL;
// Create semaphores
static ADI_OSAL_SEM_HANDLE g_usbd_tx_task_evt_sem;


/* For USBD app FreeRTOS Task Creation */
/* Create the stack for task */
uint8_t ga_usbd_app_task_stack[APP_OS_CFG_USBD_APP_TASK_STK_SIZE];
/* Create handler for task */
ADI_OSAL_THREAD_HANDLE gh_usbd_app_task_handler;
/* Create task attributes variable */
ADI_OSAL_STATIC_THREAD_ATTR g_usbd_app_task_attributes;
/* Create TCB for task */
StaticTask_t g_usbd_app_task_tcb;

/**
 * The maximum delay inside the USB task to wait for an event.
 */
#define USB_THREAD_MAX_BLOCK_TIME portMAX_DELAY

/* Mutex to protect app_usbd_cdc_acm_write/read apis */
ADI_OSAL_MUTEX_HANDLE g_usb_transfer_lock;
/* Semaphore to block until app_usbd_cdc_acm_write is done &
 * APP_USBD_CDC_ACM_USER_EVT_TX_DONE received */
ADI_OSAL_SEM_HANDLE g_usb_wr_sync;

#define CDC_ACM_COMM_INTERFACE 1
#define CDC_ACM_COMM_EPIN NRF_DRV_USBD_EPIN2

#define CDC_ACM_DATA_INTERFACE 2
#define CDC_ACM_DATA_EPIN NRF_DRV_USBD_EPIN1
#define CDC_ACM_DATA_EPOUT NRF_DRV_USBD_EPOUT1

/** @brief CDC_ACM class instance. */
APP_USBD_CDC_ACM_GLOBAL_DEF(m_app_cdc_acm, cdc_acm_user_ev_handler,
    CDC_ACM_COMM_INTERFACE, CDC_ACM_DATA_INTERFACE, CDC_ACM_COMM_EPIN,
    CDC_ACM_DATA_EPIN, CDC_ACM_DATA_EPOUT, APP_USBD_CDC_COMM_PROTOCOL_AT_V250);

/* enum to hold current status of USB connection in gb_usb_status variable */
enum {
  USB_DISCONNECTED = 0x00, /* disconnected from Host */
  USB_CONNECTED = 0x01,    /* connected to Host */
  USB_PORT_OPENED = 0x02,  /* USB port opened */
  USB_PORT_CLOSED = 0x03,  /* USB port closed */
};
/* Variable holding current USB connection status */
static volatile uint8_t gb_usb_status = USB_DISCONNECTED;
/* Variable to initiate stream stop from all sensors when USB connection
 * status is either USB_DISCONNECTED or USB_PORT_CLOSED */
static volatile bool gb_force_stream_stop = false;
/* Flag to mark that usbd tx task is suspended or not(active) */
uint8_t gn_usbd_tx_task_suspended = 0;
/* Variable to hold the current status of USB tx: done or not; to be used by
 * FS task for log download decisions */
static volatile uint8_t g_usb_tx_pending_flag = 0;
/* To hold the USB connect src address(CLI/WT,etc) */
M2M2_ADDR_ENUM_t usb_pkt_src = M2M2_ADDR_UNDEFINED;
/* Flag to indicate usb powered event */
volatile bool g_usb_power_detected_flag = false;

extern volatile uint32_t  g_lt_task_timeout;
extern ADI_OSAL_SEM_HANDLE   lt_task_evt_sem;
extern ADI_OSAL_THREAD_HANDLE gh_lt_task_handler;
extern uint8_t gLowTouchRunning;
/*!
 ****************************************************************************
 * @brief  Function to do FDS init adn RTC init
 * @param  None
 * @return None
 *****************************************************************************/
void fds_rtc_init(void)
{
    ret_code_t rc;
    rc = adi_fds_init();//must after ble init.
    if(NRF_SUCCESS != rc)
    {
        NRF_LOG_INFO("adi_fds_init fail");
        APP_ERROR_CHECK(rc);
    }
    rtc_init();
    //nrf_delay_ms(500);
}

/*!
 ****************************************************************************
 * @brief  Function to send PO pkts to USB transmit task
 * @param  p_pkt m2m2 packet to be send to USB Tx task
 * @return None
 *****************************************************************************/
void send_message_usbd_tx_task(m2m2_hdr_t *p_pkt) {
  ADI_OSAL_STATUS osal_result;
  osal_result = adi_osal_MsgQueuePost(gh_usbd_tx_task_msg_queue, p_pkt);
  if (osal_result != ADI_OSAL_SUCCESS)
    post_office_consume_msg(p_pkt);
  adi_osal_SemPost(g_usbd_tx_task_evt_sem);
}

/*!
 ****************************************************************************
 * @brief USB Tx task, USBD app init function.
 *****************************************************************************/
void usbd_task_init(void) {
  ADI_OSAL_STATUS eOsStatus;

  /* Create USBD tx thread */
  g_usbd_tx_task_attributes.pThreadFunc = usbd_tx_task;
  g_usbd_tx_task_attributes.nPriority = APP_OS_CFG_USBD_TX_TASK_PRIO;
  g_usbd_tx_task_attributes.pStackBase = &ga_usbd_tx_task_stack[0];
  g_usbd_tx_task_attributes.nStackSize = APP_OS_CFG_USBD_TX_TASK_STK_SIZE;
  g_usbd_tx_task_attributes.pTaskAttrParam = NULL;
  g_usbd_tx_task_attributes.szThreadName = "tx_USBD";
  g_usbd_tx_task_attributes.pThreadTcb = &g_usbd_tx_task_tcb;
  /* TODO: The Queue ptr holding size increased as 250 from 25
     in-order to get all sterams. The count can be decreased based on the
     calculations*/
  eOsStatus = adi_osal_MsgQueueCreate(&gh_usbd_tx_task_msg_queue, NULL, 250);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
    Debug_Handler();
  } else {
    update_task_queue_list(
        APP_OS_CFG_USBD_TX_TASK_INDEX, gh_usbd_tx_task_msg_queue);
  }
  eOsStatus = adi_osal_ThreadCreateStatic(
      &gh_usbd_tx_task_handler, &g_usbd_tx_task_attributes);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
    Debug_Handler();
  }

  /* Create usbd application thread */
  app_usbd_serial_num_generate();

  g_usbd_app_task_attributes.pThreadFunc = usbd_application_task;
  g_usbd_app_task_attributes.nPriority = APP_OS_CFG_USBD_APP_TASK_PRIO;
  g_usbd_app_task_attributes.pStackBase = &ga_usbd_app_task_stack[0];
  g_usbd_app_task_attributes.nStackSize = APP_OS_CFG_USBD_APP_TASK_STK_SIZE;
  g_usbd_app_task_attributes.pTaskAttrParam = NULL;
  g_usbd_app_task_attributes.szThreadName = "app_usbd_task";
  g_usbd_app_task_attributes.pThreadTcb = &g_usbd_app_task_tcb;

  eOsStatus = adi_osal_ThreadCreateStatic(
      &gh_usbd_app_task_handler, &g_usbd_app_task_attributes);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
    Debug_Handler();
  }

  eOsStatus = adi_osal_MutexCreate(&g_usb_transfer_lock);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
    Debug_Handler();
  }

  eOsStatus = adi_osal_SemCreate(&g_usb_wr_sync, 0U);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
    Debug_Handler();
  }

  eOsStatus = adi_osal_SemCreate(&g_usbd_tx_task_evt_sem, 0);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
    Debug_Handler();
  }
}

/*!
 ****************************************************************************
 * @brief Function to get the PHY corresponding to m2m2 addr, from USBD routing
 * table
 ******************************************************************************/
static M2M2_PHY_ENUM_t get_usbd_routing_table_entry(M2M2_ADDR_ENUM_t addr) {
  for (int i = 0; i < USBD_ROUTING_TABLE_SIZE; i++) {
    if (usbd_routing_table[i].address == addr) {
      return usbd_routing_table[i].phy;
    }
  }
  return M2M2_PHY_DEFAULT;
}

/*!
 ****************************************************************************
 * @brief Function to get g_usb_tx_pending_flag flag status - to know whether
 * USBD Tx done or not
 ******************************************************************************/
uint8_t get_usbd_tx_pending_status(void) { return (g_usb_tx_pending_flag); }
uint32_t total_rx_cnt = 0;

/*< Variable to hold FS log chunk pkt transferred via USB Tx, for log download
 */
volatile uint8_t gFsDownloadChunkCnt = 0;
/*< Variable to identify if USB Tx is doing FS log download */
volatile uint8_t gFsDownloadFlag = 0;
/*< Variable to hold the total pkt transferred via USB Tx, for log download */
uint8_t total_packet_cnt_transferred = 0;

/*!
 ****************************************************************************
 * @brief Function to control gFsDownloadFlag flag from FS task
 ******************************************************************************/
uint8_t setFsDownloadFlag(uint8_t flag) {
  gFsDownloadFlag = flag;
  if (!flag)
    gFsDownloadChunkCnt = 0;
  return 0;
}


uint8_t get_file_download_chunk_count()
{
 return(gFsDownloadChunkCnt);
}

/**
  * @brief  USBD tx task - control trasmission through USB peripheral
  * @param  pArgument not used
  * @return None
  */
#ifdef PROFILE_TIME_ENABLED
uint32_t usb_cdc_end_time, usb_avg_cdc_write_time,usb_cdc_start_time;
uint32_t usb_min_cdc_time,usb_max_cdc_time;
uint32_t usb_min_tx_time,usb_max_tx_time;
uint32_t usb_tx_time_end, usb_avg_tx_time,usb_tx_time_start;
uint32_t max_num_of_retries;
#endif

uint32_t num_bytes_transferred=0;
uint8_t num_retry=0;
uint8_t usb_cdc_write_failed=0;

#ifdef DCB
/*!
 ****************************************************************************
 * @brief Funtion to update the status of all DCB blocks if present/not
 *
 * @details This function is called after FDS init on bootup, to update DCB
 * present flags, based on what DCB blocks are present/absent Later the flags
 * will be updated on every DCB write/erase
 *
 * @param None
 ******************************************************************************/
void dcb_block_status_update() {
#ifdef LOW_TOUCH_FEATURE
  gen_blk_update_dcb_present_flag();
  lt_app_lcfg_update_dcb_present_flag();
#endif
  adpd4000_update_dcb_present_flag();
  adxl_update_dcb_present_flag();
#ifdef ENABLE_PPG_APP
  ppg_update_dcb_present_flag();
#endif
#ifdef ENABLE_ECG_APP
  ecg_update_dcb_present_flag();
#endif
#ifdef ENABLE_EDA_APP
  eda_update_dcb_present_flag();
#endif
  ad7156_update_dcb_present_flag();
}
#endif

/*!
 ****************************************************************************
 * @brief  Reset the msg queue of USBD tx transmit task
 * @param  None
 * @return None
 ******************************************************************************/
void usbd_tx_msg_queue_reset()
{
  xQueueReset( gh_usbd_tx_task_msg_queue );
}

/* Debug variable to count total USB Tx pkts transferred */
static uint32_t gn_num_pkt_submit_cnt = 0;
/*< 'ind' variable to track how many times app_usbd_cdc_acm_write() api was
 * called for retry on failure */
uint8_t ind = 0;
/*< 'write_retry_cnt' variable to hold max retries allowed when
 * app_usbd_cdc_acm_write() api failes */
uint8_t write_retry_cnt = 6;

uint8_t usb_success = 0;
uint32_t submission_success_cnt = 0;
uint32_t interrupt_success_cnt = 0;
#ifdef PROFILE_TIME_ENABLED
uint32_t usb_cdc_end_time, usb_avg_cdc_write_time, usb_cdc_start_time;
uint32_t usb_min_cdc_time, usb_max_cdc_time;
uint32_t usb_min_tx_time, usb_max_tx_time;
uint32_t usb_tx_time_end, usb_avg_tx_time, usb_tx_time_start;
uint32_t num_bytes_transferred;
uint32_t max_num_of_retries;
#endif
/*!
 ****************************************************************************
 * @brief  USBD transmit task - handle post office messages and control
 * trasmission through USB peripheral
 * @param  pArgument not used
 * @return None
 ******************************************************************************/
static void usbd_tx_task(void *pArgument) {
  m2m2_hdr_t *pkt;
  M2M2_ADDR_ENUM_t destination;
  static uint8_t usbd_tx_buf[2064];
  uint32_t msg_len;
  ADI_OSAL_STATUS eOsStatus;

  /* Doing FDS init and All DCB Block status update from FreeRTOS task context
   Doing it from USB Tx task, since it has highest priority */
  fds_rtc_init();
  dcb_block_status_update();

  /* Suspend USBD tx task */
  gn_usbd_tx_task_suspended = 1;
  vTaskSuspend(NULL);

  while (1) {
    adi_osal_SemPend(g_usbd_tx_task_evt_sem, ADI_OSAL_TIMEOUT_FOREVER);
    gn_usbd_tx_task_suspended = 0;
    if (gb_usb_status == USB_PORT_OPENED) {
      pkt = post_office_get(
          ADI_OSAL_TIMEOUT_NONE, APP_OS_CFG_USBD_TX_TASK_INDEX);
      if (pkt == NULL) {
        continue;
      } else {
        msg_len = pkt->length;
        destination = pkt->dest;

        switch (get_usbd_routing_table_entry(destination)) {
        case M2M2_PHY_USB: {
#ifdef COLLISION_DEBUG
             if((pkt->dest == M2M2_ADDR_APP_CLI) && (pkt->length == 14))
                 gPingRespCnt++;
#endif //COLLISION_DEBUG
             pkt->src = (M2M2_ADDR_ENUM_t)BYTE_SWAP_16(pkt->src);
             pkt->dest = (M2M2_ADDR_ENUM_t)BYTE_SWAP_16(pkt->dest);
             pkt->length = BYTE_SWAP_16(pkt->length);
             pkt->checksum = BYTE_SWAP_16(pkt->checksum);
             memcpy(usbd_tx_buf,(uint8_t *)pkt,msg_len);
             gn_num_pkt_submit_cnt++;

             // Swap the header around so that the length can be properly freed.
             pkt->length = BYTE_SWAP_16(pkt->length);

             g_usb_tx_pending_flag = 1;
             if(gFsDownloadFlag)
             {
                gFsDownloadChunkCnt++;
                total_packet_cnt_transferred++;
             }
             ind =0;
             ret_code_t ret = NRF_SUCCESS;
             num_retry = 0;
             do{
               eOsStatus = adi_osal_MutexPend(g_usb_transfer_lock, ADI_OSAL_TIMEOUT_FOREVER);
               if (eOsStatus != ADI_OSAL_SUCCESS)
               {
                  Debug_Handler();
               }
               usb_success = 0;
               NRF_LOG_INFO("transaction status before call= %d",usb_success);
#if PROFILE_TIME_ENABLED
               usb_cdc_start_time = get_micro_sec();
#endif
              ret = app_usbd_cdc_acm_write(&m_app_cdc_acm,usbd_tx_buf,msg_len);

              /* increment bytes transferred only for fs stream packet */
              if(((M2M2_ADDR_ENUM_t)BYTE_SWAP_16(pkt->src)) == M2M2_ADDR_SYS_FS_STREAM){
                if(num_retry == 0){
                  num_bytes_transferred += msg_len;
                  num_retry = 1;
                }
              }
              eOsStatus = adi_osal_MutexPost(g_usb_transfer_lock);
              if (eOsStatus != ADI_OSAL_SUCCESS)
              {
                Debug_Handler();
              }
              if(ret != NRF_SUCCESS)
              {
                NRF_LOG_INFO("CDC ACM unavailable, data received: %s", usbd_tx_buf);
                MCU_HAL_Delay(4);   /*TODO: need to check why the usb busy comes*/
              }
              else
              {
                submission_success_cnt += 1;
                NRF_LOG_INFO("Successfull submission cnt= %d",submission_success_cnt);
#if PROFILE_TIME_ENABLED
                usb_cdc_end_time = get_micro_sec();
                usb_avg_cdc_write_time += (usb_cdc_end_time - usb_cdc_start_time);

                if ((usb_cdc_end_time - usb_cdc_start_time) < usb_min_cdc_time)
                  usb_min_cdc_time = (usb_cdc_end_time - usb_cdc_start_time);

                if ((usb_cdc_end_time - usb_cdc_start_time) > usb_max_cdc_time)
                  usb_max_cdc_time = (usb_cdc_end_time - usb_cdc_start_time);

                usb_tx_time_start = get_micro_sec();
#endif
              /* Block until transfer is completed */
              eOsStatus = adi_osal_SemPend(g_usb_wr_sync, ADI_OSAL_TIMEOUT_FOREVER);
              if (eOsStatus != ADI_OSAL_SUCCESS) {
                Debug_Handler();
              }
#if PROFILE_TIME_ENABLED
              usb_tx_time_end = get_micro_sec();
              usb_avg_tx_time += (usb_tx_time_end - usb_tx_time_start);

              if ((usb_tx_time_end - usb_tx_time_start) < usb_min_tx_time)
                usb_min_tx_time = (usb_tx_time_end - usb_tx_time_start);

              if ((usb_tx_time_end - usb_tx_time_start) > usb_max_tx_time)
                usb_max_tx_time = (usb_tx_time_end - usb_tx_time_start);
#endif
              usb_success = 1;
              NRF_LOG_INFO("transaction status after call = %d", usb_success);
            }
            ind++;
#ifdef PROFILE_TIME_ENABLED
               if(ind > max_num_of_retries)
                max_num_of_retries = ind;
#endif
               }while((ind < write_retry_cnt) && (ret != NRF_SUCCESS));
                if(ret != NRF_SUCCESS){
                  usb_cdc_write_failed=1;
                }
                post_office_consume_msg(pkt);
                break;
         }
          default: {
          /* Something went horribly wrong, swallow the packet and carry on. */
          post_office_consume_msg(pkt);
          break;
         }
       }
     }
    }
    else if ((gb_usb_status == USB_PORT_CLOSED) ||
               (gb_usb_status == USB_DISCONNECTED)) {
      if (!gb_force_stream_stop) {
        gn_usbd_tx_task_suspended = 1;
        usbd_tx_msg_queue_reset();
        usb_pkt_src = M2M2_ADDR_UNDEFINED;
        vTaskSuspend(NULL); /* Suspend USB task */
      } else {
        /* Force-stop Sensor streaming that wasn't stopped */
        m2m2_hdr_t *req_pkt = NULL;
        ADI_OSAL_STATUS err;

        req_pkt = post_office_create_msg(
            M2M2_HEADER_SZ + sizeof(m2m2_pm_force_stream_stop_cmd_t));
        if (req_pkt != NULL) {

          m2m2_pm_force_stream_stop_cmd_t *force_stream_stop_req =
              (m2m2_pm_force_stream_stop_cmd_t *)&req_pkt->data[0];

          /* send REQ packet */
          req_pkt->src = usb_pkt_src;
          req_pkt->dest = M2M2_ADDR_SYS_PM;

          force_stream_stop_req->command =
              M2M2_PM_SYS_COMMAND_FORCE_STREAM_STOP_REQ;
          NRF_LOG_INFO("Sending Force stream stop cmd from USB");
          post_office_send(req_pkt, &err);
          gb_force_stream_stop = false;
          adi_osal_SemPost(g_usbd_tx_task_evt_sem);
        }
      }
    }
  }
}

static m2m2_hdr_t *gp_pkt_hdr;
#ifdef USE_USB_READ
static bool nRxLengthFlag = 0;
static uint16_t nRxLength = 8;
#endif //#USE_USB_READ
/*!
 ****************************************************************************
 * @brief User event handler @ref app_usbd_cdc_acm_user_ev_handler_t
 ******************************************************************************/
static void cdc_acm_user_ev_handler(
    app_usbd_class_inst_t const *p_inst, app_usbd_cdc_acm_user_event_t event) {
  static uint8_t m_cdc_data_array[BLE_NUS_MAX_DATA_LEN];
  app_usbd_cdc_acm_t const *p_cdc_acm = app_usbd_cdc_acm_class_get(p_inst);
  ADI_OSAL_STATUS eOsStatus;
  static uint16_t pkt_len = 0;
  static uint8_t pkt_complete = 1;

  switch (event) {
  case APP_USBD_CDC_ACM_USER_EVT_PORT_OPEN: {
    gb_usb_status = USB_PORT_OPENED;
    gb_force_stream_stop = false;
    adi_osal_ThreadResumeFromISR(gh_usbd_tx_task_handler);
    adi_osal_SemPost(g_usbd_tx_task_evt_sem);
    /* Set up the first transfer */
#ifdef USE_USB_READ
    if (!nRxLengthFlag) {
      /* Read the packet header */
      nRxLengthFlag = 1;
      nRxLength = 8;
      eOsStatus =
          adi_osal_MutexPend(g_usb_transfer_lock, ADI_OSAL_TIMEOUT_FOREVER);
      if (eOsStatus != ADI_OSAL_SUCCESS) {
        Debug_Handler();
      }
      app_usbd_cdc_acm_read(&m_app_cdc_acm, (m_cdc_data_array), nRxLength);
      eOsStatus = adi_osal_MutexPost(g_usb_transfer_lock);
      if (eOsStatus != ADI_OSAL_SUCCESS) {
        Debug_Handler();
      }
    }
    NRF_LOG_INFO("CDC ACM port opened");
#endif // Use app_usbd_cdc_acm_read API
#if USE_USB_READ_ANY
    eOsStatus =
        adi_osal_MutexPend(g_usb_transfer_lock, ADI_OSAL_TIMEOUT_FOREVER);
    if (eOsStatus != ADI_OSAL_SUCCESS) {
      Debug_Handler();
    }
    app_usbd_cdc_acm_read_any(
        &m_app_cdc_acm, (m_cdc_data_array), BLE_NUS_MAX_DATA_LEN);
    pkt_complete = 1;
    pkt_len = 0;
    eOsStatus = adi_osal_MutexPost(g_usb_transfer_lock);
    if (eOsStatus != ADI_OSAL_SUCCESS) {
      Debug_Handler();
    }
#endif // Use app_usbd_cdc_acm_read_any API
    break;
  }

  case APP_USBD_CDC_ACM_USER_EVT_PORT_CLOSE:
    NRF_LOG_INFO("CDC ACM port closed");
    gb_usb_status = USB_PORT_CLOSED;
    gb_force_stream_stop = true;
    adi_osal_SemPost(g_usbd_tx_task_evt_sem);
    break;

  case APP_USBD_CDC_ACM_USER_EVT_TX_DONE:
    interrupt_success_cnt += 1;
    NRF_LOG_INFO("ISR count = %d", interrupt_success_cnt);
    g_usb_tx_pending_flag = 0;
    eOsStatus = adi_osal_SemPost(g_usb_wr_sync);
    if (eOsStatus != ADI_OSAL_SUCCESS)
      Debug_Handler();
    break;

  case APP_USBD_CDC_ACM_USER_EVT_RX_DONE: {
#ifdef USE_USB_READ
    ret_code_t ret;
    static uint16_t index = 8;

    do {
      if (nRxLengthFlag) {
        /* The data received is the header */
        nRxLengthFlag = 0;
        nRxLength = (((uint8_t)m_cdc_data_array[4] << 8) +
                        (uint8_t)m_cdc_data_array[5]) -
                    8;
        index = 8;
      } else {
        /* The data received is the payload, (packet Length - 8(header length))
         * bytes */
        nRxLengthFlag = 1;
        nRxLength = 8;
        index = 0;
      }

      /*Get amount of data transferred*/
      static size_t size;
      static size_t loop_retry = 0;
      static size_t read_err = 0;
      size = app_usbd_cdc_acm_rx_size(p_cdc_acm);
      NRF_LOG_DEBUG(
          "RX: size: %lu char: %c", size, m_cdc_data_array[index - 1]);

      eOsStatus =
          adi_osal_MutexPend(g_usb_transfer_lock, ADI_OSAL_TIMEOUT_FOREVER);
      if (eOsStatus != ADI_OSAL_SUCCESS) {
        Debug_Handler();
      }
      /* Fetch data until internal buffer is empty,submit buffer for next read
       */
      ret = app_usbd_cdc_acm_read(
          &m_app_cdc_acm, &m_cdc_data_array[index], nRxLength);
      if (ret == NRF_SUCCESS) {
      } else {
        loop_retry++;
        read_err = ret;
      }
      eOsStatus = adi_osal_MutexPost(g_usb_transfer_lock);
      if (eOsStatus != ADI_OSAL_SUCCESS) {
        Debug_Handler();
      }
      UNUSED_PARAMETER(size);
      UNUSED_PARAMETER(read_err);
    } while (ret == NRF_SUCCESS);

    if (nRxLengthFlag) {
#if 0
                ADI_OSAL_STATUS osal_result;
                osal_result = adi_osal_MsgQueuePost(gh_usbd_rx_task_msg_queue,m_cdc_data_array);
                if (osal_result != ADI_OSAL_SUCCESS)
                  Debug_Handler();
#endif
      uint32_t data_len;
      m2m2_hdr_t *p_m2m2_ = NULL;
      ADI_OSAL_STATUS err;

      gp_pkt_hdr = (m2m2_hdr_t *)m_cdc_data_array;
      data_len = BYTE_SWAP_16(gp_pkt_hdr->length);

      p_m2m2_ = post_office_create_msg(data_len);
      if (p_m2m2_ == NULL) {
        // continue;
      }
      memcpy((uint8_t *)p_m2m2_, m_cdc_data_array, data_len);

      /* swap from network byte order to little endian */
      p_m2m2_->src = (M2M2_ADDR_ENUM_t)BYTE_SWAP_16(p_m2m2_->src);
      p_m2m2_->dest = (M2M2_ADDR_ENUM_t)BYTE_SWAP_16(p_m2m2_->dest);
      p_m2m2_->length = BYTE_SWAP_16(p_m2m2_->length);
      p_m2m2_->checksum = BYTE_SWAP_16(p_m2m2_->checksum);
#ifdef COLLISION_DEBUG
      if ((p_m2m2_->dest == M2M2_ADDR_SYS_PM) && (p_m2m2_->length == 14))
        gPingReqCnt++;
#endif // COLLISION_DEBUG
      post_office_send(p_m2m2_, &err);
    }
#endif // Use app_usbd_cdc_acm_read API
#ifdef USE_USB_READ_ANY
    ret_code_t ret;
#if 0
            ADI_OSAL_STATUS osal_result;
            osal_result = adi_osal_MsgQueuePost(gh_usbd_rx_task_msg_queue,m_cdc_data_array);
            if (osal_result != ADI_OSAL_SUCCESS)
              Debug_Handler();
#endif
            /*Get amount of data transferred*/
            static size_t size=0;
            size += app_usbd_cdc_acm_rx_size(p_cdc_acm);
            NRF_LOG_INFO("RX: size: %lu char: %s", size, m_cdc_data_array);

            if(pkt_complete)
            {
               gp_pkt_hdr = (m2m2_hdr_t *)m_cdc_data_array;
               pkt_len = BYTE_SWAP_16(gp_pkt_hdr->length);
               if(pkt_len>BLE_NUS_MAX_DATA_LEN)
               {
                 NRF_LOG_INFO("USB RX: received incorrect pkt");
                 pkt_len=0;//reset
                 pkt_complete=1;//reset
                 size=0;
                 return;
               }
               //ASSERT(pkt_len<=BLE_NUS_MAX_DATA_LEN);
            }

            if(pkt_len==size)
            {
              pkt_complete=1;
              size=0;

              m2m2_hdr_t *p_m2m2_ = NULL;
              ADI_OSAL_STATUS       err;

              p_m2m2_ = post_office_create_msg(pkt_len);
              if (p_m2m2_ == NULL) {
                //continue;
                break;
              }
              memcpy((uint8_t *)p_m2m2_, m_cdc_data_array, pkt_len);

              /* swap from network byte order to little endian */
              p_m2m2_->src = (M2M2_ADDR_ENUM_t)BYTE_SWAP_16(p_m2m2_->src);
              p_m2m2_->dest = (M2M2_ADDR_ENUM_t)BYTE_SWAP_16(p_m2m2_->dest);
              p_m2m2_->length = BYTE_SWAP_16(p_m2m2_->length);
              p_m2m2_->checksum = BYTE_SWAP_16(p_m2m2_->checksum);
              if(M2M2_ADDR_UNDEFINED == usb_pkt_src)
                  usb_pkt_src = p_m2m2_->src;
            /* send to po for proper routing */
            post_office_send(p_m2m2_, &err);
            }
            else
              pkt_complete=0;

    do {

      static size_t loop_retry = 0;
      static size_t read_err = 0;

      eOsStatus =
          adi_osal_MutexPend(g_usb_transfer_lock, ADI_OSAL_TIMEOUT_FOREVER);
      if (eOsStatus != ADI_OSAL_SUCCESS) {
        Debug_Handler();
      }
      ret = app_usbd_cdc_acm_read_any(
          &m_app_cdc_acm, (m_cdc_data_array + size), BLE_NUS_MAX_DATA_LEN);
      eOsStatus = adi_osal_MutexPost(g_usb_transfer_lock);
      if (eOsStatus != ADI_OSAL_SUCCESS) {
        Debug_Handler();
      }
      if (ret == NRF_SUCCESS) {
      } else {
        loop_retry++;
        read_err = ret;
      }
      UNUSED_PARAMETER(size);
      UNUSED_PARAMETER(read_err);
    } while (ret == NRF_SUCCESS);
#if 0
            uint32_t  data_len;
            m2m2_hdr_t *p_m2m2_ = NULL;
            ADI_OSAL_STATUS       err;

            gp_pkt_hdr = (m2m2_hdr_t *)m_cdc_data_array;
            data_len = BYTE_SWAP_16(gp_pkt_hdr->length);

            p_m2m2_ = post_office_create_msg(data_len);
            if (p_m2m2_ == NULL) {
              //continue;
            }
            memcpy((uint8_t *)p_m2m2_, m_cdc_data_array, data_len);

            /* swap from network byte order to little endian */
            p_m2m2_->src = (M2M2_ADDR_ENUM_t)BYTE_SWAP_16(p_m2m2_->src);
            p_m2m2_->dest = (M2M2_ADDR_ENUM_t)BYTE_SWAP_16(p_m2m2_->dest);
            p_m2m2_->length = BYTE_SWAP_16(p_m2m2_->length);
            p_m2m2_->checksum = BYTE_SWAP_16(p_m2m2_->checksum);
#ifdef COLLISION_DEBUG
            if((p_m2m2_->dest == M2M2_ADDR_SYS_PM) && (p_m2m2_->length == 14))
                gPingReqCnt++;
#endif // COLLISION_DEBUG
            post_office_send(p_m2m2_, &err);

            do {
                /*Get amount of data transferred*/
                static size_t size;
                static size_t  loop_retry=0;
                static size_t  read_err=0;
                size = app_usbd_cdc_acm_rx_size(p_cdc_acm);
                NRF_LOG_DEBUG("RX: size: %lu char: %s", size, m_cdc_data_array);
                eOsStatus = adi_osal_MutexPend(g_usb_transfer_lock, ADI_OSAL_TIMEOUT_FOREVER);
                if (eOsStatus != ADI_OSAL_SUCCESS) {
                    Debug_Handler();
                }
                ret = app_usbd_cdc_acm_read_any(&m_app_cdc_acm, (m_cdc_data_array), BLE_NUS_MAX_DATA_LEN);
                eOsStatus = adi_osal_MutexPost(g_usb_transfer_lock);
                if (eOsStatus != ADI_OSAL_SUCCESS) {
                    Debug_Handler();
                }
                if (ret == NRF_SUCCESS)
                  {
                  }
                  else {
                   loop_retry++;
                   read_err = ret;
                  }
                UNUSED_PARAMETER(size);
                UNUSED_PARAMETER(read_err);
            }
            while (ret == NRF_SUCCESS);
#endif // Use app_usbd_cdc_acm_read_any API
#endif
    break;
  }

  default:
    break;
  }
}


bool get_usb_powered_event_status()
{
  return(g_usb_power_detected_flag);
}

void reset_usb_powered_event_status()
{
  g_usb_power_detected_flag = false;
}

/*!
 ****************************************************************************
 * @brief User event handler for USBD application task
 ******************************************************************************/
static void usbd_user_ev_handler(app_usbd_event_type_t event) {
  switch (event) {
  case APP_USBD_EVT_DRV_SUSPEND:
    break;

  case APP_USBD_EVT_DRV_RESUME:
    break;

  case APP_USBD_EVT_STARTED:
    break;

  case APP_USBD_EVT_STOPPED:
    app_usbd_disable();
    break;

  case APP_USBD_EVT_POWER_DETECTED: {
    // usb_pwm_reset_init();
    NRF_LOG_INFO("USB power detected");

    if (!nrf_drv_usbd_is_enabled()) {
      app_usbd_enable();
    }
#ifdef ENABLE_WATCH_DISPLAY 
    /* Added gLowTouchRunning flag checking, in the below condition, to check
       when to give the StopLog sequence.
       Without this, if the USB cable is plugged in fast enough,(before LT logging actually starts),
       then the Stop Cmds would be missed to be send and the rest of the flags gets cleared.
    */
    if(get_low_touch_trigger_mode2_status() && get_lt_mode2_selection_status()
       && gLowTouchRunning)
    {
        reset_lt_mode2_selection_status();    /*reset the LT mode2 log selection status*/
        g_usb_power_detected_flag = true;
        adi_osal_SemPost(lt_task_evt_sem);
    }
#endif
  } break;

  case APP_USBD_EVT_POWER_REMOVED: {
    // usb_pwm_reset_uninit();
    NRF_LOG_INFO("USB power removed");
#ifdef HIBERNATE_MD_EN
    hibernate_md_set(HIB_MD_USB_DISCONNECT_EVT);
    hibernate_mode_entry();
#endif
    gb_usb_status = USB_DISCONNECTED;
    gb_force_stream_stop = true;
    app_usbd_stop();
    adi_osal_SemPost(g_usbd_tx_task_evt_sem);
  } break;

  case APP_USBD_EVT_POWER_READY: {
    NRF_LOG_INFO("USB ready");
#ifdef HIBERNATE_MD_EN
    hibernate_md_clear(HIB_MD_USB_DISCONNECT_EVT);
    hibernate_mode_entry();
#endif
    gb_usb_status = USB_CONNECTED;
    gb_force_stream_stop = false;
    app_usbd_start();
  } break;

  default:
    break;
  }
}

/*!
 ****************************************************************************
 * @brief Event handler for USBD application task ISR events
 ******************************************************************************/
void usb_new_event_isr_handler(
    app_usbd_internal_evt_t const *const p_event, bool queued) {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  UNUSED_PARAMETER(p_event);
  UNUSED_PARAMETER(queued);
  ASSERT(gh_usbd_app_task_handler != NULL);
  /* Release the semaphore */
  vTaskNotifyGiveFromISR(
      (TaskHandle_t)gh_usbd_app_task_handler, &xHigherPriorityTaskWoken);
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/*!
 ****************************************************************************
 * @brief  USBD application task - handle usbd application events -user and isr
 * events
 * @param  pArgument not used
 * @return None
 ******************************************************************************/
static void usbd_application_task(void *pArgument) {
  ret_code_t ret;

  static const app_usbd_config_t usbd_config = {
      .ev_isr_handler = usb_new_event_isr_handler,
      .ev_state_proc = usbd_user_ev_handler};
  UNUSED_PARAMETER(pArgument);

  ret = app_usbd_init(&usbd_config);
  APP_ERROR_CHECK(ret);
  app_usbd_class_inst_t const *class_cdc_acm =
      app_usbd_cdc_acm_class_inst_get(&m_app_cdc_acm);
  ret = app_usbd_class_append(class_cdc_acm);
  APP_ERROR_CHECK(ret);
  ret = app_usbd_power_events_enable();
  APP_ERROR_CHECK(ret);

  /* Set the first event to make sure that USB queue is processed after it is
   started */
  UNUSED_RETURN_VALUE(xTaskNotifyGive(xTaskGetCurrentTaskHandle()));
  /* Enter main loop. */
  for (;;) {
    /* Waiting for event */
    UNUSED_RETURN_VALUE(ulTaskNotifyTake(pdTRUE, USB_THREAD_MAX_BLOCK_TIME));
    while (app_usbd_event_queue_process()) {
      /* Nothing to do */
    }
  }
}

/*@}*/  /* end of group Application_Task */
/**@}*/ /* end of group Tasks */
