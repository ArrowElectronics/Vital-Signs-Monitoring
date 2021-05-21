/**
****************************************************************************
* @file     page_test_touch.c
* @author   ADI
* @version  V0.1
* @date     11-Mar-2021
* @brief    This is a display file used to test touch function.
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
#ifdef ENABLE_TOUCH_TEST_PAGE
#include "display_app.h"
#include "lcd_driver.h"
#include "key_detect.h"
#include "lygl.h"
#include "image_declare.h"

#include "ad7156.h"
#include "nrf_drv_gpiote.h"

char up_touch_array[30];
char down_touch_array[30];
static uint8_t level1 = 0;
static uint8_t level2 = 0;
void touch_detect_func1(uint8_t value)
{
    level1++;
}
void touch_detect_func2(uint8_t value)
{
    level2++;
}
static void touch_value_display(void)
{
    uint16_t val1,val2;

    float value1,value2;

    AD7156_ReadCapacitance(&val1,&val2);
//    val1 = (uint32_t) (value1*1000);//up
//    val2 = (uint32_t) (value2*1000);//down

    sprintf(up_touch_array,"up val:%04d level:%d",val1,level1);
    sprintf(down_touch_array,"down val:%04d level:%d",val2,level2);

    lcd_background_color_set_section(84,160,COLOR_BACKGROUND);
    lygl_dis_string_middle(&lygl_font_24,104,104,COLOR_WHITE,up_touch_array);
    lygl_dis_string_middle(&lygl_font_24,104,144,COLOR_WHITE,down_touch_array);
    lcd_display_refresh_section(84,160);

}
static void display_func(void)
{
    lcd_background_color_set(COLOR_BLACK);

    lygl_dis_string_middle(&lygl_font_47,104,64,COLOR_WHITE,"TOUCH");

    lcd_display_refresh_all();
    dis_dynamic_refresh(500);

    Register_out1_pin_detect_func(touch_detect_func1);
    Register_out2_pin_detect_func(touch_detect_func2);

}

static void key_handle(uint8_t key_value)
{
    switch(key_value)
    {
        case KEY_SELECT_LONG_VALUE:
        {
            //dis_page_jump(&page_test_entry);
        }
        break;
        case KEY_NAVIGATION_SHORT:
        {
            //dis_page_jump(&page_memory);
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
            touch_value_display();
        }
        break;
        case 1:
        {
            //value_t = dis_test_value;
            //lygl_send_graph_data(&value_t,1);

        }
        break;
        default:break;
    }
}
const PAGE_HANDLE page_test_touch = {
.display_func = &display_func,
.key_handle = &key_handle,
.signal_handle = &signal_handle
};

#endif
#endif //ENABLE_WATCH_DISPLAY