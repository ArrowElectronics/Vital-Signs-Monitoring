/**
****************************************************************************
* @file     page_power_on.c
* @author   ADI
* @version  V0.1
* @date     11-Mar-2021
* @brief    This is the source file used to display power on page.
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
#include "low_touch_task.h"

static void display_func(void)
{
    lcd_background_color_set(COLOR_BLACK);

    lygl_draw_image(&bm_power_on_ico,10,72);
#ifdef CUST4_SM
    if(get_low_touch_trigger_mode3_status())
    {
      lygl_dis_string_middle(&lygl_font_32,104,88,COLOR_WHITE,"NOT FOR");
      lygl_dis_string_middle(&lygl_font_32,104,120,COLOR_WHITE,"CLINICAL");
      lygl_dis_string_middle(&lygl_font_32,104,152,COLOR_WHITE,"USE");
    }
    else
    {
#endif
      lygl_dis_string_middle(&lygl_font_32,124,88,COLOR_WHITE,"ANALOG");
      lygl_dis_string_middle(&lygl_font_32,124,120,COLOR_WHITE,"DEVICES");
#ifdef CUST4_SM
    }
#endif
    lcd_display_refresh_all();
}


const PAGE_HANDLE page_power_on = {
.display_func = &display_func,
.page_type = DIS_STATIC_PAGE,
};
#endif