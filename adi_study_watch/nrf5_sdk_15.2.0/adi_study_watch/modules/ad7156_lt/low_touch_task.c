/**
***************************************************************************
* @addtogroup Tasks
* @{
* @file         <low_touch_task.c>
* @author       ADI
* @version      V1.0.0
* @date         17-Nov-2020
* @brief        Source file contains functions low touch application
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

/*---------------------------- Includes --------------------------------------*/
#ifdef LOW_TOUCH_FEATURE
#include "low_touch_task.h"
#include "Common.h"
#include "ad7156.h"
#include "app_timer.h"
#include "hw_if_config.h"
#include "m2m2_core.h"
#include "nrf_drv_gpiote.h"
#include "nrf_log_ctrl.h"
#include "post_office.h"
#include "touch_detect.h"
#include <adi_osal.h>
#include <app_cfg.h>
#include <common_application_interface.h>
#include <low_touch_application_interface.h>
#include <math.h>
#include <post_office.h>
#include <stdio.h>
#include <stdlib.h>
#include <task_includes.h>
#ifdef VSM_MBOARD
#include "watch_vsm_motherboard_pin_config.h"
#elif PCBA
#include "watch_board_pcba_pin_config.h"
#else
#include "watch_board_evt_pin_config.h"
#endif
#ifdef DCB
#include "adi_dcb_config.h"
#include "dcb_general_block.h"
#include "wrist_detect_block.h"
#include <dcb_interface.h>
#endif
/* Low touch App Module Log settings */
#define NRF_LOG_MODULE_NAME LT_App

#if LT_APP_CONFIG_LOG_ENABLED
#ifdef NRF_LOG_DEFAULT_LEVEL
#undef NRF_LOG_DEFAULT_LEVEL
#define NRF_LOG_DEFAULT_LEVEL LT_APP_CONFIG_LOG_LEVEL
#endif
#define NRF_LOG_INFO_COLOR LT_APP_CONFIG_INFO_COLOR
#define NRF_LOG_DEBUG_COLOR LT_APP_CONFIG_DEBUG_COLOR
#else // LT_APP_CONFIG_LOG_ENABLED
#define NRF_LOG_LEVEL 0
#endif // LT_APP_CONFIG_LOG_ENABLED

#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

/* For low touch FreeRTOS Task Creation */
ADI_OSAL_STATIC_THREAD_ATTR g_lt_task_attributes;
uint8_t ga_lt_task_stack[APP_OS_CFG_LT_APP_TASK_STK_SIZE];
StaticTask_t g_lt_task_tcb;
ADI_OSAL_THREAD_HANDLE gh_lt_task_handler;
ADI_OSAL_QUEUE_HANDLE gh_lt_task_msg_queue = NULL;
// Create semaphores
//static ADI_OSAL_SEM_HANDLE   lt_task_evt_sem;

/* ON wrist time detection for starting logging 5000 (ms). */
#define LT_ON_WRIST_TIME_INTERVAL 5000
/* OFF wrist time detection for starting logging 3000 (ms). */
#define LT_OFF_WRIST_TIME_INTERVAL 3000
/* Air Capacitance Value (uF) kept at default
* This was kept to the min. value seen in 10 trials
*/
#define LT_AIR_CAP_VAL 1380
/* Skin Capacitance Value (uF) kept at default
* This was kept to the max. value seen in 10 trials
*/
#define LT_SKIN_CAP_VAL 1340
/* Handler for repeated timer for LT ON. */
APP_TIMER_DEF(m_lt_on_timer_id);
/* Handler for repeated timer for LT OFF. */
APP_TIMER_DEF(m_lt_off_timer_id);

#ifdef DVT
/* For Watch prototype:
 * A maximum OffWrist Capacitance value in uF put, based on
 * seeing from couple of Watches, used for reset condition */
#define TOUCH_DETECTION_THRESHOLD 1490
//#define TOUCH_DETECTION_THRESHOLD     1300 //Bare board
#else
#define TOUCH_DETECTION_THRESHOLD 1300
#endif

/*Timeout value to extract msg from PO Msg Queue is to be:
 ADI_OSAL_TIMEOUT_FOREVER -> m2m2 pkts
 ADI_OSAL_TIMEOUT_NONE    -> for processing LT event */
static volatile uint32_t  g_lt_task_timeout = ADI_OSAL_TIMEOUT_FOREVER;

/* Flag which is used to enable/disable LT application; which is later used to
 * prevent mulitple init-deinit from happening */
static uint8_t gEnableLowTouchDetection = 0;

/* Debug flag to count the events happening */
uint8_t OnWristEventDetecedCnt = 0, OffWristEventDetecedCnt = 0;
/* Flag which gets set, when either ON/OFF timer expires. Its based on this flag
 * and the event detected, that the LT commands are send */
volatile uint8_t gLowTouchTimerUp = 0;

/* Flag which gets set in the AD7156 interrupt handler & cleared upon servicing
 * the event */
volatile uint8_t gsLowTouchAd7156IntFlag = 0;
/* Variable which holds the gpio output value, when the AD7156 interrupt handler
 * gets called */
volatile uint8_t gsLowTouchAd7156IntValue = 0;
/* Variable which holds the capacitance value, when the AD7156 interrupt handler
 * gets called */
volatile uint16_t gsLowTouchAd7156CapVal = 0;
/* Variable which holds the status of ad7156 CH2 specific init/deinitialisation */
static uint8_t gsLowTouchInitFlag = 0;
/* Variable to hold the last ON Wrist cap value based on which, later events cap
 * is taken, to consider for current Wrist state*/
volatile uint16_t g_onWrCapValue = TOUCH_DETECTION_THRESHOLD;
/* Variable to hold the last OFF Wrist cap value based on which, later events
 * cap is taken, to consider for current Wrist state*/
volatile uint16_t g_offWrCapValue = TOUCH_DETECTION_THRESHOLD;
/* Debug Variable which counts the no: AD7156 interrupts received */
volatile uint16_t gsLowTouchAd7156IntCount = 0;
/* Variable which holds sensitivity to be configured to AD7156 Ch2 Cap sensor
 After configuring AD7156 sensitivity, this variable is updated and used as
 an offset for comparison of cap values b/w On and Off wrist events */
uint16_t gn_ch2_sensitivity;

