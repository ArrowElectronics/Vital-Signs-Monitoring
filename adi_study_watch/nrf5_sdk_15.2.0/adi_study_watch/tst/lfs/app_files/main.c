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
/** @file
 *
 * @defgroup usbd_ble_uart_freertos_example main.c
 * @{
 * @brief    USBD CDC ACM over BLE using FreeRTOS application main file.
 *
 * This file contains the source code for a sample application that uses the Nordic UART service
 * and USBD CDC ACM library and runs using FreeRTOS.
 * This application uses the @ref srvlib_conn_params module.
 */

#include "bsp_btn_ble.h"



#include "nrf_drv_clock.h"
#include "nrf_drv_power.h"
/* FreeRTOS related */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
/*user added*/
//#include "ble_app.h"
#include "usbd_app.h"
//#include "key_detect.h"
//#include "display_app.h"
#include "adp5360.h"
#include "power_manager.h"
//#include "AD7156/AD7156.h"

#if NRF_LOG_ENABLED

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

static TaskHandle_t m_logger_thread;      /**< Logger thread. */
/**
 * The size of the stack for the Logger task (in 32-bit words).
 * Logger uses sprintf internally so it is a rather stack hungry process.
 */
#define LOGGER_STACK_SIZE 512
/**
 * The priority of the Logger task.
 */
#define LOGGER_PRIORITY 1

#endif

//#define DEAD_BEEF                       0xDEADBEEF                                  /**< Value used as error code on stack dump. Can be used to identify stack location on stack unwind. */

// BLE DEFINES END

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
//void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
//{
//    app_error_handler(DEAD_BEEF, line_num, p_file_name);
//}

#if 0
/**
 * @brief Function for putting the chip into sleep mode.
 *
 * @note This function does not return.
 */
static void sleep_mode_enter(void)
{
    uint32_t err_code = bsp_indication_set(BSP_INDICATE_IDLE);
    APP_ERROR_CHECK(err_code);

    // Prepare wakeup buttons.
//    err_code = bsp_btn_ble_sleep_mode_prepare();
//    APP_ERROR_CHECK(err_code);

    // Go to system-off mode (this function will not return; wakeup will cause a reset).
    err_code = sd_power_system_off();
    APP_ERROR_CHECK(err_code);
}

/**
 * @brief Function for handling events from the BSP module.
 *
 * @param[in]   event   Event generated by button press.
 */
void bsp_event_handler(bsp_event_t event)
{
//    uint32_t err_code;
    switch (event)
    {
        case BSP_EVENT_SLEEP:
//            sleep_mode_enter();
            break;

        case BSP_EVENT_DISCONNECT:
//            err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
//            if (err_code != NRF_ERROR_INVALID_STATE)
            {
 //               APP_ERROR_CHECK(err_code);
            }
            break;

        case BSP_EVENT_WHITELIST_OFF:
//            if (m_conn_handle == BLE_CONN_HANDLE_INVALID)
            {
//                err_code = ble_advertising_restart_without_whitelist(&m_advertising);
//                if (err_code != NRF_ERROR_INVALID_STATE)
                {
//                    APP_ERROR_CHECK(err_code);
                }
            }
            break;

        default:
            break;
    }
}
#endif

#if NRF_LOG_ENABLED
/** @brief Function for initializing the nrf_log module. */
static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}
/**@brief Thread for handling the logger.
 *
 * @details This thread is responsible for processing log entries if logs are deferred.
 *          Thread flushes all log entries and suspends. It is resumed by idle task hook.
 *
 * @param[in]   arg   Pointer used for passing some arbitrary information (context) from the
 *                    osThreadCreate() call to the thread.
 */
static void logger_thread(void * arg)
{
    UNUSED_PARAMETER(arg);
    
    while (1) 
    {
#if defined(NRF_LOG_BACKEND_USB_ENABLED) && NRF_LOG_BACKEND_USB_ENABLED
        if(app_usb_cdc_is_open())
#endif
        {
            NRF_LOG_FLUSH();
        }
        vTaskSuspend(NULL); // Suspend myself
    }
}
#endif //NRF_LOG_ENABLED

/**@brief A function which is hooked to idle task.
 * @note Idle hook must be enabled in FreeRTOS configuration (configUSE_IDLE_HOOK).
 */
void vApplicationIdleHook( void )
{
#if NRF_LOG_ENABLED
     vTaskResume(m_logger_thread);
#endif
}

void vApplicationStackOverflowHook( TaskHandle_t xTask,
                                    signed char *pcTaskName )
{
  taskDISABLE_INTERRUPTS();
  while(1);  
}

/**@brief Function for initialization oscillators.
   @note use the extern oscillator,BLE softdevice will start up LFCKL,
   so if not set the extern oscillator in here,system will use the internal oscillator in default
 */
void clock_initialization()//
{
    /* Start 16 MHz crystal oscillator */
    NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
    NRF_CLOCK->TASKS_HFCLKSTART    = 1;

    /* Wait for the external oscillator to start up */
    while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0)
    {
        // Do nothing.
    }

    /* Start low frequency crystal oscillator for app_timer(used by bsp)*/
/*    NRF_CLOCK->LFCLKSRC            = (CLOCK_LFCLKSRC_SRC_Xtal << CLOCK_LFCLKSRC_SRC_Pos);
    NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
    NRF_CLOCK->TASKS_LFCLKSTART    = 1;

    while (NRF_CLOCK->EVENTS_LFCLKSTARTED == 0)
    {
         Do nothing.
    }*/
}
/** @brief Application main function. */
//void lcd_init(void);
//void nand_flash_init(void);
void pmu_port_init(void);
//void Adxl362_test_init(void);
//void Adpd4000_test_init(void);
//void Ad5940_test_init(void);
//void Ad5940_ecg_test_init(void);
//void rtc_init(void);
void tick_init(void);
void Nand_Flash_Test_Init(void);
int main(void)
{
    ret_code_t ret;
    // Initialize.
    clock_initialization();
    
#if NRF_LOG_ENABLED
    log_init();
    // Start execution.
    if (pdPASS != xTaskCreate(logger_thread,
                              "LOGGER",
                              LOGGER_STACK_SIZE,
                              NULL,
                              LOGGER_PRIORITY,
                              &m_logger_thread))
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }
#endif
    ret = nrf_drv_power_init(NULL);
    APP_ERROR_CHECK(ret);
    ret = nrf_drv_clock_init();
    APP_ERROR_CHECK(ret);
 //   rtc_init();
   // tick_init();
    //Key_detect_init();
    Adp5360_init();
    pmu_port_init();  
    Usbd_app_init();

    Nand_Flash_Test_Init();
    // Activate deep sleep mode.
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
    // Start FreeRTOS scheduler.
    vTaskStartScheduler();

    for (;;)
    {
       APP_ERROR_HANDLER(NRF_ERROR_FORBIDDEN);
    }
}

/**
 * @}
 */