/**
****************************************************************************
* @file     page_watch_id.c
* @author   ADI
* @version  V0.1
* @date     8-August-2021
* @brief    This is the source file used to display menu page.
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
#ifdef CUST4_SM
#ifdef ENABLE_WATCH_DISPLAY
#include <stdio.h>
#include "rtc.h"
#include "display_app.h"
#include "lcd_driver.h"
#include "key_detect.h"
#include "lygl.h"
#include "image_declare.h"
#include "nrf_log.h"
#include "adp5360.h"
#include "display_m2m2_protocol.h"
#include "low_touch_task.h"
#include "user0_config_app_task.h"

//#define DEBUG_INTERMITTENT_OP_PAGE 1

char hw_id_string[6] = {0};
char exp_id_string[8] = {0};

#ifdef DEBUG_INTERMITTENT_OP_PAGE
char rtc_string1[6] = {0};
#endif
static char memory_string1[6] = {0};
static uint8_t fs_display_cnt1 = 0;//for refresh fs dispaly every 1 minute.

static void usb_detect_func(uint8_t value,ADP5360_PGOOD_STATUS status)
{
    if((0 == value)&&(status.vbusok == 1))
    {
        dis_refresh_reset(1000);
    }
}

static uint8_t flash_vol_req_refresh1,refresh_cnt1=0,status_display1;
uint16_t fs_display_query_cnt1=0;

/* Flag to indicate flash-reset-event so that vol-info get updated on display */
static uint8_t flash_reset_evt1 =0;

/* Function prototype which returns the status of NAND flash log download process*/
bool get_download_status();

extern user0_config_app_lcfg_type_t user0_config_app_lcfg;

/*****************************************************************************
 * Function      : reset_display_vol_info
 * Description   : resets the display vol info to 0; updates the
                   flags and counters responsible for updating the vol info
                   so that display vol info gets updated in next context-switch
 * Input         : None
 * Output        : None
*****************************************************************************/

void reset_display_vol_info1()
{
  flash_reset_evt1  = 1; // flag to indicate flash reset/format event
  flash_vol_req_refresh1 = 0; //vol info updated on static flag;
  refresh_cnt1 = 5; // to simulate 5 min delay
}


#ifdef USE_FS
/*****************************************************************************
 * Function      : menu_fs_display
 * Description   : Display to status of file system in menu page.
 * Input         : void
 * Output        : None
 * Return        : static
 * Others        :
 * Record
 * 1.Date        : 20200615
 *   Author      : Leo
 *   Modification: Created function

*****************************************************************************/

static void menu_fs_display(void)
{
    uint32_t ret = M2M2_SUCCESS;
    uint8_t remain_level = 100;
    uint8_t status;

    if(!(refresh_cnt1 % 5))// update for 5 min
    {
      // if download in progress, consider previous status and value
      if(!get_download_status())
      {
        ret = m2m2_memory_usage_get(&remain_level);
        flash_vol_req_refresh1 = remain_level;
        status_display1 = ret;
        fs_display_query_cnt1 += 1;
      }
      refresh_cnt1 = 0;
    }

     //copy prev value
    remain_level = flash_vol_req_refresh1;

    USER0_CONFIG_APP_STATE_t read_user0_config_app_state = get_user0_config_app_state();
    if( ( (read_user0_config_app_state == STATE_START_MONITORING) ||
          (read_user0_config_app_state == STATE_INTERMITTENT_MONITORING) ||
          (read_user0_config_app_state == STATE_INTERMITTENT_MONITORING_START_LOG) ||
          (read_user0_config_app_state == STATE_INTERMITTENT_MONITORING_STOP_LOG) ||
          (read_user0_config_app_state == STATE_END_MONITORING) ) &&
          (M2M2_SUCCESS == status_display1) )
    {
         lcd_background_color_set_section(0,70,COLOR_BACKGROUND);
         lygl_process_bar_level(remain_level);
         sprintf(memory_string1,"%d%%",remain_level);
         lygl_dis_string_middle(&lygl_font_24,104,20,COLOR_WHITE,memory_string1);
    }
    else
    {
         lcd_background_color_set_section(0,70,COLOR_BACKGROUND);//not need display flash icon.
    }

    refresh_cnt1 += 1;
}
#endif

