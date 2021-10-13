/**
 ***************************************************************************
 * @file    dcb_user0_block.c
 * @author  ADI Team
 * @version V0.0.1
 * @date    22-April-2020
 * @brief   USER0 Block DCB configuration file
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
#ifdef DCB
//#ifdef LOW_TOUCH_FEATURE
#include "nrf_log_ctrl.h"
/* User0 DCB Block Module Log settings */
#define NRF_LOG_MODULE_NAME USER0_DCB_BLK

#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

#include "adi_dcb_config.h"
#include "dcb_user0_block.h"
#include "dcb_interface.h"
#include <stdlib.h>

/**< Flag variable to check if user0_blk DCB entry is present or not. This is
 * updated for every DCB Write/Read and on bootup */
static volatile bool g_usr0_cfg_dcb_present = false;

/**< RAM buffer which holds the user0 block DCB contents */
static uint32_t g_current_user0_blk_dcb[MAXUSER0BLKDCBSIZE] = {'\0'};

// ================== lt_app_lcfg DCB Section ====================== //

/**
 * @brief    Clear the RAM buffer to hold lt_app_lcfg DCB contents
 * @retval   None
 */
void user0_config_app_lcfg_content_clear(void) {
  memset(&g_current_user0_blk_dcb[0], 0xFF, sizeof(g_current_user0_blk_dcb));
}
/**
 * @brief    Load user0_config_app_lcfg block Default DCB configuration
 * @retval   Status: OK if DCB content is read
 *                 : ERR if DCB read failed
 */
USER0_BLK_DCB_STATUS_t load_user0_config_app_lcfg_dcb() {
  // Load user0_config_app_lcfg DCB for LT configs
  NRF_LOG_INFO("Load user0_config_app_lcfg DCB for LT lcfg configs");
  uint16_t dcb_sz = (uint16_t)(MAXUSER0BLKDCBSIZE);
  user0_config_app_lcfg_content_clear();
  if (read_user0_blk_dcb(&g_current_user0_blk_dcb[0], &dcb_sz) == USER0_BLK_DCB_STATUS_OK) {
    return USER0_BLK_DCB_STATUS_OK;
  } else {
    NRF_LOG_INFO("user0_config_app_lcfg DCB Read failed!");
    return USER0_BLK_DCB_STATUS_ERR;
  }
}

/**
 * @brief  Copy the contents from RAM buffer which has the DCB contents to the
 * location passed
 * @param  p_lt_app_cfg - pointer location to where lcfg parameters are copied
 * @retval   None
 */
USER0_BLK_DCB_STATUS_t copy_user0_config_from_user0_blk_dcb(user0_config_app_lcfg_type_t *p_user0_config_app_lcfg) {
  p_user0_config_app_lcfg->agc_up_th            = (uint8_t)g_current_user0_blk_dcb[USER0_CONFIG_LCFG_AGC_UP_TH];
  p_user0_config_app_lcfg->agc_low_th           = (uint8_t)g_current_user0_blk_dcb[USER0_CONFIG_LCFG_AGC_LOW_TH];
  p_user0_config_app_lcfg->adv_timeout_monitor  = (uint16_t)g_current_user0_blk_dcb[USER0_CONFIG_LCFG_ADV_TIMEOUT_MONITOR];
  p_user0_config_app_lcfg->hw_id                = (uint16_t)g_current_user0_blk_dcb[USER0_CONFIG_LCFG_HW_ID];
  p_user0_config_app_lcfg->exp_id               = (uint16_t)g_current_user0_blk_dcb[USER0_CONFIG_LCFG_EXP_ID];
  p_user0_config_app_lcfg->adxl_start_time      = (uint16_t)g_current_user0_blk_dcb[USER0_CONFIG_LCFG_ADXL_START_TIME];
  p_user0_config_app_lcfg->adxl_tON             = (uint16_t)g_current_user0_blk_dcb[USER0_CONFIG_LCFG_ADXL_TON];
  p_user0_config_app_lcfg->adxl_tOFF            = (uint16_t)g_current_user0_blk_dcb[USER0_CONFIG_LCFG_ADXL_TOFF];
  p_user0_config_app_lcfg->temp_start_time      = (uint16_t)g_current_user0_blk_dcb[USER0_CONFIG_LCFG_TEMP_START_TIME];
  p_user0_config_app_lcfg->temp_tON             = (uint16_t)g_current_user0_blk_dcb[USER0_CONFIG_LCFG_TEMP_TON];
  p_user0_config_app_lcfg->temp_tOFF            = (uint16_t)g_current_user0_blk_dcb[USER0_CONFIG_LCFG_TEMP_TOFF];
  p_user0_config_app_lcfg->adpd_start_time      = (uint16_t)g_current_user0_blk_dcb[USER0_CONFIG_LCFG_ADPD_START_TIME];
  p_user0_config_app_lcfg->adpd_tON             = (uint16_t)g_current_user0_blk_dcb[USER0_CONFIG_LCFG_ADPD_TON];
  p_user0_config_app_lcfg->adpd_tOFF            = (uint16_t)g_current_user0_blk_dcb[USER0_CONFIG_LCFG_ADPD_TOFF];
  p_user0_config_app_lcfg->eda_start_time       = (uint16_t)g_current_user0_blk_dcb[USER0_CONFIG_LCFG_EDA_START_TIME];
  p_user0_config_app_lcfg->eda_tON              = (uint16_t)g_current_user0_blk_dcb[USER0_CONFIG_LCFG_EDA_TON];
  p_user0_config_app_lcfg->eda_tOFF             = (uint16_t)g_current_user0_blk_dcb[USER0_CONFIG_LCFG_EDA_TOFF];
  p_user0_config_app_lcfg->sleep_min            = (uint16_t)g_current_user0_blk_dcb[USER0_CONFIG_LCFG_SLEEP_MIN];
  p_user0_config_app_lcfg->signal_threshold     = (uint32_t)g_current_user0_blk_dcb[USER0_CONFIG_LCFG_SIGNAL_THRESHOLD];

  return USER0_BLK_DCB_STATUS_OK;
}

