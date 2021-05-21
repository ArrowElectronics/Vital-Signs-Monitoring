/**
****************************************************************************
* @file     key_detect.c
* @author   ADI
* @version  V0.1
* @date     20-May-2020
* @brief    This source file is used to implement key detect functions
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

#include "key_detect.h"
#ifdef PCBA
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

const nrf_drv_timer_t TIMER_KEY = NRF_DRV_TIMER_INSTANCE(2);

static Send_key_func send_key_handle = NULL;

void Register_key_send_func(Send_key_func hander)
{
    send_key_handle = hander;
}

static void gpiote_event_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    if(0 == nrf_drv_timer_is_enabled(&TIMER_KEY))
    {
        nrf_drv_timer_enable(&TIMER_KEY);
    }
}

uint8_t key_value_get(void)
{
    uint8_t value = 0;
    if(false == nrf_drv_gpiote_in_is_set(KEY_NAVIGATIONPIN))
    {
            value |= KEY_NAVIGATION_VALUE;
    }
    return value;
}
static uint8_t key_value = 0;
static uint8_t last_key_value = 0;
static uint16_t key_cnt = 0;
static void timer_key_scan_handler(nrf_timer_event_t event_type, void* p_context)
{    
    key_value = key_value_get();
    if(last_key_value == key_value)
    {
        if(0 != key_value)
        {
            key_cnt++;
            if(key_cnt > LONG_PRESS_TIMEOUT_MS)
            {
                key_cnt = 0;
                if(NULL != send_key_handle)
                {
                    send_key_handle(key_value|(APP_KEY_LONG_PUSH << 4));
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
    if(0x00 == key_value)//supend the task when no key press 
    {
        if(1 == nrf_drv_timer_is_enabled(&TIMER_KEY))
        {
            nrf_drv_timer_disable(&TIMER_KEY);
        }
    }  
}
uint32_t key_detect_init(void)
{
    uint32_t err_code = NRF_SUCCESS;
    
    nrf_drv_gpiote_in_config_t config = GPIOTE_CONFIG_IN_SENSE_HITOLO(false);
            
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
    err_code = nrf_drv_gpiote_in_init(KEY_NAVIGATIONPIN, &config, gpiote_event_handler);
    if(NRF_SUCCESS != err_code)
    {
        NRF_LOG_INFO("KEY_NAVIGATIONPIN init failue,err_code = %d",err_code);
        return err_code;
    }       
    
    
    uint32_t time_ms = 10; //Time(in miliseconds) between consecutive compare events.
    uint32_t time_ticks;

    nrf_drv_timer_config_t timer_cfg = NRF_DRV_TIMER_DEFAULT_CONFIG;
    err_code = nrf_drv_timer_init(&TIMER_KEY, &timer_cfg, timer_key_scan_handler);
    if(NRF_SUCCESS != err_code)
    {
        NRF_LOG_INFO("nrf_drv_timer_init failue,err_code = %d",err_code);
        return err_code;
    } 
    
    time_ticks = nrf_drv_timer_ms_to_ticks(&TIMER_KEY, time_ms);

    nrf_drv_timer_extended_compare(
         &TIMER_KEY, NRF_TIMER_CC_CHANNEL1, time_ticks, NRF_TIMER_SHORT_COMPARE1_CLEAR_MASK, true);
    
    nrf_drv_gpiote_in_event_enable(KEY_NAVIGATIONPIN, true);//enable after timer init.
    NRF_LOG_INFO("key_detect init success");
    return err_code;
    
}

void key_detect_uninit(void)
{
    if (nrf_drv_gpiote_is_init())
    {
        nrf_drv_gpiote_uninit(); 
        nrf_drv_timer_uninit(&TIMER_KEY);
    }

    
}