#ifdef DEBUG_INTERMITTENT_OP_PAGE
static void menu_time_display(m_time_struct *time)
{
    if(time->tm_hour >= 12)
    {
        if(time->tm_hour == 12)
        {
            sprintf(rtc_string1,"12:");
        }
        else
        {
            sprintf(rtc_string1,"%02d:",time->tm_hour-12);
        }
        lygl_dis_string(&lygl_font_24,150,130,COLOR_WHITE,"PM");
    }
    else
    {
        if(time->tm_hour == 0)
        {
            sprintf(rtc_string1,"12:");
        }
        else
        {
            sprintf(rtc_string1,"%02d:",time->tm_hour);
        }
        lygl_dis_string(&lygl_font_24,150,130,COLOR_WHITE,"AM");
    }

    sprintf(hw_id_string,"%02d",user0_config_app_lcfg.hw_id);
    sprintf(exp_id_string,"%04d",user0_config_app_lcfg.exp_id);
    lygl_dis_string_middle(&lygl_font_24,64,144,COLOR_WHITE,rtc_string1);
    lygl_dis_value_middle(&lygl_font_24,118,144,COLOR_YELLOW,time->tm_min,2);
    lygl_dis_string_middle(&lygl_font_24,80,160,COLOR_YELLOW,week_str[time->tm_wday]);
    lygl_dis_string_middle(&lygl_font_24,130,160,COLOR_WHITE,month_str[time->tm_mon]);
    lygl_dis_value_middle(&lygl_font_24,160,160,COLOR_WHITE,time->tm_mday,2);
    lygl_dis_string(&lygl_font_32,84,65,COLOR_WHITE,hw_id_string);
    lygl_dis_string(&lygl_font_62,44,85,COLOR_YELLOW,exp_id_string);
#else
static void menu_time_display()
{
    USER0_CONFIG_APP_STATE_t read_user0_config_app_state = get_user0_config_app_state();
    if((read_user0_config_app_state == STATE_START_MONITORING) ||
       (read_user0_config_app_state == STATE_INTERMITTENT_MONITORING) ||
       (read_user0_config_app_state == STATE_INTERMITTENT_MONITORING_START_LOG) ||
       (read_user0_config_app_state == STATE_INTERMITTENT_MONITORING_STOP_LOG) ||
       (read_user0_config_app_state == STATE_END_MONITORING))
    {
      sprintf(exp_id_string,"ID ");
      lygl_dis_string(&lygl_font_48,34,84,COLOR_YELLOW,exp_id_string);
      sprintf(exp_id_string,"%04d",user0_config_app_lcfg.exp_id);
      lygl_dis_string(&lygl_font_48,84,84,COLOR_YELLOW,exp_id_string);
    }

    sprintf(hw_id_string,"No.%02d",user0_config_app_lcfg.hw_id);
    lygl_dis_string(&lygl_font_32,80,140,COLOR_WHITE,hw_id_string);

#endif//DEBUG_INTERMITTENT_OP_PAGE
}

static void display_func(void)
{
    m_time_struct *dis_time;

    lcd_background_color_set(COLOR_BACKGROUND);

    lygl_creat_process_bar(&bm_memory_progressbar,80,36,0);

#ifdef DEBUG_INTERMITTENT_OP_PAGE
    dis_time = rtc_date_time_get();
#endif

#ifdef USE_FS
    menu_fs_display();
#endif


    if(0 == Adp5360_pgood_pin_status_get())
    {
        fs_display_cnt1 = 1;
    }
    else
    {
        fs_display_cnt1 = 60 - dis_time->tm_sec;
    }
    dis_dynamic_refresh(1000);

    Register_pgood_detect_func(usb_detect_func);

    menu_time_display(dis_time);

    lygl_creat_battery(&bm_battery_level_ico,30,152,0);

    uint8_t bat_soc;
    Adp5360_getBatSoC(&bat_soc);
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
          //Unregister_pgood_detect_func(usb_detect_func);
        }
        break;
        case KEY_SELECT_LONG_VALUE:
        {
          if(get_low_touch_trigger_mode2_status())
              dis_page_jump(&page_LT_mode2_log_enable);
        }
        break;
        /*can add other key handle*/
        default:break;
    }

}

