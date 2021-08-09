/**
***************************************************************************
* @addtogroup Tasks
* @{
* @file         file_system_task.c
* @author       ADI
* @version      V1.0.1
* @date         16-June-2020
* @brief        Source file contains file system access for
                wearable device framework.
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

/* Includes -----------------------------------------------------------------*/
#ifdef USE_FS
#include <task_includes.h>
#include <includes.h>
#include <file_system_interface.h>
#include <led_interface.h>
//#include <clk.h>
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "hw_if_config.h"
#include "app_timer.h"
#include "power_manager.h"

#ifdef PROFILE_TIME_ENABLED
#include "us_tick.h"
#endif

#ifdef VSM_MBOARD
#include "watch_vsm_motherboard_pin_config.h"
#elif PCBA
#include "watch_board_pcba_pin_config.h"
#else
#include "watch_board_evt_pin_config.h"
#endif

#include <file_system_utils.h>
#include <file_system_task.h>
#include <system_interface.h>
#include <system_task.h>
#include <low_touch_task.h>
#include <sensor_adpd_application_interface.h>
#include <sensor_adxl_application_interface.h>
#include <ppg_application_interface.h>
#include <ecg_application_interface.h>
#include <eda_application_interface.h>
#include <pedometer_application_interface.h>
#ifdef LOW_TOUCH_FEATURE
#include "dcb_general_block.h"
#endif
#include <sqi_application_interface.h>
#include "display_app.h"
#include "light_fs.h"

/* subscriber backup */
static M2M2_ADDR_ENUM_t SubsGlobalSrc = M2M2_ADDR_UNDEFINED;
/* filesrc backup */
static M2M2_ADDR_ENUM_t FileSrc = M2M2_ADDR_UNDEFINED;
/* Stop source backup */
static FILE_STOP_LOGGING_t StopSrc = M2M2_FILE_SYS_STOP_LOGGING_INVALID;
void file_system_task(void *pArgument);
uint8_t get_file_system_subs_count(void);
bool fs_unsubscribe_streams(void);
static void fs_send_write_error(M2M2_ADDR_ENUM_t *src);
static void fs_stream_usubscribe();
extern m2m2_pm_sys_info_t g_system_info;
#define NAND_READ_TEST
#define NAND_WRITE_TEST
#undef NAND_WRITE_TEST
#undef NAND_READ_TEST
#ifdef NAND_READ_TEST
static uint16_t g_read_time = 0;
static uint64_t g_file_read_time=0;
static uint32_t g_file_read_cnt = 0;
#endif /* NAND_READ_TEST */

#ifdef NAND_WRITE_TEST
static uint32_t g_write_time = 0;
static uint64_t g_file_write_time = 0;
#endif

static uint32_t n500bChunkCnt = 0;
extern uint8_t ConfigFileName[16];
static char PatternFileName[16] = "PATTERN.LOG";

/* If an application is not found in the routing table,
    then this default ADDR_UNDEFINED is used. */
#define M2M2_ADDRESS_DEFAULT  M2M2_ADDR_UNDEFINED
#define FS_ECG_APP_MAX_LCFG_OPS (4)

/* Size of flash in bytes = 2048 * 64 * 4096 */
#define FLASH_SIZE                      536870912
                   
/* Total num of pages in Flash */
#define TOTAL_NUM_OF_PAGES              (FLASH_SIZE/PAGE_SIZE)
/* ADI_FS_FILE_MAX_SIZE is the no of pages allocated
   for data files = 2044 * 64 */
#define ADI_FS_FILE_MAX_SIZE            130816

#ifdef PATTERN_WRITE_ENABLED
static volatile uint32_t gs2kChunkCnt = 0;
static uint8_t pArr[PAGE_SIZE];
#endif
static volatile bool gMemoryFull = false;
uint32_t nTick = 0;
extern uint64_t sum;
static uint8_t gnadpd_dcfg_tx_PacketCount = 0;

#define ADI_FS_STREAM_STARTED   0x01
#define ADI_FS_STREAM_STOPPED   0x00

uint8_t ga_fs_task_stack[APP_OS_CFG_FS_TASK_STK_SIZE];
ADI_OSAL_THREAD_HANDLE gh_fs_task_handler;
ADI_OSAL_STATIC_THREAD_ATTR g_fs_task_attributes;
StaticTask_t g_fs_task_tcb;
ADI_OSAL_QUEUE_HANDLE  gh_fs_task_msg_queue = NULL;

volatile uint8_t fs_download_total_pkt_cnt = 0;
extern uint8_t total_packet_cnt_transferred;
#ifdef ENABLE_WATCH_DISPLAY
extern uint16_t min_timer_cnt;
extern uint16_t fs_display_query_cnt;
#endif
uint8_t fs_status_stored = 0;
uint32_t num_bytes_processed = 0;

#ifdef LOW_TOUCH_FEATURE
extern uint8_t gSendRespToPmApp;
extern uint8_t ConfigFileName[16];
#endif /* LOW_TOUCH_FEATURE */

#include "nrf_log_ctrl.h"
#define NRF_LOG_MODULE_NAME FS_TASK

#if FS_TASK_CONFIG_LOG_ENABLED
#ifdef NRF_LOG_DEFAULT_LEVEL
#undef NRF_LOG_DEFAULT_LEVEL
#define NRF_LOG_DEFAULT_LEVEL FS_TASK_CONFIG_LOG_LEVEL
#endif
#define NRF_LOG_INFO_COLOR  FS_TASK_CONFIG_INFO_COLOR
#define NRF_LOG_DEBUG_COLOR FS_TASK_CONFIG_DEBUG_COLOR
#else /* FS_TASK_CONFIG_LOG_ENABLED */
#define NRF_LOG_LEVEL       0
#endif /* FS_TASK_CONFIG_LOG_ENABLED */

#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

ADI_OSAL_SEM_HANDLE   qspi_task_evt_sem;
/* semaphore handler for low touch task */
extern ADI_OSAL_SEM_HANDLE   lt_task_evt_sem;

extern uint8_t setFsDownloadFlag(uint8_t flag);
extern uint8_t get_file_download_chunk_count();
#ifdef ENABLE_WATCH_DISPLAY
extern void reset_display_vol_info();
#endif
extern volatile uint8_t gsCfgFileFoundFlag;
#ifdef PROFILE_TIME_ENABLED
extern uint32_t num_bytes_transferred,usb_avg_tx_time,usb_avg_cdc_write_time;
extern uint32_t min_file_read_time,avg_file_read_time,max_file_read_time;
extern uint16_t num_times_read;
extern uint32_t usb_min_cdc_time,usb_max_cdc_time;
extern uint32_t usb_min_tx_time,usb_max_tx_time;
uint32_t delay_time, total_time_taken_512_bytes_transfer;
extern uint32_t max_num_of_retries;
#endif

/*!
* @brief:  Data structure to define routing table entry in the filesystem
*/
typedef struct _file_routing_table_entry {
  M2M2_ADDR_ENUM_t address;
  M2M2_ADDR_ENUM_t stream;
  FILE_SYS_STREAM_SUBS_STATE_ENUM_t fs_subs_state;
}file_routing_table_entry;

/*!
* @brief:  Data structure used for file download
*/
struct _fs_stream {
  FS_STATUS_ENUM_t fs_err_status;
  char file_path[16];
  uint32_t fs_buffer_size;
  uint32_t processed_data_len;
  uint32_t data_chunk_len;
  uint32_t num_bytes_left;
  uint8_t  file_buff[8*MEMBER_SIZE(m2m2_file_sys_download_log_stream_t, byte_stream)];
  M2M2_ADDR_ENUM_t pkt_src;
  M2M2_ADDR_ENUM_t pkt_dest;
}fs_stream;


/*!
* @brief:  Array used for checking the subscription of a stream to file system
*/
static file_routing_table_entry file_routing_table[] = {
  {M2M2_ADDR_SENSOR_ADPD4000,       M2M2_ADDR_SENSOR_ADPD_STREAM1,        M2M2_FILE_SYS_UNSUBSCRIBED},
  {M2M2_ADDR_SENSOR_ADPD4000,       M2M2_ADDR_SENSOR_ADPD_STREAM2,        M2M2_FILE_SYS_UNSUBSCRIBED},
  {M2M2_ADDR_SENSOR_ADPD4000,       M2M2_ADDR_SENSOR_ADPD_STREAM3,        M2M2_FILE_SYS_UNSUBSCRIBED},
  {M2M2_ADDR_SENSOR_ADPD4000,       M2M2_ADDR_SENSOR_ADPD_STREAM4,        M2M2_FILE_SYS_UNSUBSCRIBED},
  {M2M2_ADDR_SENSOR_ADPD4000,       M2M2_ADDR_SENSOR_ADPD_STREAM5,        M2M2_FILE_SYS_UNSUBSCRIBED},
  {M2M2_ADDR_SENSOR_ADPD4000,       M2M2_ADDR_SENSOR_ADPD_STREAM6,        M2M2_FILE_SYS_UNSUBSCRIBED},
  {M2M2_ADDR_SENSOR_ADPD4000,       M2M2_ADDR_SENSOR_ADPD_STREAM7,        M2M2_FILE_SYS_UNSUBSCRIBED},
  {M2M2_ADDR_SENSOR_ADPD4000,       M2M2_ADDR_SENSOR_ADPD_STREAM8,        M2M2_FILE_SYS_UNSUBSCRIBED},
  {M2M2_ADDR_SENSOR_ADPD4000,       M2M2_ADDR_SENSOR_ADPD_STREAM9,        M2M2_FILE_SYS_UNSUBSCRIBED},
  {M2M2_ADDR_SENSOR_ADXL,           M2M2_ADDR_SENSOR_ADXL_STREAM,         M2M2_FILE_SYS_UNSUBSCRIBED},
#ifdef ENABLE_PPG_APP
  {M2M2_ADDR_MED_PPG,               M2M2_ADDR_MED_PPG_STREAM,             M2M2_FILE_SYS_UNSUBSCRIBED},
  {M2M2_ADDR_MED_PPG,               M2M2_ADDR_SYS_AGC_STREAM,             M2M2_FILE_SYS_UNSUBSCRIBED},
  {M2M2_ADDR_MED_PPG,               M2M2_ADDR_SYS_HRV_STREAM,             M2M2_FILE_SYS_UNSUBSCRIBED},
  {M2M2_ADDR_MED_SYNC_ADPD_ADXL,    M2M2_ADDR_MED_SYNC_ADPD_ADXL_STREAM,  M2M2_FILE_SYS_UNSUBSCRIBED},
#endif
#ifdef ENABLE_PEDO_APP
  {M2M2_ADDR_MED_PED,               M2M2_ADDR_MED_PED_STREAM,             M2M2_FILE_SYS_UNSUBSCRIBED},
#endif
#ifdef ENABLE_EDA_APP
  {M2M2_ADDR_MED_EDA,               M2M2_ADDR_MED_EDA_STREAM,             M2M2_FILE_SYS_UNSUBSCRIBED},
#endif
#ifdef ENABLE_ECG_APP
  {M2M2_ADDR_MED_ECG,               M2M2_ADDR_MED_ECG_STREAM,             M2M2_FILE_SYS_UNSUBSCRIBED},
#endif
#ifdef ENABLE_TEMPERATURE_APP
  {M2M2_ADDR_MED_TEMPERATURE,       M2M2_ADDR_MED_TEMPERATURE_STREAM,     M2M2_FILE_SYS_UNSUBSCRIBED},
#endif
#ifdef ENABLE_BCM_APP
  {M2M2_ADDR_MED_BCM,               M2M2_ADDR_MED_BCM_STREAM,             M2M2_FILE_SYS_UNSUBSCRIBED},
#endif
#ifdef ENABLE_SQI_APP
  {M2M2_ADDR_MED_SQI,               M2M2_ADDR_MED_SQI_STREAM,             M2M2_FILE_SYS_UNSUBSCRIBED},
#endif
  {M2M2_ADDR_APP_CLI,               M2M2_ADDR_APP_CLI_STREAM,             M2M2_FILE_SYS_UNSUBSCRIBED},
  {M2M2_ADDR_APP_IOS,               M2M2_ADDR_APP_IOS_STREAM,             M2M2_FILE_SYS_UNSUBSCRIBED},
  {M2M2_ADDR_APP_WT,                M2M2_ADDR_APP_WT_STREAM,              M2M2_FILE_SYS_UNSUBSCRIBED},
  {M2M2_ADDR_APP_DROID,             M2M2_ADDR_APP_DROID_STREAM,           M2M2_FILE_SYS_UNSUBSCRIBED},
  {M2M2_ADDR_SYS_PM,                M2M2_ADDR_SYS_BATT_STREAM,            M2M2_FILE_SYS_UNSUBSCRIBED},
};

/* Number of entries in the routing table */
#define FILE_ROUTING_TABLE_SIZE (sizeof(file_routing_table)/sizeof(file_routing_table[0]))

/* Number of adpd streams in routing table */
#define MAX_NUMBER_OF_ADPD_STREAM (M2M2_ADDR_SENSOR_ADPD_STREAM9 - M2M2_ADDR_SENSOR_ADPD_STREAM1)
/*!
  ****************************************************************************
    @brief Finds the number of streams subscribed to file system

    @param[in]                 - None

    @return                    count: No of streams subscribed to file system
*****************************************************************************/
uint8_t get_file_system_subs_count(void) {
  uint8_t count = 0;
  for(int i = 0; i < FILE_ROUTING_TABLE_SIZE; i++) {
    if(file_routing_table[i].fs_subs_state == M2M2_FILE_SYS_SUBSCRIBED)
      count++;
  }
  return count;
}

/* FS task priority used while downloading the logs from the flash
  For proper functioning of fs_stream command, FS TASK should have
  priority above USB TX task */
#define FS_APP_DOWNLOAD_PRIO  APP_OS_CFG_USBD_TX_TASK_PRIO+1
uint8_t gsFsDownloadReqByExtTool = 0;
extern uint32_t TimeOutCount;


/*!
  ****************************************************************************
    @brief Changes the priority fs task above the usb tx task

    @param[in]                    - None

    @return ADI_OSAL_SUCCESS      - If successfully changed the priority of the
                                    specified thread
    @return ADI_OSAL_BAD_PRIORITY - If the specified priority is invalid
    @return ADI_OSAL_CALLER_ERROR - If the function is invoked from an invalid
                                    location (i.e an ISR)
    @return ADI_OSAL_BAD_HANDLE   -  If the specified thread handle is invalid
    @see adi_osal_ThreadGetPrio
*****************************************************************************/
ADI_OSAL_STATUS hike_fs_prio(void)
{
  return(adi_osal_ThreadSetPrio(gh_fs_task_handler,FS_APP_DOWNLOAD_PRIO));
}

/*!
  ****************************************************************************
    @brief Reverts the FS priority which changed during download (fs_stream)
           operation to its initial priority level

    @param[in]                    - None

    @return ADI_OSAL_SUCCESS      - If successfully changed the priority of the
                                    specified thread
    @return ADI_OSAL_BAD_PRIORITY - If the specified priority is invalid
    @return ADI_OSAL_CALLER_ERROR - If the function is invoked from an invalid
                                    location (i.e an ISR)
    @return ADI_OSAL_BAD_HANDLE   -  If the specified thread handle is invalid
    @see adi_osal_ThreadGetPrio
*****************************************************************************/
ADI_OSAL_STATUS revert_fs_prio(void)
{
  return(adi_osal_ThreadSetPrio(gh_fs_task_handler,APP_OS_CFG_FS_TASK_PRIO));
}

/*!
  ****************************************************************************
    @brief File system initialization including thread creation

    @param[in]                    - None

    @return                       - None
*****************************************************************************/
void file_system_task_init(void) {
#ifdef USE_FS
  ADI_OSAL_STATUS eOsStatus;
  /* Create file system thread */
  g_fs_task_attributes.pThreadFunc = file_system_task;
  g_fs_task_attributes.nPriority = APP_OS_CFG_FS_TASK_PRIO;
  g_fs_task_attributes.pStackBase = &ga_fs_task_stack[0];
  g_fs_task_attributes.nStackSize = APP_OS_CFG_FS_TASK_STK_SIZE;
  g_fs_task_attributes.pTaskAttrParam = NULL;
  g_fs_task_attributes.szThreadName = "file_system_task";
  g_fs_task_attributes.pThreadTcb = &g_fs_task_tcb;
  eOsStatus = adi_osal_MsgQueueCreate(&gh_fs_task_msg_queue,NULL,100);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
      Debug_Handler();
  }
  else
  {
    update_task_queue_list(APP_OS_CFG_FS_TASK_INDEX,gh_fs_task_msg_queue);
  }

  eOsStatus = adi_osal_ThreadCreateStatic(&gh_fs_task_handler,
                                    &g_fs_task_attributes);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
      Debug_Handler();
  }

  adi_osal_SemCreate(&qspi_task_evt_sem, 0);
#endif
}

/*!
  ****************************************************************************
    @brief Initializes the test pattern buffer

    @param[in]        pBuff: Pointer to the test buffer

    @param[in]        nBufSize: Size of test buffer

    @return           None
*****************************************************************************/
static void InitTestBuffer(uint8_t *pBuff, uint32_t nBufSize)
{
    for (uint32_t nIndex = 0; nIndex < nBufSize; nIndex += 4)
    {
        pBuff[nIndex] = 0xDE;
        pBuff[nIndex + 1] = 0xAD;
        pBuff[nIndex + 2] = 0xBE;
        pBuff[nIndex + 3] = 0xEF;
    }
   NRF_LOG_INFO("Verify test pattern");
}

/*!
  ****************************************************************************
    @brief Sends the message packet to the post office

    @param[in]        p_pkt: Pointer to the packet

    @return           None
*****************************************************************************/
uint32_t gFileSystemMsgPostCnt =0;
uint32_t gFileSystemMsgProcessCnt =0;
uint32_t FS_msg_fail_tick = 0, g_fs_status_reps_tick = 0;
void send_message_file_system_task(m2m2_hdr_t *p_pkt) {
  ADI_OSAL_STATUS osal_result;
  osal_result = adi_osal_MsgQueuePost(gh_fs_task_msg_queue, p_pkt);
  if (osal_result != ADI_OSAL_SUCCESS)
  {
    FS_msg_fail_tick = get_sensor_time_stamp();
    post_office_consume_msg(p_pkt);
  }
  else
    gFileSystemMsgPostCnt++;
}

uint8_t get_fs_logging_status(void)
{
    if (FileSrc != M2M2_ADDR_UNDEFINED) {
      return (M2M2_FILE_SYS_STATUS_LOGGING_IN_PROGRESS);
    }else {
      return (M2M2_FILE_SYS_STATUS_LOGGING_NOT_STARTED);
    }
}

M2M2_FILE_SYS_STATUS_ENUM_t get_fs_available_memory(uint32_t *p_available_mem)
{
    uint32_t VolumeInfo[3] = {0};
    if (fs_hal_get_vol_info(VolumeInfo) == FS_STATUS_OK) {
      *p_available_mem = VolumeInfo[2];
      return M2M2_FILE_SYS_STATUS_OK;
    }
    else  {
      *p_available_mem=0;
       return M2M2_FILE_SYS_STATUS_ERROR;
    }
}


