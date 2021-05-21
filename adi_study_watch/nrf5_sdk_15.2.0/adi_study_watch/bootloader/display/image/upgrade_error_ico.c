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

static const unsigned char _acupgrade_error_ico[] = {
  ________, ________, ________, ________, ________,
  ________, ________, ________, ________, ________,
  ________, ________, ________, ________, ________,
  ______XX, X_______, ________, ________, XXX_____,
  _____XXX, XXX_____, ________, _______X, XXXX____,
  ____XXXX, XXXX____, ________, ______XX, XXXXX___,
  ___XXXXX, XXXXX___, ________, _____XXX, XXXXXX__,
  ___XXXXX, XXXXX___, ________, ____XXXX, XXXXXX__,
  ___XXXXX, XXXXXX__, ________, ___XXXXX, XXXXXX__,
  ____XXXX, XXXXXXX_, ________, __XXXXXX, XXXXX___,
  _____XXX, XXXXXXXX, ________, _XXXXXXX, XXXXX___,
  ______XX, XXXXXXXX, X_______, XXXXXXXX, XXXX____,
  _______X, XXXXXXXX, XX_____X, XXXXXXXX, XXX_____,
  ________, XXXXXXXX, XXX___XX, XXXXXXXX, XX______,
  ________, XXXXXXXX, XXXXXXXX, XXXXXXXX, X_______,
  ________, _XXXXXXX, XXXXXXXX, XXXXXXXX, ________,
  ________, __XXXXXX, XXXXXXXX, XXXXXXX_, ________,
  ________, ___XXXXX, XXXXXXXX, XXXXXX__, ________,
  ________, ____XXXX, XXXXXXXX, XXXXX___, ________,
  ________, _____XXX, XXXXXXXX, XXXX____, ________,
  ________, ______XX, XXXXXXXX, XXX_____, ________,
  ________, _____XXX, XXXXXXXX, XXXX____, ________,
  ________, ____XXXX, XXXXXXXX, XXXXX___, ________,
  ________, ___XXXXX, XXXXXXXX, XXXXXX__, ________,
  ________, __XXXXXX, XXXXXXXX, XXXXXXX_, ________,
  ________, _XXXXXXX, XXXXXXXX, XXXXXXXX, ________,
  ________, XXXXXXXX, XXXXXXXX, XXXXXXXX, X_______,
  _______X, XXXXXXXX, XXX___XX, XXXXXXXX, XX______,
  ______XX, XXXXXXXX, XX_____X, XXXXXXXX, XXX_____,
  _____XXX, XXXXXXXX, X_______, XXXXXXXX, XXXX____,
  ____XXXX, XXXXXXXX, ________, _XXXXXXX, XXXXX___,
  ___XXXXX, XXXXXXX_, ________, __XXXXXX, XXXXXX__,
  ___XXXXX, XXXXXX__, ________, ___XXXXX, XXXXXX__,
  ___XXXXX, XXXXX___, ________, ____XXXX, XXXXXXX_,
  ___XXXXX, XXXX____, ________, _____XXX, XXXXXXX_,
  ___XXXXX, XXX_____, ________, ______XX, XXXXXX__,
  ____XXXX, XX______, ________, _______X, XXXXXX__,
  _____XXX, X_______, ________, ________, XXXX____,
  ________, ________, ________, ________, ________,
  ________, ________, ________, ________, ________
};

const lv_img_dsc_t bmupgrade_error_ico = {
  40, // xSize
  40, // ySize
  5, // BytesPerLine
  1, // BitsPerPixel
  _acupgrade_error_ico,  // Pointer to picture data (indices)
};

/*************************** End of file ****************************/
