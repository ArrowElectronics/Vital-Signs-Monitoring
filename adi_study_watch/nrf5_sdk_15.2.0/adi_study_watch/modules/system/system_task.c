/**
***************************************************************************
* @addtogroup Tasks
* @{
* @file         system_task.c
* @author       ADI
* @version      V1.0.0
* @date         16-June-2016
* @brief        Source file contains system process for wearable device
*framework.
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

/*---------------------------- Includes --------------------------------------*/
#include "file_system_utils.h"
#include "light_fs.h"
#include <dcb_interface.h>
#include <file_system_interface.h>
#include <file_system_task.h>
#include <includes.h>
#include <system_interface.h>
#include <system_task.h>
#include <system_version.h>
#include <task_includes.h>

#include "Adxl362.h"
#include "ad5940.h"
#include "ad7156.h"
#include "ad7156_dcfg.h"
#include "adp5360.h"
#include "adpd4000_buffering.h"
#include "adpd4000_dcfg.h"
#include "bcm_application_task.h"
#include "adxl_dcfg.h"
#include "app_timer.h"
#include "ble_gap.h"
#include "ble_task.h"
#include "dcb_general_block.h"
#include "lt_app_lcfg_block.h"
#ifdef USER0_CONFIG_APP
#include "dcb_user0_block.h"
#endif
#include "fds_drv.h"
#include "file_system_utils.h"
#include "mw_ppg.h"
#include "power_manager.h"
#include "rtc.h"
#include "sensor_ad5940.h"
#include "timers.h"
#include "touch_test.h"
#include "us_tick.h"
#include <adpd4000_task.h>
#include "display_app.h"
#include "manufacture_date.h"
/* System Task Module Log settings */
#define NRF_LOG_MODULE_NAME System_Task

#if SYSTEM_TASK_CONFIG_LOG_ENABLED
#ifdef NRF_LOG_DEFAULT_LEVEL
#undef NRF_LOG_DEFAULT_LEVEL
#define NRF_LOG_DEFAULT_LEVEL SYSTEM_TASK_CONFIG_LOG_LEVEL
#endif
#define NRF_LOG_INFO_COLOR SYSTEM_TASK_CONFIG_INFO_COLOR
#define NRF_LOG_DEBUG_COLOR SYSTEM_TASK_CONFIG_DEBUG_COLOR
#else // SYSTEM_TASK_CONFIG_LOG_ENABLED
#define NRF_LOG_LEVEL 0
#endif // SYSTEM_TASK_CONFIG_LOG_ENABLED

#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

/*------------------------------ Private Function Prototype ---------------- */
static void system_task(void *pArgument);
static void StreamBatt_Info(void);

/*------------------------------ Public Function Prototype ------------------ */
extern void trigger_nRF_MCU_hw_reset();
extern uint32_t ad5940_port_Init(void);
extern uint32_t ad5940_port_deInit(void);
extern bool ecg_get_dcb_present_flag(void);
extern bool eda_get_dcb_present_flag(void);
extern void delete_low_touch_config_file();
extern void load_default_low_touch_config();
#ifdef USER0_CONFIG_APP
extern void start_eda_app_timer();
extern void start_adxl_app_timer();
extern void start_temp_app_timer();
extern void start_adpd_app_timer();
#endif
/* For system FreeRTOS Task Creation */
/* Create the stack for task */
uint8_t ga_system_task_stack[APP_OS_CFG_PM_TASK_STK_SIZE];
/* Create handler for task */
ADI_OSAL_THREAD_HANDLE gh_system_task_handler;
/* Create task attributes variable */
ADI_OSAL_STATIC_THREAD_ATTR g_system_task_attributes;
/* Create TCB for task */
StaticTask_t g_system_task_tcb;
/* Create Queue Handler for task */
ADI_OSAL_QUEUE_HANDLE gh_system_task_msg_queue = NULL;
// ADI_OSAL_SEM_HANDLE   g_sys_task_evt_sem;

/* Structure to hold system info */
m2m2_pm_sys_info_t g_system_info = {
    (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_PM_SYS_COMMAND_GET_INFO_RESP,
    (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_PM_SYS_STATUS_OK,
    0x0000,                               // version number
    {0x00, 0xE0, 0x22, 0xF0, 0xF1, 0xF2}, // MAC address
    0x00000000,                           // device id
    0x00000000,                           // model number
    0x0000,                               // hw_id
    0x0000,                               // bom_id
    0x00,                                 // batch_id
    0x00000000};                          // date

/*Battery-full charge level  */
#define BATTERY_FULL_CHARGE_LEVEL 99U

/*default value for battery-low level alert  */
#define DEFAULT_BATTERY_LEVEL_LOW 5U

/*default value for battery-critical level alert  */
#define DEFAULT_BATTERY_LEVEL_CRITICAL 1U

/*Structure to hold battery notifications related information*/
struct battery_level_alerts_ {
  uint8_t level_low;           /*Battery low level threshold*/
  uint8_t level_critical;      /*Battery critical level threshold*/
  uint8_t critical_level_flag; /*Battery critical level flag*/
  M2M2_ADDR_ENUM_t
      tool_addr; /*Destination address for battery notifications alerts*/
  CHARGE_STATUS_ENUM_t charge_alert_status;
} battery_level_alerts;

static uint32_t BattInfoChecksum = 0;
// static uint8_t  BatteryLevelCritical = 5;
// static uint8_t  BatteryLevelLow = 15;
// static uint8_t  gsLogBatteryCriticalFlag = 0;

/*Indicate the availability of config file in the NAND flash*/
volatile uint8_t gsCfgFileFoundFlag = 0;
#ifdef LOW_TOUCH_FEATURE
#include <low_touch_task.h>
/*Indicate the availability of gen blk DCB config for low touch*/
static volatile uint8_t gbDCBCfgFoundFlag = 0;
#endif
static uint16_t BattInfoSubsciberCount = 0;

/*Ping Test Initial Values*/
static uint16_t ping_count = 10;
static uint8_t ping_pkt_sz = 70;
static M2M2_ADDR_ENUM_t ping_pkt_src;
static void ping_timer_init(void);
static void ping_timer_start(void);
static void ping_timer_stop(void);

#ifdef DCB
static m2m2_hdr_t *lt_app_lcfg_dcb_command_read_config(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *lt_app_lcfg_dcb_command_write_config(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *lt_app_lcfg_dcb_command_delete_config(m2m2_hdr_t *p_pkt);
#endif

/* To distinguish between stream_stop_resp/unsub_resp pkt received in s/m task
 * against the low touch specific stream_stop_resp/unsub_resp pkts */
volatile uint8_t gb_sensor_stop_cmds = 0;
/* Flag to Stop sensor streaming only if logging is not going on, otherwise only
 * Unsub */
volatile uint8_t gb_stop_streaming_if_no_logging = 0;
/* Flag to handle only Unsub command */
volatile uint8_t gb_unsub_only = 0;

/* To get info on all the sensor apps in Watch
   from M2M2_PM_SYS_COMMAND_GET_APPS_INFO_REQ
*/
static uint8_t gsSensorStatusPktBuf[sizeof(m2m2_pm_sys_sensor_apps_info_req_t)];
static m2m2_pm_sys_sensor_apps_info_req_t *gsSensorStatusRespPkt = NULL;
static uint8_t gsSensorAppsInfoPkt[M2M2_HEADER_SZ +
                                   sizeof(m2m2_pm_sys_sensor_apps_info_req_t)];
static m2m2_hdr_t *gsSensorAppsInfoHdr = NULL;
static m2m2_pm_sys_sensor_apps_info_req_t *gsSensorAppsInfoPayload = NULL;
/* To go through all sensor apps in the routing table */
static uint8_t gsPsAppNumber = 0;
/* No: of Active sensor apps */
static uint8_t gsActivePsAppCnt = 0;
/* Flag to specify that all slots within ADPD are being gone through
   from M2M2_ADDR_SENSOR_ADPD4000 sensor app, while GetSensorAppStatus() */
static uint8_t gnAdpdSensorApp = 0;
/* 20 -> max num of sensor apps in Watch; gets updated later from the actual
 * routing table */
static uint8_t gsRoutingTableSize = 20;
/* to hold the source address of the command */
static M2M2_ADDR_ENUM_t gsStausCmdSrcAddr = NULL;

extern g_state_t g_state;

void GetSensorAppsStatus();
void SendSensorStopCmds();
#ifdef ENABLE_WATCH_DISPLAY
extern void reset_display_vol_info();
#endif

/* M2M2 address(ble/usb) from which Force stream stop/unsub REQ needs to be
 * given out; to be used from M2M2_PM_SYS_COMMAND_FORCE_STREAM_STOP_REQ */
static M2M2_ADDR_ENUM_t gnForceStopReqAddr = M2M2_ADDR_UNDEFINED;
/* Flag to handle whether ADXL sensor was 'start'ed to get raw
 * data(CLI_USB/CLI_BLE) or if it was started by internal applications like PPG
 */
extern uint8_t gb_adxl_raw_start_ppg;
/* Flag to handle whether ADPD sensor was 'start'ed to get raw
 * data(CLI_USB/CLI_BLE) or if it was started by internal applications like
 * PPG */
extern uint8_t gb_adpd_raw_start_ppg;
/* Flag to handle whether ADPD sensor was 'start'ed to get raw
 * data(CLI_USB/CLI_BLE) or if it was started by internal applications like
 * Temp */
extern uint8_t gb_adpd_raw_start_temp;
/* Flag to handle whether ADPD sensor was 'start'ed and multiple subscribers
* (more than 1) added to get raw data(CLI_USB/CLI_BLE) */
extern uint8_t gb_adpd_multi_sub_start;

/*To send the LT error responses */
extern M2M2_ADDR_ENUM_t usb_pkt_src;
extern M2M2_ADDR_ENUM_t ble_pkt_src;

#define ADI_PM_BAT_LEVEL_LOG_CRITICAL 1

/* LT config file name that resided in NAND Flash COnfig block */
uint8_t ConfigFileName[16] = "USER_CONFIG.LOG";

/* State machine variables for ON/OFF wrist events */
extern volatile LOW_TOUCH_DETECTION_STATUS_ENUM_t eCurDetection_State;
/*!
  ****************************************************************************
*    @brief Sends Battery Level Alert Message to the Tool Registered

*    @param[in]                    - M2M2_PM_SYS_STATUS_ENUM_t
*    nAlertMsg                     - battery level status enum could be
low/critical
*    @return                       - None

*****************************************************************************/
static void SendBatteryLevelAlertMsg(M2M2_PM_SYS_STATUS_ENUM_t nAlertMsg) {
  m2m2_hdr_t *response_mail = NULL;
  ADI_OSAL_STATUS err;

#ifndef CUST4_SM
  if (battery_level_alerts.tool_addr != M2M2_ADDR_UNDEFINED) {
#endif
    response_mail =
        post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_pm_sys_cmd_t));
    if (response_mail != NULL) {
      m2m2_pm_sys_cmd_t *battery_level_alert_msg =
          (m2m2_pm_sys_cmd_t *)&response_mail->data[0];

      /* send response packet */
      response_mail->src = M2M2_ADDR_SYS_PM;
#ifdef CUST4_SM
      response_mail->dest = M2M2_ADDR_GLOBAL;
#else
      response_mail->dest = (M2M2_ADDR_ENUM_t)battery_level_alerts.tool_addr;
#endif
      response_mail->length = 10;
      response_mail->checksum = 0x0000;
      battery_level_alert_msg->command =
          M2M2_PM_SYS_BATTERY_LEVEL_ALERT; /* Battery Alert Asynchronous
                                              message*/
      battery_level_alert_msg->status = nAlertMsg;
      post_office_send(response_mail, &err);
    }
#ifndef CUST4_SM
  }
#endif
}
#ifdef LOW_TOUCH_FEATURE
#define MAX_CMD_RETRY_CNT 5
#define MAX_FILE_COUNT 62
#define MAX_USABLE_MEMORY_SIZE 536868864
/* // M2M2_HEADER_SZ + sizeof(m2m2_file_sys_user_cfg_summary_pkt_t) */
#define CONFIG_FILE_SUMMARY_PKT_SIZE    20
#define STATUS_VALID 1
#define STATUS_INVALID 0
//#define LOW_TOUCH_FORCE_STOP_LOG            1//To force stop the regular/ Low
// touch logging on Button A event

/* Variable which holds the status LT app/logging running or not */
uint8_t gLowTouchRunning = 0;
/* Flag to enable FS task to send RESP to sysmte task */
uint8_t gSendRespToPmApp = 0;
/* Indicate the availability of config file copy in the RAM */
uint8_t gsCfgCopyAvailable = 0;
/* Flag to start sending LT command sequence */
static uint8_t gsSendNextConfigCmd = 0;
/* Variable to hold start or stop commands end point */
static uint32_t gsConfigFileSize = 0;
/* Variable to track no: of cmds currently executed in either Start/Stop LT
 * sequence */
static uint32_t gsNumOfCommands = 0;
/* Variable to update the read file pointer, with pkt len for a command that was
 * read & executed from Start/Stop LT sequence */
static uint16_t gsPktLenCopy = 0;
/* Variable pointer to hold start(head) of either Start/Stop LT sequence of
 * commands */
static uint8_t *gsConfigFileHeadPtr = NULL;
/* Variable pointer to hold end(tail) of either Start/Stop LT sequence of
 * commands */
static uint8_t *gsConfigFileTailPtr;
/* Variable pointer to read a command from either Start/Stop LT sequence of
 * commands */
static uint8_t *gsConfigFileReadPtr;
/* Variable pointer to extract control command from either Start/Stop LT
 * sequence of commands */
static _m2m2_app_common_cmd_t *gsConfigCtrl =
    NULL; //(_m2m2_app_common_cmd_t*)&pkt->data[0];
/* Variable pointer to extract m2m2 header info from either Start/Stop LT
 * sequence of commands */
static m2m2_hdr_t *gsConfigCmdHdr = NULL;
/* Debug variables used in ValidateStatus() api*/
static uint8_t gsStartStatusCnt = 0, gsSubStatusCnt = 0, gsfslogStatusCnt = 0,
               gsCommonStatusCnt = 0, gsCmdRetryCnt = 0;
/* RAM buffer which hold a copy of either DCB/NAND config file which has LT
 * start/stop sequence of commands
 * This buffer is to be reused between holding the copy of LT config file
 * as well as temporary storage for DCB content during READ/WRITE DCB block
 * for ADPD, Gen block DCB
 * NAND config file size = 1 page size = 4096 bytes.
 * Gen block DCB size, with MAX_GEN_BLK_DCB_PKTS=18 pkts,
 * each having 57 * 4 bytes = 4104 bytes.
 * ADPD4000 DCB size, with MAX_ADPD4000_DCB_PKTS=4 pkts,
 * each having 57 * 4 bytes = 912 bytes.
 * In the below array size definition, maximum of the above is chosen.
 */
uint32_t gsConfigTmpFile[MAXGENBLKDCBSIZE * MAX_GEN_BLK_DCB_PKTS];
/* Variable to hold the index where DCB/NAND config file is to be written to */
static uint16_t gsCfgWrIndex = 0;
/* Variable pointer which holds the summary info on the DCB/NAND config file
 * which has LT start/stop sequence of commands */
static m2m2_file_sys_user_cfg_summary_pkt_t *gsCfgFileSummaryPkt = NULL;
/* Variable which indicates whether LT Start or Stop sequence of cmds is being
 * executed currently */
static uint8_t gsStartCmdsRunning = 0, gsStopCmdsRunning = 0;
struct _low_touch_info {
  M2M2_ADDR_ENUM_t tool_addr; /*Tool Address*/
  uint8_t cmd;                /*Command sent from tool*/
} low_touch_info;

/*!
 ****************************************************************************
 * @brief   Initialize the low touch configuration paramteres
 * @param    nStart:  1 - to start LT Start sequence, 0 - to start the LT stop
 * sequence
 * @retval   None
 ******************************************************************************/
void InitConfigParam(bool_t nStart) {

  uint8_t *gsConfigTmpFile_ptr = (uint8_t *)&gsConfigTmpFile[0];

  gsCfgFileSummaryPkt =
      (m2m2_file_sys_user_cfg_summary_pkt_t *)&gsConfigTmpFile_ptr[M2M2_HEADER_SZ];

  if (nStart) {
    gsStartCmdsRunning = 1;
    gsStopCmdsRunning = 0;
    gsConfigFileHeadPtr =
        &gsConfigTmpFile_ptr[CONFIG_FILE_SUMMARY_PKT_SIZE]; // Load the start cmds
                                                        // starting point
    gsConfigFileSize =
        gsCfgFileSummaryPkt->start_cmd_len; // start commands end point
  } else {
    gsStopCmdsRunning = 1;
    gsStartCmdsRunning = 0;
    // Load the stop cmds starting point
    gsConfigFileHeadPtr = &gsConfigTmpFile_ptr[CONFIG_FILE_SUMMARY_PKT_SIZE +
                                           gsCfgFileSummaryPkt->start_cmd_len];
    gsConfigFileSize =
        gsCfgFileSummaryPkt->stop_cmd_len; // stop commands end point
  }
  gsNumOfCommands = 0;
  gsStartStatusCnt = 0;
  gsSubStatusCnt = 0;
  gsfslogStatusCnt = 0;
  gsCommonStatusCnt = 0;

  gsConfigFileReadPtr = gsConfigFileHeadPtr;
  gsConfigFileTailPtr = gsConfigFileHeadPtr + (gsConfigFileSize - 1);
  gsSendNextConfigCmd = 1; // start sending config commands
}

/*!
 ****************************************************************************
 * @brief   Initiates the commmand to stop the low-touch logs and also to stop
 * the sensors
 * @param    None
 * @retval   None
 ******************************************************************************/
void StopLowTouchLogging() {
  m2m2_hdr_t *response_mail = NULL;
  ADI_OSAL_STATUS err;

  response_mail =
      post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_pm_sys_cmd_t));
  if (response_mail != NULL) {
    m2m2_file_sys_cmd_t *stop_low_touch_log_req =
        (m2m2_file_sys_cmd_t *)&response_mail->data[0];

    /* send response packet */
    response_mail->src = low_touch_info.tool_addr;
    response_mail->dest = M2M2_ADDR_SYS_PM;
    response_mail->length = 10;
    response_mail->checksum = 0x0000;
    stop_low_touch_log_req->command =
        M2M2_PM_SYS_COMMAND_DISABLE_USER_CONFIG_LOG_REQ;
    stop_low_touch_log_req->status = M2M2_APP_COMMON_STATUS_OK;
    //              file_sys_get_config_req->file_name = M2M2_PM_SYS_MCU_M3;
    post_office_send(response_mail, &err);
  }
}

