
/**
    ***************************************************************************
    * @file    adxl_dcfg.c
    * @author  ADI Team
    * @version V0.0.1
    * @date    28-July-2016
    * @brief   ADXL default configuration file
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
/* ONLY INCLUDE IN ONE FILE, ALL OTHERS MUST USE EXTERN */

#include "nrf_log_ctrl.h"
#define NRF_LOG_MODULE_NAME ADXL

#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

#include "Adxl362.h"
#include <adxl_dcfg.h>

#ifdef DCB
#include "adi_dcb_config.h"
#include "dcb_interface.h"
#endif

#define MAXADXLDCFGSIZE (50)
#define MAXGETDCFGSIZE (15)

     /*      DCFG description

  -ACTIVITY:            Not enable. Thresholds to 0.
  -INACTIVITY:          Not enable. Thresholds to 0.
  -FIFO CONTROL:        Temp disabled ; Stream mode ; Watermark = 12 samples (4 complete sets)
  -INTMAP1:             Active low. FIFO watermark enabled.
  -INTMAP2:             Active low. All disabled.
  -FILTER CONTROL:      Range +-8gees ; Filter to 1/2 of BW ; Ext sample disabled ; ODR = 50Hz
  -POWER CONTROL:       Ext CLK disabled ; Low noise mode ; no wake up ; no autosleep ; standby
  -SELF TEST:           Self test disabled.

*/
uint16_t dcfg_org_adxl362[MAXADXLDCFGSIZE] = {
    0x2000,
    0x2100,
    0x2200,
    0x2300,
    0x2400,
    0x2500,
    0x2600,
    0x2700,
    0x2802,
    0x290B, //For ADXL watermark=4
    0x2A84,
    0x2B80,
    0x2C92,
    0x2D10,
    0x2E00,
};

static uint16_t g_current_adxldcfg[MAXADXLDCFGSIZE];

#ifdef DCB
static volatile bool g_adxl_dcb_Present = false;
static uint16_t g_current_adxldcb[MAXADXLDCBSIZE] = {'\0'};
#endif
/**
* @brief    Load ADPD4000 Default configuration
* @param    device_id - Adpd4000 device index/type
* @retval   Status
*/
ADXL_DCB_STATUS_t load_adxl_cfg(uint16_t device_id)
{
    ADXL_DCB_STATUS_t adxl_cfg_status = ADXL_DCB_STATUS_ERR;
    ADXL_DCFG_STATUS_t ret;
#ifdef DCB
    bool dcb_cfg = false;

    dcb_cfg = adxl_get_dcb_present_flag();
    if(dcb_cfg == true)
    {
        //Load dcb Settings
        adxl_cfg_status = load_adxl_dcb(device_id);
        if(adxl_cfg_status != ADXL_DCB_STATUS_OK)
        {
            NRF_LOG_INFO("Failed in Loading adxl DCB cfg");
        }
        NRF_LOG_INFO("Load adxl DCB cfg");
    }
    else
    {
#endif
        //Load dcfg Settings
        ret = load_adxl_dcfg(device_id);
        adxl_cfg_status= (!ret) ? ADXL_DCB_STATUS_OK : ADXL_DCB_STATUS_ERR;
        if(adxl_cfg_status != ADXL_DCB_STATUS_OK)
        {
            NRF_LOG_INFO("Failed in Loading adxl Default f/w cfg");
        }
        NRF_LOG_INFO("Load adxl Default f/w cfg");

#ifdef DCB
    }
#endif
    return adxl_cfg_status;
}

/**
* @brief    Load ADPD default configuration
* @param    device_id - Adpd device index/type
* @retval   Status
*/
ADXL_DCFG_STATUS_t load_adxl_dcfg(uint16_t device_id) {
  //uint8_t nAdpdOdr, nFifoWatermark = 4;
  //nAdpdOdr = 50;
  if (device_id != 0) {
    stage_adxl_dcfg(&device_id);
  }
  //AdxlDrvOpenDriver(nAdpdOdr, nFifoWatermark);
  if (write_adxl_dcfg(&g_current_adxldcfg[0], sizeof(g_current_adxldcfg)) != ADXL_DCFG_STATUS_OK)
  {
      return ADXL_DCFG_STATUS_ERR;
  }
  return ADXL_DCFG_STATUS_OK;
}

