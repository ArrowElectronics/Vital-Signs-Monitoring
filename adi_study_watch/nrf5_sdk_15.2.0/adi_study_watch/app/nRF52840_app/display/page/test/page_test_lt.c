/**
****************************************************************************
* @file     page_test_It.c
* @author   ADI
* @version  V0.1
* @date     11-Mar-2021
* @brief    This is a display file used to test low touch function.
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
#ifdef LOW_TOUCH_FEATURE
#ifdef ENABLE_LT_TEST_PAGE
#include "display_app.h"
#include "lcd_driver.h"
#include "key_detect.h"
#include "lygl.h"
#include "image_declare.h"

#include "ad7156.h"
#include "nrf_drv_gpiote.h"
#include "dcb_general_block.h"
#include "low_touch_task.h"

char str_array0[20];
char str_array1[30];
char str_array2[30];
char str_array3[30];
char str_array4[30];

static uint8_t gn_toggle_lt_cntrl = 0;
static uint16_t capVal;

extern volatile uint8_t gsCfgFileFoundFlag;

void lt_disp_str(char *str0, char *str1, char *str2);

static void touch_value_display(void)
{
    gn_toggle_lt_cntrl ? sprintf(str_array4, "LT:ON  ") : sprintf(str_array4, "LT:OFF ");
    sprintf(str_array2,"C:%04d ",capVal);
    lt_disp_str(str_array0, str_array1, str_array2+7);

    lcd_background_color_set(COLOR_BACKGROUND);
    lygl_dis_string_middle(&lygl_font_47,104,64,COLOR_WHITE,"LT_APP");
    lygl_dis_string_middle(&lygl_font_24,104,104,COLOR_WHITE,str_array1);//LowTouchAd7156IntCount, LowTouchAd7156IntValue, LowTouchAd7156CapVal from interrupt handler
    lygl_dis_string_middle(&lygl_font_24,104,124,COLOR_WHITE,str_array2);//LT ON/OFF, On/OFF Wrist, LowTouch running status
    lygl_dis_string_middle(&lygl_font_24,104,144,COLOR_WHITE,str_array0);//onWrCapValue, offWrCapValue

    if(gen_blk_get_dcb_present_flag())
       sprintf(str_array3,"LT cfg:DCB");
    else if(gsCfgFileFoundFlag)
       sprintf(str_array3,"LT cfg:NAND");
    else
        sprintf(str_array3,"LT cfg:None");
    lygl_dis_string_middle(&lygl_font_24,104,164,COLOR_WHITE,str_array3);//DCB/NAND/None Cfg

    lygl_dis_string_middle(&lygl_font_24,104,184,COLOR_WHITE,str_array4);//LT ON/OFF

    lcd_display_refresh_all();
}
static void display_func(void)
{
    lcd_background_color_set(COLOR_BLACK);

    gn_toggle_lt_cntrl = (gen_blk_get_dcb_present_flag() || gsCfgFileFoundFlag) ? 1 : 0;

    touch_value_display();
    dis_dynamic_refresh(1000);
}

static void key_handle(uint8_t key_value)
{
    switch(key_value)
    {
        case KEY_SELECT_SHORT:
        {
            capVal = AD7156_ReadChannelCap(2); // unit in fF
        }
        break;
        case KEY_SELECT_LONG_VALUE:
        {
            dis_page_jump(&page_menu);
        }
        break;
        case KEY_NAVIGATION_SHORT:
        {
            gn_toggle_lt_cntrl ^= 0x1;
            if(gn_toggle_lt_cntrl)
              EnableLowTouchDetection(true);
            else
              EnableLowTouchDetection(false);

        }
        break;
        default:break;
    }

}
static void signal_handle(uint8_t value)
{
    switch(value)
    {
        case DIS_REFRESH_SIGNAL:
        {
            touch_value_display();
        }
        break;
        default:break;
    }
}
const PAGE_HANDLE page_test_lt = {
.display_func = &display_func,
.key_handle = &key_handle,
.signal_handle = &signal_handle
};

#endif
#endif
#endif //ENABLE_WATCH_DISPLAY