/*!
  ****************************************************************************
    @brief Retrieves the stream address, if present, from the routing table
            if the stream is not present, then returns the default address
            as M2M2_ADDR_UNDEFINED

    @param[in]        stream: ENUM for the stream

    @return           M2M2_ADDR_ENUM_t
*****************************************************************************/
M2M2_ADDR_ENUM_t get_file_routing_table_entry(M2M2_ADDR_ENUM_t stream) {
  for (int i = 0; i < FILE_ROUTING_TABLE_SIZE; i++) {
    if (file_routing_table[i].stream == stream) {
      return file_routing_table[i].address;
    }
  }
  return M2M2_ADDRESS_DEFAULT;
}

/*!
  ****************************************************************************
    @brief Retrieves the stream, if present, from the routing table
            if the address is not present, then returns the default address
            as M2M2_ADDR_UNDEFINED

    @param[in]        address: ENUM for the application address

    @return           M2M2_ADDR_ENUM_t
*****************************************************************************/
M2M2_ADDR_ENUM_t get_file_routing_table_entry_stream(M2M2_ADDR_ENUM_t address) {
  for (int i = 0; i < FILE_ROUTING_TABLE_SIZE; i++) {
    if (file_routing_table[i].address == address) {
      return file_routing_table[i].stream;
    }
  }
  return M2M2_ADDRESS_DEFAULT;
}

/*!
  ****************************************************************************
    @brief Retrieves the stream index, if present, from the routing table
            if the stream is not present, then returns -1

    @param[in]        stream: ENUM for the stream

    @return           index, if stream is present in routing table
                             else -1
*****************************************************************************/
int16_t get_file_routing_table_index(M2M2_ADDR_ENUM_t stream) {
  for (int i = 0; i < FILE_ROUTING_TABLE_SIZE; i++) {
    if (file_routing_table[i].stream == stream) {
      return i;
    }
  }
  return -1;
}
/*!
  ****************************************************************************
    @brief Retrieves the subscription status of ADPD application, if present,
            from the routing table. If the appliation is not present, then
            returns M2M2_FILE_SYS_SUBS_INVALID

    @param[in]        None

    @return           FILE_SYS_STREAM_SUBS_STATE_ENUM_t
*****************************************************************************/
FILE_SYS_STREAM_SUBS_STATE_ENUM_t get_adpd_address_sub_state() {
  for(int i = 0; i < MAX_NUMBER_OF_ADPD_STREAM; i++){
    if(file_routing_table[i].fs_subs_state == M2M2_FILE_SYS_SUBSCRIBED){
      return M2M2_FILE_SYS_SUBSCRIBED;
    }
  }
  return M2M2_FILE_SYS_SUBS_INVALID;
}

/*!
  ****************************************************************************
    @brief Retrieves the subscription status of an application, if present,
            from the routing table. If the appliation is not present, then
            returns M2M2_FILE_SYS_SUBS_INVALID

    @param[in]        addr: ENUM for the application address

    @return           FILE_SYS_STREAM_SUBS_STATE_ENUM_t
*****************************************************************************/
FILE_SYS_STREAM_SUBS_STATE_ENUM_t get_address_sub_state(M2M2_ADDR_ENUM_t addr) {
  if(addr == M2M2_ADDR_SENSOR_ADPD4000)
    return get_adpd_address_sub_state(); // if its ADPD4000 check for all the ADPD streams for sub state
  for (int i = 0; i < FILE_ROUTING_TABLE_SIZE; i++) {
    if (file_routing_table[i].address == addr) {
      return file_routing_table[i].fs_subs_state;
    }
  }
  return M2M2_FILE_SYS_SUBS_INVALID;
}

/*!
* @brief:  The data structure used to debug checksum and missed stream
*           packets in the filesystem.
*/
typedef struct _file_misd_packet_debug_table {
  M2M2_ADDR_ENUM_t stream;
  uint16_t count;
  uint32_t totalpacketcount;
  uint32_t missedpackets;
}file_misd_packet_debug_table;

/*!
* @brief:  The array used to debug checksum and missed stream
*           packets in the filesystem.
*/
static file_misd_packet_debug_table file_misd_packet_table[] = {
  {M2M2_ADDR_SENSOR_ADPD_STREAM1,0,0,0},
  {M2M2_ADDR_SENSOR_ADXL_STREAM,0,0,0},
  {M2M2_ADDR_MED_PPG_STREAM,0,0,0},
  {M2M2_ADDR_MED_PED_STREAM,0,0,0},
#ifdef UNUSED_CODES
  {M2M2_ADDR_MED_AGC_STREAM,0,0,0},
#endif
  {M2M2_ADDR_MED_EDA_STREAM,0,0,0},
  {M2M2_ADDR_MED_ECG_STREAM,0,0,0},
  {M2M2_ADDR_MED_BCM_STREAM,0,0,0},
  {M2M2_ADDR_MED_SQI_STREAM,0,0,0},
  {M2M2_ADDR_MED_TEMPERATURE_STREAM,0,0,0},
  {M2M2_ADDR_MED_SYNC_ADPD_ADXL_STREAM,0,0,0},
#ifdef UNUSED_CODES
  {M2M2_ADDR_APP_DROID_STREAM,0,0,0},
  {M2M2_ADDR_APP_WT_STREAM,0,0,0},
#endif
  {M2M2_ADDR_APP_CLI_STREAM,0,0,0},
  {M2M2_ADDR_APP_DISPLAY_STREAM,0,0,0},
  {M2M2_ADDR_SYS_BATT_STREAM,0,0,0},
#ifdef UNUSED_CODES
  {M2M2_ADDR_APP_IOS_STREAM,0,0,0},
  {M2M2_ADDR_MED_AD7689_STREAM,0,0,0},
#endif
};

/* Number of entries in the file system debug table */
#define FILE_DEBUG_TABLE_SIZE (sizeof(file_misd_packet_table)/sizeof(file_misd_packet_table[0]))

/*!
  ****************************************************************************
    @brief Retrieves the stream index of a stream, if present,
            from the file system debug table.
            If the stream is not present, then returns -1

    @param[in]        stream: ENUM for the stream address

    @return           int16_t
*****************************************************************************/
int16_t get_file_misd_packet_table_index(M2M2_ADDR_ENUM_t stream) {
  for (int i = 0; i < FILE_DEBUG_TABLE_SIZE; i++) {
    if (file_misd_packet_table[i].stream == stream) {
      return i;
    }
  }
  return -1;
}

/*!
  ****************************************************************************
    @brief Retrieves the stream debug information, if present,
            from the file system debug table.

    @param[in]        debug_info: structure holding debug information

    @return           None
*****************************************************************************/
#ifdef PAGE_READ_DEBUG_INFO
extern uint32_t last_page_read;
extern uint32_t last_page_read_offset;
extern uint8_t last_page_read_status;
extern uint32_t num_bytes_transferred;
extern uint32_t bytes_processed_from_fs_task;
extern uint8_t usb_cdc_write_failed;
#endif

#ifdef FORMAT_DEBUG_INFO_CMD
extern fs_format_debug_info tmp_fs_format_debug_info;
#endif
void get_file_misd_packet_debug_info(m2m2_file_sys_debug_info_resp_t *debug_info) {
    for (int i = 0; i < FILE_DEBUG_TABLE_SIZE; i++) {
    if (file_misd_packet_table[i].stream == debug_info->stream) {
      debug_info->packets_received = file_misd_packet_table[i].totalpacketcount;
      debug_info->packets_missed = file_misd_packet_table[i].missedpackets;
    }
  }

#ifdef PAGE_READ_DEBUG_INFO
  debug_info->last_page_read = last_page_read;
  debug_info->last_page_read_offset = last_page_read_offset;
  debug_info->last_page_read_status = last_page_read_status;
  debug_info->num_bytes_transferred = num_bytes_transferred;
  debug_info->bytes_read = bytes_processed_from_fs_task;
  debug_info->usb_cdc_write_failed = usb_cdc_write_failed;
#else
  debug_info->last_page_read  = 0;
  debug_info->last_page_read_offset = 0;
  debug_info->last_page_read_status  = 0;
  debug_info->num_bytes_transferred = 0;
  debug_info->bytes_read = 0;
  debug_info->usb_cdc_write_failed = 0;
#endif
  NRF_LOG_INFO("bytes proc=%d",debug_info->bytes_read);
}

/*!
  ***************************************************************************************************
    @brief            get fs format debug info

    @param[in]        debug_info: Pointer to structure m2m2_file_sys_format_debug_info_resp_t

    @return           void
****************************************************************************************************/
void get_fs_format_debug_info(m2m2_file_sys_format_debug_info_resp_t *debug_info) {
    debug_info->format_src_blk_ind = tmp_fs_format_debug_info.format_src_blk_ind; 
    debug_info->format_dest_blk_ind_1 = tmp_fs_format_debug_info.format_dest_blk_ind_1; 
    debug_info->format_dest_blk_ind_2 = tmp_fs_format_debug_info.format_dest_blk_ind_2; 
    debug_info->wrap_around_cond = tmp_fs_format_debug_info.wrap_around_cond; 
    debug_info->erase_failed_due_bad_block_check = tmp_fs_format_debug_info.erase_failed_due_bad_block_check; 
    debug_info->nothing_is_written_to_erase_error = tmp_fs_format_debug_info.nothing_is_written_to_erase_error; 
    debug_info->mem_full_in_partial_erase = tmp_fs_format_debug_info.mem_full_in_partial_erase; 
    debug_info->num_blocks_erased_in_mem_full_partial_erase = tmp_fs_format_debug_info.num_blocks_erased_in_mem_full_partial_erase; 
    debug_info->num_blocks_erased_in_partial_erase_1 = tmp_fs_format_debug_info.num_blocks_erased_in_partial_erase_1; 
    debug_info->num_blocks_erased_in_partial_erase_2 = tmp_fs_format_debug_info.num_blocks_erased_in_partial_erase_2; 
    debug_info->num_times_format_failed_due_bad_blocks_1 = tmp_fs_format_debug_info.num_times_format_failed_due_bad_blocks_1; 
    debug_info->num_times_format_failed_due_bad_blocks_2 = tmp_fs_format_debug_info.num_times_format_failed_due_bad_blocks_2; 
    debug_info->toc_mem_erased_flag = tmp_fs_format_debug_info.toc_mem_erased_flag; 
    debug_info->succesfull_erase_flag = tmp_fs_format_debug_info.succesfull_erase_flag; 
  }

/*!
  ****************************************************************************
    @brief Unsubscribes the streams subscribed to file system

    @param[in]        None

    @return           bool:true if no stream is subscribed, else false
*****************************************************************************/
bool fs_unsubscribe_streams(void) {
  m2m2_hdr_t *pkt = NULL;
  ADI_OSAL_STATUS err;
  uint8_t sub_cnt = 0;

  /* Check for stream subscription and send Unsub Req to stream */
  for(int i = 0; i<FILE_ROUTING_TABLE_SIZE; i++) {
    if(file_routing_table[i].fs_subs_state == M2M2_FILE_SYS_SUBSCRIBED) {
      pkt = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_file_sys_log_stream_t));
      if (pkt != NULL) {
        m2m2_file_sys_log_stream_t *unsubscribe_req = (m2m2_file_sys_log_stream_t*)&pkt->data[0];
        unsubscribe_req->command = M2M2_FILE_SYS_CMD_STOP_STREAM_REQ;
        unsubscribe_req->status = M2M2_APP_COMMON_STATUS_OK;
        pkt->src = M2M2_ADDR_SYS_FS;
        pkt->dest = M2M2_ADDR_SYS_FS;
        unsubscribe_req->stream = file_routing_table[i].stream;
        post_office_send(pkt, &err);
        sub_cnt++;
      }
    }
  }
  /* Return true if no stream is subscribed */
  if(sub_cnt == 0)
    return true;
  else
    return false;
}

/*!
  ****************************************************************************
    @brief Find the configuration file in the configuration block

    @param[in]        pCfgFileFoundFlag: flag indicating the config file

    @return           None
*****************************************************************************/
void FindConfigFile(volatile uint8_t* pCfgFileFoundFlag)
{
 uint8_t cfgFileFoundFlag;
 if (fs_flash_power_on(true) == FS_STATUS_OK) {
    fs_hal_find_config_file(ConfigFileName, sizeof(ConfigFileName),0, &cfgFileFoundFlag);
    *pCfgFileFoundFlag = cfgFileFoundFlag;
  }
  if (fs_flash_power_on(false)  != FS_STATUS_OK){
    NRF_LOG_INFO("Error in flash power off");
  }
}

/* macros, array and variables used for injecting kep ID before a log */
#define M2M2_KEY_ID 0xC0FF
#define M2M2_DEFAULT_KEY_ID 0xFFFF
static m2m2_file_sys_set_key_value_pair_resp_t g_userinfo = {0,0,M2M2_DEFAULT_KEY_ID,{0x00},0,0,0,0,0,0,0};
static uint8_t FileStreamSubsciberCount = 0;
static volatile uint8_t fs_download_chunk_cnt = 0;
uint8_t  fs_stream_state = ADI_FS_STREAM_STOPPED;
uint8_t fs_last_cmd = 0;

#ifdef PROFILE_TIME_ENABLED
extern uint64_t g_page_hold_write_time;
#endif


/*!
  ****************************************************************************
    @brief Gets the file download status

    @param[in]        None

    @return           bool: true if started, else false
*****************************************************************************/
bool get_download_status()
{
  if(fs_stream_state == ADI_FS_STREAM_STARTED)
    return true;
  else
    return false;
}


/*!
  ****************************************************************************
    @brief            Close opened
                      file before watch reset if logging is happening.

    @param[in]        None

    @return           bool: true if started, else false
*****************************************************************************/

bool UpdateFileInfo(){
  FS_STATUS_ENUM_t fs_error;
  if(fs_hal_write_access_state() == FS_FILE_ACCESS_IN_PROGRESS) {
    /* close file */
    fs_error = fs_hal_close_file(fs_hal_write_file_pointer());
    if(fs_error == FS_STATUS_OK) {
       fs_flash_power_on(false);
    }
    else{
    /* error handling when file is not closed properly */
      return false;
    }
  }
  return true;
}

/*!
  ****************************************************************************
* @brief      Commands and response for file system task interface
* @param      pArgument not used
* @return     None
  ****************************************************************************/
#ifdef ADPD_SEM_CORRUPTION_DEBUG
 uint32_t start_log_counter_rx=0;
 uint32_t stop_log_counter_rx=0;
 uint32_t start_fs_sub_rx=0;
 uint32_t stop_fs_sub_rx=0;
#endif

#ifdef DEBUG_LOG_START
  uint32_t start_log_timer_start=0;
  uint32_t get_file_count_timer_start=0;
  uint32_t fs_sub_timer_start=0;
  uint32_t fs_sub_timer_gap=0;
  uint32_t get_file_count_timer_gap=0;
  uint32_t start_log_timer_gap=0;
  uint32_t vol_info_timer_start=0;
  uint32_t vol_info_timer_gap=0;
#endif
void file_system_task(void *pArgument) {
  m2m2_hdr_t                            *pkt = NULL;
  _m2m2_app_common_cmd_t                *ctrl_cmd = NULL;
  m2m2_hdr_t                            *response_mail = NULL;
  ADI_OSAL_STATUS                       err;
  bool                                  file_hdr_wr_progress = false;
  uint32_t                              timeout = ADI_OSAL_TIMEOUT_FOREVER;
  uint8_t                               fs_stream_final_buffer = 1;
  uint16_t                              nSeqNum = 0;
  M2M2_ADDR_ENUM_t                      nTempAdress;

#ifdef DEBUG_LOG_START
         us_timer_init();
#endif /* PROFILE_TIME_ENABLED */

  /* flash power on */
  adp5360_enable_ldo(FS_LDO,true);

  if(fs_hal_init() == FS_STATUS_INIT_DRIVER_ERR){
      /* delete thread, only if driver error */
      adi_osal_ThreadDestroy(gh_fs_task_handler);
  }
  
  /* flash power off */
  adp5360_enable_ldo(FS_LDO,false);


  /* Create a mailbox for FS stream */
  pkt = post_office_create_msg(M2M2_HEADER_SZ + sizeof(post_office_config_t));
  if(pkt != NULL) {
    post_office_config_t *payload = (post_office_config_t *)&pkt->data[0];
    pkt->src = M2M2_ADDR_SYS_FS;
    pkt->dest = M2M2_ADDR_POST_OFFICE;
    pkt->length = M2M2_HEADER_SZ + sizeof(post_office_config_t);

    payload->box = M2M2_ADDR_SYS_FS_STREAM;
    payload->cmd = POST_OFFICE_CFG_CMD_ADD_MAILBOX;
    post_office_send(pkt, &err);
  }

  FindConfigFile(&gsCfgFileFoundFlag);
#ifdef LOW_TOUCH_FEATURE

  adi_osal_SemPost(lt_task_evt_sem);
#endif

  while (1) {
    pkt = post_office_get(timeout,APP_OS_CFG_FS_TASK_INDEX);
    if (pkt != NULL) {
//      gFileSystemMsgProcessCnt++;
      /* check for stream source to log the packet */
      nTempAdress = get_file_routing_table_entry(pkt->src);
      /* If it is NOT a stream packet */
      if (nTempAdress == M2M2_ADDRESS_DEFAULT) {
      ctrl_cmd = (_m2m2_app_common_cmd_t*)&pkt->data[0];
#ifdef ADPD_SEM_CORRUPTION_DEBUG
      if(ctrl_cmd->command == M2M2_FILE_SYS_CMD_START_LOGGING_REQ){
        start_log_counter_rx += 1;
      }
      else if(ctrl_cmd->command == M2M2_FILE_SYS_CMD_STOP_LOGGING_REQ){
        stop_log_counter_rx += 1;
      }
      else if(ctrl_cmd->command == M2M2_FILE_SYS_CMD_LOG_STREAM_REQ){
        start_fs_sub_rx += 1;
      }
      else if(ctrl_cmd->command == M2M2_FILE_SYS_CMD_STOP_STREAM_REQ){
        stop_fs_sub_rx += 1;
      }
#endif
#ifdef DEBUG_LOG_START
      if(ctrl_cmd->command == M2M2_FILE_SYS_CMD_START_LOGGING_REQ){
        start_log_timer_start = get_micro_sec();
      }
      else if(ctrl_cmd->command == M2M2_FILE_SYS_CMD_GET_NUMBER_OF_FILE_REQ){
        get_file_count_timer_start = get_micro_sec();
      }
      else if(ctrl_cmd->command == M2M2_FILE_SYS_CMD_LOG_STREAM_REQ){
        fs_sub_timer_start = get_micro_sec();
      }
       else if(ctrl_cmd->command == M2M2_FILE_SYS_CMD_VOL_INFO_REQ){
        vol_info_timer_start = get_micro_sec();
      }
#endif
      fs_last_cmd = ctrl_cmd->command;
      switch (ctrl_cmd->command) {
      case M2M2_FILE_SYS_CMD_MOUNT_REQ: {
        response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(_m2m2_app_common_cmd_t));
        if (response_mail != NULL) {
          _m2m2_app_common_cmd_t *mount_resp = (_m2m2_app_common_cmd_t *)&response_mail->data[0];
          /* Power On and mount memory partition */
          if (fs_flash_power_on(true) != FS_STATUS_OK) {
            mount_resp->status = M2M2_FILE_SYS_STATUS_ERROR;
          } else {
            mount_resp->status = M2M2_FILE_SYS_STATUS_OK;
          }
          mount_resp->command = (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_FILE_SYS_CMD_MOUNT_RESP;
          /* send response packet */
          response_mail->src = pkt->dest;
          response_mail->dest = pkt->src;
          post_office_send(response_mail, &err);

          /* Power off Flash */
          if (fs_flash_power_on(false)  != FS_STATUS_OK){
            NRF_LOG_INFO("Error in flash power off");
          }
        }
        break;
      } case M2M2_FILE_SYS_CMD_FORMAT_REQ: {
        response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_file_sys_cmd_t));
        if (response_mail != NULL) {
          m2m2_file_sys_cmd_t *format_resp = (m2m2_file_sys_cmd_t *)&response_mail->data[0];
          /* Switch on and mount memory */
          if (fs_flash_power_on(true) == FS_STATUS_OK) {
            /* Format memory partition */
            /* Deletes only the data files; preserves config files */
            if(fs_hal_format(false) != FS_STATUS_OK) {
              format_resp->status = M2M2_FILE_SYS_ERR_FORMAT;
            }
            else {
#ifdef ENABLE_WATCH_DISPLAY
              reset_display_vol_info();
#endif
              format_resp->status = M2M2_FILE_SYS_STATUS_OK;
            }
          } else {
            format_resp->status = M2M2_FILE_SYS_STATUS_ERROR;
          }
          format_resp->command = M2M2_FILE_SYS_CMD_FORMAT_RESP;
          /* send response packet */
          response_mail->src = pkt->dest;
          response_mail->dest = pkt->src;
          post_office_send(response_mail, &err);
          /* Power off Flash */
          fs_flash_power_on(false);
        }
        break;
       } /* response_mail != NULL */
#ifdef FS_TEST_CODE        
        case M2M2_FILE_SYS_CMD_BLOCK_ERASE_REQ: {
        m2m2_file_sys_blk_erase_cmd_t *block_erase_req = (m2m2_file_sys_blk_erase_cmd_t *)&pkt->data[0];
        response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_file_sys_blk_erase_cmd_t));
        if (response_mail != NULL) {
          m2m2_file_sys_blk_erase_cmd_t *erase_resp = (m2m2_file_sys_blk_erase_cmd_t *)&response_mail->data[0];
          /* Switch on and mount memory */
          if (fs_flash_power_on(true) == FS_STATUS_OK) {
            /* Format memory partition */
            /* Deletes only the data files; preserves config files */
            if(fs_block_erase(block_erase_req->block_no) != FS_STATUS_OK) {
              erase_resp->status = M2M2_FILE_SYS_STATUS_ERROR;
            }
            else{
              erase_resp->status = M2M2_FILE_SYS_STATUS_OK;
            }
          } else {
            erase_resp->status = M2M2_FILE_SYS_STATUS_ERROR;
          }
          erase_resp->command = M2M2_FILE_SYS_CMD_FORMAT_RESP;
          /* send response packet */
          response_mail->src = pkt->dest;
          response_mail->dest = pkt->src;
          post_office_send(response_mail, &err);
          /* Power off Flash */
          fs_flash_power_on(false);
        }
        break;
       }       
        case M2M2_FILE_SYS_WRITE_RANDOM_DATA_TO_RSD_BLK_REQ: {
        m2m2_file_sys_write_rsd_blk_cmd_t *input_payload = (m2m2_file_sys_write_rsd_blk_cmd_t *)&pkt->data[0];
        response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_file_sys_write_rsd_blk_cmd_t));
        if (response_mail != NULL) {
          m2m2_file_sys_write_rsd_blk_cmd_t *random_data = (m2m2_file_sys_write_rsd_blk_cmd_t *)&response_mail->data[0];
          for(int i=0;i<MAX_NUM_OF_CHAR;i++){
            NRF_LOG_INFO("%d",input_payload->data[i]);
          }
          /* Switch on and mount memory */
          if (fs_flash_power_on(true) == FS_STATUS_OK) {
            if(fs_write_rsd_block(input_payload->data,MAX_NUM_OF_CHAR) != FS_STATUS_OK){
              random_data->status = M2M2_FILE_SYS_STATUS_ERROR;
            }
            random_data->command = M2M2_FILE_SYS_WRITE_RANDOM_DATA_TO_RSD_BLK_RESP;
            /* send response packet */
            response_mail->src = pkt->dest;
            response_mail->dest = pkt->src;
            post_office_send(response_mail, &err);
            /* Power off Flash */
            fs_flash_power_on(false);
          }
        }
        break;
       }