/**
* @brief    Gets the entire ADXL current device configuration
* @param    pDcfg - pointer to dcfg register/value pairs
* @retval   Status
*/
ADXL_DCFG_STATUS_t read_adxl_dcfg(uint32_t *p_dcfg, uint8_t *p_dcfg_size) {
  uint8_t reg_addr;
  uint8_t reg_data;
  uint16_t *np_dcfg;

  if (p_dcfg == NULL) {
    return ADXL_DCFG_STATUS_NULL_PTR;
  }
  np_dcfg = (uint16_t *)p_dcfg;

#ifdef DCB
  bool dcb_cfg = false;
  dcb_cfg = adxl_get_dcb_present_flag();
#endif

  for (int i = 0; i < MAXGETDCFGSIZE; i++) {
#ifdef DCB
    if(dcb_cfg == true) {
      reg_addr = (uint8_t) (g_current_adxldcb[i] >> 8);
    }else {
      reg_addr = (uint8_t) (g_current_adxldcfg[i] >> 8);
    }
#else
    reg_addr = (uint8_t) (g_current_adxldcfg[i] >> 8);
#endif
    if (AdxlDrvRegRead(reg_addr, &reg_data) != ADXLDrv_SUCCESS) {
      return ADXL_DCFG_STATUS_ERR;
    }
    *np_dcfg = (reg_addr << 8) | reg_data;
     np_dcfg++;
    *p_dcfg_size = i + 1;
  }
  return ADXL_DCFG_STATUS_OK;
}

/**
* @brief    Stage default DCFG to buffer
* @param    p_device_id - pointer to a device ID
* @retval   Success/Error
*/
ADXL_DCFG_STATUS_t stage_adxl_dcfg(uint16_t *p_device_id) {
  if (p_device_id == NULL) {
    return ADXL_DCFG_STATUS_NULL_PTR;
  }

  adxl_dcfg_clear();

  memcpy(&g_current_adxldcfg[0], &dcfg_org_adxl362[0], sizeof(dcfg_org_adxl362));
  return ADXL_DCFG_STATUS_OK;
}

/**
* @brief    Write a dcfg to device
* @param    p_dcfg - pointer to Dcfg array
* @retval   Status
*/
ADXL_DCFG_STATUS_t write_adxl_dcfg(uint16_t *p_dcfg, uint16_t Size)
{
  if (p_dcfg == NULL)
  {
      return ADXL_DCFG_STATUS_NULL_PTR;
  }
  // clear FIFO and IRQs
  AdxlDrvSetOperationMode(PRM_MEASURE_STANDBY_MODE);
  AdxlDrvSetOperationMode(PRM_MEASURE_STANDBY_MODE);


  if (LoadDcfg(p_dcfg, Size) != ADXLDrv_SUCCESS) {
      AdxlDrvSetOperationMode(PRM_MEASURE_STANDBY_MODE);
      return ADXL_DCFG_STATUS_ERR;
    }
  AdxlDrvSetOperationMode(PRM_MEASURE_STANDBY_MODE);
  return ADXL_DCFG_STATUS_OK;
}


/** @brief  Driver Middleware to load the default dcfg
  *
  * @param  pointer to config array
  * @param  config array size
  * @return int8_t A 8-bit integer: 0 - success; < 0 - failure
  */

static int8_t LoadDcfg(uint16_t *pCfg, uint16_t nBufSize){

  uint16_t nSize = nBufSize;
  uint8_t aTxValues[MAXADXLDCFGSIZE + 2]; // Size  + 2 bytes for commands & address

  for(uint16_t i = 0 ; i < (nSize >> 1) ; i++)
      aTxValues[i + 2] = *pCfg++ & 0x00FF;

  aTxValues[0] = COMMAND_WRITE;
  aTxValues[1] = REG_THRESH_ACT_L;
  GPIO_IRQ_ADXL362_Disable();
  ADXL362_SPI_Transmit(aTxValues, nSize + 2);
  GPIO_IRQ_ADXL362_Enable();

  return 0;
}

void adxl_dcfg_clear(void) {
  memset(&g_current_adxldcfg[0], 0xFF, sizeof(g_current_adxldcfg));
}

#ifdef DCB
// ================== ADXL_DCB Section ====================== //

/**
* @brief    Load ADXL Default DCB configuration
* @param    device_id - ADXL device index/type
* @retval   Status
*/
ADXL_DCB_STATUS_t load_adxl_dcb(uint16_t device_id)
{
    ADXL_DCB_STATUS_t adxl_dcb_sts = ADXL_DCB_STATUS_ERR;

    if (device_id != 0)
    {
        adxl_dcb_sts = stage_adxl_dcb(&device_id);
    }

   if(adxl_dcb_sts == ADXL_DCB_STATUS_OK)
   {
    //AdxlDrvOpenDriver(nAdpdOdr, nFifoWatermark);
    if (write_adxl_dcfg(&g_current_adxldcb[0], sizeof(g_current_adxldcb)) != ADXL_DCFG_STATUS_OK) //
    {
        adxl_dcb_sts = ADXL_DCB_STATUS_ERR;
    }
    else
    {
        adxl_dcb_sts = ADXL_DCB_STATUS_OK;
    }
   }

    return adxl_dcb_sts;
}

