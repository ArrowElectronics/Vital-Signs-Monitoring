/*!
 *  \copyright Analog Devices
 * ****************************************************************************
 *
 * License Agreement
 *
 * Copyright (c) 2019 Analog Devices Inc.
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
#include "boards.h"
#include "hw_if_config.h"
#include "nrf.h"
#include "nrf_drv_gpiote.h"
#include "sdk_config.h"

#ifdef VSM_MBOARD
#include "watch_vsm_motherboard_pin_config.h"
#elif PCBA
#include "watch_board_pcba_pin_config.h"
#else
#include "watch_board_evt_pin_config.h"
#endif//PCBA

#ifdef EVTBOARD
#define ADPD_INT_PIN    ADPD4K_INT_PIN
#define ADXL_INT_PIN    ADXL362_INT1_PIN
#else
#define ADPD_INT_PIN    30
#define ADXL_INT_PIN    31
#define MUX_SEL_PIN     27
#endif //EVTBOARD

#ifdef BSP_BUTTON_0
    #define PIN_IN 30//BSP_BUTTON_0
#endif
#ifndef PIN_IN
    #error "Please indicate input pin"
#endif

#ifdef BSP_LED_0
    #define PIN_OUT BSP_LED_0
#endif
#ifndef PIN_OUT
    #error "Please indicate output pin"
#endif

//void in_pin_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
//{
//    nrf_drv_gpiote_out_toggle(PIN_OUT);
////    Adpd400xISR();
//    AdxlISR();
//}
static uint32_t gsAdpdIntCnt =0,gsAdxlIntCnt =0;
volatile uint32_t gExtTriggerIntCnt=0;

void ext_trigger_int_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
  gExtTriggerIntCnt++;
}
void adpd_int_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    gsAdpdIntCnt++;
    Adpd400xISR();
}

void adxl_int_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    gsAdxlIntCnt++;
    AdxlISR();
}

/**
 * @brief Function for configuring: PIN_IN pin for input, PIN_OUT pin for output,
 * and configures GPIOTE to give an interrupt on pin change.
 */
void gpio_init(void)
{
//    ret_code_t err_code;
#ifndef EVTBOARD
//    err_code =
      nrf_drv_gpiote_init();
//    APP_ERROR_CHECK(err_code);
#endif //EVTBOARD

    /*set the mux sel line high by default*/
      nrf_drv_gpiote_out_config_t out_config = GPIOTE_CONFIG_OUT_SIMPLE(true);
#ifdef EVTBOARD
      nrf_drv_gpiote_out_init(ADPD4K_SPI_CS_PIN, &out_config);
      nrf_drv_gpiote_out_init(ADXL362_SPI_CS_PIN, &out_config);
#else
      nrf_drv_gpiote_out_init(MUX_SEL_PIN, &out_config);
      SetMuxSelectLine(true);
#endif //EVTBOARD

//    APP_ERROR_CHECK(err_code);
//    nrf_drv_gpiote_in_config_t ext_in_config = NRFX_GPIOTE_CONFIG_IN_SENSE_HITOLO(true);
//    ext_in_config.pull = NRF_GPIO_PIN_PULLUP;
//    nrf_drv_gpiote_in_init(ADPD4K_GPIO3_PIN, &ext_in_config, ext_trigger_int_handler);

    nrf_drv_gpiote_in_config_t in_config = NRFX_GPIOTE_CONFIG_IN_SENSE_HITOLO(true);
    in_config.pull = NRF_GPIO_PIN_PULLUP;
    nrf_drv_gpiote_in_init(ADXL_INT_PIN, &in_config, adxl_int_handler);
    nrf_drv_gpiote_in_init(ADPD_INT_PIN, &in_config, adpd_int_handler);
//    nrf_drv_gpiote_in_init(ADXL362_INT2_PIN, &in_config, ext_trigger_int_handler);
//    APP_ERROR_CHECK(err_code);

    //nrf_drv_gpiote_in_event_enable(ADXL_INT_PIN, true);
    //nrf_drv_gpiote_in_event_enable(ADPD_INT_PIN, true);

}

void enable_adxl_trigger_pin()
{
  nrf_drv_gpiote_out_config_t out_config = GPIOTE_CONFIG_OUT_SIMPLE(false);
  nrf_drv_gpiote_out_init(ADXL362_INT2_PIN, &out_config);
}

__STATIC_INLINE void nrf_gpio_cfg_output_high_drive(uint32_t pin_number)
{
    nrf_gpio_cfg(
        pin_number,
        NRF_GPIO_PIN_DIR_OUTPUT,
        NRF_GPIO_PIN_INPUT_DISCONNECT,
        NRF_GPIO_PIN_NOPULL,
        NRF_GPIO_PIN_H0H1,
        NRF_GPIO_PIN_NOSENSE);
}

