/**
****************************************************************************
* @file     lygl_def.h
* @author   ADI
* @version  V0.1
* @date     11-Mar-2021
* @brief    This is the header file used for definitions
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

#ifndef  _LYGL_DEF_H_
#define  _LYGL_DEF_H_

#include "stdint.h"


typedef struct {
  uint16_t XSize;
  uint16_t YSize;
  uint16_t BytesPerLine;
  uint16_t BitsPerPixel;
  const uint8_t * pData;
} lv_img_dsc_t;

void Display_icon(const lv_img_dsc_t * icon,uint8_t x,uint8_t y);

typedef struct
{
    uint32_t w_px         :8;
    uint32_t glyph_index  :24;
} lv_font_glyph_dsc_t;

typedef struct _lv_font_struct
{
    uint32_t unicode_first;
    uint32_t unicode_last;
    const uint8_t * glyph_bitmap;
    const lv_font_glyph_dsc_t * glyph_dsc;
    const uint32_t * unicode_list;
    struct _lv_font_struct * next_page;    /*Pointer to a font extension*/
    uint32_t h_px       :8;
    uint32_t bpp        :4;                /*Bit per pixel: 1, 2 or 4*/
    uint32_t monospace  :8;                /*Fix width (0: normal width)*/
    uint16_t glyph_cnt;                    /*Number of glyphs (letters) in the font*/
} lv_font_t;