/**
* @brief    Stage default DCFG to buffer
* @param    p_device_id - pointer to a device ID
* @retval   Success/Error
*/
ADXL_DCB_STATUS_t stage_adxl_dcb(uint16_t *p_device_id)
{
    static uint32_t adxl_dcb_adxl362_settings[MAXADXLDCBSIZE] = {'\0'};
    uint16_t size = MAXADXLDCBSIZE;
    ADXL_DCB_STATUS_t adxl_dcb_sts = ADXL_DCB_STATUS_ERR;

    if (p_device_id == NULL)
    {
        adxl_dcb_sts = ADXL_DCB_STATUS_NULL_PTR;
        return adxl_dcb_sts;
    }

    adxl_dcb_clear();

    if(read_adxl_dcb(adxl_dcb_adxl362_settings,&size) == ADXL_DCB_STATUS_OK)
    {
        for(int adxldcb_e = 0;adxldcb_e < MAXADXLDCBSIZE; adxldcb_e++)
        {
            g_current_adxldcb[adxldcb_e] = (uint16_t)adxl_dcb_adxl362_settings[adxldcb_e];
        }
        adxl_dcb_sts = ADXL_DCB_STATUS_OK;
    }
    else
    {
        adxl_dcb_sts = ADXL_DCB_STATUS_ERR;
    }

    return adxl_dcb_sts;
}

/**
* @brief    Gets the entire ADXL DCB configuration written in flash
* @param    Data - pointer to dcb struct variable, in_Size - size of data in Double Word (32-bits)
*           rec_size - The Size of the Record to be returned to the user
* @retval   Status
*/
ADXL_DCB_STATUS_t read_adxl_dcb(uint32_t *adxl_dcb_data, uint16_t* read_size)
{
    ADXL_DCB_STATUS_t dcb_status = ADXL_DCB_STATUS_ERR;

    if(adi_dcb_read_from_fds(ADI_DCB_ADXL362_BLOCK_IDX, adxl_dcb_data, read_size) == DEF_OK)
    {
        dcb_status = ADXL_DCB_STATUS_OK;
    }
    return dcb_status;
}

/**
* @brief    Sets the entire ADXL DCB configuration in flash
* @param    Data - pointer to dcb struct variable, in_Size - size of data in Double Word (32-bits)
* @retval   Status
*/
ADXL_DCB_STATUS_t write_adxl_dcb(uint32_t *adxl_dcb_data, uint16_t write_Size)
{
    ADXL_DCB_STATUS_t dcb_status = ADXL_DCB_STATUS_ERR;

    if(adi_dcb_write_to_fds(ADI_DCB_ADXL362_BLOCK_IDX, adxl_dcb_data, write_Size) == DEF_OK)
    {
        dcb_status = ADXL_DCB_STATUS_OK;
    }

    return dcb_status;
}

/**
* @brief    Delete the entire ADXL DCB configuration in flash
* @param    void
* @retval   Status
*/
ADXL_DCB_STATUS_t delete_adxl_dcb(void)
{
    ADXL_DCB_STATUS_t dcb_status = ADXL_DCB_STATUS_ERR;

    if(adi_dcb_delete_fds_settings(ADI_DCB_ADXL362_BLOCK_IDX) == DEF_OK)
    {
        dcb_status = ADXL_DCB_STATUS_OK;
    }

    return dcb_status;
}

void adxl_dcb_clear(void)
{
  memset(&g_current_adxldcb[0], 0xFF, sizeof(g_current_adxldcb));
}

void adxl_set_dcb_present_flag(bool set_flag)
{
    g_adxl_dcb_Present = set_flag;
    NRF_LOG_INFO("Setting..ADXL DCB present: %s",(g_adxl_dcb_Present == true ? "TRUE" : "FALSE"));
}

bool adxl_get_dcb_present_flag(void)
{
    NRF_LOG_INFO("ADXL DCB present: %s", (g_adxl_dcb_Present == true ? "TRUE" : "FALSE"));
    return g_adxl_dcb_Present;
}

void adxl_update_dcb_present_flag(void)
{
    g_adxl_dcb_Present = adi_dcb_check_fds_entry(ADI_DCB_ADXL362_BLOCK_IDX);
    NRF_LOG_INFO("Updated. ADXL DCB present: %s", (g_adxl_dcb_Present == true ? "TRUE" : "FALSE"));
}
#endif
