/**
****************************************************************************
* @file     setting_factory_reset_ico.c
* @author   ADI
* @version  V0.1
* @date     04-Dec-2019
* @brief    This is the source file used to display factory rest information
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
documentation files (the ¡°Software¡±), to deal in the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to
whom the Software is furnished to do so, subject to the following conditions:
 
The above copyright notice and this permission notice shall be included in all copies or substantial portions of
the Software. 

THE SOFTWARE IS PROVIDED ¡°AS IS¡±, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
******************************************************************************/
#ifdef ENABLE_WATCH_DISPLAY

#include "lygl_def.h"

static const unsigned char _ac_setting_factory_reset_ico[] = {
  __XXXXXX, XXXXXXXX, XXX_____, ________, ________, ________, ________,
  __XXXXXX, XXXXXXXX, XXX_____, ________, ________, ________, ________,
  ___XXXXX, XXXXXXXX, XXX_____, ________, ________, ________, ________,
  ____XXXX, XXXXXXXX, XXX_____, ________, __XX____, ________, ________,
  _____XXX, XXXXXXXX, XXX_____, ________, __XXXX__, ________, ________,
  ______XX, XXXXXXXX, XXX_____, ________, __XXXXXX, ________, ________,
  _______X, XXXXXXXX, XXX_____, ________, __XXXXXX, X_______, ________,
  ________, XXXXXXXX, XXX_____, ________, __XXXXXX, XX______, ________,
  ________, XXXXXXXX, XXX_____, ________, ____XXXX, XXX_____, ________,
  _______X, XXXXXXXX, XXX_____, ________, _____XXX, XXXX____, ________,
  ______XX, XXXXXXXX, XXX_____, ________, ______XX, XXXXX___, ________,
  _____XXX, XXXX_XXX, XXX_____, ________, _______X, XXXXXX__, ________,
  _____XXX, XXX___XX, XXX_____, ________, ________, XXXXXX__, ________,
  ____XXXX, XX_____X, XXX_____, ________, ________, _XXXXXX_, ________,
  ____XXXX, XX______, XXX_____, ________, ________, _XXXXXX_, ________,
  ___XXXXX, X_______, _XX_____, ________, ________, __XXXXXX, ________,
  ___XXXXX, ________, ________, ________, ________, ___XXXXX, ________,
  ___XXXXX, ________, ________, ________, ________, ___XXXXX, ________,
  __XXXXXX, ________, ________, ________, ________, ___XXXXX, X_______,
  __XXXXX_, ________, ________, ________, ________, ____XXXX, X_______,
  __XXXXX_, ________, ________, ________, ________, ____XXXX, X_______,
  __XXXXX_, ________, ________, ________, ________, ____XXXX, X_______,
  _XXXXX__, ________, ________, ________, ________, _____XXX, XX______,
  _XXXXX__, ________, ________, ________, ________, _____XXX, XX______,
  _XXXXX__, ________, ________, ________, ________, _____XXX, XX______,
  _XXXXX__, ________, ________, ________, ________, _____XXX, XX______,
  _XXXXX__, ________, ________, ________, ________, _____XXX, XX______,
  _XXXXX__, ________, ________, ________, ________, _____XXX, XX______,
  _XXXXXX_, ________, ________, ________, ________, ____XXXX, XX______,
  __XXXXX_, ________, ________, ________, ________, ____XXXX, X_______,
  __XXXXX_, ________, ________, ________, ________, ____XXXX, X_______,
  __XXXXXX, ________, ________, ________, ________, ___XXXXX, X_______,
  __XXXXXX, ________, ________, ________, ________, ___XXXXX, X_______,
  ___XXXXX, X_______, ________, ________, ________, __XXXXXX, ________,
  ___XXXXX, X_______, ________, ________, ________, __XXXXXX, ________,
  ____XXXX, XX______, ________, ________, ________, _XXXXXX_, ________,
  ____XXXX, XX______, ________, ________, ________, XXXXXXX_, ________,
  _____XXX, XXX_____, ________, ________, ________, XXXXXX__, ________,
  _____XXX, XXXX____, ________, ________, _______X, XXXXXX__, ________,
  ______XX, XXXXX___, ________, ________, ______XX, XXXXX___, ________,
  _______X, XXXXXX__, ________, ________, _____XXX, XXXX____, ________,
  ________, XXXXXXXX, ________, ________, ___XXXXX, XXX_____, ________,
  ________, _XXXXXXX, X_______, ________, __XXXXXX, XX______, ________,
  ________, __XXXXXX, XXXX____, _______X, XXXXXXXX, X_______, ________,
  ________, ___XXXXX, XXXXXXXX, ___XXXXX, XXXXXXXX, ________, ________,
  ________, _____XXX, XXXXXXXX, XXXXXXXX, XXXXXX__, ________, ________,
  ________, ______XX, XXXXXXXX, XXXXXXXX, XXXXX___, ________, ________,
  ________, ________, XXXXXXXX, XXXXXXXX, XXX_____, ________, ________,
  ________, ________, ___XXXXX, XXXXXXXX, ________, ________, ________,
  ________, ________, _______X, XXXX____, ________, ________, ________
};

const lv_img_dsc_t bm_setting_factory_reset_ico = {
  50, // xSize
  50, // ySize
  7, // BytesPerLine
  1, // BitsPerPixel
  _ac_setting_factory_reset_ico,  // Pointer to picture data (indices)
};
#endif
/*************************** End of file ****************************/