#endif
	case M2M2_FILE_SYS_CMD_VOL_INFO_REQ: {
        uint32_t VolumeInfo[3] = {0};
        response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_file_sys_vol_info_resp_t));
        if (response_mail != NULL) {
          m2m2_file_sys_vol_info_resp_t *fs_free_space_resp = (m2m2_file_sys_vol_info_resp_t *)&response_mail->data[0];
          if(FileSrc == M2M2_ADDR_UNDEFINED) {
            /* Switch on, mount memory and get free volume size */
            if (fs_flash_power_on(true) == FS_STATUS_OK){
             if (fs_hal_get_vol_info(VolumeInfo) == FS_STATUS_OK) {
                fs_free_space_resp->totalmemory = VolumeInfo[0];
                fs_free_space_resp->usedmemory = VolumeInfo[1];
                fs_free_space_resp->availmemory = VolumeInfo[2];
  #ifdef PRINTS_OUT
              NRF_LOG_INFO("In Vol Req: Total memory = %d",fs_free_space_resp->totalmemory);
              NRF_LOG_INFO("In Vol Req: Used memory = %d",fs_free_space_resp->usedmemory);
              NRF_LOG_INFO("In Vol Req: Free memory = %d",fs_free_space_resp->availmemory);
  #endif
              fs_free_space_resp->status =(M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_FILE_SYS_STATUS_OK;
            }
            else
            {
              fs_free_space_resp->totalmemory = 0;
              fs_free_space_resp->usedmemory = 0;
              fs_free_space_resp->availmemory = 0;
              fs_free_space_resp->status = (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_FILE_SYS_STATUS_ERROR;
            }
           }else {
              fs_free_space_resp->totalmemory = 0;
              fs_free_space_resp->usedmemory = 0;
              fs_free_space_resp->availmemory = 0;
              fs_free_space_resp->status = (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_FILE_SYS_STATUS_ERROR;
           }
          } /* FileSrc == M2M2_ADDR_UNDEFINED */
          else {
          /* Switch on, mount memory and get free volume size */
            if(fs_hal_get_vol_info(VolumeInfo) == FS_STATUS_OK) {
                fs_free_space_resp->totalmemory = VolumeInfo[0];
                fs_free_space_resp->usedmemory = VolumeInfo[1];
                fs_free_space_resp->availmemory = VolumeInfo[2];
#ifdef PRINTS_OUT
                NRF_LOG_INFO("In Vol Req: Total memory = %d",fs_free_space_resp->totalmemory);
                NRF_LOG_INFO("In Vol Req: Used memory = %d",fs_free_space_resp->usedmemory);
                NRF_LOG_INFO("In Vol Req: Free memory = %d",fs_free_space_resp->availmemory);
#endif

                fs_free_space_resp->status =(M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_FILE_SYS_STATUS_OK;
#ifdef DEBUG_LOG_START
                vol_info_timer_gap = get_micro_sec() - vol_info_timer_start;
#endif
            } else {
                fs_free_space_resp->totalmemory = 0;
                fs_free_space_resp->usedmemory = 0;
                fs_free_space_resp->availmemory = 0;
                fs_free_space_resp->status = (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_FILE_SYS_STATUS_ERROR;
            }
          } /* FileSrc != M2M2_ADDR_UNDEFINED */

          fs_free_space_resp->command = (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_FILE_SYS_CMD_VOL_INFO_RESP;
          /* send response packet */
          response_mail->src = pkt->dest;
          response_mail->dest = pkt->src;
          post_office_send(response_mail, &err);

           /* Power off Flash */
          if(FileSrc == M2M2_ADDR_UNDEFINED) {
            if(fs_flash_power_on(false) != FS_STATUS_OK){
              NRF_LOG_INFO("Error in flash power off");
            }
          }
        } /* response_mail != NULL */
        break;
	}
#ifdef LOW_TOUCH_FEATURE
        case M2M2_FILE_SYS_CMD_FIND_CONFIG_FILE_REQ: {
          uint8_t nCfgFileFoundFlag  = 0;
          FS_STATUS_ENUM_t fs_err_status = FS_STATUS_ERR;
          fs_flash_power_on(true);
          fs_err_status = fs_hal_find_config_file(ConfigFileName, sizeof(ConfigFileName),0, &nCfgFileFoundFlag);
          response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_file_sys_cmd_t));
          if (response_mail != NULL) {
            m2m2_file_sys_cmd_t *find_cfg_file_resp = (m2m2_file_sys_cmd_t *)&response_mail->data[0];
            find_cfg_file_resp->command = (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_FILE_SYS_CMD_FIND_CONFIG_FILE_RESP;
            if(nCfgFileFoundFlag)
                find_cfg_file_resp->status = (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_FILE_SYS_CONFIG_FILE_FOUND;
            else
                find_cfg_file_resp->status = (M2M2_APP_COMMON_STATUS_ENUM_t)fs_err_status;
            response_mail->src = pkt->dest;
            response_mail->dest = pkt->src;
            /* send response packet */
            post_office_send(response_mail, &err);
          }
          fs_flash_power_on(false);
        break;
      }
#endif /* LOW_TOUCH_FEATURE */

        case M2M2_FILE_SYS_CMD_GET_NUMBER_OF_FILE_REQ: {
          uint8_t nFileCount = 0;
          FS_STATUS_ENUM_t fs_err_status = FS_STATUS_ERR;
          if(fs_flash_power_on(true) != FS_STATUS_OK){
            NRF_LOG_INFO("Error in flash power on");
          }
          fs_err_status = fs_hal_get_file_count(&nFileCount);
          response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_file_sys_get_file_count_pkt_t));
          if (response_mail != NULL) {
            m2m2_file_sys_get_file_count_pkt_t *get_num_files_resp = (m2m2_file_sys_get_file_count_pkt_t *)&response_mail->data[0];
            get_num_files_resp->command = (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_FILE_SYS_CMD_GET_NUMBER_OF_FILE_RESP;
            get_num_files_resp->status = (M2M2_APP_COMMON_STATUS_ENUM_t)fs_err_status;
            get_num_files_resp->file_count = nFileCount;
            response_mail->src = pkt->dest;
            response_mail->dest = pkt->src;
            /* send response packet */
            post_office_send(response_mail, &err);
#ifdef DEBUG_LOG_START
           get_file_count_timer_gap = get_micro_sec() - get_file_count_timer_start;
#endif
          }
          if(fs_flash_power_on(false) != FS_STATUS_OK){
            NRF_LOG_INFO("Error in flash power on");
          }
          break;
      }

      case M2M2_FILE_SYS_CMD_GET_FILE_INFO_REQ: {
        FS_STATUS_ENUM_t fs_err_status = FS_STATUS_ERR;
        m2m2_file_sys_get_file_info_req_pkt_t *get_file_info_req = (m2m2_file_sys_get_file_info_req_pkt_t *)&pkt->data[0];
        /* Power On and mount flash memory */
        if(fs_flash_power_on(true) != FS_STATUS_OK) {
          response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_file_sys_get_file_info_resp_pkt_t));
          if (response_mail != NULL) {
            m2m2_file_sys_get_file_info_resp_pkt_t *get_file_info_resp = (m2m2_file_sys_get_file_info_resp_pkt_t *)&response_mail->data[0];
            get_file_info_resp->command = (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_FILE_SYS_CMD_GET_FILE_INFO_RESP;
            get_file_info_resp->status = (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_FILE_SYS_STATUS_ERROR;
            response_mail->src = pkt->dest;
            response_mail->dest = pkt->src;
            /* send response packet */
            post_office_send(response_mail, &err);
          }
        }else {
            response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_file_sys_get_file_info_resp_pkt_t));
            if (response_mail != NULL) {
              m2m2_file_sys_get_file_info_resp_pkt_t *get_file_info_resp = (m2m2_file_sys_get_file_info_resp_pkt_t *)&response_mail->data[0];
              get_file_info_resp->command = (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_FILE_SYS_CMD_GET_FILE_INFO_RESP;
              response_mail->src = pkt->dest;
              response_mail->dest = pkt->src;
              memset(&get_file_info_resp->file_name, 0, sizeof(get_file_info_resp->file_name));
              file_info_t get_file_info;
              /* get list files in directory */
              fs_err_status = fs_hal_get_file_info(&get_file_info_req->file_index, &get_file_info);
              if (fs_err_status != FS_STATUS_OK) {
                get_file_info_resp->status = fs_err_status;
              } else {
                get_file_info_resp->status = M2M2_FILE_SYS_STATUS_OK;
                memcpy(&get_file_info_resp->file_name,&get_file_info.file_name, sizeof(get_file_info_resp->file_name));
                get_file_info_resp->start_page = get_file_info.start_page;
                get_file_info_resp->end_page = get_file_info.end_page;
                get_file_info_resp->file_size = get_file_info.file_size;
              }
              post_office_send(response_mail, &err);
            }

          /* Power off Flash */
          if(fs_flash_power_on(false) != FS_STATUS_OK) {
            NRF_LOG_INFO("Error flash power on");
          }
        }
        break;
      }

      case M2M2_FILE_SYS_CMD_PAGE_READ_TEST_REQ: {
        FS_STATUS_ENUM_t fs_err_status = FS_STATUS_ERR;
        m2m2_file_sys_page_read_test_req_pkt_t *page_read_test_req = (m2m2_file_sys_page_read_test_req_pkt_t *)&pkt->data[0];
        /* Power On and mount flash memory */
        if(fs_flash_power_on(true) != FS_STATUS_OK) {
          response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_file_sys_page_read_test_resp_pkt_t));
          if (response_mail != NULL) {
            m2m2_file_sys_page_read_test_resp_pkt_t *page_test_resp = (m2m2_file_sys_page_read_test_resp_pkt_t *)&response_mail->data[0];
            page_test_resp->command = (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_FILE_SYS_CMD_PAGE_READ_TEST_RESP;
            page_test_resp->status = (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_FILE_SYS_STATUS_ERROR;
            response_mail->src = pkt->dest;
            response_mail->dest = pkt->src;
            /* send response packet */
            post_office_send(response_mail, &err);
          }
        }else {
            response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_file_sys_page_read_test_resp_pkt_t));
            if (response_mail != NULL) {
              m2m2_file_sys_page_read_test_resp_pkt_t *page_read_test_resp = (m2m2_file_sys_page_read_test_resp_pkt_t *)&response_mail->data[0];
              page_read_test_resp->command = (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_FILE_SYS_CMD_PAGE_READ_TEST_RESP;
              response_mail->src = pkt->dest;
              response_mail->dest = pkt->src;
              page_read_test_resp->page_num = page_read_test_req->page_num;
              /* read the data and spare area of a page */
              page_read_test_resp->num_bytes = page_read_test_req->num_bytes;
              fs_err_status = fs_hal_page_read_test(&page_read_test_req->page_num,page_read_test_resp,\
              page_read_test_req->num_bytes);
              if (fs_err_status != FS_STATUS_OK) {
                page_read_test_resp->status = fs_err_status;
              } else {
                page_read_test_resp->status = M2M2_FILE_SYS_STATUS_OK;

              }
              post_office_send(response_mail, &err);
            }
            
          /* Power off Flash */
          if(fs_flash_power_on(false) != FS_STATUS_OK) {
            NRF_LOG_INFO("Error flash power on");
          }
        }
        break;
      }

      case M2M2_FILE_SYS_CMD_LS_REQ: {
        FS_STATUS_ENUM_t fs_err_status = FS_STATUS_ERR;
        char file_dir[16];
        uint32_t FileSize = 0;
        uint8_t FileType = 0;
        snprintf(file_dir, 16, "%s\\", fs_hal_vol_name());
        /* Power On and mount flash memory */
        if(fs_flash_power_on(true) != FS_STATUS_OK) {
          response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_file_sys_ls_resp_t));
          if (response_mail != NULL) {
            m2m2_file_sys_ls_resp_t *list_files_resp = (m2m2_file_sys_ls_resp_t *)&response_mail->data[0];
            list_files_resp->command = (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_FILE_SYS_CMD_LS_RESP;
            list_files_resp->status = (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_FILE_SYS_STATUS_ERROR;
            response_mail->src = pkt->dest;
            response_mail->dest = pkt->src;
            /* send response packet */
            post_office_send(response_mail, &err);
          }
        }else {
          do {
            response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_file_sys_ls_resp_t));
            if (response_mail != NULL) {
              m2m2_file_sys_ls_resp_t *list_files_resp = (m2m2_file_sys_ls_resp_t *)&response_mail->data[0];
              list_files_resp->command = (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_FILE_SYS_CMD_LS_RESP;
              response_mail->src = pkt->dest;
              response_mail->dest = pkt->src;
              memset(&list_files_resp->full_file_name, 0, sizeof(list_files_resp->full_file_name));

              /* get list files in directory */
              fs_err_status = fs_hal_list_dir(file_dir, (char *)list_files_resp->full_file_name, \
                (sizeof(list_files_resp->full_file_name)), &FileType, &FileSize);
              if (fs_err_status != FS_STATUS_OK) {
                list_files_resp->filetype = M2M2_FILE_SYS_INVALID_TYPE;
                list_files_resp->filesize = 0;
                switch(fs_err_status) {
                case FS_STATUS_ERR_INVALID_DIR: {
                  list_files_resp->status = (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_FILE_SYS_ERR_INVALID;
                  break;
                } case FS_STATUS_ERR_EMPTY_DIR:
                case FS_STATUS_ERR_EOD:{
                  list_files_resp->status = (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_FILE_SYS_END_OF_DIR;
                  break;
                } default: {
                  list_files_resp->status = M2M2_FILE_SYS_STATUS_ERROR;
                  break;
                }
                }
              } else {
                list_files_resp->status = M2M2_FILE_SYS_STATUS_OK;
                list_files_resp->filetype = (FILE_TYPE_ENUM_t)FileType;
                list_files_resp->filesize = FileSize;
              }
              post_office_send(response_mail, &err);
            }
          } while(fs_err_status == FS_STATUS_OK);

          /* Power off Flash */
          if(fs_flash_power_on(false) != FS_STATUS_OK) {
            NRF_LOG_INFO("Error flash power on");
          }
        }
        break;
      }

      case M2M2_FILE_SYS_CMD_GET_BAD_BLOCKS_REQ: {
        response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_file_sys_get_bad_blocks_cmd_t));
        if (response_mail != NULL) {
            m2m2_file_sys_get_bad_blocks_cmd_t *resp = (m2m2_file_sys_get_bad_blocks_cmd_t*)&response_mail->data[0];
            resp->command = M2M2_FILE_SYS_CMD_GET_BAD_BLOCKS_RESP;
            response_mail->src = pkt->dest;
            response_mail->dest = pkt->src;

            /* Power on Flash */
            if(fs_flash_power_on(true) != FS_STATUS_OK) {
              NRF_LOG_INFO("Error flash power on");
            }

            /* Get bad blocks number */
            uint32_t bad_blocks;
            if( LFS_SUCCESS != get_bad_block(&bad_blocks)) {
              resp->bad_blocks = bad_blocks;
              resp->status = (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_APP_COMMON_STATUS_ERROR;
            } else {
              resp->bad_blocks = 0;
              resp->status =  (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_APP_COMMON_STATUS_OK;
            }
            /* Send subscription packet */
            post_office_send(response_mail, &err);

 /* Power off Flash */
            if(fs_flash_power_on(false) != FS_STATUS_OK) {
              NRF_LOG_INFO("Error flash power off");
            }
        }
        break;
      }

      case M2M2_FILE_SYS_CMD_DOWNLOAD_LOG_REQ: {
        if(fs_stream_state != ADI_FS_STREAM_STARTED) {
#ifdef PROFILE_TIME_ENABLED
          usb_avg_tx_time = 0;
          num_bytes_transferred = 0;
          avg_file_read_time = 0;
          num_times_read = 0;
          usb_min_cdc_time = 4,294,967,295;
          usb_max_cdc_time = 0;
          usb_min_tx_time= 4,294,967,295;
          usb_max_tx_time = 0;
          max_num_of_retries = 0;
          total_time_taken_512_bytes_transfer = 0;
#endif
#ifdef PAGE_READ_DEBUG_INFO
          /* reset variables */
          last_page_read = 0;
          last_page_read_offset = 0;
          last_page_read_status = 0;
          num_bytes_transferred = 0;
          bytes_processed_from_fs_task = 0;
          usb_cdc_write_failed = 0;
#endif
          memset(&fs_stream.file_path,0,sizeof(fs_stream.file_path));
          m2m2_file_sys_get_req_t *get_file_req = (m2m2_file_sys_get_req_t *)&pkt->data[0];
          /* Copy file name global path */
          snprintf(fs_stream.file_path, ((pkt->length - M2M2_HEADER_SZ - 1)) , \
            "%s", get_file_req->file_name);
          memset(&get_file_req->file_name, 0,
                 pkt->length - (M2M2_HEADER_SZ + \
                   sizeof(m2m2_file_sys_get_req_t) - 1));
          /* Power On and mount flash memory */
          if(fs_flash_power_on(true) != FS_STATUS_OK) {
            response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_file_sys_cmd_t));
            if (response_mail != NULL) {
              m2m2_file_sys_cmd_t *err_resp = (m2m2_file_sys_cmd_t *)&response_mail->data[0];
              err_resp->command = M2M2_FILE_SYS_CMD_DOWNLOAD_LOG_RESP;
              err_resp->status = M2M2_FILE_SYS_STATUS_ERROR;
              response_mail->src = pkt->dest;
              response_mail->dest = pkt->src;
              /* send response packet */
              post_office_send(response_mail, &err);
            }
          } else {
            fs_stream.fs_buffer_size = sizeof(fs_stream.file_buff);
            fs_stream.processed_data_len = 0;
            setFsDownloadFlag(0);
            fs_download_chunk_cnt = 0;
            /* Open file for reading data */
            fs_stream.fs_err_status = fs_hal_read_file(fs_stream.file_path,
                                                       &fs_stream.file_buff[0],
                                                       &fs_stream.fs_buffer_size);
            fs_status_stored = fs_stream.fs_err_status;
            if(((fs_stream.fs_err_status == FS_STATUS_OK)
                                             || (fs_stream.fs_err_status == FS_STATUS_ERR_EOF))
                                             && (FileStreamSubsciberCount == 0)){
              fs_stream.num_bytes_left = fs_stream.fs_buffer_size;
              fs_stream.pkt_src = pkt->src;
              fs_stream.pkt_dest = pkt->dest;
              /*Check if DOWNLOAD_LOG is requested by external Tool Entity*/
              if((pkt->src >= M2M2_ADDR_APP_DROID) && (pkt->src <= M2M2_ADDR_APP_CLI_BLE))
                gsFsDownloadReqByExtTool = 1;
              FileStreamSubsciberCount++;
              /* Subscribe to mailbox */
              m2m2_hdr_t *sub_pkt = post_office_create_msg(M2M2_HEADER_SZ + sizeof(post_office_config_t));
              if(sub_pkt != NULL) {
                post_office_config_t *payload = (post_office_config_t *)&sub_pkt->data;
                payload->cmd = POST_OFFICE_CFG_CMD_MAILBOX_SUBSCRIBE;
                payload->sub = pkt->src;
                payload->box = M2M2_ADDR_SYS_FS_STREAM;
                sub_pkt->src = pkt->dest;
                sub_pkt->dest = M2M2_ADDR_POST_OFFICE;
                post_office_send(sub_pkt, &err);
              }
              fs_stream_state = ADI_FS_STREAM_STARTED;
              timeout = ADI_OSAL_TIMEOUT_NONE;
              nSeqNum = 0;
              /* clear variables for new file download */
              num_bytes_processed = 0;
              fs_download_total_pkt_cnt = 0;
              total_packet_cnt_transferred = 0;

#ifdef NAND_READ_TEST
             // g_file_read_time = 0;
             // g_file_read_cnt = 0;
#endif //NAND_READ_TEST
            } else {

              response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_file_sys_cmd_t));
              if (response_mail != NULL) {
                m2m2_file_sys_cmd_t *err_resp = (m2m2_file_sys_cmd_t *)&response_mail->data[0];
                err_resp->command = M2M2_FILE_SYS_CMD_DOWNLOAD_LOG_RESP;
                err_resp->status = M2M2_FILE_SYS_STATUS_ERROR;
                response_mail->src = pkt->dest;
                response_mail->dest = pkt->src;
                /* send response packet */
                post_office_send(response_mail, &err);
              }
            }
          }
         }
        break;
      }
      case M2M2_FILE_SYS_CMD_LOG_STREAM_REQ : {
        m2m2_file_sys_log_stream_t *subscribe_req = (m2m2_file_sys_log_stream_t*)&pkt->data[0];
        response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_file_sys_log_stream_t));
        if (response_mail != NULL) {
          m2m2_file_sys_log_stream_t *req = (m2m2_file_sys_log_stream_t*)&response_mail->data[0];
          /* Check for destination of stream */
          response_mail->dest = get_file_routing_table_entry(subscribe_req->stream);
          if (response_mail->dest == M2M2_ADDR_UNDEFINED) {
            req->command = M2M2_FILE_SYS_CMD_LOG_STREAM_RESP;
            req->status = M2M2_FILE_SYS_STATUS_ERROR;
            req->stream = subscribe_req->stream;
            response_mail->dest = pkt->src;
            response_mail->src = pkt->dest;
            SubsGlobalSrc = M2M2_ADDR_UNDEFINED;
            /* Send response packet */
            post_office_send(response_mail, &err);
          } else {
            /* Send subscription request to PS */
            SubsGlobalSrc = pkt->src;
            req->command = M2M2_APP_COMMON_CMD_STREAM_SUBSCRIBE_REQ;
            req->status =  (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_APP_COMMON_STATUS_OK;
            req->stream = subscribe_req->stream;
            response_mail->src = pkt->dest;
            /* Send subscription packet */
            post_office_send(response_mail, &err);
          }
        }
        break;
      } case M2M2_FILE_SYS_CMD_GET_REQ: {
        if(fs_stream_state != ADI_FS_STREAM_STARTED) {
          memset(&fs_stream.file_path,0,sizeof(fs_stream.file_path));
          m2m2_file_sys_get_req_t *get_file_req = (m2m2_file_sys_get_req_t *)&pkt->data[0];
          snprintf(fs_stream.file_path, ((pkt->length - M2M2_HEADER_SZ - 1)) , \
            "%s", get_file_req->file_name);
          memset(&get_file_req->file_name, 0,
                 pkt->length - (M2M2_HEADER_SZ + \
                   sizeof(m2m2_file_sys_get_req_t) - 1));

          /* Power On and mount flash memory */
          if(fs_flash_power_on(true) != FS_STATUS_OK) {
            response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_file_sys_cmd_t));
            if (response_mail != NULL) {
              m2m2_file_sys_cmd_t *err_resp = (m2m2_file_sys_cmd_t *)&response_mail->data[0];
              err_resp->command = (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_FILE_SYS_CMD_GET_RESP;
              err_resp->status = (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_FILE_SYS_STATUS_ERROR;
              response_mail->src = pkt->dest;
              response_mail->dest = pkt->src;
              /* send response packet */
              post_office_send(response_mail, &err);
            }
          } else {
            fs_stream.fs_buffer_size = sizeof(fs_stream.file_buff);
            fs_stream.processed_data_len = 0;
            fs_stream.fs_err_status = fs_hal_read_file(fs_stream.file_path,
                                                       &fs_stream.file_buff[0],
                                                       &fs_stream.fs_buffer_size);
            fs_status_stored = fs_stream.fs_err_status;
            if((fs_stream.fs_err_status == FS_STATUS_OK)
                  || (fs_stream.fs_err_status == FS_STATUS_ERR_EOF)) {
              fs_stream.num_bytes_left = fs_stream.fs_buffer_size;
              fs_stream.pkt_src = pkt->src;
              fs_stream.pkt_dest = pkt->dest;

              fs_stream_state = ADI_FS_STREAM_STARTED;
              timeout = ADI_OSAL_TIMEOUT_NONE;
              nSeqNum = 0;
            } else {
              response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_file_sys_cmd_t));
              if (response_mail != NULL) {
                m2m2_file_sys_cmd_t *err_resp = (m2m2_file_sys_cmd_t *)&response_mail->data[0];
                err_resp->command = (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_FILE_SYS_CMD_GET_RESP;
                err_resp->status = (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_FILE_SYS_STATUS_ERROR;
                response_mail->src = pkt->dest;
                response_mail->dest = pkt->src;
                /* send response packet */
                post_office_send(response_mail, &err);
              }
            }
          }
        }
        break;
      } case M2M2_APP_COMMON_CMD_STREAM_SUBSCRIBE_RESP: {
        int16_t Table_Index;
        m2m2_file_sys_log_stream_t *p_payload = (m2m2_file_sys_log_stream_t*)&pkt->data[0];
        response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_file_sys_log_stream_t));
        if (response_mail != NULL) {
          m2m2_file_sys_log_stream_t *resp = (m2m2_file_sys_log_stream_t*)&response_mail->data[0];
          /* Update FS table subscription status */
          Table_Index = get_file_routing_table_index(p_payload->stream);
          if(Table_Index != -1)
            file_routing_table[Table_Index].fs_subs_state = M2M2_FILE_SYS_SUBSCRIBED;
          /* Send response to APP */
          resp->command = M2M2_FILE_SYS_CMD_LOG_STREAM_RESP;
          if (p_payload->status == M2M2_APP_COMMON_STATUS_SUBSCRIBER_ADDED) {
            resp->status = M2M2_APP_COMMON_STATUS_SUBSCRIBER_ADDED;
            NRF_LOG_INFO("status = %d",resp->status);
          } else {
            resp->status = M2M2_FILE_SYS_STATUS_ERROR;
          }
          resp->stream = p_payload->stream;
          response_mail->dest = SubsGlobalSrc;
          response_mail->src = pkt->dest;
          SubsGlobalSrc = M2M2_ADDR_UNDEFINED;
          /* Send response packet */
          post_office_send(response_mail, &err);
        }
        break;
      } case M2M2_FILE_SYS_CMD_STOP_STREAM_REQ : {
        m2m2_file_sys_log_stream_t *req_payload= (m2m2_file_sys_log_stream_t*)&pkt->data[0];
        response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_file_sys_log_stream_t));
        if (response_mail != NULL) {
          m2m2_file_sys_log_stream_t *resp_payload = (m2m2_file_sys_log_stream_t*)&response_mail->data[0];
          response_mail->dest = get_file_routing_table_entry(req_payload->stream);
          if (response_mail->dest != M2M2_ADDR_UNDEFINED) {
            /* Send stream unsubscribe request */
            SubsGlobalSrc = pkt->src;
            resp_payload->command = M2M2_APP_COMMON_CMD_STREAM_UNSUBSCRIBE_REQ;
            resp_payload->status =  (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_APP_COMMON_STATUS_OK;
          } else {
            /* Send error resp */
            resp_payload->command = M2M2_FILE_SYS_CMD_STOP_STREAM_RESP;
            resp_payload->status =  M2M2_FILE_SYS_STATUS_ERROR;
            response_mail->dest = pkt->src;
          }
          /* Send response packet */
          resp_payload->stream = req_payload->stream;
          response_mail->src = pkt->dest;
          post_office_send(response_mail, &err);
        }
        break;
      }case M2M2_FILE_SYS_CMD_GET_IMPT_DEBUG_INFO_REQ : {
        response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_file_sys_debug_impt_info_resp_t));
        if (response_mail != NULL) {
          /* flash power on */
          if(fs_flash_power_on(true) != FS_STATUS_OK){
              NRF_LOG_INFO("Error in flash power on");
          }
          m2m2_file_sys_debug_impt_info_resp_t *resp_payload = (m2m2_file_sys_debug_impt_info_resp_t*)&response_mail->data[0];
          uint32_t head_pointer;
          uint32_t tail_pointer;
          uint16_t table_page_flags[MAX_NUM_OF_VARIABLES_IN_TABLE_PAGE];
          memset(table_page_flags,0,MAX_NUM_OF_VARIABLES_IN_TABLE_PAGE);
          if(get_pointers_info(&head_pointer,&tail_pointer,table_page_flags) != 0) {
            resp_payload->head_pointer = 0;
            resp_payload->tail_pointer = 0;
            resp_payload->tail_pointer = 0;
            resp_payload->init_circular_buffer_flag = 0;
            resp_payload->mem_full_flag = 0;
            resp_payload->data_offset = 0;
            resp_payload->config_file_occupied = 0;
            resp_payload->status =  M2M2_FILE_SYS_STATUS_ERROR;
          } else {
            resp_payload->head_pointer = head_pointer;
            resp_payload->tail_pointer = tail_pointer;
            resp_payload->init_circular_buffer_flag = table_page_flags[0];
            resp_payload->mem_full_flag = table_page_flags[1];
            resp_payload->data_offset = table_page_flags[2];
            resp_payload->config_file_occupied = table_page_flags[3];
          }
#ifdef ENABLE_WATCH_DISPLAY
           resp_payload->fs_display_query_cnt = fs_display_query_cnt;
           resp_payload->min_timer_cnt = min_timer_cnt;
#else
           resp_payload->fs_display_query_cnt = 0;
           resp_payload->min_timer_cnt = 0;
#endif
#ifdef PROFILE_TIME_ENABLED
          resp_payload->page_read_time = avg_file_read_time;
          resp_payload->usb_avg_tx_time = usb_avg_tx_time;
          resp_payload->usb_avg_port_write_time = usb_avg_cdc_write_time;
          resp_payload->page_write_time = g_page_hold_write_time;
#else
          resp_payload->usb_avg_tx_time = 0;
          resp_payload->usb_avg_port_write_time = 0;
          resp_payload->page_read_time = 0;
          resp_payload->page_write_time = 0;
#endif

          /* Send response packet */
          resp_payload->command = M2M2_FILE_SYS_CMD_GET_IMPT_DEBUG_INFO_RESP;
          resp_payload->status = (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_APP_COMMON_STATUS_OK;
          response_mail->dest = pkt->src;
          response_mail->src = pkt->dest;
          post_office_send(response_mail, &err);
          /* flash power on */
          if(fs_flash_power_on(false) != FS_STATUS_OK){
           NRF_LOG_INFO("Error in flash power off");
          }
        }
        break;
      }case M2M2_APP_COMMON_CMD_STREAM_UNSUBSCRIBE_RESP : {
        int16_t Table_Index;
        m2m2_file_sys_log_stream_t *p_payload = (m2m2_file_sys_log_stream_t*)&pkt->data[0];
        response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_file_sys_log_stream_t));
        if (response_mail != NULL) {
          m2m2_file_sys_log_stream_t *resp_payload = (m2m2_file_sys_log_stream_t*)&response_mail->data[0];
            /* Update FS table for unsubscription */
            Table_Index = get_file_routing_table_index(p_payload->stream);
            if(Table_Index != -1)
              file_routing_table[Table_Index].fs_subs_state = M2M2_FILE_SYS_UNSUBSCRIBED;
            if(SubsGlobalSrc != M2M2_ADDR_SYS_FS) {
              /* Send response to APP */
              resp_payload->command = M2M2_FILE_SYS_CMD_STOP_STREAM_RESP;
              if (p_payload->status == M2M2_APP_COMMON_STATUS_SUBSCRIBER_REMOVED){
                resp_payload->status = M2M2_APP_COMMON_STATUS_SUBSCRIBER_REMOVED;
               }
               else if(p_payload->status == M2M2_APP_COMMON_STATUS_SUBSCRIBER_COUNT_DECREMENT) {
                  resp_payload->status = M2M2_APP_COMMON_STATUS_SUBSCRIBER_COUNT_DECREMENT;
                } else {
                  resp_payload->status = M2M2_FILE_SYS_STATUS_ERROR;
                }
              response_mail->dest = SubsGlobalSrc;
            } else {
              /* Send Force stop log req */
              resp_payload->command = M2M2_FILE_SYS_CMD_STOP_LOGGING_REQ;
              resp_payload->status = M2M2_FILE_SYS_STATUS_OK;
              response_mail->dest = M2M2_ADDR_SYS_FS;
            }
            /* Send response packet */
            resp_payload->stream = p_payload->stream;
            response_mail->src = pkt->dest;
            SubsGlobalSrc = M2M2_ADDR_UNDEFINED;
            post_office_send(response_mail, &err);
        }
        break;
      } case M2M2_FILE_SYS_CMD_DELETE_CONFIG_FILE_REQ: {
        FS_STATUS_ENUM_t fs_err_status = FS_STATUS_ERR;
        response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_file_sys_cmd_t));
        if (response_mail != NULL) {
          m2m2_file_sys_cmd_t *file_stream_start_resp = (m2m2_file_sys_cmd_t *)&response_mail->data[0];
          /* if file writing in progress, do not delete config file, */
          if(fs_hal_write_access_state() != FS_FILE_ACCESS_IN_PROGRESS) {
            fs_flash_power_on(true);
            /* Check if config file already exist,
            if so delete it to update the new one */
            fs_err_status = fs_hal_delete_config_file(ConfigFileName, sizeof(ConfigFileName));
            file_stream_start_resp->command = M2M2_FILE_SYS_CMD_DELETE_CONFIG_FILE_RESP;
            if(fs_err_status == FS_STATUS_ERR){
              file_stream_start_resp->status = (M2M2_FILE_SYS_STATUS_ENUM_t)M2M2_FILE_SYS_STATUS_ERROR;
            }
            else if(fs_err_status == FS_STATUS_OK){
              file_stream_start_resp->status = (M2M2_FILE_SYS_STATUS_ENUM_t) M2M2_FILE_SYS_STATUS_OK;
            }
            else if(fs_err_status == FS_STATUS_ERR_CONFIG_FILE_NOT_FOUND){
              file_stream_start_resp->status = (M2M2_FILE_SYS_STATUS_ENUM_t) M2M2_FILE_SYS_CONFIG_FILE_NOT_FOUND;
            }
            fs_flash_power_on(false);
#ifdef LOW_TOUCH_FEATURE
            /*update the config file availability flag*/
            SetCfgFileAvailableFlag(false);
            NRF_LOG_INFO("LT NAND User Cfg file deleted");
            if( check_lt_app_capsense_tuned_trigger_status() )  {
              if (!gen_blk_get_dcb_present_flag() && !gsCfgFileFoundFlag)
                EnableLowTouchDetection(false);
            }
#endif
           }
           /* send logging in progress */
           else{
            file_stream_start_resp->command = M2M2_FILE_SYS_CMD_DELETE_CONFIG_FILE_RESP;
            file_stream_start_resp->status = M2M2_FILE_SYS_STATUS_LOGGING_IN_PROGRESS;
           }
            /* send response packet */
            response_mail->src = pkt->dest;
            response_mail->dest = pkt->src;
            post_office_send(response_mail, &err);
          }
        break;
      }
