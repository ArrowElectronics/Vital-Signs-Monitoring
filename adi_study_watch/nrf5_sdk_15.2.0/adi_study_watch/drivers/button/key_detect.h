/**
****************************************************************************
* @file     key_detect.h
* @author   ADI
* @version  V0.1
* @date     20-May-2020
* @brief    This header file is used to implement key detect functions
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
*   or more patent holders.  This license does not release you from the
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

#ifndef _KEY_DETECT_H_
#define _KEY_DETECT_H_
#include <stdint.h>

#define KEY_NUMBER  (2)

#define APP_KEY_PUSH        (0)                               /**< Indicates that a button is pushed. */
#define APP_KEY_RELEASE    (1)                               /**< Indicates that a button is released. */
#define APP_KEY_LONG_PUSH   (2) 

#define KEY_SELECT_VALUE  (0x01)
#define KEY_NAVIGATION_VALUE  (0x02)

#define KEY_SELECT_SHORT (0x11)//short press
#define KEY_NAVIGATION_SHORT (0x12) //short press

//#define KEY_SELECT_NAVIGATION_VALUE (0x03)
#define KEY_SELECT_LONG_VALUE  (0x21)
#define KEY_NAVIGATION_LONG_VALUE  (0x22)
#define KEY_COMBINATION_LONG_VALUE  (0x23)

#define SHORT_PRESS_TIME_MS  (5)
#define LONG_PRESS_TIMEOUT_MS (300) /**< The time to hold for a long push (in milliseconds). */

//#define APP_KEY_PUSH        (0)                               /**< Indicates that a button is pushed. */
//#define APP_KEY_RELEASE    (1)                               /**< Indicates that a button is released. */
//#define APP_KEY_LONG_PUSH   (2) 

typedef void (*Send_key_func)(uint8_t value);
void Register_key_send_func(Send_key_func hander);
void Unregister_key_send_func(Send_key_func hander);
void Clear_register_key_func(void);
void key_detect_init(void);
#endif