/**@brief   Gets the entire User0 Blk DCB configuration written in flash
 *
 * @param user0_blk_dcb_data - pointer to dcb struct variable,
 * @param read_size: size of user0_blk_dcb_data array filled from FDS(length in
 * Double Word (32-bits))
 * @return return value of type USER0_BLK_DCB_STATUS_t
 */
USER0_BLK_DCB_STATUS_t read_user0_blk_dcb(
    uint32_t *user0_blk_dcb_data, uint16_t *read_size) {
  USER0_BLK_DCB_STATUS_t dcb_status = USER0_BLK_DCB_STATUS_ERR;

  if (adi_dcb_read_from_fds(
          ADI_DCB_USER0_BLOCK_IDX, user0_blk_dcb_data, read_size) == DEF_OK) {
    dcb_status = USER0_BLK_DCB_STATUS_OK;
  }
  return dcb_status;
}

/**@brief   Sets the entire User0 Blk DCB configuration in flash
 *
 * @param user0_blk_dcb_data - pointer to dcb struct variable,
 * @param in_size: size of user0_blk_dcb_data array(length in Double Word
 * (32-bits))
 * @return return value of type USER0_BLK_DCB_STATUS_t
 */
USER0_BLK_DCB_STATUS_t write_user0_blk_dcb(
    uint32_t *user0_blk_dcb_data, uint16_t in_size) {
  USER0_BLK_DCB_STATUS_t dcb_status = USER0_BLK_DCB_STATUS_ERR;

  if (adi_dcb_write_to_fds(
          ADI_DCB_USER0_BLOCK_IDX, user0_blk_dcb_data, in_size) == DEF_OK) {
    dcb_status = USER0_BLK_DCB_STATUS_OK;
  }

  return dcb_status;
}

/**
 * @brief    Delete the entire User0 blk DCB configuration in flash
 * @param    void
 * @retval   Status
 */
USER0_BLK_DCB_STATUS_t delete_user0_blk_dcb(void) {
  USER0_BLK_DCB_STATUS_t dcb_status = USER0_BLK_DCB_STATUS_ERR;

  if (adi_dcb_delete_fds_settings(ADI_DCB_USER0_BLOCK_IDX) == DEF_OK) {
    dcb_status = USER0_BLK_DCB_STATUS_OK;
  }

  return dcb_status;
}

/**
 * @brief  Function to set the global flag which holds the status for user0_blk
 * DCB presence. This gets called from DCB write/delete api
 * @param  set_flag - true/flag
 * @retval   None
 */
void user0_blk_set_dcb_present_flag(bool set_flag) {
  g_usr0_cfg_dcb_present = set_flag;
  NRF_LOG_INFO("Setting..USER0 BLK DCB present: %s",
      (g_usr0_cfg_dcb_present == true ? "TRUE" : "FALSE"));
}

/**
 * @brief  Function to get the value of global flag which holds the status for
 * user0_blk DCB presence.
 * @param  None
 * @retval   true/false
 */
bool user0_blk_get_dcb_present_flag(void) {
  NRF_LOG_INFO("USER0 BLK DCB present: %s",
      (g_usr0_cfg_dcb_present == true ? "TRUE" : "FALSE"));
  return g_usr0_cfg_dcb_present;
}

/**
 * @brief  Function to which does actual FDS read to update the global flag
 * which holds the status for user0_blk DCB presence.
 * @param  None
 * @retval   None
 */
void user0_blk_update_dcb_present_flag(void) {
  g_usr0_cfg_dcb_present = adi_dcb_check_fds_entry(ADI_DCB_USER0_BLOCK_IDX);
  NRF_LOG_INFO("Updated. USER0 BLK DCB present: %s",
      (g_usr0_cfg_dcb_present == true ? "TRUE" : "FALSE"));
}
//#endif//LOW_TOUCH_FEATURE
#endif
#endif