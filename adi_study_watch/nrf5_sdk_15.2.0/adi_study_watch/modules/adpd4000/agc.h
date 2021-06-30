/**
***************************************************************************
* @file         agc.h
* @author       ADI
* @version      V1.0.0
* @date         10-June-2019
* @brief        Header file contains the static agc functions.
***************************************************************************
* @attention
***************************************************************************
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
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define SLOT_NUM 12
#define SKIP_SAMPLES 10
#define SAMPLE_AVG_NUM 10

/* Rob's Sensor board/ StudyWatch */
#define G_LED_AGC_SLOTS 0x20 /* F */
#define R_LED_AGC_SLOTS 0x40 /* G */
#define IR_LED_AGC_SLOTS 0x80 /* H */
#define B_LED_AGC_SLOTS 0x100 /* I */

typedef enum AGC_LED_SLOT_ENUM_t {
AGC_LED_SLOT_GREEN = 0,
AGC_LED_SLOT_RED = 1,
AGC_LED_SLOT_IR = 2,
AGC_LED_SLOT_BLUE = 3,
}AGC_LED_SLOT_ENUM_t;

typedef struct agc_data_t{
  uint32_t ch1[SAMPLE_AVG_NUM];
  uint32_t ch2[SAMPLE_AVG_NUM];
}agc_data_t;

typedef struct _agc_data_avg_t {
  uint64_t ch1[SLOT_NUM];
  uint64_t ch2[SLOT_NUM];
}agc_data_avg_t;

typedef struct _agc_DC0_cal_t {
  float ch1[SLOT_NUM];
  float ch2[SLOT_NUM];
}agc_DC0_cal_t;

typedef struct _agc_DC_level_cal {
  float ch[SLOT_NUM];
}agc_DC_level_cal_t;

void agc_init();
void agc_data_process();
void agc_deinit(void);
