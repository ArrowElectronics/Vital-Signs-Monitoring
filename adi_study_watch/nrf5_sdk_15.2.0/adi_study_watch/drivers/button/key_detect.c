/**
****************************************************************************
* @file     key_detect.c
* @author   ADI
* @version  V0.1
* @date     20-May-2020
* @brief    This source file is used to implement key detect functions
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
*   or more patent holders.  This license does not release you from the
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

#include "key_detect.h"
#ifdef VSM_MBOARD
#include "watch_vsm_motherboard_pin_config.h"
#elif PCBA
#include "watch_board_pcba_pin_config.h"
#else
#include "watch_board_evt_pin_config.h"
#endif
//#include "app_timer.h"
#include "nrf_drv_gpiote.h"
#include "app_error.h"
#include "sdk_common.h"
#include "ble_task.h"
#include "file_system_utils.h"

#include <adi_osal.h>
#include <app_cfg.h>
#include <task_includes.h>
ADI_OSAL_STATIC_THREAD_ATTR key_task_attributes;
uint8_t key_task_stack[APP_OS_CFG_KEY_TASK_STK_SIZE];
StaticTask_t key_task_tcb;
ADI_OSAL_THREAD_HANDLE key_task_handler;
#include "nrf_log.h"
#if NRF_LOG_ENABLED
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#endif

#define KEY_USER_MAX (2)  //!< Maximum number of key handlers can be registered

/*------------------------- Private Function Prototypes ----------------------*/
static Send_key_func key_user_handle[KEY_USER_MAX] = {NULL};

/**
  * @brief  Registers the handler into the key user handler array
  * @param  handler: handler to be added into the key user handler array
  * @retval None
  */
void Register_key_send_func(Send_key_func handler)
{
    for(int i = 0;i<KEY_USER_MAX;i++)
    {
        if(NULL == key_user_handle[i])
        {
            key_user_handle[i] = handler;
            break;
        }
    }
}

/**
  * @brief  Removes the handler from key user handler array
  * @param  handler: handler to be removed from the key user handler array
  * @retval None
  */

void Unregister_key_send_func(Send_key_func handler)
{
    for(int i = 0;i<KEY_USER_MAX;i++)
    {
        if(handler == key_user_handle[i])
        {
            key_user_handle[i] = NULL;
            break;
        }
    }
}

/**
  * @brief  Clears the key user handler array
  * @param  None
  * @retval None
  */

void Clear_register_key_func(void)
{
    for(int i = 0;i<KEY_USER_MAX;i++)
    {
        key_user_handle[i] = NULL;
    }
}

/**
  * @brief Returns key value based on the key(s) pressed
  * @param  None
  * @retval uint8_t 0: No key is pressed
  * @retval  KEY_SELECT_VALUE: when only Select key is pressed
  * @retval  KEY_NAVIGATION_VALUE: when only Navigation key is pressed
  * @retval  (KEY_SELECT_VALUE | KEY_NAVIGATION_VALUE): when both the keys are pressed
  */
uint8_t key_value_get(void)
{
    uint8_t value = 0;
    if(false == nrf_drv_gpiote_in_is_set(KEY_SELECT_PIN))
    {
            value |= KEY_SELECT_VALUE;
    }
    if(false == nrf_drv_gpiote_in_is_set(KEY_NAVIGATIONPIN))
    {
            value |= KEY_NAVIGATION_VALUE;
    }
    return value;
}

/** @brief  gpio interrupt callback handler
*
* @param  pin pin number of which gpio event is triggered
* @param  action polarity of the gpio pin changes
* @return None
*/
static void gpiote_event_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    if(0 == nrf_drv_gpiote_in_is_set(pin))
    {
        adi_osal_ThreadResumeFromISR(key_task_handler);
//        NRF_LOG_INFO("have key press!");
    }
}

