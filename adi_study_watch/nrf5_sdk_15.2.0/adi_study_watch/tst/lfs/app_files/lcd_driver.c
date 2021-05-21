#include "lcd_driver.h"
#include "pin_config.h"
#include "nrf_drv_spi.h"
#include "app_pwm.h"
#include "nrf_drv_gpiote.h"


#define SPI_INSTANCE  3 /**< SPI instance index. */
static const nrf_drv_spi_t spi =  { 3, { .spim = NRFX_SPIM_INSTANCE(3) }, true };  /**< SPI instance. */

APP_PWM_INSTANCE(PWM1,1);                   // Create the instance "PWM1" using TIMER1.


uint8_t dis_buf[HIGH_SIZE][LENGTH_SIZE/8*COLOR_BIT] ={{0}};
uint8_t bit_indicate = 0;

void Set_color_bit(uint8_t color_bit)
{
    bit_indicate = color_bit;
}

uint8_t Get_color_bit(void)
{
    return bit_indicate;
}
void Lcd_color(uint8_t value)
{
    int i,j = 0;
    for(i = 0;i<HIGH_SIZE;i++)
    {
        for(j=0;j<LENGTH_SIZE/8*COLOR_BIT;j++)
        {
            dis_buf[i][j] = value;
        }
    }
}
void lcd_init(void)
{
    ret_code_t err_code;

    nrf_drv_spi_config_t spi_config = NRF_DRV_SPI_DEFAULT_CONFIG;
    spi_config.ss_pin   = NRF_DRV_SPI_PIN_NOT_USED;
    spi_config.miso_pin = NRF_DRV_SPI_PIN_NOT_USED;
    spi_config.mosi_pin = LCD_SPI_MOSI_PIN;
    spi_config.sck_pin  = LCD_SPI_SCK_PIN;
    spi_config.frequency = NRF_DRV_SPI_FREQ_2M;
    APP_ERROR_CHECK(nrf_drv_spi_init(&spi, &spi_config, NULL, NULL));

    /* 2-channel PWM, 200Hz, output on DK LED pins. */
    app_pwm_config_t pwm1_cfg = APP_PWM_DEFAULT_CONFIG_1CH(10000L, LCD_EXTCOMIN);
    /* Switch the polarity of the second channel. */
    pwm1_cfg.pin_polarity[0] = APP_PWM_POLARITY_ACTIVE_HIGH;
    /* Initialize and enable PWM. */
    err_code = app_pwm_init(&PWM1,&pwm1_cfg,NULL);
    APP_ERROR_CHECK(err_code);
    app_pwm_enable(&PWM1);
    app_pwm_channel_duty_set(&PWM1, 0, 1);
    app_pwm_disable(&PWM1);

    nrf_gpio_cfg_output(LCD_DISP_SWITCH);//H:display;L:not display
    nrf_gpio_pin_write(LCD_DISP_SWITCH,1);//先点亮，不操作此引脚          ////

    nrf_gpio_cfg_output(LCD_SPI_SS_3388_EN_PIN);
    nrf_gpio_pin_write(LCD_SPI_SS_3388_EN_PIN,0);//
                  
    nrf_gpio_cfg_output(LCD_BL_EN_PIN);
    nrf_gpio_pin_write(LCD_BL_EN_PIN,0);//
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
//    return;
    uint8_t i;
    uint8_t cmd_addr_value[2] = {0};
    cmd_addr_value[0] = (DISPLAY_OUT_4BIT << 2);
    app_pwm_enable(&PWM1);
    app_pwm_channel_duty_set(&PWM1, 0, 1);
    nrf_gpio_pin_write(LCD_SPI_SS_3388_EN_PIN,1);
    for(i=0;i<HIGH_SIZE;i++)
    {
        cmd_addr_value[1]++;
        APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi, (uint8_t *)cmd_addr_value, 2, NULL, 0));
        APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi, (uint8_t *)&dis_buf[i], LENGTH_SIZE/8*COLOR_BIT, NULL, 0));
    }
    APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi, (uint8_t *)cmd_addr_value, 2, NULL, 0));
    nrf_gpio_pin_write(LCD_SPI_SS_3388_EN_PIN,0);
    app_pwm_disable(&PWM1);
}

void lcd_display_refresh_all_1bit(void)
{
//    return;
    uint8_t i;
    uint8_t cmd_addr_value[2] = {0};
    cmd_addr_value[0] = (DISPLAY_OUT_1BIT << 2);
    app_pwm_enable(&PWM1);
    app_pwm_channel_duty_set(&PWM1, 0, 1);
    nrf_gpio_pin_write(LCD_SPI_SS_3388_EN_PIN,1);
    for(i=0;i<HIGH_SIZE;i++)
    {
        cmd_addr_value[1]++;
        APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi, (uint8_t *)cmd_addr_value, 2, NULL, 0));
        APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi, (uint8_t *)&dis_buf[i], LENGTH_SIZE/8, NULL, 0));
    }
    APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi, (uint8_t *)cmd_addr_value, 2, NULL, 0));
    nrf_gpio_pin_write(LCD_SPI_SS_3388_EN_PIN,0);
    app_pwm_disable(&PWM1);
}

void lcd_display_refresh_all_3bit(void)
{
    uint8_t i;
    uint8_t cmd_addr_value[2] = {0};
    cmd_addr_value[0] = (DISPLAY_OUT_3BIT << 2);
    app_pwm_enable(&PWM1);
    app_pwm_channel_duty_set(&PWM1, 0, 1);
    nrf_gpio_pin_write(LCD_SPI_SS_3388_EN_PIN,1);
    for(i=0;i<HIGH_SIZE;i++)
    {
        cmd_addr_value[1]++;
        APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi, (uint8_t *)cmd_addr_value, 2, NULL, 0));
        APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi, (uint8_t *)&dis_buf[i], LENGTH_SIZE/8*3, NULL, 0));
    }
    APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi, (uint8_t *)cmd_addr_value, 2, NULL, 0));
    nrf_gpio_pin_write(LCD_SPI_SS_3388_EN_PIN,0);
    app_pwm_disable(&PWM1);
}

void lcd_display_refresh_section(uint8_t x0,uint8_t x1)
{
    uint16_t cmd_addr_value,i;
    cmd_addr_value = (DISPLAY_OUT_4BIT << 10) + x0;
    app_pwm_enable(&PWM1);
    nrf_gpio_pin_write(LCD_SPI_SS_3388_EN_PIN,1);
    for(i=x0;i<=x1;i++)
    {
        cmd_addr_value++;
        APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi, (uint8_t *)&cmd_addr_value, 2, NULL, 0));
        APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi, (uint8_t *)&dis_buf[i], LENGTH_SIZE/8*COLOR_BIT, NULL, 0));
    }
    APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi, (uint8_t *)&cmd_addr_value, 2, NULL, 0));
    nrf_gpio_pin_write(LCD_SPI_SS_3388_EN_PIN,0);
    app_pwm_disable(&PWM1);
}  