#ifdef LOW_TOUCH_FEATURE
      case M2M2_FILE_SYS_CMD_FORCE_STOP_LOG_REQ: {
        FS_STATUS_ENUM_t fs_error;

        if(FileSrc != M2M2_ADDR_UNDEFINED) {
          /* Send stream stop request */
          fs_unsubscribe_streams();
            /* Stop File logging */
            fs_error = fs_hal_close_file(fs_hal_write_file_pointer());
            if(fs_error == FS_STATUS_OK) {
              fs_flash_power_on(false);
            /* Create Response packet */
            response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(_m2m2_app_common_cmd_t));
            if(response_mail != NULL) {
              _m2m2_app_common_cmd_t *resp_payload = (_m2m2_app_common_cmd_t *)&response_mail->data[0];
              response_mail->src = M2M2_ADDR_SYS_FS;
              if(gSendRespToPmApp) {
                gSendRespToPmApp = 0;
                response_mail->dest = M2M2_ADDR_SYS_PM;
              } else {
                response_mail->dest = FileSrc;
              }
              resp_payload->command = M2M2_FILE_SYS_CMD_FORCE_STOP_LOG_RESP;
              if(fs_error == FS_STATUS_OK) {
                FileSrc = M2M2_ADDR_UNDEFINED;
                if(gMemoryFull == true) {
                  gMemoryFull = false;
                  resp_payload->status = (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_FILE_SYS_ERR_MEMORY_FULL;
                } else {
                  resp_payload->status = (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_FILE_SYS_ERR_LOG_FORCE_STOPPED;
                }
              } else {
                resp_payload->status = M2M2_APP_COMMON_STATUS_ERROR;
              }
              /* send response packet */
              post_office_send(response_mail, &err);
            }
            /*error handling for closing file which is not proper */
          }
        } else {
          /* Send log not started response */
          response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(_m2m2_app_common_cmd_t));
          if(response_mail != NULL) {
            _m2m2_app_common_cmd_t *resp_payload = (_m2m2_app_common_cmd_t *)&response_mail->data[0];
            resp_payload->command = M2M2_FILE_SYS_CMD_FORCE_STOP_LOG_RESP;
            response_mail->src = pkt->dest;
            response_mail->dest = pkt->src;
            resp_payload->status = M2M2_APP_COMMON_STATUS_STREAM_NOT_STARTED;
            /* send response packet */
            post_office_send(response_mail, &err);
          }
        }
        break;
      }
