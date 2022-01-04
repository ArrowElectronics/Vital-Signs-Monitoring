/**
***************************************************************************
* @addtogroup Tasks
* @{
* @file         watchdog_task.c
* @author       ADI
* @version      V1.0.0
* @date         19-May-2020
* @brief        Source file contains watch dog timer check for
                wearable device framework.
***************************************************************************
* @attention
***************************************************************************
*/
/*!
*  \copyright Analog Devices
* ****************************************************************************
*
* License Agreement
*
* Copyright (c) 2020 Analog Devices Inc.
* All rights reserved.
*
* This source code is intended for the recipient only under the guidelines of
* the non-disclosure agreement with Analog Devices Inc.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER  LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING  FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER  DEALINGS IN THE SOFTWARE.
* ****************************************************************************
*/
/* Includes -----------------------------------------------------------------*/
#ifdef USE_ADP5360_WDT
#include <task_includes.h>
#include <includes.h>
#include <watch_dog_task.h>
#include "adp5360.h"

#define ADP5360_WD_TIME  (25600)             //ADP5360 WD_TIME configured in millisec
#define WD_TASK_SCH_TIME (ADP5360_WD_TIME/2) //Keep schedule time as WD_TIME/2

uint8_t watchdog_system_task_stack[APP_OS_CFG_WDT_TASK_STK_SIZE];
ADI_OSAL_THREAD_HANDLE watchdog_system_task_handler;
ADI_OSAL_STATIC_THREAD_ATTR watchdog_system_task_attributes;
// Create TCB for task
static StaticTask_t watchdogTaskTcb;
// Create Queue Handler for task
static ADI_OSAL_QUEUE_HANDLE  watchdog_task_msg_queue = NULL;

static void watchdog_task(void *pArgument);

volatile bool gb_stop_reset = false;

void trigger_nRF_MCU_hw_reset()
{
  gb_stop_reset = true;
}

/**
* @brief  watchdog_task_init - Initializes wathcdog task for adp5360 WDT
* @param  pArgument not used
* @return None
*/
void watchdog_task_init(void) {
  ADI_OSAL_STATUS eOsStatus = ADI_OSAL_SUCCESS;
  /* Create watchdog application thread */
  watchdog_system_task_attributes.pThreadFunc = watchdog_task;
  watchdog_system_task_attributes.nPriority = APP_OS_CFG_WDT_TASK_PRIO;
  watchdog_system_task_attributes.pStackBase = &watchdog_system_task_stack[0];
  watchdog_system_task_attributes.nStackSize = APP_OS_CFG_WDT_TASK_STK_SIZE;
  watchdog_system_task_attributes.pTaskAttrParam = NULL;
  /* Thread Name should be of max 10 Characters */
  watchdog_system_task_attributes.szThreadName = "WD_Task";
  watchdog_system_task_attributes.pThreadTcb = &watchdogTaskTcb;
  eOsStatus = adi_osal_MsgQueueCreate(&watchdog_task_msg_queue,NULL,
                                    5);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
      Debug_Handler();
  }
  else
  {
    update_task_queue_list(APP_OS_CFG_WDT_TASK_INDEX,watchdog_task_msg_queue);
  }

  eOsStatus = adi_osal_ThreadCreateStatic(&watchdog_system_task_handler,
                                    &watchdog_system_task_attributes);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
      Debug_Handler();
  }
}

/**
* @brief  watchdog_task - task to reset ADP5360 WD timer
* @param  pArgument not used
* @return None
*/
static void watchdog_task(void *pArgument) {
  while (1) {
    if( gb_stop_reset==false )
    {
        Adp5350_resetWdTimer(); //WD timer in ADP5360 is configured with WDT_TIME=25.6
    }
    adi_osal_ThreadSleep(WD_TASK_SCH_TIME); //Have schedule time of WDT_TIME/2
  }
}
#else
#include "adp5360.h"
void trigger_nRF_MCU_hw_reset()
{
  Adp5350_wdt_set(ADP5360_ENABLE); //Enable ADP5360 WDT
}
#endif//USE_ADP5360_WDT