/**
***************************************************************************
* @file    /Inc/main.h
* @author  ADI Team
* @version V0.0.2
* @date    23-May-2014
* @brief   Header for main.c module
***************************************************************************
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
#ifndef __APPCOMMON_H
#define __APPCOMMON_H

/* Function prototypes */
int8_t GetAdpdOutputRate(uint16_t*, uint16_t*);
int8_t SetAdpdOutputRate(uint16_t, uint16_t);
int8_t GetAdxlOutputRate(uint16_t* sampleRate);
int16_t AdpdSelectSlot(uint8_t SlotAMode,uint8_t SlotBMode);
int16_t AdpdSetOperationMode(uint8_t eOpState);
int16_t AdpdClSetOperationMode(uint8_t eOpState);
int16_t AdpdClSelectSlot(uint8_t eSlotAMode,uint8_t eSlotBMode);
int8_t GetAdpdClOutputRate(uint16_t* sampleRate, uint16_t* decimation, uint16_t slotN);
int8_t SetAdpdClOutputRate(uint16_t sampleRate, uint16_t decimation, uint16_t slotNum);
#endif /* __MAIN_H */