#endif
      case M2M2_FILE_SYS_CMD_DCFG_START_LOG_REQ: {
        uint8_t cfgFileFoundFlag=0;
        FS_STATUS_ENUM_t fs_err_status = FS_STATUS_ERR;
        response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_file_sys_cmd_t));
        if (response_mail != NULL) {
            m2m2_file_sys_cmd_t *file_stream_start_resp = (m2m2_file_sys_cmd_t *)&response_mail->data[0];
            file_stream_start_resp->command = M2M2_FILE_SYS_CMD_DCFG_START_LOG_RESP;
            if(fs_hal_write_access_state() == FS_FILE_ACCESS_IN_PROGRESS) {
              /* send file is accessed send in progress status */
              file_stream_start_resp->status = M2M2_FILE_SYS_STATUS_LOGGING_IN_PROGRESS;
            }
          else{
              /* power on only if flash operations are required */
              fs_flash_power_on(true);
              /* find if config file is found, if yes return error */
              fs_hal_find_config_file(ConfigFileName, sizeof(ConfigFileName),0, &cfgFileFoundFlag);
              if(cfgFileFoundFlag){
                /* config file is found, hence cannot create new config file */
                file_stream_start_resp->status = (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_FILE_SYS_CONFIG_FILE_FOUND;
              }
                /* if config file is not present create new config file */
              else{
                 /* create the new config file */
                  fs_err_status = fs_hal_open_config_file(ConfigFileName);

                  switch(fs_err_status) {
                    case FS_STATUS_OK: {
                      file_stream_start_resp->status = (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_FILE_SYS_STATUS_OK;
                    break;
                    } case FS_STATUS_ERR_INVALID_DIR: {
                      file_stream_start_resp->status = (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_FILE_SYS_ERR_INVALID;
                      break;
                    } case FS_STATUS_ERR_CONFIG_FILE_POSITION: {
                      file_stream_start_resp->status = (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_FILE_SYS_ERR_CONFIG_FILE_POSITION;
                      break;
                    }default: {
                      file_stream_start_resp->status = M2M2_FILE_SYS_STATUS_ERROR;
                      break;
                    }
                  }
                }
              /* power off */
              fs_flash_power_on(false);
            }
            /* send response packet */
            response_mail->src = pkt->dest;
            response_mail->dest = pkt->src;
            post_office_send(response_mail, &err);
        }
        break;
      } case M2M2_FILE_SYS_CMD_DCFG_STOP_LOG_REQ: {
        FS_STATUS_ENUM_t fs_err_status = FS_STATUS_ERR;
        response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_file_sys_cmd_t));
        if (response_mail != NULL) {
          m2m2_file_sys_cmd_t *file_stream_stop_resp = (m2m2_file_sys_cmd_t *)&response_mail->data[0];
          file_stream_stop_resp->command = M2M2_FILE_SYS_CMD_DCFG_STOP_LOG_RESP;
          if(fs_hal_write_access_state() == FS_FILE_ACCESS_IN_PROGRESS){
              fs_err_status = fs_hal_close_file(fs_hal_write_file_pointer());
              if (fs_err_status == FS_STATUS_OK) {
                file_stream_stop_resp->status = M2M2_FILE_SYS_STATUS_OK;
                FileSrc = M2M2_ADDR_UNDEFINED;
#ifdef LOW_TOUCH_FEATURE
                /*update the config file copy availability flag*/
                SetCfgCopyAvailableFlag(false);
                /*update the config file availability flag*/
                SetCfgFileAvailableFlag(true);
                NRF_LOG_INFO("LT NAND User Cfg file written");
                if( check_lt_app_capsense_tuned_trigger_status() )
                {
                  EnableLowTouchDetection(true);
                }
#endif
              } else {
                file_stream_stop_resp->status = M2M2_FILE_SYS_STATUS_ERROR;
              }
          } else {
            file_stream_stop_resp->status = M2M2_FILE_SYS_ERR_INVALID;
          }
          /* send response packet */
          response_mail->src = pkt->dest;
          response_mail->dest = pkt->src;
          post_office_send(response_mail, &err);

          /* Power off Flash */
          fs_flash_power_on(false);
        }
        break;
      } case M2M2_FILE_SYS_CMD_LOG_USER_CONFIG_DATA_REQ: {
          M2M2_APP_COMMON_STATUS_ENUM_t fs_err_status ;
          uint32_t  len_configdata = 0;
          uint8_t configdata_buffer[70];
          m2m2_file_sys_user_config_data *config_data = (m2m2_file_sys_user_config_data*)&pkt->data[0];
          len_configdata = config_data->len_configstream;
          memset(configdata_buffer, 0x00, sizeof(configdata_buffer));
          memcpy(&configdata_buffer[0], &config_data->byte_configstream, len_configdata);
          if(fs_hal_write_access_state() == FS_FILE_ACCESS_IN_PROGRESS) {
            fs_err_status = (M2M2_APP_COMMON_STATUS_ENUM_t)fs_hal_test_pattern_config_write(configdata_buffer, len_configdata);
          }else {
            fs_err_status = (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_FILE_SYS_STATUS_ERROR;
          }
          response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(_m2m2_app_common_cmd_t));
          if (response_mail != NULL) {
            _m2m2_app_common_cmd_t  *ctrl = (_m2m2_app_common_cmd_t *)&response_mail->data[0];
            ctrl->command = M2M2_FILE_SYS_CMD_LOG_USER_CONFIG_DATA_RESP;
            if(fs_err_status != (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_FILE_SYS_ERR_MEMORY_FULL){
              ctrl->status = fs_err_status;
            } else {
              ctrl->status = (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_FILE_SYS_STATUS_OK;
            }
            response_mail->dest = pkt->src;
            response_mail->src =  pkt->dest;
            post_office_send(response_mail, &err);
          }
          break;
      } case M2M2_FILE_SYS_CMD_START_LOGGING_REQ: {
        FS_STATUS_ENUM_t fs_err_status = FS_STATUS_ERR;
        m_time_struct  *local_get_date_time;
        char caTime[7] = { 0 };
        char date[9] = { 0 };
        uint8_t nIndex = 0;
        char fname[40];
        uint8_t nFileCount=0;
        gnadpd_dcfg_tx_PacketCount = 0;

        /* Power On and mount flash memory */
        if(fs_flash_power_on(true) != FS_STATUS_OK) {
          response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_file_sys_cmd_t));
          if (response_mail != NULL) {
            m2m2_file_sys_cmd_t *resp_payload = (m2m2_file_sys_cmd_t *)&response_mail->data[0];
            resp_payload->command = (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_FILE_SYS_CMD_LS_RESP;
            resp_payload->status = (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_FILE_SYS_STATUS_ERROR;
            response_mail->src = pkt->dest;
            response_mail->dest = pkt->src;
            /* send response packet */
            post_office_send(response_mail, &err);
          }
        } else {
          if(fs_hal_write_access_state() == FS_FILE_ACCESS_START) {
#ifdef HIBERNATE_MD_EN
            hibernate_md_clear(HIB_MD_FS_LOG_STOP_EVT);
            hibernate_mode_entry();
#endif
            /* Create file and start logging */
            file_hdr_wr_progress = true;
            /* Generate file name from RTC Time */
            time_t return_time;
            int16_t timezone_offset;

            fs_err_status = fs_hal_get_file_count(&nFileCount);
            if( (fs_err_status == FS_STATUS_OK) && (nFileCount >= (MAXFILENUMBER-1)) )
              fs_err_status = FS_STATUS_ERR_MAX_FILE_COUNT;

            if(fs_err_status == FS_STATUS_OK) {
              /*reintialize the status variable*/
              fs_err_status = FS_STATUS_ERR;
              if( !get_log_time_stamp(&return_time, &timezone_offset)) {
              /* For file name creation timezone_offset is not required */
               local_get_date_time = m_sec_to_date_time(return_time + timezone_offset);
                 do {
                   snprintf(caTime, sizeof(caTime), "%2u%2u%2u", local_get_date_time->tm_hour,
                            local_get_date_time->tm_min,nFileCount+1);
                   /* set spaces to zeros */
                   while (caTime[nIndex] != '\0') {
                     if (caTime[nIndex] == ' ') {
                       caTime[nIndex] = '0';
                     }
                     nIndex++;
                   }
                   nIndex = 0;
                   snprintf(date, sizeof(date), "%1X%2X%5X",
                           local_get_date_time->tm_mon, local_get_date_time->tm_mday, atoi(caTime));
                   local_get_date_time->tm_sec++;
                   if (local_get_date_time->tm_sec > 59) {
                     local_get_date_time->tm_sec = 0;
                     local_get_date_time->tm_min++;
                     if(local_get_date_time->tm_min > 59) {
                       local_get_date_time->tm_min = 0;
                       local_get_date_time->tm_hour++;
                       if (local_get_date_time->tm_hour > 23) {
                         local_get_date_time->tm_hour = 0;
                         /* handled upto valid time values */
                       }
                     }
                   }
                   /* set spaces to zeros */
                   while (date[nIndex] != '\0') {
                     if (date[nIndex] == ' ') {
                       date[nIndex] = '0';
                     }
                     nIndex++;
                   }
                   nIndex = 0;
                   snprintf(fname, sizeof(fname), "%s.LOG\0",&date[0]);
                   fs_err_status = fs_hal_find_file(fname);
                 } while(fs_err_status == FS_STATUS_OK);
               } else {
                 /* use default file name if file name could not be generated */
                 snprintf(fname, sizeof(fname), "%s\\NANDDFLT.LOG", fs_hal_vol_name());
               }
              fs_err_status = fs_hal_open_file(fname);
            }
            if (fs_err_status != FS_STATUS_OK) {
              /* Send response for log start */
              response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_file_sys_cmd_t));
              if (response_mail != NULL) {
                m2m2_file_sys_cmd_t *file_stream_start_resp = (m2m2_file_sys_cmd_t *)&response_mail->data[0];
                file_stream_start_resp->command = M2M2_FILE_SYS_CMD_START_LOGGING_RESP;
                switch(fs_err_status) {
                 case FS_STATUS_ERR_MAX_FILE_COUNT: {
                   file_stream_start_resp->status = M2M2_FILE_SYS_ERR_MAX_FILE_COUNT;
                   break;
                 }
                 case FS_STATUS_ERR_CONFIG_FILE_POSITION: {
                   file_stream_start_resp->status = M2M2_FILE_SYS_ERR_CONFIG_FILE_POSITION;
                   break;
                 }
                 case FS_STATUS_ERR_INVALID_DIR: {
                   file_stream_start_resp->status = M2M2_FILE_SYS_ERR_INVALID;
                   break;
                 }
                 default: {
                   file_stream_start_resp->status = M2M2_FILE_SYS_STATUS_ERROR;
                   break;
                 }
                }
                /* send response packet */
                response_mail->src = pkt->dest;
                response_mail->dest = pkt->src;
                post_office_send(response_mail, &err);
              }
            } else {
              FileSrc = pkt->src;
              if((g_userinfo.keyID != M2M2_DEFAULT_KEY_ID)){
                /* Create Key Value pair packet */
                response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_file_sys_set_key_value_pair_resp_t));
                if (response_mail != NULL) {
                  m2m2_file_sys_set_key_value_pair_resp_t *payloadUserinfo = (m2m2_file_sys_set_key_value_pair_resp_t *)&response_mail->data[0];
                  memcpy(payloadUserinfo,(void *)&g_userinfo, sizeof(m2m2_file_sys_set_key_value_pair_resp_t));
                  payloadUserinfo->command = (M2M2_FILE_SYS_CMD_ENUM_t) M2M2_FILE_SYS_CMD_SET_KEY_VALUE_PAIR_RESP;
                  payloadUserinfo->status = M2M2_FILE_SYS_STATUS_OK;
                  response_mail->src = pkt->dest;
                  response_mail->dest = pkt->src;

                  /* 1 - Write Key Value Pair to Flash */
                  if (fs_hal_write_packet_stream(response_mail) != M2M2_APP_COMMON_STATUS_OK) {
                    /* Send Error response if write to Flash fails */
                    fs_send_write_error(&FileSrc);
                  }
                  post_office_consume_msg(response_mail);
                }
              }
              /* 2 - Request PM Date and time Information */
              response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(_m2m2_app_common_cmd_t));
              if (response_mail != NULL) {
                _m2m2_app_common_cmd_t *get_datetime_req = (_m2m2_app_common_cmd_t *)&response_mail->data[0];
                get_datetime_req->command = (M2M2_APP_COMMON_CMD_ENUM_t) M2M2_PM_SYS_COMMAND_GET_DATE_TIME_REQ;
                get_datetime_req->status = M2M2_APP_COMMON_STATUS_OK;
                /* send response packet */
                response_mail->src = M2M2_ADDR_SYS_FS;
                response_mail->dest = M2M2_ADDR_SYS_PM;
                post_office_send(response_mail, &err);
              }
            }
#ifdef ENABLE_WATCH_DISPLAY
            reset_display_vol_info();
            //Update the NAND memory progress bar in Main menu page of Display
            send_private_type_value(DIS_REFRESH_SIGNAL);
#endif
          } else {
            /* Send Error response as logging is already in progress */
            response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(_m2m2_app_common_cmd_t));
            if (response_mail != NULL) {
              _m2m2_app_common_cmd_t *file_stream_start_resp = (_m2m2_app_common_cmd_t *)&response_mail->data[0];
              file_stream_start_resp->command = M2M2_FILE_SYS_CMD_START_LOGGING_RESP;
              file_stream_start_resp->status = M2M2_FILE_SYS_STATUS_LOGGING_IN_PROGRESS;
              /* send response packet */
              response_mail->src = pkt->dest;
              response_mail->dest = pkt->src;
              post_office_send(response_mail, &err);
            }
          }
        }
        break;
      }
      /* Resp for Date time req(step2) */
      case M2M2_PM_SYS_COMMAND_GET_DATE_TIME_RESP: {
        if (fs_hal_write_packet_stream(pkt) != M2M2_APP_COMMON_STATUS_OK) {
          /* Send Error response if write to Flash fails */
          fs_send_write_error(&FileSrc);
          file_hdr_wr_progress = true;
        } else {
            /* 3 - request PM system info */
            response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(_m2m2_app_common_cmd_t));
            if (response_mail != NULL) {
              _m2m2_app_common_cmd_t *get_pm_sysinfo_req = (_m2m2_app_common_cmd_t *)&response_mail->data[0];
              get_pm_sysinfo_req->command = (M2M2_APP_COMMON_CMD_ENUM_t) M2M2_PM_SYS_COMMAND_GET_INFO_REQ;
              get_pm_sysinfo_req->status = M2M2_APP_COMMON_STATUS_OK;
              /* send response packet */
              response_mail->src = M2M2_ADDR_SYS_FS;
              response_mail->dest = M2M2_ADDR_SYS_PM;
              post_office_send(response_mail, &err);
            }
        }
        break;
      }
      /* Response for PM system info req(step3) */
      case M2M2_PM_SYS_COMMAND_GET_INFO_RESP: {
        if (fs_hal_write_packet_stream(pkt) != M2M2_APP_COMMON_STATUS_OK) {
          /* Send Error response if write to Flash fails */
          fs_send_write_error(&FileSrc);
          file_hdr_wr_progress = true;
        } else {
          /* 4 - request PM version Information */
          response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_app_common_version_t));
          if (response_mail != NULL) {
            m2m2_app_common_version_t *version_num_req = (m2m2_app_common_version_t *)&response_mail->data[0];
            version_num_req->command = M2M2_APP_COMMON_CMD_GET_VERSION_REQ;
            version_num_req->status = M2M2_APP_COMMON_STATUS_OK;
            /* send response packet */
            response_mail->src = M2M2_ADDR_SYS_FS;
            response_mail->dest = M2M2_ADDR_SYS_PM;
            post_office_send(response_mail, &err);
          }
        }
        break;
      }
      //Response for Get Version info(step 4 & 5)
      case M2M2_APP_COMMON_CMD_GET_VERSION_RESP: {
        /* Response for Version info req(step 4) */
        if (pkt->src == M2M2_ADDR_SYS_PM) {
          if (fs_hal_write_packet_stream(pkt) != M2M2_APP_COMMON_STATUS_OK) {
            /* Send Error response if write to Flash fails */
            fs_send_write_error(&FileSrc);
            file_hdr_wr_progress = true;
          } else {
#ifdef ENABLE_PPG_APP
          response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(_m2m2_app_common_cmd_t));
          if (response_mail != NULL) {
              /* 6 - request PPG Algo version */
            _m2m2_app_common_cmd_t *algo_version = (_m2m2_app_common_cmd_t *)&response_mail->data[0];
            algo_version->command = M2M2_PPG_APP_CMD_GET_ALGO_VENDOR_VERSION_REQ;
            response_mail->dest = M2M2_ADDR_MED_PPG;
            algo_version->status = M2M2_APP_COMMON_STATUS_OK;
            /* send response packet */
             response_mail->src = M2M2_ADDR_SYS_FS;
             post_office_send(response_mail, &err);
            }
          }
        }
        break;
      }
      /* Response for PPG Algo request(step 6) */
      case M2M2_PPG_APP_CMD_GET_ALGO_VENDOR_VERSION_RESP: {
        if (fs_hal_write_packet_stream(pkt) != M2M2_APP_COMMON_STATUS_OK) {
          /* Send Error response if write to Flash fails */
          fs_send_write_error(&FileSrc);
          file_hdr_wr_progress = true;
        } else {
#endif
#ifdef ENABLE_PEDO_APP
          /* 7 - request PED Algo version */
          response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(_m2m2_app_common_cmd_t));
          if (response_mail != NULL) {
            _m2m2_app_common_cmd_t *algo_version = (_m2m2_app_common_cmd_t *)&response_mail->data[0];
            algo_version->command = M2M2_PED_APP_CMD_GET_ALGO_VENDOR_VERSION_REQ;
            algo_version->status = M2M2_APP_COMMON_STATUS_OK;
            /* send response packet */
            response_mail->src = M2M2_ADDR_SYS_FS;
            response_mail->dest = M2M2_ADDR_MED_PED;
            post_office_send(response_mail, &err);
          }
        }
