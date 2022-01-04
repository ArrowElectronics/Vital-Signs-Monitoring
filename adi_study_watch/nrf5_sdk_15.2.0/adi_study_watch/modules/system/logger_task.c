/**
 * Copyright (c) 2017 - 2018, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include "nordic_common.h"
#include "nrf.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

/* FreeRTOS related */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include <logger_task.h>
#include <post_office.h>
#include <hw_if_config.h>
#include <adi_osal.h>
#include <task_includes.h>
#include <app_cfg.h>
#include "app_util.h"

#if NRF_LOG_ENABLED
uint8_t logger_task_stack[APP_OS_CFG_LOGGER_TASK_STK_SIZE];
ADI_OSAL_THREAD_HANDLE logger_task_handler;
ADI_OSAL_STATIC_THREAD_ATTR logger_task_attributes;
StaticTask_t LoggerTaskTcb;

/**@brief A function which is hooked to idle task.
 * @note Idle hook must be enabled in FreeRTOS configuration (configUSE_IDLE_HOOK).
 */
void logger_task_resume( void )
{
     vTaskResume(logger_task_handler);
}

/** @brief Logger task function. */
static void logger_task(void * arg)
{
  while (1) {
    NRF_LOG_FLUSH();
    vTaskSuspend(NULL); // Suspend myself
  }

}

/** @brief Logger task init function. */
void logger_task_init(void)
{  
    ADI_OSAL_STATUS eOsStatus;
  
    /* Create logger thread */
    logger_task_attributes.pThreadFunc = logger_task;
    logger_task_attributes.nPriority = APP_OS_CFG_LOGGER_TASK_PRIO;
    logger_task_attributes.pStackBase = &logger_task_stack[0];
    logger_task_attributes.nStackSize = APP_OS_CFG_LOGGER_TASK_STK_SIZE;
    logger_task_attributes.pTaskAttrParam = NULL;
    /* Thread Name should be of max 10 Characters */
    logger_task_attributes.szThreadName = "LoggerTask";
    logger_task_attributes.pThreadTcb = &LoggerTaskTcb;
  
    eOsStatus = adi_osal_ThreadCreateStatic(&logger_task_handler,
                                      &logger_task_attributes);
    if (eOsStatus != ADI_OSAL_SUCCESS) {
        Debug_Handler();
    }
    
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

/**
 * @}
 */
#endif //NRF_LOG_ENABLED