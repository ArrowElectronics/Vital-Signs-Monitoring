/**
***************************************************************************
* @addtogroup Tasks
* @{
* @file         <user0_config_app_task.c>
* @author       ADI
* @version      V1.0.0
* @date         13-July-2021
* @brief        Source file contains functions user0_config_app application
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
* Copyright (c) 2021 Analog Devices Inc.
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
#ifdef USER0_CONFIG_APP
/*---------------------------- Includes --------------------------------------*/
#include "user0_config_app_task.h"
#include "display_app.h"
#include "low_touch_task.h"
#include "system_task.h"
#include "ble_task.h"
#include "app_timer.h"
#include "power_manager.h"
#include <common_application_interface.h>
#include <user0_config_application_interface.h>
#include <post_office.h>
#ifdef DCB
#include "adi_dcb_config.h"
#include "dcb_user0_block.h"
#include "dcb_general_block.h"
#include "lcd_driver.h"
#include <dcb_interface.h>
#endif
/* Low touch App Module Log settings */
#define NRF_LOG_MODULE_NAME user0_config_app

#if USER0_CONFIG_LOG_ENABLED
#ifdef NRF_LOG_DEFAULT_LEVEL
#undef NRF_LOG_DEFAULT_LEVEL
#define NRF_LOG_DEFAULT_LEVEL USER0_CONFIG_LOG_LEVEL
#endif
#define NRF_LOG_INFO_COLOR USER0_CONFIG_INFO_COLOR
#define NRF_LOG_DEBUG_COLOR USER0_CONFIG_DEBUG_COLOR
#else // USER0_CONFIG_LOG_ENABLED
#define NRF_LOG_LEVEL 0
#endif // USER0_CONFIG_LOG_ENABLED

#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

/* For user0_config_app FreeRTOS Task Creation */
ADI_OSAL_STATIC_THREAD_ATTR g_user0_config_app_attributes;
uint8_t ga_user0_config_app_stack[APP_OS_CFG_USER0_CONFIG_APP_TASK_STK_SIZE];
StaticTask_t g_user0_config_app_tcb;
ADI_OSAL_THREAD_HANDLE gh_user0_config_app_handler;
ADI_OSAL_QUEUE_HANDLE gh_user0_config_app_msg_queue = NULL;
/* semaphore handler for user0_config_app task */
ADI_OSAL_SEM_HANDLE   user0_config_app_evt_sem;

/* Handler for repeated timer for USER0 config ON. */
APP_TIMER_DEF(m_user0_config_app_on_timer_id);

/* Flag which gets set, when either ON/OFF timer expires. Its based on this flag
 * and the event detected, that the user0 config app state changes */
volatile uint8_t gUser0ConfigAppTimerUp = 0;


/* Variable which holds the status of user0 config init/deinitialisation */
static uint8_t gnUser0ConfigInitFlag = 0;

#ifdef CUST4_SM
#include "usbd_task.h"
//Changes to make sure that the variable is not cleared to zero after Watch reset
volatile USER0_CONFIG_APP_STATE_t gnUser0ConfigAppState __attribute__((section(".non_init")));

//Debug variable to count no: of times intermittent logging is done
volatile uint16_t gn_intermittent_op_count __attribute__((section(".non_init")));
#endif

static uint16_t user_applied_agc_up_th;
static uint16_t user_applied_agc_low_th;
static uint16_t user_applied_adv_timeout_monitor;
static uint16_t user_applied_hw_id;
static uint16_t user_applied_exp_id;
static uint16_t user_applied_adxl_start_time;
static uint16_t user_applied_adxl_tON;
static uint16_t user_applied_adxl_tOFF;
static uint16_t user_applied_temp_start_time;
static uint16_t user_applied_temp_tON;
static uint16_t user_applied_temp_tOFF;
static uint16_t user_applied_adpd_start_time;
static uint16_t user_applied_adpd_tON;
static uint16_t user_applied_adpd_tOFF;
static uint16_t user_applied_eda_start_time;
static uint16_t user_applied_eda_tON;
static uint16_t user_applied_eda_tOFF;
static uint16_t user_applied_sleep_min;
static uint16_t user_applied_signal_threshold;
static uint8_t battery_low_event_cnt = 0;
static uint8_t battery_critical_event_cnt = 0;
static uint8_t battery_full_event_cnt = 0;

user0_config_app_lcfg_type_t user0_config_app_lcfg;
/*-------------------- Private Function Declarations ------------------------*/
#ifdef CUST4_SM
static void on_timer_init();
static void user0_config_app_on_timer_start(void);
static void user0_config_app_on_timer_stop(void);
static m2m2_hdr_t *user0_battery_level_alert_handler(m2m2_hdr_t *p_pkt);
#endif

