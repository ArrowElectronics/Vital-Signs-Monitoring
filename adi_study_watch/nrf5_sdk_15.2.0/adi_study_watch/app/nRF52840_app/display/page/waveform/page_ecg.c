/**
****************************************************************************
* @file     page_ecg.c
* @author   ADI
* @version  V0.1
* @date     11-Mar-2021
* @brief    This is the source file used to display ECG waveform.
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
#include "power_manager.h"
#include "display_m2m2_protocol.h"
#include "ecg_application_interface.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#define ECG_DATA_VALIDBITS     0xFFFF
static lygl_graph_param_t ecg_graph_param =
{
     .x0 = 4,
     .y0 = 140,
     .x1 = 204,
     .y1 = 40,
     .x_space = 2,
     .y_unit = 100,
     .y_offset = 0,
     .wf_color = COLOR_YELLOW,
     .bg_color = COLOR_BLUE,
};

static void display_func(void)
{
    uint8_t status;
    uint32_t err;

    lcd_background_color_set(COLOR_BACKGROUND);
    lygl_draw_image(&bm_hr_icon_small,30,155);
    lygl_dis_string_middle(&lygl_font_32,104,24,COLOR_WHITE,"ECG");
    lygl_dis_string_middle(&lygl_font_32,154,165,COLOR_WHITE,"BPM");
    lygl_dis_string_middle(&lygl_font_32,94,165,COLOR_WHITE,"- -");
//    lygl_dis_string_middle(&lygl_font_24,104,185,COLOR_WHITE,"TEMP");
//    lygl_draw_image(&bm_arrows_down_ico,98,195);

    lygl_creat_graph(&ecg_graph_param);
    lcd_display_refresh_all();
    dis_dynamic_refresh(500);

    err = m2m2_subscribe_stream(M2M2_ADDR_MED_ECG,M2M2_ADDR_MED_ECG_STREAM,&status);
    APP_ERROR_CHECK(err);
    if(M2M2_APP_COMMON_STATUS_SUBSCRIBER_ADDED != status)
    {
        NRF_LOG_INFO("subscribe_stream fail,status = %d",status);
        return;
    }
    m2m2_set_stream_status(SENSOR_ECG,1);
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
            m2m2_set_stream_status(SENSOR_ECG,0);
            err = m2m2_unsubscribe_stream(M2M2_ADDR_MED_ECG,M2M2_ADDR_MED_ECG_STREAM,&status);
            APP_ERROR_CHECK(err);
            if((M2M2_APP_COMMON_STATUS_SUBSCRIBER_REMOVED != status)
                &&(M2M2_APP_COMMON_STATUS_SUBSCRIBER_COUNT_DECREMENT != status))
            {
                NRF_LOG_INFO("unsubscribe stream fail,status = %d",status);
                //return;
            }
#ifdef HIBERNATE_MD_EN
            hibernate_md_set(HIB_MD_DISP_INACTIVE_EVT);
            hibernate_mode_entry();
#endif
            dis_page_jump(&page_ecg_title);
        }
        break;
        default:break;
    }

}

static void signal_handle(uint8_t value)
{
    switch(value)
    {
        case 0:
        {
            lcd_display_refresh_section(40,200);
        }
        break;
        case 1:
        {

        }
        break;
        default:break;
    }
}
static void m2m2_protocol_handle(void * ptr)
{
    m2m2_hdr_t *p_m2m2_ = NULL;
    ecg_app_stream_t *payload3 = NULL;
    uint32_t ecg_value = 0;

    p_m2m2_ = (m2m2_hdr_t *)ptr;
    if(M2M2_ADDR_MED_ECG_STREAM == p_m2m2_->src)
    {
        payload3 = (ecg_app_stream_t *)&p_m2m2_->data[0];
        if(payload3->command == M2M2_SENSOR_COMMON_CMD_STREAM_DATA)
        {
            ecg_value = (uint32_t)(payload3->firstecgdata);
            lygl_send_graph_data(&ecg_value,1,ECG_DATA_VALIDBITS);
            ecg_value = (uint32_t)(payload3->ecg_data[3].ecgdata);
            lygl_send_graph_data(&ecg_value,1,ECG_DATA_VALIDBITS);
            ecg_value = (uint32_t)(payload3->ecg_data[7].ecgdata);
            lygl_send_graph_data(&ecg_value,1,ECG_DATA_VALIDBITS);

            lygl_draw_rectangle(69,141,119,181,COLOR_BACKGROUND);
            lygl_dis_value_middle(&lygl_font_32,94,165,COLOR_WHITE,payload3->HR,0);
        }
    }
}
const PAGE_HANDLE page_ecg = {
.display_func = &display_func,
.key_handle = &key_handle,
.signal_handle = &signal_handle,
.m2m2_protocol_handle = &m2m2_protocol_handle,
.page_type = DIS_DYNAMIC_PAGE,
};
#endif //ENABLE_WATCH_DISPLAY