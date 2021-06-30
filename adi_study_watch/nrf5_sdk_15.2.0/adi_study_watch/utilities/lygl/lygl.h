/**
****************************************************************************
* @file     lygl.h
* @author   ADI
* @version  V0.1
* @date     11-Mar-2021
* @brief    This is the header file used for lygl
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

#ifndef _LYGL_H_
#define _LYGL_H_

#include "lygl_def.h"

typedef struct{
    uint16_t x0;//The origin of the axes
    uint16_t y0;//The origin of the axes
    uint16_t x1;//x_axis length
    uint16_t y1;
    uint16_t x_space;
    uint32_t y_unit;//unit and offset holds application related data,so 32bit is used 
    uint32_t y_offset;
    uint8_t wf_color;
    uint8_t bg_color;
}lygl_graph_param_t;

void lygl_dis_string(const lv_font_t *font,uint8_t x,uint8_t y,uint8_t color,char *string);
void lygl_dis_string_middle(const lv_font_t *font,uint8_t x,uint8_t y,uint8_t color,char *string);
void lygl_dis_value_middle(const lv_font_t *font,uint8_t x,uint8_t y,uint8_t color,int32_t value,uint8_t dis_len);
void lygl_dis_decimal_middle(const lv_font_t *font,uint8_t x,uint8_t y,uint8_t color,int32_t value,uint8_t dec_len);

void lygl_draw_image(const lv_img_dsc_t * icon,uint8_t x,uint8_t y);

void lygl_draw_line(uint8_t x1,uint8_t y1,uint8_t x2,uint8_t y2,uint8_t color);
void lygl_draw_rectangle(uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2,uint8_t color);
void lygl_creat_graph(lygl_graph_param_t *graph_param);
void lygl_send_graph_data(uint32_t *data,uint16_t len,uint32_t valid_bits);

void lygl_creat_battery(const lv_img_dsc_t * battery_ico,uint8_t x,uint8_t y,uint8_t dir);
void lygl_battry_level(uint8_t percent);

void lygl_creat_process_bar(const lv_img_dsc_t * battery_ico,uint8_t x,uint8_t y,uint8_t dir);
void lygl_process_bar_level(uint8_t percent);
#endif




