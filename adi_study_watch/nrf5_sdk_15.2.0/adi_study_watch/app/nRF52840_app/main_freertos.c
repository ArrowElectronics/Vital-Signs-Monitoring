/**
    ***************************************************************************
    * @addtogroup User
    * @{
    * @file         main_freertos.c
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
#include "app_timer.h"
#include "bsp.h"
#include "nordic_common.h"
#include "nrf_drv_clock.h"
#include "sdk_errors.h"
#include "app_error.h"
#include <adi_osal.h>
#include <adpd4000_task.h>
#include <adxl_task.h>
#include <ecg_task.h>
#include <usbd_task.h>
#include <system_task.h>
#include <ppg_application_task.h>
#include <sync_data_application_task.h>
#include <ble_task.h>
#include <temperature_task.h>
#include <logger_task.h>
#include <file_system_task.h>
#include "file_system_utils.h"
#include <app_util.h>
#include "nrf_drv_power.h"
#include "nrf_log.h"
#ifdef ENABLE_WATCH_DISPLAY
#include "display_app.h"
#endif
#include "key_detect.h"
#include <eda_application_task.h>
#include <bia_application_task.h>
#include "low_touch_task.h"
#include <pedometer_task.h>
#include <sqi_task.h>
#include <watch_dog_task.h>
#include "wdt.h"
#include "ad7156.h"
#ifdef USER0_CONFIG_APP
#include <user0_config_app_task.h>
#endif
#ifdef RTOS_TASK_INFO_PRINT
#include "rtos_test.h"
#endif
#ifdef EVTBOARD
#include <adp5360.h>
#include "hal_twi0.h"
#include <power_manager.h>
#else
#include "fds_drv.h"
#include "rtc.h"
#endif //EVTBOARD

#ifdef VSM_MBOARD
#include "watch_vsm_motherboard_pin_config.h"
#elif PCBA
#include "watch_board_pcba_pin_config.h"
#else
#include "watch_board_evt_pin_config.h"
#endif

#include <stdio.h>
#define LED_TASK_DELAY    200           /**< Task delay. Delays a LED0 task for 200 ms */
#define TIMER_PERIOD      1000          /**< Timer period. LED1 timer will expire after 1000 ms */

uint32_t ad5940_port_Init(void);
void AD5940_HWReset_NrfDelay(void);

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

void Debug_Handler()
{
//  while(1);
}

void vApplicationStackOverflowHook( TaskHandle_t xTask,
                                    signed char *pcTaskName )
{
  taskDISABLE_INTERRUPTS();
  while(1);
}

#define FPU_EXCEPTION_MASK 0x0000009F

void FPU_IRQHandler(void)
{
    uint32_t *fpscr = (uint32_t *)(FPU->FPCAR+0x40);
    (void)__get_FPSCR();

    *fpscr = *fpscr & ~(FPU_EXCEPTION_MASK);
}
void ad5940_spi_thread_init();

int main(void)
{
    NVIC_SetPriority(FPU_IRQn, APP_IRQ_PRIORITY_LOW);
    NVIC_EnableIRQ(FPU_IRQn);
#ifdef DEBUG
     //debug();
#endif

#ifdef TRC_USE_TRACEALYZER_RECORDER
#if (TRC_CFG_RECORDER_MODE==TRC_RECORDER_MODE_STREAMING)
         vTraceEnable(TRC_START);
#else
         vTraceEnable(TRC_START);
#endif
#endif
    /* Initialize clock driver for better time accuracy in FREERTOS */
    nrf_drv_power_init(NULL);
    nrf_drv_clock_init();
    for(int i = 0;i<48;i++)
    {
        nrf_gpio_cfg_input(i,NRF_GPIO_PIN_NOPULL);
    }
#ifdef EVTBOARD
   // tick_init();
    twi0_init();
    Adp5360_init();
#ifdef USE_ADP5360_WDT
    watchdog_task_init();
#endif
    pmu_port_init();
#endif //EVTBOARD
    gpio_init();
    spi_init();
    ad5940_spi_thread_init();

#if NRF_LOG_ENABLED
    logger_task_init();
#endif //NRF_LOG_ENABLED

#ifdef DISABLE_BLE
    // Initialize timer module.
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);
#else
    ble_application_task_init();
    update_ble_system_info();
    ble_services_sensor_task_init();
#endif

#ifdef HIBERNATE_MD_EN
    /*After bootup, try Hibernate Mode entry condition*/
    hibernate_md_timers_init();
    hibernate_mode_entry();
#endif

#ifdef ENABLE_WATCH_DISPLAY
#ifdef EVTBOARD
    display_app_init();
#endif
#endif
    key_detect_init();

    memory_manager_init();

    post_office_task_init ();
    sensor_adpd4000_task_init();
    sensor_adxl_task_init();
    usbd_task_init();
    system_task_init();
#ifdef ENABLE_PPG_APP
    ppg_application_task_init();
#endif
#ifdef ENABLE_PEDO_APP
    pedometer_app_task_init();
#endif
#ifdef ENABLE_SQI_APP
    sqi_app_task_init();
#endif
#ifdef EVTBOARD

#ifdef USE_FS
    file_system_task_init();
#endif
#ifdef LOW_TOUCH_FEATURE
    low_touch_task_init();
#endif
    Ad5940Init();
#ifdef ENABLE_ECG_APP
    ad5940_ecg_task_init();
#endif
#ifdef ENABLE_EDA_APP
    ad5940_eda_task_init();
#endif
#ifdef ENABLE_BIA_APP
    ad5940_bia_task_init();
#endif
#ifdef ENABLE_TEMPERATURE_APP
    temperature_app_task_init();
#endif
    twi0_mutex_enable();
#endif

#if defined(PROFILE_TIME_ENABLED) || defined(MEASURE_BLE_ADV_TIME)
   us_timer_init();
#endif

#ifdef RTOS_TASK_INFO_PRINT
    rtos_test_init();
#endif

#ifdef USER0_CONFIG_APP
    user0_config_app_task_init();
#endif

    nrf_gpio_pin_set(PWE_EPHYZ_PIN);
    /*
      Not using task delay call hwreset as sheduler not started.
      And also using nrfdelay only here,not in app as we dont want sheduler blocking delays
      Note: Boot time is subjected to change with these changes
    */ 
    AD5940_HWReset_NrfDelay();
    AD5940_Initialize();
    DG2502_SW_control_AD5940(0);    
    DG2502_SW_control_AD8233(0);
    nrf_gpio_pin_clear(PWE_EPHYZ_PIN);
    NRF_LOG_INFO("App started\n");

    /* Activate deep sleep mode */
//    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
    /* Start FreeRTOS scheduler. */
    vTaskStartScheduler();

    while (true)
    {
        /* FreeRTOS should not be here... FreeRTOS goes back to the start of stack
         * in vTaskStartScheduler function. */
    }
}

/**
 *@}
 **/