/*!
 ****************************************************************************
 * @brief   Send Force stop logging request to the fs-task
 * @param    None
 * @retval   None
 ******************************************************************************/
void SendForceStopLogging() {
#ifdef USE_FS
  m2m2_hdr_t *pkt = NULL;
  ADI_OSAL_STATUS err;
  /* If File logging is in progress */
  //        if (fs_hal_write_access_state() == FS_FILE_ACCESS_IN_PROGRESS) {
  /* Force Stop Logging */
  pkt = post_office_create_msg(M2M2_HEADER_SZ + sizeof(_m2m2_app_common_cmd_t));
  if (pkt != NULL) {
    _m2m2_app_common_cmd_t *payload = (_m2m2_app_common_cmd_t *)&pkt->data[0];
    payload->command =
        (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_FILE_SYS_CMD_FORCE_STOP_LOG_REQ;
    pkt->src = M2M2_ADDR_SYS_PM;
    pkt->dest = M2M2_ADDR_SYS_FS;
    post_office_send(pkt, &err);
  }
//        }
#endif // USE_FS
}

/*!
 ****************************************************************************
 * @brief   Set the flag with NAND cfg file availablity status
 * @param    None
 * @retval   None
 ******************************************************************************/
void SetCfgFileAvailableFlag(uint8_t nflag) { gsCfgFileFoundFlag = nflag; }

/*!
 ****************************************************************************
 * @brief   Get the flag with NAND cfg file availablity status
 * @param    None
 * @retval   None
 ******************************************************************************/
uint8_t GetCfgFileAvailableFlag(void) { return(gsCfgFileFoundFlag); }

/*!
 ****************************************************************************
 * @brief   Set the flag with status of RAM buffer availability for DCB/NAND
 * cfg file
 * @param    None
 * @retval   None
 ******************************************************************************/
void SetCfgCopyAvailableFlag(uint8_t nflag) { gsCfgCopyAvailable = nflag; }

/*!
 ****************************************************************************
 * @brief   Return the flag with status of RAM buffer availability for DCB/NAND
 * cfg file
 * @param    None
 * @retval   None
 ******************************************************************************/
static uint8_t IsCfgCopyAvailable() { return (gsCfgCopyAvailable); }

/*!
 ****************************************************************************
 * @brief   Return the flag with status of gen_blk DCB availability with LT
 * configs
 * @param    None
 * @retval   None
 ******************************************************************************/
void find_low_touch_DCB() {
  gbDCBCfgFoundFlag = gen_blk_get_dcb_present_flag();
}

/*!
 ****************************************************************************
 * @brief   Function to send m2m2 cmd to get FS vol info
 * @param    None
 * @retval   None
 ******************************************************************************/
void GetVolInfo() {
  m2m2_hdr_t *pkt = NULL;
  ADI_OSAL_STATUS err;

  pkt = post_office_create_msg(M2M2_HEADER_SZ + sizeof(_m2m2_app_common_cmd_t));
  if (pkt != NULL) {
    _m2m2_app_common_cmd_t *payload = (_m2m2_app_common_cmd_t *)&pkt->data[0];
    payload->command =
        (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_FILE_SYS_CMD_VOL_INFO_REQ;
    pkt->src = M2M2_ADDR_SYS_PM;
    pkt->dest = M2M2_ADDR_SYS_FS;
    post_office_send(pkt, &err);
  }
}

/*!
 ****************************************************************************
 * @brief   Function to send m2m2 cmd to get FS file count info
 * @param    None
 * @retval   None
 ******************************************************************************/
void GetFileCount() {
  m2m2_hdr_t *pkt = NULL;
  ADI_OSAL_STATUS err;

  pkt = post_office_create_msg(M2M2_HEADER_SZ + sizeof(_m2m2_app_common_cmd_t));
  if (pkt != NULL) {
    _m2m2_app_common_cmd_t *payload = (_m2m2_app_common_cmd_t *)&pkt->data[0];
    payload->command =
        (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_FILE_SYS_CMD_GET_NUMBER_OF_FILE_REQ;
    pkt->src = M2M2_ADDR_SYS_PM;
    pkt->dest = M2M2_ADDR_SYS_FS;
    post_office_send(pkt, &err);
  }
}

/*!
 ****************************************************************************
 * @brief   Sends the battery critical info to the tool if registerd during low
 * touch logging
 * @param    None
 * @retval   None
 ******************************************************************************/
static void SendBatteryCriticalErrResp() {
  m2m2_hdr_t *response_mail = NULL;
  ADI_OSAL_STATUS err;

  if (low_touch_info.tool_addr != M2M2_ADDR_UNDEFINED) {
    response_mail =
        post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_pm_sys_cmd_t));
    if (response_mail != NULL) {
      m2m2_pm_sys_cmd_t *low_touch_resp =
          (m2m2_pm_sys_cmd_t *)&response_mail->data[0];

      /* send response packet */
      response_mail->src = M2M2_ADDR_SYS_PM;
      response_mail->dest = (M2M2_ADDR_ENUM_t)low_touch_info.tool_addr;
      response_mail->length = 10;
      response_mail->checksum = 0x0000;
      low_touch_resp->command =
          M2M2_FILE_SYS_CMD_FORCE_STOP_LOG_RESP; // keeping it same as that of
                                                 // normal logging
      low_touch_resp->status = M2M2_FILE_SYS_ERR_LOG_FORCE_STOPPED;
      //              file_sys_get_config_req->file_name = M2M2_PM_SYS_MCU_M3;
      post_office_send(response_mail, &err);
    }
  }
}

/*!
 ****************************************************************************
 * @brief   Sends the response of the low-touch commands to the tool if
 * registerd
 * @param    None
 * @retval   None
 ******************************************************************************/
static void SendLowTouchErrResp(M2M2_PM_SYS_STATUS_ENUM_t nStatus) {
  m2m2_hdr_t *response_mail = NULL;
  ADI_OSAL_STATUS err;

  if (low_touch_info.tool_addr != M2M2_ADDR_UNDEFINED) {
    response_mail =
        post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_pm_sys_cmd_t));
    if (response_mail != NULL) {
      m2m2_pm_sys_cmd_t *low_touch_resp =
          (m2m2_pm_sys_cmd_t *)&response_mail->data[0];

      /* send response packet */
      response_mail->src = M2M2_ADDR_SYS_PM;
      response_mail->dest = (M2M2_ADDR_ENUM_t)low_touch_info.tool_addr;
      response_mail->length = 10;
      response_mail->checksum = 0x0000;
      low_touch_resp->command =
          (low_touch_info.cmd +
              1); // response for the command sent from the tool
      low_touch_resp->status = nStatus;
      //              file_sys_get_config_req->file_name = M2M2_PM_SYS_MCU_M3;
      post_office_send(response_mail, &err);
    }
  }
}

/*!
 ****************************************************************************
 * @brief   Send m2m2 cmd to find config file REQ from NAND Flash
 * @param    None
 * @retval   None
 ******************************************************************************/
static void SendFindConfigFileReq() {
  m2m2_hdr_t *response_mail = NULL;
  ADI_OSAL_STATUS err;

  response_mail =
      post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_file_sys_cmd_t));
  if (response_mail != NULL) {
    m2m2_file_sys_cmd_t *find_cfg_file_req =
        (m2m2_file_sys_cmd_t *)&response_mail->data[0];

    /* send response packet */
    response_mail->src = M2M2_ADDR_SYS_PM;
    response_mail->dest = M2M2_ADDR_SYS_FS;
    response_mail->length = 10;
    response_mail->checksum = 0x0000;
    find_cfg_file_req->command = M2M2_FILE_SYS_CMD_FIND_CONFIG_FILE_REQ;
    find_cfg_file_req->status = M2M2_APP_COMMON_STATUS_OK;
    //              file_sys_get_config_req->file_name = M2M2_PM_SYS_MCU_M3;
    post_office_send(response_mail, &err);
  }
}

/*!
 ****************************************************************************
 * @brief   Send m2m2 cmd to start read/download of config file from NAND Flash
 * @param    None
 * @retval   None
 ******************************************************************************/
static uint8_t ReadConfigFile() {
  m2m2_hdr_t *response_mail = NULL;
  ADI_OSAL_STATUS err;

  response_mail =
      post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_file_sys_get_req_t));
  if (response_mail != NULL) {
    m2m2_file_sys_get_req_t *file_sys_get_config_req =
        (m2m2_file_sys_get_req_t *)&response_mail->data[0];

    /* send response packet */
    response_mail->src = M2M2_ADDR_SYS_PM;
    response_mail->dest = M2M2_ADDR_SYS_FS;
    response_mail->length = 26;
    response_mail->checksum = 0x0000;
    file_sys_get_config_req->command = M2M2_FILE_SYS_CMD_DOWNLOAD_LOG_REQ;
    memcpy(&file_sys_get_config_req->file_name, ConfigFileName, 16);
    //              file_sys_get_config_req->file_name = M2M2_PM_SYS_MCU_M3;
    post_office_send(response_mail, &err);
  }
  return 0;
}

/*!
 ****************************************************************************
 * @brief   Function to handle MAX file error seen with LT logging
 * @param    None
 * @retval   None
 ******************************************************************************/
static void MaxFileErr() {
#ifdef ENABLE_WATCH_DISPLAY
  // Insert display code handling
  send_global_type_value(DIS_MAX_FILE_ALARM);//pop up max file error alam.
  resume_key_and_lt_task();
#endif
}

/*!
 ****************************************************************************
 * @brief   Function to indicate the error on any events of Low touch feature
 * @param    None
 * @retval   None
 ******************************************************************************/
void LowTouchErr(void) {
  //    gLowTouchRunning = 0;
#ifdef ENABLE_WATCH_DISPLAY
  // Insert display code handling
  send_global_type_value(DIS_LOW_TOUCH_ALARM);//pop up low touch alarm.
  resume_key_and_lt_task();
#endif
}

/* Debug variable to hold status of stream stop REQ */
uint8_t gs_in_prgrs_cnt = 0;
/*!
 ****************************************************************************
 * @brief   Function to validate the RESP command and status obtained after
 * running a m2m2 cmd from either the Start/Stop LT sequence
 * @param    src - The application address used to differentiate FS application
 * @param    nCommand-last command RESP received
 * @param    nStatus-status of last REQ cmd run
 * @retval   STATUS_VALID or STATUS_INVALID based on nStatus
 ******************************************************************************/
static uint8_t ValidateStatus(M2M2_ADDR_ENUM_t  src, uint8_t nCommand, uint8_t nStatus) {
  uint8_t nStatusValid = STATUS_INVALID;

  switch (nCommand) {
  case M2M2_FILE_SYS_CMD_LOG_STREAM_REQ:
    if(M2M2_ADDR_SYS_FS == src)
    {
      gsSubStatusCnt++;
      nStatusValid = ((nStatus == M2M2_APP_COMMON_STATUS_SUBSCRIBER_ADDED))
                         ? STATUS_VALID
                         : STATUS_INVALID;
    }
    else
    {
      gsCommonStatusCnt++;
      nStatusValid = (nStatus == M2M2_APP_COMMON_STATUS_OK)? STATUS_VALID :
      STATUS_INVALID;
    }
    break;
  case M2M2_FILE_SYS_CMD_STOP_STREAM_REQ:
    if(M2M2_ADDR_SYS_FS == src)
    {
      gsSubStatusCnt++;
      nStatusValid =
          ((nStatus == M2M2_APP_COMMON_STATUS_SUBSCRIBER_REMOVED) ||
              (nStatus == M2M2_APP_COMMON_STATUS_SUBSCRIBER_COUNT_DECREMENT))
              ? STATUS_VALID
              : STATUS_INVALID;
    }
    else
    {
      gsCommonStatusCnt++;
      nStatusValid = (nStatus == M2M2_APP_COMMON_STATUS_OK)? STATUS_VALID :
      STATUS_INVALID;
    }
    break;
  case M2M2_APP_COMMON_CMD_STREAM_START_REQ:
    gsStartStatusCnt++;
    nStatusValid = ((nStatus == M2M2_APP_COMMON_STATUS_STREAM_STARTED) ||
                       (nStatus == M2M2_APP_COMMON_STATUS_OK) ||
                       (nStatus == M2M2_APP_COMMON_STATUS_STREAM_IN_PROGRESS))
                       ? STATUS_VALID
                       : STATUS_INVALID;
    break;
  case M2M2_APP_COMMON_CMD_STREAM_STOP_REQ:
    gsStartStatusCnt++;
    if (nStatus == M2M2_APP_COMMON_STATUS_STREAM_IN_PROGRESS) {
      nStatusValid = 1;
      gs_in_prgrs_cnt++;
    } else
      nStatusValid =
          ((nStatus == M2M2_APP_COMMON_STATUS_STREAM_STOPPED) ||
              (nStatus == M2M2_APP_COMMON_STATUS_STREAM_COUNT_DECREMENT) ||
              (nStatus == M2M2_APP_COMMON_STATUS_OK) ||
              (nStatus == M2M2_APP_COMMON_STATUS_STREAM_NOT_STARTED))
              ? STATUS_VALID
              : STATUS_INVALID;
    break;
  case M2M2_FILE_SYS_CMD_APPEND_FILE_REQ:
  case M2M2_FILE_SYS_CMD_START_LOGGING_REQ:
    if(M2M2_ADDR_SYS_FS == src)
    {
      gsfslogStatusCnt++;
      nStatusValid = (nStatus == M2M2_FILE_SYS_STATUS_OK)
                         ? STATUS_VALID
                         : STATUS_INVALID; // M2M2_APP_COMMON_STATUS_OK
    }
    else
    {
      gsCommonStatusCnt++;
      nStatusValid = (nStatus == M2M2_APP_COMMON_STATUS_OK)? STATUS_VALID :
      STATUS_INVALID;
    }
    break;
  case M2M2_FILE_SYS_CMD_STOP_LOGGING_REQ:
    if(M2M2_ADDR_SYS_FS == src)
    {
      gsfslogStatusCnt++;
      nStatusValid = (nStatus == M2M2_FILE_SYS_STATUS_LOGGING_STOPPED)
                         ? STATUS_VALID
                         : STATUS_INVALID; // M2M2_APP_COMMON_STATUS_OK
    }
    else
    {
      gsCommonStatusCnt++;
      nStatusValid = (nStatus == M2M2_APP_COMMON_STATUS_OK)? STATUS_VALID :
      STATUS_INVALID;
    }
    break;
  default:
    gsCommonStatusCnt++;
    nStatusValid = STATUS_VALID;
    break;
  }

  NRF_LOG_INFO("Command = %x, Status valid flag=%d", nCommand, nStatusValid);
  return nStatusValid;
}

/*!
 ****************************************************************************
 * @brief   Function which forms a m2m2 cmd from Start/Stop LT sequence and
 * sends it out
 * @param    None
 * @retval   None
 ******************************************************************************/
static void SendUserConfigCommands(void) {
  m2m2_hdr_t *pkt = NULL;
  ADI_OSAL_STATUS err;

  //    MCU_HAL_Delay(10);
  while (gsConfigFileReadPtr < gsConfigFileTailPtr) {
#if LOW_TOUCH_DEBUG
    if (gsNumOfCommands > 23) {
      gsSendNextConfigCmd = 0;
      break;
    }
#endif // LOW_TOUCH_DEBUG
    gsConfigCmdHdr = (m2m2_hdr_t *)gsConfigFileReadPtr;
    gsConfigCmdHdr->src = M2M2_ADDR_SYS_PM;
    // if(!IsCfgCopyAvailable())
    //{

    // gsConfigCmdHdr->src =
    // (M2M2_ADDR_ENUM_t)BYTE_SWAP_16(gsConfigCmdHdr->src);
    gsConfigCmdHdr->dest = (M2M2_ADDR_ENUM_t)BYTE_SWAP_16(gsConfigCmdHdr->dest);
    gsConfigCmdHdr->length = BYTE_SWAP_16(gsConfigCmdHdr->length);
    gsConfigCmdHdr->checksum = BYTE_SWAP_16(gsConfigCmdHdr->checksum);
    //}
    gsPktLenCopy = gsConfigCmdHdr->length;
    gsConfigCtrl = (_m2m2_app_common_cmd_t *)&gsConfigCmdHdr->data[0];
    pkt = post_office_create_msg(gsConfigCmdHdr->length);
    if (pkt != NULL) {
      memcpy(pkt, gsConfigCmdHdr, gsConfigCmdHdr->length);
      NRF_LOG_HEXDUMP_INFO(gsConfigCmdHdr, gsConfigCmdHdr->length);
      gsSendNextConfigCmd = 0;
      post_office_send(pkt, &err);
      break;
    }
  }
}
#endif // LOW_TOUCH_FEATURE

/* counters for tracking the number of stop command sent, Unsub cmd sent
 * and senor applications respecively*/
static uint8_t gsSensorStopCnt = 0, gsSensorSubCnt = 0, gsSensorAppsCnt = 0;

/*!
 ****************************************************************************
 * @brief   Sends the sensor unsubscribe, stop commands to each of the sensor
 *          applications based on their sensor start count
 * @param    None
 * @retval   None
 ******************************************************************************/
