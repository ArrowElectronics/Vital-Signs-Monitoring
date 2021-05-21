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

#include "lcd_driver.h"
//#include "pin_config.h"
#include "nrf_drv_spi.h"
//#include "app_pwm.h"
#include "nrf_drv_gpiote.h"
#include "nrf_delay.h"
#include "nrf_drv_pwm.h"
#ifdef PCBA
#include "watch_board_pcba_pin_config.h"
#else
#include "watch_board_evt_pin_config.h"
#endif


#define SPI_INSTANCE  2 /**< SPI instance index. */
static const nrf_drv_spi_t spi =  { SPI_INSTANCE, { .spim = NRFX_SPIM_INSTANCE(SPI_INSTANCE) }, true };   /**< SPI instance. */

static nrf_drv_pwm_t m_pwm1 = NRF_DRV_PWM_INSTANCE(1);

DISPLAY_1BIT dis_buf[HIGH_SIZE+1][LENGTH_SIZE/8*COLOR_BIT+CMD_OFFSET] ={{0}};

#define LCD_PWM_TOP (1250)
static nrf_pwm_values_common_t m_lcd_seq_values;
static nrf_pwm_sequence_t const    m_lcd_seq =
{
    .values.p_common = &m_lcd_seq_values,
    .length              = NRF_PWM_VALUES_LENGTH(m_lcd_seq_values),
    .repeats             = 0,
    .end_delay           = 0
};

void lcd_pwm_port_uninit(void)
{    
    nrf_drv_pwm_uninit(&m_pwm1);
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

void lcd_background_color_set(uint8_t value)
{
    int i,j = 0;
    uint16_t length,offset = 0;
    uint8_t *buff = NULL;

    length = LENGTH_SIZE/8*COLOR_BIT + CMD_OFFSET;
    buff = (uint8_t *)dis_buf;
        
    for(i = 0,offset = 0;i<HIGH_SIZE;i++,offset+=length)
    {
        for(j=CMD_OFFSET;j< length;j++)
        {
            *(buff+offset+j) = value;
        }
    }
}

void lcd_buffer_init(void)
{
    int i = 0;
    for(i = 0;i<HIGH_SIZE;i++)
    {
        dis_buf[i][0].byte = (DISPLAY_1BIT_CMD << 2);//cmd
#if (Y_AXIS_DIRECTION == 0)
        dis_buf[i][1].byte = i+1;//address
#else
        dis_buf[i][1].byte = HIGH_SIZE -i;//address
#endif
    }
}

void lcd_spi_port_init(void)
{
    nrfx_spim_config_t config_spim = NRFX_SPIM_DEFAULT_CONFIG;
    config_spim.ss_pin   = LCD_SPI_SS_3388_EN_PIN;
    config_spim.miso_pin = NRF_DRV_SPI_PIN_NOT_USED;
    config_spim.mosi_pin = LCD_SPI_MOSI_PIN;
    config_spim.sck_pin  = LCD_SPI_SCK_PIN;
    config_spim.frequency = NRF_DRV_SPI_FREQ_2M;
    config_spim.mode           = NRF_SPIM_MODE_0;
    config_spim.bit_order      = NRF_SPIM_BIT_ORDER_MSB_FIRST;
    config_spim.ss_active_high = true;
    APP_ERROR_CHECK(nrfx_spim_init(&spi.u.spim,&config_spim,NULL,(void *)&spi.inst_idx));
}

void lcd_spi_port_uninit(void)
{
    nrfx_spim_uninit(&spi.u.spim );
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
    lcd_spi_port_uninit();
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



void lcd_init(void)
{

    nrf_gpio_cfg_output(PWR_OPTICAL_PIN);    
    nrf_gpio_pin_set(PWR_OPTICAL_PIN);//DVT2 will delete this line

    nrf_gpio_cfg_output(LCD_DISP_SWITCH);//H:display;L:not display
    nrf_gpio_pin_set(LCD_DISP_SWITCH);//先点亮，不操作此引脚          ////
    
    lcd_display_clear();//clear display screen first.
    nrf_gpio_cfg_output(LCD_BL_EN_PIN);
    nrf_gpio_pin_set(LCD_BL_EN_PIN);//

    lcd_pwm_port_init();
    nrf_drv_pwm_simple_playback(&m_pwm1, &m_lcd_seq, 1,NRF_DRV_PWM_FLAG_LOOP);

    lcd_buffer_init();
}

void LCD_disp_on(void)
{
    nrf_gpio_pin_set(LCD_DISP_SWITCH);
}
void LCD_disp_off(void)
{
    nrf_gpio_pin_clear(LCD_DISP_SWITCH);
}

void lcd_display_refresh_all(void)
{
    uint8_t *buff;
    buff = (uint8_t *)&dis_buf[0];

    lcd_spi_send(buff,(CMD_OFFSET+LENGTH_SIZE/8*COLOR_BIT)*Y_AXIS_MAX+CMD_OFFSET);
}




 

