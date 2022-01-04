/**
****************************************************************************
* @file     power_manager.c
* @author   ADI
* @version  V0.1
* @date     10-Mar-2020
* @brief    This Source file used to implement power manager functions.
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

#include "power_manager.h"
#ifdef VSM_MBOARD
#include "watch_vsm_motherboard_pin_config.h"
#elif PCBA
#include "watch_board_pcba_pin_config.h"
#else
#include "watch_board_evt_pin_config.h"
#endif
#include "nrf_drv_gpiote.h"
#include "hal_twi0.h"
#include "nand_cmd.h"

#include <adi_osal.h>
#include <app_cfg.h>
#include <task_includes.h>

#include "adp5360.h"
#include "ble_task.h"
#include "nrf_pwr_mgmt.h"
#include "power_on_detect.h"
#ifdef HIBERNATE_MD_EN
#include "app_timer.h"
#include "timers.h"
#endif

#include "rtc.h"
#include "nrf_rtc.h"
#include "lcd_driver.h"
#include "wdt.h"
#include "nrf_power.h"
#include "app_usbd.h"
#include "ad7156.h"
#include "adpd400x_drv.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "ad5940.h"

#ifdef CUST4_SM
volatile bool gnGpioPowerOn __attribute__((section(".non_init")));
#endif

extern uint8_t dvt2; //whether dvt2 or not
bool gb_ecgldo_enable = false;
bool g_disable_ad5940_port_init =false;
bool g_disable_ad5940_port_deinit = false;

#ifdef HIBERNATE_MD_EN
/* Variable which controls the enable/disable of Hibernate Mode from m2m2 command */
bool gb_hib_mode_control = true;
/** Flag to store the event status, deciding when to do Hibernate Mode Entry-Exit
 ** Initial value is based on the fact that on Boot up,
 ** Display is Inactive in the sense, no streaming is happening to the Display,
 ** (Display is Active from HR sub-page or Waveform sub-pages)
 ** there will be No key press, FS log is stopped, BLE/USB is Disconnected. Later EVTs will be update this flag accordingly *
 ** Bit 0, in position 'p' indicates condition for NO Hib. mode entry for EVT associated with 'p',
 ** Bit 1, in position 'p' indicates condition for Hib. mode entry for EVT associated with 'p'
 ** Eg: bit 0 is position 0 indicates No Hib. Mode entry as decided by HIB_MD_USB_DISCONNECT_EVT */
volatile HIB_MD_EVT_T hib_mode_status = (HIB_MD_DISP_INACTIVE_EVT | HIB_MD_NO_KEY_PRESS_EVT | HIB_MD_FS_LOG_STOP_EVT | HIB_MD_BLE_DISCONNECT_EVT | HIB_MD_USB_DISCONNECT_EVT);

void hibernate_md_set(HIB_MD_EVT_T evt)
{
    hib_mode_status = hib_mode_status | evt;
    NRF_LOG_INFO("Hibernate Mode set from EVT %d",evt);
}

void hibernate_md_clear(HIB_MD_EVT_T evt)
{
    hib_mode_status = hib_mode_status & ~evt;
    NRF_LOG_INFO("Hibernate Mode cleared from EVT %d",evt);
}

APP_TIMER_DEF(m_hib_timer_id);                 /**< Handler for repeated timer for hib mode. */
#define HIB_MODE_ENTRY_INTERVAL         300000 /**< Hibernate Mode entry interval 5mins = 300 (s). */

/**@brief Function for handling the Hibernate mode entry timer time-out.
 *
 * @details This function will be called each time the hibernate mode entry timer expires.
 *
 */
static void hibernate_md_entry_timeout_handler(void * p_context)
{
    NRF_LOG_INFO("Hibernate Mode entry Timer expiry after:%d s, entering poweroff", HIB_MODE_ENTRY_INTERVAL);

    enter_poweroff_mode();
}

/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module. This creates and starts application timers.
 */
void hibernate_md_timers_init(void)
{
    ret_code_t err_code;

    // Create timers
    err_code =  app_timer_create(&m_hib_timer_id, APP_TIMER_MODE_REPEATED, hibernate_md_entry_timeout_handler);

    APP_ERROR_CHECK(err_code);
}

/**@brief   Function for starting application timers.
 * @details Timers are run after the scheduler has started.
 */
