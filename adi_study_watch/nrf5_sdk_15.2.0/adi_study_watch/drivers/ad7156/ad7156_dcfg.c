
/**
    ***************************************************************************
    * @file    ad7156_dcfg.c
    * @author  ADI Team
    * @version V0.0.1
    * @date    17-August-2020
    * @brief   AD7156 default configuration file
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
/* ONLY INCLUDE IN ONE FILE, ALL OTHERS MUST USE EXTERN */

#include "nrf_log_ctrl.h"
#define NRF_LOG_MODULE_NAME AD7156

#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

#include "ad7156.h"
#include <ad7156_dcfg.h>

#ifdef DCB
#include "adi_dcb_config.h"
#include "dcb_interface.h"
#endif
#define MAXAD7156DCFGSIZE (20)

uint16_t dcfg_org_ad7156[MAXAD7156DCFGSIZE] = {
    0x0919, //CH1 sensitivity
    0x0A56, //CH1 timeout
    0x0BCB, //CH1 setup
    0x0C06, //CH2 sensitivity
    0x0D96, //CH2 timeout
    0x0ECB, //CH2 setup
    0x0F19, //Config
    0x1040, //PWR DWN Timer
    0x11C0, //CH1 CAPDAC
    0x12C0, //CH2 CAPDAC
};

static uint16_t g_current_ad7156dcfg[MAXAD7156DCFGSIZE];

#ifdef DCB
static volatile bool g_ad7156_dcb_Present = false;
static uint16_t g_current_ad7156dcb[MAXAD7156DCFGSIZE] = {'\0'};
#endif

/**
* @brief    Load AD7156 Default configuration
* @retval   Status
*/
AD7156_DCB_STATUS_t load_ad7156_cfg() 
{
    AD7156_DCB_STATUS_t ad7156_cfg_status = AD7156_DCB_STATUS_ERR;
    AD7156_DCFG_STATUS_t ret;
#ifdef DCB
    bool dcb_cfg = false;

    dcb_cfg = ad7156_get_dcb_present_flag();
    if(true == dcb_cfg)
    {
        //Load dcb Settings
        ad7156_cfg_status = load_ad7156_dcb();
        if(ad7156_cfg_status != AD7156_DCB_STATUS_OK)
        {
            NRF_LOG_INFO("Failed in Loading ad7156 DCB cfg");
        }
        NRF_LOG_INFO("Load ad7156 DCB cfg");
    }
    else
    {   
#endif
        //Load dcfg Settings
        ret = load_ad7156_dcfg();
        ad7156_cfg_status= (!ret) ? AD7156_DCB_STATUS_OK : AD7156_DCB_STATUS_ERR;
        if(ad7156_cfg_status != AD7156_DCB_STATUS_OK)
        {
            NRF_LOG_INFO("Failed in Loading ad7156 Default f/w cfg");
        }
        NRF_LOG_INFO("Load ad7156 Default f/w cfg");

#ifdef DCB
    }
#endif
    return ad7156_cfg_status;
}

/**
* @brief    Load AD7156 default configuration
* @retval   Status
*/
AD7156_DCFG_STATUS_t load_ad7156_dcfg() {
  stage_ad7156_dcfg();

  if (write_ad7156_dcfg(&g_current_ad7156dcfg[0], sizeof(g_current_ad7156dcfg)/sizeof(g_current_ad7156dcfg[0])) != AD7156_DCFG_STATUS_OK) 
  {
      return AD7156_DCFG_STATUS_ERR;
  }
  return AD7156_DCFG_STATUS_OK;
}

