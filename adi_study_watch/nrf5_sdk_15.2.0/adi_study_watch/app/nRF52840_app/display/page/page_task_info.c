/**
****************************************************************************
* @file     page_task_info.c
* @author   ADI
* @version  V0.1
* @date     11-Mar-2021
* @brief    This is the source file used to display task information
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
//#include "ADP5360.h"
#include "lygl.h"


#include "FreeRTOS.h"
#if ( configGENERATE_RUN_TIME_STATS == 1 )

#if 1
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#endif
#include "task.h"
#include "semphr.h"

#define MAX_TASK_NUM        10
TaskStatus_t pxTaskStatusArray[MAX_TASK_NUM];

void print_task_state(void)
{
    const char task_state[]={'r','R','B','S','D'};
    UBaseType_t uxArraySize, x;
    uint32_t ulTotalRunTime,ulStatsAsPercentage;

    /* 获取任务总数目 */
    uxArraySize = uxTaskGetNumberOfTasks();
   if(uxArraySize>MAX_TASK_NUM)
    {
        NRF_LOG_INFO("task number over");
    }

    /*获取每个任务的状态信息 */
    uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, &ulTotalRunTime );

    NRF_LOG_INFO("name,status,ID,prio,stack,cpu");

    /* 避免除零错误 */
    if( ulTotalRunTime > 0 )
    {
        /* 将获得的每一个任务状态信息部分的转化为程序员容易识别的字符串格式 */
        for( x = 0; x < uxArraySize; x++ )
        {

            /* 计算任务运行时间与总运行时间的百分比。*/
            ulStatsAsPercentage =(uint64_t)(pxTaskStatusArray[ x ].ulRunTimeCounter)*100 / ulTotalRunTime;

            if( ulStatsAsPercentage > 0UL )
            {

               NRF_LOG_INFO("%-8s%c   %-3d%-5d%-5d%d%%",pxTaskStatusArray[ x].pcTaskName,task_state[pxTaskStatusArray[ x ].eCurrentState],
                                                                       pxTaskStatusArray[ x ].xTaskNumber,pxTaskStatusArray[ x].uxCurrentPriority,
                                                                       pxTaskStatusArray[ x ].usStackHighWaterMark,ulStatsAsPercentage);
            }
            else
            {
                /* 任务运行时间不足总运行时间的1%*/
                NRF_LOG_INFO("%-8s%c   %-3d%-5d%-5dt<1%%",pxTaskStatusArray[x ].pcTaskName,task_state[pxTaskStatusArray[ x ].eCurrentState],
                                                                       pxTaskStatusArray[ x ].xTaskNumber,pxTaskStatusArray[ x].uxCurrentPriority,
                                                                       pxTaskStatusArray[ x ].usStackHighWaterMark);
            }
        }
    }
    NRF_LOG_INFO("task status r-run R-ready B-block S-susspend D-delete");

}
#endif
//ADP5360_RESULT Adp5360_DefaultReset(void);
//ADP5360_RESULT Adp5360_getBuckCfg(ADP5360_BUCK_CFG *buck_cfg_t) ;
//ADP5360_RESULT Adp5360_getBuckBstCfg(ADP5360_BUCK_BOOST_CFG *buckbst_cfg_t) ;
static void display_func(void)
{
    dis_dynamic_refresh(100);
    lcd_background_color_set(0xff);
    lygl_dis_string_middle(&GUI_Fontweiruanyahei32,104,104,COLOR_DEFAULT,"print task infoxx");
    static uint16_t bat_vol = 0;
    bat_vol++;
    lygl_dis_value_middle(&GUI_Fontweiruanyahei48,102,150,COLOR_DEFAULT,bat_vol,0);
    if(bat_vol>10)
    {
    bat_vol = 0;
//    print_task_state();
    }
    lcd_display_refresh_all();
#if 0
    uint16_t value1,value2;
//    ADP5360_BUCK_CFG value1;
//    ADP5360_BUCK_BOOST_CFG value2;
    Adp5360_getBuckCfg((ADP5360_BUCK_CFG *)&value1);
    Adp5360_getBuckBstCfg((ADP5360_BUCK_BOOST_CFG*)&value2);
    NRF_LOG_INFO("buckCfg = 0x%4x;buck_boost_cfg = 0x%4x",(uint16_t) value1,(uint16_t) value2);
//    NRF_LOG_INFO("buck_en = %d,buck_vout=0x%x",value1.buck_cfg.buck_output_en,value1.output_vol_set.vout_buck);
//    NRF_LOG_INFO("buck_boost_en = %d,buck_boost_vout=0x%x",value2.buckbst_cfg.buckbst_output_en,value2.output_vol_set.vout_buckbst);
    NRF_LOG_INFO("Adp5360_DefaultReset");
    Adp5360_DefaultReset();
    Adp5360_getBuckCfg((ADP5360_BUCK_CFG *)&value1);
    Adp5360_getBuckBstCfg((ADP5360_BUCK_BOOST_CFG*)&value2);
    NRF_LOG_INFO("buckCfg = 0x%4x;buck_boost_cfg = 0x%4x",(uint16_t) value1,(uint16_t) value2);
//    NRF_LOG_INFO("buck_en = %d,buck_vout=0x%x",value1.buck_cfg.buck_output_en,value1.output_vol_set.vout_buck);
//    NRF_LOG_INFO("buck_boost_en = %d,buck_boost_vout=0x%x",value2.buckbst_cfg.buckbst_output_en,value2.output_vol_set.vout_buckbst);
#endif

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
            dis_page_jump(&page_menu);
        }
        break;
        /*can add other key handle*/
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
        case 0:
        {

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
const PAGE_HANDLE page_task_info = {
.display_func = &display_func,
.key_handle = &key_handle,
.signal_handle = &signal_handle
};
#endif