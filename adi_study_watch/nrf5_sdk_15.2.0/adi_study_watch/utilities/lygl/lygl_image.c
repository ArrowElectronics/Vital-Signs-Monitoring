/**
****************************************************************************
* @file     lygl_image.c
* @author   ADI
* @version  V0.1
* @date     11-Mar-2021
* @brief    This is the source file used to draw image display.
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
void lygl_draw_image(const lv_img_dsc_t * icon,uint8_t x,uint8_t y)//
{
    uint8_t x_size,y_size,x_byte,bits,pexel;
    uint8_t l,h,l_s,l_m;
    uint16_t l_d;
    uint8_t c_mask,mask,color;
    const uint8_t * iData;
    uint8_t x_spot,y_spot;
    if((x>X_AXIS_MAX)||(y>Y_AXIS_MAX))
    {
        return;
    }
    x_spot = x;
    x_size = icon->XSize;
    y_size = icon->YSize;
    x_byte = icon->BytesPerLine;

    bits = icon->BitsPerPixel;
    c_mask = (0xFF << (8-bits));
    iData = icon->pData;
    pexel = 8/bits;
    for(l = 0;l < x_size;l++,x_spot++)
    {
        l_d = l/pexel;
        l_s = l%pexel;
        mask = (c_mask >> (l_s*bits));
        l_m = 8 - bits-l_s*bits;
        for(h = 0,y_spot = y;h < y_size;h++,l_d+=x_byte,y_spot++)
        {
            color = ((iData[l_d]&mask) >> l_m);
            lygl_draw_dot(x_spot,y_spot,color);
        }
    }
}
#endif