static void hibernate_md_application_timers_start(void)
{
    // Start repeated timer
    ret_code_t err_code = app_timer_start(m_hib_timer_id, APP_TIMER_TICKS(HIB_MODE_ENTRY_INTERVAL), NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_INFO("Hibernate Mode Timers start");
}

static void hibernate_md_application_timers_stop(void)
{
    // Stop the repeated timer
    ret_code_t err_code = app_timer_stop(m_hib_timer_id);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_INFO("Hibernate Mode Timers stop");
}

void hibernate_mode_entry()
{
    uint8_t index;
    bool hib_cond = true;

    /* Check Hib Mode enab/disable control condition & start hibernate timers,
       only if Hibernate mode is enabled */
    if(gb_hib_mode_control)
    {
      //Check among all five EVT sources, whether any EVT prevents Hib. Mode entry
      for(index=0;index<5;index++)
      {
        if( ( ((hib_mode_status & 0x001F) >> index) & 0x0001 ) == 0 ) //0 indicates No Hibernate
        {
          hib_cond = false;
          break;
        }
      }
      if(hib_cond)
        hibernate_md_application_timers_start();
      else
        hibernate_md_application_timers_stop();
    }
}
#endif //HIBERNATE_MD_EN

void pmu_port_init(void)
{
    nrf_gpio_cfg_output(PWR_FLASH_PIN);
    nrf_gpio_cfg_output(PWR_OPTICAL_PIN);
    nrf_gpio_cfg_output(PWE_EPHYZ_PIN);
    nrf_gpio_pin_set(PWR_OPTICAL_PIN);
}

_Bool adp5360_is_ldo_enable(int PIN)
{
  switch(PIN)
  {
    case FS_LDO:
#ifdef PRINTS_OUT
            NRF_LOG_INFO("Flash power LDO");
#endif
            return(nrfx_gpiote_in_is_set(PWR_FLASH_PIN));
    break;
    case OPT_DEV_LDO:
#ifdef PRINTS_OUT
            NRF_LOG_INFO("Optical device LDO and Led's LDO");
#endif
            return(nrfx_gpiote_in_is_set(PWR_OPTICAL_PIN));
    break;
    case ECG_LDO:
#ifdef PRINTS_OUT
            NRF_LOG_INFO("Power for 8233 and bio impedance LDO");
#endif
            return(nrfx_gpiote_in_is_set(PWE_EPHYZ_PIN));
    break;
    default:
#ifdef PRINTS_OUT
            NRF_LOG_INFO("Enter valid LDO choice");
#endif
            break;
  }
  return false;
}

void adp5360_enable_ldo(int PIN,bool en)
{
  switch(PIN)
  {
    case FS_LDO:
            if(en)
            {
#ifdef PRINTS_OUT
              NRF_LOG_INFO("Enabling Flash power LDO");
#endif
              nrf_gpio_pin_set(PWR_FLASH_PIN);
#ifndef PCBA
               nrf_gpio_pin_set(PWR_OPTICAL_PIN);//EVT board has flash power pin and optical pin wired up
#endif
            }
            else
            {
#ifdef PRINTS_OUT
              NRF_LOG_INFO("Disabling Flash power LDO");
#endif
              nrf_gpio_pin_clear(PWR_FLASH_PIN);
#ifndef PCBA
              nrf_gpio_pin_clear(PWR_OPTICAL_PIN);
#endif
            }
    break;
    case OPT_DEV_LDO:
            if(en)
            {
#ifdef PRINTS_OUT
              NRF_LOG_INFO("Enabling Optical device LDO and Led's LDO");
#endif
              nrf_gpio_pin_set(PWR_OPTICAL_PIN);
            }
            else
            {
#ifdef PRINTS_OUT
              NRF_LOG_INFO("Disabling Optical device LDO and Led's LDO");
#endif
              nrf_gpio_pin_clear(PWR_OPTICAL_PIN);
            }
    break;
    case ECG_LDO:
          if(en)
            {
#ifdef PRINTS_OUT
              NRF_LOG_INFO("Enabling Power for 8233 and bio impedance LDO");

#endif
              gb_ecgldo_enable = true;
              nrf_gpio_pin_set(PWE_EPHYZ_PIN);
              if(g_disable_ad5940_port_init == false) {
              /* After enabling LDO,Initializing AD5940 */ 
              ad5940_port_Init();
              }
              /* Use hardware reset */
              AD5940_HWReset();
              /* Platform configuration */
              AD5940_Initialize();
            }
            else
            {
#ifdef PRINTS_OUT
              NRF_LOG_INFO("Disabling Power for 8233 and bio impedance LDO");
#endif
              if(g_disable_ad5940_port_deinit == false) {
              /* DeInitializing AD5940 Port */ 
              ad5940_port_deInit();
              }
              nrf_gpio_pin_clear(PWE_EPHYZ_PIN);
              gb_ecgldo_enable = false;
            }
    break;
    default:
#ifdef PRINTS_OUT
            NRF_LOG_INFO("Enter valid LDO choice");
#endif
            break;
  }
}

void enter_sleep_status(void)
{

//    NRF_LOG_INFO("enter_sleep_status");
//    twi0_uninit();//
}
void exit_sleep_status(void)
{
//    NRF_LOG_INFO("exit_sleep_status");
//    twi0_init();
}

static void power_on(void)
{
#ifdef CUST4_SM
    gnGpioPowerOn = true;
#endif
    rtc_timestamp_store(320);
    NVIC_SystemReset();
}

void enter_poweroff_mode(void)
{
    AD7156_SetPowerMode(AD7156_CONV_MODE_PWR_DWN);
#ifdef USE_ADP5360_WDT
    Adp5350_wdt_set(ADP5360_DISABLE);/* close the WDT of AD5360*/
#endif
    if( !dvt2 )
      adi_adpddrv_CloseDriver();//will delete this after DVT2.
#ifdef ENABLE_WATCH_DISPLAY
    lcd_disp_off();
#endif
    /* Stop tick events */
//    nrf_rtc_int_disable(portNRF_RTC_REG, NRF_RTC_INT_TICK_MASK);//rtc need keep work in the power off mode.
    vTaskSuspendAll();//suspend all task.



    nrf_gpio_pin_clear(PWE_EPHYZ_PIN);
    if(!dvt2)
      nrf_gpio_pin_set(PWR_OPTICAL_PIN);//DVT2 will change the hardware, Then can to delete this line.
    nrf_gpio_pin_clear(PWR_FLASH_PIN);

    ble_disconnect_and_unbond();
    ble_disable_softdevice();

    app_usbd_uninit();

    twi0_uninit();
#ifdef USE_FS
    nand_flash_qspi_uninit();//Need as uninitialize at here.
#endif

    if (nrf_drv_gpiote_is_init())
    {
        nrf_drv_gpiote_uninit();
    }

    for(int i = 0;i<48;i++)
    {
#ifndef VSM_MBOARD
        if( !dvt2 )
        {
          if((i == PWR_OPTICAL_PIN)||(i == LCD_DISP_SWITCH))//DVT2 will change the hardware, Then can to delete this line.
          {
              continue;
          }
        }
#endif
        nrf_gpio_cfg_default(i);
    }

    /* add power on detect initialize to detect power on key*/
    register_power_on_func(power_on);
    power_on_detect_init();

    __DSB();
    do{
          __WFE();
      } while (1);
}

#ifdef HIBERNATE_MD_EN
/*!
 ****************************************************************************
 * @brief Function to control the hibernate mode enable/disable used in Fw
 *
 * @details This function is to be used by iOS app or any other tool which wants to
 * keep the Watch turned ON
 *
 * @param[in] hib_mode_cntrl   1 -> to enable 0-> to disable Hibernate mode
 * @param[out] 0-> success | 1-> failure
 ******************************************************************************/
uint8_t set_hib_mode_control(uint8_t hib_mode_cntrl) {
    if(hib_mode_cntrl == 1)
    {
      gb_hib_mode_control = true;
      hibernate_mode_entry();
    }
    else if(hib_mode_cntrl == 0)
    {
      hibernate_md_application_timers_stop();
      gb_hib_mode_control = false;
    }
    else
    {
      return 1;
    }

    return 0;
}

/*!
 ****************************************************************************
 * @brief Function to get the hibernate mode enable/disable control value used in Fw
 *
 * @details This function is to be used by iOS app or any other tool which wants to
 * keep the Watch turned ON
 *
 * @param[in] None
 * @param[out] value being set in Fw currently
 ******************************************************************************/
uint8_t get_hib_mode_control() {
    return gb_hib_mode_control;
}
#endif