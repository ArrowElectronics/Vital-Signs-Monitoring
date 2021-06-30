/**
****************************************************************************
* @file     display_app.c
* @author   ADI
* @version  V0.1
* @date     11-Mar-2021
* @brief    This is the source file used to do display task handler
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
#include "display_interface.h"

#include <post_office.h>
#include <adi_osal.h>
#include <app_cfg.h>
#include <task_includes.h>

#include "nrf_drv_gpiote.h"
#include "key_detect.h"
#include "touch_detect.h"
#include "app_timer.h"
#include "lcd_driver.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "ble_task.h"
#include "key_test.h"
#include "low_touch_task.h"
#ifdef HIBERNATE_MD_EN
#include "power_manager.h"
#endif

ADI_OSAL_STATIC_THREAD_ATTR display_task_attributes;
uint8_t display_task_stack[APP_OS_CFG_DISPLAY_TASK_STK_SIZE];
StaticTask_t display_task_tcb;
ADI_OSAL_QUEUE_HANDLE  display_task_msg_queue = NULL;
ADI_OSAL_THREAD_HANDLE dispaly_task_handler;


//static QueueHandle_t xDisplayQueue = NULL;

const PAGE_HANDLE *Current_page = NULL;
const PAGE_HANDLE *Backup_page = NULL;
APP_TIMER_DEF(m_dis_refresh_tmr);
APP_TIMER_DEF(m_backlight_tmr);

uint8_t get_user_backlight_flag(void);

extern void change_sd_task_prio();

static void backlight_timeout_handler(void * p_context)
{
    if(Current_page->page_type == DIS_STATIC_PAGE)//dynamic page not need close backlight
    {
        lcd_backlight_set(LCD_BACKLIGHT_OFF);
    }
#ifdef HIBERNATE_MD_EN
    hibernate_md_set(HIB_MD_NO_KEY_PRESS_EVT);
    hibernate_mode_entry();
#endif
}

static void turn_on_backlight(void)
{
    if(0 != get_user_backlight_flag())
    {
        NRF_LOG_INFO("turn_on_backlight");
        app_timer_stop(m_backlight_tmr);
        if(Current_page->page_type == DIS_STATIC_PAGE)
        {
            app_timer_start(m_backlight_tmr, APP_TIMER_TICKS(30000), NULL);
        }
        else
        {
            app_timer_start(m_backlight_tmr, APP_TIMER_TICKS(60000), NULL);
        }
        lcd_backlight_set(LCD_BACKLIGHT_ON);
    }

}

static void send_touch_value(uint8_t  touch_value)
{
    if(TOUCH_TOP_SHORT == touch_value)
    {
        turn_on_backlight();
    }
}

static void send_signal_to_display(uint8_t type,uint8_t sub_type)
{
    m2m2_hdr_t *p_m2m2_ = NULL;
    display_signal_t *signal;

    p_m2m2_ = post_office_create_msg(M2M2_HEADER_SZ+sizeof(display_signal_t));
    if (p_m2m2_ == NULL) {
      return;
    }

    p_m2m2_->src = M2M2_ADDR_DISPLAY;
    p_m2m2_->dest = M2M2_ADDR_DISPLAY;

    signal = (display_signal_t *)p_m2m2_->data;
    signal->signal_type = type;
    signal->signal_value = sub_type;

    send_message_display_task(p_m2m2_);//send to display task directly.
}

void send_key_value(uint8_t  k_value)
{
    send_signal_to_display(DIS_KEY_SIGNAL,k_value);
}

void send_global_type_value(uint8_t sub_type)
{
    send_signal_to_display(DIS_GLOBLE_SIGNAL,sub_type);
}

void send_private_type_value(uint8_t value)
{
    send_signal_to_display(DIS_PRIVATE_SIGNAL,value);
}

static void dis_refresh_timeout_handler(void * p_context)
{
    send_private_type_value(DIS_REFRESH_SIGNAL);
}


void dis_dynamic_refresh(uint16_t refresh_ms)
{
    app_timer_start(m_dis_refresh_tmr, APP_TIMER_TICKS(refresh_ms), NULL);
}

void dis_refresh_reset(uint16_t refresh_ms)
{
    app_timer_stop(m_dis_refresh_tmr);
    app_timer_start(m_dis_refresh_tmr, APP_TIMER_TICKS(refresh_ms), NULL);
}

void dis_page_jump(const PAGE_HANDLE *page)
{
    app_timer_stop(m_dis_refresh_tmr);
    Current_page = page;
    Current_page->display_func();
}

void dis_page_back(void)
{
//    app_timer_stop(m_dis_refresh_tmr);
    if(NULL != Backup_page)
    {
        Current_page = Backup_page;
        Backup_page = NULL;
        Current_page->display_func();
    }
}

void display_page_init(void)
{
    uint32_t err_code = NRF_SUCCESS;
    err_code = app_timer_create(&m_dis_refresh_tmr,APP_TIMER_MODE_REPEATED,dis_refresh_timeout_handler);
    if(NRF_SUCCESS != err_code)
    {
        return;
    }
    err_code = app_timer_create(&m_backlight_tmr,APP_TIMER_MODE_SINGLE_SHOT,backlight_timeout_handler);
    if(NRF_SUCCESS != err_code)
    {
        return;
    }
    lcd_dis_bit_set(DISPLAY_OUT_4BIT);
    Current_page = &page_power_on;
    Current_page->display_func();
    turn_on_backlight();
    //fds_rtc_init();
    adi_osal_ThreadSleep(3000);//display 3s log page.
    Current_page = &page_menu;
    Current_page->display_func();
    //Change the SD task prio, after display init is completed
    change_sd_task_prio();
}

void send_message_display_task(m2m2_hdr_t *p_pkt) {
  ADI_OSAL_STATUS osal_result;
  osal_result = adi_osal_MsgQueuePost(display_task_msg_queue,p_pkt);
  if (osal_result != ADI_OSAL_SUCCESS)
    post_office_consume_msg(p_pkt);
}
static void display_thread(void * arg)
{
//    ret_code_t ret;
    ADI_OSAL_STATUS                       err;
    display_signal_t *display_signal;
    m2m2_hdr_t              *pkt;
    UNUSED_PARAMETER(arg);

    lcd_port_init();
    lcd_buffer_init();
    display_page_init();
    Register_key_send_func(send_key_value);
    //key_detect_init();
#ifdef ENABLE_TOP_TOUCH
    Register_touch_send_func(send_touch_value);
#endif
    //touch_detect_init();
    // Enter main loop.
    for (;;)
    {
        pkt = post_office_get(ADI_OSAL_TIMEOUT_FOREVER, APP_OS_CFG_DISPLAY_TASK_INDEX);
        if((pkt == NULL)||(pkt->data == NULL))
        {
            NRF_LOG_INFO("display_thread:pkt = NULL!");
            continue;
        }
        if(pkt->src == M2M2_ADDR_DISPLAY)
        {
            display_signal = (display_signal_t *)pkt->data;
            switch(display_signal->signal_type)
            {
                case DIS_KEY_SIGNAL:
                {
                    if(KEY_COMBINATION_LONG_VALUE == display_signal->signal_value)
                    {

                        enter_bootloader_and_restart();
                    }


                    if(NULL != Current_page->key_handle)
                    {
                        Current_page->key_handle(display_signal->signal_value);
                        turn_on_backlight();

                        if(Current_page->page_type == DIS_STATIC_PAGE)
                        {
#ifdef ENABLE_TOP_TOUCH
                            lcd_extcomin_status_set(DISPLAY_REVERSE_LOW);
                            top_touch_func_set(1);//enable touch function
#else
                            lcd_extcomin_status_set(DISPLAY_REVERSE_HIGH);
#endif
                        }
                        else
                        {
                            lcd_extcomin_status_set(DISPLAY_REVERSE_HIGH);
                            top_touch_func_set(0);//disable touch function
                        }

#ifdef HIBERNATE_MD_EN
                        hibernate_md_clear(HIB_MD_NO_KEY_PRESS_EVT);
                        hibernate_mode_entry();
#endif
                    }
                }
                break;
                case DIS_PRIVATE_SIGNAL:
                {
                    if(NULL != Current_page->signal_handle)
                    {
                        Current_page->signal_handle(display_signal->signal_value);
                    }
                }
                break;
                case DIS_GLOBLE_SIGNAL:
                {
                    switch(display_signal->signal_value)
                    {
#ifdef BLE_PEER_ENABLE
                        case DIS_BLE_PEER_REQUEST:
                        {
                            if(NULL == Backup_page)
                            {
                                Backup_page = Current_page;
                                dis_page_jump(&page_ble_peer_request);
                            }
                        }
                        break;
#endif
#ifdef TEMP_PROTECT_FUNC_EN
                        case DIS_TMP_HIGH_ALARM:
                        {
                            if(NULL == Backup_page)
                            {
                                Backup_page = Current_page;
                                dis_page_jump(&page_tmp_high_alarm);
                            }
                        }
                        break;
                        case DIS_TMP_LOW_ALARM:
                        {
                            if(NULL == Backup_page)
                            {
                                Backup_page = Current_page;
                                dis_page_jump(&page_tmp_low_alarm);
                            }
                        }
                        break;
#endif
                        case DIS_VOLTAGE_LOW_ALARM:
                        {
                            if(NULL == Backup_page)
                            {
                                Backup_page = Current_page;
                                dis_page_jump(&page_low_voltage_alarm);
                            }
                        }
                        break;
                        case DIS_MAX_FILE_ALARM:
                        {
                            if(NULL == Backup_page)
                            {
                                Backup_page = Current_page;
                                dis_page_jump(&page_max_file_alarm);
                            }
                        }
                        break;
                        case DIS_NO_BATTERY_ALARM:
                        {
                            if(NULL == Backup_page)
                            {
                                Backup_page = Current_page;
                                dis_page_jump(&page_no_battery_error);
                            }
                        }
                        break;
                        case DIS_LOW_TOUCH_ALARM:
                        {
                            if(NULL == Backup_page)
                            {
                                Backup_page = Current_page;
                                dis_page_jump(&page_low_touch_alarm);
                            }
                        }
                        break;
                        default:break;
                    }
                }
                break;
                /*add other type signal at here*/
                default:break;
            }

        }//if(pkt->src == M2M2_ADDR_DISPLAY)
        else
        {
            if(NULL != Current_page->m2m2_protocol_handle)
            {
                Current_page->m2m2_protocol_handle(pkt);
            }

            _m2m2_app_common_cmd_t *ctrl_cmd = NULL;
            m2m2_hdr_t *response_mail = NULL;
            ctrl_cmd = (_m2m2_app_common_cmd_t*)&pkt->data[0];
            switch (ctrl_cmd->command)
            {
                case M2M2_DISPLAY_APP_CMD_SET_DISPLAY_REQ:
                {
                    response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_display_set_command_t));
                    if (response_mail != NULL)
                    {
                        m2m2_display_set_command_t *req = (m2m2_display_set_command_t *)&pkt->data[0];
                        m2m2_display_set_command_t *resp = (m2m2_display_set_command_t *)&response_mail->data[0];
                        /*TODO*/
                        app_timer_stop(m_dis_refresh_tmr);
                        lcd_dis_bit_set(DISPLAY_OUT_4BIT);
                        switch(req->colour)
                        {
                            case M2M2_DISPLAY_SET_WHITE:
                            {
                                lcd_background_color_set(COLOR_WHITE);
                            }
                            break;
                            case M2M2_DISPLAY_SET_BLACK:
                            {
                                lcd_background_color_set(COLOR_BLACK);
                            }
                            break;
                            case M2M2_DISPLAY_SET_RED:
                            {
                                lcd_background_color_set(COLOR_RED);
                            }
                            break;
                            case M2M2_DISPLAY_SET_GREEN:
                            {
                                lcd_background_color_set(COLOR_GREEN);
                            }
                            break;
                            case M2M2_DISPLAY_SET_BLUE:
                            {
                                lcd_background_color_set(COLOR_BLUE);
                            }
                            break;
                            default:
                            {
                                if(NULL != Current_page->display_func)
                                {
                                    Current_page->display_func();
                                }
                            }break;
                        }

                        lcd_display_refresh_all();

                        /* send response packet */
                        response_mail->src = pkt->dest;
                        response_mail->dest = pkt->src;
                        resp->command = M2M2_DISPLAY_APP_CMD_SET_DISPLAY_RESP;
                        resp->status = M2M2_APP_COMMON_STATUS_OK;
                        resp->colour = req->colour;
                        NRF_LOG_INFO("Received Set Display Colour Command");
                        post_office_send(response_mail, &err);
                    }
                }
                break;
                case M2M2_DISPLAY_APP_CMD_BACKLIGHT_CNTRL_REQ:
                {
                    response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_backlight_cntrl_command_t));
                    if (response_mail != NULL)
                    {
                        m2m2_backlight_cntrl_command_t *resp = (m2m2_backlight_cntrl_command_t *)&response_mail->data[0];
                        m2m2_backlight_cntrl_command_t *req = (m2m2_backlight_cntrl_command_t *)&pkt->data[0];
                        /*TODO*/
                        lcd_backlight_set(req->control);
                        /* send response packet */
                        response_mail->src = pkt->dest;
                        response_mail->dest = pkt->src;
                        resp->command = M2M2_DISPLAY_APP_CMD_BACKLIGHT_CNTRL_RESP;
                        resp->status = M2M2_APP_COMMON_STATUS_OK;
                        resp->control = req->control;
                        NRF_LOG_INFO("Received Set Backlight control Command");
                        post_office_send(response_mail, &err);
                    }
                }
                break;
                case M2M2_DISPLAY_APP_CMD_KEY_TEST_REQ:
                {
                    key_test_func(pkt);
                }
                break;
                default:break;
            }//switch
        }
        post_office_consume_msg(pkt);
    }
}




void display_app_init(void) {
  ADI_OSAL_STATUS eOsStatus;

  /* Create USBD tx thread */
  display_task_attributes.pThreadFunc = display_thread;
  display_task_attributes.nPriority = APP_OS_CFG_DISPLAY_TASK_PRIO;
  display_task_attributes.pStackBase = &display_task_stack[0];
  display_task_attributes.nStackSize = APP_OS_CFG_DISPLAY_TASK_STK_SIZE;
  display_task_attributes.pTaskAttrParam = NULL;
  display_task_attributes.szThreadName = "display";
  display_task_attributes.pThreadTcb = &display_task_tcb;
  eOsStatus = adi_osal_MsgQueueCreate(&display_task_msg_queue,NULL,
                                    50);//increase this value.
  if (eOsStatus != ADI_OSAL_SUCCESS) {
      Debug_Handler();
  }
  else
  {
    update_task_queue_list(APP_OS_CFG_DISPLAY_TASK_INDEX,display_task_msg_queue);
  }
  eOsStatus = adi_osal_ThreadCreateStatic(&dispaly_task_handler,
                                    &display_task_attributes);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
      Debug_Handler();
  }

  //MCU_HAL_Delay(500);
  //nrf_delay_ms(500);not need this delay.
}
#endif