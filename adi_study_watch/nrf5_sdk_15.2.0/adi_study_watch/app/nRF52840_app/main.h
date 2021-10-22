/**
***************************************************************************
* @file    main.h
* @author  ADI Team
* @version V0.0.2
* @date    23-May-2014
* @brief   Header for main.c module
*
******************************************************************************
*/
/*!
 *  \copyright Analog Devices
 * ****************************************************************************
 *
 * License Agreement
 *
 * Copyright (c) 2019 Analog Devices Inc.
 * All rights reserved.
 *
 * This source code is intended for the recipient only under the guidelines of
 * the non-disclosure agreement with Analog Devices Inc.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER  LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING  FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER  DEALINGS IN THE SOFTWARE.
 * ****************************************************************************
 */
#ifndef __MAIN_H
#define __MAIN_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <printf.h>
#include <hw_if_config.h>
#include <adi_osal.h>
#include <task_includes.h>

#include <app_cfg.h>
#include "m2m2_packet.h"

#define FW_VERSION_MAJOR (0x05)
#define FW_VERSION_MINOR (0x03)
#define FW_VERSION_PATCH (0x00)

#define FW_VERSION_SUFFIX ("")

extern void MCU_Init(void);
extern uint32_t gnDebugVariable;
extern void Debug_Handler(void);

#endif /* __MAIN_H */