#ifndef ENABLE_PPG_APP
      }
#endif
      break;
    }
      /* Response for PED Algo request(step 7) */
      case M2M2_PED_APP_CMD_GET_ALGO_VENDOR_VERSION_RESP: {
        if (fs_hal_write_packet_stream(pkt) != M2M2_APP_COMMON_STATUS_OK) {
          /* Send Error response if write to Flash fails */
          fs_send_write_error(&FileSrc);
          file_hdr_wr_progress = true;
        } else {
#endif
#ifdef ENABLE_ECG_APP
          /* 8 - request ECG Algo version */
          response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(_m2m2_app_common_cmd_t));
          if (response_mail != NULL) {
            _m2m2_app_common_cmd_t *algo_version = (_m2m2_app_common_cmd_t *)&response_mail->data[0];
            algo_version->command = M2M2_ECG_APP_CMD_GET_ALGO_VENDOR_VERSION_REQ;
            algo_version->status = M2M2_APP_COMMON_STATUS_OK;
            /* send response packet */
            response_mail->src = M2M2_ADDR_SYS_FS;
            response_mail->dest = M2M2_ADDR_MED_ECG;
            post_office_send(response_mail, &err);
          }
        }
#if !defined(ENABLE_PPG_APP) && !defined(ENABLE_PEDO_APP)
       }
#endif
        break;
      }
      /* Response for ECG Algo request(step 8) */
      case M2M2_ECG_APP_CMD_GET_ALGO_VENDOR_VERSION_RESP: {
        if (fs_hal_write_packet_stream(pkt) != M2M2_APP_COMMON_STATUS_OK) {
          /* Send Error response if write to Flash fails */
          fs_send_write_error(&FileSrc);
          file_hdr_wr_progress = true;
        } else {
#endif
#ifdef ENABLE_SQI_APP
          /* 9 - request SQI Algo version */
          response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(_m2m2_app_common_cmd_t));
          if (response_mail != NULL) {
            _m2m2_app_common_cmd_t *algo_version = (_m2m2_app_common_cmd_t *)&response_mail->data[0];
            algo_version->command = M2M2_SQI_APP_CMD_GET_ALGO_VENDOR_VERSION_REQ;
            algo_version->status = M2M2_APP_COMMON_STATUS_OK;
            /* send response packet */
            response_mail->src = M2M2_ADDR_SYS_FS;
            response_mail->dest = M2M2_ADDR_MED_SQI;
            post_office_send(response_mail, &err);
          }
        }
#if !defined(ENABLE_PPG_APP) && !defined(ENABLE_PEDO_APP) && !defined(ENABLE_ECG_APP)
       }
#endif
        break;
      }
      /* Response for SQI Algo request(step 9) */
      case M2M2_SQI_APP_CMD_GET_ALGO_VENDOR_VERSION_RESP: {
        if (fs_hal_write_packet_stream(pkt) != M2M2_APP_COMMON_STATUS_OK) {
          /* Send Error response if write to Flash fails */
          fs_send_write_error(&FileSrc);
          file_hdr_wr_progress = true;
        } else {
#endif
          /* 10 - request ADPD DCFG */
          response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_file_sys_cmd_t));
          if (response_mail != NULL) {
            m2m2_file_sys_cmd_t *dcfg_req = (m2m2_file_sys_cmd_t *)&response_mail->data[0];

            if((get_address_sub_state(M2M2_ADDR_SENSOR_ADPD4000) == M2M2_FILE_SYS_SUBSCRIBED) ||
               (get_address_sub_state(M2M2_ADDR_MED_PPG) == M2M2_FILE_SYS_SUBSCRIBED)){
                 dcfg_req->command = (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_SENSOR_COMMON_CMD_GET_DCFG_REQ;
                 response_mail->dest = M2M2_ADDR_SENSOR_ADPD4000;
                 response_mail->src = M2M2_ADDR_SYS_FS;
               } else {
                 /* Send Duplicate response to FS itself if ADPD is
                    not subscribed to go to step 10 */
                 dcfg_req->command = (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_SENSOR_COMMON_CMD_GET_DCFG_RESP;
                 response_mail->dest = M2M2_ADDR_SYS_FS;
                 response_mail->src = M2M2_ADDR_SENSOR_ADPD4000;
               }
            dcfg_req->status = M2M2_APP_COMMON_STATUS_OK;
            /* send response packet */
            post_office_send(response_mail, &err);
          }
        }
#if !defined(ENABLE_PPG_APP) && !defined(ENABLE_PEDO_APP) && !defined(ENABLE_ECG_APP) && !defined(ENABLE_SQI_APP)
       }
