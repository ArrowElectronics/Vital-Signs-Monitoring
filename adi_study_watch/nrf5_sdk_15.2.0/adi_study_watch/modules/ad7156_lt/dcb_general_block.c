/**
 ***************************************************************************
 * @file    dcb_general_block.c
 * @author  ADI Team
 * @version V0.0.1
 * @date    22-April-2020
 * @brief   General Block DCB configuration file
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
#ifdef DCB
#ifdef LOW_TOUCH_FEATURE
#include "nrf_log_ctrl.h"
/* General DCB Block Module Log settings */
#define NRF_LOG_MODULE_NAME GEN_DCB_BLK

#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

#include "adi_dcb_config.h"
#include "dcb_general_block.h"
#include "dcb_interface.h"
#include <stdlib.h>

/**< Flag variable to check if gen_blk DCB entry is present or not. This is
 * updated for every DCB Write/Read and on bootup */
static volatile bool g_dcb_Present = false;
/**< RAM buffer which holds the gen_blk DCB contents */
static uint32_t g_curr_gen_blk_dcb[(MAXGENBLKDCBSIZE * 4)] = {'\0'};

// ================== GENERAL_DCB Section ====================== //

/**
 * @brief    Clear the RAM buffer to hold gen blk DCB contents
 * @retval   None
 */
void gen_blk_dcb_content_clear(void) {
  memset(&g_curr_gen_blk_dcb[0], 0xFF, sizeof(g_curr_gen_blk_dcb));
}
/**
 * @brief    Load General block Default DCB configuration
 * @retval   Status: OK if DCB content is read
 *                 : ERR if DCB read failed
 */
GEN_BLK_DCB_STATUS_t load_gen_blk_dcb() {
  // Load General Block DCB for LT configs
  NRF_LOG_INFO("Load General Block DCB for LT configs");
  uint16_t dcb_sz = (uint16_t)(MAXGENBLKDCBSIZE * 4);
  gen_blk_dcb_content_clear();
  if (read_gen_blk_dcb(&g_curr_gen_blk_dcb[0], &dcb_sz) == GEN_BLK_DCB_STATUS_OK) {
    return GEN_BLK_DCB_STATUS_OK;
  } else {
    NRF_LOG_INFO("General Block DCB Read failed!");
    return GEN_BLK_DCB_STATUS_ERR;
  }
}

/**
 * @brief  Copy the contents from RAM buffer which has the DCB contents to the
 * location passed
 * @param  dest_ptr - pointer location to where contents are copied
 * @param  dest_len - no: of bytes from the RAM buffer, which gets copied to the
 * dest_ptr
 * @retval   None
 */
void copy_lt_config_from_gen_blk_dcb(uint8_t *dest_ptr, uint16_t *dest_len) {
  uint16_t src_len = 0;

  for (src_len = 0; g_curr_gen_blk_dcb[src_len] != 0xFFFFFFFF; src_len++)
    ;

  src_len =
      src_len * sizeof(g_curr_gen_blk_dcb[0]); // converting array length to total
                                          // no: of valid bytes in the array
  memcpy(dest_ptr, g_curr_gen_blk_dcb, src_len);
  *dest_len = src_len;
}

/**@brief   Gets the entire General Blk DCB configuration written in flash
 *
 * @param gen_blk_dcb_data - pointer to dcb struct variable,
 * @param read_size: size of gen_blk_dcb_data array filled from FDS(length in
 * Double Word (32-bits))
 * @return return value of type GEN_BLK_DCB_STATUS_t
 */
GEN_BLK_DCB_STATUS_t read_gen_blk_dcb(
    uint32_t *gen_blk_dcb_data, uint16_t *read_size) {
  GEN_BLK_DCB_STATUS_t dcb_status = GEN_BLK_DCB_STATUS_ERR;

  if (adi_dcb_read_from_fds(
          ADI_DCB_GENERAL_BLOCK_IDX, gen_blk_dcb_data, read_size) == DEF_OK) {
    dcb_status = GEN_BLK_DCB_STATUS_OK;
  }
  return dcb_status;
}

/**@brief   Sets the entire General Blk DCB configuration in flash
 *
 * @param gen_blk_dcb_data - pointer to dcb struct variable,
 * @param in_size: size of gen_blk_dcb_data array(length in Double Word
 * (32-bits))
 * @return return value of type GEN_BLK_DCB_STATUS_t
 */
GEN_BLK_DCB_STATUS_t write_gen_blk_dcb(
    uint32_t *gen_blk_dcb_data, uint16_t in_size) {
  GEN_BLK_DCB_STATUS_t dcb_status = GEN_BLK_DCB_STATUS_ERR;

  if (adi_dcb_write_to_fds(
          ADI_DCB_GENERAL_BLOCK_IDX, gen_blk_dcb_data, in_size) == DEF_OK) {
    dcb_status = GEN_BLK_DCB_STATUS_OK;
  }

  return dcb_status;
}

/**
 * @brief    Delete the entire General blk DCB configuration in flash
 * @param    void
 * @retval   Status
 */
GEN_BLK_DCB_STATUS_t delete_gen_blk_dcb(void) {
  GEN_BLK_DCB_STATUS_t dcb_status = GEN_BLK_DCB_STATUS_ERR;

  if (adi_dcb_delete_fds_settings(ADI_DCB_GENERAL_BLOCK_IDX) == DEF_OK) {
    // gen_blk_curr_cmd_content_clear();
    gen_blk_dcb_content_clear();
    dcb_status = GEN_BLK_DCB_STATUS_OK;
  }

  return dcb_status;
}

/**
 * @brief  Function to set the global flag which holds the status for gen_blk
 * DCB presence. This gets called from DCB write/delete api
 * @param  set_flag - true/flag
 * @retval   None
 */
void gen_blk_set_dcb_present_flag(bool set_flag) {
  g_dcb_Present = set_flag;
  NRF_LOG_INFO("Setting..GEN BLK DCB present: %s",
      (g_dcb_Present == true ? "TRUE" : "FALSE"));
}

/**
 * @brief  Function to get the value of global flag which holds the status for
 * gen_blk DCB presence.
 * @param  None
 * @retval   true/false
 */
bool gen_blk_get_dcb_present_flag(void) {
  NRF_LOG_INFO("GENERAL BLK DCB present: %s",
      (g_dcb_Present == true ? "TRUE" : "FALSE"));
  return g_dcb_Present;
}

/**
 * @brief  Function to which does actual FDS read to update the global flag
 * which holds the status for gen_blk DCB presence.
 * @param  None
 * @retval   None
 */
void gen_blk_update_dcb_present_flag(void) {
  g_dcb_Present = adi_dcb_check_fds_entry(ADI_DCB_GENERAL_BLOCK_IDX);
  NRF_LOG_INFO("Updated. GENERAL BLK DCB present: %s",
      (g_dcb_Present == true ? "TRUE" : "FALSE"));
}
#endif//LOW_TOUCH_FEATURE
#endif