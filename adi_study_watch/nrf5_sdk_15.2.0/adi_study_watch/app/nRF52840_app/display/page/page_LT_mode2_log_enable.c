/**
****************************************************************************
* @file     page_LT_mode2_log_enable.c
* @author   ADI
* @version  V0.1
* @date     06-Apr-2021
* @brief    This is the source file used to start the LT mode2 logging
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
#ifdef LOW_TOUCH_FEATURE
#ifdef ENABLE_WATCH_DISPLAY
#include "display_app.h"
#include "lcd_driver.h"
#include "key_detect.h"
#include "lygl.h"
#include "image_declare.h"
#include "dcb_general_block.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "low_touch_task.h"

static bool gs_LT_mode2_log_display_status = false;     //!< flag used to track the LT mode2 log status for the display; used only in this file
static volatile bool g_LT_mode2_selection_status = false;  //!< flag used to track the LT mode2 log ON or OFF status set by the user; used by low touch task

extern ADI_OSAL_SEM_HANDLE   lt_task_evt_sem;

static void display_LT_mode2_log_config(void)
{
    char str_enab_status[10];
    lcd_dis_bit_set(DISPLAY_OUT_4BIT);
    lcd_background_color_set(COLOR_BACKGROUND);
    if(gs_LT_mode2_log_display_status)
      strcpy(str_enab_status,"ON");
    else
      strcpy(str_enab_status,"OFF");
    lygl_dis_string_middle(&lygl_font_47,104,74,COLOR_WHITE,"LOG EN");

    lygl_dis_string_middle(&lygl_font_48,104,140,COLOR_YELLOW,str_enab_status);
    lcd_display_refresh_all();
}
static void display_func(void)
{
    display_LT_mode2_log_config();
    dis_dynamic_refresh(500);
}

static void key_handle(uint8_t key_value)
{
    switch(key_value)
    {
        case KEY_SELECT_SHORT:
        {
          gs_LT_mode2_log_display_status = true;
          destroy_key_detect_task();      /* disable the button action*/
          display_LT_mode2_log_config();  /*Update the display status*/
          vTaskDelay(5000);               /*wait for watch stabilization*/
          g_LT_mode2_selection_status = true;
          adi_osal_SemPost(lt_task_evt_sem);  /*Wake up the low touch task*/
          gs_LT_mode2_log_display_status = false; /* reset the display status */
          dis_page_jump(&page_menu);  /* go back to main menu display page*/
        }
        break;
        case KEY_SELECT_LONG_VALUE:
        {
            dis_page_jump(&page_menu);
        }
        break;
        default:break;
    }
}

/*!
 ****************************************************************************
 * @brief    Returns the LT mode2 log selection status set by the user
 *           This flag get set by the user in the LT mode2 log enable page
 *           of the display
 *           Used by the low touch task to initiate the low touch logging
 * @param    None
 * @retval   True ---> Enabled; False ---> Disabled
 *****************************************************************************/
bool get_lt_mode2_selection_status()
{
  return(g_LT_mode2_selection_status);
}

/*!
 ****************************************************************************
 * @brief    Resets the LT mode2 log selection status
 *           Resets LT mode2 log selection status which was set by the user
 *           after starting the low touch log
 *           Used by the low touch task
 * @param    None
 * @retval   None
 *****************************************************************************/
void reset_lt_mode2_selection_status()
{
  g_LT_mode2_selection_status = false;
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
            display_LT_mode2_log_config();
        }
        break;
        /*can add other key handle*/
        default:break;
    }
}
const PAGE_HANDLE page_LT_mode2_log_enable = {
.display_func = &display_func,
.key_handle = &key_handle,
.signal_handle = &signal_handle,
.page_type = DIS_STATIC_PAGE,
};
#endif
#endif