#endif
        break;
      }
      /* Response to ADPD and ADXL DCFG req(step 9 & 10) */
      case M2M2_SENSOR_COMMON_CMD_GET_DCFG_RESP: {
        if (pkt->src == M2M2_ADDR_SENSOR_ADPD4000) {
          /*if TX packets from ADPD DCFG more than 1 wait for the next ADPD DCFG packet*/
          if (pkt->length > sizeof(m2m2_sensor_dcfg_data_t)) { // To avoid dummy reuest from FS
            m2m2_sensor_dcfg_data_t *dcfg_data_req = (m2m2_sensor_dcfg_data_t *)&pkt->data[0];
            if(gnadpd_dcfg_tx_PacketCount == 0)
              gnadpd_dcfg_tx_PacketCount = dcfg_data_req->num_tx_pkts;
          }
          M2M2_APP_COMMON_STATUS_ENUM_t err_status = M2M2_APP_COMMON_STATUS_OK;
          if ((get_address_sub_state(pkt->src) == M2M2_FILE_SYS_SUBSCRIBED) ||
              (get_address_sub_state(M2M2_ADDR_MED_PPG) == M2M2_FILE_SYS_SUBSCRIBED)) {
                err_status = fs_hal_write_packet_stream(pkt);
            }
          if (err_status != M2M2_APP_COMMON_STATUS_OK) {
            /* Send Error response if write to Flash fails */
            fs_send_write_error(&FileSrc);
            file_hdr_wr_progress = true;
          } else {
            /* 10 - request ADXL DCFG */
            if(gnadpd_dcfg_tx_PacketCount > 0)
              --gnadpd_dcfg_tx_PacketCount;
            if(gnadpd_dcfg_tx_PacketCount == 0) {// send ADXL DCFG request once the ADPD DCFG tx packets received
              response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_file_sys_cmd_t));
              if (response_mail != NULL) {
                m2m2_file_sys_cmd_t *dcfg_req = (m2m2_file_sys_cmd_t *)&response_mail->data[0];
                if((get_address_sub_state(M2M2_ADDR_SENSOR_ADXL) == M2M2_FILE_SYS_SUBSCRIBED) ||
                   (get_address_sub_state(M2M2_ADDR_MED_PPG) == M2M2_FILE_SYS_SUBSCRIBED)){
                     dcfg_req->command = (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_SENSOR_COMMON_CMD_GET_DCFG_REQ;
                     response_mail->dest = M2M2_ADDR_SENSOR_ADXL;
                     response_mail->src = M2M2_ADDR_SYS_FS;
                   } else {
                     /* Send duplicate response to FS itself
                        if ADXL is not subscribed to go to step 11 */
                     dcfg_req->command = (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_SENSOR_COMMON_CMD_GET_DCFG_RESP;
                     response_mail->dest = M2M2_ADDR_SYS_FS;
                     response_mail->src = M2M2_ADDR_SENSOR_ADXL;
                   }
                dcfg_req->status = M2M2_APP_COMMON_STATUS_OK;
                post_office_send(response_mail, &err);
              }
            }
          }
        } else if (pkt->src == M2M2_ADDR_SENSOR_ADXL) {
          M2M2_APP_COMMON_STATUS_ENUM_t err_status = M2M2_APP_COMMON_STATUS_OK;
          if ((get_address_sub_state(pkt->src) == M2M2_FILE_SYS_SUBSCRIBED) ||
              (get_address_sub_state(M2M2_ADDR_MED_PPG) == M2M2_FILE_SYS_SUBSCRIBED)) {
                err_status = fs_hal_write_packet_stream(pkt);
          }
          if (err_status != M2M2_APP_COMMON_STATUS_OK) {
            /* Send Error response if write to Flash fails */
            fs_send_write_error(&FileSrc);
            file_hdr_wr_progress = true;
          } else {
#ifdef ENABLE_PPG_APP
            /* 11 - request PPG LCFG */
            response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_file_sys_cmd_t));
            if (response_mail != NULL) {
              m2m2_file_sys_cmd_t *lcfg_req = (m2m2_file_sys_cmd_t *)&response_mail->data[0];
              if(get_address_sub_state(M2M2_ADDR_MED_PPG) == M2M2_FILE_SYS_SUBSCRIBED) {
                lcfg_req->command = M2M2_APP_COMMON_CMD_GET_LCFG_REQ;
                response_mail->dest = M2M2_ADDR_MED_PPG;
                response_mail->src = M2M2_ADDR_SYS_FS;
              } else {
                /* Send duplicate response to FS itself
                   if PPG is not subscribed to go to step 12 */
                lcfg_req->command = M2M2_APP_COMMON_CMD_GET_LCFG_RESP;
                response_mail->dest = M2M2_ADDR_SYS_FS;
                response_mail->src = M2M2_ADDR_MED_PPG;
              }
              lcfg_req->status = M2M2_APP_COMMON_STATUS_OK;
              post_office_send(response_mail, &err);
            }
          }
        }
        break;
      }
      /* Response for PPG LCFG request(step 11) */
      case M2M2_APP_COMMON_CMD_GET_LCFG_RESP: {
        if (pkt->src == M2M2_ADDR_MED_PPG) {
          M2M2_APP_COMMON_STATUS_ENUM_t err_status = M2M2_APP_COMMON_STATUS_OK;
          if (get_address_sub_state(pkt->src) == M2M2_FILE_SYS_SUBSCRIBED) {
            err_status = fs_hal_write_packet_stream(pkt);
          }
          if (err_status != M2M2_APP_COMMON_STATUS_OK) {
            /* Send Error response if write to flash fails */
            fs_send_write_error(&FileSrc);
            file_hdr_wr_progress = true;
          } else {
#endif
#ifdef ENABLE_ECG_APP
            /* 12 - request ECG LCFG */
            response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(ecg_app_lcfg_op_hdr_t) + FS_ECG_APP_MAX_LCFG_OPS * sizeof(ecg_app_lcfg_op_t));
            if (response_mail != NULL) {
              uint8_t field = 0;
              ecg_app_lcfg_op_hdr_t *lcfgecg_read_req = (ecg_app_lcfg_op_hdr_t *)&response_mail->data[0];
              if(get_address_sub_state(M2M2_ADDR_MED_ECG) == M2M2_FILE_SYS_SUBSCRIBED) {
                lcfgecg_read_req->num_ops = FS_ECG_APP_MAX_LCFG_OPS;
                for (int i = 0; i < lcfgecg_read_req->num_ops; i++) {
                  lcfgecg_read_req->ops[i].field = field++;
                  lcfgecg_read_req->ops[i].value = 0;
                }
                lcfgecg_read_req->command = (M2M2_APP_COMMON_CMD_ENUM_t) M2M2_APP_COMMON_CMD_READ_LCFG_REQ;
                response_mail->dest = M2M2_ADDR_MED_ECG;
                response_mail->src = M2M2_ADDR_SYS_FS;
              } else {
                /* Send duplicate response to FS itself
                    if ECG is not subscribed to go to step 13 */
                lcfgecg_read_req->command = (M2M2_APP_COMMON_CMD_ENUM_t) M2M2_APP_COMMON_CMD_READ_LCFG_RESP;
                response_mail->dest = M2M2_ADDR_SYS_FS;
                response_mail->src = M2M2_ADDR_MED_ECG;
              }
              lcfgecg_read_req->status = M2M2_APP_COMMON_STATUS_OK;
              /* send response packet */
              post_office_send(response_mail, &err);
              }
          }
        }
        break;
      }
      /* Response for ECG LCFG request(step 12) */
      case M2M2_APP_COMMON_CMD_READ_LCFG_RESP: {
        if (pkt->src == M2M2_ADDR_MED_ECG) {
          M2M2_APP_COMMON_STATUS_ENUM_t err_status = M2M2_APP_COMMON_STATUS_OK;
          if (get_address_sub_state(pkt->src) == M2M2_FILE_SYS_SUBSCRIBED) {
            err_status = fs_hal_write_packet_stream(pkt);
          }
          if (err_status != M2M2_APP_COMMON_STATUS_OK) {
            /* Send Error response if write to Flash fails */
            fs_send_write_error(&FileSrc);
            file_hdr_wr_progress = true;
          } else {
#endif
            response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(_m2m2_app_common_cmd_t));
            if (response_mail != NULL) {
              _m2m2_app_common_cmd_t *file_stream_start_resp = (_m2m2_app_common_cmd_t *)&response_mail->data[0];
              file_stream_start_resp->command = M2M2_FILE_SYS_CMD_START_LOGGING_RESP;
              file_stream_start_resp->status = M2M2_FILE_SYS_STATUS_OK;
              file_hdr_wr_progress = false;
              /* send response packet */
              response_mail->src = pkt->dest;
              response_mail->dest = FileSrc;
              post_office_send(response_mail, &err);
#ifdef DEBUG_LOG_START
              start_log_timer_gap = get_micro_sec() -  start_log_timer_start;
#endif
#ifdef ENABLE_WATCH_DISPLAY
              /* reset variables to track counters for logging */ 
              min_timer_cnt = fs_display_query_cnt = 0;
#endif
            }
          }
        }
        break;
      }
       case M2M2_FILE_SYS_BLOCKS_WRITE_REQ: {
#ifdef PATTERN_WRITE_ENABLED
       m2m2_file_sys_write_blocks_t *write_blocks_req = (m2m2_file_sys_write_blocks_t *)&pkt->data[0];
       uint16_t start_block_num;
       uint16_t num_blocks_write;
       static uint8_t first_time_write=1;
        response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(_m2m2_app_common_cmd_t));
        if (response_mail != NULL) {
          _m2m2_app_common_cmd_t *resp = (_m2m2_app_common_cmd_t*)&response_mail->data[0];
          resp->command = (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_FILE_SYS_BLOCKS_WRITE_RESP;
          /* Copy start block and end block no */
           num_blocks_write = write_blocks_req->num_blocks_write;
           start_block_num = write_blocks_req->start_block_num;
          if(fs_flash_power_on(true) != FS_STATUS_OK) {
                NRF_LOG_INFO("Flash power on error");
                resp->status = M2M2_FILE_SYS_STATUS_BLOCKS_WRITE_ERROR;
          } else {
             for(int i = 0; i < num_blocks_write; i++) {
              for(int j = 0; j < PAGES_PER_BLOCK; j++) {
                  memset(pArr,0,sizeof(pArr));
                  InitTestBuffer(pArr, sizeof(pArr));
                  /* write blocks */
                  if(fs_hal_write_blocks(pArr,start_block_num,PAGE_SIZE,first_time_write) == FS_STATUS_OK){
                    resp->status |= M2M2_FILE_SYS_STATUS_OK;
                  }else {
                    resp->status |= M2M2_FILE_SYS_STATUS_BLOCKS_WRITE_ERROR;
                }
                first_time_write = 0;
              }
            }
          }
          response_mail->dest = pkt->src;
          response_mail->src = pkt->dest;
          post_office_send(response_mail, &err);
        }
        if(fs_flash_power_on(false) != FS_STATUS_OK){
            NRF_LOG_INFO("Flash power off error");
          }
#endif
        break;
      }
      case M2M2_FILE_SYS_CMD_GET_FS_STATUS_REQ : {
        g_fs_status_reps_tick = get_sensor_time_stamp();
        response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(_m2m2_app_common_cmd_t));
        if (response_mail != NULL) {
          _m2m2_app_common_cmd_t *resp = (_m2m2_app_common_cmd_t*)&response_mail->data[0];
          resp->command = (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_FILE_SYS_CMD_GET_FS_STATUS_RESP;
          if (FileSrc != M2M2_ADDR_UNDEFINED) {
            resp->status = M2M2_FILE_SYS_STATUS_LOGGING_IN_PROGRESS;
          }else {
            resp->status = M2M2_FILE_SYS_STATUS_LOGGING_NOT_STARTED;
          }
          response_mail->dest = pkt->src;
          response_mail->src = pkt->dest;
          post_office_send(response_mail, &err);
        }
        break;
      } case M2M2_FILE_SYS_CMD_GET_FS_STREAM_SUB_STATUS_REQ : {
        M2M2_ADDR_ENUM_t TempAdress;
        int16_t Table_Index;
        m2m2_app_common_sub_op_t *subs_status_req = (m2m2_app_common_sub_op_t*)&pkt->data[0];
        response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_file_sys_get_subs_status_resp_t));
        if (response_mail != NULL) {
          m2m2_file_sys_get_subs_status_resp_t *resp = (m2m2_file_sys_get_subs_status_resp_t*)&response_mail->data[0];
          resp->command = (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_FILE_SYS_CMD_GET_FS_STREAM_SUB_STATUS_RESP;
          TempAdress = get_file_routing_table_entry(subs_status_req->stream);
          if (TempAdress != M2M2_ADDR_UNDEFINED) {
            resp->status = (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_FILE_SYS_STATUS_OK;
            Table_Index = get_file_routing_table_index(subs_status_req->stream);
            resp->stream = subs_status_req->stream;
            if(Table_Index != -1)
              resp->subs_state = file_routing_table[Table_Index].fs_subs_state;
          } else {
            resp->status = (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_FILE_SYS_STATUS_ERROR;
            resp->stream = subs_status_req->stream;
            resp->subs_state = M2M2_FILE_SYS_SUBS_INVALID;
          }
          response_mail->dest = pkt->src;
          response_mail->src = pkt->dest;
          post_office_send(response_mail, &err);
#ifdef DEBUG_LOG_START
           fs_sub_timer_gap = get_micro_sec() - fs_sub_timer_start;
#endif
        }
        break;
      }
      case M2M2_FILE_SYS_CMD_TEST_LOG_REQ: {
      FS_STATUS_ENUM_t fs_error;
#ifdef PATTERN_WRITE_ENABLED
        InitTestBuffer(pArr, sizeof(pArr));
        /* power on flash */
        if(fs_flash_power_on(true) != FS_STATUS_OK) {
           NRF_LOG_INFO("Flash power on error");
         }
         if(fs_hal_write_access_state() == FS_FILE_ACCESS_START) {
          /* Create file and start logging */
            file_hdr_wr_progress = true;
            /* open file name */
            fs_error = fs_hal_open_file(PatternFileName);
             response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_file_sys_cmd_t));
            if (response_mail != NULL) {
              m2m2_file_sys_cmd_t  *ctrl = (m2m2_file_sys_cmd_t *)&response_mail->data[0];
              ctrl->command = M2M2_FILE_SYS_CMD_TEST_LOG_RESP;
              if(fs_error != FS_STATUS_OK) {
                ctrl->status = M2M2_APP_COMMON_STATUS_ERROR;
              }
              else {
                ctrl->status = M2M2_APP_COMMON_STATUS_OK;
              }
                response_mail->dest = pkt->src;
                response_mail->src =  pkt->dest;
                post_office_send(response_mail, &err);
            }

#ifdef NAND_WRITE_TEST
            g_file_write_time = 0;
#endif
            if (fs_error == FS_STATUS_OK) {
            for (gs2kChunkCnt=0; gs2kChunkCnt < ADI_FS_FILE_MAX_SIZE; gs2kChunkCnt++)
            {
#ifdef NAND_WRITE_TEST
              nTick = MCU_HAL_GetTick();
#endif
              fs_error |= fs_hal_test_pattern_write(pArr, sizeof(pArr));
              
#ifdef NAND_WRITE_TEST
            g_write_time = (MCU_HAL_GetTick() - nTick);
            g_file_write_time += g_write_time;
            NRF_LOG_INFO("Write time for a page = %d",g_write_time);
#endif
          }
        }
#ifdef NAND_WRITE_TEST
          NRF_LOG_INFO("Write time for %d pages = %d",ADI_FS_FILE_MAX_SIZE,g_file_write_time);
         NRF_LOG_INFO("***** Average pattern file write time=%d***********",(g_file_write_time/ADI_FS_FILE_MAX_SIZE));
#endif
       }
	if(fs_hal_write_access_state() == FS_FILE_ACCESS_IN_PROGRESS){
          fs_error = fs_hal_close_file(fs_hal_write_file_pointer());
          if(fs_error == FS_STATUS_OK) {
            //flash power off
          if(fs_flash_power_on(false) != FS_STATUS_OK){
              NRF_LOG_INFO("Flash power off error");
          }
            file_hdr_wr_progress = false;
          }
        }
#endif /* PATTERN_WRITE_ENABLED */
        break;
      }
      case M2M2_FILE_SYS_CMD_PATTERN_WRITE_REQ: {
#ifdef PATTERN_WRITE_ENABLED
      FS_STATUS_ENUM_t fs_error;
      uint32_t num_pages_to_write;
      char Filename[16];
      uint8_t nFileCount;
      m2m2_file_sys_pattern_write_req_pkt_t *pattern_write_req = (m2m2_file_sys_pattern_write_req_pkt_t*)&pkt->data[0];
        M2M2_FILE_SYS_STATUS_ENUM_t fs_err_open_status = (M2M2_FILE_SYS_STATUS_ENUM_t)M2M2_FILE_SYS_STATUS_ERROR;
        InitTestBuffer(pArr, sizeof(pArr));
        for(uint32_t nIndex = 0; nIndex < 4;nIndex++){
          NRF_LOG_INFO("%d",pArr[ nIndex]);
        }
        /* power on flash */
        if(fs_flash_power_on(true) != FS_STATUS_OK) {
           NRF_LOG_INFO("Flash power on error");
         }

         if(fs_hal_write_access_state() == FS_FILE_ACCESS_START) {
          /* Create file and start logging */
            file_hdr_wr_progress = true;
            fs_error = fs_hal_get_file_count(&nFileCount);
            /* create new file */
            snprintf(Filename,16,"%s_%d",PatternFileName,(nFileCount+1));
            /* dont create file if file size less than page size */
            if(pattern_write_req->file_size >= PAGE_SIZE) {
              /* open file name */
              fs_error = fs_hal_open_file(Filename);
              num_pages_to_write = (pattern_write_req->file_size/PAGE_SIZE);

              /* file opening error */
              if(fs_error != FS_STATUS_OK)  {
                if(fs_error == FS_STATUS_ERR_MAX_FILE_COUNT)  {
                  fs_err_open_status = M2M2_FILE_SYS_ERR_MAX_FILE_COUNT;
                } 
                else if(fs_error == FS_STATUS_ERR_CONFIG_FILE_POSITION) {
                  fs_err_open_status = M2M2_FILE_SYS_ERR_CONFIG_FILE_POSITION;
                } 
                else if(fs_error == FS_STATUS_ERR_MEMORY_FULL)  {
                  fs_err_open_status =  (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_FILE_SYS_ERR_MEMORY_FULL;
                } 
               else  {
                 fs_err_open_status = M2M2_FILE_SYS_STATUS_ERROR;
                }
              }
              else {
                fs_err_open_status = FS_STATUS_OK;
              }
#ifdef NAND_WRITE_TEST
            g_file_write_time=0;
#endif
              /* write if creation of file is successful */
              if (fs_error == FS_STATUS_OK) {
                /* set file src to valid , for display to progress bar */
                FileSrc = pkt->src;

#ifdef ENABLE_WATCH_DISPLAY
                reset_display_vol_info();
                //Update the NAND memory progress bar in Main menu page of Display
                send_private_type_value(DIS_REFRESH_SIGNAL);
#endif

                for (gs2kChunkCnt = 0; gs2kChunkCnt < num_pages_to_write; gs2kChunkCnt++) {
#ifdef NAND_WRITE_TEST
                  nTick = MCU_HAL_GetTick();
#endif
                  fs_error |= fs_hal_test_pattern_write(pArr, sizeof(pArr));
                  /* process errors */
                  if(fs_error == (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_FILE_SYS_ERR_MEMORY_FULL){
                    break;
                  }
#ifdef NAND_WRITE_TEST
                  g_write_time = (MCU_HAL_GetTick() - nTick);
                  g_file_write_time += g_write_time;
                  NRF_LOG_INFO("Write time for a page = %d",g_write_time);
#endif  
                }
              }
            }
           else  {
              NRF_LOG_INFO("pattern write wont create file less than page size");
              fs_error = FS_STATUS_ERR;
          }
          response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_file_sys_cmd_t));
          if (response_mail != NULL) {
            m2m2_file_sys_cmd_t  *ctrl = (m2m2_file_sys_cmd_t *)&response_mail->data[0];
            ctrl->command = M2M2_FILE_SYS_CMD_PATTERN_WRITE_RESP;
            if(fs_err_open_status != FS_STATUS_OK)  {
              /* failure in creation of file */
              ctrl->status = fs_err_open_status;
            } 
            else  {
              /* process write errors */
              if(fs_error ==  (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_FILE_SYS_ERR_MEMORY_FULL){
                /* send memory full */
                ctrl->status  = (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_FILE_SYS_ERR_MEMORY_FULL;
              }
              else if(fs_error != FS_STATUS_OK){
                ctrl->status = fs_error;
              }
              else {
                ctrl->status = FS_STATUS_OK;
              }
            }
              
            response_mail->dest = pkt->src;
            response_mail->src =  pkt->dest;
            post_office_send(response_mail, &err);
          }
#ifdef NAND_WRITE_TEST
         NRF_LOG_INFO("Write time for %d pages = %d",ADI_FS_FILE_MAX_SIZE,g_file_write_time);
         NRF_LOG_INFO("***** Average pattern file write time=%d***********",(g_file_write_time/ADI_FS_FILE_MAX_SIZE));
#endif
       }
	if(fs_hal_write_access_state() == FS_FILE_ACCESS_IN_PROGRESS){
          fs_error = fs_hal_close_file(fs_hal_write_file_pointer());
          if(fs_error == FS_STATUS_OK) {
            /* flash power off */
            if(fs_flash_power_on(false) != FS_STATUS_OK){
                NRF_LOG_INFO("Flash power off error");
            }
            file_hdr_wr_progress = false;
          }
        }
            /* update file src */
            FileSrc = M2M2_ADDR_UNDEFINED;
#ifdef ENABLE_WATCH_DISPLAY
              reset_display_vol_info();
              //Update the NAND memory progress bar in Main menu page of Display
              send_private_type_value(DIS_REFRESH_SIGNAL);
#endif

#endif /* PATTERN_WRITE_ENABLED */
        break;
      }
      case M2M2_FILE_SYS_CMD_CANCEL_DOWNLOAD_LOG_REQ: {
        response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(_m2m2_app_common_cmd_t));
        if (response_mail != NULL) {
          _m2m2_app_common_cmd_t *file_stream_stop_resp = (_m2m2_app_common_cmd_t *)&response_mail->data[0];
          file_stream_stop_resp->command = (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_FILE_SYS_CMD_STOP_FS_STREAM_RESP;
           fs_stream_usubscribe();
           fs_stream.pkt_src = M2M2_ADDR_UNDEFINED;
           fs_stream.pkt_dest = M2M2_ADDR_UNDEFINED;
           fs_stream_state = ADI_FS_STREAM_STOPPED;
           revert_fs_prio();
           timeout = ADI_OSAL_TIMEOUT_FOREVER;
           file_stream_stop_resp->status = M2M2_FILE_SYS_STATUS_OK;

          /* send response packet */
          response_mail->src = pkt->dest;
          response_mail->dest = pkt->src;
          post_office_send(response_mail, &err);
        }
        break;
      }
      case M2M2_FILE_SYS_CMD_GET_FS_DEBUG_INFO_REQ: {
        m2m2_file_sys_debug_info_req_t *payload = (m2m2_file_sys_debug_info_req_t*)&pkt->data[0];
        response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_file_sys_debug_info_resp_t));
        if(response_mail != NULL) {
          m2m2_file_sys_debug_info_resp_t *resp_payload = (m2m2_file_sys_debug_info_resp_t *)&response_mail->data[0];
          resp_payload->command = M2M2_FILE_SYS_CMD_GET_FS_DEBUG_INFO_RESP;
          response_mail->src = pkt->dest;
          response_mail->dest = pkt->src;
          resp_payload->status = M2M2_FILE_SYS_STATUS_OK;
          resp_payload->stream = payload->stream;
          get_file_misd_packet_debug_info(resp_payload);
          /* send response packet */
          post_office_send(response_mail, &err);
        }
        break;
      }

       case M2M2_FILE_SYS_CMD_GET_FS_FORMAT_INFO_REQ: {
        m2m2_file_sys_format_debug_info_req_t *payload = (m2m2_file_sys_format_debug_info_req_t*)&pkt->data[0];
        response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_file_sys_format_debug_info_resp_t));
        if(response_mail != NULL) {
          m2m2_file_sys_format_debug_info_resp_t *resp_payload = (m2m2_file_sys_format_debug_info_resp_t *)&response_mail->data[0];
          resp_payload->command = M2M2_FILE_SYS_CMD_GET_FS_FORMAT_INFO_RESP;
          response_mail->src = pkt->dest;
          response_mail->dest = pkt->src;
          resp_payload->status = M2M2_FILE_SYS_STATUS_OK;
          get_fs_format_debug_info(resp_payload);
          /* send response packet */
          post_office_send(response_mail, &err);
        }
        break;
       }
      case M2M2_FILE_SYS_CMD_STOP_LOGGING_REQ: {
        FS_STATUS_ENUM_t fs_error;
        uint8_t stream_in_progress = 0;

        if(FileSrc != M2M2_ADDR_UNDEFINED) {
#ifdef HIBERNATE_MD_EN
          hibernate_md_set(HIB_MD_FS_LOG_STOP_EVT);
          hibernate_mode_entry();
#endif
//#ifdef ENABLE_WATCH_DISPLAY
//          reset_display_vol_info();
//          //Update the NAND memory progress bar in Main menu page of Display
//          send_private_type_value(DIS_REFRESH_SIGNAL);
//#endif

          FileSrc = pkt->src;
          /* Backup stop source */
          m2m2_file_sys_stop_log_cmd_t *payload = (m2m2_file_sys_stop_log_cmd_t *)&pkt->data[0];
          if(StopSrc == M2M2_FILE_SYS_STOP_LOGGING_INVALID) {
            StopSrc = payload->stop_type;
          }
          /* Send stream stop request */
          if(fs_hal_write_access_state() == FS_FILE_ACCESS_IN_PROGRESS) {
            if(get_file_system_subs_count() == 0) {
              /* Stop File logging */
              fs_error = fs_hal_close_file(fs_hal_write_file_pointer());
#ifdef ENABLE_WATCH_DISPLAY
              reset_display_vol_info();
              //Update the NAND memory progress bar in Main menu page of Display
              send_private_type_value(DIS_REFRESH_SIGNAL);
#endif
              if(fs_error == FS_STATUS_OK) {
                /* Power off Flash */
                if(fs_flash_power_on(false) != FS_STATUS_OK) {
                NRF_LOG_INFO("Flash power off error");
                }
              }
            } else {
               stream_in_progress = 1;
            }

            /* Create response packet */
            response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_file_sys_stop_log_cmd_t));
            if(response_mail != NULL) {
              m2m2_file_sys_stop_log_cmd_t *resp_payload = (m2m2_file_sys_stop_log_cmd_t *)&response_mail->data[0];
              response_mail->src = M2M2_ADDR_SYS_FS;
              response_mail->dest = FileSrc;

              /* stream in progress, file cannot be closed */
              if(stream_in_progress) {
                resp_payload->status = M2M2_APP_COMMON_STATUS_STREAM_IN_PROGRESS;
              } else {
                 if(fs_error == FS_STATUS_OK) {
                  if(StopSrc == M2M2_FILE_SYS_MEMORY_FULL) {
                    resp_payload->status = M2M2_FILE_SYS_ERR_MEMORY_FULL;
                    resp_payload->stop_type = M2M2_FILE_SYS_MEMORY_FULL;
                  } else if(StopSrc == M2M2_FILE_SYS_BATTERY_LOW) {
                    resp_payload->status = M2M2_FILE_SYS_ERR_BATTERY_LOW;
                    resp_payload->stop_type = M2M2_FILE_SYS_BATTERY_LOW;
                  } else if(StopSrc == M2M2_FILE_SYS_POWER_STATE_SHUTDOWN) {
                    resp_payload->status = M2M2_FILE_SYS_ERR_POWER_STATE_SHUTDOWN;
                    resp_payload->stop_type = M2M2_FILE_SYS_POWER_STATE_SHUTDOWN;
                  }
                  else {
                    resp_payload->stop_type = M2M2_FILE_SYS_STOP_LOGGING;
                    resp_payload->status = M2M2_FILE_SYS_STATUS_LOGGING_STOPPED;
                  }
                  FileSrc = M2M2_ADDR_UNDEFINED;
                  g_userinfo.keyID = M2M2_DEFAULT_KEY_ID;
                } else {
                  resp_payload->status = M2M2_FILE_SYS_STATUS_ERROR;
                }
              }
              resp_payload->command = M2M2_FILE_SYS_CMD_STOP_LOGGING_RESP;
              StopSrc = M2M2_FILE_SYS_STOP_LOGGING_INVALID;
              /* send response packet */
              post_office_send(response_mail, &err);
            }
          }
        }
        else {
          /* Send error response */
          response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_file_sys_stop_log_cmd_t));
          if(response_mail != NULL) {
            m2m2_file_sys_stop_log_cmd_t *resp_payload = (m2m2_file_sys_stop_log_cmd_t *)&response_mail->data[0];
            resp_payload->command = M2M2_FILE_SYS_CMD_STOP_LOGGING_RESP;
            response_mail->src = pkt->dest;
            response_mail->dest = pkt->src;
            resp_payload->status = M2M2_FILE_SYS_STATUS_LOGGING_NOT_STARTED;
            resp_payload->stop_type = M2M2_FILE_SYS_STOP_LOGGING_INVALID;
            /* send response packet */
            post_office_send(response_mail, &err);
          }
        }
         NRF_LOG_INFO("******* Time out count for whole session = %d ************",TimeOutCount);
          break;
      }
      case M2M2_SENSOR_COMMON_CMD_STREAM_DATA: {
        M2M2_ADDR_ENUM_t TempAdress;
        M2M2_APP_COMMON_STATUS_ENUM_t TempStatus;
        int16_t index;
        TempAdress = get_file_routing_table_entry(pkt->src);
        if (TempAdress != M2M2_ADDR_UNDEFINED) {
          index = get_file_misd_packet_table_index(pkt->src);
          _m2m2_app_data_stream_hdr_t *payload = (_m2m2_app_data_stream_hdr_t*)&pkt->data[0];
          if ((FileSrc != M2M2_ADDR_UNDEFINED) && (file_hdr_wr_progress != true)) {
            TempStatus = fs_hal_write_packet_stream(pkt);
            /* Increase total packet count */
            if(index != -1) {
              file_misd_packet_table[index].totalpacketcount++;
            }
            /* Check write packet response and send error respose if required */
            if ( TempStatus != M2M2_APP_COMMON_STATUS_OK) {
              response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(_m2m2_app_common_cmd_t));
              if (response_mail != NULL) {
                if(TempStatus != (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_FILE_SYS_ERR_MEMORY_FULL) {
                  /* Stop logging into file */
                  fs_hal_close_file(fs_hal_write_file_pointer());
                  fs_hal_write_error_resp(response_mail,&FileSrc);
                  post_office_send(response_mail,&err);
                } else {
                  gMemoryFull = true;
                  _m2m2_app_common_cmd_t *stream_resp = (_m2m2_app_common_cmd_t *)&response_mail->data[0];
                  stream_resp->command = (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_FILE_SYS_CMD_FORCE_STOP_LOG_REQ;
                  stream_resp->status = M2M2_APP_COMMON_STATUS_OK;
                  response_mail->src = M2M2_ADDR_SYS_FS;
                  response_mail->dest = M2M2_ADDR_SYS_FS;
                  post_office_send(response_mail, &err);
                }
              }
              if (index != -1) {
                file_misd_packet_table[index].missedpackets++;
              }
            }
            if (index != -1) {
              /* check with prev sequence number */
              if (payload->sequence_num == 0) {
                if(file_misd_packet_table[index].count != 0) {
                  file_misd_packet_table[index].missedpackets += (65535 - file_misd_packet_table[index].count);
                }
              } else {
                if((file_misd_packet_table[index].count + 1) != payload->sequence_num) {
                  /* Get missed packet count and update the debug info structure */
                  file_misd_packet_table[index].missedpackets += (payload->sequence_num - file_misd_packet_table[index].count);
                  file_misd_packet_table[index].totalpacketcount += (payload->sequence_num - file_misd_packet_table[index].count);
                }
              }
              file_misd_packet_table[index].count = payload->sequence_num;
            }
          }
        }
        break;
      }
      case M2M2_FILE_SYS_CMD_CHUNK_RETRANSMIT_REQ: {
        uint32_t nSeqNumber = 0;
        uint32_t nRollOver = 0;
        uint32_t Page_offset = 0;
        uint32_t nfilesize = 0;
        int8_t BlockNumber = 0;
        response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_file_sys_get_resp_t));
        if(response_mail != NULL) {
          response_mail->src  =  pkt->dest;
          response_mail->dest = pkt->src;
          m2m2_file_sys_get_resp_t *resp = (m2m2_file_sys_get_resp_t *)&response_mail->data[0];
          resp->command = (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_FILE_SYS_CMD_CHUNK_RETRANSMIT_RESP;// to change
          resp->status = (M2M2_FILE_SYS_STATUS_ENUM_t) M2M2_FILE_SYS_STATUS_ERROR;
          if (fs_stream_state != ADI_FS_STREAM_STARTED) {
            if(fs_flash_power_on(true) != FS_STATUS_OK) {
              resp->status = (M2M2_FILE_SYS_STATUS_ENUM_t) M2M2_FILE_SYS_STATUS_ERROR;
            } else {
              m2m2_file_sys_pkt_retransmit_req_t *retransmit_req = (m2m2_file_sys_pkt_retransmit_req_t*)&pkt->data[0];
              snprintf(fs_stream.file_path, ((pkt->length - M2M2_HEADER_SZ - 1)) , \
                "%s", retransmit_req->file_name);
              memset(&retransmit_req->file_name, 0,
                     pkt->length - (M2M2_HEADER_SZ + \
                       sizeof(m2m2_file_sys_pkt_retransmit_req_t) - 1));
              /* calculating sequence number based on roll-over */
              nRollOver = retransmit_req->Roll_over * 65536;
              nSeqNumber = retransmit_req->chunk_number + nRollOver;
              fs_stream.fs_buffer_size = sizeof(fs_stream.file_buff);
              /* calculating page-offset based on sequence number */
              BlockNumber = (nSeqNumber % (fs_stream.fs_buffer_size/MEMBER_SIZE(m2m2_file_sys_get_resp_t, byte_stream)));
              Page_offset = (nSeqNumber / (fs_stream.fs_buffer_size/MEMBER_SIZE(m2m2_file_sys_get_resp_t, byte_stream))) * fs_stream.fs_buffer_size;
              fs_stream.processed_data_len = BlockNumber * MEMBER_SIZE(m2m2_file_sys_get_resp_t, byte_stream);
              fs_stream.fs_err_status = fs_hal_read_pageoffset(fs_stream.file_path, &fs_stream.file_buff[0], &fs_stream.fs_buffer_size, &nfilesize, Page_offset);
              if((fs_stream.fs_err_status == FS_STATUS_OK) || (fs_stream.fs_err_status == FS_STATUS_ERR_EOF)) {
                fs_stream.data_chunk_len =  fs_stream.fs_buffer_size;
                if (fs_stream.data_chunk_len > sizeof(resp->byte_stream)) {
                  fs_stream.data_chunk_len = sizeof(resp->byte_stream);
                }
                memset(&resp->byte_stream[0], 0, MEMBER_SIZE(m2m2_file_sys_get_resp_t, byte_stream));
                memcpy(&resp->byte_stream[0], &fs_stream.file_buff[fs_stream.processed_data_len],fs_stream.data_chunk_len);
                if (fs_stream.fs_err_status == FS_STATUS_ERR_EOF) {
                  resp->status = (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_FILE_SYS_END_OF_FILE;
                  if(fs_stream.fs_buffer_size != sizeof(fs_stream.file_buff)) {
                    fs_stream.num_bytes_left = nfilesize % sizeof(fs_stream.file_buff);
                    if((BlockNumber * MEMBER_SIZE(m2m2_file_sys_get_resp_t, byte_stream)) > fs_stream.num_bytes_left) {
                      resp->status = (M2M2_FILE_SYS_STATUS_ENUM_t) M2M2_FILE_SYS_STATUS_ERROR;
                    } else {
                      fs_stream.data_chunk_len = (fs_stream.num_bytes_left - (BlockNumber * MEMBER_SIZE(m2m2_file_sys_get_resp_t, byte_stream)));
                        if (fs_stream.data_chunk_len > sizeof(resp->byte_stream)) {
                          fs_stream.data_chunk_len = sizeof(resp->byte_stream);
                          resp->status = M2M2_FILE_SYS_STATUS_OK;
                        } else {
                          resp->status = (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_FILE_SYS_END_OF_FILE;
                        }
                    }
                  }
                } else if (fs_stream.fs_err_status == FS_STATUS_OK) {
                  resp->status = M2M2_FILE_SYS_STATUS_OK;
                }
                resp->len_stream =  fs_stream.data_chunk_len;
                response_mail->checksum =  nSeqNumber;
                resp->crc16 = crc16_compute((uint8_t *)response_mail,fs_stream.data_chunk_len + 12,NULL);
              } else {
                resp->status = (M2M2_FILE_SYS_STATUS_ENUM_t) M2M2_FILE_SYS_STATUS_ERROR;
              }
            }
          }
          post_office_send(response_mail, &err);
          if(fs_flash_power_on(false) != FS_STATUS_OK) {
	     NRF_LOG_INFO("Error flash power off");
          }
        }
        break;
      }
      case M2M2_FILE_SYS_CMD_SET_KEY_VALUE_PAIR_REQ: {
        m_time_struct *local_get_date_time;
        time_t return_time;
        int16_t timezone_offset;

        m2m2_file_sys_set_key_value_pair_req_t *payload = (m2m2_file_sys_set_key_value_pair_req_t*)&pkt->data[0];
        response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(_m2m2_app_common_cmd_t));
        if(response_mail != NULL) {
          _m2m2_app_common_cmd_t *resp_payload = (_m2m2_app_common_cmd_t *)&response_mail->data[0];
          resp_payload->command = (M2M2_FILE_SYS_CMD_ENUM_t) M2M2_FILE_SYS_CMD_SET_KEY_VALUE_PAIR_RESP;
          get_log_time_stamp(&return_time, &timezone_offset);
          local_get_date_time = m_sec_to_date_time(return_time + timezone_offset);//For file getDateTime timezone_offset IS required
          memcpy(&g_userinfo.valueID[0], (void *)&payload->valueID[0],sizeof(g_userinfo.valueID));
          g_userinfo.keyID = M2M2_KEY_ID;
          g_userinfo.year = local_get_date_time->tm_year;
          g_userinfo.month = local_get_date_time->tm_mon;
          g_userinfo.day = local_get_date_time->tm_mday;
          g_userinfo.hour = local_get_date_time->tm_hour;
          g_userinfo.minute = local_get_date_time->tm_min;
          g_userinfo.second = local_get_date_time->tm_sec;
          g_userinfo.TZ_sec = timezone_offset;
          resp_payload->status = (M2M2_FILE_SYS_STATUS_ENUM_t) M2M2_FILE_SYS_STATUS_OK;
          response_mail->src = pkt->dest;
          response_mail->dest = pkt->src;
          post_office_send(response_mail, &err);
         }
          break;
        }
      }
    } else {
      M2M2_APP_COMMON_STATUS_ENUM_t TempStatus;
      int16_t index;
      FS_STATUS_ENUM_t fs_error;
        index = get_file_misd_packet_table_index(pkt->src);
        _m2m2_app_data_stream_hdr_t *payload = (_m2m2_app_data_stream_hdr_t*)&pkt->data[0];
#ifdef NAND_WRITE_TEST
          uint32_t nTick = 0;
          nTick = MCU_HAL_GetTick();
#endif
         if ((FileSrc != M2M2_ADDR_UNDEFINED) && (file_hdr_wr_progress != true)) {
          TempStatus = fs_hal_write_packet_stream(pkt);
          /* Increase total packet count */
          if(index != -1) {
            file_misd_packet_table[index].totalpacketcount++;
          }
#ifdef NAND_WRITE_TEST
          g_write_time = MCU_HAL_GetTick() - nTick;
          g_file_write_time += g_write_time;
         /* NRF_LOG_INFO("Packet write time = %d",g_write_time); */
#endif
          /* Check write packet response and send error respose if required */
          if (TempStatus != M2M2_APP_COMMON_STATUS_OK) {
            if(TempStatus != (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_FILE_SYS_ERR_MEMORY_FULL) {
              /* Stop logging into file */
              fs_hal_close_file(fs_hal_write_file_pointer());
              /* Send Error response if write to Flash fails */
              fs_send_write_error(&FileSrc);
            } else {
                /* unsubscribe streams */
                fs_unsubscribe_streams();

                /* Power off Flash */
                if(fs_flash_power_on(false) != FS_STATUS_OK) {
                  NRF_LOG_INFO("Flash power off error");
                }
                
                /* send message to source */
                response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_file_sys_stop_log_cmd_t));
                if(response_mail != NULL) {
                    m2m2_file_sys_stop_log_cmd_t *resp_payload = (m2m2_file_sys_stop_log_cmd_t *)&response_mail->data[0];
                    response_mail->src = M2M2_ADDR_SYS_FS;
                    response_mail->dest = FileSrc;
                    resp_payload->status = M2M2_FILE_SYS_ERR_MEMORY_FULL;
                    resp_payload->command = M2M2_FILE_SYS_CMD_STOP_LOGGING_RESP;
                    StopSrc = M2M2_FILE_SYS_STOP_LOGGING_INVALID;
                    /* send response packet */
                    post_office_send(response_mail, &err);
                  }
                  FileSrc = M2M2_ADDR_UNDEFINED;
                }
            
            if (index != -1) {
              file_misd_packet_table[index].missedpackets++;
            }
          }
          /* Update FS missed packet and total packet info */
          if (index != -1) {
            /* check with prev sequence number */
            if (payload->sequence_num == 0) {
              if(file_misd_packet_table[index].count != 0) {
                file_misd_packet_table[index].missedpackets += (65535 - file_misd_packet_table[index].count);
              }
            } else {
              if((file_misd_packet_table[index].count + 1) != payload->sequence_num) {
                /* Get missed packet count and update the debug info structure */
                file_misd_packet_table[index].missedpackets += (payload->sequence_num - file_misd_packet_table[index].count);
                file_misd_packet_table[index].totalpacketcount += (payload->sequence_num - file_misd_packet_table[index].count);
              }
            }
            file_misd_packet_table[index].count = payload->sequence_num;
          }
        }
    }
      gFileSystemMsgProcessCnt++;
      post_office_consume_msg(pkt);
    }
    if(fs_stream_state == ADI_FS_STREAM_STARTED) {
      hike_fs_prio();
      if((fs_stream.fs_err_status != FS_STATUS_ERR_EOF) || (fs_stream_final_buffer == 1)) {
        do {
          fs_stream.data_chunk_len = fs_stream.fs_buffer_size - fs_stream.processed_data_len;
          /* Give the system some time to clear out pending messages */
          response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_file_sys_download_log_stream_t));
          while (response_mail == NULL) {
            MCU_HAL_Delay(50);
            response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_file_sys_download_log_stream_t));
          }
          /* to compute CRC with stream as source and destination as application */
          response_mail->src  =  M2M2_ADDR_SYS_FS_STREAM;
          response_mail->dest =  fs_stream.pkt_src;
          m2m2_file_sys_download_log_stream_t *resp = (m2m2_file_sys_download_log_stream_t *)&response_mail->data[0];
          resp->command = (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_FILE_SYS_CMD_DOWNLOAD_LOG_RESP;
          if (fs_stream.fs_buffer_size <= sizeof(fs_stream.file_buff)) {
            resp->status = M2M2_FILE_SYS_STATUS_OK;
          }
          if (fs_stream.data_chunk_len > sizeof(resp->byte_stream)) {
            fs_stream.data_chunk_len = sizeof(resp->byte_stream);
          }
          memcpy(&resp->byte_stream[0], &fs_stream.file_buff[fs_stream.processed_data_len], fs_stream.data_chunk_len);
          fs_stream.num_bytes_left -= fs_stream.data_chunk_len;
          fs_stream.processed_data_len += fs_stream.data_chunk_len;
          num_bytes_processed += fs_stream.processed_data_len;
          resp->len_stream = fs_stream.data_chunk_len;
          if (fs_stream.fs_err_status == FS_STATUS_ERR_EOF && fs_stream.num_bytes_left == 0) {
            resp->status = (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_FILE_SYS_END_OF_FILE;
            response_mail->checksum =  nSeqNum;
            /* crc calculation */
            resp->crc16 = crc16_compute((uint8_t *)response_mail,fs_stream.data_chunk_len + 12,NULL);
            response_mail->src = M2M2_ADDR_SYS_FS;
            response_mail->dest = M2M2_ADDR_SYS_FS_STREAM;// FS STREAM
            setFsDownloadFlag(1);
            fs_download_chunk_cnt++;
            fs_download_total_pkt_cnt++;
            post_office_send(response_mail, &err);
            /* Unsubscribe from mailbox */
            fs_stream_usubscribe();
            fs_stream.pkt_src = M2M2_ADDR_UNDEFINED;
            fs_stream.pkt_dest = M2M2_ADDR_UNDEFINED;
            fs_stream_state = ADI_FS_STREAM_STOPPED;
            timeout = ADI_OSAL_TIMEOUT_FOREVER;
            revert_fs_prio();
#ifdef PROFILE_TIME_ENABLED
          /* avg cal*/
          usb_avg_tx_time = usb_avg_tx_time / num_bytes_transferred;
          usb_avg_cdc_write_time = usb_avg_cdc_write_time / num_bytes_transferred;
          /* page size */
          avg_file_read_time = avg_file_read_time / num_times_read;
          /* 4 milli sec delay = 4000 micro sec */
          delay_time = max_num_of_retries*4000;
          total_time_taken_512_bytes_transfer = avg_file_read_time+delay_time + usb_avg_tx_time+usb_avg_cdc_write_time;
#endif
            break;
          }
          if ( fs_stream.fs_err_status == FS_STATUS_OK) {
            resp->len_stream = fs_stream.data_chunk_len;
            resp->status = M2M2_FILE_SYS_STATUS_OK;
          } else if (fs_stream.fs_err_status == FS_STATUS_ERR_STDIO) {
            resp->len_stream = 0;
            resp->status = (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_FILE_SYS_ERR_INVALID;
          }
          response_mail->checksum =  nSeqNum;
          resp->crc16 = crc16_compute((uint8_t *)response_mail,fs_stream.data_chunk_len + 12,NULL);
          response_mail->src = M2M2_ADDR_SYS_FS;
          response_mail->dest = M2M2_ADDR_SYS_FS_STREAM;
          setFsDownloadFlag(1);
          fs_download_chunk_cnt++;
          fs_download_total_pkt_cnt++;
          post_office_send(response_mail, &err);
          nSeqNum++;
          n500bChunkCnt++;
        } while (fs_stream.processed_data_len < fs_stream.fs_buffer_size);

        /* Power Off Nand if final buffer is transmitted */
        if(fs_stream_final_buffer == 1) {
          /* Power off Flash */
          if(fs_flash_power_on(false) != FS_STATUS_OK){
            NRF_LOG_INFO("flash power off error");
          }
        }
        fs_stream_final_buffer = 0;
        fs_stream.processed_data_len = 0;

        if(fs_stream.fs_err_status != FS_STATUS_ERR_EOF) {
#ifdef NAND_READ_TEST
          uint32_t nTick = 0;
          static uint8_t index = 0;

          nTick = MCU_HAL_GetTick();
#endif /* NAND_READ_TEST */
          fs_stream.fs_err_status = fs_hal_read_file(fs_stream.file_path, &fs_stream.file_buff[0], &fs_stream.fs_buffer_size);
          fs_status_stored = fs_stream.fs_err_status;
#ifdef NAND_READ_TEST
          g_read_time = MCU_HAL_GetTick() - nTick;
          g_file_read_time += g_read_time;
          g_file_read_cnt++;
          if(index == 14)
            index =0;
          else
            index++;
#endif /* NAND_READ_TEST */

          /* Check Error status */
          if(fs_stream.fs_err_status == FS_STATUS_ERR_EOF) {
            fs_stream_final_buffer = 1;
          } else if(fs_stream.fs_err_status != FS_STATUS_OK) {
             response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_file_sys_download_log_stream_t));
             if(response_mail != NULL) {
               response_mail->src = M2M2_ADDR_SYS_FS;
               response_mail->dest = M2M2_ADDR_SYS_FS_STREAM;
               m2m2_file_sys_download_log_stream_t *resp_payload = (m2m2_file_sys_download_log_stream_t *)&response_mail->data[0];
               resp_payload->len_stream = 0;
               resp_payload->status = M2M2_FILE_SYS_STATUS_ERROR;
               /* Send Error response to App */
               post_office_send(response_mail, &err);
               /* Unsubscribe from mailbox */
               fs_stream_usubscribe();
               /* Stop streaming file */
               fs_stream.pkt_src = M2M2_ADDR_UNDEFINED;
               fs_stream.pkt_dest = M2M2_ADDR_UNDEFINED;
               fs_stream_state = ADI_FS_STREAM_STOPPED;
               timeout = ADI_OSAL_TIMEOUT_FOREVER;
               revert_fs_prio();
             }
            }
        }
        /* wait for packet transfer only if DOWNLOAD_LOG requested by external tool */
        if(gsFsDownloadReqByExtTool)
        {
          while(get_file_download_chunk_count() < fs_download_chunk_cnt)
          {
            MCU_HAL_Delay(4);
          }
          while(get_usbd_tx_pending_status())
          {
            MCU_HAL_Delay(1);
          }
        }
        setFsDownloadFlag(0);
        fs_download_chunk_cnt = 0;
        if(fs_stream.fs_err_status == FS_STATUS_ERR_EOF) {
          fs_stream_final_buffer = 1;
        }
        fs_stream.num_bytes_left = fs_stream.fs_buffer_size;
      }
    }
    /* reset the variable everytime after completing the download request */
    gsFsDownloadReqByExtTool = 0;
  }
}