/**
* @brief    Gets the entire AD7156 current device configuration
* @param    pDcfg - pointer to dcfg register/value pairs
* @retval   Status
*/
AD7156_DCFG_STATUS_t read_ad7156_dcfg(uint16_t *p_dcfg, uint8_t *p_dcfg_size) {
  uint8_t reg_addr;
  uint8_t reg_data;
  if (p_dcfg == NULL) {
    return AD7156_DCFG_STATUS_NULL_PTR;
  }

  for (int i = 0; i < MAXAD7156DCFGSIZE; i++) {
    reg_addr = (uint8_t) (g_current_ad7156dcfg[i] >> 8);
    AD7156_GetRegisterValue((uint8_t *)&reg_data, reg_addr, 1);
    *p_dcfg = (reg_addr << 8) | reg_data;
    p_dcfg++;
    *p_dcfg_size = i + 1;
  }
  return AD7156_DCFG_STATUS_OK;
}

/**
* @brief    Stage default DCFG to buffer
* @retval   Success/Error
*/
AD7156_DCFG_STATUS_t stage_ad7156_dcfg() {
  ad7156_dcfg_clear();

  memcpy(&g_current_ad7156dcfg[0], &dcfg_org_ad7156[0], sizeof(dcfg_org_ad7156));
  return AD7156_DCFG_STATUS_OK;
}

/**
* @brief    Write a dcfg to device
* @param    p_dcfg - pointer to Dcfg array
* @retval   Status
*/
AD7156_DCFG_STATUS_t write_ad7156_dcfg(uint16_t *p_dcfg, uint16_t Size) 
{
  if (p_dcfg == NULL) 
  {
      return AD7156_DCFG_STATUS_NULL_PTR;
  }
  AD7156_SetPowerMode(AD7156_CONV_MODE_PWR_DWN);

  if (LoadDcfg(p_dcfg, Size)) {
      AD7156_SetPowerMode(AD7156_CONV_MODE_PWR_DWN);
      return AD7156_DCFG_STATUS_ERR;
    }
  AD7156_SetPowerMode(AD7156_CONV_MODE_CONT_CONV);
  return AD7156_DCFG_STATUS_OK;
}


/** @brief  Driver Middleware to load the default dcfg
  *
  * @param  pointer to config array
  * @param  config array size
  * @return int8_t A 8-bit integer: 0 - success; < 0 - failure
  */

static int8_t LoadDcfg(uint16_t *pCfg, uint16_t nBufSize){

  uint8_t reg_addr, reg_val;

  for(uint16_t i = 0 ; i < (nBufSize) ; i++)
  {
    if(pCfg[i] == 0xFFFF)
      break;
    else
    {
      reg_addr = (uint8_t)(*(pCfg+i)>>8);
      reg_val  = (uint8_t)( *(pCfg+i) & 0x00FF);
      AD7156_SetRegisterValue( reg_val, reg_addr, 1);
      //NRF_LOG_INFO("Load %x %x",reg_addr, reg_val);
    }
  }
  return 0;
}

void ad7156_dcfg_clear(void) {
  memset(&g_current_ad7156dcfg[0], 0xFF, sizeof(g_current_ad7156dcfg));
}

#ifdef DCB
// ================== AD7156_DCB Section ====================== //

/**
* @brief    Load AD7156 Default DCB configuration
* @retval   Status
*/
AD7156_DCB_STATUS_t load_ad7156_dcb() 
{
    AD7156_DCB_STATUS_t ad7156_dcb_sts = AD7156_DCB_STATUS_ERR;

    ad7156_dcb_sts = stage_ad7156_dcb();
    
   if(ad7156_dcb_sts == AD7156_DCB_STATUS_OK)
   {
    if (write_ad7156_dcfg(&g_current_ad7156dcb[0], sizeof(g_current_ad7156dcb)/sizeof(g_current_ad7156dcb[0])) != AD7156_DCFG_STATUS_OK) //
    {
        ad7156_dcb_sts = AD7156_DCB_STATUS_ERR;
    }
    else
    {
        ad7156_dcb_sts = AD7156_DCB_STATUS_OK;
    }
   }

    return ad7156_dcb_sts;
}