void enable_ad5940_trigger_pin()
{
  //nrf_gpio_cfg_output(3);
 // nrf_drv_gpiote_out_config_t out_config = GPIOTE_CONFIG_OUT_SIMPLE(true);
 // nrf_drv_gpiote_out_init(3, &out_config);
 nrf_gpio_cfg_output_high_drive(3);
}

void disable_ad5940_trigger_pin()
{
   //nrfx_gpiote_out_uninit(3);
}

void disable_adxl_trigger_pin()
{
  nrfx_gpiote_out_uninit(ADXL362_INT2_PIN);
}

void invert_adxl_trigger_signal()
{
  nrf_drv_gpiote_out_toggle(ADXL362_INT2_PIN);
}

void invert_ad5940_trigger_signal()
{
  nrf_gpio_pin_toggle(3);
  //nrf_drv_gpiote_out_toggle(3);
}

void reset_ad5940_trigger_signal(bool n_state)
{
  if(n_state){
    //nrf_drv_gpiote_out_set(3);
    nrf_gpio_pin_set(3);
    }
  else{
    //nrf_drv_gpiote_out_clear(3);
    nrf_gpio_pin_clear(3);
    }
}

void reset_trigger_pulses()
{
  nrf_drv_gpiote_out_set(ADXL362_INT2_PIN);
  nrf_drv_gpiote_out_set(ADPD4K_GPIO3_PIN);
}

void enable_adpd_trigger_pin()
{
  nrf_drv_gpiote_out_config_t out_config = GPIOTE_CONFIG_OUT_SIMPLE(false);
  nrf_drv_gpiote_out_init(ADPD4K_GPIO3_PIN, &out_config);
}

void disable_adpd_trigger_pin()
{
  nrfx_gpiote_out_uninit(ADPD4K_GPIO3_PIN);
}

void invert_adpd_trigger_signal()
{
  nrf_drv_gpiote_out_toggle(ADPD4K_GPIO3_PIN);
}

#ifdef EVTBOARD
ADI_HAL_STATUS_t set_adpd4k_cs_line(bool_t bValue)
{
  if(bValue)
    nrf_drv_gpiote_out_set(ADPD4K_SPI_CS_PIN);
  else
    nrf_drv_gpiote_out_clear(ADPD4K_SPI_CS_PIN);
  return (ADI_HAL_OK);
}

ADI_HAL_STATUS_t set_adxl_cs_line(bool_t bValue)
{
  if(bValue)
    nrf_drv_gpiote_out_set(ADXL362_SPI_CS_PIN);
  else
    nrf_drv_gpiote_out_clear(ADXL362_SPI_CS_PIN);
  return (ADI_HAL_OK);
}

#else
ADI_HAL_STATUS_t SetMuxSelectLine(bool_t bValue)
{
  if(bValue)
    nrf_drv_gpiote_out_set(MUX_SEL_PIN);
  else
    nrf_drv_gpiote_out_clear(MUX_SEL_PIN);
  return (ADI_HAL_OK);
}
#endif //EVTBOARD
ADI_HAL_STATUS_t GPIO_IRQ_ADXL362_Disable()
{
  //TODO: Caution!!! This disables all other gpio interrupts
  nrfx_gpiote_in_event_disable(ADXL_INT_PIN);
  return (ADI_HAL_OK);
}

ADI_HAL_STATUS_t GPIO_IRQ_ADXL362_Enable()
{
  nrf_drv_gpiote_in_event_enable(ADXL_INT_PIN, true);
  return (ADI_HAL_OK);
}


ADI_HAL_STATUS_t GPIO_IRQ_ADPD_Disable()
{
  //TODO: Caution!!! This disables all other gpio interrupts
  nrfx_gpiote_in_event_disable(ADPD_INT_PIN);
  return (ADI_HAL_OK);
}

ADI_HAL_STATUS_t GPIO_IRQ_ADPD_Enable()
{
  nrf_drv_gpiote_in_event_enable(ADPD_INT_PIN, true);
  return (ADI_HAL_OK);
}

/* Function to get timestamps in ms resolution */
TickType_t MCU_HAL_GetTick()
{
 //return(xTaskGetTickCount());
 return(get_ms_time_stamp());
}

uint32_t AdpdLibGetTick() { // in ms

    return (uint32_t)xTaskGetTickCount();
}
void GPIO_Clock_Cal_TriggerTS()
{

}

/* 
  *  @brief   Function to get the current timestamp in 32kHz ticks resolution, by the sensors for packet TS
  *           This function will be called from library for AGC timestamp udpate.
  *   @param  None
  *   @retval unit32_t ticks in 32K resolution 
*/
uint32_t AdpdLibGetSensorTimeStamp() {

    return get_sensor_time_stamp();
}
