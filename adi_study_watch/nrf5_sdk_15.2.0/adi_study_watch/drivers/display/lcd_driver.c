/**
****************************************************************************
* @file     lcd_driver.c
* @author   ADI
* @version  V0.1
* @date     20-May-2020
* @brief    This source file is used to implement lcd driver functions
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

#ifdef ENABLE_WATCH_DISPLAY
#include "lcd_driver.h"
#ifdef VSM_MBOARD
#include "watch_vsm_motherboard_pin_config.h"
#elif PCBA
#include "watch_board_pcba_pin_config.h"
#else
#include "watch_board_evt_pin_config.h"
#endif
#include "nrf_drv_spi.h"
#include "nrf_drv_gpiote.h"
#include "nrf_drv_pwm.h"
#include "app_timer.h"
#include "hw_if_config.h"

#define SPI_INSTANCE  3 /**< SPI instance index. */
static const nrf_drv_spi_t spi =  { SPI_INSTANCE, { .spim = NRFX_SPIM_INSTANCE(SPI_INSTANCE) }, true };  /**< SPI instance. */

static ADI_OSAL_SEM_HANDLE xSpiSem = NULL;

DISPLAY_4BIT dis_buf_4bit[HIGH_SIZE+1][LENGTH_SIZE/8*DISPLAY_OUT_4BIT+CMD_OFFSET] ={{0}};
DISPLAY_1BIT dis_buf_1bit[HIGH_SIZE+1][LENGTH_SIZE/8*DISPLAY_OUT_1BIT+CMD_OFFSET] ={{0}};
lcd_dis_out_bit lcd_bit_indicate = DISPLAY_OUT_4BIT;

static nrf_drv_pwm_t m_pwm1 = NRF_DRV_PWM_INSTANCE(1);

#define LCD_PWM_TOP (1000)

static nrf_pwm_values_common_t m_lcd_seq_values;
static nrf_pwm_sequence_t const    m_lcd_seq =
{
    .values.p_common = &m_lcd_seq_values,
    .length              = NRF_PWM_VALUES_LENGTH(m_lcd_seq_values),
    .repeats             = 0,
    .end_delay           = 0
};
#ifdef BACKLIGHT_ADJUST_TEST
static nrf_drv_pwm_t m_pwm0 = NRF_DRV_PWM_INSTANCE(0);

#define OHR_PWM_TOP (10)

static nrf_pwm_values_common_t m_demo1_seq_values;
static nrf_pwm_sequence_t const    m_demo1_seq =
{
    .values.p_common = &m_demo1_seq_values,
    .length              = NRF_PWM_VALUES_LENGTH(m_demo1_seq_values),
    .repeats             = 0,
    .end_delay           = 0
};
#endif
APP_TIMER_DEF(m_reverse_polarity_tmr);
#define REVERSE_POLARITY_PERIOD (10000)//10s Make EXTCOMIN freq=0.1Hz, which otherwise was affecting toptouch stability

static uint8_t lcd_backlight_status = 0;
static lcd_reverse_frequency lcd_extcomin_status = DISPLAY_REVERSE_NO;

void lcd_dis_bit_set(lcd_dis_out_bit lcd_dis_bit)
{
    lcd_bit_indicate = lcd_dis_bit;
}

lcd_dis_out_bit lcd_dis_bit_get(void)
{
    return lcd_bit_indicate;
}

void lcd_background_color_set(uint8_t value)
{
    int i,j = 0;
    uint16_t length,offset = 0;
    uint8_t *dis_buff = NULL;
    if(DISPLAY_OUT_4BIT == lcd_bit_indicate)
    {
        length = LENGTH_SIZE/8*DISPLAY_OUT_4BIT + CMD_OFFSET;
        dis_buff = (uint8_t *)dis_buf_4bit;
    }
    else if(DISPLAY_OUT_1BIT == lcd_bit_indicate)
    {
        length = LENGTH_SIZE/8*DISPLAY_OUT_1BIT + CMD_OFFSET;
        dis_buff = (uint8_t *)dis_buf_1bit;
    }
    for(i = 0,offset = 0;i<HIGH_SIZE;i++,offset+=length)
    {
        for(j=CMD_OFFSET;j< length;j++)
        {
            *(dis_buff+offset+j) = value;
        }
    }
}