/**
* @brief    Stage default DCFG to buffer
* @param    p_device_id - pointer to a device ID
* @retval   Success/Error
*/
AD7156_DCB_STATUS_t stage_ad7156_dcb() 
{
    static uint32_t ad7156_dcb_ad7156_settings[MAXAD7156DCBSIZE] = {'\0'};
    uint16_t size = MAXAD7156DCBSIZE;
    AD7156_DCB_STATUS_t ad7156_dcb_sts = AD7156_DCB_STATUS_ERR;

    ad7156_dcb_clear();

    if(read_ad7156_dcb(ad7156_dcb_ad7156_settings,&size) == AD7156_DCB_STATUS_OK)
    {
        for(int ad7156dcb_e = 0;ad7156dcb_e < MAXAD7156DCBSIZE; ad7156dcb_e++)
        {
            g_current_ad7156dcb[ad7156dcb_e] = (uint16_t)ad7156_dcb_ad7156_settings[ad7156dcb_e];
        }
        ad7156_dcb_sts = AD7156_DCB_STATUS_OK;
    }
    else
    {
        ad7156_dcb_sts = AD7156_DCB_STATUS_ERR;
    }

    return ad7156_dcb_sts;
}

/**
* @brief    Gets the entire AD7156 DCB configuration written in flash
* @param    Data - pointer to dcb struct variable, in_Size - size of data in Double Word (32-bits)
*           rec_size - The Size of the Record to be returned to the user
* @retval   Status
*/
AD7156_DCB_STATUS_t read_ad7156_dcb(uint32_t *ad7156_dcb_data, uint16_t* read_size)
{
    AD7156_DCB_STATUS_t dcb_status = AD7156_DCB_STATUS_ERR;
    
    if(adi_dcb_read_from_fds(ADI_DCB_AD7156_BLOCK_IDX, ad7156_dcb_data, read_size) == DEF_OK)
    {
        dcb_status = AD7156_DCB_STATUS_OK;
    }
    return dcb_status;
}

/**
* @brief    Sets the entire AD7156 DCB configuration in flash
* @param    Data - pointer to dcb struct variable, in_Size - size of data in Double Word (32-bits)
* @retval   Status
*/
AD7156_DCB_STATUS_t write_ad7156_dcb(uint32_t *ad7156_dcb_data, uint16_t write_Size)
{
    AD7156_DCB_STATUS_t dcb_status = AD7156_DCB_STATUS_ERR;
  
    if(adi_dcb_write_to_fds(ADI_DCB_AD7156_BLOCK_IDX, ad7156_dcb_data, write_Size) == DEF_OK)
    {
        dcb_status = AD7156_DCB_STATUS_OK; 
    }

    return dcb_status;
}

/**
* @brief    Delete the entire AD7156 DCB configuration in flash
* @param    void
* @retval   Status
*/
AD7156_DCB_STATUS_t delete_ad7156_dcb(void)
{
    AD7156_DCB_STATUS_t dcb_status = AD7156_DCB_STATUS_ERR;

    if(adi_dcb_delete_fds_settings(ADI_DCB_AD7156_BLOCK_IDX) == DEF_OK)
    {
        dcb_status = AD7156_DCB_STATUS_OK; 
    }

    return dcb_status;
}

void ad7156_dcb_clear(void) 
{
  memset(&g_current_ad7156dcb[0], 0xFF, sizeof(g_current_ad7156dcb));
}

void ad7156_set_dcb_present_flag(bool set_flag)
{
    g_ad7156_dcb_Present = set_flag;
    NRF_LOG_INFO("Setting..AD7156 DCB present: %s",(g_ad7156_dcb_Present == true ? "TRUE" : "FALSE"));
}

bool ad7156_get_dcb_present_flag(void)
{
    NRF_LOG_INFO("AD7156 DCB present: %s", (g_ad7156_dcb_Present == true ? "TRUE" : "FALSE"));
    return g_ad7156_dcb_Present;
}

void ad7156_update_dcb_present_flag(void)
{
    g_ad7156_dcb_Present = adi_dcb_check_fds_entry(ADI_DCB_AD7156_BLOCK_IDX);
    NRF_LOG_INFO("Updated. AD7156 DCB present: %s", (g_ad7156_dcb_Present == true ? "TRUE" : "FALSE"));
}
#endif