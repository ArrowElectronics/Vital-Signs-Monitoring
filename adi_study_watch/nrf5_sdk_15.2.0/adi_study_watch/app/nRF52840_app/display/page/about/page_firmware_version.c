/**
****************************************************************************
* @file     page_firmware_version.c
* @author   ADI
* @version  V0.1
* @date     11-Mar-2021
* @brief    This is the source file used to display firmware version.
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
#include <stdio.h>
#include <post_office.h>
void get_app_version(void)
{
    m2m2_hdr_t *p_m2m2_ = NULL;
    m2m2_app_common_sub_op_t *version_request;
    ADI_OSAL_STATUS       err;
    p_m2m2_ = post_office_create_msg(M2M2_HEADER_SZ+sizeof(m2m2_app_common_sub_op_t));
    if (p_m2m2_ == NULL) {
      return;
    }
    /* swap from network byte order to little endian */
    p_m2m2_->src = M2M2_ADDR_DISPLAY;
    p_m2m2_->dest = M2M2_ADDR_SYS_PM;

    version_request = (m2m2_app_common_sub_op_t *)p_m2m2_->data;
    version_request->command = M2M2_APP_COMMON_CMD_GET_VERSION_REQ;
//    p_m2m2_->length = BYTE_SWAP_16(p_m2m2_->length);
//    p_m2m2_->checksum = BYTE_SWAP_16(p_m2m2_->checksum);
    post_office_send(p_m2m2_,&err);
}
static void display_func(void)
{
    lcd_dis_bit_set(DISPLAY_OUT_4BIT);
    lcd_background_color_set(COLOR_BACKGROUND);
    lygl_dis_string_middle(&lygl_font_32,104,34,COLOR_WHITE,"Version");
    
    lygl_draw_image(&bm_fcc_id_ico,22,135);
    lygl_draw_image(&bm_arrows_down_ico,98,190);
    //lygl_dis_string_middle(&lygl_font_32,104,125,COLOR_WHITE,"FCC ID");
    //lygl_dis_string_middle(&lygl_font_32,104,157,COLOR_YELLOW,"HRF-VSMW4");
    lcd_display_refresh_all();
    get_app_version();
}

static void key_handle(uint8_t key_value)
{
    switch(key_value)
    {
        case KEY_SELECT_SHORT:
        {
            dis_page_jump(&page_fcc_info);
        }
        break;
        case KEY_NAVIGATION_SHORT:
        {
            dis_page_jump(&page_device_name);
        }
        break;
        case KEY_SELECT_LONG_VALUE:
        {
            dis_page_jump(&page_about);
        }
        break;
        default:break;
    }

}

void m2m2_protocol_handle(void * ptr)
{
    m2m2_hdr_t *p_m2m2_ = NULL;
    m2m2_app_common_version_t *payload = NULL;
    char version_string[10] = {0};
    char commit_id_string[14] = {0};
    uint8_t i = 0;
    uint8_t j = 0;

    p_m2m2_ = (m2m2_hdr_t *)ptr;
    payload = (m2m2_app_common_version_t *)p_m2m2_->data;
    if(payload->command == M2M2_APP_COMMON_CMD_GET_VERSION_RESP)
    {
        lcd_background_color_set_section(45,104,COLOR_BACKGROUND);
        i = 0;
        j = 0;
        while(payload->str[i++] != '|');
        while(payload->str[i] != '|')
        {
            commit_id_string[j++] = payload->str[i++];
        };
        commit_id_string[j] = '\0';
        lygl_dis_string_middle(&lygl_font_32,104,73,COLOR_YELLOW,commit_id_string);
        sprintf(version_string,"%02d-%02d-%02d",payload->major,payload->minor,payload->patch);
        lygl_dis_string_middle(&lygl_font_32,104,99,COLOR_YELLOW,version_string);
        lcd_display_refresh_all();
    }
}
const PAGE_HANDLE page_firmware_version = {
.display_func = &display_func,
.key_handle = &key_handle,
.m2m2_protocol_handle = &m2m2_protocol_handle,
.page_type = DIS_STATIC_PAGE,
};
#endif