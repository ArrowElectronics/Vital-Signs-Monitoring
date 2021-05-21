/**
****************************************************************************
* @file     power_on_detect.h
* @author   ADI
* @version  V0.1
* @date     15-May-2020
* @brief    This header file used to implement power on detection functions
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

#ifndef _POWER_ON_DETECT_H_
#define _POWER_ON_DETECT_H_

#include <stdint.h>

//#define POWER_ON_PRESS_MS (500) /**< The time to hold for a long push (in milliseconds). ~7s */
#define POWER_ON_PRESS_MS (30) /**< The time to hold for a long push (in milliseconds). ~1s */

typedef void (*power_on_func)(void);
/*****************************************************************************
 * Function      : register_power_on_func
 * Description   : register power on handler function
 * Input         : power_on_func hander  
 * Output        : None
 * Return        : 
 * Others        : 
 * Record
 * 1.Date        : 20200624
 *   Modification: Created function

*****************************************************************************/
void register_power_on_func(power_on_func hander);

/*****************************************************************************
 * Function      : power_on_detect_init
 * Description   : initialize the power on detect
 * Input         : void  
 * Output        : None
 * Return        : 
 * Others        : 
 * Record
 * 1.Date        : 20200624
 *   Modification: Created function

*****************************************************************************/
uint32_t power_on_detect_init(void);

/*****************************************************************************
 * Function      : power_on_detect_uninit
 * Description   : uninitialize the power on detect
 * Input         : void  
 * Output        : None
 * Return        : 
 * Others        : 
 * Record
 * 1.Date        : 20200624
 *   Modification: Created function

*****************************************************************************/
void power_on_detect_uninit(void);
#endif