static int16_t user_applied_onWristTimeThreshold = 0;
static int16_t user_applied_offWristTimeThreshold = 0;
static int16_t user_applied_airCapVal = 0;
static int16_t user_applied_skinCapVal = 0;

lt_app_cfg_type lt_app_cfg;
/*-------------------- Private Function Declarations ------------------------*/
static void LowTouchAd7156IntCallback(uint8_t value);
static void SendStartLowTouchLogReq();
static void SendStopLowTouchLogReq();
static void on_timer_init();
static void off_timer_init();
static void lt_on_timer_start(void);
static void lt_on_timer_stop(void);
static void lt_off_timer_start(void);
static void lt_off_timer_stop(void);
static void lt_task(void *arg);
static void out2_pin_detect(uint8_t value);
static void LowTouchResetTimer();
static void init_lt_app_lcfg();
static m2m2_hdr_t *lt_app_lcfg_access(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *lt_app_read_ch2_cap(m2m2_hdr_t *p_pkt);
#ifdef DCB
static m2m2_hdr_t *gen_blk_dcb_command_read_config(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *gen_blk_dcb_command_write_config(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *gen_blk_dcb_command_delete_config(m2m2_hdr_t *p_pkt);
#endif

/*--------------------- Public Funtion Declarations -------------------------*/
extern void SetCfgCopyAvailableFlag(uint8_t nflag);
extern void find_low_touch_DCB();

/* Variable which holds the status of NAND config file */
extern volatile uint8_t gsCfgFileFoundFlag;

/* State machine variables for ON/OFF wrist events */
volatile LOW_TOUCH_DETECTION_STATUS_ENUM_t eDetection_State = OFF_WRIST,
                                  eCurDetection_State = OFF_WRIST;

/* Uncomment the below macro & change variables from SES Debug session:
   eCurDetection_State = ON_WRIST , gb_trig_event = 1 -> to generate ON_WR event
   eCurDetection_State = OFF_WRIST, gb_trig_event = 1 -> to generate OFF_WR
   event
*/
//#define TEST_LT_APP_WITHOUT_EVENTS 1

#define LT_APP_ROUTING_TBL_SZ                                                  \
  (sizeof(lt_app_routing_table) / sizeof(lt_app_routing_table[0]))

typedef m2m2_hdr_t *(app_cb_function_t)(m2m2_hdr_t *);

typedef struct _app_routing_table_entry_t {
  uint8_t command;
  app_cb_function_t *cb_handler;
} app_routing_table_entry_t;

/* Routing table structure, to handle m2m2 command REQ from LT task function */
app_routing_table_entry_t lt_app_routing_table[] = {
    {M2M2_APP_COMMON_CMD_READ_LCFG_REQ,  lt_app_lcfg_access},
    {M2M2_APP_COMMON_CMD_WRITE_LCFG_REQ, lt_app_lcfg_access},
    {M2M2_LT_COMMAND_RD_CH2_CAP_REQ, lt_app_read_ch2_cap},
#ifdef DCB
    {M2M2_DCB_COMMAND_READ_CONFIG_REQ, gen_blk_dcb_command_read_config},
    {M2M2_DCB_COMMAND_WRITE_CONFIG_REQ, gen_blk_dcb_command_write_config},
    {M2M2_DCB_COMMAND_ERASE_CONFIG_REQ, gen_blk_dcb_command_delete_config},
#endif
};

static void init_lt_app_lcfg() {

  /* Check & Load from DCB LT lcfg if lcfg is available in wrist_detect_dcb */
  if (wrist_detect_get_dcb_present_flag()) {

    lt_app_lcfg_set_from_dcb();
  }

  /* Loading default or modified fw LT lcfg */
  else
  {
    if( !user_applied_onWristTimeThreshold )
      lt_app_cfg.onWristTimeThreshold  = LT_ON_WRIST_TIME_INTERVAL;
    else
      user_applied_onWristTimeThreshold = 0;

    if( !user_applied_offWristTimeThreshold )
      lt_app_cfg.offWristTimeThreshold = LT_OFF_WRIST_TIME_INTERVAL;
    else
      user_applied_offWristTimeThreshold = 0;

    if( !user_applied_airCapVal )
      lt_app_cfg.airCapVal  = LT_AIR_CAP_VAL;
    else
      user_applied_airCapVal = 0;

    if( !user_applied_skinCapVal )
      lt_app_cfg.skinCapVal = LT_SKIN_CAP_VAL;
    else
      user_applied_skinCapVal = 0;
  }

  //Sanity check on parameters
  if( lt_app_cfg.airCapVal < lt_app_cfg.skinCapVal        ||
      (lt_app_cfg.airCapVal - lt_app_cfg.skinCapVal) < 10 )
  {
    NRF_LOG_INFO("Unexpected lcfg parameters passed, falling to fw default");
    lt_app_cfg.airCapVal  = LT_AIR_CAP_VAL;
    lt_app_cfg.skinCapVal = LT_SKIN_CAP_VAL;
  }

  gn_ch2_sensitivity = (lt_app_cfg.airCapVal - lt_app_cfg.skinCapVal)/2;
  AD7156_SetSensitivity(AD7156_CHANNEL2,gn_ch2_sensitivity);
  //AD7156_SetSensitivity(AD7156_CHANNEL2,25);
}

/*!
 ****************************************************************************
 * @brief    Example of how to write an LCFG parameter
 * @param    LCFG field that has to be written
 * @param    Value to be written
 * @retval   LT_APP_ERROR_CODE_t
 *****************************************************************************/
LT_APP_ERROR_CODE_t lt_app_write_lcfg(uint8_t field, uint16_t value) {
  if (field < LT_APP_LCFG_MAX) {
    switch (field) {
    case LT_APP_LCFG_ONWR_TIME:
      lt_app_cfg.onWristTimeThreshold = value;
      user_applied_onWristTimeThreshold = 1;
      break;
    case LT_APP_LCFG_OFFWR_TIME:
      lt_app_cfg.offWristTimeThreshold = value;
      user_applied_offWristTimeThreshold = 1;
      break;
    case LT_APP_LCFG_AIR_CAP_VAL:
      lt_app_cfg.airCapVal = value;
      user_applied_airCapVal = 1;
      break;
    case LT_APP_LCFG_SKIN_CAP_VAL:
      lt_app_cfg.skinCapVal = value;
      user_applied_skinCapVal = 1;
      break;
    }
    return LT_APP_SUCCESS;
  }
  return LT_APP_ERROR;
}

/*!
 ****************************************************************************
 * @brief    Read LCFG parameter
 * @param    LCFG field
 * @param    Returned corresponding LCFG value
 * @retval   LT_APP_ERROR_CODE_t
 *****************************************************************************/
LT_APP_ERROR_CODE_t lt_app_read_lcfg(uint8_t index, uint16_t *value) {
  if (index < LT_APP_LCFG_MAX) {
    switch (index) {
    case LT_APP_LCFG_ONWR_TIME:
      *value = lt_app_cfg.onWristTimeThreshold;
      break;
    case LT_APP_LCFG_OFFWR_TIME:
      *value = lt_app_cfg.offWristTimeThreshold;
      break;
    case LT_APP_LCFG_AIR_CAP_VAL:
      *value = lt_app_cfg.airCapVal;
      break;
    case LT_APP_LCFG_SKIN_CAP_VAL:
      *value = lt_app_cfg.skinCapVal;
      break;
    }
    return LT_APP_SUCCESS;
  }
  return LT_APP_ERROR;
}

/*!
 ****************************************************************************
 *@brief      LT app lcfg Read/write configuration options
 *@param      pPkt: pointer to the packet structure
 *@return     m2m2_hdr_t
 *****************************************************************************/
static m2m2_hdr_t *lt_app_lcfg_access(m2m2_hdr_t *p_pkt) {
  M2M2_APP_COMMON_STATUS_ENUM_t status = M2M2_APP_COMMON_STATUS_ERROR;
  PYLD_CST(p_pkt, lt_app_lcfg_op_hdr_t, p_in_payload);
  /* Allocate a response packet with space for the correct number of operations
   */
  PKT_MALLOC(p_resp_pkt, lt_app_lcfg_op_hdr_t,
      p_in_payload->num_ops * sizeof(p_in_payload->ops[0]));
  if (NULL != p_resp_pkt) {
    PYLD_CST(p_resp_pkt, lt_app_lcfg_op_hdr_t, p_resp_payload);
    uint16_t reg_data = 0;

    switch (p_in_payload->command) {
    case M2M2_APP_COMMON_CMD_READ_LCFG_REQ:
      for (int i = 0; i < p_in_payload->num_ops; i++) {
        if (lt_app_read_lcfg(p_in_payload->ops[i].field, &reg_data) == LT_APP_SUCCESS) {
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
        if (lt_app_write_lcfg(p_in_payload->ops[i].field,
                p_in_payload->ops[i].value) == LT_APP_SUCCESS) {
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

/*!
 ****************************************************************************
 *@brief      LT app lcfg Read/write configuration options
 *@param      pPkt: pointer to the packet structure
 *@return     m2m2_hdr_t
 ******************************************************************************/
static m2m2_hdr_t *lt_app_read_ch2_cap(m2m2_hdr_t *p_pkt) {
  M2M2_APP_COMMON_STATUS_ENUM_t status = M2M2_APP_COMMON_STATUS_ERROR;
  /* Allocate a response packet with space for the correct number of operations
   */
  PKT_MALLOC(p_resp_pkt, lt_app_rd_ch2_cap, 0);
  if (NULL != p_resp_pkt) {
    PYLD_CST(p_resp_pkt, lt_app_rd_ch2_cap, p_resp_payload);
    uint16_t capVal = 0;

    //if(gsLowTouchInitFlag)
    //{
      capVal = AD7156_ReadChannelCap(2); // unit in uF
      status = M2M2_APP_COMMON_STATUS_OK;
    //} else {
    //  status = M2M2_APP_COMMON_STATUS_ERROR;
    //}

    p_resp_payload->command = M2M2_LT_COMMAND_RD_CH2_CAP_RESP;
    p_resp_payload->status = status;
    p_resp_payload->capVal = capVal;
    p_resp_pkt->dest = p_pkt->src;
    p_resp_pkt->src = p_pkt->dest;
  }
  return p_resp_pkt;
}

#ifdef ENABLE_LT_TEST_PAGE
/* Variable which holds the status of LT logging in progress or not */
extern uint8_t gLowTouchRunning;
/**
 * @brief Funtion to take LT specific variables, to be shown from LT test page
 *
 * @details This function fills the string input arguments, with LT status
 * variables, to be displayed from LT test page
 *
 * @param str0, str1, str2 - string input arguments, to which LT status gets
 * copied to
 * @retval   None
 */
void lt_disp_str(char *str0, char *str1, char *str2) {
  sprintf(str0, "OnC:%04d OffC:%04d", g_onWrCapValue, g_offWrCapValue);
  sprintf(str1, "Int:%d Val:%d Cap:%04d", gsLowTouchAd7156IntCount,
      gsLowTouchAd7156IntValue, gsLowTouchAd7156CapVal);
  (eCurDetection_State == OFF_WRIST)
      ? sprintf(str2, "OFF_WR %d", gLowTouchRunning)
      : sprintf(str2, "ON_WR %d", gLowTouchRunning);
}
#endif

/** @brief    Low Touch Enable Function
 * @details  Sets/Resets the Low touch enable flag; which activates/deactivates
 * the touch sensor and touch detection mechanism
 * @param    True --> Sets the flag
 *           False --> Resets the flag
 * @retval   None
 */
int EnableLowTouchDetection(bool bFlag) {
  uint32_t ret;
  gEnableLowTouchDetection = bFlag;
  if (bFlag) {
    NRF_LOG_INFO("Enabling LT app");
    ret = low_touch_init();
  } else {
    NRF_LOG_INFO("Disabling LT app");
    ret = low_touch_deinit();
  }
  return ret;
}

/** @brief   Low Touch Application Initialization status
 * @details  Check if LT application is enabled or disabled
 * @param    None
 * @retval   0  --> LT app not enabled 1 --> LT app already enabled
 */
int get_lt_app_status() {
  return gsLowTouchInitFlag;
}

/** @brief   Low Touch Initialization
 * @details  Register handle for low touch application from AD7156 for wrist
 * Touch detection
 * @param    None
 * @retval   0  --> success 1 --> fail(returned when it has been initialised already)
 */
int low_touch_init() {
  uint16_t capVal;
  if (!gsLowTouchInitFlag) {
    init_lt_app_lcfg();
    Register_out2_pin_detect_func(out2_pin_detect);
    bottom_touch_func_set(1);
    gsLowTouchInitFlag = 1;

    //Detect initial Wrist status
    capVal = AD7156_ReadChannelCap(2); // unit in uF
    /*Its seen that an offset of 10 could come in the capVal read*/
    if((capVal-10) <= lt_app_cfg.skinCapVal)
    {
      NRF_LOG_INFO("Init:Resetting timer, ON wrist detected.");
      LowTouchResetTimer();
      eDetection_State = ON_WRIST; /*watch is on wrist interrupt detected*/
      NRF_LOG_INFO("ON wrist detected. cap2=%d", capVal);
      g_onWrCapValue = capVal;
    }
    else if(capVal > lt_app_cfg.skinCapVal &&  capVal <= lt_app_cfg.airCapVal)
    {
      NRF_LOG_INFO("Init: OFF wrist detected.");
      eDetection_State = OFF_WRIST; /*watch is off wrist interrupt detected*/
      NRF_LOG_INFO("OFF wrist detected. cap2=%d", capVal);
      g_offWrCapValue = capVal;
    }
    else
      eDetection_State = OFF_WRIST; /*watch is off wrist interrupt detected*/
    return 0;
  }
  return 1;
}

/** @brief    Low Touch Deinitialization
 * @details  Unregister handle for low touch application from AD7156 for wrist
 * Touch detection; stop app timers
 * @param    None
 * @retval   0  --> success 1 --> fail(returned when it has been de-initialised already)
 */
int low_touch_deinit() {
  if (gsLowTouchInitFlag) {
    bottom_touch_func_set(0);
    Unregister_out2_pin_detect_func(out2_pin_detect);
    gsLowTouchAd7156IntFlag = 0;
    gsLowTouchAd7156IntCount = 0;
    gsLowTouchAd7156IntValue = 0;
    gsLowTouchAd7156CapVal = 0;
    g_onWrCapValue = TOUCH_DETECTION_THRESHOLD;
    g_offWrCapValue = TOUCH_DETECTION_THRESHOLD;
    lt_on_timer_stop();
    lt_off_timer_stop();
    gLowTouchTimerUp = 0;
    eDetection_State = OFF_WRIST, eCurDetection_State = OFF_WRIST;
    /*stop low touch logging*/
    SendStopLowTouchLogReq();
    gsLowTouchInitFlag = 0;

    return 0;
  }
  return 1;
}

/** @brief    Reset the LT ON/OFF timers
 * @param    None
 * @retval   None
 */
static void LowTouchResetTimer() {
  lt_on_timer_stop();
  lt_on_timer_start();
  lt_off_timer_stop();
  lt_off_timer_start();
  gLowTouchTimerUp = 0;
}

/** @brief   LT Timer event function, which is executed from the LT task.
 * @details  LT ON/OFF wrist detection timer expiry is handled.
 *           Based on detected state, LT start / stop commands are sent.
 * @param    None
 * @retval   0 --> success
 */
int LowTouchTimerEvent() {
  if (gEnableLowTouchDetection) {
    if (gLowTouchTimerUp) {
      /*Timer is up; check the new detected state*/
      gLowTouchTimerUp = 0;

#ifndef DVT
      if (!gsLowTouchAd7156IntValue) // value=0 implies ON or touch
      {
#endif
        if (eDetection_State == ON_WRIST) {
          /*watch on wrist event deteced*/
          if (eCurDetection_State != eDetection_State) {
            OnWristEventDetecedCnt++;
            SendStartLowTouchLogReq(); /*start low touch logging*/
            NRF_LOG_INFO("send start low touch log req");
            eCurDetection_State =
                eDetection_State; /*Update the current detected state*/
          }
        }
        // eDetection_State = ON_WRIST;
#ifndef DVT
      } else {
#endif
        if (eDetection_State == OFF_WRIST) {
          /*watch off wrist event deteced*/
          if (eCurDetection_State != eDetection_State) {
            OffWristEventDetecedCnt++;
            SendStopLowTouchLogReq(); /*stop low touch logging*/
            NRF_LOG_INFO("send stop low touch log req");
            eCurDetection_State =
                eDetection_State; /*Update the current detected state*/
          }
        }
        // eDetection_State = OFF_WRIST;
#ifndef DVT
      }
#endif
    } // if(gLowTouchTimerUp)
  }   // if (gEnableLowTouchDetection)
  return 0;
}

/** @brief    LT event function, which is executed from the LT task.
 * @details  Wrist ON/OFF Detection state machine logic gets executed here.
 *           Also LT ON/OFF detection timer expiry handling
 *           AD7156 configuration for LT application:
 *           -----------------------------------------
 *           Adaptive Threshold,In-Window Threshold Mode,with sensitivity of
 * 25uF Output2 from channel2, becomes Active(value:1) when,
 * Data > Avg - Sensitivity
 * Data < Avg + Sensitivity
 * ON_WRIST/OFF_WRIST is detected based on the state machine vairable
 * eDetection_State and comparison of g_onWrCapValue with gsLowTouchAd7156CapVal;
 * g_offWrCapValue is maintained for debugging purpose only
 *
 *           -----------------------------------------
 * @param    None
 * @retval   0 --> success
 */
int LowTouchSensorEvent() {
  if (gEnableLowTouchDetection) {
    if (gsLowTouchAd7156IntFlag) {
      /*AD7156 triggered an interrupt*/
      gsLowTouchAd7156IntFlag = 0;
      gsLowTouchAd7156CapVal = AD7156_ReadChannelCap(2); // unit in uF
#ifndef DVT
      if (!gsLowTouchAd7156IntValue) // value=0 implies ON or touch
      {
#endif
#ifdef DVT
        /* Check if curr Cap value detected is less than or equal to last ONWr
           Cap value, and that the last Detection state was OFFWrist*/
        if (gsLowTouchAd7156CapVal-(gn_ch2_sensitivity/2) <= g_onWrCapValue &&
        //if (gsLowTouchAd7156CapVal-10 <= g_onWrCapValue &&
            (eDetection_State != ON_WRIST))
#else
      if (gsLowTouchAd7156CapVal > TOUCH_DETECTION_THRESHOLD)
#endif
        {
          g_onWrCapValue = gsLowTouchAd7156CapVal;
          /*start the timer to see if the detected state is consistent for 5
           * more seconds*/
          NRF_LOG_INFO("Resetting timer, ON wrist detected.");
          LowTouchResetTimer();
          eDetection_State = ON_WRIST; /*watch is on wrist interrupt detected*/
          NRF_LOG_INFO("ON wrist detected. cap2=%d", gsLowTouchAd7156CapVal);
          return 0;
        }
#ifndef DVT
      } else {
#endif
#ifdef DVT
        /* Check if curr Cap value detected is greater than last ONWr Cap value
         * and that the last Detection state was ONWrist*/
        if (gsLowTouchAd7156CapVal > g_onWrCapValue &&
            (eDetection_State != OFF_WRIST))
#else
      if (gsLowTouchAd7156CapVal < TOUCH_DETECTION_THRESHOLD)
#endif
        {
          g_offWrCapValue = gsLowTouchAd7156CapVal;
          gn_ch2_sensitivity = g_offWrCapValue - g_onWrCapValue;
          /*start the timer to see if the detected state is consistent for 3
           * more seconds*/
          NRF_LOG_INFO("Resetting timer, OFF wrist detected.");
          LowTouchResetTimer();
          eDetection_State =
              OFF_WRIST; /*watch is off wrist interrupt detected*/
          NRF_LOG_INFO("OFF wrist detected. cap2=%d", gsLowTouchAd7156CapVal);
          return 0;
        }

        /* To adjust for Capcitance changes and interrupt triggers detected
         within OFF_WRIST conditions */
        if (gsLowTouchAd7156CapVal > g_onWrCapValue &&
            (eDetection_State == OFF_WRIST)) {
          g_offWrCapValue = gsLowTouchAd7156CapVal;
          return 0;
        }
#ifndef DVT
      }
#endif
    } // if(gsLowTouchAd7156IntFlag)
  }   // if (gEnableLowTouchDetection)
  return 0;
}

/**
 * @brief  Sends Low touch log start request to the system task
 * @param  None
 * @return None
 */
static void SendStartLowTouchLogReq() {
  m2m2_hdr_t *response_mail = NULL;
  ADI_OSAL_STATUS err;

  response_mail =
      post_office_create_msg(M2M2_HEADER_SZ + sizeof(_m2m2_app_common_cmd_t));

  if (response_mail != NULL) {
    _m2m2_app_common_cmd_t *start_low_touch_log_req =
        (_m2m2_app_common_cmd_t *)&response_mail->data[0];

    /* send request packet */
    response_mail->src = M2M2_ADDR_UNDEFINED;
    response_mail->dest = M2M2_ADDR_SYS_PM;
    response_mail->checksum = 0x0000;

    start_low_touch_log_req->command =
        M2M2_PM_SYS_COMMAND_ENABLE_USER_CONFIG_LOG_REQ;
    start_low_touch_log_req->status = M2M2_APP_COMMON_STATUS_OK;
    post_office_send(response_mail, &err);
  }
}

/**
 * @brief  Sends Low touch log stop request to the system task
 * @param  None
 * @return None
 */
static void SendStopLowTouchLogReq() {
  m2m2_hdr_t *response_mail = NULL;
  ADI_OSAL_STATUS err;

  response_mail =
      post_office_create_msg(M2M2_HEADER_SZ + sizeof(_m2m2_app_common_cmd_t));

  if (response_mail != NULL) {
    _m2m2_app_common_cmd_t *stop_low_touch_log_req =
        (_m2m2_app_common_cmd_t *)&response_mail->data[0];

    /* send request packet */
    response_mail->src = M2M2_ADDR_UNDEFINED;
    response_mail->dest = M2M2_ADDR_SYS_PM;
    response_mail->checksum = 0x0000;

    stop_low_touch_log_req->command =
        M2M2_PM_SYS_COMMAND_DISABLE_USER_CONFIG_LOG_REQ;
    stop_low_touch_log_req->status = M2M2_APP_COMMON_STATUS_OK;
    post_office_send(response_mail, &err);
  }
}

/**
 * @brief  Callback for Ad7156 Interrupt
 * @param  value - gpio value at the time interrupt handler was called
 * @return none
 */
static void LowTouchAd7156IntCallback(uint8_t value) {
  gsLowTouchAd7156IntFlag = 1;
  gsLowTouchAd7156IntCount++;
  /* either 1 or 0 ie Touch or No Touch */
  gsLowTouchAd7156IntValue = value;
}

/**@brief Function for handling the low touch on wrist detection timer time-out.
 *
 * @details This function will be called each time the ON-wrist detection timer
 * expires.
 *
 * @param[in] p_context - unused
 */
static void lt_on_wrist_timeout_handler(void *p_context) {
  NRF_LOG_INFO("LT ON wrist Timer expiry..");
  gLowTouchTimerUp = 1;
  //lt_on_timer_stop();
  g_lt_task_timeout = ADI_OSAL_TIMEOUT_NONE;
  adi_osal_ThreadResumeFromISR(gh_lt_task_handler);
  //adi_osal_SemPost(lt_task_evt_sem);
}

/**@brief Function for handling the low touch off wrist detection timer
 * time-out.
 *
 * @details This function will be called each time the off-wrist detction timer
 * expires
 *
 * @param[in]  p_context - unused
 */
static void lt_off_wrist_timeout_handler(void *p_context) {
  NRF_LOG_INFO("LT OFF wrist Timer expiry..");
  gLowTouchTimerUp = 1;
  //lt_off_timer_stop();
  g_lt_task_timeout = ADI_OSAL_TIMEOUT_NONE;
  adi_osal_ThreadResumeFromISR(gh_lt_task_handler);
  //adi_osal_SemPost(lt_task_evt_sem);
}

/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module. This creates and starts application
 * timers for ON-wrist detection.
 */
static void on_timer_init() {
  ret_code_t err_code;

  // Create timers
  err_code = app_timer_create(
      &m_lt_on_timer_id, APP_TIMER_MODE_SINGLE_SHOT, lt_on_wrist_timeout_handler);

  APP_ERROR_CHECK(err_code);

  NRF_LOG_INFO("LT ON wrist Timer created");
}

/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module. This creates and starts application
 * timers for OFF-wrist detection.
 */
static void off_timer_init() {
  ret_code_t err_code;

  // Create timers
  err_code = app_timer_create(&m_lt_off_timer_id, APP_TIMER_MODE_SINGLE_SHOT,
      lt_off_wrist_timeout_handler);

  APP_ERROR_CHECK(err_code);

  NRF_LOG_INFO("LT OFF wrist Timer created");
}

/**@brief   Function for starting ON-wrist application timers.
 * @details Timers are run after the scheduler has started.
 */
static void lt_on_timer_start(void) {
  uint32_t time = lt_app_cfg.onWristTimeThreshold;

  // Start repeated timer
  ret_code_t err_code = app_timer_start(
      m_lt_on_timer_id, APP_TIMER_TICKS(time), NULL);
  APP_ERROR_CHECK(err_code);
}

/**@brief   Function for stopping ON-wrist application timers.
 */
static void lt_on_timer_stop(void) {
  // Stop the repeated timer
  ret_code_t err_code = app_timer_stop(m_lt_on_timer_id);
  APP_ERROR_CHECK(err_code);
}

/**@brief   Function for starting OOF-wrist application timers.
 * @details Timers are run after the scheduler has started.
 */
static void lt_off_timer_start(void) {
  uint32_t time = lt_app_cfg.offWristTimeThreshold;

  // Start repeated timer
  ret_code_t err_code = app_timer_start(
      m_lt_off_timer_id, APP_TIMER_TICKS(time), NULL);
  APP_ERROR_CHECK(err_code);
}

/**@brief   Function for stopping OFF-wrist application timers.
 */
static void lt_off_timer_stop(void) {
  // Stop the repeated timer
  ret_code_t err_code = app_timer_stop(m_lt_off_timer_id);
  APP_ERROR_CHECK(err_code);
}

/** @brief Low touch task init function. */
void low_touch_task_init(void) {
  ADI_OSAL_STATUS eOsStatus;

  /* Create LT app thread */
  g_lt_task_attributes.pThreadFunc = lt_task;
  g_lt_task_attributes.nPriority = APP_OS_CFG_LT_APP_TASK_PRIO;
  g_lt_task_attributes.pStackBase = &ga_lt_task_stack[0];
  g_lt_task_attributes.nStackSize = APP_OS_CFG_LT_APP_TASK_STK_SIZE;
  g_lt_task_attributes.pTaskAttrParam = NULL;
  g_lt_task_attributes.szThreadName = "low_touch";
  g_lt_task_attributes.pThreadTcb = &g_lt_task_tcb;

  eOsStatus = adi_osal_MsgQueueCreate(&gh_lt_task_msg_queue, NULL, 9);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
    Debug_Handler();
  } else {
    update_task_queue_list(APP_OS_CFG_LT_TASK_INDEX, gh_lt_task_msg_queue);
  }

  eOsStatus =
      adi_osal_ThreadCreateStatic(&gh_lt_task_handler, &g_lt_task_attributes);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
    Debug_Handler();
  }

  //adi_osal_SemCreate(&lt_task_evt_sem, 0);
  // touch_detect_init();

  // ON-OFF detection Timer creation
  on_timer_init();
  off_timer_init();
}

/** @brief Function to enable/disable the bottom touch channel detection in
 * AD7156
 * @param  en - 1 -> enable 0 -> disable
 * @return none
 */
void bottom_touch_func_set(uint8_t en) {
  Ad7156_detect_set(AD7156_CHANNEL2, en);
}

/** @brief Function handler which gets registered to AD7156 OUT2 gpio event
 * @param  value - gpio value which gets detected upon gpio event
 * @return none
 */
static void out2_pin_detect(uint8_t value) {
  LowTouchAd7156IntCallback(value);
  g_lt_task_timeout = ADI_OSAL_TIMEOUT_NONE;
  adi_osal_ThreadResumeFromISR(gh_lt_task_handler);
  //adi_osal_SemPost(lt_task_evt_sem);
}

/** @brief LT task function
 * @param  arg - unused
 * @return none
 */
static void lt_task(void *arg) {
  m2m2_hdr_t *p_in_pkt = NULL;
  m2m2_hdr_t *p_out_pkt = NULL;
  ADI_OSAL_STATUS err;
  UNUSED_PARAMETER(arg);

  touch_detect_init();

  /* Wait for FS task FindConfigFile() completes */
  adi_osal_ThreadSuspend(NULL);
  /* Wait for FS task FindConfigFile() completes */
  //adi_osal_SemPend(lt_task_evt_sem, ADI_OSAL_TIMEOUT_FOREVER);

  /* Check & Load from DCB LT lcfg if lcfg is available in wrist_detect_dcb
     so that lcfg parameters when read from the Tool, gives values correctly
  */
  if (wrist_detect_get_dcb_present_flag()) {
    lt_app_lcfg_set_from_dcb();
  }
  else
    lt_app_lcfg_set_fw_default();

  // On Bootup Enable LT app if LT cfg files are present
  if (gen_blk_get_dcb_present_flag() || gsCfgFileFoundFlag)
    EnableLowTouchDetection(true);

// Test DCB/NAND cfg working without ON/OFF Wrist events
#ifdef TEST_LT_APP_WITHOUT_EVENTS
  static volatile uint8_t gb_trig_event = 0;

  while (1) {
    // adi_osal_SemPend(lt_task_evt_sem, ADI_OSAL_TIMEOUT_FOREVER);
    // p_in_pkt = post_office_get(ADI_OSAL_TIMEOUT_NONE,
    // APP_OS_CFG_LT_TASK_INDEX);
    p_in_pkt = post_office_get(5000, APP_OS_CFG_LT_TASK_INDEX);

    // We got an m2m2 message from the queue, process it.
    if (p_in_pkt != NULL) {
      // We got an m2m2 message from the queue, process it.
      PYLD_CST(p_in_pkt, _m2m2_app_common_cmd_t, p_in_cmd);
      // Look up the appropriate function to call in the function table
      for (int i = 0; i < LT_APP_ROUTING_TBL_SZ; i++) {
        if (lt_app_routing_table[i].command == p_in_cmd->command) {
          p_out_pkt = lt_app_routing_table[i].cb_handler(p_in_pkt);
          break;
        }
      }
      post_office_consume_msg(p_in_pkt);
      if (p_out_pkt != NULL) {
        post_office_send(p_out_pkt, &err);
      }
    } else {
      if (eCurDetection_State == ON_WRIST && gb_trig_event) {
        OnWristEventDetecedCnt++;
        SendStartLowTouchLogReq(); /*start low touch logging*/
        NRF_LOG_INFO("send start low touch log req");
        gb_trig_event = 0;
      }
      if (eCurDetection_State == OFF_WRIST && gb_trig_event) {
        OffWristEventDetecedCnt++;
        SendStopLowTouchLogReq(); /*stop low touch logging*/
        NRF_LOG_INFO("send stop low touch log req");
        gb_trig_event = 0;
      }
    } // else
  }
#else

  while (1) {
    //adi_osal_ThreadSuspend(NULL);
    //adi_osal_SemPend(lt_task_evt_sem, ADI_OSAL_TIMEOUT_FOREVER);

    //p_in_pkt = post_office_get(ADI_OSAL_TIMEOUT_NONE, APP_OS_CFG_LT_TASK_INDEX); //LT events
    //p_in_pkt = post_office_get(ADI_OSAL_TIMEOUT_FOREVER, APP_OS_CFG_LT_TASK_INDEX); //m2m2 pkt
    p_in_pkt = post_office_get(g_lt_task_timeout, APP_OS_CFG_LT_TASK_INDEX);

    // We got an m2m2 message from the queue, process it.
    if (p_in_pkt != NULL) {
      // We got an m2m2 message from the queue, process it.
      PYLD_CST(p_in_pkt, _m2m2_app_common_cmd_t, p_in_cmd);
      // Look up the appropriate function to call in the function table
      for (int i = 0; i < LT_APP_ROUTING_TBL_SZ; i++) {
        if (lt_app_routing_table[i].command == p_in_cmd->command) {
          p_out_pkt = lt_app_routing_table[i].cb_handler(p_in_pkt);
          break;
        }
      }
      post_office_consume_msg(p_in_pkt);
      if (p_out_pkt != NULL) {
        post_office_send(p_out_pkt, &err);
      }
    } else {
      LowTouchTimerEvent();
      LowTouchSensorEvent();
    }
    adi_osal_ThreadSuspend(NULL);
  }
#endif
}

/**
 * @brief  Function to send PO pkts to LT task
 * @param  p_pkt m2m2 packet to be send to LT task
 * @return None
 */
void send_message_lt_task(m2m2_hdr_t *p_pkt) {
  ADI_OSAL_STATUS osal_result;
  osal_result = adi_osal_MsgQueuePost(gh_lt_task_msg_queue, p_pkt);
  if (osal_result != ADI_OSAL_SUCCESS)
    post_office_consume_msg(p_pkt);
  g_lt_task_timeout = ADI_OSAL_TIMEOUT_FOREVER;
  vTaskResume((TaskHandle_t)gh_lt_task_handler);
  //adi_osal_SemPost(lt_task_evt_sem);
}

/**
 * @brief  Function to set the LT app lcfg to default fw values
 * @param  None
 * @return None
 */
void lt_app_lcfg_set_fw_default()
{
    lt_app_cfg.onWristTimeThreshold  = LT_ON_WRIST_TIME_INTERVAL;
    lt_app_cfg.offWristTimeThreshold = LT_OFF_WRIST_TIME_INTERVAL;
    lt_app_cfg.airCapVal  = LT_AIR_CAP_VAL;
    lt_app_cfg.skinCapVal = LT_SKIN_CAP_VAL;
}

/**
 * @brief  Function to set the LT app lcfg to from wrist detect DCB
 * @param  None
 * @return None
 */
void lt_app_lcfg_set_from_dcb()
{
  load_wrist_detect_dcb();
  copy_lt_lcfg_from_wrist_detect_dcb(&lt_app_cfg);
}
#ifdef DCB
/**
 * @brief  Function which handles the m2m2 command to do gen_blk_DCB read
 * @param  p_pkt m2m2 REQ packet
 * @return m2m2 RESP packet
 */
static m2m2_hdr_t *gen_blk_dcb_command_read_config(m2m2_hdr_t *p_pkt) {
  uint16_t r_size;
  uint16_t i, p, num_of_pkts, dcbdata_index;;
  uint32_t dcbdata[MAXGENBLKDCBSIZE * 4];
  ADI_OSAL_STATUS err;

  M2M2_DCB_STATUS_ENUM_t status = M2M2_DCB_STATUS_ERR_NOT_CHKD;

  PKT_MALLOC(p_resp_pkt, m2m2_dcb_gen_blk_data_t, 0);
  if(NULL != p_resp_pkt)
  {
    // Declare a pointer to the response packet payload
    PYLD_CST(p_resp_pkt, m2m2_dcb_gen_blk_data_t, p_resp_payload);

    memset(dcbdata, 0, sizeof(dcbdata));
    r_size =
        (uint16_t)(MAXGENBLKDCBSIZE * 4); // Max words that can be read from FDS

    if (read_gen_blk_dcb(&dcbdata[0], &r_size) == GEN_BLK_DCB_STATUS_OK) {
      num_of_pkts = (r_size/MAXGENBLKDCBSIZE) +
                    ( (r_size%MAXGENBLKDCBSIZE) ? 1 : 0 );
      dcbdata_index = 0;
      for( p=0; p<num_of_pkts; p++ )
      {
      	p_resp_payload->command = M2M2_DCB_COMMAND_READ_CONFIG_RESP;
      	p_resp_payload->status = M2M2_DCB_STATUS_OK;
      	p_resp_payload->size = (p != num_of_pkts-1) ? MAXGENBLKDCBSIZE :
                                                     (r_size%MAXGENBLKDCBSIZE);
      	p_resp_payload->num_of_pkts = num_of_pkts;
      	for (i = 0; i < p_resp_payload->size; i++)
      		p_resp_payload->dcbdata[i] = dcbdata[dcbdata_index++];
      	p_resp_pkt->src = p_pkt->dest;
      	p_resp_pkt->dest = p_pkt->src;
      	NRF_LOG_INFO("%d pkt's sz->%d", (p+1), p_resp_payload->size);

      	if(p != num_of_pkts-1)
      	{
      	  post_office_send(p_resp_pkt, &err);

      	  /*Delay is required b/w two pkts send, this was increased from 20,
      	  kept initially to 60, when back to back write/read/delete DCB
      	  tests were seen to fail
      	  */
      	  MCU_HAL_Delay(60);

      	  PKT_MALLOC(p_resp_pkt, m2m2_dcb_gen_blk_data_t, 0);
      	  if(NULL != p_resp_pkt)
      	  {
      	  	// Declare a pointer to the response packet payload
      	  	PYLD_CST(p_resp_pkt, m2m2_dcb_gen_blk_data_t, p_resp_payload);

      	  }//if(NULL != p_resp_pkt)
      	  else
      	  {
      	  	return NULL;
      	  }
      	}
      }//for loop
    } else {
      p_resp_payload->size = 0;
      p_resp_payload->num_of_pkts = 0;
      status = M2M2_DCB_STATUS_ERR_ARGS;
    }

    p_resp_payload->status = status;
    p_resp_payload->command = M2M2_DCB_COMMAND_READ_CONFIG_RESP;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  }//if(NULL != p_resp_pkt)
  return p_resp_pkt;
}

/* Reuse the dcbdata[] buffer used in adpd4000_task.c for
 * adpd_dcb_command_write_config() api */
/* Buffer to be shared task( General Blk DCB & ADPD4000 DCB */
extern uint32_t dcbdata[MAXADPD4000DCBSIZE * 4];
/**
 * @brief  Function which handles the m2m2 command to do gen_blk_DCB write
 * @param  p_pkt m2m2 REQ packet
 * @return m2m2 RESP packet
 */
static m2m2_hdr_t *gen_blk_dcb_command_write_config(m2m2_hdr_t *p_pkt) {
  M2M2_DCB_STATUS_ENUM_t status = M2M2_DCB_STATUS_ERR_NOT_CHKD;
  // static uint32_t  dcbdata[MAXGENBLKDCBSIZE*2];
  static uint16_t i = 0;
  static uint16_t num_of_pkts = 0;
  uint16_t j;

  // Declare a pointer to access the input packet payload
  PYLD_CST(p_pkt, m2m2_dcb_gen_blk_data_t, p_in_payload);
  PKT_MALLOC(p_resp_pkt, m2m2_dcb_gen_blk_data_t, 0);
  if(NULL != p_resp_pkt)
  {
    // Declare a pointer to the response packet payload
    PYLD_CST(p_resp_pkt, m2m2_dcb_gen_blk_data_t, p_resp_payload);

    // Maximum 4 packets can be written to GEN_BLK_DCB
    if (p_in_payload->num_of_pkts >= 1 && p_in_payload->num_of_pkts <= 4) {
      num_of_pkts += 1;
      for (j = 0; j < p_in_payload->size; j++)
        dcbdata[i++] = p_in_payload->dcbdata[j];
      NRF_LOG_INFO("Wr DCB:pkt sz->%d, arr index=%d", p_in_payload->size, i);
      if (num_of_pkts == p_in_payload->num_of_pkts) {
        if (write_gen_blk_dcb(&dcbdata[0], i) == GEN_BLK_DCB_STATUS_OK) {
          gen_blk_set_dcb_present_flag(true);
          NRF_LOG_INFO("LT gen blk DCB written");
          EnableLowTouchDetection(true);
          status = M2M2_DCB_STATUS_OK;
        } else {
          status = M2M2_DCB_STATUS_ERR_ARGS;
        }
        num_of_pkts = 0;
        i = 0;
      } else if (num_of_pkts < p_in_payload->num_of_pkts) {
        status = M2M2_DCB_STATUS_OK;
      } else {
          /* Unexpected, some error condition, clear the static variables kept */
          num_of_pkts = 0;
          i = 0;
          status = M2M2_DCB_STATUS_ERR_ARGS;
      }
    }
    else
    {
      /* Unexpected, some error condition, clear the static variables kept */
      num_of_pkts = 0;
      i = 0;
      status = M2M2_DCB_STATUS_ERR_ARGS;
    }

    p_resp_payload->status = status;
    p_resp_payload->command = M2M2_DCB_COMMAND_WRITE_CONFIG_RESP;
    p_resp_payload->size = 0;
    p_resp_payload->num_of_pkts = p_in_payload->num_of_pkts;
    for(uint16_t k=0; k<MAXGENBLKDCBSIZE; k++)
      p_resp_payload->dcbdata[k] = 0;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  }//if(NULL != p_resp_pkt)
  return p_resp_pkt;
}

/**
 * @brief  Function which handles the m2m2 command to do gen_blk_DCB delete
 * @param  p_pkt m2m2 REQ packet
 * @return m2m2 RESP packet
 */
static m2m2_hdr_t *gen_blk_dcb_command_delete_config(m2m2_hdr_t *p_pkt) {
  M2M2_DCB_STATUS_ENUM_t status = M2M2_DCB_STATUS_ERR_NOT_CHKD;

  PKT_MALLOC(p_resp_pkt, m2m2_dcb_gen_blk_data_t, 0);
  if(NULL != p_resp_pkt)
  {
    // Declare a pointer to the response packet payload
    PYLD_CST(p_resp_pkt, m2m2_dcb_gen_blk_data_t, p_resp_payload);

    if (delete_gen_blk_dcb() == GEN_BLK_DCB_STATUS_OK) {
      gen_blk_set_dcb_present_flag(false);
      SetCfgCopyAvailableFlag(false); /*update the config file copy availability
                                         flag upon LT gen blk DCB deletion */
      find_low_touch_DCB();//Update gbDCBCfgFoundFlag in system task
      NRF_LOG_INFO("LT gen blk DCB deleted");
      if (!gen_blk_get_dcb_present_flag() && !gsCfgFileFoundFlag)
        EnableLowTouchDetection(false);
      status = M2M2_DCB_STATUS_OK;
    } else {
      status = M2M2_DCB_STATUS_ERR_ARGS;
    }

    p_resp_payload->status = status;
    p_resp_payload->command = M2M2_DCB_COMMAND_ERASE_CONFIG_RESP;
    p_resp_payload->size = 0;
    for(uint16_t i=0; i<MAXGENBLKDCBSIZE; i++)
      p_resp_payload->dcbdata[i] = 0;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  }//if(NULL != p_resp_pkt)
  return p_resp_pkt;
}
#endif // DCB

#endif