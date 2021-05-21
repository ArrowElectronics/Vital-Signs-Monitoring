/**
****************************************************************************
* @file     timer_display.c
* @author   ADI
* @version  V0.1
* @date     20-June-2020
* @brief    This source file is used to implement timer for display functions
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

#include <stdbool.h>
#include <stdint.h>
#include "nrf_drv_timer.h"
#include "boards.h"
#include "lcd_driver.h"
#include "dis_driver.h"
#include "timer_display.h"
#include "key_detect.h"
#include "rtc.h"
#include "nrf_bootloader_dfu_timers.h"
#include "app_scheduler.h"
#include "nrf_log.h"
extern const lv_img_dsc_t bmupgrading_ico;
extern const lv_img_dsc_t bmupgrade_error_ico;
extern const lv_img_dsc_t bmupgrade_success_ico;

const nrf_drv_timer_t TIMER_LED = NRF_DRV_TIMER_INSTANCE(1);

static void display_upgrading_status(void * p_evt, uint16_t event_length)
{
//    nrf_dfu_request_t * p_req = (nrf_dfu_request_t *)(p_evt);
    static uint8_t led_cnt = 0;
    
    led_cnt++;
    if(led_cnt >= 5)
    {
        led_cnt = 0;
    }
    lcd_background_color_set(COLOR_BLACK);

    if(0 == led_cnt)
    {
        Display_image(&bmupgrading_ico,80,81);
    }
    else if(1 == led_cnt)
    {
        Display_image(&bmupgrading_ico,80,47);
    }
    else if(2 == led_cnt)
    {
        Display_image(&bmupgrading_ico,80,13);
    }
    else if(4 == led_cnt)
    {
        Display_image(&bmupgrading_ico,80,149);
    }
    else if(5 == led_cnt)
    {
        Display_image(&bmupgrading_ico,80,115);
    }
    lcd_display_refresh_all();

}

void timer_led_event_handler(nrf_timer_event_t event_type, void* p_context)
{
    ret_code_t ret;

    ret = app_sched_event_put(NULL,0, display_upgrading_status);
    if (ret != NRF_SUCCESS)
    {
        NRF_LOG_WARNING("Scheduler ran out of space!");
    }
     
}
void Stop_loop_display(void)
{
    if(1 == nrf_drv_timer_is_enabled(&TIMER_LED))
    {
        nrf_drv_timer_disable(&TIMER_LED);
        nrf_drv_timer_uninit(&TIMER_LED);
    }
    
}

void key_reset_handle(void * p_event_data, uint16_t event_size)
{
    key_detect_uninit();
    Stop_loop_display();
    nrf_bootloader_dfu_inactivity_timer_delete();
    
    rtc_timestamp_store(80);
    NVIC_SystemReset();    
}
void Send_key_value(uint8_t  k_value)
{    
    if(k_value == KEY_NAVIGATION_LONG_VALUE)
    {
        ret_code_t ret_code = app_sched_event_put(NULL, 0, key_reset_handle);
        APP_ERROR_CHECK(ret_code);       
    }
}

void Lcd_display_loop(void)
{
    Register_key_send_func(Send_key_value);
    key_detect_init();
    lcd_init();
}

void Start_loop_display(void)
{
    key_detect_uninit();
    
    uint32_t err_code = NRF_SUCCESS;
    uint32_t time_ms = 200; //Time(in miliseconds) between consecutive compare events.
    uint32_t time_ticks;

    nrf_drv_timer_config_t timer_cfg = NRF_DRV_TIMER_DEFAULT_CONFIG;
    err_code = nrf_drv_timer_init(&TIMER_LED, &timer_cfg, timer_led_event_handler);
    APP_ERROR_CHECK(err_code);
    
    time_ticks = nrf_drv_timer_ms_to_ticks(&TIMER_LED, time_ms);

    nrf_drv_timer_extended_compare(
         &TIMER_LED, NRF_TIMER_CC_CHANNEL1, time_ticks, NRF_TIMER_SHORT_COMPARE1_CLEAR_MASK, true);
    
    if(0 == nrf_drv_timer_is_enabled(&TIMER_LED))
    {
        nrf_drv_timer_enable(&TIMER_LED);
    }
}

void Enter_upgrade_mode(void)
{
    lcd_background_color_set(COLOR_BLACK);
    Display_image(&bmupgrading_ico,80,81);
    lcd_display_refresh_all();
}

void Upgrade_success(void)
{
    lcd_background_color_set(COLOR_BLACK);
    Display_image(&bmupgrade_success_ico,80,84);
    lcd_display_refresh_all();
}

void Upgrade_failure(void)
{
    lcd_background_color_set(COLOR_BLACK);
    Display_image(&bmupgrade_error_ico,84,84);
    lcd_display_refresh_all();
}











