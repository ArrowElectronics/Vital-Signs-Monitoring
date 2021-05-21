/**
****************************************************************************
* @file     clock_calibration.h
* @author   ADI
* @version  V0.1
* @date     04-Dec-2019
* @brief    Contains definitions and functions for clockcalibration.c
*           source file.
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
#ifndef __CLOCK_CALIBRATION_H__
#define __CLOCK_CALIBRATION_H__

#define NTRIAL_32M    20    /*!< Allowed number of trials for 32M calibration */
#define NTRIAL_32K    6     /*!< Allowed number of trials for 32K calibration */
#ifdef DVT
#define NTRIAL_1M     (20U) /*!< Allowed number of trials for 1M calibration */
#else
#define NTRIAL_1M     6     /*!< Allowed number of trials for 1M calibration */
#endif
#define TRIM_SLOP     420   /*!< 420Hz per code */
#define TRIM_SLOP_1M  1000  /*!< 1000Hz per code */
#define CAL_WATERMARK 64    /*!< FIFO Water mark level for calibration */
#define CAL_TIMEOUT   500   /*!< 64/400Hz = 160ms */
#define ADPD400xCAL_WATERMARK   64    /*!< FIFO Water mark level for calibration */
#define ADPD400x_SAMPLING_RATE  400   /*!< Sampling rate for clock calibration */
#ifdef DVT
#define DELTA_TIME_ADJUSTMENT   0 /*!< Delta Time Adjustment */
#define DELTA_TIME_TRIM         1 /*!< Time Deviation Adjustment */
#else
#define DELTA_TIME_ADJUSTMENT   3 /*!< Delta Time Adjustment */
#define DELTA_TIME_TRIM         2 /*!< Time Deviation Adjustment */
#endif
#define TRIM_1MHZ 0x10            /*!< Trim value for 1MHz calibration */
/*!< Calibration time */
#define CALIBRATION_TIME        (((ADPD400xCAL_WATERMARK * 10)/(ADPD400x_SAMPLING_RATE / 100))- (DELTA_TIME_ADJUSTMENT))
#define ALLOWED_DEVIATION_MAX   (CALIBRATION_TIME + DELTA_TIME_TRIM) /*!< Max deviation allowed */
#define ALLOWED_DEVIATION_MIN   (CALIBRATION_TIME - DELTA_TIME_TRIM) /*!< Min deviation allowed */
int8_t Adpd4000DoClockCalibration(uint8_t cal_id);
uint32_t AdpdGetClockCalResults();

#endif /* __CLOCK_CALIBRATION_H__ */
