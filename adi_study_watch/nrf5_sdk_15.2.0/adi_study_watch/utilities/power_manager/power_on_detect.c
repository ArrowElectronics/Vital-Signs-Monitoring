/**
****************************************************************************
* @file     power_on_detect.c
* @author   ADI
* @version  V0.1
* @date     15-May-2020
* @brief    This source file used to implement power on detection functions
****************************************************************************
* @attention
******************************************************************************
* Copyright (c) 2021 Analog Devices, Inc.  All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
* - Redistributions of source code must retain the above copyright notice, this
*   list of conditions and the following disclaimer.
* - Redistributions in binary form must reproduce the above copyright notice,
*   this list of conditions and the following disclaimer in the documentation
*   and/or other materials provided with the distribution.
* - Modified versions of the software must be conspicuously marked as such.
* - This software is licensed solely and exclusively for use with
*   processors/products manufactured by or for Analog Devices, Inc.
* - This software may not be combined or merged with other code in any manner
*   that would cause the software to become subject to terms and conditions
*   which differ from those listed here.
* - Neither the name of Analog Devices, Inc. nor the names of its contributors
*   may be used to endorse or promote products derived from this software
*   without specific prior written permission.
* - The use of this software may or may not infringe the patent rights of one
*   or more patent holders.  This license does not release you from the
*   requirement that you obtain separate licenses from these patent holders to
*   use this software.
*
* THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES, INC. AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
* NONINFRINGEMENT, TITLE, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL ANALOG DEVICES, INC. OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, PUNITIVE OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, DAMAGES ARISING OUT OF
* CLAIMS OF INTELLECTUAL PROPERTY RIGHTS INFRINGEMENT; PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
******************************************************************************/

#include "power_on_detect.h"
#ifdef VSM_MBOARD
#include "watch_vsm_motherboard_pin_config.h"
#elif PCBA
#include "watch_board_pcba_pin_config.h"
#else
#include "watch_board_evt_pin_config.h"
#endif
#include "nrf_drv_timer.h"
#include "nrf_drv_gpiote.h"

#include "nrf_log.h"
#if NRF_LOG_ENABLED
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#endif

#define POWER_ON_PIN (KEY_NAVIGATIONPIN)//select the navigation pin as the power on pin.
const nrf_drv_timer_t TIMER_POWER_ON = NRF_DRV_TIMER_INSTANCE(2);

