/**
****************************************************************************
* @file     page_low_touch_logging_status.c
* @author   ADI
* @version  V0.1
* @date     11-Mar-2021
* @brief    This is the source file used to display low touch logging status.
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
#include "display_app.h"
#include "lcd_driver.h"
#include "key_detect.h"
#include "lygl.h"
#include "image_declare.h"
#include "display_m2m2_protocol.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
static void display_low_touch_logging_status(void)
{
    uint8_t status;
    uint32_t err;
    lcd_background_color_set(COLOR_BACKGROUND);
    lygl_dis_string_middle(&lygl_font_47,104,74,COLOR_WHITE,"STATUS");

    err = m2m2_check_logging_status(&status);
    APP_ERROR_CHECK(err);
    if(M2M2_PM_SYS_STATUS_LOW_TOUCH_LOGGING_IN_PROGRESS == status)
    {
        lygl_dis_string_middle(&lygl_font_48,104,140,COLOR_YELLOW,"started");
    }
    else if(M2M2_PM_SYS_STATUS_LOW_TOUCH_LOGGING_NOT_STARTED == status)
    {
        lygl_dis_string_middle(&lygl_font_48,104,140,COLOR_YELLOW,"stopped");
    }
    else
    {
        lygl_dis_string_middle(&lygl_font_48,104,140,COLOR_YELLOW,"error");
    }

    lcd_display_refresh_all();
}

static void display_func(void)
{
    display_low_touch_logging_status();

    dis_dynamic_refresh(5000);
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
            dis_page_jump(&page_low_touch_config);
        }
        break;
        case KEY_SELECT_LONG_VALUE:
        {
            dis_page_jump(&page_low_touch_logging);
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
            display_low_touch_logging_status();
        }
        break;
        /*can add other key handle*/
        default:break;
    }
}
const PAGE_HANDLE page_logging_status = {
.display_func = &display_func,
.key_handle = &key_handle,
.signal_handle = &signal_handle,
.page_type = DIS_STATIC_PAGE,
};
#endif //LOW_TOUCH_FEATURE
#endif