void lcd_background_color_set_section(uint8_t y0,uint8_t y1,uint8_t value)
{
    int i,j = 0;
    uint16_t length,offset = 0;
    uint8_t *dis_buff = NULL;
    if(DISPLAY_OUT_4BIT == lcd_bit_indicate)
    {
        length = LENGTH_SIZE/8*DISPLAY_OUT_4BIT + CMD_OFFSET;
        dis_buff = (uint8_t *)dis_buf_4bit;
    }
    else if(DISPLAY_OUT_1BIT == lcd_bit_indicate)
    {
        length = LENGTH_SIZE/8*DISPLAY_OUT_1BIT + CMD_OFFSET;
        dis_buff = (uint8_t *)dis_buf_1bit;
    }
    for(i = y0,offset = length*y0;i<y1;i++,offset+=length)
    {
        for(j=CMD_OFFSET;j< length;j++)
        {
            *(dis_buff+offset+j) = value;
        }
    }
}

void lcd_buffer_init(void)
{
    int i = 0;
    for(i = 0;i<HIGH_SIZE;i++)
    {
        dis_buf_4bit[i][0].byte = (DISPLAY_4BIT_CMD << 2);//cmd
        dis_buf_1bit[i][0].byte = (DISPLAY_1BIT_CMD << 2);//cmd
#if (Y_AXIS_DIRECTION == 0)
        dis_buf_4bit[i][1].byte = i+1;//address
        dis_buf_1bit[i][1].byte = i+1;//address
#else
        dis_buf_4bit[i][1].byte = HIGH_SIZE -i;//address
        dis_buf_1bit[i][1].byte = HIGH_SIZE -i;//address
#endif
    }
}
void lcd_spi_port_uninit(void)
{
    nrfx_spim_uninit(&spi.u.spim );
}
void lcd_pwm_port_uninit(void)
{
    nrf_drv_pwm_uninit(&m_pwm1);
}
static void lcd_spi_event_handler(nrfx_spim_evt_t const * p_event,void *                  p_context)
{
    if(p_event->type == NRFX_SPIM_EVENT_DONE)
    {
        adi_osal_SemPost( xSpiSem);
    }
}
void lcd_spi_port_init(void)
{
    nrfx_spim_config_t config_spim = NRFX_SPIM_DEFAULT_CONFIG;
    config_spim.sck_pin        = LCD_SPI_SCK_PIN;
    config_spim.mosi_pin       = LCD_SPI_MOSI_PIN;
    config_spim.miso_pin       = NRF_DRV_SPI_PIN_NOT_USED;
    config_spim.ss_pin         = LCD_SPI_SS_3388_EN_PIN;
    config_spim.frequency      = NRF_SPIM_FREQ_2M;//NRF_DRV_SPI_FREQ_2M
    config_spim.mode           = NRF_SPIM_MODE_0;
    config_spim.bit_order      = NRF_SPIM_BIT_ORDER_MSB_FIRST;
    config_spim.ss_active_high = true;
    APP_ERROR_CHECK(nrfx_spim_init(&spi.u.spim,&config_spim,lcd_spi_event_handler,(void *)&spi.inst_idx));
}


void lcd_pwm_port_init(void)
{
    nrf_drv_pwm_config_t const config1 =
    {
        .output_pins =
        {
            LCD_EXTCOMIN | NRF_DRV_PWM_PIN_INVERTED,    // channel 0
            NRF_DRV_PWM_PIN_NOT_USED,    // channel 1
            NRF_DRV_PWM_PIN_NOT_USED,    // channel 2
            NRF_DRV_PWM_PIN_NOT_USED     // channel 3
        },
        .irq_priority = APP_IRQ_PRIORITY_LOWEST,
        .base_clock   = NRF_PWM_CLK_125kHz,
        .count_mode   = NRF_PWM_MODE_UP,
        .top_value    = LCD_PWM_TOP,
        .load_mode    = NRF_PWM_LOAD_COMMON,
        .step_mode    = NRF_PWM_STEP_AUTO
    };
    nrf_drv_pwm_init(&m_pwm1, &config1, NULL);
    m_lcd_seq_values = LCD_PWM_TOP/2;
}