static m2m2_hdr_t *user0_config_app_lcfg_access(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *user0_config_app_get_set_state(m2m2_hdr_t *p_pkt);
#ifdef DCB
static m2m2_hdr_t *user0_blk_dcb_command_read_config(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *user0_blk_dcb_command_write_config(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *user0_blk_dcb_command_delete_config(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *id_operation(m2m2_hdr_t *p_pkt);
#endif

static void user0_config_app(void *arg);
static void init_user0_config_app_lcfg();
/*--------------------- Public Funtion Declarations -------------------------*/

#define USER0_CONFIG_APP_ROUTING_TBL_SZ                                                  \
  (sizeof(user0_config_app_routing_table) / sizeof(user0_config_app_routing_table[0]))

typedef m2m2_hdr_t *(app_cb_function_t)(m2m2_hdr_t *);

typedef struct _app_routing_table_entry_t {
  uint8_t command;
  app_cb_function_t *cb_handler;
} app_routing_table_entry_t;

/* Routing table structure, to handle m2m2 command REQ from user0_config task function */
app_routing_table_entry_t user0_config_app_routing_table[] = {
    {M2M2_APP_COMMON_CMD_READ_LCFG_REQ,  user0_config_app_lcfg_access},
    {M2M2_APP_COMMON_CMD_WRITE_LCFG_REQ, user0_config_app_lcfg_access},
#ifdef CUST4_SM
    {M2M2_USER0_CONFIG_APP_COMMAND_SET_STATE_REQ, user0_config_app_get_set_state},
    {M2M2_USER0_CONFIG_APP_COMMAND_GET_STATE_REQ, user0_config_app_get_set_state},
    {M2M2_PM_SYS_BATTERY_LEVEL_ALERT, user0_battery_level_alert_handler},
#endif
#ifdef DCB
    {M2M2_DCB_COMMAND_READ_CONFIG_REQ, user0_blk_dcb_command_read_config},
    {M2M2_DCB_COMMAND_WRITE_CONFIG_REQ, user0_blk_dcb_command_write_config},
    {M2M2_DCB_COMMAND_ERASE_CONFIG_REQ, user0_blk_dcb_command_delete_config},
    {M2M2_USER0_CONFIG_APP_COMMAND_ID_OP_REQ, id_operation},
#endif
};
void user0_config_app_lcfg_set_from_dcb();
void user0_config_app_lcfg_set_fw_default();

/* Variable which holds the status of NAND config file */
extern volatile uint8_t gsCfgFileFoundFlag;
extern ADI_OSAL_SEM_HANDLE   lt_task_evt_sem;

#ifdef CUST4_SM
extern void reset_display_vol_info1();
#endif
/*!
 ****************************************************************************
 * @brief    To do init of user0 config app based on DCB/default fw lcfg
 * @param    None
 * @retval   None
*****************************************************************************/
static void init_user0_config_app_lcfg() {
  /* Check & Load from DCB USER0 BLK lcfg if lcfg is available in user0_blk_app_lcfg_dcb */
  if (user0_blk_get_dcb_present_flag()) {
      user0_config_app_lcfg_set_from_dcb();
  }

  /* Loading default or modified fw user0 config app lcfg */
  else
  {
    if( !user_applied_agc_up_th )
      user0_config_app_lcfg.agc_up_th  = USER0_CFG_AGC_UP_TH;
    else
      user_applied_agc_up_th = 0;

    if( !user_applied_agc_low_th )
      user0_config_app_lcfg.agc_low_th = USER0_CFG_AGC_LOW_TH;
    else
      user_applied_agc_low_th = 0;

    if( !user_applied_adv_timeout_monitor )
      user0_config_app_lcfg.adv_timeout_monitor  = USER0_CFG_ADV_TIMEOUT_MONITOR;
    else
      user_applied_adv_timeout_monitor = 0;

    if( !user_applied_hw_id )
    {
      user0_config_app_lcfg.hw_id = USER0_CFG_HW_ID;
      //Update hw_id in the System Info structure
      update_hw_id_in_system_info(user0_config_app_lcfg.hw_id);
    }
    else
      user_applied_hw_id = 0;

    if( !user_applied_exp_id )
      user0_config_app_lcfg.exp_id  = USER0_CFG_EXP_ID;
    else
      user_applied_exp_id = 0;

    if( !user_applied_adxl_start_time )
      user0_config_app_lcfg.adxl_start_time  = USER0_CFG_ADXL_START_TIME;
    else
      user_applied_adxl_start_time = 0;
    if( !user_applied_adxl_tON )
      user0_config_app_lcfg.adxl_tON  = USER0_CFG_ADXL_TON;
    else
      user_applied_adxl_tON = 0;
    if( !user_applied_adxl_tOFF )
      user0_config_app_lcfg.adxl_tOFF  = USER0_CFG_ADXL_TOFF;
    else
      user_applied_adxl_tOFF = 0;

    if( !user_applied_temp_start_time )
      user0_config_app_lcfg.temp_start_time  = USER0_CFG_TEMP_START_TIME;
    else
      user_applied_temp_start_time = 0;
    if( !user_applied_temp_tON )
      user0_config_app_lcfg.temp_tON  = USER0_CFG_TEMP_TON;
    else
      user_applied_temp_tON = 0;
    if( !user_applied_temp_tOFF )
      user0_config_app_lcfg.temp_tOFF  = USER0_CFG_TEMP_TOFF;
    else
      user_applied_temp_tOFF = 0;

    if( !user_applied_adpd_start_time )
      user0_config_app_lcfg.adpd_start_time  = USER0_CFG_ADPD_START_TIME;
    else
      user_applied_adpd_start_time = 0;
    if( !user_applied_adpd_tON )
      user0_config_app_lcfg.adpd_tON  = USER0_CFG_ADPD_TON;
    else
      user_applied_adpd_tON = 0;
    if( !user_applied_adpd_tOFF )
      user0_config_app_lcfg.adpd_tOFF  = USER0_CFG_ADPD_TOFF;
    else
      user_applied_adpd_tOFF = 0;

    if( !user_applied_eda_start_time )
      user0_config_app_lcfg.eda_start_time  = USER0_CFG_EDA_START_TIME;
    else
      user_applied_eda_start_time = 0;
    if( !user_applied_eda_tON )
      user0_config_app_lcfg.eda_tON  = USER0_CFG_EDA_TON;
    else
      user_applied_eda_tON = 0;
    if( !user_applied_eda_tOFF )
      user0_config_app_lcfg.eda_tOFF  = USER0_CFG_EDA_TOFF;
    else
      user_applied_eda_tOFF = 0;

    if( !user_applied_sleep_min )
      user0_config_app_lcfg.sleep_min  = USER0_CFG_SLEEP_MIN;
    else
      user_applied_sleep_min = 0;

    if( !user_applied_signal_threshold )
      user0_config_app_lcfg.signal_threshold  = USER0_CFG_SIGNAL_THRESHOLD;
    else
      user_applied_signal_threshold = 0;
  }

}

/*!
 ****************************************************************************
 * @brief    To write an LCFG parameter
 * @param    field: LCFG field that has to be written
 * @param    value: Value to be written
 * @retval   USER0_CONFIG_ERROR_CODE_t
 *****************************************************************************/
USER0_CONFIG_ERROR_CODE_t user0_config_app_write_lcfg(uint8_t field, uint16_t value) {
  if (field < USER0_CONFIG_LCFG_MAX) {
    switch (field) {
    case USER0_CONFIG_LCFG_AGC_UP_TH:
      user0_config_app_lcfg.agc_up_th = value;
      user_applied_agc_up_th = 1;
      break;
    case USER0_CONFIG_LCFG_AGC_LOW_TH:
      user0_config_app_lcfg.agc_low_th = value;
      user_applied_agc_low_th = 1;
      break;
    case USER0_CONFIG_LCFG_ADV_TIMEOUT_MONITOR:
      user0_config_app_lcfg.adv_timeout_monitor = value;
      user_applied_adv_timeout_monitor = 1;
      break;
    case USER0_CONFIG_LCFG_HW_ID:
      user0_config_app_lcfg.hw_id = value;
      user_applied_hw_id = 1;
      //Update hw_id in the System Info structure
      update_hw_id_in_system_info(user0_config_app_lcfg.hw_id);
#ifdef CUST4_SM
      //Update the id shown in page_watch_id of Display
      send_private_type_value(DIS_REFRESH_SIGNAL);
      reset_display_vol_info1();
#endif
      break;
    case USER0_CONFIG_LCFG_EXP_ID:
      user0_config_app_lcfg.exp_id = value;
      user_applied_exp_id = 1;
#ifdef CUST4_SM
      //Update the id shown in page_watch_id of Display
      send_private_type_value(DIS_REFRESH_SIGNAL);
      reset_display_vol_info1();
#endif
      break;
    case USER0_CONFIG_LCFG_ADXL_START_TIME:
      user0_config_app_lcfg.adxl_start_time = value;
      user_applied_adxl_start_time = 1;
      break;
    case USER0_CONFIG_LCFG_ADXL_TON:
      user0_config_app_lcfg.adxl_tON = value;
      user_applied_adxl_tON = 1;
    break;
    case USER0_CONFIG_LCFG_ADXL_TOFF:
      user0_config_app_lcfg.adxl_tOFF = value;
      user_applied_adxl_tOFF = 1;
    break;
    case USER0_CONFIG_LCFG_TEMP_START_TIME:
      user0_config_app_lcfg.temp_start_time = value;
      user_applied_temp_start_time = 1;
      break;
    case USER0_CONFIG_LCFG_TEMP_TON:
      user0_config_app_lcfg.temp_tON = value;
      user_applied_temp_tON = 1;
    break;
    case USER0_CONFIG_LCFG_TEMP_TOFF:
      user0_config_app_lcfg.temp_tOFF = value;
      user_applied_temp_tOFF = 1;
    break;
    case USER0_CONFIG_LCFG_ADPD_START_TIME:
      user0_config_app_lcfg.adpd_start_time = value;
      user_applied_adpd_start_time = 1;
      break;
    case USER0_CONFIG_LCFG_ADPD_TON:
      user0_config_app_lcfg.adpd_tON = value;
      user_applied_adpd_tON = 1;
    break;
    case USER0_CONFIG_LCFG_ADPD_TOFF:
      user0_config_app_lcfg.adpd_tOFF = value;
      user_applied_adpd_tOFF = 1;
    break;
    case USER0_CONFIG_LCFG_EDA_START_TIME:
      user0_config_app_lcfg.eda_start_time = value;
      user_applied_eda_start_time = 1;
      break;
    case USER0_CONFIG_LCFG_EDA_TON:
      user0_config_app_lcfg.eda_tON = value;
      user_applied_eda_tON = 1;
    break;
    case USER0_CONFIG_LCFG_EDA_TOFF:
      user0_config_app_lcfg.eda_tOFF = value;
      user_applied_eda_tOFF = 1;
    break;
    case USER0_CONFIG_LCFG_SLEEP_MIN:
      user0_config_app_lcfg.sleep_min = value;
      user_applied_sleep_min = 1;
      break;
    case USER0_CONFIG_LCFG_SIGNAL_THRESHOLD:
      user0_config_app_lcfg.signal_threshold = value;
      user_applied_signal_threshold = 1;
      break;
    }
    return USER0_CONFIG_SUCCESS;
  }
  return USER0_CONFIG_ERROR;
}

/*!
 ****************************************************************************
 * @brief    Read LCFG parameter
 * @param    index: LCFG field to be read
 * @param    value: Returned value corresponding LCFG value
 * @retval   USER0_CONFIG_ERROR_CODE_t
 *****************************************************************************/
USER0_CONFIG_ERROR_CODE_t user0_config_app_read_lcfg(uint8_t index, uint16_t *value) {
  if (index < USER0_CONFIG_LCFG_MAX) {
    switch (index) {
    case USER0_CONFIG_LCFG_AGC_UP_TH:
      *value = user0_config_app_lcfg.agc_up_th;
      break;
    case USER0_CONFIG_LCFG_AGC_LOW_TH:
      *value = user0_config_app_lcfg.agc_low_th;
      break;
    case USER0_CONFIG_LCFG_ADV_TIMEOUT_MONITOR:
      *value = user0_config_app_lcfg.adv_timeout_monitor;
      break;
    case USER0_CONFIG_LCFG_HW_ID:
      *value = user0_config_app_lcfg.hw_id;
      break;
    case USER0_CONFIG_LCFG_EXP_ID:
      *value = user0_config_app_lcfg.exp_id;
      break;
    case USER0_CONFIG_LCFG_ADXL_START_TIME:
      *value = user0_config_app_lcfg.adxl_start_time;
      break;
    case USER0_CONFIG_LCFG_ADXL_TON:
      *value = user0_config_app_lcfg.adxl_tON;
      break;
    case USER0_CONFIG_LCFG_ADXL_TOFF:
      *value = user0_config_app_lcfg.adxl_tOFF;
      break;
    case USER0_CONFIG_LCFG_TEMP_START_TIME:
      *value = user0_config_app_lcfg.temp_start_time;
      break;
    case USER0_CONFIG_LCFG_TEMP_TON:
      *value = user0_config_app_lcfg.temp_tON;
      break;
    case USER0_CONFIG_LCFG_TEMP_TOFF:
      *value = user0_config_app_lcfg.temp_tOFF;
      break;
    case USER0_CONFIG_LCFG_ADPD_START_TIME:
      *value = user0_config_app_lcfg.adpd_start_time;
      break;
    case USER0_CONFIG_LCFG_ADPD_TON:
      *value = user0_config_app_lcfg.adpd_tON;
      break;
    case USER0_CONFIG_LCFG_ADPD_TOFF:
      *value = user0_config_app_lcfg.adpd_tOFF;
      break;
      case USER0_CONFIG_LCFG_EDA_START_TIME:
      *value = user0_config_app_lcfg.eda_start_time;
      break;
    case USER0_CONFIG_LCFG_EDA_TON:
      *value = user0_config_app_lcfg.eda_tON;
      break;
    case USER0_CONFIG_LCFG_EDA_TOFF:
      *value = user0_config_app_lcfg.eda_tOFF;
      break;
    case USER0_CONFIG_LCFG_SLEEP_MIN:
      *value = user0_config_app_lcfg.sleep_min;
      break;
    case USER0_CONFIG_LCFG_SIGNAL_THRESHOLD:
      *value = user0_config_app_lcfg.signal_threshold;
      break;
    }
    return USER0_CONFIG_SUCCESS;
  }
  return USER0_CONFIG_ERROR;
}

/*!
 ****************************************************************************
 *@brief      User0 Config app lcfg Read/write configuration options
 *@param      pPkt: pointer to the packet structure
 *@return     m2m2_hdr_t * type pointer
 *****************************************************************************/
static m2m2_hdr_t *user0_config_app_lcfg_access(m2m2_hdr_t *p_pkt) {
  M2M2_APP_COMMON_STATUS_ENUM_t status = M2M2_APP_COMMON_STATUS_ERROR;
  PYLD_CST(p_pkt, user0_config_app_lcfg_op_hdr_t, p_in_payload);
  /* Allocate a response packet with space for the correct number of operations
   */
  PKT_MALLOC(p_resp_pkt, user0_config_app_lcfg_op_hdr_t,
      p_in_payload->num_ops * sizeof(p_in_payload->ops[0]));
  if (NULL != p_resp_pkt) {
    PYLD_CST(p_resp_pkt, user0_config_app_lcfg_op_hdr_t, p_resp_payload);
    uint16_t reg_data = 0;

    switch (p_in_payload->command) {
    case M2M2_APP_COMMON_CMD_READ_LCFG_REQ:
      for (int i = 0; i < p_in_payload->num_ops; i++) {
        if (user0_config_app_read_lcfg(p_in_payload->ops[i].field, &reg_data) == USER0_CONFIG_SUCCESS) {
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
        if (user0_config_app_write_lcfg(p_in_payload->ops[i].field,
                p_in_payload->ops[i].value) == USER0_CONFIG_SUCCESS) {
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
      return NULL;
    }
    p_resp_pkt->dest = p_pkt->src;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_payload->num_ops = p_in_payload->num_ops;
    p_resp_payload->status = status;
  }
  return p_resp_pkt;
}

#ifdef CUST4_SM
/*!
 ****************************************************************************
 *@brief      m2m2 command to get/set gnUser0ConfigAppState variable in User0
              Config app which decides the state
 *@param      pPkt: pointer to the packet structure
 *@return     m2m2_hdr_t * type pointer
 *****************************************************************************/
static m2m2_hdr_t *user0_config_app_get_set_state(m2m2_hdr_t *p_pkt) {
  M2M2_APP_COMMON_STATUS_ENUM_t status = M2M2_APP_COMMON_STATUS_ERROR;
  PYLD_CST(p_pkt, user0_config_app_set_state_t, p_in_payload);
  /* Allocate a response packet with space for the correct number of operations
   */
  PKT_MALLOC(p_resp_pkt, user0_config_app_set_state_t,0);
  if (NULL != p_resp_pkt) {
    PYLD_CST(p_resp_pkt, user0_config_app_set_state_t, p_resp_payload);

    switch (p_in_payload->command) {
    case M2M2_USER0_CONFIG_APP_COMMAND_SET_STATE_REQ:
        set_user0_config_app_state(p_in_payload->state);
        status = M2M2_APP_COMMON_STATUS_OK;

        p_resp_payload->state = p_in_payload->state;
        p_resp_payload->command =
          (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_USER0_CONFIG_APP_COMMAND_SET_STATE_RESP;
      break;
    case M2M2_USER0_CONFIG_APP_COMMAND_GET_STATE_REQ:
        status = M2M2_APP_COMMON_STATUS_OK;

        p_resp_payload->state = get_user0_config_app_state();
        p_resp_payload->command =
          (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_USER0_CONFIG_APP_COMMAND_GET_STATE_RESP;
      break;
    default:
      /* Something has gone horribly wrong. */
      return NULL;
    }
    p_resp_pkt->dest = p_pkt->src;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_payload->status = status;
  }
  return p_resp_pkt;
}

/*!
 ****************************************************************************
 *@brief      Handles the battery level alert notifications received
 *@param      p_pkt: pointer to the packet structure
 *@return     (m2m2_hdr_t *) NULL
 *****************************************************************************/
static m2m2_hdr_t *user0_battery_level_alert_handler(m2m2_hdr_t *p_pkt)
{
  USER0_CONFIG_APP_STATE_t read_user0_config_app_state;

  if(p_pkt != NULL)
  {
    read_user0_config_app_state = get_user0_config_app_state();
    PYLD_CST(p_pkt, m2m2_pm_sys_cmd_t, p_in_payload);

    switch (p_in_payload->status) {
    case M2M2_PM_SYS_STATUS_BATTERY_LEVEL_FULL:
          NRF_LOG_INFO("User0 config app received Battery Full");
          battery_full_event_cnt++;
          /*if((STATE_SLEEP != read_user0_config_app_state)  &&
                   (STATE_INTERMITTENT_MONITORING != read_user0_config_app_state) &&
                   (STATE_INTERMITTENT_MONITORING_START_LOG != read_user0_config_app_state)&&
                   (STATE_INTERMITTENT_MONITORING_STOP_LOG != read_user0_config_app_state)  )*/
          if((STATE_CHARGING_BATTERY == read_user0_config_app_state))
          {
            set_user0_config_app_state(STATE_ADMIT_STANDBY);
          }
          break;
    case M2M2_PM_SYS_STATUS_BATTERY_LEVEL_CRITICAL:
          NRF_LOG_INFO("User0 config app received Battery Level Critical, \
                        current state:%d", read_user0_config_app_state);
          /* set the flag and increase the count*/
          battery_critical_event_cnt++;
          if((STATE_ADMIT_STANDBY ==
                        read_user0_config_app_state) ||
             (STATE_START_MONITORING ==
                        read_user0_config_app_state) )
            set_user0_config_app_state(STATE_OUT_OF_BATTERY_STATE_BEFORE_START_MONITORING);
          else if( (STATE_SLEEP == read_user0_config_app_state)  ||
                   (STATE_INTERMITTENT_MONITORING == read_user0_config_app_state) ||
                   (STATE_INTERMITTENT_MONITORING_START_LOG == read_user0_config_app_state)||
                   (STATE_INTERMITTENT_MONITORING_STOP_LOG == read_user0_config_app_state)  )
            set_user0_config_app_state(STATE_OUT_OF_BATTERY_STATE_DURING_INTERMITTENT_MONITORING);
          break;
    case M2M2_PM_SYS_STATUS_BATTERY_LEVEL_LOW:
          NRF_LOG_INFO("User0 config app received Battery Level Low");
          /* clear the variables as voltage is above critical level*/
          battery_low_event_cnt++;
          break;
    default:
          return NULL;
    }
  }
  return NULL;
}

/*!
 ****************************************************************************
 * @brief    Returns the current state of user0 config app
 *           This variable gets changed as user0 config app runs
 *           Used by other task like low touch/key detect/ble/filesystem/system
 *           user0 config app task to know the current application state
 * @param    None
 * @retval   USER0_CONFIG_APP_STATE_t type is returned
 *****************************************************************************/
USER0_CONFIG_APP_STATE_t get_user0_config_app_state()
{
  USER0_CONFIG_APP_STATE_t state = gnUser0ConfigAppState;
  return(state);
}

/*!
 ****************************************************************************
 * @brief  Sets the curent state of user0 config app
 *         gnUser0ConfigAppState variable gets changed as user0 config app runs
 * @param  state: variable of type USER0_CONFIG_APP_STATE_t, to set
 *                gnUser0ConfigAppState with
 * @retval None
 ******************************************************************************/
void set_user0_config_app_state(USER0_CONFIG_APP_STATE_t state)
{
  //Update page_watch_id contents of Display
  send_private_type_value(DIS_REFRESH_SIGNAL);
  reset_display_vol_info1();

  switch(state)
  {
    case STATE_START_MONITORING:
    case STATE_END_MONITORING: {
      //Turn on the backlight of LCD
      lcd_backlight_set(LCD_BACKLIGHT_ON);
      break;
    }
    default: {
      //Turn off the backlight of LCD
      lcd_backlight_set(LCD_BACKLIGHT_OFF);
      break;
    }
  }
  gnUser0ConfigAppState = state;
}

/*!
 ****************************************************************************
 * @brief  User0 config App Initialization
 *         gnUser0ConfigAppState variable gets changed as user0 config app runs
 * @param  None
 * @retval 0  --> success 1 --> fail(returned when it has been initialised already)
 ******************************************************************************/
int user0_config_app_init() {
  if (!gnUser0ConfigInitFlag) {
    init_user0_config_app_lcfg();

    gnUser0ConfigInitFlag = 1;

    user0_config_app_on_timer_start();

    /*Wake up the low touch task*/
    adi_osal_SemPost(lt_task_evt_sem);

    return 0;
  }
  return 1;
}

/*!
 ****************************************************************************
 * @brief  User0 config App Deinitialization
 * @param  None
 * @retval 0-->success 1-->fail(returned when it has been deinitialised already)
 ******************************************************************************/
int user0_config_app_deinit() {
  if (gnUser0ConfigInitFlag) {

    gUser0ConfigAppTimerUp = 0;
    gnUser0ConfigInitFlag = 0;

    return 0;
  }
  return 1;
}

/*!
 ****************************************************************************
 * @brief  user0 config app Timer event function,
 *         which is executed from the user0 config app task.
 * @param  None
 * @retval 0-->success
 ******************************************************************************/
int User0ConfigTimerEvent() {
  if (gUser0ConfigAppTimerUp) {
    /*Timer is up; check the new detected state*/
    gUser0ConfigAppTimerUp = 0;

    if (get_user0_config_app_state() != STATE_OUT_OF_BATTERY_STATE_DURING_INTERMITTENT_MONITORING) {
      set_user0_config_app_state(STATE_INTERMITTENT_MONITORING_STOP_LOG);
    }

    user0_config_app_on_timer_stop();
    adi_osal_SemPost(lt_task_evt_sem);  /*Wake up the low touch task*/

  } // if(gUser0ConfigAppTimerUp)
  return 0;
}

/*!
 ****************************************************************************
 * @brief     Function for handling the low touch Mode 3 on wrist detection
 *            timer time-out.
 * @details   This function will be called each time the ON-wrist detection timer
 *            expires.
 * @param[in] p_context - unused
 * @retval    None
 ******************************************************************************/
static void user0_config_app_on_wrist_timeout_handler(void *p_context) {
  NRF_LOG_INFO("User0 config ON wrist Timer expiry..");
  gUser0ConfigAppTimerUp = 1;
  adi_osal_SemPost(user0_config_app_evt_sem);
}

/*!
 ****************************************************************************
 * @brief     Function for the Timer initialization.
 * @details   Initializes the timer module. This creates and starts application
 *            timer for ON-wrist detection. This is the log duration for Mode3
 * @param[in] None
 * @retval    None
 ******************************************************************************/
static void on_timer_init() {
  ret_code_t err_code;

  // Create timers
  err_code = app_timer_create(
      &m_user0_config_app_on_timer_id, APP_TIMER_MODE_SINGLE_SHOT, user0_config_app_on_wrist_timeout_handler);

  APP_ERROR_CHECK(err_code);

  NRF_LOG_INFO("user0_config_app ON wrist Timer created");
}

/*!
 ****************************************************************************
 * @brief     Function for starting ON-wrist application timer.
 * @details   Timers are run after the scheduler has started.
 * @param[in] None
 * @retval    None
 ******************************************************************************/
static void user0_config_app_on_timer_start(void) {
  uint32_t time = 0;

  /* LT application has to be enabled, for the longest duration of the application
    that needs to be logged. Based on this, on timer needs to be configured for this
     timing */
  if((user0_config_app_lcfg.adxl_tON+user0_config_app_lcfg.adxl_start_time) > time)
    time = (user0_config_app_lcfg.adxl_tON+user0_config_app_lcfg.adxl_start_time);
  if((user0_config_app_lcfg.temp_tON+user0_config_app_lcfg.temp_start_time) > time)
    time = (user0_config_app_lcfg.temp_tON+user0_config_app_lcfg.temp_start_time);
  if((user0_config_app_lcfg.adpd_tON+user0_config_app_lcfg.adpd_start_time) > time)
    time = (user0_config_app_lcfg.adpd_tON+user0_config_app_lcfg.adpd_start_time);
  if((user0_config_app_lcfg.eda_tON+user0_config_app_lcfg.eda_start_time) > time)
    time = (user0_config_app_lcfg.eda_tON+user0_config_app_lcfg.eda_start_time);

  /* To the log duration time from above calculation,
  Give one sec extra, for the logging to complete, 12 sec extra which is the delay in the LT app to start up */
  time += (13);
  time = time*1000;//convert seconds to millisec

  // Start repeated timer
  ret_code_t err_code = app_timer_start(
      m_user0_config_app_on_timer_id, APP_TIMER_TICKS(time), NULL);
  APP_ERROR_CHECK(err_code);
}

/*!
 ****************************************************************************
 * @brief    Function for stopping ON-wrist application timer.
 * @param[in] None
 * @retval    None
 ******************************************************************************/
static void user0_config_app_on_timer_stop(void) {
  // Stop the repeated timer
  ret_code_t err_code = app_timer_stop(m_user0_config_app_on_timer_id);
  APP_ERROR_CHECK(err_code);
}
#endif

/*!
 ****************************************************************************
 * @brief    User0 Config task init function.
 * @param[in] None
 * @retval    None
 ******************************************************************************/
void user0_config_app_task_init(void) {
  ADI_OSAL_STATUS eOsStatus;

  /* Create User0 Config app thread */
  g_user0_config_app_attributes.pThreadFunc = user0_config_app;
  g_user0_config_app_attributes.nPriority = APP_OS_CFG_USER0_CONFIG_APP_TASK_PRIO;
  g_user0_config_app_attributes.pStackBase = &ga_user0_config_app_stack[0];
  g_user0_config_app_attributes.nStackSize = APP_OS_CFG_USER0_CONFIG_APP_TASK_STK_SIZE;
  g_user0_config_app_attributes.pTaskAttrParam = NULL;
  g_user0_config_app_attributes.szThreadName = "user0_config_app";
  g_user0_config_app_attributes.pThreadTcb = &g_user0_config_app_tcb;

  eOsStatus = adi_osal_MsgQueueCreate(&gh_user0_config_app_msg_queue, NULL, 9);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
    Debug_Handler();
  } else {
    update_task_queue_list(APP_OS_CFG_USER0_CONFIG_TASK_INDEX, gh_user0_config_app_msg_queue);
  }

  eOsStatus =
      adi_osal_ThreadCreateStatic(&gh_user0_config_app_handler, &g_user0_config_app_attributes);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
    Debug_Handler();
  }

  adi_osal_SemCreate(&user0_config_app_evt_sem, 0);

#ifdef CUST4_SM
  //ON detection Timer creation, for the log duration of LT App Mode3
  on_timer_init();
#endif
}

/*!
 ****************************************************************************
 * @brief User0 Config App task function
 * @param  arg - unused
 * @return none
 ******************************************************************************/
static void user0_config_app(void *arg) {
  ADI_OSAL_STATUS err;

  UNUSED_PARAMETER(arg);

  /*Wait for FDS init to complete*/
  adi_osal_SemPend(user0_config_app_evt_sem, ADI_OSAL_TIMEOUT_FOREVER);

  init_user0_config_app_lcfg();

#ifdef CUST4_SM
  USER0_CONFIG_APP_STATE_t read_user0_config_app_state;
  read_user0_config_app_state = get_user0_config_app_state();
  USBD_CONN_STATUS_t usbd_conn_status = usbd_get_cradle_disconnection_status();

  /*After Watch reset, find previous state and set current state*/

  if((read_user0_config_app_state == STATE_ADMIT_STANDBY) ||
     (read_user0_config_app_state == STATE_START_MONITORING))
  {
    user0_config_app_enter_state_admit_standby();
  }

  else if(read_user0_config_app_state == STATE_END_MONITORING)
  {
    user0_config_app_enter_state_end_monitoring();
  }

  //connected to cradle for charging
  else if(usbd_conn_status == USBD_CONNECTED)
  {
    switch(read_user0_config_app_state)
    {
      case STATE_SLEEP:
      case STATE_INTERMITTENT_MONITORING:
      case STATE_INTERMITTENT_MONITORING_START_LOG:
      case STATE_INTERMITTENT_MONITORING_STOP_LOG:
      case STATE_OUT_OF_BATTERY_STATE_BEFORE_START_MONITORING:
        user0_config_app_enter_state_end_monitoring();
        break;
      case STATE_OUT_OF_BATTERY_STATE_DURING_INTERMITTENT_MONITORING:
        user0_config_app_enter_state_admit_standby();
        break;
      default:
        break;
    }
  }
  //Disconnected from cradle
  else if(usbd_conn_status == USBD_DISCONNECTED)
  {
    switch(read_user0_config_app_state)
    {
      case STATE_SLEEP:
      case STATE_INTERMITTENT_MONITORING:
      case STATE_INTERMITTENT_MONITORING_START_LOG:
      case STATE_INTERMITTENT_MONITORING_STOP_LOG:
        user0_config_app_enter_state_intermittent_monitoring();
        break;
      default:
        break;
    }
  }

  /* Force the current state to STATE_ADMIT_STANDBY incase of any unknown state */
  else if( (read_user0_config_app_state != STATE_ADMIT_STANDBY) &&
      (read_user0_config_app_state > STATE_OUT_OF_BATTERY_STATE_DURING_INTERMITTENT_MONITORING) )
  {
    user0_config_app_enter_state_admit_standby();
  }
#endif

  while (1) {
    m2m2_hdr_t *p_in_pkt = NULL;
    m2m2_hdr_t *p_out_pkt = NULL;
    adi_osal_SemPend(user0_config_app_evt_sem, ADI_OSAL_TIMEOUT_FOREVER);
    p_in_pkt = post_office_get(ADI_OSAL_TIMEOUT_NONE, APP_OS_CFG_USER0_CONFIG_TASK_INDEX); //user0 conig app events

    // We got an m2m2 message from the queue, process it.
    if (p_in_pkt != NULL) {
      // We got an m2m2 message from the queue, process it.
      PYLD_CST(p_in_pkt, _m2m2_app_common_cmd_t, p_in_cmd);
      // Look up the appropriate function to call in the function table
      for (int i = 0; i < USER0_CONFIG_APP_ROUTING_TBL_SZ; i++) {
        if (user0_config_app_routing_table[i].command == p_in_cmd->command) {
          p_out_pkt = user0_config_app_routing_table[i].cb_handler(p_in_pkt);
          break;
        }
      }
      post_office_consume_msg(p_in_pkt);
      if (p_out_pkt != NULL) {
        post_office_send(p_out_pkt, &err);
      }
    }
#ifdef CUST4_SM
    else
    {
      User0ConfigTimerEvent();
    }
#endif
  }
}

#ifdef CUST4_SM
/*!
 ****************************************************************************
 * @brief  Function which changes the state  of User0 Config App task function
 *         to STATE_ADMIT_STANDBY

 * @param  None
 * @return None
 *****************************************************************************/
void user0_config_app_enter_state_admit_standby()
{
  set_user0_config_app_state(STATE_ADMIT_STANDBY);
}

/*!
 ****************************************************************************
 * @brief  Function which changes the state  of User0 Config App task function
 *         to STATE_START_MONITORING
 * @param  None
 * @return None
 *****************************************************************************/
void user0_config_app_enter_state_start_monitoring()
{
  set_user0_config_app_state(STATE_START_MONITORING);
  gn_intermittent_op_count=0;
}

/*!
 ****************************************************************************
 * @brief  Function which changes the state  of User0 Config App task function
 *         to STATE_SLEEP
 * @param  None
 * @return None
 *****************************************************************************/
void user0_config_app_enter_state_sleep()
{
  uint32_t sleep_time_secs;
  set_user0_config_app_state(STATE_SLEEP);
  //Do watch power down
  /* Set the RTC to give an interupt after comparing upto total of
   * Watch current time + sleep_min in lcfg
   */
  sleep_time_secs = (user0_config_app_lcfg.sleep_min * 60);
  enable_rtc_wakeup(sleep_time_secs);
  /*After Watch Operation, go to sleep mode */
  enter_poweroff_mode();
}

/*!
 ****************************************************************************
 * @brief  Function which changes the state  of User0 Config App task function
 *         to STATE_INTERMITTENT_MONITORING
 * @param  None
 * @return None
 *****************************************************************************/
void user0_config_app_enter_state_intermittent_monitoring()
{
  /* Wait for FS task FindConfigFile() completes */
  adi_osal_SemPend(user0_config_app_evt_sem, ADI_OSAL_TIMEOUT_FOREVER);

  // On Bootup Enable User0 Config app if user0 config cfg files are present
  if( get_low_touch_trigger_mode3_status() &&
      (gen_blk_get_dcb_present_flag() || gsCfgFileFoundFlag) )
  {
      //Change state after FS task updates the config file status
      set_user0_config_app_state(STATE_INTERMITTENT_MONITORING);

      //Turn off BLE
      turn_off_BLE();
      //Change the BLE advertising duration as specified in user0 config DCB
      change_ble_adv_duration(user0_config_app_lcfg.adv_timeout_monitor);

      user0_config_app_init(true);
      /* Increment debug variable to count no: of times intermittent
         operation is done */
      gn_intermittent_op_count++;
  }
  else
  {
      /*
       * After wakeup from sleep, found that no Configs exist -> fall back to inital state
      */
      NRF_LOG_INFO("No Cfg found after sleep state-setting admit standby state");
      set_user0_config_app_state(STATE_ADMIT_STANDBY);
      user0_config_app_deinit(false);
  }
}

/*!
 ****************************************************************************
 * @brief  Function which changes the state  of User0 Config App task function
 *         to STATE_END_MONITORING
 * @param  None
 * @return None
 *****************************************************************************/
void user0_config_app_enter_state_end_monitoring()
{
  set_user0_config_app_state(STATE_END_MONITORING);
}

/*!
 ****************************************************************************
 * @brief  Function which changes the state  of User0 Config App task function
 *         to STATE_CHARGING_BATTERY
 * @param  None
 * @return None
 *****************************************************************************/
void user0_config_app_enter_charging_battery()
{
  set_user0_config_app_state(STATE_CHARGING_BATTERY);
}
#endif

/*!
 ****************************************************************************
 * @brief  Function to send PO pkts to user0 config app task
 * @param  p_pkt m2m2 packet to be send to user0 config app task
 * @return None
 *****************************************************************************/
void send_message_user0_config_app(m2m2_hdr_t *p_pkt) {
  ADI_OSAL_STATUS osal_result;
  if(p_pkt != NULL)
  {
    osal_result = adi_osal_MsgQueuePost(gh_user0_config_app_msg_queue, p_pkt);
    if (osal_result != ADI_OSAL_SUCCESS)
      post_office_consume_msg(p_pkt);
  }
  adi_osal_SemPost(user0_config_app_evt_sem);
}

/*!
 ****************************************************************************
 * @brief  Function to check if ADXL App is configured for continuous operation
 *         from user0 config app lcfg (default fw/DCB)
 * @Desc   Continuous operation is when sleep_min=tON=tOFF=0
 * @param  None
 * @return True:  ADXL App is configured for continuous operation
 *         False: ADXL App is NOT configured for continuous operation
 *
 ****************************************************************************/
bool is_adxl_app_mode_continuous()
{
  if((user0_config_app_lcfg.adxl_tON==0) &&
     (user0_config_app_lcfg.adxl_tOFF==0) &&
     (user0_config_app_lcfg.sleep_min==0))
    return true;
  else
    return false;
}

/*!
 ****************************************************************************
 * @brief  Function to get ADXL App timings from user0 config app lcfg
 *         (default fw/DCB)
 * @param  start_time: start time of App, tON: On time of app in secs
 *         tOFF: Off time of app in secs
 * @return None
 *
 ****************************************************************************/
void get_adxl_app_timings_from_user0_config_app_lcfg(uint16_t *start_time, \
              uint16_t *tON, uint16_t *tOFF)
{
  *start_time = user0_config_app_lcfg.adxl_start_time;
  *tON = user0_config_app_lcfg.adxl_tON;
  *tOFF = user0_config_app_lcfg.adxl_tOFF;
}

/*!
 ****************************************************************************
 * @brief  Function to check if Temp App is configured for continuous operation
 *         from user0 config app lcfg (default fw/DCB)
 * @Desc   Continuous operation is when sleep_min=tON=tOFF=0
 * @param  None
 * @return True:  Temp App is configured for continuous operation
 *         False: Temp App is NOT configured for continuous operation
 *
 ****************************************************************************/
bool is_temp_app_mode_continuous()
{
  if((user0_config_app_lcfg.temp_tON==0) &&
     (user0_config_app_lcfg.temp_tOFF==0) &&
     (user0_config_app_lcfg.sleep_min==0))
    return true;
  else
    return false;
}

/*!
 ****************************************************************************
 * @brief  Function to get Temp App timings from user0 config app lcfg
 *         (default fw/DCB)
 * @param  start_time: start time of App, tON: On time of app in secs
 *         tOFF: Off time of app in secs
 * @return None
 *
 ****************************************************************************/
void get_temp_app_timings_from_user0_config_app_lcfg(uint16_t *start_time, \
              uint16_t *tON, uint16_t *tOFF)
{
  *start_time = user0_config_app_lcfg.temp_start_time;
  *tON = user0_config_app_lcfg.temp_tON;
  *tOFF = user0_config_app_lcfg.temp_tOFF;
}

/*!
 ****************************************************************************
 * @brief  Function to check if ADPD App is configured for continuous operation
 *         from user0 config app lcfg (default fw/DCB)
 * @Desc   Continuous operation is when sleep_min=tON=tOFF=0
 * @param  None
 * @return True:  ADPD App is configured for continuous operation
 *         False: ADPD App is NOT configured for continuous operation
 *
 ****************************************************************************/
bool is_adpd_app_mode_continuous()
{
  if((user0_config_app_lcfg.adpd_tON==0) &&
     (user0_config_app_lcfg.adpd_tOFF==0) &&
     (user0_config_app_lcfg.sleep_min==0))
    return true;
  else
    return false;
}

/*!
 ****************************************************************************
 * @brief  Function to get ADPD App timings from user0 config app lcfg
 *         (default fw/DCB)
 * @param  start_time: start time of App, tON: On time of app in secs
 *         tOFF: Off time of app in secs
 * @return None
 *
 ****************************************************************************/
void get_adpd_app_timings_from_user0_config_app_lcfg(uint16_t *start_time, \
              uint16_t *tON, uint16_t *tOFF)
{
  *start_time = user0_config_app_lcfg.adpd_start_time;
  *tON = user0_config_app_lcfg.adpd_tON;
  *tOFF = user0_config_app_lcfg.adpd_tOFF;
}

/*!
 ****************************************************************************
 * @brief  Function to check if EDA App is configured for continuous operation
 *         from user0 config app lcfg (default fw/DCB)
 * @Desc   Continuous operation is when sleep_min=tON=tOFF=0
 * @param  None
 * @return True:  EDA App is configured for continuous operation
 *         False: EDA App is NOT configured for continuous operation
 *
 ****************************************************************************/
bool is_eda_app_mode_continuous()
{
  if((user0_config_app_lcfg.eda_tON==0) &&
     (user0_config_app_lcfg.eda_tOFF==0) &&
     (user0_config_app_lcfg.sleep_min==0))
    return true;
  else
    return false;
}

/*!
 ****************************************************************************
 * @brief  Function to get EDA App timings from user0 config app lcfg
 *         (default fw/DCB)
 * @param  start_time: start time of App, tON: On time of app in secs
 *         tOFF: Off time of app in secs
 * @return None
 *
 ****************************************************************************/
void get_eda_app_timings_from_user0_config_app_lcfg(uint16_t *start_time, \
              uint16_t *tON, uint16_t *tOFF)
{
  *start_time = user0_config_app_lcfg.eda_start_time;
  *tON = user0_config_app_lcfg.eda_tON;
  *tOFF = user0_config_app_lcfg.eda_tOFF;
}

/*!
 ****************************************************************************
 * @brief  Function to set the user0 config app lcfg to default fw values
 * @param  None
 * @return None
 *
 ****************************************************************************/
void user0_config_app_lcfg_set_fw_default()
{
    user0_config_app_lcfg.agc_up_th  = USER0_CFG_AGC_UP_TH;
    user0_config_app_lcfg.agc_low_th = USER0_CFG_AGC_LOW_TH;
    user0_config_app_lcfg.adv_timeout_monitor  = USER0_CFG_ADV_TIMEOUT_MONITOR;
    user0_config_app_lcfg.hw_id = USER0_CFG_HW_ID;
    user0_config_app_lcfg.exp_id  = USER0_CFG_EXP_ID;
    user0_config_app_lcfg.adxl_start_time  = USER0_CFG_ADXL_START_TIME;
    user0_config_app_lcfg.adxl_tON  = USER0_CFG_ADXL_TON;
    user0_config_app_lcfg.adxl_tOFF  = USER0_CFG_ADXL_TOFF;
    user0_config_app_lcfg.temp_start_time  = USER0_CFG_TEMP_START_TIME;
    user0_config_app_lcfg.temp_tON  = USER0_CFG_TEMP_TON;
    user0_config_app_lcfg.temp_tOFF  = USER0_CFG_TEMP_TOFF;
    user0_config_app_lcfg.adpd_start_time  = USER0_CFG_ADPD_START_TIME;
    user0_config_app_lcfg.adpd_tON  = USER0_CFG_ADPD_TON;
    user0_config_app_lcfg.adpd_tOFF  = USER0_CFG_ADPD_TOFF;
    user0_config_app_lcfg.eda_start_time  = USER0_CFG_EDA_START_TIME;
    user0_config_app_lcfg.eda_tON  = USER0_CFG_EDA_TON;
    user0_config_app_lcfg.eda_tOFF  = USER0_CFG_EDA_TOFF;
    user0_config_app_lcfg.sleep_min  = USER0_CFG_SLEEP_MIN;
    user0_config_app_lcfg.signal_threshold  = USER0_CFG_SIGNAL_THRESHOLD;

    //Update hw_id in the System Info structure
    update_hw_id_in_system_info(user0_config_app_lcfg.hw_id);
}

/*!
 ****************************************************************************
 * @brief  Function to set the User0 Config app lcfg to from user0_config_app_lcfg DCB
 * @param  None
 * @return None
 ****************************************************************************/
void user0_config_app_lcfg_set_from_dcb()
{
  load_user0_config_app_lcfg_dcb();
  copy_user0_config_from_user0_blk_dcb(&user0_config_app_lcfg);
  //Update hw_id in the System Info structure
  update_hw_id_in_system_info(user0_config_app_lcfg.hw_id);
}

#ifdef DCB
/**
 * @brief  Function which handles the m2m2 command to do user0_blk_DCB read
 * @param  p_pkt m2m2 REQ packet
 * @return m2m2 RESP packet
 */
static m2m2_hdr_t *user0_blk_dcb_command_read_config(m2m2_hdr_t *p_pkt) {
    uint16_t r_size = 0;
    uint32_t dcbdata[MAXUSER0BLKDCBSIZE];
    M2M2_DCB_STATUS_ENUM_t status = M2M2_DCB_STATUS_ERR_NOT_CHKD;

    PKT_MALLOC(p_resp_pkt, m2m2_dcb_user0_blk_data_t, 0);
    // Declare a pointer to the response packet payload
    PYLD_CST(p_resp_pkt, m2m2_dcb_user0_blk_data_t, p_resp_payload);

    r_size = (uint16_t)MAXUSER0BLKDCBSIZE;
    if(read_user0_blk_dcb(&dcbdata[0],&r_size) == USER0_BLK_DCB_STATUS_OK)
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
 * @brief  Function which handles the m2m2 command to do user0_blk_DCB write
 * @param  p_pkt m2m2 REQ packet
 * @return m2m2 RESP packet
 */
static m2m2_hdr_t *user0_blk_dcb_command_write_config(m2m2_hdr_t *p_pkt) {
    M2M2_DCB_STATUS_ENUM_t status = M2M2_DCB_STATUS_ERR_NOT_CHKD;
    uint32_t dcbdata[MAXUSER0BLKDCBSIZE];

    // Declare a pointer to access the input packet payload
    PYLD_CST(p_pkt, m2m2_dcb_user0_blk_data_t, p_in_payload);
    PKT_MALLOC(p_resp_pkt, m2m2_dcb_user0_blk_data_t, 0);
    // Declare a pointer to the response packet payload
    PYLD_CST(p_resp_pkt, m2m2_dcb_user0_blk_data_t, p_resp_payload);

    for(int i=0; i<p_in_payload->size; i++)
      dcbdata[i] = p_in_payload->dcbdata[i];
    if(write_user0_blk_dcb(&dcbdata[0],p_in_payload->size) == USER0_BLK_DCB_STATUS_OK)
    {
        user0_blk_set_dcb_present_flag(true);
        user0_config_app_lcfg_set_from_dcb();
        status = M2M2_DCB_STATUS_OK;
    }
    else
    {
        status = M2M2_DCB_STATUS_ERR_ARGS;
    }

    p_resp_payload->status = status;
    p_resp_payload->command = M2M2_DCB_COMMAND_WRITE_CONFIG_RESP;
    p_resp_payload->size = 0;
    for(uint16_t i=0; i< MAXUSER0BLKDCBSIZE; i++)
        p_resp_payload->dcbdata[i] = 0;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
    return p_resp_pkt;
}

/**
 * @brief  Function which handles the m2m2 command to do user0_blk_DCB delete
 * @param  p_pkt m2m2 REQ packet
 * @return m2m2 RESP packet
 */
static m2m2_hdr_t *user0_blk_dcb_command_delete_config(m2m2_hdr_t *p_pkt) {
    M2M2_DCB_STATUS_ENUM_t status = M2M2_DCB_STATUS_ERR_NOT_CHKD;

    PKT_MALLOC(p_resp_pkt, m2m2_dcb_user0_blk_data_t, 0);
    // Declare a pointer to the response packet payload
    PYLD_CST(p_resp_pkt, m2m2_dcb_user0_blk_data_t, p_resp_payload);

    if(delete_user0_blk_dcb() == USER0_BLK_DCB_STATUS_OK)
    {
        user0_blk_set_dcb_present_flag(false);
        user0_config_app_lcfg_set_fw_default();
        status = M2M2_DCB_STATUS_OK;
    }
    else
    {
        status = M2M2_DCB_STATUS_ERR_ARGS;
    }

    p_resp_payload->status = status;
    p_resp_payload->command = M2M2_DCB_COMMAND_ERASE_CONFIG_RESP;
    p_resp_payload->size = 0;
    for(uint16_t i=0; i< MAXUSER0BLKDCBSIZE; i++)
        p_resp_payload->dcbdata[i] = 0;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
    return p_resp_pkt;

}

/**
 * @brief  Gets the ID number configured in USER0_BLK DCB
 *
 * @param[in]  id_index: Index from the USER0_BLK DCB to be read from
 *             id_num: Pointer to where the id number is to be read to
 *
 * @return     uint8_t value: GET_ID_SUCCESS, 0-> read DCB success
 *                            GET_ID_FAILURE, 1-> read DCB failed
 */
uint8_t get_id_num(USER0_CONFIG_LCFG_t id_index, uint16_t * id_num) {
  uint16_t r_size = 0;
  uint32_t dcbdata[MAXUSER0BLKDCBSIZE];
  uint8_t status = GET_ID_FAILURE;

  r_size = (uint16_t)MAXUSER0BLKDCBSIZE;
  if (read_user0_blk_dcb(&dcbdata[0], &r_size) == USER0_BLK_DCB_STATUS_OK) {
    *id_num = (uint16_t)dcbdata[id_index];
    status = GET_ID_SUCCESS;
  } else {
    *id_num = 0;
    status = GET_ID_FAILURE;
  }

  return status;
}

/**
 * @brief  Gets the ID number configured in USER0_BLK default/modified
 *         Fw lcfg
 *
 * @param[in]  id_num: Pointer to where the id number is to be read to
 *
 * @return     None
 */
void get_id_num_fw_lcfg(uint16_t * id_num) {
  *id_num = user0_config_app_lcfg.exp_id;
}

/**
 * @brief  Sets the ID number configured in USER0_BLK DCB
 *
 * @param[in]  id_index: Index from the USER0_BLK DCB to be written to
 *             id_num: Value to the id_num to in the id_index field in DCB
 *
 * @return     uint8_t value: SET_ID_SUCCESS, 0-> Write DCB success
 *                            SET_ID_FAILURE, 1-> Write DCB failed
 */
uint8_t set_id_num(USER0_CONFIG_LCFG_t id_index, uint16_t id_num) {
  uint16_t r_size = 0;
  uint32_t dcbdata[MAXUSER0BLKDCBSIZE];
  uint8_t status = SET_ID_FAILURE;

  r_size = (uint16_t)MAXUSER0BLKDCBSIZE;
  if (read_user0_blk_dcb(&dcbdata[0], &r_size) == USER0_BLK_DCB_STATUS_OK) {
    dcbdata[id_index] = id_num; //modify only id_num

    if (write_user0_blk_dcb(&dcbdata[0], r_size) == USER0_BLK_DCB_STATUS_OK) {
      user0_config_app_lcfg_set_from_dcb();
      status = SET_ID_SUCCESS;
    } else {
      status = SET_ID_FAILURE;
    }
  }

  return status;
}

/**
 * @brief  Does ID operation-read/write/delete of id information(hw_id/exp_id)
 *         configured in USER0_BLK DCB
 *
 * @param[in]  p_pkt: Pointer to the input m2m2 packet structure with command
 *                    for reading id information(hw_id/exp_id)
 *
 * @return     p_resp_pkt: Pointer to the output response m2m2 packet structure
 *                         with id information(hw_id/exp_id)
 */
static m2m2_hdr_t *id_operation(m2m2_hdr_t *p_pkt) {
  M2M2_USER0_CONFIG_APP_STATUS_ENUM_t status = M2M2_USER0_CONFIG_APP_STATUS_ERR_NOT_CHKD;
  USER0_CONFIG_LCFG_t id_index;

  /*  Declare a pointer to the input packet payload */
  PYLD_CST(p_pkt, user0_config_app_id_t, p_in_payload);
  PKT_MALLOC(p_resp_pkt, user0_config_app_id_t, 0);
  if (NULL != p_resp_pkt) {

    /*  Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, user0_config_app_id_t, p_resp_payload);

    switch(p_in_payload->id_sel)
    {
      case ID_HW_ID:
        id_index = USER0_CONFIG_LCFG_HW_ID;
        status = M2M2_USER0_CONFIG_APP_STATUS_OK;
        break;
      case ID_EXP_ID:
        id_index = USER0_CONFIG_LCFG_EXP_ID;
        status = M2M2_USER0_CONFIG_APP_STATUS_OK;
        break;
      default:
        //Didnt get matching id index
        status = M2M2_USER0_CONFIG_APP_STATUS_ERR_ARGS;
        break;
    }

    switch(p_in_payload->id_op)
    {
      case ID_OPERATION_MODE_READ:

        //Found matching id index
        if(M2M2_USER0_CONFIG_APP_STATUS_OK == status)
        {
          if (GET_ID_SUCCESS == get_id_num(id_index, &p_resp_payload->id_num)) {
            status = M2M2_USER0_CONFIG_APP_STATUS_OK;
          } else {
            status = M2M2_USER0_CONFIG_APP_STATUS_ERR_ARGS;
          }
        }
        p_resp_payload->id_op = ID_OPERATION_MODE_READ;
        break;

      case ID_OPERATION_MODE_WRITE:
        //Found matching id index
        if(M2M2_USER0_CONFIG_APP_STATUS_OK == status)
        {
          if (GET_ID_SUCCESS == set_id_num(id_index, p_in_payload->id_num)) {
            status = M2M2_USER0_CONFIG_APP_STATUS_OK;
          } else {
            status = M2M2_USER0_CONFIG_APP_STATUS_ERR_ARGS;
          }
        }
        p_resp_payload->id_op = ID_OPERATION_MODE_WRITE;
        p_resp_payload->id_num = p_in_payload->id_num;
#ifdef CUST4_SM
        //Update the id shown in page_watch_id of Display
        send_private_type_value(DIS_REFRESH_SIGNAL);
        reset_display_vol_info1();
#endif
        break;

      case ID_OPERATION_MODE_DELETE:
        //Found matching id index
        if(M2M2_USER0_CONFIG_APP_STATUS_OK == status)
        {
          if (GET_ID_SUCCESS == set_id_num(id_index, 0)) {
            status = M2M2_USER0_CONFIG_APP_STATUS_OK;
          } else {
            status = M2M2_USER0_CONFIG_APP_STATUS_ERR_ARGS;
          }
        }
        p_resp_payload->id_op = ID_OPERATION_MODE_DELETE;
        p_resp_payload->id_num = 0;//Clearing the exp_id information
        break;
      default:
        p_resp_payload->id_op = p_in_payload->id_op;
        p_resp_payload->id_num = p_in_payload->id_num;
        status = M2M2_USER0_CONFIG_APP_STATUS_ERR_ARGS;
        break;
    }

    p_resp_payload->status = status;
    p_resp_payload->command = M2M2_USER0_CONFIG_APP_COMMAND_ID_OP_RESP;
    p_resp_payload->id_sel = p_in_payload->id_sel;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;

  }//if (NULL != p_resp_pkt)

  return p_resp_pkt;
}
#endif // DCB
#endif//USER0_CONFIG_APP