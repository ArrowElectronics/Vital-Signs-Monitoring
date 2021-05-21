/**
****************************************************************************
* @file     page_shipment_mode.c
* @author   ADI
* @version  V0.1
* @date     11-Mar-2021
* @brief    This is a display file used to enter shipment mode.
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
#include "ble_task.h"

static void display_func(void)
{
    lcd_dis_bit_set(DISPLAY_OUT_1BIT);
    lcd_background_color_set(COLOR_BACKGROUND);

    lygl_draw_image(&bm_setting_power_off,77,38);

    lygl_dis_string_middle(&lygl_font_32,104,140,COLOR_WHITE,"SHIPMENT");
    lygl_dis_string_middle(&lygl_font_32,104,170,COLOR_WHITE,"MODE");
    lcd_display_refresh_all();
}

static void key_handle(uint8_t key_value)
{
    switch(key_value)
    {
        case KEY_SELECT_SHORT:
        {
            dis_page_jump(&page_shipment_mode_confirm);
        }
        break;
        case KEY_NAVIGATION_SHORT:
        {
            if(get_ble_nus_status() == BLE_CONNECTED )
              dis_page_jump(&page_ble_hr_service);
            else
              dis_page_jump(&page_blue_address);
        }
        break;
        case KEY_SELECT_LONG_VALUE:
        {
            dis_page_jump(&page_setting);
        }
        break;
        default:break;
    }

}

const PAGE_HANDLE page_shipment_mode = {
.display_func = &display_func,
.key_handle = &key_handle,
.page_type = DIS_STATIC_PAGE,
};
#endif