static void lcd_spi_send(uint8_t const * buffer,uint16_t length)
{
    nrfx_spim_xfer_desc_t const spim_xfer_desc =
    {
        .p_tx_buffer = buffer,
        .tx_length   = length,
        .p_rx_buffer = NULL,
        .rx_length   = 0,
    };

    lcd_spi_port_init();
    APP_ERROR_CHECK(nrfx_spim_xfer(&spi.u.spim, &spim_xfer_desc, 0));
    adi_osal_SemPend(xSpiSem, ADI_OSAL_TIMEOUT_FOREVER);//move to here prevent the buffer be modified when transmitting.
    lcd_spi_port_uninit();
}

#ifdef BACKLIGHT_ADJUST_TEST
void backlight_pwm_init(void)
{
    uint32_t ret;
    nrf_drv_pwm_config_t const config0 =
    {
        .output_pins =
        {
            LCD_BL_EN_PIN,    // channel 0
            NRF_DRV_PWM_PIN_NOT_USED,    // channel 1
            NRF_DRV_PWM_PIN_NOT_USED,    // channel 2
            NRF_DRV_PWM_PIN_NOT_USED     // channel 3
        },
        .irq_priority = APP_IRQ_PRIORITY_LOWEST,
        .base_clock   = NRF_PWM_CLK_125kHz,
        .count_mode   = NRF_PWM_MODE_UP,
        .top_value    = OHR_PWM_TOP,
        .load_mode    = NRF_PWM_LOAD_COMMON,
        .step_mode    = NRF_PWM_STEP_AUTO
    };
    if(NRFX_SUCCESS != nrf_drv_pwm_init(&m_pwm0, &config0, NULL))
    {
        return;
    }
    m_demo1_seq_values = (OHR_PWM_TOP/2)|0x8000;
}
#endif
void lcd_backlight_set(uint8_t status)
{
    lcd_backlight_status = status;
    if(LCD_BACKLIGHT_OFF == lcd_backlight_status)
    {
#ifdef BACKLIGHT_ADJUST_TEST
        nrf_drv_pwm_stop(&m_pwm0,true);
#else
        nrf_gpio_pin_clear(LCD_BL_EN_PIN);
#endif
    }
    else if(LCD_BACKLIGHT_ON == lcd_backlight_status)
    {
#ifdef BACKLIGHT_ADJUST_TEST
        nrf_drv_pwm_simple_playback(&m_pwm0, &m_demo1_seq, 1,
                                      NRF_DRV_PWM_FLAG_LOOP);
#else
        nrf_gpio_pin_set(LCD_BL_EN_PIN);
#endif
    }
    else
    {
        ;
    }
}

#ifdef BACKLIGHT_ADJUST_TEST
void lcd_backlight_adjust(uint8_t percent)
{
    nrf_pwm_values_t value;
    m_demo1_seq_values = (percent*OHR_PWM_TOP/100)|0x8000;
    value.p_common = &m_demo1_seq_values;
    nrf_drv_pwm_sequence_values_update(&m_pwm0,1,value);
}
#endif

uint8_t lcd_backlight_get(void)
{
    return lcd_backlight_status;
}
/*****************************************************************************
 * Function      : lcd_display_clear
 * Description   : Clear display screen
 * Input         : void
 * Output        : None
 * Return        :
 * Others        :
 * Record
 * 1.Date        : 20200722
 *   Modification: Created function
*****************************************************************************/
void lcd_display_clear(void)
{
    uint8_t command[2] = {0};

    command[0] = (ALL_CLEAR_MODE << 2);
    lcd_spi_send(command,2);
}

static void lcd_reverse_polarity_handler(void * p_context)
{
    nrf_gpio_pin_set(LCD_EXTCOMIN);//
    nrf_delay_us(2);//At least 2us.
    nrf_gpio_pin_clear(LCD_EXTCOMIN);//
}