/** @brief  Initializes the gpios required for key press detection
*
* @param  None
* @retval uint32_t 0: SUCCESS, 1: ERROR
*/
uint32_t key_port_init(void)
{
    uint32_t err_code = NRF_SUCCESS;

    nrf_drv_gpiote_in_config_t config = GPIOTE_CONFIG_IN_SENSE_TOGGLE(false);

    if (!nrf_drv_gpiote_is_init())
    {
        err_code = nrf_drv_gpiote_init();
        if(NRF_SUCCESS != err_code)
        {
            NRF_LOG_INFO("key_detect gpiote init failue");
            return err_code;
        }
    }

    config.pull = NRF_GPIO_PIN_PULLUP;
    err_code = nrf_drv_gpiote_in_init(KEY_SELECT_PIN, &config, gpiote_event_handler);
    if(NRF_SUCCESS != err_code)
    {
        NRF_LOG_INFO("KEY_SELECT_PIN init failue,err_code = %d",err_code);
        return err_code;
    }

    config.pull = NRF_GPIO_PIN_PULLUP;
    err_code = nrf_drv_gpiote_in_init(KEY_NAVIGATIONPIN, &config, gpiote_event_handler);
    if(NRF_SUCCESS != err_code)
    {
        NRF_LOG_INFO("KEY_NAVIGATIONPIN init failue,err_code = %d",err_code);
        return err_code;
    }

    nrf_drv_gpiote_in_event_enable(KEY_SELECT_PIN, true);
    nrf_drv_gpiote_in_event_enable(KEY_NAVIGATIONPIN, true);
    return err_code;
}

/*
* @note: combination short press can't be detect, because it very hard to keep release at the same time.
         but we don't need short press, so ignore it for the moment.
*/

/*!
 **************************************************************************************
 * @brief    Identifies the key pressed and calls the corresponding handlers registered
 * @param    arg not used
 * @retval   None
 *************************************************************************************/
void key_detect_thread(void * arg)
{
    static uint8_t key_value = 0;
    static uint8_t last_key_value = 0;
    static uint16_t key_cnt = 0;
    UNUSED_PARAMETER(arg);

    key_port_init();
    adi_osal_ThreadSuspend(NULL);
    while(true)
    {
        key_value = key_value_get();
        if(last_key_value == key_value)
        {
            if(0 != key_value)
            {
                key_cnt++;
                if(key_cnt > LONG_PRESS_TIMEOUT_MS)
                {
                    key_cnt = 0;
                    key_value |= (APP_KEY_LONG_PUSH << 4);
                    /* reset operation not rely the other task*/
                    if(key_value == KEY_NAVIGATION_LONG_VALUE)
                    {
                        /* if logging is in progress , close file */
                        if(UpdateFileInfo() == true){
                          NRF_LOG_INFO("Success file close ");
                        }
                        else {
                          /* failure */
                          NRF_LOG_INFO("Error file close");
                        }
                        rtc_timestamp_store(320);
                        NVIC_SystemReset();
                    }
#ifndef ENABLE_WATCH_DISPLAY
                    if(key_value == KEY_COMBINATION_LONG_VALUE)
                    {
                        enter_bootloader_and_restart();


                    }
#endif
                    for(int i = 0;i<KEY_USER_MAX;i++)
                    {
                        if(NULL != key_user_handle[i])
                        {
                            key_user_handle[i](key_value);
                        }

                    }
                    key_value = 0;//if detect the long press, then stop detect.
                }
            }
        }
        else
        {
            if((key_cnt > SHORT_PRESS_TIME_MS)&&(key_value == 0))
            {
                for(int i = 0;i<KEY_USER_MAX;i++)
                {
                    if(NULL != key_user_handle[i])
                    {
                        key_user_handle[i](last_key_value|(APP_KEY_RELEASE << 4));
                    }

                }
            }
            key_cnt = 0;
        }
        last_key_value = key_value;
        if(0x00 == key_value)//supend the task when no key press
        {
            adi_osal_ThreadSuspend(NULL);
        }
        adi_osal_ThreadSleep(10);
    }
}

/*!
 ****************************************************************************
 * @brief    Destroys the key/button action detection task
 * @param    None
 * @retval   None
 *****************************************************************************/
void destroy_key_detect_task(void)
{
  adi_osal_ThreadDestroy(key_task_handler);
}

/*!
 ****************************************************************************
 * @brief key detection task initialization
 * @param[in]     None
 * @return        None
 *****************************************************************************/
void key_detect_init(void) {
  ADI_OSAL_STATUS eOsStatus;

  /* Create USBD tx thread */
  key_task_attributes.pThreadFunc = key_detect_thread;
  key_task_attributes.nPriority = APP_OS_CFG_KEY_TASK_PRIO;
  key_task_attributes.pStackBase = &key_task_stack[0];
  key_task_attributes.nStackSize = APP_OS_CFG_KEY_TASK_STK_SIZE;
  key_task_attributes.pTaskAttrParam = NULL;
  key_task_attributes.szThreadName = "key";
  key_task_attributes.pThreadTcb = &key_task_tcb;

  eOsStatus = adi_osal_ThreadCreateStatic(&key_task_handler,
                                    &key_task_attributes);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
      Debug_Handler();
  }
}