void SendSensorStopCmds() {
  m2m2_hdr_t *pkt = NULL;
  ADI_OSAL_STATUS err;
  uint8_t nAdvanceToNextApp = 0, nSendStopCmd = 0;

  while (
      gsSensorAppsCnt <
      gsSensorStatusRespPkt
          ->num_sensor_apps) /*Send sensor stop cmds to all sensor applications
                                on PS till the start count becomes zero */
  {
    nAdvanceToNextApp = 0;
    /* send the [num_start_reqs] number of stop commands OR
     * [num_subscribers] number of unsub commands to the sensor
     * application{app_info[gsSensorAppsCnt].sensor_app_id}*/
    while (gsSensorStopCnt <
           gsSensorStatusRespPkt->app_info[gsSensorAppsCnt].num_start_reqs ||
           gsSensorSubCnt <
           gsSensorStatusRespPkt->app_info[gsSensorAppsCnt].num_subscribers
           ) {
      NRF_LOG_INFO("gsSensorAppsCnt:%d gsSensorSubCnt:%d, gsSensorStopCnt:%d",gsSensorAppsCnt,gsSensorSubCnt, gsSensorStopCnt);
      pkt = post_office_create_msg(
          M2M2_HEADER_SZ + sizeof(_m2m2_app_common_cmd_t));
      if (pkt != NULL) {
        m2m2_app_common_sub_op_t *payload =
            (m2m2_app_common_sub_op_t *)&pkt->data[0];

        if (!gb_stop_streaming_if_no_logging) {
          NRF_LOG_INFO("No logging check required");
          nSendStopCmd = 1;
          gb_sensor_stop_cmds = 1;
          payload->command = M2M2_APP_COMMON_CMD_STREAM_STOP_REQ;
          payload->status = M2M2_APP_COMMON_STATUS_OK;
          pkt->src = M2M2_ADDR_SYS_PM;
          pkt->dest =
              gsSensorStatusRespPkt->app_info[gsSensorAppsCnt].sensor_app_id;
          post_office_send(pkt, &err);
        }
        // If logging is in progress, send Unsubscribe sensor stream instead of
        // Sensor Stop
        else {
          NRF_LOG_INFO("Logging check required");
          if (gsSensorStatusRespPkt->app_info[gsSensorAppsCnt].fs_sub_stat ==
              M2M2_FILE_SYS_SUBSCRIBED) {
            NRF_LOG_INFO("FS subscribed");
            if (gsSensorStatusRespPkt->app_info[gsSensorAppsCnt]
                    .num_subscribers > 1) {
              nSendStopCmd = 1;
              gsSensorSubCnt++;  //Handle sub cnt for FS
              gsSensorStopCnt++; //Handle sensor start count for FS
              NRF_LOG_DEBUG("Force Sensor Unsub Given, since Logging is in "
                            "progress for %x",
                  gsSensorStatusRespPkt->app_info[gsSensorAppsCnt]
                      .sensor_app_id);
              gb_sensor_stop_cmds = 1;
              payload->command = M2M2_APP_COMMON_CMD_STREAM_UNSUBSCRIBE_REQ;
              payload->status = M2M2_APP_COMMON_STATUS_OK;
              payload->stream = gsSensorStatusRespPkt->app_info[gsSensorAppsCnt]
                        .sensor_stream;
              pkt->src = M2M2_ADDR_SYS_PM;
              pkt->dest = gsSensorStatusRespPkt->app_info[gsSensorAppsCnt]
                              .sensor_app_id;
              gb_unsub_only = 1;
              post_office_setup_subscriber(pkt->dest,
                  gsSensorStatusRespPkt->app_info[gsSensorAppsCnt]
                      .sensor_stream,
                  gnForceStopReqAddr, false);
              if (pkt->dest == M2M2_ADDR_MED_PPG) {
                post_office_setup_subscriber(pkt->dest,
                    M2M2_ADDR_MED_SYNC_ADPD_ADXL_STREAM, gnForceStopReqAddr,
                    false);
              }
              post_office_send(pkt, &err);
            } else {
              //m2m2_hdr_t *pkt unused, so free it
              NRF_LOG_INFO("No sub count");
              post_office_consume_msg(pkt);
            }
          }
          // No logging in progress
          else {
            NRF_LOG_INFO("No FS sub count");
            uint8_t pkt_used = 0; //Flag to track usage of m2m2_hdr_t *pkt, to free it properly

            if ( gsSensorStatusRespPkt->app_info[gsSensorAppsCnt]
                        .num_subscribers >= 1) {

              nSendStopCmd = 1;
              gb_unsub_only = 1;
              pkt_used = 1;

              NRF_LOG_DEBUG("sub cnt >= 1, Force Unsub given for %x",
                  gsSensorStatusRespPkt->app_info[gsSensorAppsCnt]
                      .sensor_app_id);
              gb_sensor_stop_cmds = 1;
              payload->command = M2M2_APP_COMMON_CMD_STREAM_UNSUBSCRIBE_REQ;
              payload->status = M2M2_APP_COMMON_STATUS_OK;
              payload->stream = gsSensorStatusRespPkt->app_info[gsSensorAppsCnt]
                        .sensor_stream;
              pkt->src = M2M2_ADDR_SYS_PM;
              pkt->dest = gsSensorStatusRespPkt->app_info[gsSensorAppsCnt]
                              .sensor_app_id;
              post_office_setup_subscriber(pkt->dest,
                  gsSensorStatusRespPkt->app_info[gsSensorAppsCnt]
                      .sensor_stream,
                  gnForceStopReqAddr, false);
              // Handle Unsub of sync adpd adxl stream separately
              if (pkt->dest == M2M2_ADDR_MED_PPG) {
                post_office_setup_subscriber(pkt->dest,
                    M2M2_ADDR_MED_SYNC_ADPD_ADXL_STREAM, gnForceStopReqAddr,
                    false);
              }
              post_office_send(pkt, &err);
            } if (gsSensorStatusRespPkt->app_info[gsSensorAppsCnt]
                        .num_start_reqs >= 1) {
                m2m2_hdr_t *pkt1 = NULL;

                /* Skip Sensor Stop if gb_adxl_raw_start_ppg/gb_adpd_raw_start_ppg/
                   gb_adpd_raw_start_temp is not set OR
                   gb_adpd_multi_sub_start is set*/
                if (((M2M2_ADDR_SENSOR_ADXL ==
                       gsSensorStatusRespPkt->app_info[gsSensorAppsCnt]
                           .sensor_app_id) &&
                      !gb_adxl_raw_start_ppg) ||
                  ((M2M2_ADDR_SENSOR_ADPD4000 ==
                       gsSensorStatusRespPkt->app_info[gsSensorAppsCnt]
                           .sensor_app_id) &&
                      !gb_adpd_raw_start_ppg) ||
                  ((M2M2_ADDR_SENSOR_ADPD4000 ==
                       gsSensorStatusRespPkt->app_info[gsSensorAppsCnt]
                           .sensor_app_id) &&
                      !gb_adpd_raw_start_temp) ||
                  ((M2M2_ADDR_SENSOR_ADPD4000 ==
                       gsSensorStatusRespPkt->app_info[gsSensorAppsCnt]
                           .sensor_app_id) &&
                      gb_adpd_multi_sub_start)) {
                  NRF_LOG_INFO(
                    "Force Stream stop skipped(not raw sensor start) for %x",
                    gsSensorStatusRespPkt->app_info[gsSensorAppsCnt]
                        .sensor_app_id);
                  gsSensorStopCnt++; //Handle sensor start count for raw sensor start case
                  if(!nSendStopCmd) {
                    NRF_LOG_DEBUG("Consuming pkt");
                    post_office_consume_msg(pkt);
                  }
                  break;
                }

                nSendStopCmd = 1;
                gb_unsub_only = 0;

                NRF_LOG_DEBUG("start cnt >= 1; Force Sensor Stop Given for %x",
                    gsSensorStatusRespPkt->app_info[gsSensorAppsCnt]
                        .sensor_app_id);
                pkt1 = post_office_create_msg(
                    M2M2_HEADER_SZ + sizeof(_m2m2_app_common_cmd_t));
                m2m2_app_common_sub_op_t *payload =
                    (m2m2_app_common_sub_op_t *)&pkt1->data[0];

                gb_sensor_stop_cmds = 1;
                payload->command = M2M2_APP_COMMON_CMD_STREAM_STOP_REQ;
                payload->status = M2M2_APP_COMMON_STATUS_OK;
                pkt1->src = M2M2_ADDR_SYS_PM;
                pkt1->dest = gsSensorStatusRespPkt->app_info[gsSensorAppsCnt]
                                .sensor_app_id;
                post_office_send(pkt1, &err);

            }  if (!nSendStopCmd || pkt_used == 0) {
              //m2m2_hdr_t *pkt unused, so free it
              NRF_LOG_INFO("stream start < 1 or sub count < 1 or unused pkt");
              post_office_consume_msg(pkt);
            }
          } /* else of if (gsSensorStatusRespPkt->app_info[gsSensorAppsCnt].fs_sub_stat ==
              M2M2_FILE_SYS_SUBSCRIBED) */
        }//else of if (!gb_stop_streaming_if_no_logging)
        break;
      } // end of if(pkt != NULL)
    }   /* end of while (gsSensorStopCnt <
           gsSensorStatusRespPkt->app_info[gsSensorAppsCnt].num_start_reqs ||
           gsSensorSubCnt <
           gsSensorStatusRespPkt->app_info[gsSensorAppsCnt].num_subscribers */

    if (nSendStopCmd) {
      NRF_LOG_INFO("Breaking");
      nSendStopCmd = 0;
      break;
    } else {
      NRF_LOG_INFO("Advancing to next sensor");
      nAdvanceToNextApp = 1;
      gsSensorAppsCnt++; /* num_start_reqs is equal to number of stop cmds
                            sent[gsSensorStopCnt] OR
                            num_subscribers is equal to number of unsub cmds
                            sent[gsSensorSubCnt], move onto next sensor app*/
      gsSensorStopCnt =
          0; /*clear the sensor stop cmd cnt for the next sensor application*/
     gsSensorSubCnt =
          0; /*clear the sensor sub cmd cnt for the next sensor application*/
    }
  } // end of while (gsSensorAppsCnt < gsSensorStatusRespPkt->num_sensor_apps)

  if (nAdvanceToNextApp) {
    gsSensorAppsCnt = 0;
    gsSensorStopCnt = 0;
    gsSensorSubCnt = 0;
#ifdef LOW_TOUCH_FEATURE
      if (!gb_stop_streaming_if_no_logging) {
        /* All sensor applications are stopped;
          send out the response */
        SendLowTouchErrResp(M2M2_PM_SYS_STATUS_USER_CONFIG_LOG_DISABLED);
      }
#endif
  }
  gb_stop_streaming_if_no_logging = 0;
}

/*!
 ****************************************************************************
 * @brief   Send Force stop streaming request to the All tasks
 * @param    None
 * @retval   None
 ******************************************************************************/
void SendForceStopStreaming() {
  // Stop sensor streaming only if logging is not going on, otherwise only Unsub
  gb_stop_streaming_if_no_logging = 1;
  GetSensorAppsStatus();
}

/*!
 ****************************************************************************
 * @brief   Sends the cmd for getting all the sensor application status
 * @param    None
 * @retval   None
 ******************************************************************************/
void GetSensorAppsStatus() {
  m2m2_hdr_t *pkt = NULL;
  ADI_OSAL_STATUS err;

  pkt = post_office_create_msg(M2M2_HEADER_SZ + sizeof(_m2m2_app_common_cmd_t));
  if (pkt != NULL) {
    _m2m2_app_common_cmd_t *payload = (_m2m2_app_common_cmd_t *)&pkt->data[0];
    payload->command = M2M2_PM_SYS_COMMAND_GET_APPS_INFO_REQ;
    payload->status = M2M2_APP_COMMON_STATUS_OK;
    pkt->src = M2M2_ADDR_SYS_PM;
    pkt->dest = M2M2_ADDR_SYS_PM;
    post_office_send(pkt, &err);
  }
}

/*!
 ****************************************************************************
 *@brief  Update the g_system_info structure with ble mac address
 *
 * @param None
 * @return None
 ******************************************************************************/
void update_ble_system_info() {
  ble_gap_addr_t ble_addr_t;
  int i;
  sd_ble_gap_addr_get(&ble_addr_t);

  for (i = 0; i < BLE_GAP_ADDR_LEN; i++)
    g_system_info.mac_addr[i] = ble_addr_t.addr[BLE_GAP_ADDR_LEN-1-i];
}

#ifdef USER0_CONFIG_APP
/*!
 ****************************************************************************
 * @brief  Update the g_system_info structure with hardware id
 *
 * @param hw_id: hardware id to be updated the g_system_info structure contents
                 with
 * @return None
 ******************************************************************************/
void update_hw_id_in_system_info(uint16_t hw_id) {
  g_system_info.hw_id = hw_id;
}
#endif

/*!
 ****************************************************************************
 * @brief System task init function.
 ******************************************************************************/
void system_task_init(void) {
  ADI_OSAL_STATUS eOsStatus;

  /* Create system application thread */
  g_system_task_attributes.pThreadFunc = system_task;
  g_system_task_attributes.nPriority = APP_OS_CFG_PM_TASK_PRIO;
  g_system_task_attributes.pStackBase = &ga_system_task_stack[0];
  g_system_task_attributes.nStackSize = APP_OS_CFG_PM_TASK_STK_SIZE;
  g_system_task_attributes.pTaskAttrParam = NULL;
  g_system_task_attributes.szThreadName = "system_task";
  g_system_task_attributes.pThreadTcb = &g_system_task_tcb;
  eOsStatus = adi_osal_MsgQueueCreate(&gh_system_task_msg_queue, NULL, 9);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
    Debug_Handler();
  } else {
    update_task_queue_list(APP_OS_CFG_SYS_TASK_INDEX, gh_system_task_msg_queue);
  }

  eOsStatus = adi_osal_ThreadCreateStatic(
      &gh_system_task_handler, &g_system_task_attributes);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
    Debug_Handler();
  }
  // adi_osal_SemCreate(&g_sys_task_evt_sem, 0);
  gsSensorAppsInfoHdr = (m2m2_hdr_t *)&gsSensorAppsInfoPkt[0];
  gsSensorAppsInfoPayload = (m2m2_pm_sys_sensor_apps_info_req_t *)&(
      gsSensorAppsInfoPkt[M2M2_HEADER_SZ]);
  gsSensorStatusRespPkt =
      (m2m2_pm_sys_sensor_apps_info_req_t *)gsSensorStatusPktBuf;

  battery_level_alerts.tool_addr = M2M2_ADDR_UNDEFINED;
  battery_level_alerts.level_low = DEFAULT_BATTERY_LEVEL_LOW;
  battery_level_alerts.level_critical = DEFAULT_BATTERY_LEVEL_CRITICAL;
  battery_level_alerts.charge_alert_status = E_NORMAL_LEVEL;
  battery_level_alerts.critical_level_flag = 0;
  ping_timer_init();
}

uint32_t gPmMsgPostCnt = 0;
/*!
 ****************************************************************************
 * @brief  Function to send PO pkts to system task
 * @param  p_pkt m2m2 packet to be send to system task
 * @return None
 ******************************************************************************/
void send_message_system_task(m2m2_hdr_t *p_pkt) {
  ADI_OSAL_STATUS osal_result;
  osal_result = adi_osal_MsgQueuePost(gh_system_task_msg_queue, p_pkt);
  if (osal_result != ADI_OSAL_SUCCESS)
    post_office_consume_msg(p_pkt);
  else
    gPmMsgPostCnt++;
  // adi_osal_SemPost(g_sys_task_evt_sem);
}

#ifdef USE_FS
#ifdef FS_TEST_BLOCK_READ_DEBUG
uint8_t fs_task_read_tmp_blk_flag=0;
uint32_t page_num=0;
uint8_t type_arg;
uint32_t pdata_mem[10];
#endif
#endif

/*!
 ****************************************************************************
 * @brief  system_task - Get commands and response for system task APIs
 * @param  pArgument not used
 * @return None
 ******************************************************************************/
