/**
****************************************************************************
* @file     page_shipment_mode_confirm.c
* @author   ADI
* @version  V0.1
* @date     11-Mar-2021
* @brief    This is a display file used to confirm whether enter shipment mode.
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
#include "adp5360.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

static uint8_t shipment_mode_option = 1;//0:no shipment mode,1:shipment mode.


static void display_func(void)
{
    lcd_dis_bit_set(DISPLAY_OUT_4BIT);
    lcd_background_color_set_section(70,160,COLOR_YELLOW);
    lygl_dis_string_middle(&lygl_font_32,104,90,COLOR_BLUE,"Shipment mode?");

    shipment_mode_option = 1;
    lygl_dis_string_middle(&lygl_font_48,60,130,COLOR_RED,"yes");
    lygl_dis_string_middle(&lygl_font_32,148,130,COLOR_BLUE,"no");
    lcd_display_refresh_section(70,160);

}

static void key_handle(uint8_t key_value)
{
    switch(key_value)
    {
        case KEY_SELECT_SHORT:
        {
            if(0 != shipment_mode_option)
            {
                Adp5360_enter_shipment_mode();
            }
            else
            {
                dis_page_jump(&page_shipment_mode);
            }

        }
        break;
        case KEY_NAVIGATION_SHORT:
        {
            lcd_background_color_set_section(110,160,COLOR_YELLOW);
            if(0 != shipment_mode_option)
            {
                shipment_mode_option = 0;
                lygl_dis_string_middle(&lygl_font_32,60,130,COLOR_BLUE,"yes");
                lygl_dis_string_middle(&lygl_font_48,148,130,COLOR_RED,"no");
            }
            else
            {
                shipment_mode_option = 1;
                lygl_dis_string_middle(&lygl_font_48,60,130,COLOR_RED,"yes");
                lygl_dis_string_middle(&lygl_font_32,148,130,COLOR_BLUE,"no");
            }
            lcd_display_refresh_section(110,160);
        }
        break;
        case KEY_SELECT_LONG_VALUE:
        {
//            dis_page_jump(&page_memory);
        }
        break;
        default:break;
    }

}

const PAGE_HANDLE page_shipment_mode_confirm = {
.display_func = &display_func,
.key_handle = &key_handle,
.page_type = DIS_STATIC_PAGE,
};
#endif