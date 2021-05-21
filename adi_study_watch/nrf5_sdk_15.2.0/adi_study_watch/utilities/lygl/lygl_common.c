/**
****************************************************************************
* @file     lygl_common.c
* @author   ADI
* @version  V0.1
* @date     11-Mar-2021
* @brief    This is the source file used to offer common API of lygl
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
**   or more patent holders.  This license does not release you from the
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
#include "lygl_common.h"
#include "lcd_driver.h"
#include "lygl.h"

extern lcd_dis_out_bit lcd_bit_indicate;
inline void lygl_draw_dot(uint8_t x,uint8_t y,uint8_t color)
{
    if(DISPLAY_OUT_4BIT == lcd_bit_indicate)
    {
        if(x&0x01)
        {
            dis_buf_4bit[y][CMD_OFFSET+((X_AXIS_MAX-x)>>1)].f4bit.pixel2 = color;
        }
        else
        {
            dis_buf_4bit[y][CMD_OFFSET+((X_AXIS_MAX-x)>>1)].f4bit.pixel1 = color;
        }
    }
    else if(DISPLAY_OUT_1BIT == lcd_bit_indicate)
    {
        switch(x&0x07)
        {
            case 0:
            {
                dis_buf_1bit[y][CMD_OFFSET+((X_AXIS_MAX-x)>>3)].f1bit.pixel1 = color;
            }
            break;
            case 1:
            {
                dis_buf_1bit[y][CMD_OFFSET+((X_AXIS_MAX-x)>>3)].f1bit.pixel2 = color;
            }
            break;
            case 2:
            {
                dis_buf_1bit[y][CMD_OFFSET+((X_AXIS_MAX-x)>>3)].f1bit.pixel3 = color;
            }
            break;
            case 3:
            {
                dis_buf_1bit[y][CMD_OFFSET+((X_AXIS_MAX-x)>>3)].f1bit.pixel4 = color;
            }
            break;
            case 4:
            {
                dis_buf_1bit[y][CMD_OFFSET+((X_AXIS_MAX-x)>>3)].f1bit.pixel5 = color;
            }
            break;
            case 5:
            {
                dis_buf_1bit[y][CMD_OFFSET+((X_AXIS_MAX-x)>>3)].f1bit.pixel6 = color;
            }
            break;
            case 6:
            {
                dis_buf_1bit[y][CMD_OFFSET+((X_AXIS_MAX-x)>>3)].f1bit.pixel7 = color;
            }
            break;
            case 7:
            {
                dis_buf_1bit[y][CMD_OFFSET+((X_AXIS_MAX-x)>>3)].f1bit.pixel8 = color;
            }
            break;
            default:break;
        }
    }
    else//3bit color//
    {
    }
}

inline uint8_t lygl_get_dot_color(uint8_t x,uint8_t y)
{
    uint8_t color_ret = 0;

    if(DISPLAY_OUT_4BIT == lcd_bit_indicate)
    {

        if(x&0x01)
        {
            color_ret = dis_buf_4bit[y][CMD_OFFSET+((X_AXIS_MAX-x)>>1)].f4bit.pixel2;
        }
        else
        {
            color_ret = dis_buf_4bit[y][CMD_OFFSET+((X_AXIS_MAX-x)>>1)].f4bit.pixel1;
        }
    }
    else if(DISPLAY_OUT_1BIT == lcd_bit_indicate)
    {
        switch(x&0x07)
        {
            case 0:
            {
                color_ret = dis_buf_1bit[y][CMD_OFFSET+((X_AXIS_MAX-x)>>3)].f1bit.pixel1;
            }
            break;
            case 1:
            {
                color_ret = dis_buf_1bit[y][CMD_OFFSET+((X_AXIS_MAX-x)>>3)].f1bit.pixel2;
            }
            break;
            case 2:
            {
                color_ret = dis_buf_1bit[y][CMD_OFFSET+((X_AXIS_MAX-x)>>3)].f1bit.pixel3;
            }
            break;
            case 3:
            {
                color_ret = dis_buf_1bit[y][CMD_OFFSET+((X_AXIS_MAX-x)>>3)].f1bit.pixel4;
            }
            break;
            case 4:
            {
                color_ret = dis_buf_1bit[y][CMD_OFFSET+((X_AXIS_MAX-x)>>3)].f1bit.pixel5;
            }
            break;
            case 5:
            {
                color_ret = dis_buf_1bit[y][CMD_OFFSET+((X_AXIS_MAX-x)>>3)].f1bit.pixel6;
            }
            break;
            case 6:
            {
                color_ret = dis_buf_1bit[y][CMD_OFFSET+((X_AXIS_MAX-x)>>3)].f1bit.pixel7;
            }
            break;
            case 7:
            {
                color_ret = dis_buf_1bit[y][CMD_OFFSET+((X_AXIS_MAX-x)>>3)].f1bit.pixel8;
            }
            break;
            default:break;
        }
    }
    else//3bit color//
    {
    }
    return  (color_ret);
}
void lygl_draw_rectangle(uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2,uint8_t color)
{
    uint8_t x,y;
    if((x1>X_AXIS_MAX)||(x2>X_AXIS_MAX)||(y1>Y_AXIS_MAX)||(y2>Y_AXIS_MAX))
    {
        return;//error
    }
    for(x=x1;x<=x2;x++)
    {
        for(y=y1;y<=y2;y++)
        {
            lygl_draw_dot(x,y,color);
        }
    }
}
#endif