static power_on_func power_on_handle = NULL;
/*****************************************************************************
 * Function      : register_power_on_func
 * Description   : register power on handler function
 * Input         : power_on_func hander  
 * Output        : None
 * Return        : 
 * Others        : 
 * Record
 * 1.Date        : 20200624
 *   Modification: Created function

*****************************************************************************/
void register_power_on_func(power_on_func hander)
{
    if(NULL == power_on_handle)
    {
        power_on_handle = hander;
    }
}
/*****************************************************************************
 * Function      : gpiote_event_handler
 * Description   : power on pin interrupt handler function
 * Input         : nrf_drv_gpiote_pin_t pin      
                nrf_gpiote_polarity_t action  
 * Output        : None
 * Return        : static
 * Others        : 
 * Record
 * 1.Date        : 20200624
 *   Modification: Created function

*****************************************************************************/
static void gpiote_event_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    if(0 == nrf_drv_timer_is_enabled(&TIMER_POWER_ON))
    {
        nrf_drv_timer_enable(&TIMER_POWER_ON);
    }
}
/*****************************************************************************
 * Function      : pin_value_get
 * Description   : get the power on pin value 
 * Input         : void  
 * Output        : None
 * Return        : static
 * Others        : 
 * Record
 * 1.Date        : 20200624
 *   Modification: Created function

*****************************************************************************/
static uint8_t pin_value_get(void)
{
    uint8_t value = 0;
    if(false == nrf_drv_gpiote_in_is_set(POWER_ON_PIN))
    {
            value = 0xff;
    }
    return value;
}
/*****************************************************************************
 * Function      : timer_key_scan_handler
 * Description   : power on key scan function
 * Input         : nrf_timer_event_t event_type  
                void* p_context               
 * Output        : None
 * Return        : static
 * Others        : 
 * Record
 * 1.Date        : 20200624
 *   Modification: Created function

*****************************************************************************/
static void timer_key_scan_handler(nrf_timer_event_t event_type, void* p_context)
{
    static uint8_t key_value = 0;
    static uint8_t last_key_value = 0;
    static uint16_t key_cnt = 0;
    
    key_value = pin_value_get();
    if(last_key_value == key_value)
    {
        if(0 != key_value)
        {
            key_cnt++;
            if(key_cnt > POWER_ON_PRESS_MS)
            {
                key_cnt = 0;
                if(NULL != power_on_handle)
                {
                    power_on_handle();
                    key_value = 0;//if detect the long press, then stop detect.
                }
            } 
        }
    }
    else
    {
        key_cnt = 0;
    }        
    last_key_value = key_value;
    if(0 == key_value)//supend the task when no key press 
    {
        if(1 == nrf_drv_timer_is_enabled(&TIMER_POWER_ON))
        {
            nrf_drv_timer_disable(&TIMER_POWER_ON);
        }
    }  
}
/*****************************************************************************
 * Function      : power_on_detect_init
 * Description   : initialize the power on detect
 * Input         : void  
 * Output        : None
 * Return        : 
 * Others        : 
 * Record
 * 1.Date        : 20200624
 *   Modification: Created function

*****************************************************************************/
uint32_t power_on_detect_init(void)
{
    uint32_t err_code = NRF_SUCCESS;
    
    nrf_drv_gpiote_in_config_t config = GPIOTE_CONFIG_IN_SENSE_HITOLO(false);//only one interrupt pin, So here can set as to detect high to low.
            
    if (!nrf_drv_gpiote_is_init())
    {
        err_code = nrf_drv_gpiote_init();
        if(NRF_SUCCESS != err_code)
        {
            NRF_LOG_INFO("key_detect gpiote init failue");
            return err_code;
        }
    }
    
    config.pull = NRF_GPIO_PIN_PULLUP;
    err_code = nrf_drv_gpiote_in_init(POWER_ON_PIN, &config, gpiote_event_handler);
    if(NRF_SUCCESS != err_code)
    {
        NRF_LOG_INFO("POWER_ON_PIN init failue,err_code = %d",err_code);
        return err_code;
    }       
    
    
    uint32_t time_ms = 10; //Time(in miliseconds) between consecutive compare events.
    uint32_t time_ticks;

    nrf_drv_timer_config_t timer_cfg = NRF_DRV_TIMER_DEFAULT_CONFIG;
    err_code = nrf_drv_timer_init(&TIMER_POWER_ON, &timer_cfg, timer_key_scan_handler);
    if(NRF_SUCCESS != err_code)
    {
        NRF_LOG_INFO("nrf_drv_timer_init failue,err_code = %d",err_code);
        return err_code;
    } 
    
    time_ticks = nrf_drv_timer_ms_to_ticks(&TIMER_POWER_ON, time_ms);

    nrf_drv_timer_extended_compare(
         &TIMER_POWER_ON, NRF_TIMER_CC_CHANNEL1, time_ticks, NRF_TIMER_SHORT_COMPARE1_CLEAR_MASK, true);
    
    nrf_drv_gpiote_in_event_enable(POWER_ON_PIN, true);//enable after timer init.
    NRF_LOG_INFO("key_detect init success");
    return err_code;
    
}
/*****************************************************************************
 * Function      : power_on_detect_uninit
 * Description   : uninitialize the power on detect
 * Input         : void  
 * Output        : None
 * Return        : 
 * Others        : 
 * Record
 * 1.Date        : 20200624
 *   Modification: Created function

*****************************************************************************/
void power_on_detect_uninit(void)
{
    if (nrf_drv_gpiote_is_init())
    {
        nrf_drv_gpiote_uninit(); 
        nrf_drv_timer_uninit(&TIMER_POWER_ON);
    }    
}