static void system_task(void *pArgument) {
  m2m2_hdr_t *pkt = NULL;
  _m2m2_app_common_cmd_t *ctrl_cmd = NULL;
  ADI_OSAL_STATUS err;

  /* Create a mailbox for Battery info stream */
  pkt = post_office_create_msg(M2M2_HEADER_SZ + sizeof(post_office_config_t));
  if (pkt != NULL) {
    post_office_config_t *payload = (post_office_config_t *)&pkt->data[0];
    pkt->src = M2M2_ADDR_SYS_PM;
    pkt->dest = M2M2_ADDR_POST_OFFICE;
    pkt->length = M2M2_HEADER_SZ + sizeof(post_office_config_t);

    payload->box = M2M2_ADDR_SYS_BATT_STREAM;
    payload->cmd = POST_OFFICE_CFG_CMD_ADD_MAILBOX;
    post_office_send(pkt, &err);
  }
  NRF_LOG_DEBUG("Entered System Application Task\n");

//  delete_low_touch_config_file();
//  load_default_low_touch_config();
#ifdef USE_FS
  // FindConfigFile(&gsCfgFileFoundFlag);
#endif // USE_FS
#ifdef LOW_TOUCH_FEATURE
#ifdef DCB
  find_low_touch_DCB();
#endif
#endif
  while (1) {
    m2m2_hdr_t *response_mail = NULL;
#ifdef USE_FS
#ifdef FS_TEST_BLOCK_READ_DEBUG
    if(fs_task_read_tmp_blk_flag)
    {
      read_tmp_blk(page_num,pdata_mem,type_arg);
      fs_task_read_tmp_blk_flag = 0;
    }
#endif
#endif
    pkt = post_office_get(5000, APP_OS_CFG_SYS_TASK_INDEX);
    // adi_osal_SemPend(g_sys_task_evt_sem, ADI_OSAL_TIMEOUT_FOREVER);
    // pkt = post_office_get(ADI_OSAL_TIMEOUT_FOREVER,
    // APP_OS_CFG_SYS_TASK_INDEX);
    if (pkt != NULL) {
      ctrl_cmd = (_m2m2_app_common_cmd_t *)&pkt->data[0];
      switch (ctrl_cmd->command) {
      case M2M2_APP_COMMON_CMD_GET_VERSION_REQ: {
        response_mail = post_office_create_msg(
            M2M2_HEADER_SZ + sizeof(m2m2_app_common_version_t));
        if (response_mail != NULL) {
          uint8_t string_len = 0;
          m2m2_app_common_version_t *version_num_resp =
              (m2m2_app_common_version_t *)&response_mail->data[0];
          /* send response packet */
          response_mail->src = pkt->dest;
          response_mail->dest = pkt->src;
          version_num_resp->command = M2M2_APP_COMMON_CMD_GET_VERSION_RESP;
          version_num_resp->major = FW_VERSION_MAJOR;
          version_num_resp->minor = FW_VERSION_MINOR;
          version_num_resp->patch = FW_VERSION_PATCH;
          memcpy(&version_num_resp->verstr[0], "-Perseus", 10);
          if (SYSTEM_BUILD_VERSION_LEN > sizeof(version_num_resp->str) - 1) {
            string_len = sizeof(version_num_resp->str) - 1;
          } else {
            string_len = SYSTEM_BUILD_VERSION_LEN;
          }

          memcpy(&version_num_resp->str[0], SYSTEM_BUILD_VERSION, string_len);

          if (SYSTEM_BUILD_VERSION_LEN + SYSTEM_BUILD_COMMIT_ID_LEN <
              sizeof(version_num_resp->str)) {
            version_num_resp->str[SYSTEM_BUILD_VERSION_LEN - 1] = '|';
            memcpy(&version_num_resp->str[SYSTEM_BUILD_VERSION_LEN],
                SYSTEM_BUILD_COMMIT_ID, SYSTEM_BUILD_COMMIT_ID_LEN);
          }
          if (SYSTEM_BUILD_VERSION_LEN + SYSTEM_BUILD_COMMIT_ID_LEN +
                  SYSTEM_BUILD_TIME_LEN <
              sizeof(version_num_resp->str)) {
            version_num_resp->str[SYSTEM_BUILD_VERSION_LEN +
                                  SYSTEM_BUILD_COMMIT_ID_LEN - 1] = '|';
            memcpy(&version_num_resp->str[SYSTEM_BUILD_VERSION_LEN +
                                          SYSTEM_BUILD_COMMIT_ID_LEN],
                SYSTEM_BUILD_TIME, SYSTEM_BUILD_TIME_LEN);
          }

          // NUL terminate string
          version_num_resp->str[sizeof(version_num_resp->str) - 1] = 0;
          NRF_LOG_INFO("GetVersion System Task Info");
          NRF_LOG_DEBUG("GetVersion System Task Debug");
          post_office_send(response_mail, &err);
        }

        /*Check if GetVersion is requested by valid Tool Entity*/
        if ((pkt->src >= M2M2_ADDR_APP_DROID) &&
            (pkt->src <= M2M2_ADDR_APP_CLI_BLE))
          battery_level_alerts.tool_addr = pkt->src;
        post_office_consume_msg(pkt);
        break;
      }

      case M2M2_PM_SYS_COMMAND_SET_BAT_THR_REQ: {
        m2m2_pm_sys_bat_thr_req_t *bat_thr_req =
            (m2m2_pm_sys_bat_thr_req_t *)&pkt->data[0];
        response_mail = post_office_create_msg(
            M2M2_HEADER_SZ + sizeof(_m2m2_app_common_cmd_t));
        if (response_mail != NULL) {
          _m2m2_app_common_cmd_t *batt_thr_resp =
              (_m2m2_app_common_cmd_t *)&response_mail->data[0];

          /* Set battery critical and low levels */
          battery_level_alerts.level_low = bat_thr_req->bat_level_low;
          battery_level_alerts.level_critical = bat_thr_req->bat_level_critical;
          //Reset other fields in the structure
          //battery_level_alerts.tool_addr = M2M2_ADDR_UNDEFINED;
          battery_level_alerts.charge_alert_status = E_NORMAL_LEVEL;
          //battery_level_alerts.critical_level_flag = 0;

          /* send response packet */
          response_mail->src = pkt->dest;
          response_mail->dest = pkt->src;
          batt_thr_resp->command =
              (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_PM_SYS_COMMAND_SET_BAT_THR_RESP;
          batt_thr_resp->status =
              (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_PM_SYS_STATUS_OK;
          post_office_send(response_mail, &err);
        }
        post_office_consume_msg(pkt);
        break;
      }
#ifdef TO_BE_USED
      case M2M2_PM_SYS_COMMAND_ENABLE_BAT_CHARGE_REQ: {
        response_mail = post_office_create_msg(
            M2M2_HEADER_SZ + sizeof(_m2m2_app_common_cmd_t));
        if (response_mail != NULL) {
          _m2m2_app_common_cmd_t *resp_payload =
              (_m2m2_app_common_cmd_t *)&response_mail->data[0];

          /* send response packet */
          response_mail->src = pkt->dest;
          response_mail->dest = pkt->src;
          resp_payload->command = (M2M2_APP_COMMON_CMD_ENUM_t)
              M2M2_PM_SYS_COMMAND_ENABLE_BAT_CHARGE_RESP;

          /* Enable battery charge */
          if (Adp5360_enable_batt_charging(true) == ADP5360_SUCCESS) {
            resp_payload->status =
                (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_APP_COMMON_STATUS_OK;
          } else {
            resp_payload->status =
                (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_APP_COMMON_STATUS_ERROR;
          }
          post_office_send(response_mail, &err);
        }

        post_office_consume_msg(pkt);
        break;
      }
      case M2M2_PM_SYS_COMMAND_DISABLE_BAT_CHARGE_REQ: {
        response_mail = post_office_create_msg(
            M2M2_HEADER_SZ + sizeof(_m2m2_app_common_cmd_t));
        if (response_mail != NULL) {
          _m2m2_app_common_cmd_t *resp_payload =
              (_m2m2_app_common_cmd_t *)&response_mail->data[0];

          /* send response packet */
          response_mail->src = pkt->dest;
          response_mail->dest = pkt->src;
          resp_payload->command = (M2M2_APP_COMMON_CMD_ENUM_t)
              M2M2_PM_SYS_COMMAND_DISABLE_BAT_CHARGE_RESP;

          /* Disable battery charge */
          if (Adp5360_enable_batt_charging(false) == ADP5360_SUCCESS) {
            resp_payload->status =
                (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_APP_COMMON_STATUS_OK;
          } else {
            resp_payload->status =
                (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_APP_COMMON_STATUS_ERROR;
          }
          post_office_send(response_mail, &err);
        }
        post_office_consume_msg(pkt);
        break;
      }
#endif
      case M2M2_PM_SYS_COMMAND_SET_PWR_STATE_REQ: {
        m2m2_pm_sys_pwr_state_t *power_state =
            (m2m2_pm_sys_pwr_state_t *)&pkt->data[0];
#ifdef USE_FS
        if (power_state->state == M2M2_PM_SYS_PWR_STATE_HIBERNATE ||
            power_state->state == M2M2_PM_SYS_PWR_STATE_SHUTDOWN) {
          /* If File logging is in progress */
          if (fs_hal_write_access_state() == FS_FILE_ACCESS_IN_PROGRESS) {
            /* Force Stop Logging */
            response_mail = post_office_create_msg(
                M2M2_HEADER_SZ + sizeof(_m2m2_app_common_cmd_t));
            if (response_mail != NULL) {
              _m2m2_app_common_cmd_t *payload =
                  (_m2m2_app_common_cmd_t *)&response_mail->data[0];
              payload->command = (M2M2_APP_COMMON_CMD_ENUM_t)
                  M2M2_FILE_SYS_CMD_FORCE_STOP_LOG_REQ;
              response_mail->src = M2M2_ADDR_SYS_PM;
              response_mail->dest = M2M2_ADDR_SYS_FS;
              post_office_send(response_mail, &err);
            }
          }
        }
#endif
        response_mail = post_office_create_msg(
            M2M2_HEADER_SZ + sizeof(m2m2_pm_sys_pwr_state_t));
        if (response_mail != NULL) {
          m2m2_pm_sys_pwr_state_t *set_power_resp =
              (m2m2_pm_sys_pwr_state_t *)&response_mail->data[0];

          /* send response packet */
          response_mail->src = pkt->dest;
          response_mail->dest = pkt->src;
          set_power_resp->command = (M2M2_APP_COMMON_CMD_ENUM_t)
              M2M2_PM_SYS_COMMAND_SET_PWR_STATE_RESP;
          set_power_resp->status =
              (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_PM_SYS_STATUS_OK;
          set_power_resp->state = power_state->state;
          post_office_send(response_mail, &err);

          /* Same as Power off from Watch display page. */
          if (power_state->state == M2M2_PM_SYS_PWR_STATE_HIBERNATE) {
            /* In Watch Hibernate / power off mode ADP5360 would be still
               active, we configure a GPIO for wake up with a timer for 1s
               button press wakeup and MCU enters into WFE. When it detects
               this press, it just calls the NVIC system reset */
            enter_poweroff_mode();
          }
          /* Same as the Shipment Mode from Watch display page.
             To see it in action, Watch needs to be removed from a USB connection.
             Only then it will enter shipment mode. To bring the Watch up, you need to plug in the USB cable.
          */
          else if (power_state->state == M2M2_PM_SYS_PWR_STATE_SHUTDOWN) {
            /* When Watch is in Shipment mode, ADP5360 will be powered down as well,
               thats why USB cable needs to be plugged in come out of it */
            Adp5360_enter_shipment_mode();
          }
          else if(power_state->state == M2M2_PM_SYS_PWR_STATE_ACTIVE) {
            //Watch is already in active mode, Do nothing
          }
          else {
            //Unsupported state
            set_power_resp->status =
              (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_PM_SYS_STATUS_ERR_ARGS;
          }
        }
        post_office_consume_msg(pkt);
        break;
      }

#ifdef USE_FS
      case M2M2_PM_SYS_COMMAND_FLASH_RESET_REQ: {
        response_mail = post_office_create_msg(
            M2M2_HEADER_SZ + sizeof(m2m2_file_reset_cmd_t));
        if (response_mail != NULL) {
          m2m2_file_reset_cmd_t *format_resp =
              (m2m2_file_reset_cmd_t *)&response_mail->data[0];
          /* Switch on and mount memory */
          fs_flash_power_on(true);

          /* Format memory partition */
          /*Deletes only the data files; preserves config files*/
          if (fs_hal_flash_reset() == FS_STATUS_ERR) {
            format_resp->status = M2M2_PM_SYS_ERR_RESET;
          } else {
#ifdef ENABLE_WATCH_DISPLAY
            reset_display_vol_info();
#endif
            format_resp->status = M2M2_PM_SYS_STATUS_OK;
          }
          format_resp->command = M2M2_PM_SYS_COMMAND_FLASH_RESET_RESP;
          /* send response packet */
          response_mail->src = pkt->dest;
          response_mail->dest = pkt->src;

          post_office_send(response_mail, &err);

          /* Power off Flash */
          fs_flash_power_on(false);
        }
        post_office_consume_msg(pkt);
        break;
      }
#endif
      case M2M2_PM_SYS_COMMAND_SYSTEM_RESET_REQ: {
        response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_pm_sys_cmd_t));
        if (response_mail != NULL) {
          m2m2_pm_sys_cmd_t *set_sys_rst_resp =
              (m2m2_pm_sys_cmd_t *)&response_mail->data[0];

          /* send response packet */
          response_mail->src = pkt->dest;
          response_mail->dest = pkt->src;
          set_sys_rst_resp->command =
              (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_PM_SYS_COMMAND_SYSTEM_RESET_RESP;
          set_sys_rst_resp->status =
              (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_PM_SYS_STATUS_OK;
          post_office_send(response_mail, &err);
          MCU_HAL_Delay(20);
          /* if logging is in progress , close file */
          if(UpdateFileInfo() == true){
            NRF_LOG_INFO("Success file close ");
          }
          else {
            /* failure */
            NRF_LOG_INFO("Error file close");
          }
          rtc_timestamp_store(320);
          NVIC_SystemReset();
          // pm_System_reboot();

          NRF_LOG_INFO("Initiating Software reset");
        }
        post_office_consume_msg(pkt);
        break;
      }
      case M2M2_PM_SYS_COMMAND_SYSTEM_HW_RESET_REQ: {
        response_mail =
            post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_pm_sys_cmd_t));
        if (response_mail != NULL) {
          m2m2_pm_sys_cmd_t *set_sys_rst_resp =
              (m2m2_pm_sys_cmd_t *)&response_mail->data[0];

          /* send response packet */
          response_mail->src = pkt->dest;
          response_mail->dest = pkt->src;
          set_sys_rst_resp->command = (M2M2_APP_COMMON_CMD_ENUM_t)
              M2M2_PM_SYS_COMMAND_SYSTEM_HW_RESET_RESP;
          set_sys_rst_resp->status =
              (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_PM_SYS_STATUS_OK;
          post_office_send(response_mail, &err);
          MCU_HAL_Delay(20);
          // rtc_timestamp_store(320);//no use at here,because the reset time is
          // unfixed.
          /* if logging is in progress , close file */
          if(UpdateFileInfo() == true){
            NRF_LOG_INFO("Success file close ");
          }
          else {
            /* failure */
            NRF_LOG_INFO("Error file close");
          }
          trigger_nRF_MCU_hw_reset();
          NRF_LOG_INFO("Initiating Hardware reset");
        }
        post_office_consume_msg(pkt);
        break;
      }
      case M2M2_SENSOR_COMMON_CMD_READ_REG_16_REQ:
      case M2M2_SENSOR_COMMON_CMD_WRITE_REG_16_REQ:
        response_mail = adp5360_app_reg_access(pkt);
        if (response_mail != NULL) {
          post_office_send(response_mail, &err);
        }
        post_office_consume_msg(pkt);
        break;
      case M2M2_PM_SYS_BATTERY_LEVEL_ALERT:
        post_office_consume_msg(pkt);
        break;
      case M2M2_PM_SYS_COMMAND_GET_BAT_INFO_REQ: {
        response_mail = post_office_create_msg(
            M2M2_HEADER_SZ + sizeof(m2m2_pm_sys_bat_info_resp_t));
        if (response_mail != NULL) {
          m2m2_pm_sys_bat_info_resp_t *batt_info_resp =
              (m2m2_pm_sys_bat_info_resp_t *)&response_mail->data[0];
          BATTERY_STATUS_t bat_status;
          if (Adp5360_get_battery_details(&bat_status) != ADP5360_SUCCESS) {
            batt_info_resp->status = M2M2_PM_SYS_STATUS_ERR_ARGS;
          } else {
            batt_info_resp->status = M2M2_PM_SYS_STATUS_OK;
          }
          /* send response packet */
          response_mail->src = pkt->dest;
          response_mail->dest = pkt->src;
          batt_info_resp->command =
              (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_PM_SYS_COMMAND_GET_BAT_INFO_RESP;

          switch (bat_status.chrg_status) {
          case BATTERY_NOT_AVAILABLE: {
            batt_info_resp->bat_chrg_stat = M2M2_PM_SYS_BAT_STATE_NOT_AVAIL;
            break;
          }
          case BATTERY_CHARGING: {
            batt_info_resp->bat_chrg_stat = M2M2_PM_SYS_BAT_STATE_CHARGING;
            break;
          }
          case BATTERY_CHARGE_COMPLETE: {
            batt_info_resp->bat_chrg_stat = M2M2_PM_SYS_BAT_STATE_COMPLETE;
            break;
          }
          case BATTERY_NOT_CHARGING: {
            batt_info_resp->bat_chrg_stat = M2M2_PM_SYS_BAT_STATE_NOT_CHARGING;
            break;
          }
          case BATTERY_DETECTION: {
            batt_info_resp->bat_chrg_stat = M2M2_PM_SYS_BAT_STATE_DETECTION;
            break;
          }
          case BATTERY_CHARGER_LDO_MODE: {
            batt_info_resp->bat_chrg_stat =
                M2M2_PM_SYS_BAT_STATE_CHARGE_LDO_MODE;
            break;
          }
          case BATTERY_CHARGER_TIMER_EXPIRED: {
            batt_info_resp->bat_chrg_stat =
                M2M2_PM_SYS_BAT_STATE_CHARGE_TIMER_EXPIRED;
            break;
          }
          case BATTERY_UNKNOWN:
          default: {
            batt_info_resp->bat_chrg_stat = M2M2_PM_SYS_BAT_STATE_CHARGE_ERR;
            break;
          }
          }
          time_t return_time;
          int16_t timezone_offset;

          get_log_time_stamp(&return_time, &timezone_offset);
          batt_info_resp->timestamp = return_time + timezone_offset;
          batt_info_resp->bat_mv = bat_status.voltage_mv;
          // batt_info_resp->bat_temp = bat_status.temp_c;
          batt_info_resp->bat_lvl = bat_status.level;
          post_office_send(response_mail, &err);
        }
        post_office_consume_msg(pkt);
        break;
      }
#ifdef TO_BE_USED
      case M2M2_PM_SYS_COMMAND_SET_BAT_THR_REQ: {
        m2m2_pm_sys_bat_thr_req_t *bat_thr_req =
            (m2m2_pm_sys_bat_thr_req_t *)&pkt->data[0];
        response_mail = post_office_create_msg(
            M2M2_HEADER_SZ + sizeof(_m2m2_app_common_cmd_t));
        if (response_mail != NULL) {
          _m2m2_app_common_cmd_t *batt_thr_resp =
              (_m2m2_app_common_cmd_t *)&response_mail->data[0];

          /* Set battery critical and low levels */
          battery_level_alerts.level_low = bat_thr_req->bat_level_low;
          battery_level_alerts.level_critical = bat_thr_req->bat_level_critical;

          /* send response packet */
          response_mail->src = pkt->dest;
          response_mail->dest = pkt->src;
          batt_thr_resp->command =
              (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_PM_SYS_COMMAND_SET_BAT_THR_RESP;
          batt_thr_resp->status =
              (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_PM_SYS_STATUS_OK;
          post_office_send(response_mail, &err);
        }
        post_office_consume_msg(pkt);
        break;
      }
#endif // TO_BE_USED
      case M2M2_PM_SYS_COMMAND_GET_INFO_REQ: {
        response_mail =
            post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_pm_sys_info_t));
        if (response_mail != NULL) {
          m2m2_pm_sys_info_t *system_info =
              (m2m2_pm_sys_info_t *)&response_mail->data[0];

          /* copy system information from global system info */
          memcpy(
              system_info, (void *)&g_system_info, sizeof(m2m2_pm_sys_info_t));

          /* send response packet */
          response_mail->src = pkt->dest;
          response_mail->dest = pkt->src;
          post_office_send(response_mail, &err);
        }
        post_office_consume_msg(pkt);
        break;
      }
      case M2M2_APP_COMMON_CMD_PING_REQ: {
        m2m2_app_common_ping_t *ctrl1 = (m2m2_app_common_ping_t *)&pkt->data[0];

        ping_timer_start();
        ping_count = ctrl1->sequence_num;
        ping_pkt_sz = pkt->length;
        ping_pkt_src = pkt->src;

        post_office_consume_msg(pkt);

        break;
      }
      case M2M2_APP_COMMON_CMD_STREAM_SUBSCRIBE_REQ: {
        BattInfoSubsciberCount++;
        /* Subscribe for Mailbox */
        m2m2_hdr_t *sub_pkt = post_office_create_msg(
            M2M2_HEADER_SZ + sizeof(post_office_config_t));
        if (sub_pkt != NULL) {
          post_office_config_t *payload =
              (post_office_config_t *)&sub_pkt->data;
          payload->cmd = POST_OFFICE_CFG_CMD_MAILBOX_SUBSCRIBE;
          payload->sub = pkt->src;
          payload->box = M2M2_ADDR_SYS_BATT_STREAM;

          sub_pkt->src = pkt->dest;
          sub_pkt->dest = M2M2_ADDR_POST_OFFICE;
          post_office_send(sub_pkt, &err);
        }

        /* Send Response packet */
        response_mail = post_office_create_msg(
            M2M2_HEADER_SZ + sizeof(m2m2_app_common_sub_op_t));
        if (response_mail != NULL) {
          m2m2_app_common_sub_op_t *resp_payload =
              (m2m2_app_common_sub_op_t *)&response_mail->data;
          resp_payload->command = M2M2_APP_COMMON_CMD_STREAM_SUBSCRIBE_RESP;
          resp_payload->status = M2M2_APP_COMMON_STATUS_SUBSCRIBER_ADDED;
          resp_payload->stream = M2M2_ADDR_SYS_BATT_STREAM;
          response_mail->src = pkt->dest;
          response_mail->dest = pkt->src;
          post_office_send(response_mail, &err);
        }

        post_office_consume_msg(pkt);
        break;
      }
      case M2M2_APP_COMMON_CMD_STREAM_UNSUBSCRIBE_REQ: {
        /* UnSubscribe for Mailbox */
        m2m2_hdr_t *sub_pkt = post_office_create_msg(
            M2M2_HEADER_SZ + sizeof(post_office_config_t));
        if (sub_pkt != NULL) {
          post_office_config_t *payload =
              (post_office_config_t *)&sub_pkt->data;
          payload->cmd = POST_OFFICE_CFG_CMD_MAILBOX_UNSUBSCRIBE;
          payload->sub = pkt->src;
          payload->box = M2M2_ADDR_SYS_BATT_STREAM;

          sub_pkt->src = pkt->dest;
          sub_pkt->dest = M2M2_ADDR_POST_OFFICE;
          post_office_send(sub_pkt, &err);
        }

        /* Send Response packet */
        response_mail = post_office_create_msg(
            M2M2_HEADER_SZ + sizeof(m2m2_app_common_sub_op_t));
        if (response_mail != NULL) {
          m2m2_app_common_sub_op_t *resp_payload =
              (m2m2_app_common_sub_op_t *)&response_mail->data;
          resp_payload->command = M2M2_APP_COMMON_CMD_STREAM_UNSUBSCRIBE_RESP;

          if (BattInfoSubsciberCount <= 1) {
            BattInfoSubsciberCount = 0;
            resp_payload->status = M2M2_APP_COMMON_STATUS_SUBSCRIBER_REMOVED;
          } else {
            BattInfoSubsciberCount--;
            resp_payload->status =
                M2M2_APP_COMMON_STATUS_SUBSCRIBER_COUNT_DECREMENT;
          }
          resp_payload->stream = M2M2_ADDR_SYS_BATT_STREAM;
          response_mail->src = pkt->dest;
          response_mail->dest = pkt->src;
          post_office_send(response_mail, &err);
        }

        post_office_consume_msg(pkt);
        break;
      }
      case M2M2_PM_SYS_COMMAND_GET_DATE_TIME_REQ: {
        M2M2_PM_SYS_STATUS_ENUM_t status = M2M2_PM_SYS_STATUS_ERR_NOT_CHKD;
        m_time_struct *local_get_date_time;

        response_mail = post_office_create_msg(
            M2M2_HEADER_SZ + sizeof(m2m2_pm_sys_date_time_req_t));
        if (response_mail != NULL) {
          m2m2_pm_sys_date_time_req_t *date_time_resp =
              (m2m2_pm_sys_date_time_req_t *)&response_mail->data[0];
          time_t return_time;
          int16_t timezone_offset;

          get_log_time_stamp(&return_time, &timezone_offset);
          local_get_date_time = m_sec_to_date_time(
              return_time + timezone_offset); // For file getDateTime
                                              // timezone_offset IS required

          status = (M2M2_PM_SYS_STATUS_ENUM_t)M2M2_APP_COMMON_STATUS_OK;
          date_time_resp->year = local_get_date_time->tm_year;
          date_time_resp->month = local_get_date_time->tm_mon;
          date_time_resp->day = local_get_date_time->tm_mday;
          date_time_resp->hour = local_get_date_time->tm_hour;
          date_time_resp->minute = local_get_date_time->tm_min;
          date_time_resp->second = local_get_date_time->tm_sec;
          date_time_resp->TZ_sec = timezone_offset;

          /* send response packet */
          response_mail->src = pkt->dest;
          response_mail->dest = pkt->src;
          date_time_resp->command = (M2M2_APP_COMMON_CMD_ENUM_t)
              M2M2_PM_SYS_COMMAND_GET_DATE_TIME_RESP;
          date_time_resp->status = (M2M2_APP_COMMON_STATUS_ENUM_t)status;
          post_office_send(response_mail, &err);
        }
        post_office_consume_msg(pkt);
        break;
      }
      case M2M2_PM_SYS_COMMAND_SET_DATE_TIME_REQ : {
        m_time_struct set_date_time;
        uint32_t nTimeInSec;
        m2m2_pm_sys_date_time_req_t *date_time =
            (m2m2_pm_sys_date_time_req_t *)&pkt->data[0];

        set_date_time.tm_year = date_time->year;
        set_date_time.tm_mon = date_time->month;
        set_date_time.tm_mday = date_time->day;
        set_date_time.tm_hour = date_time->hour;
        set_date_time.tm_min = date_time->minute;
        set_date_time.tm_sec = date_time->second;

        nTimeInSec = m_date_time_to_sec(&set_date_time);

        M2M2_PM_SYS_STATUS_ENUM_t status;
        status = (M2M2_PM_SYS_STATUS_ENUM_t)M2M2_APP_COMMON_STATUS_OK;

        int32_t tz_value;

        tz_value = date_time->TZ_sec;
        rtc_timestamp_set(nTimeInSec - tz_value);
        rtc_timezone_set(tz_value);
#ifdef ENABLE_WATCH_DISPLAY
        send_private_type_value(DIS_REFRESH_SIGNAL);
#endif
        StreamBatt_Info();
        response_mail = post_office_create_msg(
            M2M2_HEADER_SZ + sizeof(_m2m2_app_common_cmd_t));
        if (response_mail != NULL) {
          _m2m2_app_common_cmd_t *date_time_resp =
              (_m2m2_app_common_cmd_t *)&response_mail->data[0];

          /* send response packet */
          response_mail->src = pkt->dest;
          response_mail->dest = pkt->src;
          date_time_resp->command = (M2M2_APP_COMMON_CMD_ENUM_t)
              M2M2_PM_SYS_COMMAND_SET_DATE_TIME_RESP;
          date_time_resp->status = (M2M2_APP_COMMON_STATUS_ENUM_t)status;
          post_office_send(response_mail, &err);
        }
        post_office_consume_msg(pkt);
        break;
      }
      case M2M2_PM_SYS_COMMAND_SET_MANUFACTURE_DATE_REQ: {
        manufacture_date_t date;

        manufacture_date_t *date_req =
            (manufacture_date_t *)&pkt->data[2];
        date.year = date_req->year;
        date.month = date_req->month;
        date.day = date_req->day;
        M2M2_PM_SYS_STATUS_ENUM_t status;

        if(NRF_SUCCESS == manufacture_date_save( &date))
        {
            status = (M2M2_PM_SYS_STATUS_ENUM_t)M2M2_APP_COMMON_STATUS_OK;
        }
        else
        {
            status = (M2M2_PM_SYS_STATUS_ENUM_t)M2M2_APP_COMMON_STATUS_ERROR;
        }
#ifdef ENABLE_WATCH_DISPLAY
        send_private_type_value(DIS_REFRESH_SIGNAL);
#endif
        response_mail = post_office_create_msg(
            M2M2_HEADER_SZ + sizeof(_m2m2_app_common_cmd_t));
        if (response_mail != NULL) {
          _m2m2_app_common_cmd_t *date_time_resp =
              (_m2m2_app_common_cmd_t *)&response_mail->data[0];

          /* send response packet */
          response_mail->src = pkt->dest;
          response_mail->dest = pkt->src;
          date_time_resp->command = (M2M2_APP_COMMON_CMD_ENUM_t)
              M2M2_PM_SYS_COMMAND_SET_MANUFACTURE_DATE_RESP;
          date_time_resp->status = (M2M2_APP_COMMON_STATUS_ENUM_t)status;
          post_office_send(response_mail, &err);
        }
        post_office_consume_msg(pkt);
        break;
      }
      case M2M2_PM_SYS_COMMAND_GET_MANUFACTURE_DATE_REQ: {
        M2M2_PM_SYS_STATUS_ENUM_t status = M2M2_PM_SYS_STATUS_ERR_NOT_CHKD;
        m_time_struct *local_get_date_time;

        response_mail = post_office_create_msg(
            M2M2_HEADER_SZ + sizeof(m2m2_manufacture_date_t));
        if (response_mail != NULL) {
          m2m2_manufacture_date_t *date_resp =
              (m2m2_manufacture_date_t *)&response_mail->data[0];
          manufacture_date_t *date =
            (manufacture_date_t *)&response_mail->data[2];
          uint16_t len = sizeof(date_resp);
          if (manufacture_date_read(date) == NRF_SUCCESS)
          {
                status = (M2M2_PM_SYS_STATUS_ENUM_t)M2M2_APP_COMMON_STATUS_OK;
          }
          else
          {
                status = (M2M2_PM_SYS_STATUS_ENUM_t)M2M2_APP_COMMON_STATUS_ERROR;
          }

          /* send response packet */
          response_mail->src = pkt->dest;
          response_mail->dest = pkt->src;
          date_resp->command = (M2M2_APP_COMMON_CMD_ENUM_t)
              M2M2_PM_SYS_COMMAND_GET_MANUFACTURE_DATE_RESP;
          date_resp->status = (M2M2_APP_COMMON_STATUS_ENUM_t)status;
          post_office_send(response_mail, &err);
        }
        post_office_consume_msg(pkt);
        break;
      }
      case M2M2_PM_SYS_COMMAND_GET_MCU_VERSION_REQ: {
        response_mail = post_office_create_msg(
            M2M2_HEADER_SZ + sizeof(m2m2_pm_sys_mcu_version_t));
        if (response_mail != NULL) {
          m2m2_pm_sys_mcu_version_t *mcu_version_response =
              (m2m2_pm_sys_mcu_version_t *)&response_mail->data[0];

          /* send response packet */
          response_mail->src = pkt->dest;
          response_mail->dest = pkt->src;
          mcu_version_response->command =
              M2M2_PM_SYS_COMMAND_GET_MCU_VERSION_RESP;
          mcu_version_response->status =
              M2M2_PM_SYS_STATUS_OK;
          mcu_version_response->mcu = M2M2_PM_SYS_MCU_M4;
          post_office_send(response_mail, &err);
        }
        post_office_consume_msg(pkt);
        break;
      }
      case M2M2_PM_GET_APPS_HEALTH_REQ: {
        response_mail = post_office_create_msg(
            M2M2_HEADER_SZ + sizeof(m2m2_get_apps_running_stat_resp_cmd_t));
        if (response_mail != NULL) {
          m2m2_get_apps_running_stat_resp_cmd_t
              *apps_health_stat_info_response =
                  (m2m2_get_apps_running_stat_resp_cmd_t *)&response_mail
                      ->data[0];
          /* send response packet */
          response_mail->src = pkt->dest;
          response_mail->dest = pkt->src;
          apps_health_stat_info_response->command =
              M2M2_PM_GET_APPS_HEALTH_RESP;
          apps_health_stat_info_response->status =
              (M2M2_PM_SYS_STATUS_ENUM_t)M2M2_APP_COMMON_STATUS_OK;
          apps_health_stat_info_response->ad5940_isr_cnt =
              Ad5940DrvGetDebugInfo();
          apps_health_stat_info_response->adpd4000_isr_cnt =
              Adpd400xDrvGetISRDebugInfo();
          apps_health_stat_info_response->adxl_isr_cnt = AdxlDrvGetDebugInfo();
          post_office_send(response_mail, &err);
        }
        post_office_consume_msg(pkt);
        break;
      }
      case M2M2_PM_SYS_COMMAND_DG2502_SW_CNTRL_REQ: {
        response_mail = post_office_create_msg(
            M2M2_HEADER_SZ + sizeof(m2m2_pm_sys_dg2502_sw_ctrl_cmd_t));
        if (response_mail != NULL) {
          m2m2_pm_sys_dg2502_sw_ctrl_cmd_t *dg2502_sw_cntrl_resp =
              (m2m2_pm_sys_dg2502_sw_ctrl_cmd_t *)&response_mail->data[0];
          m2m2_pm_sys_dg2502_sw_ctrl_cmd_t *req1 =
              (m2m2_pm_sys_dg2502_sw_ctrl_cmd_t *)&pkt->data[0];

          switch (req1->sw_name) {
#ifdef ENABLE_ECG_APP
          case M2M2_PM_SYS_DG2502_8233_SW:
            adp5360_enable_ldo(ECG_LDO, true);
            DG2502_SW_control_AD8233(req1->sw_enable);
            dg2502_sw_cntrl_resp->status = M2M2_PM_SYS_STATUS_OK;
            adp5360_enable_ldo(ECG_LDO, false);
            break;
#endif
          case M2M2_PM_SYS_DG2502_5940_SW:
            adp5360_enable_ldo(ECG_LDO, true);
            DG2502_SW_control_AD5940(req1->sw_enable);
            dg2502_sw_cntrl_resp->status = M2M2_PM_SYS_STATUS_OK;
            adp5360_enable_ldo(ECG_LDO, false);
            break;
          case M2M2_PM_SYS_DG2502_4K_SW:
            DG2502_SW_control_ADPD4000(req1->sw_enable);
            dg2502_sw_cntrl_resp->status = M2M2_PM_SYS_STATUS_OK;
            break;
          default:
            dg2502_sw_cntrl_resp->status = M2M2_PM_SYS_STATUS_ERR_ARGS;
            break;
          }

          /* send response packet */
          response_mail->src = pkt->dest;
          response_mail->dest = pkt->src;
          dg2502_sw_cntrl_resp->command =
              M2M2_PM_SYS_COMMAND_DG2502_SW_CNTRL_RESP;
          dg2502_sw_cntrl_resp->sw_name = req1->sw_name;
          dg2502_sw_cntrl_resp->sw_enable = req1->sw_enable;
          post_office_send(response_mail, &err);
        }
        post_office_consume_msg(pkt);
        break;
      }
      case M2M2_PM_SYS_COMMAND_LDO_CNTRL_REQ: {
        response_mail = post_office_create_msg(
            M2M2_HEADER_SZ + sizeof(m2m2_pm_sys_ldo_ctrl_cmd_t));
        if (response_mail != NULL) {
          m2m2_pm_sys_ldo_ctrl_cmd_t *ldo_cntrl_resp =
              (m2m2_pm_sys_ldo_ctrl_cmd_t *)&response_mail->data[0];
          m2m2_pm_sys_ldo_ctrl_cmd_t *req1 =
              (m2m2_pm_sys_ldo_ctrl_cmd_t *)&pkt->data[0];

          switch (req1->ldo_num) {
          case FS_LDO:
          case OPT_DEV_LDO:
          case ECG_LDO:
            adp5360_enable_ldo(
                req1->ldo_num, (req1->ldo_enable == 0) ? false : true);
            ldo_cntrl_resp->status = M2M2_PM_SYS_STATUS_OK;
            break;
          default:
            ldo_cntrl_resp->status = M2M2_PM_SYS_STATUS_ERR_ARGS;
            break;
          }

          /* send response packet */
          response_mail->src = pkt->dest;
          response_mail->dest = pkt->src;
          ldo_cntrl_resp->command = M2M2_PM_SYS_COMMAND_LDO_CNTRL_RESP;
          ldo_cntrl_resp->ldo_num = req1->ldo_num;
          ldo_cntrl_resp->ldo_enable = req1->ldo_enable;
          post_office_send(response_mail, &err);
        }
        post_office_consume_msg(pkt);
        break;
      }
      case M2M2_PM_SYS_COMMAND_CHIP_ID_REQ: {
        response_mail = post_office_create_msg(
            M2M2_HEADER_SZ + sizeof(m2m2_pm_sys_chip_id_cmd_t));
        if (response_mail != NULL) {
          uint8_t devID;
          uint8_t manufacture_id;
          m2m2_pm_sys_chip_id_cmd_t *chip_id_resp =
              (m2m2_pm_sys_chip_id_cmd_t *)&response_mail->data[0];
          m2m2_pm_sys_chip_id_cmd_t *req1 =
              (m2m2_pm_sys_chip_id_cmd_t *)&pkt->data[0];

          switch (req1->chip_name) {
          case M2M2_PM_SYS_ADXL362:
            // Get Part ID of ADXL362
            chip_id_resp->chip_id = GetPartIdAdxl362();
            chip_id_resp->status = M2M2_PM_SYS_STATUS_OK;
            break;
          case M2M2_PM_SYS_ADPD4K:
            // Get Chip ID of ADPD4000
            chip_id_resp->chip_id = GetChipIdAdpd();
            chip_id_resp->status = M2M2_PM_SYS_STATUS_OK;
            break;
          case M2M2_PM_SYS_ADP5360:
            // Get Device ID of ADP5360
            Adp5360_getDevID(&devID);
            chip_id_resp->chip_id = devID;
            chip_id_resp->status = M2M2_PM_SYS_STATUS_OK;
            break;
          case M2M2_PM_SYS_AD5940:
            // Get chip id of AD5940
            adp5360_enable_ldo(ECG_LDO, true); // power on LDO
            ad5940_port_Init();
            AD5940_HWReset();
            chip_id_resp->chip_id = AD5940_GetChipID();
            ad5940_port_deInit();
            adp5360_enable_ldo(ECG_LDO, false); // power off LDO
            chip_id_resp->status = M2M2_PM_SYS_STATUS_OK;
            break;
#ifdef USE_FS
          case M2M2_PM_SYS_NAND_FLASH:
            // Get Device id of NAND Flash
            nand_flash_read_id(&manufacture_id, &devID);
            chip_id_resp->chip_id = devID;
            chip_id_resp->status = M2M2_PM_SYS_STATUS_OK;
            break;
#endif
          case M2M2_PM_SYS_AD7156:
            // Get chip ID of AD7156
            devID = getAD7156ChipID();
            chip_id_resp->chip_id = devID;
            chip_id_resp->status = M2M2_PM_SYS_STATUS_OK;
            break;
          default:
            chip_id_resp->status = M2M2_PM_SYS_STATUS_ERR_ARGS;
            break;
          }

          /* send response packet */
          response_mail->src = pkt->dest;
          response_mail->dest = pkt->src;
          chip_id_resp->command = M2M2_PM_SYS_COMMAND_CHIP_ID_RESP;
          chip_id_resp->chip_name = req1->chip_name;
          post_office_send(response_mail, &err);
        }
        post_office_consume_msg(pkt);
        break;
      }
      case M2M2_PM_SYS_COMMAND_CAP_SENSE_TEST_REQ: {
        touch_test_func(pkt);
        break;
      }
      case M2M2_PM_SYS_COMMAND_ENTER_BOOTLOADER_REQ: {
        response_mail = post_office_create_msg(
            M2M2_HEADER_SZ + sizeof(m2m2_pm_sys_enter_bloader_cmd_t));
        if (response_mail != NULL) {
          m2m2_pm_sys_enter_bloader_cmd_t *resp =
              (m2m2_pm_sys_enter_bloader_cmd_t *)&response_mail->data[0];

          /* send response packet */
          response_mail->src = pkt->dest;
          response_mail->dest = pkt->src;
          resp->command = M2M2_PM_SYS_COMMAND_ENTER_BOOTLOADER_RESP;

          if (NRF_SUCCESS != enter_bootloader_and_restart()) {
            resp->status = M2M2_PM_SYS_STATUS_ERR_ARGS;
          } else {
            resp->status = M2M2_PM_SYS_STATUS_OK; // in fact, can't go here.
          }

          post_office_send(response_mail, &err);
        }
        post_office_consume_msg(pkt);
        break;
      }

      case M2M2_APP_COMMON_CMD_SENSOR_STATUS_QUERY_RESP: {
        m2m2_hdr_t *resp_pkt = NULL;
        ADI_OSAL_STATUS err;

        resp_pkt = post_office_create_msg(
            M2M2_HEADER_SZ + sizeof(m2m2_app_common_sub_op_t));
        if (resp_pkt != NULL) {

          m2m2_app_common_sub_op_t *resp =
              (m2m2_app_common_sub_op_t *)&(resp_pkt->data[0]);
          m2m2_app_common_status_t *in_resp =
              (m2m2_app_common_status_t *)&(pkt->data[0]);
          m2m2_pm_sys_sensor_app_status *sensor_info_resp =
              (m2m2_pm_sys_sensor_app_status *)&(
                  gsSensorAppsInfoPayload->app_info[gsActivePsAppCnt]);

          sensor_info_resp->sensor_app_id = pkt->src;
          sensor_info_resp->num_subscribers = in_resp->num_subscribers;
          sensor_info_resp->num_start_reqs = in_resp->num_start_reqs;
          sensor_info_resp->sensor_stream = in_resp->stream;

#ifdef USE_FS
          resp_pkt->dest = M2M2_ADDR_SYS_FS;
          resp_pkt->src = M2M2_ADDR_SYS_PM; // gsSensorAppsInfoPkt
          resp->stream = in_resp->stream;//get_file_routing_table_entry_stream(pkt->src);
          resp->command = M2M2_FILE_SYS_CMD_GET_FS_STREAM_SUB_STATUS_REQ;
#else
          resp_pkt->dest = M2M2_ADDR_SYS_PM;
          resp_pkt->src = M2M2_ADDR_SYS_PM; // gsSensorAppsInfoPkt
          resp->command = M2M2_PM_SYS_COMMAND_GET_APPS_INFO_REQ;
          gsPsAppNumber++;
          gsActivePsAppCnt++;
#endif
          post_office_send(resp_pkt, &err);
        }
        post_office_consume_msg(pkt);
        break;
      }
      case M2M2_FILE_SYS_CMD_GET_FS_STREAM_SUB_STATUS_RESP: {
        m2m2_hdr_t *resp_pkt = NULL;
        ADI_OSAL_STATUS err;

        resp_pkt = post_office_create_msg(
            M2M2_HEADER_SZ + sizeof(_m2m2_app_common_cmd_t));
        if (resp_pkt != NULL) {

          m2m2_app_common_status_t *resp =
              (m2m2_app_common_status_t *)&(resp_pkt->data[0]);
          m2m2_file_sys_get_subs_status_resp_t *in_resp =
              (m2m2_file_sys_get_subs_status_resp_t *)&(pkt->data[0]);
          m2m2_pm_sys_sensor_app_status *sensor_info_resp =
              (m2m2_pm_sys_sensor_app_status *)&(
                  gsSensorAppsInfoPayload->app_info[gsActivePsAppCnt]);

          sensor_info_resp->fs_sub_stat = in_resp->subs_state;

          //            gsSensorAppsInfoPayload->num_sensor_apps =
          //            gsPsAppNumber;
          resp_pkt->dest = M2M2_ADDR_SYS_PM;
          resp_pkt->src = M2M2_ADDR_SYS_PM; // gsSensorAppsInfoPkt
          resp->command = M2M2_PM_SYS_COMMAND_GET_APPS_INFO_REQ;
          /* Handle next sensor App once M2M2_ADDR_SENSOR_ADPD4000 is completed */
          if( !gnAdpdSensorApp )
            gsPsAppNumber++;
          gsActivePsAppCnt++;
          post_office_send(resp_pkt, &err);
        }
        post_office_consume_msg(pkt);
        break;
      }
      case M2M2_PM_SYS_BLE_GET_MAX_TX_PKT_COMB_CNT_REQ: {
        response_mail = post_office_create_msg(
            M2M2_HEADER_SZ + sizeof(m2m2_ble_max_tx_pkt_comb_cnt_resp_cmd_t));
        if (response_mail != NULL) {
          m2m2_ble_max_tx_pkt_comb_cnt_resp_cmd_t *ble_max_tx_pkt_comb_cnt_resp =
              (m2m2_ble_max_tx_pkt_comb_cnt_resp_cmd_t *)&response_mail->data[0];

          /* send response packet */
          response_mail->src = pkt->dest;
          response_mail->dest = pkt->src;
          ble_max_tx_pkt_comb_cnt_resp->command =
              M2M2_PM_SYS_BLE_GET_MAX_TX_PKT_COMB_CNT_RESP;
          ble_max_tx_pkt_comb_cnt_resp->status =
              M2M2_PM_SYS_STATUS_OK;
          ble_max_tx_pkt_comb_cnt_resp->max_tx_pkt_comb_cnt = get_max_tx_pkt_comb_cnt();
          post_office_send(response_mail, &err);
        }
        post_office_consume_msg(pkt);
        break;
      }
      case M2M2_PM_SYS_BLE_SET_MAX_TX_PKT_COMB_CNT_REQ: {
        m2m2_ble_max_tx_pkt_comb_cnt_resp_cmd_t *ble_max_tx_pkt_comb_cnt_req =
              (m2m2_ble_max_tx_pkt_comb_cnt_resp_cmd_t *)&pkt->data[0];
        response_mail = post_office_create_msg(
            M2M2_HEADER_SZ + sizeof(m2m2_ble_max_tx_pkt_comb_cnt_resp_cmd_t));
        if (response_mail != NULL) {
          uint8_t ret_val;
          m2m2_ble_max_tx_pkt_comb_cnt_resp_cmd_t *ble_max_tx_pkt_comb_cnt_resp =
              (m2m2_ble_max_tx_pkt_comb_cnt_resp_cmd_t *)&response_mail->data[0];

          ret_val = set_max_tx_pkt_comb_cnt(ble_max_tx_pkt_comb_cnt_req->max_tx_pkt_comb_cnt);
          /* send response packet */
          response_mail->src = pkt->dest;
          response_mail->dest = pkt->src;
          ble_max_tx_pkt_comb_cnt_resp->command =
              M2M2_PM_SYS_BLE_SET_MAX_TX_PKT_COMB_CNT_RESP;
          ble_max_tx_pkt_comb_cnt_resp->status = (ret_val == 0)?
              M2M2_PM_SYS_STATUS_OK : M2M2_PM_SYS_STATUS_ERR_ARGS;
          ble_max_tx_pkt_comb_cnt_resp->max_tx_pkt_comb_cnt =
                                    ble_max_tx_pkt_comb_cnt_req->max_tx_pkt_comb_cnt;
          post_office_send(response_mail, &err);
        }
        post_office_consume_msg(pkt);
        break;
      }
#ifdef HIBERNATE_MD_EN
      case M2M2_PM_SYS_GET_HIBERNATE_MODE_STATUS_REQ: {
        response_mail = post_office_create_msg(
            M2M2_HEADER_SZ + sizeof(m2m2_hibernate_mode_status_resp_cmd_t));
        if (response_mail != NULL) {
          m2m2_hibernate_mode_status_resp_cmd_t *hib_mode_status_resp =
              (m2m2_hibernate_mode_status_resp_cmd_t *)&response_mail->data[0];

          /* send response packet */
          response_mail->src = pkt->dest;
          response_mail->dest = pkt->src;
          hib_mode_status_resp->command =
              M2M2_PM_SYS_GET_HIBERNATE_MODE_STATUS_RESP;
          hib_mode_status_resp->status =
              M2M2_PM_SYS_STATUS_OK;
          hib_mode_status_resp->hib_mode_status = get_hib_mode_control();
          post_office_send(response_mail, &err);
        }
        post_office_consume_msg(pkt);
        break;
      }
      case M2M2_PM_SYS_SET_HIBERNATE_MODE_STATUS_REQ: {
        m2m2_hibernate_mode_status_resp_cmd_t *hib_mode_status_req =
              (m2m2_hibernate_mode_status_resp_cmd_t *)&pkt->data[0];
        response_mail = post_office_create_msg(
            M2M2_HEADER_SZ + sizeof(m2m2_hibernate_mode_status_resp_cmd_t));
        if (response_mail != NULL) {
          uint8_t ret_val;
          m2m2_hibernate_mode_status_resp_cmd_t *hib_mode_status_resp =
              (m2m2_hibernate_mode_status_resp_cmd_t *)&response_mail->data[0];

          ret_val = set_hib_mode_control(hib_mode_status_req->hib_mode_status);
          /* send response packet */
          response_mail->src = pkt->dest;
          response_mail->dest = pkt->src;
          hib_mode_status_resp->command =
              M2M2_PM_SYS_SET_HIBERNATE_MODE_STATUS_RESP;
          hib_mode_status_resp->status = (ret_val == 0)?
              M2M2_PM_SYS_STATUS_OK : M2M2_PM_SYS_STATUS_ERR_ARGS;
          hib_mode_status_resp->hib_mode_status =
                                    hib_mode_status_req->hib_mode_status;
          post_office_send(response_mail, &err);
        }
        post_office_consume_msg(pkt);
        break;
      }
#endif
#ifdef LOW_TOUCH_FEATURE
      case M2M2_PM_SYS_COMMAND_ENABLE_USER_CONFIG_LOG_REQ: {
        //            gsPsReadyFlag = 1;
        if (usb_pkt_src != M2M2_ADDR_UNDEFINED) {
          low_touch_info.tool_addr = usb_pkt_src;
        }
        else if (ble_pkt_src != M2M2_ADDR_UNDEFINED) {
          low_touch_info.tool_addr = ble_pkt_src;
        }
        /*if (pkt->src != M2M2_ADDR_UNDEFINED) {
            low_touch_info.tool_addr = pkt->src;
          }*/
        low_touch_info.cmd = ctrl_cmd->command;
        if (!gLowTouchRunning) {
          GetVolInfo();
        } else {
          SendLowTouchErrResp(
              M2M2_PM_SYS_STATUS_LOW_TOUCH_LOGGING_ALREADY_STARTED);
        }
        post_office_consume_msg(pkt);
        break;
      }
      case M2M2_FILE_SYS_CMD_VOL_INFO_RESP: {
        m2m2_file_sys_vol_info_resp_t *fs_free_space_resp =
            (m2m2_file_sys_vol_info_resp_t *)&pkt->data[0];
        if (fs_free_space_resp->status == M2M2_FILE_SYS_STATUS_OK) {
          if (fs_free_space_resp->usedmemory < MAX_USABLE_MEMORY_SIZE) {
            gSendRespToPmApp = 1;
            GetFileCount();
          } else {
            SendLowTouchErrResp(M2M2_PM_SYS_STATUS_LOW_TOUCH_MEMORY_FULL_ERR);
            MaxFileErr(); /*Memory is full ; give indication on Display*/
          }
        } else {
          LowTouchErr(); /* Error while getting the fs vol info, give indication on Display*/
        }
        post_office_consume_msg(pkt);
        break;
      }
      case M2M2_FILE_SYS_CMD_GET_NUMBER_OF_FILE_RESP: {
        m2m2_file_sys_get_file_count_pkt_t *get_num_files_resp =
            (m2m2_file_sys_get_file_count_pkt_t *)&pkt->data[0];
        if (get_num_files_resp->status != M2M2_APP_COMMON_STATUS_ERROR) {
          if (get_num_files_resp->file_count < MAX_FILE_COUNT) {
            uint8_t *gsConfigTmpFile_ptr = (uint8_t *)&gsConfigTmpFile[0];
#ifdef DCB
            // Load from gen Blk DCB
            find_low_touch_DCB();
            if (gbDCBCfgFoundFlag) {
              GEN_BLK_DCB_STATUS_t ret_val;
              gsCfgWrIndex = 0;
              SetCfgCopyAvailableFlag(
                  false); /*update the config file copy availability flag-since
                             new DCB is going to be loaded*/
              ret_val = copy_lt_config_from_gen_blk_dcb(
                  &gsConfigTmpFile_ptr[gsCfgWrIndex], &gsCfgWrIndex);
              if(GEN_BLK_DCB_STATUS_ERR == ret_val)
              {
                SendLowTouchErrResp(M2M2_PM_SYS_STATUS_CONFIG_FILE_NOT_FOUND);
                LowTouchErr(); // config file read failed
              }
              else
              {
                gsConfigFileSize = gsCfgWrIndex - CONFIG_FILE_SUMMARY_PKT_SIZE;
                InitConfigParam(1); // Enable sending start commands
                SendUserConfigCommands();
              }
            } else {
#endif
              SendFindConfigFileReq();
#ifdef DCB
            }
#endif
          } else {
            SendLowTouchErrResp(M2M2_PM_SYS_STATUS_LOW_TOUCH_MAX_FILE_ERR);
            MaxFileErr(); /*Maximum File count reached; give indication on Display*/
          }
        } else {
          LowTouchErr(); /* Error while getting the num of files, give indication on Display*/
        }

        post_office_consume_msg(pkt);
        break;
      }
      case M2M2_PM_SYS_COMMAND_DISABLE_USER_CONFIG_LOG_REQ: {
        if (gLowTouchRunning) {
          if (usb_pkt_src != M2M2_ADDR_UNDEFINED) {
            low_touch_info.tool_addr = usb_pkt_src;
          }
          else if (ble_pkt_src != M2M2_ADDR_UNDEFINED) {
            low_touch_info.tool_addr = ble_pkt_src;
          }
          /*if (pkt->src != M2M2_ADDR_UNDEFINED) {
            low_touch_info.tool_addr = pkt->src;
          }*/
          low_touch_info.cmd = ctrl_cmd->command;
          gSendRespToPmApp = 1;
#ifdef LOW_TOUCH_FORCE_STOP_LOG
          SendForceStopLogging();
#else
          InitConfigParam(0);
          SendUserConfigCommands();
          // GetSensorAppsStatus();
#endif // LOW_TOUCH_FORCE_STOP_LOG
        }
        post_office_consume_msg(pkt);
        break;
      }
      case M2M2_FILE_SYS_CMD_FORCE_STOP_LOG_RESP: {
        // SendLowTouchErrResp(M2M2_PM_SYS_STATUS_USER_CONFIG_LOG_DISABLED);
        if (ctrl_cmd->status == M2M2_FILE_SYS_ERR_MEMORY_FULL) {
          /*Force log stop response came because of memory full event*/
          SendLowTouchErrResp(M2M2_PM_SYS_STATUS_LOW_TOUCH_MEMORY_FULL_ERR);
          /*low touch log stopped due to mem-full; stop the sensors and inform
           * the tool*/
          LowTouchErr(); // provide indication on Display
        } else {
          if (battery_level_alerts.critical_level_flag) {
            battery_level_alerts.critical_level_flag = 0;
            /*Force log stop respose came because of battery critical event*/
            SendBatteryCriticalErrResp();
          }
        }

        /*Force log stop respose came as part of low touch log stop cmd */
        GetSensorAppsStatus();
        post_office_consume_msg(pkt);
        break;
      }
#endif
      case M2M2_PM_SYS_COMMAND_GET_APPS_INFO_REQ: {
        M2M2_ADDR_ENUM_t nSensorAppAddr;
        /*As M2M2_PM_SYS_COMMAND_GET_APPS_INFO_REQ keeps getting called in a sequence
         to go over the apps, variable 'i' is used to go over the ADPD slots within the ADPD
         application, hence made static, to continue using it from the previous value.*/
        static uint8_t slot_index=0;
        m2m2_hdr_t *resp_pkt = NULL;
        ADI_OSAL_STATUS err;

        resp_pkt = post_office_create_msg(
            M2M2_HEADER_SZ + sizeof(_m2m2_app_common_cmd_t));
        if (resp_pkt != NULL) {

          while (gsPsAppNumber <
                 (gsRoutingTableSize)) // 20 -> max num of sensor apps in PM
          {
            if (!gsPsAppNumber) {
              get_routing_table_size(&gsRoutingTableSize);
              gsPsAppNumber = get_routing_table_index(
                  M2M2_ADDR_SYS_PM); // update PM task index
              gsPsAppNumber += 1;    // exclude PM application
              gsActivePsAppCnt = 0;
              gsStausCmdSrcAddr = pkt->src; // source address of the command
            }

            nSensorAppAddr = get_routing_table_element(gsPsAppNumber);
            if (nSensorAppAddr) {
              m2m2_app_common_status_t *resp =
                  (m2m2_app_common_status_t *)&(resp_pkt->data[0]);
              resp_pkt->dest = nSensorAppAddr;
              resp_pkt->checksum = 0x0000;
              resp_pkt->src = M2M2_ADDR_SYS_PM;
              resp->command = M2M2_APP_COMMON_CMD_SENSOR_STATUS_QUERY_REQ;
              if( nSensorAppAddr == M2M2_ADDR_SENSOR_ADPD4000 )
              {
                gnAdpdSensorApp = 1; //!< Set the flag to indicate that ADPD4000 Slots are being queried for status
                resp->stream =
                        (M2M2_ADDR_ENUM_t)(slot_index++ + M2M2_ADDR_SENSOR_ADPD_STREAM1);
                if( slot_index == (ADPD4K_SLOT_L-1) )
                {
                  slot_index=(ADPD4K_SLOT_A-1); //!< reset after going through 12 slots
                  gnAdpdSensorApp = 0; //!< Clear the flag to indicate ADPD4000 Slots queried for status is completed
                }
              }
              post_office_send(resp_pkt, &err);
              break;
            } else {
              gsPsAppNumber++; // Move onto next app in the routing table
            }
          }
          if (gsPsAppNumber >=
              (gsRoutingTableSize)) // 20 -> max num of sensor apps in PM
          {
            m2m2_hdr_t *sensorAppsInfoHdr = NULL;
            gsSensorAppsInfoHdr->src = M2M2_ADDR_SYS_PM;
            gsSensorAppsInfoHdr->dest = gsStausCmdSrcAddr;
            gsSensorAppsInfoHdr->length = sizeof(gsSensorAppsInfoPkt);
            gsSensorAppsInfoHdr->checksum = 0x0000;
            gsSensorAppsInfoPayload->command =
                M2M2_PM_SYS_COMMAND_GET_APPS_INFO_RESP;
            gsSensorAppsInfoPayload->status = M2M2_APP_COMMON_STATUS_OK;
            gsSensorAppsInfoPayload->num_sensor_apps = gsActivePsAppCnt;
            gsPsAppNumber = 0;
            gsActivePsAppCnt = 0;

            sensorAppsInfoHdr = post_office_create_msg(
                M2M2_HEADER_SZ + sizeof(m2m2_pm_sys_sensor_apps_info_req_t));
            memcpy(sensorAppsInfoHdr, gsSensorAppsInfoHdr,
                gsSensorAppsInfoHdr->length);
            post_office_send(sensorAppsInfoHdr, &err);
            post_office_consume_msg(resp_pkt);

#if 0
                gLowTouchRunning = 0;
                memcpy(gsSensorStatusRespPkt,&sensorAppsInfoHdr->data[0],sizeof(m2m2_pm_sys_sensor_apps_info_req_t)); /*copy the sensor status resp pkt payload*/
                SendSensorStopCmds();       /*start sending the sensor stop commands*/
                post_office_consume(sensorAppsInfoHdr);
#endif
          }
        }
        post_office_consume_msg(pkt);
        break;
      }
      case M2M2_PM_SYS_COMMAND_GET_APPS_INFO_RESP: {
#ifdef LOW_TOUCH_FEATURE
        if (!gb_stop_streaming_if_no_logging)
          gLowTouchRunning = 0;
#endif
        memcpy(gsSensorStatusRespPkt, &pkt->data[0],
            sizeof(
                m2m2_pm_sys_sensor_apps_info_req_t)); /*copy the sensor status
                                                         resp pkt payload*/
        gsSensorStopCnt = 0, gsSensorSubCnt = 0, gsSensorAppsCnt = 0;
        SendSensorStopCmds(); /*start sending the sensor stop commands*/
        post_office_consume_msg(pkt);
        break;
      }
#ifdef LOW_TOUCH_FEATURE
      case M2M2_FILE_SYS_CMD_FIND_CONFIG_FILE_RESP: {
        m2m2_file_sys_cmd_t *find_cfg_file_resp =
            (m2m2_file_sys_cmd_t *)&pkt->data[0];
        if (find_cfg_file_resp->status == M2M2_FILE_SYS_CONFIG_FILE_FOUND) {
          if (IsCfgCopyAvailable()) {
            InitConfigParam(1); // Enable sending start commands
            SendUserConfigCommands();
          } else {
            gsCfgWrIndex = 0;
            ReadConfigFile();
          }
        } else {
          SendLowTouchErrResp(M2M2_PM_SYS_STATUS_CONFIG_FILE_NOT_FOUND);
          LowTouchErr(); // config file doesn't exist
        }
        post_office_consume_msg(pkt);
        break;
      }
      case M2M2_FILE_SYS_CMD_DOWNLOAD_LOG_RESP: {
        m2m2_file_sys_download_log_stream_t *resp = (m2m2_file_sys_download_log_stream_t *)&pkt->data[0];
        if (resp->status == M2M2_FILE_SYS_STATUS_OK ||
            resp->status == M2M2_FILE_SYS_END_OF_FILE) {
          uint8_t *gsConfigTmpFile_ptr = (uint8_t *)&gsConfigTmpFile[0];
          memcpy(&gsConfigTmpFile_ptr[gsCfgWrIndex], &resp->byte_stream[0],resp->len_stream);
          gsCfgWrIndex += resp->len_stream;
          if (resp->status == M2M2_FILE_SYS_END_OF_FILE) {
            gsConfigFileSize = gsCfgWrIndex - CONFIG_FILE_SUMMARY_PKT_SIZE;
            //                    gsConfigFileFromFlash =1;
            InitConfigParam(1); // Enable sending start commands
            SendUserConfigCommands();
          }
        } else {
          SendLowTouchErrResp(M2M2_PM_SYS_STATUS_CONFIG_FILE_READ_ERR);
          LowTouchErr(); // Reading config file failed
        }
        post_office_consume_msg(pkt);
        break;
      }
      case M2M2_PS_SYS_COMMAND_GET_LOW_TOUCH_LOGGING_STATUS_REQ: {
        response_mail =
            post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_pm_sys_cmd_t));
        if (response_mail != NULL) {
          m2m2_pm_sys_cmd_t *low_touch_resp =
              (m2m2_pm_sys_cmd_t *)&response_mail->data[0];

          /* send response packet */
          response_mail->src = pkt->dest;
          response_mail->dest = pkt->src;
          response_mail->checksum = 0x0000;
          low_touch_resp->command =
              M2M2_PS_SYS_COMMAND_GET_LOW_TOUCH_LOGGING_STATUS_RESP;

          if (gLowTouchRunning)
            low_touch_resp->status =
                M2M2_PM_SYS_STATUS_LOW_TOUCH_LOGGING_IN_PROGRESS;
          else
            low_touch_resp->status =
                M2M2_PM_SYS_STATUS_LOW_TOUCH_LOGGING_NOT_STARTED;

          post_office_send(response_mail, &err);
        }
        post_office_consume_msg(pkt);
        break;
      }
      case M2M2_PM_SYS_COMMAND_ACTIVATE_TOUCH_SENSOR_REQ: {

        uint8_t ret_val;
        if (gen_blk_get_dcb_present_flag() || gsCfgFileFoundFlag)
        {
          ret_val =
              EnableLowTouchDetection(true); // enable  the low touch detection mechanism
        }
        else
        {
          //No LT config file
          ret_val = 1;//Error
        }
        response_mail =
            post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_pm_sys_cmd_t));
        if (response_mail != NULL) {
          m2m2_pm_sys_cmd_t *resp =
              (m2m2_pm_sys_cmd_t *)&response_mail->data[0];

          /*Send the response*/
          response_mail->dest = pkt->src;
          response_mail->src = pkt->dest;

          resp->command = (M2M2_APP_COMMON_CMD_ENUM_t)
              M2M2_PM_SYS_COMMAND_ACTIVATE_TOUCH_SENSOR_RESP;
          resp->status =
              (ret_val == 0)
                  ? (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_PM_SYS_STATUS_OK
                  : (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_PM_SYS_STATUS_ERR_ARGS;
          post_office_send(response_mail, &err);
        }
        post_office_consume_msg(pkt);
        break;
      }

      case M2M2_PM_SYS_COMMAND_DEACTIVATE_TOUCH_SENSOR_REQ: {

        uint8_t ret_val;
        ret_val = EnableLowTouchDetection(false); // disable  the low touch detection
                                           // mechanism
        response_mail =
            post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_pm_sys_cmd_t));
        if (response_mail != NULL) {
          m2m2_pm_sys_cmd_t *resp =
              (m2m2_pm_sys_cmd_t *)&response_mail->data[0];

          /*Send the response*/
          response_mail->dest = pkt->src;
          response_mail->src = pkt->dest;
          resp->command = (M2M2_APP_COMMON_CMD_ENUM_t)
              M2M2_PM_SYS_COMMAND_DEACTIVATE_TOUCH_SENSOR_RESP;
          resp->status =
              (ret_val == 0)
                  ? (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_PM_SYS_STATUS_OK
                  : (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_PM_SYS_STATUS_ERR_ARGS;
          post_office_send(response_mail, &err);
        }
        post_office_consume_msg(pkt);
        break;
      }
#endif // LOW_TOUCH_FEATURE

      case M2M2_PM_SYS_COMMAND_FORCE_STREAM_STOP_REQ: {
        gnForceStopReqAddr = pkt->src; // Update from which src Force stop
                                       // Request was received(ble/usb)
        // Force-stop Sensor streaming that wasn't stopped
        SendForceStopStreaming();
        post_office_consume_msg(pkt);
        break;
      }
#ifdef DCB
      case M2M2_DCB_COMMAND_QUERY_STATUS_REQ: {

        response_mail = post_office_create_msg(
            M2M2_HEADER_SZ + sizeof(m2m2_dcb_block_status_t));
        if (response_mail != NULL) {
          m2m2_dcb_block_status_t *resp =
              (m2m2_dcb_block_status_t *)&response_mail->data[0];

          memset(resp->dcb_blk_array, 0, sizeof(resp->dcb_blk_array));

          // Query & Fill in the DCB Block status currently in the firmware
#ifdef LOW_TOUCH_FEATURE
          resp->dcb_blk_array[ADI_DCB_GENERAL_BLOCK_IDX] =
              gen_blk_get_dcb_present_flag();
          resp->dcb_blk_array[ADI_DCB_LT_APP_LCFG_BLOCK_IDX] =
              lt_app_lcfg_get_dcb_present_flag();
#endif
          resp->dcb_blk_array[ADI_DCB_ADPD4000_BLOCK_IDX] =
              adpd4000_get_dcb_present_flag();
          resp->dcb_blk_array[ADI_DCB_ADXL362_BLOCK_IDX] =
              adxl_get_dcb_present_flag();
#ifdef ENABLE_PPG_APP
          resp->dcb_blk_array[ADI_DCB_PPG_BLOCK_IDX] =
              ppg_get_dcb_present_flag();
#endif
#ifdef ENABLE_ECG_APP
          resp->dcb_blk_array[ADI_DCB_ECG_BLOCK_IDX] =
              ecg_get_dcb_present_flag();
#endif
#ifdef ENABLE_EDA_APP
          resp->dcb_blk_array[ADI_DCB_EDA_BLOCK_IDX] =
              eda_get_dcb_present_flag();
#endif
#ifdef ENABLE_BCM_APP
          resp->dcb_blk_array[ADI_DCB_BCM_BLOCK_IDX] =
              bcm_get_dcb_present_flag();
#endif
          resp->dcb_blk_array[ADI_DCB_AD7156_BLOCK_IDX] =
              ad7156_get_dcb_present_flag();
#ifdef USER0_CONFIG_APP
          resp->dcb_blk_array[ADI_DCB_USER0_BLOCK_IDX] =
              user0_blk_get_dcb_present_flag();
#endif

          /*Send the response*/
          response_mail->dest = pkt->src;
          response_mail->src = pkt->dest;
          resp->command =
              (M2M2_DCB_COMMAND_ENUM_t)M2M2_DCB_COMMAND_QUERY_STATUS_RESP;
          resp->status =
              M2M2_PM_SYS_STATUS_OK;
          post_office_send(response_mail, &err);
        }
        post_office_consume_msg(pkt);
        break;
      }
#ifdef LOW_TOUCH_FEATURE
      case M2M2_DCB_COMMAND_READ_CONFIG_REQ: {
        response_mail = lt_app_lcfg_dcb_command_read_config(pkt);
        if (response_mail != NULL) {
          post_office_send(response_mail, &err);
        }
        post_office_consume_msg(pkt);
        break;
      }
      case M2M2_DCB_COMMAND_WRITE_CONFIG_REQ: {

        response_mail = lt_app_lcfg_dcb_command_write_config(pkt);
        if (response_mail != NULL) {
          post_office_send(response_mail, &err);
        }
        post_office_consume_msg(pkt);
        break;
      }
      case M2M2_DCB_COMMAND_ERASE_CONFIG_REQ: {

        response_mail = lt_app_lcfg_dcb_command_delete_config(pkt);
        if (response_mail != NULL) {
          post_office_send(response_mail, &err);
        }
        post_office_consume_msg(pkt);
        break;
      }
#endif//LOW_TOUCH_FEATURE
#endif

      case M2M2_FILE_SYS_CMD_STOP_LOGGING_RESP: {
        /* check if memory full event has occured */
         m2m2_file_sys_stop_log_cmd_t *stop_log_resp = (m2m2_file_sys_stop_log_cmd_t *)&pkt->data[0];
         if(stop_log_resp->status == (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_FILE_SYS_ERR_MEMORY_FULL) {
          /*low touch log stopped due to mem-full; stop the sensors and inform
          * the tool*/
          MaxFileErr(); /*Memory is full ; give indication on Display*/
        }
        //post_office_consume_msg(pkt);
        //break;//intentionally commented out to let the default case for
        // LT execute
      }

      /* Insert new cases to be handled above this comment line */

      case M2M2_APP_COMMON_CMD_STREAM_STOP_RESP: {
        gsSensorStopCnt++; // Increment the number of stop commands sent
        if (gb_sensor_stop_cmds) {
          gb_stop_streaming_if_no_logging = 1;
          gb_sensor_stop_cmds = 0;
          SendSensorStopCmds(); // Send Next stop command
          post_office_consume_msg(pkt);
          break;
        } else
          ; // break;//intentionally commented out to let the default case for
            // LT execute
      }
      case M2M2_APP_COMMON_CMD_STREAM_UNSUBSCRIBE_RESP: {
        gsSensorSubCnt++; // Increment the number of unsub commands sent
        if (gb_sensor_stop_cmds) {
          gb_stop_streaming_if_no_logging = 1;
          if (gb_unsub_only) {
            gb_unsub_only = 0;
            gb_sensor_stop_cmds = 0;
            SendSensorStopCmds(); // Send Next stop command
          }
          post_office_consume_msg(pkt);
          break;
        } else
          ; // break;//intentionally commented out to let the default case for
            // LT execute
      }
      /* Add new cases to be handled above the case for
         M2M2_APP_COMMON_CMD_STREAM_STOP_RESP */
      default: {
#ifdef LOW_TOUCH_FEATURE
        //if(0x28 != ctrl_cmd->command)
        //  NRF_LOG_INFO("Received cmd:%x",ctrl_cmd->command);
        if (ctrl_cmd->command == (gsConfigCtrl->command + 1)) {
          gsSendNextConfigCmd = ValidateStatus(pkt->src, gsConfigCtrl->command,
              ctrl_cmd->status); // Check the status of the response pkt
              /* TODO:  This block is commented out, as as ValidateStatus()
                was not handling command REQ cases which were having same values
                for eg: M2M2_FILE_SYS_CMD_START_LOGGING_REQ =
                M2M2_SENSOR_ADPD_COMMAND_AGC_ON_OFF_REQ = 118.
                We would need to do better handling here and enable check of the
                return value of ValidateStatus() api.*/
              if(!gsSendNextConfigCmd)
              {
                  gsCmdRetryCnt++;            //Increment the Retry count
                  gsSendNextConfigCmd = 1;    // previous command will be sent again; as Config file read ptr is not updated

                  if(gsCmdRetryCnt >= MAX_CMD_RETRY_CNT)      //If retry count reaches its max limit goto the next command in the config file
                  {
                      gsConfigFileReadPtr += gsPktLenCopy;    // Update the Config file read ptr to next command
                      gsCmdRetryCnt = 0;                      //TODO: Intimate  missing cmd to the user
                      if(gsStartCmdsRunning)
                      {
                        if(gbDCBCfgFoundFlag)
                          SendLowTouchErrResp(M2M2_PM_SYS_STATUS_ENABLE_DCB_CONFIG_LOG_FAILED);
                        else
                          SendLowTouchErrResp(M2M2_PM_SYS_STATUS_ENABLE_USER_CONFIG_LOG_FAILED);
                      }
                      else
                      {
                        if(gbDCBCfgFoundFlag)
                          SendLowTouchErrResp(M2M2_PM_SYS_STATUS_DISABLE_DCB_CONFIG_LOG_FAILED);
                        else
                          SendLowTouchErrResp(M2M2_PM_SYS_STATUS_DISABLE_USER_CONFIG_LOG_FAILED);
                      }
                      LowTouchErr();
                      post_office_consume_msg(pkt);
                      break;      //skip sending the further command
                  }
              }
              else
              {
          gsCmdRetryCnt = 0;
          gsNumOfCommands++; // Command is properly being sent and ackd
          gsConfigFileReadPtr +=
              gsPktLenCopy; // Update the Config file read ptr to next command
          }
          if ((gsStartCmdsRunning) &&
              (gsCfgFileSummaryPkt->start_cmd_cnt == gsNumOfCommands)) {
            gsStartCmdsRunning = 0;
            gLowTouchRunning = 1; // Low touch logging has started
#ifdef DCB
            if (!gbDCBCfgFoundFlag) {
#endif
              SendLowTouchErrResp(M2M2_PM_SYS_STATUS_USER_CONFIG_LOG_ENABLED);
#ifdef DCB
            } else
              SendLowTouchErrResp(M2M2_PM_SYS_STATUS_DCB_CONFIG_LOG_ENABLED);
#endif
              if(!get_low_touch_trigger_mode2_status() && !get_low_touch_trigger_mode3_status())
              {
                //Check if OFFWrist event had come in b/w start cmd seq execution
                if(eCurDetection_State == OFF_WRIST)
                {
                  InitConfigParam(0);
                  SendUserConfigCommands();
                  gLowTouchRunning = 0;
                }
              }
#ifdef USER0_CONFIG_APP
              else if(get_low_touch_trigger_mode3_status())
              {
                //Send app timer start cmd to start LT logging
                start_eda_app_timer();
                start_adxl_app_timer();
                start_temp_app_timer();
                start_adpd_app_timer();
              }
#endif
          } else if ((gsStopCmdsRunning) &&
                     (gsCfgFileSummaryPkt->stop_cmd_cnt == gsNumOfCommands)) {
            gsStopCmdsRunning = 0;
            SetCfgCopyAvailableFlag(
                true); /*update the config file copy availability flag*/
            gLowTouchRunning = 0; // Low Touch loggging has stopped
#ifdef CUST4_SM
            //Low touch logging stopped
            USER0_CONFIG_APP_STATE_t read_user0_config_app_state = get_user0_config_app_state();
            if(read_user0_config_app_state ==
               STATE_INTERMITTENT_MONITORING_STOP_LOG)
            {
              set_user0_config_app_state(STATE_INTERMITTENT_MONITORING);
            }
            if(read_user0_config_app_state ==
               STATE_INTERMITTENT_MONITORING_STOP_LOG ||
               read_user0_config_app_state ==
               STATE_OUT_OF_BATTERY_STATE_DURING_INTERMITTENT_MONITORING)
            {
              //Turn on BLE
              turn_on_BLE();
            }
#endif
#ifdef DCB
            if (!gbDCBCfgFoundFlag) {
#endif
              SendLowTouchErrResp(M2M2_PM_SYS_STATUS_USER_CONFIG_LOG_DISABLED);
#ifdef DCB
            } else
              SendLowTouchErrResp(M2M2_PM_SYS_STATUS_DCB_CONFIG_LOG_DISABLED);
#endif
          } else {
            SendUserConfigCommands();
          }
        }
#endif // LOW_TOUCH_FEATURE
        post_office_consume_msg(pkt);
        break;
      } // end of default
      } // end of switch
    }   /*if(pkt != NULL)*/
    else {
      StreamBatt_Info();
    } // end of else, for if(pkt != NULL)*/
  }
}

/**
 * @brief  StreamBatt_Info - Gets Battery info from ADP5360 and streams to
 * subscribers
 * @param  pArgument not used
 * @return None
 */
void StreamBatt_Info(void) {
  m2m2_hdr_t *pkt = NULL;
  ADI_OSAL_STATUS err;
  BATTERY_STATUS_t bat_status;
  M2M2_PM_SYS_BAT_STATE_ENUM_t enBatstatus = M2M2_PM_SYS_BAT_STATE_CHARGE_ERR;
  PWR_CTRL_STATUS_t pwr_err;
  ADP5360_RESULT ret;
  static uint8_t ble_bas_timer_cnt = 0;
  // uint16_t nGpioVal;

  // adi_gpio_GetData(ADI_GPIO_PORT1, ADI_GPIO_PIN_1, &nGpioVal);
  /* Read Battery Info */
  ret = Adp5360_get_battery_details(&bat_status);
  pwr_err = (ret != ADP5360_SUCCESS) ? PWR_CTRL_ERROR : PWR_CTRL_SUCCESS;
  if (pwr_err != PWR_CTRL_ERROR /* && nGpioVal != 0*/) {
    /* Set the battery voltage, temperature and status */
    switch (bat_status.chrg_status) {
    case BATTERY_NOT_AVAILABLE: {
      enBatstatus = M2M2_PM_SYS_BAT_STATE_NOT_AVAIL;
      break;
    }
    case BATTERY_CHARGING: {
      enBatstatus = M2M2_PM_SYS_BAT_STATE_CHARGING;
      if (bat_status.level >= BATTERY_FULL_CHARGE_LEVEL) {
        if(battery_level_alerts.charge_alert_status != E_FULL_LEVEL)
        {
          battery_level_alerts.charge_alert_status = E_FULL_LEVEL;
          SendBatteryLevelAlertMsg(M2M2_PM_SYS_STATUS_BATTERY_LEVEL_FULL);
        }
      }
      break;
    }
    case BATTERY_CHARGER_LDO_MODE:
    case BATTERY_CHARGER_TIMER_EXPIRED: {
      enBatstatus = M2M2_PM_SYS_BAT_STATE_CHARGING;
      break;
    }
    case BATTERY_CHARGE_COMPLETE: {
      enBatstatus = M2M2_PM_SYS_BAT_STATE_COMPLETE;
      break;
    }
    case BATTERY_NOT_CHARGING: {
      enBatstatus = M2M2_PM_SYS_BAT_STATE_NOT_CHARGING;
#ifdef USE_FS
      if (bat_status.level <= ADI_PM_BAT_LEVEL_LOG_CRITICAL) {
        battery_level_alerts.critical_level_flag = 1; /*Critical Battery level for logging*/
        /* If File logging is in progress */
        if (fs_hal_write_access_state() == FS_FILE_ACCESS_IN_PROGRESS) {
          /* Force Stop Logging */
          pkt = post_office_create_msg(
              M2M2_HEADER_SZ + sizeof(_m2m2_app_common_cmd_t));
          if (pkt != NULL) {
            _m2m2_app_common_cmd_t *payload =
                (_m2m2_app_common_cmd_t *)&pkt->data[0];
            payload->command = (M2M2_APP_COMMON_CMD_ENUM_t)
                M2M2_FILE_SYS_CMD_FORCE_STOP_LOG_REQ;
            pkt->src = M2M2_ADDR_SYS_PM;
            pkt->dest = M2M2_ADDR_SYS_FS;
            post_office_send(pkt, &err);
          }
        }
      }
#endif
      if (bat_status.level <= battery_level_alerts.level_critical) {
        // TODO: Insert display code handling
        if(battery_level_alerts.charge_alert_status != E_CRITICAL_LEVEL)
        {
          battery_level_alerts.charge_alert_status = E_CRITICAL_LEVEL;
          SendBatteryLevelAlertMsg(M2M2_PM_SYS_STATUS_BATTERY_LEVEL_CRITICAL);
        }

      } else if (bat_status.level <= battery_level_alerts.level_low) {
        // TODO: Insert display code handling
       if(battery_level_alerts.charge_alert_status != E_LOW_LEVEL)
        {
          battery_level_alerts.charge_alert_status = E_LOW_LEVEL;
          SendBatteryLevelAlertMsg(M2M2_PM_SYS_STATUS_BATTERY_LEVEL_LOW);
        }
      } else if (bat_status.level < BATTERY_FULL_CHARGE_LEVEL) {
        if(battery_level_alerts.charge_alert_status != E_NORMAL_LEVEL)
        {
          battery_level_alerts.charge_alert_status = E_NORMAL_LEVEL;
        }
      } else if (bat_status.level >= BATTERY_FULL_CHARGE_LEVEL) {
        // TODO: Insert display code handling
        if(battery_level_alerts.charge_alert_status != E_FULL_LEVEL)
        {
          battery_level_alerts.charge_alert_status = E_FULL_LEVEL;
          SendBatteryLevelAlertMsg(M2M2_PM_SYS_STATUS_BATTERY_LEVEL_FULL);
        }
      } else {
        // No need to handle
      }
      break;
    }
    case BATTERY_DETECTION: {
      enBatstatus = M2M2_PM_SYS_BAT_STATE_DETECTION;
      break;
    }
    case BATTERY_UNKNOWN:
    default: {
      enBatstatus = M2M2_PM_SYS_BAT_STATE_CHARGE_ERR;
      // TODO: Insert display code handling
      break;
    }
    }
  }
  /* Send Battery info as Stream to subscribers */
  if (BattInfoSubsciberCount > 0) {
    pkt = post_office_create_msg(
        M2M2_HEADER_SZ + sizeof(m2m2_pm_sys_bat_info_resp_t));
    if (pkt != NULL) {
      m2m2_pm_sys_bat_info_resp_t *payload =
          (m2m2_pm_sys_bat_info_resp_t *)&pkt->data[0];
      payload->command =
          (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_SENSOR_COMMON_CMD_STREAM_DATA;

      /* Set the status for stream packet */
      if (pwr_err) {
        payload->status = M2M2_APP_COMMON_STATUS_ERROR;
      } else {
        payload->status = M2M2_APP_COMMON_STATUS_OK;
      }
      payload->bat_chrg_stat = enBatstatus;
      payload->bat_mv = bat_status.voltage_mv;
      payload->bat_lvl = bat_status.level;
      time_t return_time;
      int16_t timezone_offset;
      get_log_time_stamp(&return_time, &timezone_offset);
      payload->timestamp = return_time + timezone_offset;
      pkt->checksum = BattInfoChecksum++;
      pkt->src = M2M2_ADDR_SYS_PM;
      pkt->dest = M2M2_ADDR_SYS_BATT_STREAM;
      post_office_send(pkt, &err);
    }
  }
  if( get_ble_nus_status() == BLE_CONNECTED )
  {
    if(++ble_bas_timer_cnt == 2)
    {
      ble_bas_timer_cnt = 0;

      battery_level_update();
    }
  }
  else
    ble_bas_timer_cnt = 0;
}
/////////////////////////////////////////////////////////////////////////
#define PING_DATA_TX_INTERVAL                                                  \
  20 /**< Ping Data transmitted interval 20 (ms).  \ \                                                                             \
      */

APP_TIMER_DEF(m_ping_timer_id); /**< Handler for repeated timer for ping. */

/*!
 ****************************************************************************
 *@brief Function for handling the PING DATA TX timer time-out.
 *
 * @details This function will be called each time the ping data transmit timer
 * expires.
 ******************************************************************************/
static void ping_data_tx_timeout_handler(void *p_context) {
  m2m2_hdr_t *response_mail = NULL;
  ADI_OSAL_STATUS err;
  static uint16_t seq_no = 0;

  NRF_LOG_INFO("Ping Timer expiry ping_count:%d, pkt_sz:%d", seq_no, ping_pkt_sz);
  // response_mail = post_office_create_msg(M2M2_HEADER_SZ +
  // sizeof(m2m2_app_common_ping_t));
  response_mail = post_office_create_msg(ping_pkt_sz);
  if (response_mail != NULL) {
    m2m2_app_common_ping_t *ctrl =
        (m2m2_app_common_ping_t *)&response_mail->data[0];
    response_mail->dest = ping_pkt_src;
    response_mail->src = M2M2_ADDR_SYS_PM;
    response_mail->length = ping_pkt_sz;
    ctrl->command = M2M2_APP_COMMON_CMD_PING_RESP;
    ctrl->status = M2M2_OK;
    ctrl->sequence_num = ++seq_no;
    post_office_send(response_mail, &err);
    if (--ping_count == 0) {
      seq_no = 0;
      ping_timer_stop();
    }
  }
}

/*!
 ****************************************************************************
 *@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module. This creates and starts application
 * timers.
 ******************************************************************************/
static void ping_timer_init(void) {
  ret_code_t err_code;

  // Create timers
  err_code = app_timer_create(&m_ping_timer_id, APP_TIMER_MODE_REPEATED, ping_data_tx_timeout_handler);

  APP_ERROR_CHECK(err_code);
}

/*!
 ****************************************************************************
 *@brief   Function for starting application timers.
 * @details Timers are run after the scheduler has started.
 * @param   None
 * @retval   None
 ******************************************************************************/
static void ping_timer_start(void) {
  /* Start repeated timer */
  ret_code_t err_code = app_timer_start(m_ping_timer_id, APP_TIMER_TICKS(PING_DATA_TX_INTERVAL), NULL);
  APP_ERROR_CHECK(err_code);
}

/*!
 ****************************************************************************
 *@brief   Function for stopping application timers.
 * @param   None
 * @retval   None
 ******************************************************************************/
static void ping_timer_stop(void) {
  /* Stop the repeated timer */
  ret_code_t err_code = app_timer_stop(m_ping_timer_id);
  APP_ERROR_CHECK(err_code);
}

/************************** DCB for LT_APP_LCFG_BLOCK **************************/

#ifdef DCB
/**
 * @brief  Function which handles the m2m2 command to do lt_app_lcfg_dcb read
 * @param  p_pkt m2m2 REQ packet
 * @return m2m2 RESP packet
 */
static m2m2_hdr_t *lt_app_lcfg_dcb_command_read_config(m2m2_hdr_t *p_pkt)
{
    static uint16_t r_size = 0;
    uint32_t dcbdata[MAXLTAPPLCFGDCBSIZE];
    M2M2_DCB_STATUS_ENUM_t status = M2M2_DCB_STATUS_ERR_NOT_CHKD;

    PKT_MALLOC(p_resp_pkt, m2m2_dcb_lt_app_lcfg_data_t, 0);
    // Declare a pointer to the response packet payload
    PYLD_CST(p_resp_pkt, m2m2_dcb_lt_app_lcfg_data_t, p_resp_payload);

    r_size = (uint16_t)MAXLTAPPLCFGDCBSIZE;
    if(read_lt_app_lcfg_dcb(&dcbdata[0],&r_size) == LT_APP_LCFG_DCB_STATUS_OK)
    {
        for(int i=0; i< r_size; i++)
          p_resp_payload->dcbdata[i] = dcbdata[i];
        p_resp_payload->size = (r_size);
        status = M2M2_DCB_STATUS_OK;
    }
    else
    {
        p_resp_payload->size = 0;
        status = M2M2_DCB_STATUS_ERR_ARGS;
    }

    p_resp_payload->status = status;
    p_resp_payload->command = M2M2_DCB_COMMAND_READ_CONFIG_RESP;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
    return p_resp_pkt;
}

/**
 * @brief  Function which handles the m2m2 command to do lt_app_lcfg_dcb write
 * @param  p_pkt m2m2 REQ packet
 * @return m2m2 RESP packet
 */
static m2m2_hdr_t *lt_app_lcfg_dcb_command_write_config(m2m2_hdr_t *p_pkt)
{
    M2M2_DCB_STATUS_ENUM_t status = M2M2_DCB_STATUS_ERR_NOT_CHKD;
    uint32_t dcbdata[MAXLTAPPLCFGDCBSIZE];

    // Declare a pointer to access the input packet payload
    PYLD_CST(p_pkt, m2m2_dcb_lt_app_lcfg_data_t, p_in_payload);
    PKT_MALLOC(p_resp_pkt, m2m2_dcb_lt_app_lcfg_data_t, 0);
    // Declare a pointer to the response packet payload
    PYLD_CST(p_resp_pkt, m2m2_dcb_lt_app_lcfg_data_t, p_resp_payload);

    for(int i=0; i<p_in_payload->size; i++)
      dcbdata[i] = p_in_payload->dcbdata[i];
    if(write_lt_app_lcfg_dcb(&dcbdata[0],p_in_payload->size) == LT_APP_LCFG_DCB_STATUS_OK)
    {
        lt_app_lcfg_set_dcb_present_flag(true);
        lt_app_lcfg_set_from_dcb();
        status = M2M2_DCB_STATUS_OK;
    }
    else
    {
        status = M2M2_DCB_STATUS_ERR_ARGS;
    }

    p_resp_payload->status = status;
    p_resp_payload->command = M2M2_DCB_COMMAND_WRITE_CONFIG_RESP;
    p_resp_payload->size = 0;
    for(uint16_t i=0; i< MAXLTAPPLCFGDCBSIZE; i++)
        p_resp_payload->dcbdata[i] = 0;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
    return p_resp_pkt;
}

/**
 * @brief  Function which handles the m2m2 command to do lt_app_lcfg_dcb delete
 * @param  p_pkt m2m2 REQ packet
 * @return m2m2 RESP packet
 */
static m2m2_hdr_t *lt_app_lcfg_dcb_command_delete_config(m2m2_hdr_t *p_pkt)
{
    M2M2_DCB_STATUS_ENUM_t status = M2M2_DCB_STATUS_ERR_NOT_CHKD;

    PKT_MALLOC(p_resp_pkt, m2m2_dcb_lt_app_lcfg_data_t, 0);
    // Declare a pointer to the response packet payload
    PYLD_CST(p_resp_pkt, m2m2_dcb_lt_app_lcfg_data_t, p_resp_payload);

    if(delete_lt_app_lcfg_dcb() == LT_APP_LCFG_DCB_STATUS_OK)
    {
        lt_app_lcfg_set_dcb_present_flag(false);
        lt_app_lcfg_set_fw_default();
        status = M2M2_DCB_STATUS_OK;
    }
    else
    {
        status = M2M2_DCB_STATUS_ERR_ARGS;
    }

    p_resp_payload->status = status;
    p_resp_payload->command = M2M2_DCB_COMMAND_ERASE_CONFIG_RESP;
    p_resp_payload->size = 0;
    for(uint16_t i=0; i< MAXLTAPPLCFGDCBSIZE; i++)
        p_resp_payload->dcbdata[i] = 0;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
    return p_resp_pkt;

}
#endif // DCB
/*@}*/  /* end of group system_task */
/**@}*/ /* end of group Tasks */