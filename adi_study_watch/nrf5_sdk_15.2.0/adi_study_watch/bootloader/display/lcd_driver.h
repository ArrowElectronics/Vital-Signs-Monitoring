/**
****************************************************************************
* @file     lcd_driver.h
* @author   ADI
* @version  V0.1
* @date     20-May-2020
* @brief    This header file is used to implement lcd driver functions
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

#ifndef _LCD_DRIVER_H_
#define _LCD_DRIVER_H_

#include <stdint.h>

typedef union{
    struct{
    uint8_t pixel1:1;
    uint8_t pixel2:1;
    uint8_t pixel3:1;
    uint8_t pixel4:1;
    uint8_t pixel5:1;
    uint8_t pixel6:1;
    uint8_t pixel7:1;
    uint8_t pixel8:1;
    }f1bit;
    uint8_t byte;
}DISPLAY_1BIT;

#define DISPLAY_4BIT_CMD (0x24)
#define DISPLAY_1BIT_CMD (0x22)
#define DISPLAY_3BIT_CMD (0x20)

#define NO_UPDATE_MODE  (0x28)
#define ALL_CLEAR_MODE  (0x08) //low 3 bit can be H or L.
#define DISPLAY_WHILE_COLOR  (0x06)
#define DISPLAY_BLACK_COLOR  (0x04)
#define COLOR_INVERSION_MODE  (0x05)

#define LENGTH_SIZE (208)
#define HIGH_SIZE   (208)
#define CMD_OFFSET  (2)//byte

#define X_AXIS_MAX  (LENGTH_SIZE - 1)
#define Y_AXIS_MAX  (HIGH_SIZE - 1)

#define COLOR_BIT   (1) //先暂时用4，后续再尝试使用3

#define COLOR_WHITE (0xff)
#define COLOR_BLACK (0x00)

#define X_AXIS_DIRECTION (0)//0:positive,1:reverse
#define Y_AXIS_DIRECTION (1)//0:positive,1:reverse

extern DISPLAY_1BIT dis_buf[HIGH_SIZE+1][LENGTH_SIZE/8*COLOR_BIT+CMD_OFFSET];

void lcd_init(void);
void LCD_disp_on(void);
void LCD_disp_off(void);
void lcd_background_color_set(uint8_t value);
void lcd_display_refresh_all(void);
#endif


