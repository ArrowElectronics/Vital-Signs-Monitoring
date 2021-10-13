/**
****************************************************************************
* @file     page_battery_level.c
* @author   ADI
* @version  V0.1
* @date     11-Mar-2021
* @brief    This is the source file used to display battery level.
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

static void display_func(void)
{
    dis_dynamic_refresh(10000);
    char bat_string[10] = {0};
    lcd_dis_bit_set(DISPLAY_OUT_1BIT);
    lcd_background_color_set(COLOR_BACKGROUND);

    uint8_t bat_soc;
    Adp5360_getBatSoC(&bat_soc);

    sprintf(bat_string,"%d%%",bat_soc);
    lygl_dis_string_middle(&lygl_font_47,104,150,COLOR_WHITE,bat_string);

    lygl_creat_battery(&bm_setting_battery_level_ico,78,38,1);

    lygl_battry_level(bat_soc);
    lcd_display_refresh_all();
}

static void key_handle(uint8_t key_value)
{
    switch(key_value)
    {
        case KEY_SELECT_SHORT:
        {

        }
        break;
        case KEY_NAVIGATION_SHORT:
        {
#ifdef USE_FS
            dis_page_jump(&page_memory_usage);
#else
            dis_page_jump(&page_power_off);
#endif
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

/*used to handle signal except key,
  for example
*/
static void signal_handle(uint8_t signal_value)
{
    switch(signal_value)
    {
        case DIS_REFRESH_SIGNAL:
        {
            char bat_string[10] = {0};
            lcd_background_color_set(COLOR_BACKGROUND);

            uint8_t bat_soc;
            Adp5360_getBatSoC(&bat_soc);

            sprintf(bat_string,"%d%%",bat_soc);
            lygl_dis_string_middle(&lygl_font_47,104,150,COLOR_WHITE,bat_string);

            lygl_battry_level(bat_soc);
            lcd_display_refresh_all();
        }
        break;
        case 1:
        {

        }
        break;
        /*can add other key handle*/
        default:break;
    }
}
const PAGE_HANDLE page_battery_level = {
.display_func = &display_func,
.key_handle = &key_handle,
.signal_handle = &signal_handle,
.page_type = DIS_STATIC_PAGE,
};
#endif