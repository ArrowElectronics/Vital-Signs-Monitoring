/**
****************************************************************************
* @file     lygl_battry_level.c
* @author   ADI
* @version  V0.1
* @date     11-Mar-2021
* @brief    This is the source file used to display battery level info.
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
#include "lygl.h"

static uint16_t x_start = 0xffff;
static uint16_t y_start = 0xffff;
static uint16_t x_end = 0xffff;
static uint16_t y_end = 0xffff;
static uint8_t fill_color = 0x00;
static uint8_t bk_color = 0x00;
static uint8_t direction = 0x00;

static const lv_img_dsc_t * static_ico;
static uint8_t static_x;
static uint8_t static_y;
/*
@param dir:0,x-axis direction,1,y-axis direction
*/
void lygl_creat_battery(const lv_img_dsc_t * battery_ico,uint8_t x,uint8_t y,uint8_t dir)
{
    static uint8_t x_middle,y_middle;
    static uint8_t x_cnt,y_cnt;

    if((x>X_AXIS_MAX)||(y>Y_AXIS_MAX))
    {
        return;
    }
    static_ico = battery_ico;
    static_x = x;
    static_y = y;
    lygl_draw_image(static_ico,static_x,static_y);
    direction = dir;
    if(direction != 0)
    {
        x_middle = battery_ico->XSize/2 + x;
        y_middle = battery_ico->YSize + y - 1;
        fill_color = lygl_get_dot_color(x_middle,y_middle);
        for(y_cnt = 0;y_cnt<battery_ico->YSize;y_cnt++)
        {
            bk_color = lygl_get_dot_color(x_middle,(y_middle - y_cnt));
            if(fill_color != bk_color)
            {
                break;
            }
        }
        for(;y_cnt<battery_ico->YSize;y_cnt++)
        {
            fill_color = lygl_get_dot_color(x_middle,(y_middle - y_cnt));
            if(fill_color != bk_color)
            {
                break;
            }
        }
        y_end = y_middle - y_cnt;
        for(;y_cnt<battery_ico->YSize;y_cnt++)
        {
            if(fill_color != lygl_get_dot_color(x_middle,(y_middle - y_cnt)))
            {
                break;
            }
        }
        y_start = y_middle - y_cnt + 1;
        for(x_cnt=0;x_cnt<battery_ico->XSize/2;x_cnt++)
        {
            if(fill_color != lygl_get_dot_color(x_middle-x_cnt,y_end))
            {
                break;
            }
        }
        x_start = x_middle-x_cnt +1;
        for(x_cnt=0;x_cnt<battery_ico->XSize/2;x_cnt++)
        {
            if(fill_color != lygl_get_dot_color(x_middle+x_cnt,y_end))
            {
                break;
            }
        }
        x_end = x_middle+x_cnt-1;
    }
    else
    {
        y_middle = battery_ico->YSize/2 + y;
        x_middle = x;
        fill_color = lygl_get_dot_color(x_middle,y_middle);
        for(x_cnt = 0;x_cnt<battery_ico->XSize;x_cnt++)
        {
            bk_color = lygl_get_dot_color((x_middle + x_cnt),y_middle);
            if(fill_color != bk_color)
            {
                break;
            }
        }
        for(;x_cnt<battery_ico->XSize;x_cnt++)
        {
            fill_color = lygl_get_dot_color((x_middle + x_cnt),y_middle);
            if(fill_color != bk_color)
            {
                break;
            }
        }
        x_start = x_middle + x_cnt;
        for(;x_cnt<battery_ico->XSize;x_cnt++)
        {
            if(fill_color != lygl_get_dot_color((x_middle + x_cnt),y_middle))
            {
                break;
            }
        }
        x_end = x_middle + x_cnt - 1;
        for(y_cnt=0;y_cnt<battery_ico->YSize/2;y_cnt++)
        {
            if(fill_color != lygl_get_dot_color(x_start,y_middle - y_cnt))
            {
                break;
            }
        }
        y_start = y_middle-y_cnt +1 ;
        for(y_cnt=0;y_cnt<battery_ico->YSize/2;y_cnt++)
        {
            if(fill_color != lygl_get_dot_color(x_start,y_middle + y_cnt))
            {
                break;
            }
        }
        y_end = y_middle+y_cnt -1;
    }
}


void lygl_battry_level(uint8_t percent)
{
    static uint16_t y_length,y_start_v = 0;
    static uint16_t x_length,x_end_v = 0;
    lygl_draw_image(static_ico,static_x,static_y);
    if(direction != 0)
    {
        y_length = y_end - y_start;
        y_start_v = y_end - y_length*percent/100;

        lygl_draw_rectangle(x_start,y_start,x_end,y_end,bk_color);
        if(0 != percent)
        {
            lygl_draw_rectangle(x_start,y_start_v,x_end,y_end,fill_color);
        }
    }
    else
    {
        x_length = x_end - x_start;
        x_end_v = x_start + x_length*percent/100;
        lygl_draw_rectangle(x_start,y_start,x_end,y_end,bk_color);
        if(0 != percent)
        {
            lygl_draw_rectangle(x_start,y_start,x_end_v,y_end,fill_color);
        }

    }
}
#endif