/*!
  ****************************************************************************
* @brief  Send Write error resp
* @param  src: the stream to send error to
* @return None
*****************************************************************************/
void fs_send_write_error(M2M2_ADDR_ENUM_t *src) {
  m2m2_hdr_t                    *response_mail = NULL;
  ADI_OSAL_STATUS               err;
  response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_file_sys_cmd_t));
  if(response_mail != NULL) {
    m2m2_file_sys_cmd_t *resp_payload = (m2m2_file_sys_cmd_t *)&response_mail->data[0];
    resp_payload->command = M2M2_FILE_SYS_CMD_START_LOGGING_RESP;
    response_mail->src = M2M2_ADDR_SYS_FS;
    response_mail->dest = *src;
    resp_payload->status = M2M2_FILE_SYS_STATUS_LOGGING_ERROR;
    *src = M2M2_ADDR_UNDEFINED;
    /* send response packet */
    post_office_send(response_mail, &err);
  }
}

/*!
  ****************************************************************************
* @brief  Unsubscribe fs stream
* @param  None
* @return None
*****************************************************************************/
void fs_stream_usubscribe() {
  ADI_OSAL_STATUS err;
  /* Unsubscribe from mailbox */
  if(FileStreamSubsciberCount!=0){
    FileStreamSubsciberCount--;
    m2m2_hdr_t *sub_pkt = post_office_create_msg(M2M2_HEADER_SZ + sizeof(post_office_config_t));
    if(sub_pkt != NULL) {
      post_office_config_t *payload = (post_office_config_t *)&sub_pkt->data;
      payload->cmd = POST_OFFICE_CFG_CMD_MAILBOX_UNSUBSCRIBE;
      payload->sub = fs_stream.pkt_src;
      payload->box = M2M2_ADDR_SYS_FS_STREAM;
      sub_pkt->src = fs_stream.pkt_dest;
      sub_pkt->dest = M2M2_ADDR_POST_OFFICE;
      post_office_send(sub_pkt, &err);
    }
  }
}
#endif /* USE_FS */
/*@}*/  /* end of group file_system_task */
/**@}*/ /* end of group Tasks */
/*/ /* end of group Tasks */
