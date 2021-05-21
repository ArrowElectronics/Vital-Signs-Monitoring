/**
    ***************************************************************************
    * @addtogroup User
    * @{
    * @file         Main.c
    * @author       ADI
    * @version      V1.0.0
    * @date         17-April-2019
    * @brief        Watch application
    *
    ***************************************************************************
    * @attention
    ***************************************************************************
*/
/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2019 Analog Devices Inc.                                      *
* All rights reserved.                                                        *
*                                                                             *
* This source code is intended for the recipient only under the guidelines of *
* the non-disclosure agreement with Analog Devices Inc.                       *
*                                                                             *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,    *
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE *
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      *
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING     *
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER         *
* DEALINGS IN THE SOFTWARE.                                                   *
*                                                                             *
*                                                                             *
* This software is intended for use with the ADPD and derivative parts     *
* only                                                                        *
*                                                                             *
******************************************************************************/

/* Includes -----------------------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "bsp.h"
#include "nordic_common.h"
#include "nrf_drv_clock.h"
#include "sdk_errors.h"
#include "app_error.h"
#include <adi_osal.h>
#include <logger_task.h>
#include <app_util.h>
#include "nrf_drv_power.h"
#include "nrf_log.h"
#include <EDA_App_Test.h>
#ifdef EVTBOARD
#include <adp5360.h>
#include <power_manager.h>
#endif 

uint32_t ad5940_port_Init(void);

uint8_t pTimerTcbBuf[sizeof(StaticTask_t)];
StackType_t pTimerTaskStkBuf[configMINIMAL_STACK_SIZE+100];

void vApplicationGetTimerTaskMemory (StaticTask_t **pTcb,StackType_t **pStk, uint32_t *pStkSize )
{
  *pTcb = (StaticTask_t*) pTimerTcbBuf;
  *pStk = (StackType_t*) pTimerTaskStkBuf;
  *pStkSize = configMINIMAL_STACK_SIZE;
}

static StaticTask_t    IdleTaskTCBbuf;
static StackType_t     IdleTaskStkbuf[configMINIMAL_STACK_SIZE+100];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &IdleTaskTCBbuf;
  *ppxIdleTaskStackBuffer = IdleTaskStkbuf;
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;

}

uint8_t boot_task_stack[500];
ADI_OSAL_THREAD_HANDLE boot_task_handler;
ADI_OSAL_STATIC_THREAD_ATTR boot_task_attributes;
StaticTask_t BootTaskTcb;

#define DEAD_BEEF                       0xDEADBEEF   
void Debug_Handler()
{
}

/**
 * @brief Function for assert macro callback.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of an assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num    Line number of the failing ASSERT call.
 * @param[in] p_file_name File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

void vApplicationStackOverflowHook( TaskHandle_t xTask,
                                    signed char *pcTaskName )
{
  taskDISABLE_INTERRUPTS();
  while(1);  
}
int main(void)
{    
    /* Initialize clock driver for better time accuracy in FREERTOS */
    nrf_drv_power_init(NULL);
    nrf_drv_clock_init();

   // tick_init();
    Adp5360_init();
    pmu_port_init();
    ad5940_port_Init();

//#if NRF_LOG_ENABLED
    //logger_task_init();
//#endif //NRF_LOG_ENABLED
        
    // eda
    AD5940_Main_EDA();
  
    NRF_LOG_INFO("App started\n");
    /* Start FreeRTOS scheduler. */
    vTaskStartScheduler();

    while (true)
    {
        /* FreeRTOS should not be here... FreeRTOS goes back to the start of stack
         * in vTaskStartScheduler function. */
    }
}

void vAssertCalled( const char *pcFileName, unsigned long ulLine )
{
  taskDISABLE_INTERRUPTS();
  while(1);
}


/**
 *@}
 **/
