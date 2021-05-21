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
#ifndef _WATCH_BOARD_EVT_PIN_CONFIG_H_
#define _WATCH_BOARD_EVT_PIN_CONFIG_H_

// <0=> 0 (P0.0)
// <1=> 1 (P0.1)
// <2=> 2 (P0.2)
// <3=> 3 (P0.3)
// <4=> 4 (P0.4)
// <5=> 5 (P0.5)
// <6=> 6 (P0.6)
// <7=> 7 (P0.7)
// <8=> 8 (P0.8)
// <9=> 9 (P0.9)
// <10=> 10 (P0.10)
// <11=> 11 (P0.11)
// <12=> 12 (P0.12)
// <13=> 13 (P0.13)
// <14=> 14 (P0.14)
// <15=> 15 (P0.15)
// <16=> 16 (P0.16)
// <17=> 17 (P0.17)
// <18=> 18 (P0.18)
// <19=> 19 (P0.19)
// <20=> 20 (P0.20)
// <21=> 21 (P0.21)
// <22=> 22 (P0.22)
// <23=> 23 (P0.23)
// <24=> 24 (P0.24)
// <25=> 25 (P0.25)
// <26=> 26 (P0.26)
// <27=> 27 (P0.27)
// <28=> 28 (P0.28)
// <29=> 29 (P0.29)
// <30=> 30 (P0.30)
// <31=> 31 (P0.31)
// <32=> 32 (P1.0)
// <33=> 33 (P1.1)
// <34=> 34 (P1.2)
// <35=> 35 (P1.3)
// <36=> 36 (P1.4)
// <37=> 37 (P1.5)
// <38=> 38 (P1.6)
// <39=> 39 (P1.7)
// <40=> 40 (P1.8)
// <41=> 41 (P1.9)
// <42=> 42 (P1.10)
// <43=> 43 (P1.11)
// <44=> 44 (P1.12)
// <45=> 45 (P1.13)
// <46=> 46 (P1.14)
// <47=> 47 (P1.15)
// <4294967295=> Not connected

#define KEY_SELECT_PIN (25)
#define KEY_NAVIGATIONPIN  (32)


#if 1
#define LCD_SPI_SS_3388_EN_PIN (45)
#define LCD_SPI_MOSI_PIN       (44)
#define LCD_SPI_SCK_PIN        (43)
#define LCD_EXTCOMIN           (46)
#define LCD_DISP_SWITCH        (47)
#else
#define LCD_SPI_SS_3388_EN_PIN (29)
#define LCD_SPI_MOSI_PIN       (30)
#define LCD_SPI_SCK_PIN        (31)
#define LCD_EXTCOMIN           (28)
#define LCD_DISP_SWITCH        (4)
#endif
#define LCD_BL_EN_PIN          (42)

#define AD8233_LOD_PIN          (28)
#define AD8233_RLD_PIN          (31)//not used

#define AD5940_SPI_CS_PIN          (13)
#define AD5940_SPI_MOSI_PIN       (15)
#define AD5940_SPI_SCLK_PIN        (14)
#define AD5940_SPI_MISO_PIN        (16)
#define AD5940_RESET_PIN        (17)
#define AD5940_INT0_PIN        (3)

#define ADPD4K_SPI_CS_PIN          (41)
#define ADPD4K_SPI_MOSI_PIN       (8)
#define ADPD4K_SPI_SCLK_PIN        (6)
#define ADPD4K_SPI_MISO_PIN        (7)
#define ADPD4K_INT_PIN        (11)
#define ADPD4K_GPIO3_PIN        (29 ) 

#define ADXL362_SPI_CS_PIN          (40)
#define ADXL362_SPI_MOSI_PIN       (8)
#define ADXL362_SPI_SCLK_PIN        (6)
#define ADXL362_SPI_MISO_PIN        (7)
#define ADXL362_INT1_PIN           (4)
#define ADXL362_INT2_PIN           (5)

#define AD7156_TWI_SCL_PIN  (33)
#define AD7156_TWI_SDA_PIN  (34)
#define AD7156_OUT1_PIN  (38)
#define AD7156_OUT2_PIN  (39)

#define ADP5360_TWI_SCL_PIN  (33)
#define ADP5360_TWI_SDA_PIN  (34)
#define ADP5360_INT1_PIN      (35)
#define ADP5360_PGOOD_PIN    (36)
#define ADP5360_ENSD_PIN    (37)


#define QSPI_NAND_FLASH_SCK_PIN   (19)
#define QSPI_NAND_FLASH_CSN_PIN   (24)
#define QSPI_NAND_FLASH_IO0_PIN   (20)
#define QSPI_NAND_FLASH_IO1_PIN   (21)
#define QSPI_NAND_FLASH_IO2_PIN   (23)
#define QSPI_NAND_FLASH_IO3_PIN   (22)

#define PWR_FLASH_PIN    (2)
#define PWR_OPTICAL_PIN  (27)
#define PWE_EPHYZ_PIN    (26)

//#define PWM_RESET_PIN    (10)
#define HARDWARE_RESET_PIN (18)
#endif //_WATCH_BOARD_EVT_PIN_CONFIG_H_