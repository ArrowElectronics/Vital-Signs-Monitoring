/**
****************************************************************************
* @file     adpd400_task.h
* @author   ADI
* @version  V0.1
* @date     04-Dec-2019
* @brief    Contains definitions and functions for adpd4000 module.
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
#ifndef __ADPD4000_TASK__H
#define __ADPD4000_TASK__H
#include <sensor_adpd_application_interface.h>
#include <task_includes.h>
#include <post_office.h>
#include <adpd400x_drv.h>
#include <adpd4000_dcfg.h>
#include <app_cfg.h>

/*----------------------------- Typedefs -------------------------------------*/

/* Enum defines ADPD4K Slot channels */
typedef enum{
  CH1,
  CH2,
  NUM_CH_PER_SLOT
} ADPD4K_SLOT_CH_t;

/*! \struct packetizer_t ""
    Packetizer structure for adpd data stream */
typedef struct _packetizer_t {
  m2m2_hdr_t                *p_pkt;
  uint16_t                  packet_max_nsamples;
  uint16_t                  sample_sz;
  uint16_t                  decimation_nsamples;
  uint8_t                   *payload_ptr;
  uint16_t                  packet_nsamples;
} packetizer_t;

/*! \struct g_state_t ""
    State structure for adpd data stream */
typedef struct _g_state_t {
  uint16_t  num_subs[SLOT_NUM];
  uint16_t  num_starts;
  uint8_t   decimation_factor;
  uint16_t  data_pkt_seq_num[SLOT_NUM][NUM_CH_PER_SLOT];
  packetizer_t  sl_pktizer1[SLOT_NUM];
  packetizer_t  sl_pktizer2[SLOT_NUM];
} g_state_t;


typedef struct Adpd400xAgcStruct {
  uint32_t  timestamp;
  uint16_t  mts[6];
  uint16_t  setting[10];
} AGCState_t;

/*Enum for AGC stream info - setting[0] field*/
typedef enum {
  ADPD400x_AGCLOG_STATIC_AGC_INVALID = 0,//!<Invalid State
  ADPD400x_AGCLOG_STATIC_AGC_FIRST_CAL = 7,//!< Indicates Initial staticAGC calibration settings applied
  ADPD400x_AGCLOG_STATIC_AGC_RECAL = 8//!< Indicates staticAGC Re-calibration settings applied
} ADPD400x_AGCLOG_state_t;

/*! \struct g_state_agc ""
    State structure for static AGC data stream */
typedef struct _g_state_agc {
  uint16_t  num_subs;
  uint16_t data_pkt_seq_num;
  uint16_t skip_samples;
  uint32_t active_slots;
  ADPD400x_AGCLOG_state_t agc_log_state;
  packetizer_t  agc_pktizer;
} g_state_agc;

typedef struct _led_reg_t {
  uint16_t reg_pow12;
  uint16_t reg_pow34;
}led_reg_t;

typedef struct _slot_led_reg_t {
  led_reg_t reg_val[SLOT_NUM];
}slot_led_reg_t;

//SlotA-L, starts from 1
typedef enum{
  ADPD4K_SLOT_A = 1,
  ADPD4K_SLOT_B,
  ADPD4K_SLOT_C,
  ADPD4K_SLOT_D,
  ADPD4K_SLOT_E,
  ADPD4K_SLOT_F,
  ADPD4K_SLOT_G,
  ADPD4K_SLOT_H,
  ADPD4K_SLOT_I,
  ADPD4K_SLOT_J,
  ADPD4K_SLOT_K,
  ADPD4K_SLOT_L,
} ADPD4K_SLOT_NUM;

void sensor_adpd4000_task_init(void);
void send_message_adpd4000_task(m2m2_hdr_t *p_pkt);
uint16_t GetChipIdAdpd();
#endif // __ADPD4000_TASK__H