/*********************************************************************
*
*       Defines for constants
*/
#if 1
#define	________	0x0
#define	_______X	0x1
#define	______X_	0x2
#define	______XX	0x3
#define	_____X__	0x4
#define	_____X_X	0x5
#define	_____XX_	0x6
#define	_____XXX	0x7
#define	____X___	0x8
#define	____X__X	0x9
#define	____X_X_	0xa
#define	____X_XX	0xb
#define	____XX__	0xc
#define	____XX_X	0xd
#define	____XXX_	0xe
#define	____XXXX	0xf
#define	___X____	0x10
#define	___X___X	0x11
#define	___X__X_	0x12
#define	___X__XX	0x13
#define	___X_X__	0x14
#define	___X_X_X	0x15
#define	___X_XX_	0x16
#define	___X_XXX	0x17
#define	___XX___	0x18
#define	___XX__X	0x19
#define	___XX_X_	0x1a
#define	___XX_XX	0x1b
#define	___XXX__	0x1c
#define	___XXX_X	0x1d
#define	___XXXX_	0x1e
#define	___XXXXX	0x1f
#define	__X_____	0x20
#define	__X____X	0x21
#define	__X___X_	0x22
#define	__X___XX	0x23
#define	__X__X__	0x24
#define	__X__X_X	0x25
#define	__X__XX_	0x26
#define	__X__XXX	0x27
#define	__X_X___	0x28
#define	__X_X__X	0x29
#define	__X_X_X_	0x2a
#define	__X_X_XX	0x2b
#define	__X_XX__	0x2c
#define	__X_XX_X	0x2d
#define	__X_XXX_	0x2e
#define	__X_XXXX	0x2f
#define	__XX____	0x30
#define	__XX___X	0x31
#define	__XX__X_	0x32
#define	__XX__XX	0x33
#define	__XX_X__	0x34
#define	__XX_X_X	0x35
#define	__XX_XX_	0x36
#define	__XX_XXX	0x37
#define	__XXX___	0x38
#define	__XXX__X	0x39
#define	__XXX_X_	0x3a
#define	__XXX_XX	0x3b
#define	__XXXX__	0x3c
#define	__XXXX_X	0x3d
#define	__XXXXX_	0x3e
#define	__XXXXXX	0x3f
#define	_X______	0x40
#define	_X_____X	0x41
#define	_X____X_	0x42
#define	_X____XX	0x43
#define	_X___X__	0x44
#define	_X___X_X	0x45
#define	_X___XX_	0x46
#define	_X___XXX	0x47
#define	_X__X___	0x48
#define	_X__X__X	0x49
#define	_X__X_X_	0x4a
#define	_X__X_XX	0x4b
#define	_X__XX__	0x4c
#define	_X__XX_X	0x4d
#define	_X__XXX_	0x4e
#define	_X__XXXX	0x4f
#define	_X_X____	0x50
#define	_X_X___X	0x51
#define	_X_X__X_	0x52
#define	_X_X__XX	0x53
#define	_X_X_X__	0x54
#define	_X_X_X_X	0x55
#define	_X_X_XX_	0x56
#define	_X_X_XXX	0x57
#define	_X_XX___	0x58
#define	_X_XX__X	0x59
#define	_X_XX_X_	0x5a
#define	_X_XX_XX	0x5b
#define	_X_XXX__	0x5c
#define	_X_XXX_X	0x5d
#define	_X_XXXX_	0x5e
#define	_X_XXXXX	0x5f
#define	_XX_____	0x60
#define	_XX____X	0x61
#define	_XX___X_	0x62
#define	_XX___XX	0x63
#define	_XX__X__	0x64
#define	_XX__X_X	0x65
#define	_XX__XX_	0x66
#define	_XX__XXX	0x67
#define	_XX_X___	0x68
#define	_XX_X__X	0x69
#define	_XX_X_X_	0x6a
#define	_XX_X_XX	0x6b
#define	_XX_XX__	0x6c
#define	_XX_XX_X	0x6d
#define	_XX_XXX_	0x6e
#define	_XX_XXXX	0x6f
#define	_XXX____	0x70
#define	_XXX___X	0x71
#define	_XXX__X_	0x72
#define	_XXX__XX	0x73
#define	_XXX_X__	0x74
#define	_XXX_X_X	0x75
#define	_XXX_XX_	0x76
#define	_XXX_XXX	0x77
#define	_XXXX___	0x78
#define	_XXXX__X	0x79
#define	_XXXX_X_	0x7a
#define	_XXXX_XX	0x7b
#define	_XXXXX__	0x7c
#define	_XXXXX_X	0x7d
#define	_XXXXXX_	0x7e
#define	_XXXXXXX	0x7f
#define	X_______	0x80
#define	X______X	0x81
#define	X_____X_	0x82
#define	X_____XX	0x83
#define	X____X__	0x84
#define	X____X_X	0x85
#define	X____XX_	0x86
#define	X____XXX	0x87
#define	X___X___	0x88
#define	X___X__X	0x89
#define	X___X_X_	0x8a
#define	X___X_XX	0x8b
#define	X___XX__	0x8c
#define	X___XX_X	0x8d
#define	X___XXX_	0x8e
#define	X___XXXX	0x8f
#define	X__X____	0x90
#define	X__X___X	0x91
#define	X__X__X_	0x92
#define	X__X__XX	0x93
#define	X__X_X__	0x94
#define	X__X_X_X	0x95
#define	X__X_XX_	0x96
#define	X__X_XXX	0x97
#define	X__XX___	0x98
#define	X__XX__X	0x99
#define	X__XX_X_	0x9a
#define X__XX_XX	0x9b
#define X__XXX__	0x9c
#define X__XXX_X	0x9d
#define	X__XXXX_	0x9e
#define	X__XXXXX	0x9f
#define	X_X_____	0xa0
#define	X_X____X	0xa1
#define	X_X___X_	0xa2
#define	X_X___XX	0xa3
#define	X_X__X__	0xa4
#define	X_X__X_X	0xa5
#define	X_X__XX_	0xa6
#define	X_X__XXX	0xa7
#define	X_X_X___	0xa8
#define	X_X_X__X	0xa9
#define	X_X_X_X_	0xaa
#define	X_X_X_XX	0xab
#define	X_X_XX__	0xac
#define	X_X_XX_X	0xad
#define	X_X_XXX_	0xae
#define	X_X_XXXX	0xaf
#define	X_XX____	0xb0
#define X_XX___X	0xb1
#define	X_XX__X_	0xb2
#define	X_XX__XX	0xb3
#define	X_XX_X__	0xb4
#define	X_XX_X_X	0xb5
#define	X_XX_XX_	0xb6
#define	X_XX_XXX	0xb7
#define	X_XXX___	0xb8
#define	X_XXX__X	0xb9
#define	X_XXX_X_	0xba
#define	X_XXX_XX	0xbb
#define	X_XXXX__	0xbc
#define	X_XXXX_X	0xbd
#define	X_XXXXX_	0xbe
#define	X_XXXXXX	0xbf
#define	XX______	0xc0
#define	XX_____X	0xc1
#define	XX____X_	0xc2
#define	XX____XX	0xc3
#define	XX___X__	0xc4
#define	XX___X_X	0xc5
#define	XX___XX_	0xc6
#define	XX___XXX	0xc7
#define	XX__X___	0xc8
#define	XX__X__X	0xc9
#define	XX__X_X_	0xca
#define	XX__X_XX	0xcb
#define	XX__XX__	0xcc
#define	XX__XX_X	0xcd
#define	XX__XXX_	0xce
#define XX__XXXX	0xcf
#define	XX_X____	0xd0
#define	XX_X___X	0xd1
#define	XX_X__X_	0xd2
#define	XX_X__XX	0xd3
#define	XX_X_X__	0xd4
#define	XX_X_X_X	0xd5
#define	XX_X_XX_	0xd6
#define	XX_X_XXX	0xd7
#define	XX_XX___	0xd8
#define	XX_XX__X	0xd9
#define	XX_XX_X_	0xda
#define	XX_XX_XX	0xdb
#define	XX_XXX__	0xdc
#define	XX_XXX_X	0xdd
#define	XX_XXXX_	0xde
#define	XX_XXXXX	0xdf
#define	XXX_____	0xe0
#define	XXX____X	0xe1
#define	XXX___X_	0xe2
#define	XXX___XX	0xe3
#define	XXX__X__	0xe4
#define	XXX__X_X	0xe5
#define	XXX__XX_	0xe6
#define	XXX__XXX	0xe7
#define	XXX_X___	0xe8
#define	XXX_X__X	0xe9
#define	XXX_X_X_	0xea
#define	XXX_X_XX	0xeb
#define	XXX_XX__	0xec
#define	XXX_XX_X	0xed
#define	XXX_XXX_	0xee
#define	XXX_XXXX	0xef
#define	XXXX____	0xf0
#define	XXXX___X	0xf1
#define	XXXX__X_	0xf2
#define	XXXX__XX	0xf3
#define	XXXX_X__	0xf4
#define	XXXX_X_X	0xf5
#define	XXXX_XX_	0xf6
#define	XXXX_XXX	0xf7
#define	XXXXX___	0xf8
#define	XXXXX__X	0xf9
#define	XXXXX_X_	0xfa
#define	XXXXX_XX	0xfb
#define	XXXXXX__	0xfc
#define	XXXXXX_X	0xfd
#define	XXXXXXX_	0xfe
#define	XXXXXXXX	0xff

