/**
****************************************************************************
* @file     setting_firmware_version_ico.c
* @author   ADI
* @version  V0.1
* @date     04-Dec-2019
* @brief    This is the source file used to display firmware version
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

static const unsigned char _ac_setting_firmware_version_ico[] = {
  ________, ________, ______XX, XXXX____, ________, ________, ________,
  ________, ________, __XXXXXX, XXXXXXXX, ________, ________, ________,
  ________, ________, XXXXXXXX, XXXXXXXX, XX______, ________, ________,
  ________, ______XX, XXXXXXXX, XXXXXXXX, XXXX____, ________, ________,
  ________, ____XXXX, XXXXXXXX, XXXXXXXX, XXXXXX__, ________, ________,
  ________, __XXXXXX, XXXXXXXX, XXXXXXXX, XXXXXXXX, ________, ________,
  ________, _XXXXXXX, XXX_____, _______X, XXXXXXXX, X_______, ________,
  ________, XXXXXXXX, ________, ________, __XXXXXX, XX______, ________,
  _______X, XXXXXX__, ________, ________, ____XXXX, XXX_____, ________,
  ______XX, XXXXX___, ________, ________, _____XXX, XXXX____, ________,
  _____XXX, XXXX____, ________, _XXX____, ______XX, XXXXX___, ________,
  ____XXXX, XXX_____, ________, XXXXX___, _______X, XXXXXX__, ________,
  ____XXXX, XX______, _______X, XXXXXX__, ________, XXXXXX__, ________,
  ___XXXXX, X_______, _______X, XXXXXX__, ________, _XXXXXX_, ________,
  ___XXXXX, X_______, _______X, XXXXXX__, ________, _XXXXXX_, ________,
  __XXXXXX, ________, _______X, XXXXXX__, ________, __XXXXXX, ________,
  __XXXXX_, ________, ________, XXXXX___, ________, ___XXXXX, ________,
  __XXXXX_, ________, ________, _XXX____, ________, ___XXXXX, ________,
  _XXXXXX_, ________, ________, ________, ________, ___XXXXX, X_______,
  _XXXXX__, ________, ________, ________, ________, ____XXXX, X_______,
  _XXXXX__, ________, ________, ________, ________, ____XXXX, X_______,
  _XXXXX__, ________, _______X, XXXX____, ________, ____XXXX, X_______,
  XXXXX___, ________, _____XXX, XXXX____, ________, _____XXX, XX______,
  XXXXX___, ________, ____XXXX, XXXX____, ________, _____XXX, XX______,
  XXXXX___, ________, __XXX_XX, XXXX____, ________, _____XXX, XX______,
  XXXXX___, ________, _XXX__XX, XXXX____, ________, _____XXX, XX______,
  XXXXX___, ________, ______XX, XXX_____, ________, _____XXX, XX______,
  XXXXX___, ________, _____XXX, XXX_____, ________, _____XXX, XX______,
  _XXXXX__, ________, _____XXX, XX______, ________, ____XXXX, X_______,
  _XXXXX__, ________, _____XXX, XX______, ________, ____XXXX, X_______,
  _XXXXX__, ________, _____XXX, XX______, ________, ____XXXX, X_______,
  _XXXXXX_, ________, ____XXXX, XX______, ________, ___XXXXX, X_______,
  __XXXXX_, ________, ____XXXX, XX___X__, ________, ___XXXXX, ________,
  __XXXXXX, ________, ____XXXX, XXX_XX__, ________, __XXXXXX, ________,
  __XXXXXX, ________, ____XXXX, XXXXX___, ________, __XXXXXX, ________,
  ___XXXXX, X_______, ____XXXX, XXXX____, ________, _XXXXXX_, ________,
  ___XXXXX, XX______, ____XXXX, XX______, ________, XXXXXXX_, ________,
  ____XXXX, XX______, ____XXXX, ________, ________, XXXXXX__, ________,
  ____XXXX, XXX_____, ________, ________, _______X, XXXXXX__, ________,
  _____XXX, XXXX____, ________, ________, ______XX, XXXXX___, ________,
  ______XX, XXXXX___, ________, ________, _____XXX, XXXX____, ________,
  _______X, XXXXXXX_, ________, ________, ___XXXXX, XXX_____, ________,
  ________, XXXXXXXX, ________, ________, __XXXXXX, XX______, ________,
  ________, _XXXXXXX, XXX_____, _______X, XXXXXXXX, X_______, ________,
  ________, __XXXXXX, XXXXXXXX, __XXXXXX, XXXXXXXX, ________, ________,
  ________, ____XXXX, XXXXXXXX, XXXXXXXX, XXXXXX__, ________, ________,
  ________, _____XXX, XXXXXXXX, XXXXXXXX, XXXXX___, ________, ________,
  ________, _______X, XXXXXXXX, XXXXXXXX, XXX_____, ________, ________,
  ________, ________, __XXXXXX, XXXXXXXX, ________, ________, ________,
  ________, ________, ______XX, XXXX____, ________, ________, ________
};

const lv_img_dsc_t bm_setting_firmware_version_ico = {
  50, // xSize
  50, // ySize
  7, // BytesPerLine
  1, // BitsPerPixel
  _ac_setting_firmware_version_ico,  // Pointer to picture data (indices)
};
#endif
/*************************** End of file ****************************/
