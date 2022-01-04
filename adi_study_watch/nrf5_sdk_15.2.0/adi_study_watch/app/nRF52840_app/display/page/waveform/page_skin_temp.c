/**
****************************************************************************
* @file     page_skin_temp.c
* @author   ADI
* @version  V0.1
* @date     11-Mar-2021
* @brief    This is the source file used to display skin temperature waveform.
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
#include "display_m2m2_protocol.h"
#include "temperature_application_interface.h"
#include "common_sensor_interface.h"
#include "power_manager.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#define TEMP_DATA_VALIDBITS 0XFFFF
static void display_func(void)
{
    uint8_t status;
    uint32_t err;

    lcd_background_color_set(COLOR_BACKGROUND);

    lygl_dis_string_middle(&lygl_font_32,104,24,COLOR_WHITE,"TEMP");
    lygl_dis_string_middle(&lygl_font_32,94,165,COLOR_WHITE,"--.---");
    lygl_draw_image(&bm_setting_temp_ico,142,145);
//    lygl_dis_string_middle(&lygl_font_24,104,185,COLOR_WHITE,"PPG");
//    lygl_draw_image(&bm_arrows_down_ico,98,195);

    lygl_graph_param_t temp_graph_param =
    {
         .x0 = 4,
         .y0 = 140,
         .x1 = 204,
         .y1 = 40,
         .x_space = 40,
         .y_unit = 10,
         .y_offset = 0,
         .wf_color = COLOR_YELLOW,
         .bg_color = COLOR_BLUE,
    };
    lygl_creat_graph(&temp_graph_param);

    err = m2m2_subscribe_stream(M2M2_ADDR_MED_TEMPERATURE,M2M2_ADDR_MED_TEMPERATURE_STREAM,&status);
    APP_ERROR_CHECK(err);
    if(M2M2_APP_COMMON_STATUS_SUBSCRIBER_ADDED != status)
    {
        NRF_LOG_INFO("add_temp_subscribe fail,status = %d",status);
    }
    err = m2m2_start_stream(M2M2_ADDR_MED_TEMPERATURE,&status);
    APP_ERROR_CHECK(err);
    if((M2M2_APP_COMMON_STATUS_STREAM_STARTED != status)&&
    (M2M2_APP_COMMON_STATUS_STREAM_IN_PROGRESS != status))
    {
        NRF_LOG_INFO("sensor temp start fail,status = %d",status);
    }
    lcd_display_refresh_all();
}

static void key_handle(uint8_t key_value)
{
    uint8_t status;
    uint32_t err;
    switch(key_value)
    {
        case KEY_SELECT_SHORT:
        {

        }
        break;
        case KEY_SELECT_LONG_VALUE:
        {
            err = m2m2_unsubscribe_stream(M2M2_ADDR_MED_TEMPERATURE,M2M2_ADDR_MED_TEMPERATURE_STREAM,&status);
            if((M2M2_APP_COMMON_STATUS_SUBSCRIBER_REMOVED != status)
            &&(M2M2_APP_COMMON_STATUS_SUBSCRIBER_COUNT_DECREMENT != status))
            {
                NRF_LOG_INFO("remove temp subscribe ppg fail,status = %d",status);
            }
            err = m2m2_stop_stream(M2M2_ADDR_MED_TEMPERATURE,&status);
            APP_ERROR_CHECK(err);
            NRF_LOG_INFO("stop stream,status = %d",status);
            if(M2M2_APP_COMMON_STATUS_STREAM_STOPPED != status)
            {
                NRF_LOG_INFO("stop stream fail,status = %d",status);
            }
#ifdef HIBERNATE_MD_EN
            hibernate_md_set(HIB_MD_DISP_INACTIVE_EVT);
            hibernate_mode_entry();
#endif
            dis_page_jump(&page_skin_temp_title);
        }
        break;
        default:break;
    }

}

static void m2m2_protocol_handle(void * ptr)
{
    m2m2_hdr_t *p_m2m2_ = NULL;
    temperature_app_stream_t *payload = NULL;
    uint32_t temp_value;
    p_m2m2_ = (m2m2_hdr_t *)ptr;

    if(p_m2m2_->src==M2M2_ADDR_MED_TEMPERATURE_STREAM)
    {
        payload = (temperature_app_stream_t *)&p_m2m2_->data[0];
        if(payload->command == M2M2_SENSOR_COMMON_CMD_STREAM_DATA)
        {
            temp_value = (uint32_t)(payload->nTemperature1);
            lygl_send_graph_data(&temp_value,1,TEMP_DATA_VALIDBITS);
            lygl_draw_rectangle(48,149,138,181,COLOR_BACKGROUND);
            lygl_dis_decimal_middle(&lygl_font_32,94,165,COLOR_WHITE,payload->nTemperature1,3);/* display decimal*/
            lcd_display_refresh_section(40,181);
        }
    }
}
const PAGE_HANDLE page_skin_temp = {
.display_func = &display_func,
.key_handle = &key_handle,
.m2m2_protocol_handle = &m2m2_protocol_handle,
.page_type = DIS_DYNAMIC_PAGE,
};
#endif