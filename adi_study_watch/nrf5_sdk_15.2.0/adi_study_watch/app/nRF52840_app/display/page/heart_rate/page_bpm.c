/**
****************************************************************************
* @file     page_bpm.c
* @author   ADI
* @version  V0.1
* @date     11-Mar-2021
* @brief    This is the source file used to dynamic display BPM
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
**   or more patent holders.  This license does not release you from the
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
#ifdef ENABLE_WATCH_DISPLAY
#include "display_app.h"
#include "lcd_driver.h"
#include "key_detect.h"
#include "lygl.h"
#include "image_declare.h"

#include "ppg_application_interface.h"
#include "sync_data_application_interface.h"
#include "display_m2m2_protocol.h"
#include "power_manager.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"



static void display_func(void)
{
    uint8_t status;
    uint32_t err;
    lcd_background_color_set(COLOR_BACKGROUND);

    lygl_draw_image(&bm_hr_waveform_icon,86,16);

    lygl_dis_string_middle(&lygl_font_32,104,170,COLOR_WHITE,"BPM");
    lcd_display_refresh_all();

    err = m2m2_subscribe_stream(M2M2_ADDR_MED_PPG,M2M2_ADDR_MED_PPG_STREAM,&status);
    APP_ERROR_CHECK(err);
    if(M2M2_APP_COMMON_STATUS_SUBSCRIBER_ADDED != status)
    {
        NRF_LOG_INFO("subscribe rppg fail,status = %d",status);
        return;
    }
    m2m2_set_stream_status(SENSOR_PPG,1);
}

static void key_handle(uint8_t key_value)
{
    uint8_t status;
    uint32_t err;
    switch(key_value)
    {
        case KEY_SELECT_SHORT:
        {
//            dis_page_jump(&page_hr);
        }
        break;
        case KEY_NAVIGATION_SHORT:
        {

        }
        break;
        case KEY_SELECT_LONG_VALUE:
        {


            err = m2m2_unsubscribe_stream(M2M2_ADDR_MED_PPG,M2M2_ADDR_MED_PPG_STREAM,&status);
            APP_ERROR_CHECK(err);
            if((M2M2_APP_COMMON_STATUS_SUBSCRIBER_REMOVED != status)
                &&(M2M2_APP_COMMON_STATUS_SUBSCRIBER_COUNT_DECREMENT != status))
            {
                NRF_LOG_INFO("unsubscribe stream fail,status = %d",status);
                //return;//even unsubscribe failure, the page need jump.
            }
            m2m2_set_stream_status(SENSOR_PPG,0);
#ifdef HIBERNATE_MD_EN
            hibernate_md_set(HIB_MD_DISP_INACTIVE_EVT);
            hibernate_mode_entry();
#endif
            dis_page_jump(&page_hr);
        }
        break;
        default:break;
    }

}

static void m2m2_protocol_handle(void * ptr)
{
    m2m2_hdr_t *p_m2m2_ = NULL;
    ppg_app_hr_debug_stream_t *payload = NULL;

    p_m2m2_ = (m2m2_hdr_t *)ptr;

    if(p_m2m2_->src==M2M2_ADDR_MED_PPG_STREAM)
    {
        payload = (ppg_app_hr_debug_stream_t *)&p_m2m2_->data[0];
        if(payload->command == M2M2_SENSOR_COMMON_CMD_STREAM_DATA)
        {
            lcd_background_color_set_section(70,138,COLOR_BACKGROUND);
            lygl_dis_value_middle(&lygl_font_62,104,104,COLOR_WHITE,payload->hr>>4,0);
            lcd_display_refresh_section(70,138);
        }
    }
}
const PAGE_HANDLE page_bpm = {
.display_func = &display_func,
.key_handle = &key_handle,
.m2m2_protocol_handle = &m2m2_protocol_handle,
.page_type = DIS_DYNAMIC_PAGE,
};
#endif