void lcd_port_init(void)
{
    ret_code_t err_code;

    adi_osal_SemCreate(&xSpiSem,0);
    err_code = app_timer_create(&m_reverse_polarity_tmr, APP_TIMER_MODE_REPEATED, lcd_reverse_polarity_handler);
    APP_ERROR_CHECK(err_code);

    nrf_gpio_cfg_output(LCD_DISP_SWITCH);//H:display;L:not display

    lcd_disp_on();//PWN keep open when display open.

    lcd_display_clear();//clear display screen first.
#ifdef BACKLIGHT_ADJUST_TEST
    backlight_pwm_init();
#else
    nrf_gpio_cfg_output(LCD_BL_EN_PIN);
#endif
#ifndef CUST4_SM
    lcd_backlight_set(LCD_BACKLIGHT_OFF);
#endif
}

void lcd_extcomin_status_set(lcd_reverse_frequency status)
{
    if(status == lcd_extcomin_status)
    {
        return;
    }
    if(DISPLAY_REVERSE_LOW == lcd_extcomin_status)
    {
        app_timer_stop(m_reverse_polarity_tmr);
        nrf_gpio_cfg_default(LCD_EXTCOMIN);
        lcd_extcomin_status = DISPLAY_REVERSE_NO;
    }
    if(DISPLAY_REVERSE_HIGH == lcd_extcomin_status)
    {
        nrf_drv_pwm_stop(&m_pwm1,false);
        lcd_pwm_port_uninit(); //PWN close when display close.
        lcd_extcomin_status = DISPLAY_REVERSE_NO;
    }

    if(DISPLAY_REVERSE_LOW == status)
    {
        nrf_gpio_cfg_output(LCD_EXTCOMIN);
        app_timer_start(m_reverse_polarity_tmr, APP_TIMER_TICKS(REVERSE_POLARITY_PERIOD), NULL);
        lcd_extcomin_status = DISPLAY_REVERSE_LOW;
    }
    if(DISPLAY_REVERSE_HIGH == status)
    {
        lcd_pwm_port_init();
        nrf_drv_pwm_simple_playback(&m_pwm1, &m_lcd_seq, 1,NRF_DRV_PWM_FLAG_LOOP);//PWN keep open when display open.
        lcd_extcomin_status = DISPLAY_REVERSE_HIGH;
    }
}

void lcd_disp_on(void)
{
    nrf_gpio_pin_set(LCD_DISP_SWITCH);

    nrf_gpio_cfg_output(LCD_EXTCOMIN);
#ifdef ENABLE_TOP_TOUCH
    app_timer_start(m_reverse_polarity_tmr, APP_TIMER_TICKS(REVERSE_POLARITY_PERIOD), NULL);
    lcd_extcomin_status = DISPLAY_REVERSE_LOW;
#else
    //lcd_extcomin_status = DISPLAY_REVERSE_HIGH;
    lcd_extcomin_status_set(DISPLAY_REVERSE_HIGH);
#endif

}
void lcd_disp_off(void)
{
    nrf_gpio_pin_clear(LCD_DISP_SWITCH);
    if(DISPLAY_REVERSE_LOW == lcd_extcomin_status)
    {
        app_timer_stop(m_reverse_polarity_tmr);
        nrf_gpio_cfg_default(LCD_EXTCOMIN);
        lcd_extcomin_status = DISPLAY_REVERSE_NO;
    }
    if(DISPLAY_REVERSE_HIGH == lcd_extcomin_status)
    {
        nrf_drv_pwm_stop(&m_pwm1,false);
        lcd_pwm_port_uninit(); //PWN close when display close.
        lcd_extcomin_status = DISPLAY_REVERSE_NO;
    }
    lcd_backlight_set(LCD_BACKLIGHT_OFF);//backlight must close after close display.
}



void lcd_display_refresh_section(uint8_t y0,uint8_t y1)
{
    uint8_t *data_ptr = NULL;

    if(DISPLAY_OUT_4BIT == lcd_bit_indicate)
    {
        data_ptr = (uint8_t *)&dis_buf_4bit[y0];
    }
    else if(DISPLAY_OUT_1BIT == lcd_bit_indicate)
    {
        data_ptr = (uint8_t *)&dis_buf_1bit[y0];
    }
    lcd_spi_send(data_ptr, (CMD_OFFSET+LENGTH_SIZE/8*lcd_bit_indicate)*(y1-y0)+CMD_OFFSET);

}

void lcd_display_refresh_all(void)
{
    lcd_display_refresh_section(0,Y_AXIS_MAX);
}
#endif
