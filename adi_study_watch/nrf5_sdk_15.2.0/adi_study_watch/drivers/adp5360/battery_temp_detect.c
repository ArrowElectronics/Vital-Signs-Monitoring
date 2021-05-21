/*! *****************************************************************************
    @file:    battery_temp_detect.c
    @brief:   Battery temperature detect functions
    -----------------------------------------------------------------------------
    Copyright (c) 2019 Analog Devices, Inc.

    All rights reserved.

    Redistribution and use in source and binary forms, with or without modification,
    are permitted provided that the following conditions are met:
    - Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
    - Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
    - Modified versions of the software must be conspicuously marked as such.
    - This software is licensed solely and exclusively for use with processors
    manufactured by or for Analog Devices, Inc.
    - This software may not be combined or merged with other code in any manner
    that would cause the software to become subject to terms and conditions
    which differ from those listed here.
    - Neither the name of Analog Devices, Inc. nor the names of its
    contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.
    - The use of this software may or may not infringe the patent rights of one
    or more patent holders.  This license does not release you from the
    requirement that you obtain separate licenses from these patent holders
    to use this software.

    THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES, INC. AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-
    INFRINGEMENT, TITLE, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL ANALOG DEVICES, INC. OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, PUNITIVE OR
    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, DAMAGES ARISING OUT OF
    CLAIMS OF INTELLECTUAL PROPERTY RIGHTS INFRINGEMENT; PROCUREMENT OF
    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.

*****************************************************************************/

#ifdef TEMP_PROTECT_FUNC_EN
#include "battery_temp_detect.h"
#include "adp5360.h"
#include "power_manager.h"
#include <stdbool.h>
#include "display_app.h"

#include <adi_osal.h>
#include <app_cfg.h>
#include <task_includes.h>

#define N8_DEGREE_THR_VALUE (2326) //impedance value when temperature equal -8 degree.
#define P55_DEGREE_THR_VALUE (212) //impedance value when temperature equal 55 degree.

ADI_OSAL_STATIC_THREAD_ATTR battery_temp_detect_task_attributes;
uint8_t battery_temp_detect_task_stack[APP_OS_BATTERY_TEMP_DETECT_TASK_STK_SIZE];
StaticTask_t battry_temp_detect_task_tcb;
ADI_OSAL_THREAD_HANDLE battery_temp_detect_task_handler;

/*****************************************************************************
 * Function      : battery_temp_detect_func
 * Description   : To resume the temperature detect task WHen detected the temperature interrupt.
 * Input         : uint8_t value
                ADP5360_INTERRUPT status
 * Output        : None
 * Return        :
 * Others        :
 * Record
 * 1.Date        : 20200623
 *   Modification: Created function

*****************************************************************************/
void battery_temp_detect_func(uint8_t value,ADP5360_INTERRUPT status)
{
    if((0 == value)&&(status.int1.thr_int == 1))
    {
        adi_osal_ThreadResumeFromISR(battery_temp_detect_task_handler);
    }
}

/*****************************************************************************
 * Function      : battery_temp_detect_thread
 * Description   : battery temperature detect thread
 * Input         : void * arg
 * Output        : None
 * Return        : static
 * Others        :
 * Record
 * 1.Date        : 20200623
 *   Modification: Created function

*****************************************************************************/
static void battery_temp_detect_thread(void * arg)
{
    uint16_t thr_value = 0;
    uint8_t sleep_flg = 0;
    uint8_t temp_high_flg = 0;
    ADP5360_CHG_STATUS2 bat_status;
    static ADP5360_CHG_STATUS2 bat_status_backup;
    UNUSED_PARAMETER(arg);

    Register_int_detect_func(battery_temp_detect_func);
    bat_status_backup.thr_status = ADP5360_THERM_OK;
    while(1)
    {
        sleep_flg = 0xff;
        Adp5360_getChargerStatus2(&bat_status);
        switch (bat_status.thr_status)
        {
            case ADP5360_THERM_COLD:
            {
                Adp5360_get_THR_Voltage(&thr_value);
                if(thr_value > N8_DEGREE_THR_VALUE)
                {
                    enter_poweroff_mode();
                }
                else
                {
                    if(ADP5360_THERM_COLD != bat_status_backup.thr_status)//prevent send alarm many times.
                    {
#ifdef ENABLE_WATCH_DISPLAY
                        send_global_type_value(DIS_TMP_LOW_ALARM);
#endif
                    }
                }
                sleep_flg = 0;
                temp_high_flg = 0;
            }
            break;

            case ADP5360_THERM_WARM:
            {
                Adp5360_get_THR_Voltage(&thr_value);
                if(thr_value < P55_DEGREE_THR_VALUE)
                {
                    if(0 == temp_high_flg)//prevent send alarm many times.
                    {
#ifdef ENABLE_WATCH_DISPLAY
                        send_global_type_value(DIS_TMP_HIGH_ALARM);
#endif
                    }
                    temp_high_flg = 0xff;
                }
                else
                {
                    if((0 != temp_high_flg)||(ADP5360_THERM_COLD == bat_status_backup.thr_status))
                    {
#ifdef ENABLE_WATCH_DISPLAY
                        dis_page_back();
#endif
                    }
                    temp_high_flg = 0;
                }
                sleep_flg = 0;
            }
            break;
            case ADP5360_THERM_HOT:
            {
                //send_global_type_value(DIS_TMP_OVER_60_ALARM);
                enter_poweroff_mode();
            }
            break;
            case ADP5360_THERM_COOL:
            case ADP5360_THERM_OK:
            {
                if((0 != temp_high_flg)||(ADP5360_THERM_COLD == bat_status_backup.thr_status))
                {
#ifdef ENABLE_WATCH_DISPLAY
                    dis_page_back();
#endif
                }
                temp_high_flg = 0;
            }
            break;
            default:break;
        }
        bat_status_backup.thr_status = bat_status.thr_status;

        if(0 != sleep_flg)
        {
            adi_osal_ThreadSuspend(NULL);
        }
        else
        {
            vTaskDelay(10000);
        }
    }
}

/*****************************************************************************
 * Function      : battery_temp_detect_task_init
 * Description   : battery temperature detect task initialize
 * Input         : void
 * Output        : None
 * Return        :
 * Others        :
 * Record
 * 1.Date        : 20200623
 *   Modification: Created function

*****************************************************************************/
void battery_temp_detect_task_init(void)
{
    ADI_OSAL_STATUS eOsStatus;

    /* Create battery temp detect thread */
    battery_temp_detect_task_attributes.pThreadFunc = battery_temp_detect_thread;
    battery_temp_detect_task_attributes.nPriority = APP_OS_BATTERY_TEMP_DETECT_TASK_PRIO;
    battery_temp_detect_task_attributes.pStackBase = &battery_temp_detect_task_stack[0];
    battery_temp_detect_task_attributes.nStackSize = APP_OS_BATTERY_TEMP_DETECT_TASK_STK_SIZE;
    battery_temp_detect_task_attributes.pTaskAttrParam = NULL;
    battery_temp_detect_task_attributes.szThreadName = "tmp_detect";
    battery_temp_detect_task_attributes.pThreadTcb = &battry_temp_detect_task_tcb;

    eOsStatus = adi_osal_ThreadCreateStatic(&battery_temp_detect_task_handler,
                                &battery_temp_detect_task_attributes);
    if (eOsStatus != ADI_OSAL_SUCCESS)
    {
        Debug_Handler();
    }
}
#endif