#else
#define	________	0xff
#define	_______X	0xfe
#define	______X_	0xfd
#define	______XX	0xfc
#define	_____X__	0xfb
#define	_____X_X	0xfa
#define	_____XX_	0xf9
#define	_____XXX	0xf8
#define	____X___	0xf7
#define	____X__X	0xf6
#define	____X_X_	0xf5
#define	____X_XX	0xf4
#define	____XX__	0xf3
#define	____XX_X	0xf2
#define	____XXX_	0xf1
#define	____XXXX	0xf0
#define	___X____	0xef
#define	___X___X	0xee
#define	___X__X_	0xed
#define	___X__XX	0xec
#define	___X_X__	0xeb
#define	___X_X_X	0xea
#define	___X_XX_	0xe9
#define	___X_XXX	0xe8
#define	___XX___	0xe7
#define	___XX__X	0xe6
#define	___XX_X_	0xe5
#define	___XX_XX	0xe4
#define	___XXX__	0xe3
#define	___XXX_X	0xe2
#define	___XXXX_	0xe1
#define	___XXXXX	0xe0
#define	__X_____	0xdf
#define	__X____X	0xde
#define	__X___X_	0xdd
#define	__X___XX	0xdc
#define	__X__X__	0xdb
#define	__X__X_X	0xda
#define	__X__XX_	0xd9
#define	__X__XXX	0xd8
#define	__X_X___	0xd7
#define	__X_X__X	0xd6
#define	__X_X_X_	0xd5
#define	__X_X_XX	0xd4
#define	__X_XX__	0xd3
#define	__X_XX_X	0xd2
#define	__X_XXX_	0xd1
#define	__X_XXXX	0xd0
#define	__XX____	0xcf
#define	__XX___X	0xce
#define	__XX__X_	0xcd
#define	__XX__XX	0xcc
#define	__XX_X__	0xcb
#define	__XX_X_X	0xca
#define	__XX_XX_	0xc9
#define	__XX_XXX	0xc8
#define	__XXX___	0xc7
#define	__XXX__X	0xc6
#define	__XXX_X_	0xc5
#define	__XXX_XX	0xc4
#define	__XXXX__	0xc3
#define	__XXXX_X	0xc2
#define	__XXXXX_	0xc1
#define	__XXXXXX	0xc0
#define	_X______	0xbf
#define	_X_____X	0xbe
#define	_X____X_	0xbd
#define	_X____XX	0xbc
#define	_X___X__	0xbb
#define	_X___X_X	0xba
#define	_X___XX_	0xb9
#define	_X___XXX	0xb8
#define	_X__X___	0xb7
#define	_X__X__X	0xb6
#define	_X__X_X_	0xb5
#define	_X__X_XX	0xb4
#define	_X__XX__	0xb3
#define	_X__XX_X	0xb2
#define	_X__XXX_	0xb1
#define	_X__XXXX	0xb0
#define	_X_X____	0xaf
#define	_X_X___X	0xae
#define	_X_X__X_	0xad
#define	_X_X__XX	0xac
#define	_X_X_X__	0xab
#define	_X_X_X_X	0xaa
#define	_X_X_XX_	0xa9
#define	_X_X_XXX	0xa8
#define	_X_XX___	0xa7
#define	_X_XX__X	0xa6
#define	_X_XX_X_	0xa5
#define	_X_XX_XX	0xa4
#define	_X_XXX__	0xa3
#define	_X_XXX_X	0xa2
#define	_X_XXXX_	0xa1
#define	_X_XXXXX	0xa0
#define	_XX_____	0x9f
#define	_XX____X	0x9e
#define	_XX___X_	0x9d
#define	_XX___XX	0x9c
#define	_XX__X__	0x9b
#define	_XX__X_X	0x9a
#define	_XX__XX_	0x99
#define	_XX__XXX	0x98
#define	_XX_X___	0x97
#define	_XX_X__X	0x96
#define	_XX_X_X_	0x95
#define	_XX_X_XX	0x94
#define	_XX_XX__	0x93
#define	_XX_XX_X	0x92
#define	_XX_XXX_	0x91
#define	_XX_XXXX	0x90
#define	_XXX____	0x8f
#define	_XXX___X	0x8e
#define	_XXX__X_	0x8d
#define	_XXX__XX	0x8c
#define	_XXX_X__	0x8b
#define	_XXX_X_X	0x8a
#define	_XXX_XX_	0x89
#define	_XXX_XXX	0x88
#define	_XXXX___	0x87
#define	_XXXX__X	0x86
#define	_XXXX_X_	0x85
#define	_XXXX_XX	0x84
#define	_XXXXX__	0x83
#define	_XXXXX_X	0x82
#define	_XXXXXX_	0x81
#define	_XXXXXXX	0x80
#define	X_______	0x7f
#define	X______X	0x7e
#define	X_____X_	0x7d
#define	X_____XX	0x7c
#define	X____X__	0x7b
#define	X____X_X	0x7a
#define	X____XX_	0x79
#define	X____XXX	0x78
#define	X___X___	0x77
#define	X___X__X	0x76
#define	X___X_X_	0x75
#define	X___X_XX	0x74
#define	X___XX__	0x73
#define	X___XX_X	0x72
#define	X___XXX_	0x71
#define	X___XXXX	0x70
#define	X__X____	0x6f
#define	X__X___X	0x6e
#define	X__X__X_	0x6d
#define	X__X__XX	0x6c
#define	X__X_X__	0x6b
#define	X__X_X_X	0x6a
#define	X__X_XX_	0x69
#define	X__X_XXX	0x68
#define	X__XX___	0x67
#define	X__XX__X	0x66
#define	X__XX_X_	0x65
#define X__XX_XX	0x64
#define X__XXX__	0x63
#define X__XXX_X	0x62
#define	X__XXXX_	0x61
#define	X__XXXXX	0x60
#define	X_X_____	0x5f
#define	X_X____X	0x5e
#define	X_X___X_	0x5d
#define	X_X___XX	0x5c
#define	X_X__X__	0x5b
#define	X_X__X_X	0x5a
#define	X_X__XX_	0x59
#define	X_X__XXX	0x58
#define	X_X_X___	0x57
#define	X_X_X__X	0x56
#define	X_X_X_X_	0x55
#define	X_X_X_XX	0x54
#define	X_X_XX__	0x53
#define	X_X_XX_X	0x52
#define	X_X_XXX_	0x51
#define	X_X_XXXX	0x50
#define	X_XX____	0x4f
#define X_XX___X	0x4e
#define	X_XX__X_	0x4d
#define	X_XX__XX	0x4c
#define	X_XX_X__	0x4b
#define	X_XX_X_X	0x4a
#define	X_XX_XX_	0x49
#define	X_XX_XXX	0x48
#define	X_XXX___	0x47
#define	X_XXX__X	0x46
#define	X_XXX_X_	0x45
#define	X_XXX_XX	0x44
#define	X_XXXX__	0x43
#define	X_XXXX_X	0x42
#define	X_XXXXX_	0x41
#define	X_XXXXXX	0x40
#define	XX______	0x3f
#define	XX_____X	0x3e
#define	XX____X_	0x3d
#define	XX____XX	0x3c
#define	XX___X__	0x3b
#define	XX___X_X	0x3a
#define	XX___XX_	0x39
#define	XX___XXX	0x38
#define	XX__X___	0x37
#define	XX__X__X	0x36
#define	XX__X_X_	0x35
#define	XX__X_XX	0x34
#define	XX__XX__	0x33
#define	XX__XX_X	0x32
#define	XX__XXX_	0x31
#define XX__XXXX	0x30
#define	XX_X____	0x2f
#define	XX_X___X	0x2e
#define	XX_X__X_	0x2d
#define	XX_X__XX	0x2c
#define	XX_X_X__	0x2b
#define	XX_X_X_X	0x2a
#define	XX_X_XX_	0x29
#define	XX_X_XXX	0x28
#define	XX_XX___	0x27
#define	XX_XX__X	0x26
#define	XX_XX_X_	0x25
#define	XX_XX_XX	0x24
#define	XX_XXX__	0x23
#define	XX_XXX_X	0x22
#define	XX_XXXX_	0x21
#define	XX_XXXXX	0x20
#define	XXX_____	0x1f
#define	XXX____X	0x1e
#define	XXX___X_	0x1d
#define	XXX___XX	0x1c
#define	XXX__X__	0x1b
#define	XXX__X_X	0x1a
#define	XXX__XX_	0x19
#define	XXX__XXX	0x18
#define	XXX_X___	0x17
#define	XXX_X__X	0x16
#define	XXX_X_X_	0x15
#define	XXX_X_XX	0x14
#define	XXX_XX__	0x13
#define	XXX_XX_X	0x12
#define	XXX_XXX_	0x11
#define	XXX_XXXX	0x10
#define	XXXX____	0x0f
#define	XXXX___X	0x0e
#define	XXXX__X_	0x0d
#define	XXXX__XX	0x0c
#define	XXXX_X__	0x0b
#define	XXXX_X_X	0x0a
#define	XXXX_XX_	0x09
#define	XXXX_XXX	0x08
#define	XXXXX___	0x07
#define	XXXXX__X	0x06
#define	XXXXX_X_	0x05
#define	XXXXX_XX	0x04
#define	XXXXXX__	0x03
#define	XXXXXX_X	0x02
#define	XXXXXXX_	0x01
#define	XXXXXXXX	0x0
#endif



extern lv_font_t lygl_font_48;
extern lv_font_t lygl_font_64;
extern lv_font_t lygl_font_32;
extern lv_font_t lygl_font_24;
extern lv_font_t lygl_font_16;
extern lv_font_t lygl_font_62;
extern lv_font_t lygl_font_47;
#if defined(__cplusplus)


}
#endif

#endif   /* ifdef GUI_H */

/*************************** End of file ****************************/