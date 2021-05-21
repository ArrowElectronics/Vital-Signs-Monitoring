/**
****************************************************************************
* @file     lygl_text.c
* @author   ADI
* @version  V0.1
* @date     11-Mar-2021
* @brief    This is the source file used to display text.
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

void lygl_dis_string(const lv_font_t *font,uint8_t x,uint8_t y,uint8_t color,char *string)
{
    uint16_t x_spot,y_spot,x_len,y_len,x_byte;
    uint16_t offset,array_offset;
    uint16_t h,l,l_d,l_s;
    char *ptr;
    const unsigned char * fData;

    if((x>X_AXIS_MAX)||(y>Y_AXIS_MAX))
    {
        return;
    }
    y_len = font->h_px;

    for(ptr = string,x_spot = x;*ptr != '\0';ptr++)
    {
        offset = *ptr - font->unicode_first;
        x_len = font->glyph_dsc[offset].w_px;
        x_byte = ((x_len&0x07) == 0)? (x_len/8):(x_len/8 +1);
        array_offset = font->glyph_dsc[offset].glyph_index;
        fData = &(font->glyph_bitmap[array_offset]);
        for(l = 0;l < x_len;l++,x_spot++)
        {
            l_d = l>>3;
            l_s = (0x80 >> (l&0x07));
            for(h = 0,y_spot = y;h < y_len;h++,l_d+=x_byte,y_spot++)
            {
                if(0 != (fData[l_d]&l_s))
                {
                    lygl_draw_dot(x_spot,y_spot,color);
                }
            }
        }
        x_spot += font->monospace;
    }
}

void lygl_dis_string_middle(const lv_font_t *font,uint8_t x,uint8_t y,uint8_t color,char *string)//
{

    uint16_t x_spot,y_len;
//    uint16_t x_len;
//    uint16_t x_spot_int,x_spot_remain,x_byte;
    uint16_t offset;
//    uint8_t h,l,l_d,l_s;
    char *ptr;

    if((NULL == font)||(NULL == string))
    {
        return;
    }

    y_len = font->h_px;
    for(x_spot = 0,ptr = string;*ptr != '\0';ptr++)
    {
        offset = *ptr - font->unicode_first;
        x_spot += (font->glyph_dsc[offset].w_px+font->monospace);
    }
    if(x >= x_spot/2)
    {
        x -= x_spot/2;
    }
    else
    {
        return;//
    }
    if(y > y_len/2)
    {
        y -= y_len/2;
    }
    else
    {
        return;//
    }
    lygl_dis_string(font,x,y,color,string);
}

void lygl_dis_value_middle(const lv_font_t *font,uint8_t x,uint8_t y,uint8_t color,int32_t value,uint8_t dis_len)
{
    char v_string[12] = {0};
    uint8_t i = 0;
    uint8_t valid_flg = 0;
    if(value < 0)
    {
        v_string[i++] = '-';
        value = 0 - value;
    }
    if(value >= 1000000)
    {
        v_string[i++] = value/1000000 + '0';
        valid_flg = 1;
        value = value%1000000;
    }
    else
    {
        if(dis_len >= 7)
        {
            valid_flg = 1;
        }
        if(1 == valid_flg)
        {
            v_string[i++] = '0';
        }
    }
    if(value >= 100000)
    {
        v_string[i++] = value/100000 + '0';
        valid_flg = 1;
        value = value%100000;
    }
    else
    {
        if(dis_len >= 6)
        {
            valid_flg = 1;
        }
        if(1 == valid_flg)
        {
            v_string[i++] = '0';
        }
    }
    if(value >= 10000)
    {
        v_string[i++] = value/10000 + '0';
        valid_flg = 1;
        value = value%10000;
    }
    else
    {
        if(dis_len >= 5)
        {
            valid_flg = 1;
        }
        if(1 == valid_flg)
        {
            v_string[i++] = '0';
        }
    }
    if(value >= 1000)
    {
        v_string[i++] = value/1000 + '0';
        valid_flg = 1;
        value = value%1000;
    }
    else
    {
        if(dis_len >= 4)
        {
            valid_flg = 1;
        }
        if(1 == valid_flg)
        {
            v_string[i++] = '0';
        }
    }
    if(value >= 100)
    {
        v_string[i++] = value/100 + '0';
        valid_flg = 1;
        value = value%100;
    }
    else
    {
        if(dis_len >= 3)
        {
            valid_flg = 1;
        }
        if(1 == valid_flg)
        {
            v_string[i++] = '0';
        }
    }
    if(value >= 10)
    {
        v_string[i++] = value/10 + '0';
        valid_flg = 1;
        value = value%10;
    }
    else
    {
        if(dis_len >= 2)
        {
            valid_flg = 1;
        }
        if(1 == valid_flg)
        {
            v_string[i++] = '0';
        }
    }

    v_string[i++] = value + '0';
    v_string[i++] = '\0';

    lygl_dis_string_middle(font,x,y,color,v_string);
}

void lygl_dis_decimal_middle(const lv_font_t *font,uint8_t x,uint8_t y,uint8_t color,int32_t value,uint8_t dec_len)
{
    char v_string[12] = {0};
    uint8_t i = 0;
    uint8_t valid_flg = 0;

    if(value < 0)
    {
        v_string[i++] = '-';
        value = 0 - value;
    }
    if(value >= 1000000)
    {
        v_string[i++] = value/1000000 + '0';
        value = value%1000000;
        valid_flg = 1;
        if(dec_len == 6)
        {
            v_string[i++] = '.';
        }
    }
    if(value >= 100000)
    {
        v_string[i++] = value/100000 + '0';
        value = value%100000;
        valid_flg = 1;
    }
    else
    {
        if(1 == valid_flg)
        {
            v_string[i++] = '0';
        }
    }
    if(dec_len == 5)
    {
        v_string[i++] = '.';
    }
    if(value >= 10000)
    {
        v_string[i++] = value/10000 + '0';
        value = value%10000;
        valid_flg = 1;
    }
    else
    {
        if(1 == valid_flg)
        {
            v_string[i++] = '0';
        }
    }
    if(dec_len == 4)
    {
        v_string[i++] = '.';
    }
    if(value >= 1000)
    {
        v_string[i++] = value/1000 + '0';
        value = value%1000;
        valid_flg = 1;
    }
    else
    {
        if(1 == valid_flg)
        {
            v_string[i++] = '0';
        }
    }
    if(dec_len == 3)
    {
        v_string[i++] = '.';
    }
    if(value >= 100)
    {
        v_string[i++] = value/100 + '0';
        value = value%100;
        valid_flg = 1;
    }
    else
    {
        if(1 == valid_flg)
        {
            v_string[i++] = '0';
        }
    }
    if(dec_len == 2)
    {
        v_string[i++] = '.';
    }
    if(value >= 10)
    {
        v_string[i++] = value/10 + '0';
        value = value%10;
        valid_flg = 1;
    }
    else
    {
        if(1 == valid_flg)
        {
            v_string[i++] = '0';
        }
    }
    if(dec_len == 1)
    {
        v_string[i++] = '.';
    }
    v_string[i++] = value + '0';
    v_string[i++] = '\0';

    lygl_dis_string_middle(font,x,y,color,v_string);
}
#endif