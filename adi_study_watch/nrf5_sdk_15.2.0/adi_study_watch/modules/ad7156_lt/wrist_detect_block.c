/**
 ***************************************************************************
 * @file    wrist_detect_block.c
 * @author  ADI Team
 * @version V0.0.1
 * @date    1-Jan-2021
 * @brief   Wrist Detect Block DCB configuration file
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
#ifdef DCB
#ifdef LOW_TOUCH_FEATURE
#include "nrf_log_ctrl.h"
/* General DCB Block Module Log settings */
#define NRF_LOG_MODULE_NAME WRIST_DETECT_DCB_BLK

#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

#include "adi_dcb_config.h"
#include "wrist_detect_block.h"
#include "dcb_interface.h"
#include <stdlib.h>

/**< Flag variable to check if wrist_detect DCB entry is present or not. This is
 * updated for every DCB Write/Read and on bootup */
static volatile bool g_wrist_detect_dcb_Present = false;
/**< RAM buffer which holds the gen_blk DCB contents */
static uint32_t g_current_wr_det_dcb[MAXWRISTDETECTDCBSIZE] = {'\0'};

// ================== wrist_detect DCB Section ====================== //

/**
 * @brief    Clear the RAM buffer to hold wrist_detect DCB contents
 * @retval   None
 */
void wrist_detect_dcb_content_clear(void) {
  memset(&g_current_wr_det_dcb[0], 0xFF, sizeof(g_current_wr_det_dcb));
}
/**
 * @brief    Load wrist_detect block Default DCB configuration
 * @retval   Status: OK if DCB content is read
 *                 : ERR if DCB read failed
 */
WRIST_DETECT_DCB_STATUS_t load_wrist_detect_dcb() {
  // Load wrist_detect DCB for LT configs
  NRF_LOG_INFO("Load wrist_detect DCB for LT lcfg configs");
  uint16_t dcb_sz = (uint16_t)(MAXWRISTDETECTDCBSIZE);
  wrist_detect_dcb_content_clear();
  if (read_wrist_detect_dcb(&g_current_wr_det_dcb[0], &dcb_sz) == WRIST_DETECT_DCB_STATUS_OK) {
    return WRIST_DETECT_DCB_STATUS_OK;
  } else {
    NRF_LOG_INFO("wrist_detect DCB Read failed!");
    return WRIST_DETECT_DCB_STATUS_ERR;
  }
}

/**
 * @brief  Copy the contents from RAM buffer which has the DCB contents to the
 * location passed
 * @param  p_lt_app_cfg - pointer location to where lcfg parameters are copied
 * @retval   None
 */
void copy_lt_lcfg_from_wrist_detect_dcb(lt_app_cfg_type *p_lt_app_cfg) {
  uint16_t lcfg_data[LT_APP_LCFG_MAX];
  uint16_t i;

  for (i=0; i<LT_APP_LCFG_MAX; i++) {
    lcfg_data[i] = (uint16_t)(g_current_wr_det_dcb[i]);
  }
  p_lt_app_cfg->onWristTimeThreshold  = lcfg_data[LT_APP_LCFG_ONWR_TIME];
  p_lt_app_cfg->offWristTimeThreshold = lcfg_data[LT_APP_LCFG_OFFWR_TIME];
  p_lt_app_cfg->airCapVal             = lcfg_data[LT_APP_LCFG_AIR_CAP_VAL];
  p_lt_app_cfg->skinCapVal            = lcfg_data[LT_APP_LCFG_SKIN_CAP_VAL];
}

/**
* @brief    Gets the entire WRIST_DETECT DCB configuration written in flash
* @param    Data - pointer to dcb struct variable, in_Size - size of data in Double Word (32-bits)
*           rec_size - The Size of the Record to be returned to the user
* @retval   Status
*/
WRIST_DETECT_DCB_STATUS_t read_wrist_detect_dcb(uint32_t *wrist_detect_dcb_data, uint16_t* read_size)
{
    WRIST_DETECT_DCB_STATUS_t dcb_status = WRIST_DETECT_DCB_STATUS_ERR;

    if(adi_dcb_read_from_fds(ADI_DCB_WRIST_DETECT_BLOCK_IDX, wrist_detect_dcb_data, read_size) == DEF_OK)
    {
        dcb_status = WRIST_DETECT_DCB_STATUS_OK;
    }
    return dcb_status;
}

/**
* @brief    Sets the entire WRIST_DETECT DCB configuration in flash
* @param    Data - pointer to dcb struct variable, in_Size - size of data in Double Word (32-bits)
* @retval   Status
*/
WRIST_DETECT_DCB_STATUS_t write_wrist_detect_dcb(uint32_t *wrist_detect_dcb_data, uint16_t write_Size)
{
    WRIST_DETECT_DCB_STATUS_t dcb_status = WRIST_DETECT_DCB_STATUS_ERR;

    if(adi_dcb_write_to_fds(ADI_DCB_WRIST_DETECT_BLOCK_IDX, wrist_detect_dcb_data, write_Size) == DEF_OK)
    {
        dcb_status = WRIST_DETECT_DCB_STATUS_OK;
    }

    return dcb_status;
}

/**
* @brief    Delete the entire WRIST_DETECT DCB configuration in flash
* @param    void
* @retval   Status
*/
WRIST_DETECT_DCB_STATUS_t delete_wrist_detect_dcb(void)
{
    WRIST_DETECT_DCB_STATUS_t dcb_status = WRIST_DETECT_DCB_STATUS_ERR;

    if(adi_dcb_delete_fds_settings(ADI_DCB_WRIST_DETECT_BLOCK_IDX) == DEF_OK)
    {
        dcb_status = WRIST_DETECT_DCB_STATUS_OK;
    }

    return dcb_status;
}

void wrist_detect_set_dcb_present_flag(bool set_flag)
{
    g_wrist_detect_dcb_Present = set_flag;
    NRF_LOG_INFO("Setting..WRIST_DETECT DCB present: %s",(g_wrist_detect_dcb_Present == true ? "TRUE" : "FALSE"));
}

bool wrist_detect_get_dcb_present_flag(void)
{
    NRF_LOG_INFO("WRIST_DETECT DCB present: %s", (g_wrist_detect_dcb_Present == true ? "TRUE" : "FALSE"));
    return g_wrist_detect_dcb_Present;
}

void wrist_detect_update_dcb_present_flag(void)
{
    g_wrist_detect_dcb_Present = adi_dcb_check_fds_entry(ADI_DCB_WRIST_DETECT_BLOCK_IDX);
    NRF_LOG_INFO("Updated. WRIST_DETECT DCB present: %s", (g_wrist_detect_dcb_Present == true ? "TRUE" : "FALSE"));
}
#endif//LOW_TOUCH_FEATURE
#endif