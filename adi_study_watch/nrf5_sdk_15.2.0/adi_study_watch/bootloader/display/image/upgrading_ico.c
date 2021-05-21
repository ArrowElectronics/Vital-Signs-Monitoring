/**
****************************************************************************
* @file     file_name.c
* @author   ADI
* @version  V0.1
* @date     04-Dec-2019
* @brief    This is the source file used to 
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
/**
****************************************************************************
Copyright (c) 2020 LVGL LLC 
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
documentation files (the ��Software��), to deal in the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to
whom the Software is furnished to do so, subject to the following conditions:
 
The above copyright notice and this permission notice shall be included in all copies or substantial portions of
the Software. 

THE SOFTWARE IS PROVIDED ��AS IS��, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
******************************************************************************/
#include "lygl_def.h"

static const unsigned char _acupgrading_ico[] = {
  ________, ________, ________, ________, ________, ________,
  ________, ________, ______XX, X_______, ________, ________,
  ________, ________, _____XXX, XX______, ________, ________,
  ________, ________, ____XXXX, XXX_____, ________, ________,
  ________, ________, ___XXXXX, XXXX____, ________, ________,
  ________, ________, __XXXXXX, XXXXX___, ________, ________,
  ________, ________, _XXXXXXX, XXXXXX__, ________, ________,
  ________, ________, XXXXXXXX, XXXXXXX_, ________, ________,
  ________, _______X, XXXXXXXX, XXXXXXXX, ________, ________,
  ________, ______XX, XXXXXXXX, XXXXXXXX, X_______, ________,
  ________, _____XXX, XXXXXXXX, XXXXXXXX, XX______, ________,
  ________, ____XXXX, XXXXXXXX, XXXXXXXX, XXX_____, ________,
  ________, ___XXXXX, XXXXXXXX, XXXXXXXX, XXXX____, ________,
  ________, __XXXXXX, XXXXXXXX, XXXXXXXX, XXXXX___, ________,
  ________, _XXXXXXX, XXXXXXXX, XXXXXXXX, XXXXXX__, ________,
  ________, XXXXXXXX, XXXXXXXX, XXXXXXXX, XXXXXXX_, ________,
  _______X, XXXXXXXX, XXXXXXXX, XXXXXXXX, XXXXXXXX, ________,
  ______XX, XXXXXXXX, XXXXXXXX, XXXXXXXX, XXXXXXXX, X_______,
  _____XXX, XXXXXXXX, XXXXXXXX, XXXXXXXX, XXXXXXXX, XX______,
  ____XXXX, XXXXXXXX, XXXXXXXX, XXXXXXXX, XXXXXXXX, XXX_____,
  ___XXXXX, XXXXXXXX, XXXXXXXX, XXXXXXXX, XXXXXXXX, XXXX____,
  __XXXXXX, XXXXXXXX, XXXXXXXX, XXXXXXXX, XXXXXXXX, XXXXX___,
  _XXXXXXX, XXXXXXXX, XXXXXXXX, XXXXXXXX, XXXXXXXX, XXXXXX__,
  _XXXXXXX, XXXXXXXX, XXXXXXXX, XXXXXXXX, XXXXXXXX, XXXXXX__,
  _XXXXXXX, XXXXXXXX, XXXXXXXX, XXXXXXXX, XXXXXXXX, XXXXXX__,
  __XXXXXX, XXXXXXXX, XXXXXXXX, XXXXXXXX, XXXXXXXX, XXXXX___,
  ________, _______X, XXXXXXXX, XXXXXXXX, X_______, ________,
  ________, _______X, XXXXXXXX, XXXXXXXX, X_______, ________,
  ________, _______X, XXXXXXXX, XXXXXXXX, X_______, ________,
  ________, _______X, XXXXXXXX, XXXXXXXX, X_______, ________,
  ________, _______X, XXXXXXXX, XXXXXXXX, X_______, ________,
  ________, _______X, XXXXXXXX, XXXXXXXX, X_______, ________,
  ________, _______X, XXXXXXXX, XXXXXXXX, X_______, ________,
  ________, _______X, XXXXXXXX, XXXXXXXX, X_______, ________,
  ________, _______X, XXXXXXXX, XXXXXXXX, X_______, ________,
  ________, _______X, XXXXXXXX, XXXXXXXX, X_______, ________,
  ________, ________, XXXXXXXX, XXXXXXXX, ________, ________,
  ________, ________, ________, ________, ________, ________,
  ________, ________, ________, ________, ________, ________,
  ________, ________, ________, ________, ________, ________,
  ________, _______X, XXXXXXXX, XXXXXXXX, X_______, ________,
  ________, _______X, XXXXXXXX, XXXXXXXX, X_______, ________,
  ________, _______X, XXXXXXXX, XXXXXXXX, X_______, ________,
  ________, ________, XXXXXXXX, XXXXXXXX, ________, ________,
  ________, ________, ________, ________, ________, ________,
  ________, ________, ________, ________, ________, ________
};

const lv_img_dsc_t bmupgrading_ico = {
  48, // xSize
  46, // ySize
  6, // BytesPerLine
  1, // BitsPerPixel
  _acupgrading_ico,  // Pointer to picture data (indices)
};

/*************************** End of file ****************************/