/*used to handle signal except key,
  for example
*/
uint16_t min_timer_cnt1=0;
static void signal_handle(uint8_t signal_value)
{

    switch(signal_value)
    {
        case DIS_REFRESH_SIGNAL:
        {
            m_time_struct *dis_time;
            dis_time = rtc_date_time_get();

            lcd_background_color_set_section(70,207,COLOR_BACKGROUND);

            if((fs_display_cnt1 >= 60) || (flash_reset_evt1))
            {
                if(flash_reset_evt1)
                {
                  flash_reset_evt1 = 0;
#ifdef USE_FS
                  menu_fs_display();
#endif
                }
                else
                {
                  fs_display_cnt1 -= 60;
#ifdef USE_FS
                  menu_fs_display();
#endif
                }
                  min_timer_cnt1+=1;

            }
            if(0 == Adp5360_pgood_pin_status_get())
            {
                fs_display_cnt1++;
            }
            else
            {
                dis_refresh_reset(1000);
                fs_display_cnt1 += (60 - dis_time->tm_sec);
            }

            menu_time_display(dis_time);

            static BATTERY_STATUS_t battery_stat;
            static uint8_t bat_refresh_rate_cnt=0; //Battery details fetched from ADP5360 every 3 sec in the loop
            bat_refresh_rate_cnt++;
            if( bat_refresh_rate_cnt == 3 ) {
              bat_refresh_rate_cnt = 0;
            }
            Adp5360_get_battery_details(&battery_stat);//move to here, to relsove sync question.
            if( BATTERY_CHARGING == battery_stat.chrg_status ||
                BATTERY_CHARGER_LDO_MODE == battery_stat.chrg_status ||
                BATTERY_CHARGER_TIMER_EXPIRED == battery_stat.chrg_status )
            {
                if(battery_stat.level < 10)//add this if condition to prevent can't see charge status at low voltage.
                {
                    battery_stat.level = 10;
                }

                if( bat_refresh_rate_cnt%2 == 0)
                  lygl_battry_level(battery_stat.level);
                else
                  lygl_battry_level(0);
            }
            else if( BATTERY_NOT_CHARGING == battery_stat.chrg_status ||
                BATTERY_DETECTION == battery_stat.chrg_status )
            {
                lygl_battry_level(battery_stat.level);
            }
            else if( BATTERY_CHARGE_COMPLETE == battery_stat.chrg_status )
            {
                lygl_battry_level(100);
            }
            else if( BATTERY_NOT_AVAILABLE == battery_stat.chrg_status )
            {
                lygl_battry_level(0);
#ifdef ENABLE_WATCH_DISPLAY
                send_global_type_value(DIS_NO_BATTERY_ALARM);
#endif
            }
            else
            {
                lygl_battry_level(0);
            }
            //lcd_display_refresh_section(70,180);

            lcd_display_refresh_all();/* refresh all screen prevent the not refresh area display error*/
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
const PAGE_HANDLE page_watch_id = {
.display_func = &display_func,
.key_handle = &key_handle,
.signal_handle = &signal_handle,
.page_type = DIS_STATIC_PAGE,
};
#endif//ENABLE_WATCH_DISPLAY
#endif//CUST4_SM