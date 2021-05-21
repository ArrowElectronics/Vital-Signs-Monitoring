/**
****************************************************************************
* @file     page_logging_ecg.c
* @author   ADI
* @version  V0.1
* @date     11-Mar-2021
* @brief    This is the source file used to display ecg logging information 
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

#include "display_app.h"
#include "lcd_driver.h"
#include "key_detect.h"
#include "lygl.h"
#include "image_declare.h"
#include "display_m2m2_protocol.h"
#include "file_system_interface.h"
#include <post_office.h>

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

static void display_func(void)
{
    uint8_t status;
    uint32_t err;
    uint8_t color;
    
    lcd_background_color_set(COLOR_BACKGROUND);
    lygl_draw_image(&bm_logging_ico,84,20);
    lygl_dis_string_middle(&GUI_Fontweiruanyahei47,104,94,COLOR_WHITE,"ECG");

    if(0 == logging_check_flag.logging_t.ecg)
    {
        err = m2m2_check_fs_sub_status(M2M2_ADDR_MED_ECG_STREAM,&status);
        APP_ERROR_CHECK(err);
        if(M2M2_FILE_SYS_SUBSCRIBED == status)
        {
            logging_status_value.logging_t.ecg = 1;
            logging_set_value.logging_t.ecg = 1;
        }
        else
        {
            logging_status_value.logging_t.ecg = 0;
            logging_set_value.logging_t.ecg = 0;
        }
        logging_check_flag.logging_t.ecg = 1;
    }

    if(logging_set_value.logging_t.ecg == logging_status_value.logging_t.ecg)
    {
        color = COLOR_GREEN;
    }
    else
    {
        color = COLOR_MAGENTA;
    }

    if(0 != logging_set_value.logging_t.ecg)
    {
        lygl_dis_string_middle(&GUI_Fontweiruanyahei48,104,150,color,"ON");
    }
    else
    {
        lygl_dis_string_middle(&GUI_Fontweiruanyahei48,104,150,color,"OFF");
    }

    lcd_display_refresh_all();
    
}

static void key_handle(uint8_t key_value)
{
    uint8_t color;
    switch(key_value)
    {
        case KEY_SELECT_SHORT:
        {
            lcd_background_color_set_section(126,174,COLOR_BACKGROUND);
            if(logging_set_value.logging_t.ecg != logging_status_value.logging_t.ecg)
            {
                color = COLOR_GREEN;
            }
            else
            {
                color = COLOR_MAGENTA;
            }
            if(0 != logging_set_value.logging_t.ecg)
            {
                logging_set_value.logging_t.ecg = 0;
                lygl_dis_string_middle(&GUI_Fontweiruanyahei48,104,150,color,"OFF");
            }
            else
            {
                logging_set_value.logging_t.ecg = 1;    
                lygl_dis_string_middle(&GUI_Fontweiruanyahei48,104,150,color,"ON");
            }
            lcd_display_refresh_section(126,174);

        }
        break;
        case KEY_NAVIGATION_SHORT:
        {
            dis_page_jump(&page_logging_all);
        }
        break;
        case KEY_SELECT_LONG_VALUE:
        {
            if(logging_set_value.logging_all != logging_status_value.logging_all)
            {
                dis_page_jump(&page_logging_confirm);
            }
            else
            {
                dis_page_jump(&page_memory);
            }
            logging_check_flag.logging_all = 0;
        }
        break;
        default:break;
    }

}

const PAGE_HANDLE page_logging_ecg = {
.display_func = &display_func,
.key_handle = &key_handle,
.page_type = DIS_STATIC_PAGE,
};
