/**
***************************************************************************
* @file         agc.c
* @author       ADI
* @version      V1.0.0
* @date          10-June-2019
* @brief        Source file contains the static agc fucntions.
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

#include <string.h>
#include "adpd400x_drv.h"
#include "adpd400x_reg.h"
#include "adpd400x_lib_common.h"
#include "agc.h"
#include "sensor_adpd_application_interface.h"
#include <math.h>
#define MODULE ("Agc.c")

/*================== LOG LEVELS=============================================*/
#include "nrf_log.h"

/*--------------------------- Defines ----------------------------------------*/
#define LED LED_SLOTS
#define INIT_AGCLED_PHASE2 0x08
#define INIT_AGCLED_PHASE1 0x0D
/*--------------------------- Typedef ----------------------------------------*/

/*--------------------------- Public Variable --------------------------------*/
agc_data_t agc_data[LED_SLOTS];
bool stop = false;
static uint16_t hrmInputRate;
#ifdef STATIC_AGC
extern uint8_t agc_samples;
extern uint8_t skip_first_10_samples;
#endif
extern uint8_t gb_static_agc_green_en;
extern uint8_t gb_ppg_static_agc_green_en;
extern uint8_t gb_static_agc_red_en;
extern uint8_t gb_static_agc_ir_en;
extern uint8_t gb_static_agc_blue_en;
extern uint32_t gn_led_slot_g;
extern uint32_t gn_led_slot_r;
extern uint32_t gn_led_slot_ir;
extern uint32_t gn_led_slot_b;
#ifdef ENABLE_PPG_APP
extern uint32_t  Ppg_Slot;
#endif

/*--------------------------- Public Function Prototype --------------------- */

/*--------------------------- Private variables ------------------------------*/
//Slot F to I pulse registers
typedef struct Register {
  uint16_t x1A7;
  uint16_t x1C7;
  uint16_t x1E7;
  uint16_t x207;
}Reg_t;

static uint16_t g_reg_base;
//Initial TIA gain 12.5k
#define INIT_TIA_GAIN_PHASE2 (0x03E4)
#define INIT_TIA_GAIN_PHASE1 (0xE3E4)
static uint16_t Init_gain = INIT_TIA_GAIN_PHASE2;
//Initial LED current 8mA
//uint32_t new_current[LED] = {INIT_AGCLED_PHASE2};
//LED full scale calculation with no of pulses
uint32_t FullScale[LED]={0};
static Reg_t PulseReg,PulseValue;
//DC level calculation for TIA gain 12.5_25_50_100_200k
agc_DC_level_cal_t DC0_init_check, dc_normal_diff, dc_div_diff, DC0, DC125, DC25, DC50, DC100, DC200, TIA_ch1, TIA_ch2;
//DC0 calculation with initial TIA gain and LED current
agc_DC0_cal_t DC0_cal, DC0_cal_div;
//AGC final LED current and TIA gains
uint16_t DC0_LEDcurrent[LED]={0},TIA_ch1_i[LED]={0},TIA_ch2_i[LED]={0};
agc_data_avg_t avg_data, gain_factor;
//Backup initial TIA Gain
uint16_t gAFE_Trim[LED] = {0};
/*--------------------------- Private Function Prototype ---------------------*/
void set_led_current(uint16_t *current);
void do_average (agc_data_avg_t *avg_data);
void calculate_DC0(agc_DC0_cal_t *DC0_cal);
void set_TIA_gain(uint16_t *slot_ch1_i, uint16_t *slot_ch2_i);

#if 1
//LUT to convert from phase2 index to phase1 index ; index is ADPD4100
/* AGC algorithm calculates LED current reg setting for ADPD4100
   LUT mapping is required for ADPD4000 register setting
   To be applied, if ADPD4000 is present in Watch
   LUT contains phase1(ADPD4000) reg val corresponding to phase2(ADPD4100) reg val as index */
const uint8_t gan_lut_phase1_reg[128] =
{
0 ,
2 ,
4 ,
5 ,
7 ,
9 ,
11 ,
12 ,
14 ,
16 ,
17 ,
18 ,
19 ,
20 ,
21 ,
22 ,
22 ,
24 ,
24 ,
26 ,
26 ,
27 ,
28 ,
29 ,
30 ,
31 ,
32 ,
33 ,
34 ,
35 ,
36 ,
37 ,
38 ,
39 ,
40 ,
41 ,
42 ,
43 ,
43 ,
44 ,
45 ,
46 ,
47 ,
48 ,
49 ,
50 ,
51 ,
52 ,
53 ,
54 ,
55 ,
56 ,
56 ,
58 ,
58 ,
59 ,
60 ,
61 ,
62 ,
63 ,
64 ,
65 ,
66 ,
67 ,
69 ,
70 ,
71 ,
72 ,
73 ,
74 ,
74 ,
75 ,
76 ,
77 ,
78 ,
79 ,
80 ,
81 ,
82 ,
83 ,
83 ,
85 ,
85 ,
87 ,
87 ,
88 ,
89 ,
90 ,
91 ,
92 ,
93 ,
94 ,
95 ,
96 ,
96 ,
97 ,
99 ,
100,
101,
102,
103,
104,
105,
105,
106,
107,
108,
109,
110,
111,
111,
112,
113,
114,
115,
115,
116,
117,
118,
119,
120,
121,
122,
123,
124,
125,
126,
127,
};
#endif

/*!****************************************************************************
*
*  \brief       Static AGC processing
*
*  \return       None
*****************************************************************************/
void agc_data_process() {

  Adpd400xDrvSetOperationMode(ADPD400xDrv_MODE_IDLE);
  memset(&avg_data, 0x00, sizeof(avg_data));
  // Find average of each slot data
  do_average(&avg_data);

  // Find TIA gain and LED current of each slot
  calculate_DC0(&DC0_cal);

  //Apply AGC final LED current settings
  set_led_current(&DC0_LEDcurrent[0]);

  //Apply AGC final TIA gain settings
  set_TIA_gain(&TIA_ch1_i[0], &TIA_ch2_i[0]);
  stop = true;
  Adpd400xDrvSetOperationMode(ADPD400xDrv_MODE_SAMPLE);

}

/*!****************************************************************************
*
*  \brief       Setting LED current
*
*  \param[in]   current: pointer to set the final AGC LED current settings
*
*  \return       None
*****************************************************************************/
void set_led_current(uint16_t *current) {
  static uint16_t nRegValue = 0x0000;
  uint16_t chip_id;
  uint8_t i;
  uint8_t lut_index[LED]={0};
  static uint8_t new_reg_val[LED];

  //Read ADPD chip ID
  Adpd400xDrvRegRead(ADPD400x_REG_CHIP_ID, &chip_id);

  //Check if its phase1 chip, if so do corresponding mapping from LUT
  if( chip_id == 0xc0 )
  {
    for( i=0;i<LED;i++)
    {
      lut_index[i] = current[i];
      new_reg_val[i] =  gan_lut_phase1_reg[ lut_index[i] ];
      NRF_LOG_INFO("LED Slot:%d lut_index:%d current[0]:%d lut val:%d", i, lut_index[i], current[i], gan_lut_phase1_reg[ lut_index[i] ]);
    }
  }
  else
  {
    for( i=0;i<LED;i++)
    {
      new_reg_val[i] = current[i];
    }
  }

if(gn_led_slot_g!=0 && gb_static_agc_green_en==1)
  {
  // Slot F LED 1A Green
  g_reg_base = log2(gn_led_slot_g) * 0x20;
  Adpd400xDrvRegRead(ADPD400x_REG_LED_POW12_A + g_reg_base, &nRegValue);
  nRegValue = nRegValue & (~BITM_LED_POW12_X_LED_CURRENT1_X);
  nRegValue |= new_reg_val[AGC_LED_SLOT_GREEN] << BITP_LED_POW12_X_LED_CURRENT1_X;
  Adpd400xDrvRegWrite(ADPD400x_REG_LED_POW12_A + g_reg_base, nRegValue);
 }

  // Slot G LED 3A Red
if(gn_led_slot_r!=0 && gb_static_agc_red_en==1)
  {
  g_reg_base = log2(gn_led_slot_r) * 0x20;
  Adpd400xDrvRegRead(ADPD400x_REG_LED_POW34_A + g_reg_base, &nRegValue);
  nRegValue = nRegValue & (~BITM_LED_POW34_X_LED_CURRENT3_X);
  nRegValue |= new_reg_val[AGC_LED_SLOT_RED] << BITP_LED_POW34_X_LED_CURRENT3_X;
  Adpd400xDrvRegWrite(ADPD400x_REG_LED_POW34_A + g_reg_base, nRegValue);
  }

  // Slot H LED 2A IR
if(gn_led_slot_ir!=0 && gb_static_agc_ir_en==1)
  {
  g_reg_base = log2(gn_led_slot_ir) * 0x20;
  Adpd400xDrvRegRead(ADPD400x_REG_LED_POW12_A + g_reg_base, &nRegValue);
  nRegValue = nRegValue & (~BITM_LED_POW12_X_LED_CURRENT2_X);
  nRegValue |= new_reg_val[AGC_LED_SLOT_IR] << BITP_LED_POW12_X_LED_CURRENT2_X;
  Adpd400xDrvRegWrite(ADPD400x_REG_LED_POW12_A + g_reg_base, nRegValue);
  }

  // Slot I LED 4A Blue
if(gn_led_slot_b!=0 && gb_static_agc_blue_en==1)
  {
  g_reg_base = log2(gn_led_slot_b) * 0x20;
  Adpd400xDrvRegRead(ADPD400x_REG_LED_POW34_A + g_reg_base, &nRegValue);
  nRegValue = nRegValue & (~BITM_LED_POW34_X_LED_CURRENT4_X);
  nRegValue |= new_reg_val[AGC_LED_SLOT_BLUE] << BITP_LED_POW34_X_LED_CURRENT4_X;
  Adpd400xDrvRegWrite(ADPD400x_REG_LED_POW34_A + g_reg_base, nRegValue);
  }

  g_reg_base = log2(gn_led_slot_g) * 0x20;
  Adpd400xDrvRegRead(ADPD400x_REG_LED_POW12_A + g_reg_base, &nRegValue);
  NRF_LOG_INFO("Green Reg:%d", nRegValue);

  g_reg_base = log2(gn_led_slot_r) * 0x20;
  Adpd400xDrvRegRead(ADPD400x_REG_LED_POW34_A + g_reg_base, &nRegValue);
  NRF_LOG_INFO("Red Reg:%d", nRegValue);
}

/*!****************************************************************************
*
*  \brief       Setting TIA gain
*
*  \param[in]   slot_ch1_i: pointer to set the final TIA gain settings for ch1
*
*  \param[in]   slot_ch2_i: pointer to set the final TIA gain settings for ch2
*
*  \return       None
*****************************************************************************/
void set_TIA_gain(uint16_t *slot_ch1_i, uint16_t *slot_ch2_i){
  uint16_t nTIAgain = 0x0000;
  uint16_t nTsCtrl = 0x0000;
  uint8_t nCh2Enable = 0x00;
if(gn_led_slot_g!=0 && gb_static_agc_green_en==1)
  {
  g_reg_base = log2(gn_led_slot_g) * 0x20;

  /* check if ch2 is enabled */
  Adpd400xDrvRegRead(ADPD400x_REG_TS_CTRL_A + g_reg_base, &nTsCtrl);
  nCh2Enable = (nTsCtrl & BITM_TS_CTRL_A_CH2_EN_A) >> BITP_TS_CTRL_A_CH2_EN_A;

  Adpd400xDrvRegRead(ADPD400x_REG_AFE_TRIM_A + g_reg_base, &nTIAgain);
  nTIAgain = nTIAgain & (~BITM_AFE_TRIM_X_TIA_GAIN_CH1_X);
  nTIAgain |= (slot_ch1_i[AGC_LED_SLOT_GREEN]) << BITP_AFE_TRIM_X_TIA_GAIN_CH1_X;
  if(nCh2Enable)
  {
    nTIAgain = nTIAgain & (~BITM_AFE_TRIM_X_TIA_GAIN_CH2_X);
    nTIAgain |= (slot_ch2_i[AGC_LED_SLOT_GREEN]) << BITP_AFE_TRIM_X_TIA_GAIN_CH2_X;
  }
  else
  {
    /* if ch2 not enabled - restore default value */
    nTIAgain = nTIAgain & (~BITM_AFE_TRIM_X_TIA_GAIN_CH2_X);
    nTIAgain |= gAFE_Trim[AGC_LED_SLOT_GREEN] & BITM_AFE_TRIM_X_TIA_GAIN_CH2_X;
  }
  Adpd400xDrvRegWrite(ADPD400x_REG_AFE_TRIM_A + g_reg_base, nTIAgain);
  }

  // Slot G TIA gain
if(gn_led_slot_r!=0 && gb_static_agc_red_en==1)
  {
  g_reg_base = log2(gn_led_slot_r) * 0x20;
  /* check if ch2 is enabled */
  Adpd400xDrvRegRead(ADPD400x_REG_TS_CTRL_A + g_reg_base, &nTsCtrl);
  nCh2Enable = (nTsCtrl & BITM_TS_CTRL_A_CH2_EN_A) >> BITP_TS_CTRL_A_CH2_EN_A;

  Adpd400xDrvRegRead(ADPD400x_REG_AFE_TRIM_A + g_reg_base, &nTIAgain);
  nTIAgain = nTIAgain & (~BITM_AFE_TRIM_X_TIA_GAIN_CH1_X);
  nTIAgain |= (slot_ch1_i[AGC_LED_SLOT_RED]) << BITP_AFE_TRIM_X_TIA_GAIN_CH1_X;
  if(nCh2Enable)
  {
    nTIAgain = nTIAgain & (~BITM_AFE_TRIM_X_TIA_GAIN_CH2_X);
    nTIAgain |= (slot_ch2_i[AGC_LED_SLOT_RED]) << BITP_AFE_TRIM_X_TIA_GAIN_CH2_X;
  }
  else
  {
    /* if ch2 not enabled - restore default value */
    nTIAgain = nTIAgain & (~BITM_AFE_TRIM_X_TIA_GAIN_CH2_X);
    nTIAgain |= gAFE_Trim[AGC_LED_SLOT_RED] & BITM_AFE_TRIM_X_TIA_GAIN_CH2_X;
  }
  Adpd400xDrvRegWrite(ADPD400x_REG_AFE_TRIM_A + g_reg_base, nTIAgain);
  }

  // Slot H TIA gain
if(gn_led_slot_ir!=0 && gb_static_agc_ir_en==1)
  {
  g_reg_base = log2(gn_led_slot_ir) * 0x20;
  /* check if ch2 is enabled */
  Adpd400xDrvRegRead(ADPD400x_REG_TS_CTRL_A + g_reg_base, &nTsCtrl);
  nCh2Enable = (nTsCtrl & BITM_TS_CTRL_A_CH2_EN_A) >> BITP_TS_CTRL_A_CH2_EN_A;

  Adpd400xDrvRegRead(ADPD400x_REG_AFE_TRIM_A + g_reg_base, &nTIAgain);
  nTIAgain = nTIAgain & (~BITM_AFE_TRIM_X_TIA_GAIN_CH1_X);
  nTIAgain |= (slot_ch1_i[AGC_LED_SLOT_IR]) << BITP_AFE_TRIM_X_TIA_GAIN_CH1_X;
  if(nCh2Enable)
  {
    nTIAgain = nTIAgain & (~BITM_AFE_TRIM_X_TIA_GAIN_CH2_X);
    nTIAgain |= (slot_ch2_i[AGC_LED_SLOT_IR]) << BITP_AFE_TRIM_X_TIA_GAIN_CH2_X;
  }
  else
  {
    /* if ch2 not enabled - restore default value */
    nTIAgain = nTIAgain & (~BITM_AFE_TRIM_X_TIA_GAIN_CH2_X);
    nTIAgain |= gAFE_Trim[AGC_LED_SLOT_IR] & BITM_AFE_TRIM_X_TIA_GAIN_CH2_X;
  }
  Adpd400xDrvRegWrite(ADPD400x_REG_AFE_TRIM_A + g_reg_base, nTIAgain);
  }

  // Slot I TIA gain
if(gn_led_slot_b!=0 && gb_static_agc_blue_en==1)
  {
  g_reg_base = log2(gn_led_slot_b) * 0x20;
  /* check if ch2 is enabled */
  Adpd400xDrvRegRead(ADPD400x_REG_TS_CTRL_A + g_reg_base, &nTsCtrl);
  nCh2Enable = (nTsCtrl & BITM_TS_CTRL_A_CH2_EN_A) >> BITP_TS_CTRL_A_CH2_EN_A;

  Adpd400xDrvRegRead(ADPD400x_REG_AFE_TRIM_A + g_reg_base, &nTIAgain);
  nTIAgain = nTIAgain & (~BITM_AFE_TRIM_X_TIA_GAIN_CH1_X);
  nTIAgain |= (slot_ch1_i[AGC_LED_SLOT_BLUE]) << BITP_AFE_TRIM_X_TIA_GAIN_CH1_X;
  if(nCh2Enable)
  {
    nTIAgain = nTIAgain & (~BITM_AFE_TRIM_X_TIA_GAIN_CH2_X);
    nTIAgain |= (slot_ch2_i[AGC_LED_SLOT_BLUE]) << BITP_AFE_TRIM_X_TIA_GAIN_CH2_X;
  }
  else
  {
    /* if ch2 not enabled - restore default value */
    nTIAgain = nTIAgain & (~BITM_AFE_TRIM_X_TIA_GAIN_CH2_X);
    nTIAgain |= gAFE_Trim[AGC_LED_SLOT_BLUE] & BITM_AFE_TRIM_X_TIA_GAIN_CH2_X;
  }
  Adpd400xDrvRegWrite(ADPD400x_REG_AFE_TRIM_A + g_reg_base, nTIAgain);
  }
}

/*!****************************************************************************
*
*  \brief       Static AGC initialization
*
*  \return       None
*****************************************************************************/
void agc_init() {
  uint16_t nRegValue = 0x0000;
  uint16_t nLedCurrent = INIT_AGCLED_PHASE2;
  uint16_t adpdFreq, chip_id;

  //Read ADPD chip ID
  Adpd400xDrvRegRead(ADPD400x_REG_CHIP_ID, &chip_id);

  //Check if its phase1 chip and set corresponding INIT_AGCLED
  if( chip_id == 0xc0 )
  {
    nLedCurrent = INIT_AGCLED_PHASE1;
    Init_gain = INIT_TIA_GAIN_PHASE1;
  }

  memset(&agc_data[0], 0x00, sizeof(agc_data));
  memset(&avg_data, 0x00, sizeof(avg_data));
  memset(&DC0_init_check, 0x00, sizeof(DC0_init_check));
  memset(&dc_normal_diff, 0x00, sizeof(dc_normal_diff));
  memset(&dc_div_diff , 0x00, sizeof(dc_div_diff));
  memset(&DC0 , 0x00, sizeof(DC0));
  memset(&DC125 , 0x00, sizeof(DC125));
  memset(&DC25 , 0x00, sizeof(DC25));
  memset(&DC50 , 0x00, sizeof(DC50));
  memset(&DC100 , 0x00, sizeof(DC100));
  memset(&DC200 , 0x00, sizeof(DC200));
  memset(&TIA_ch1 , 0x00, sizeof(TIA_ch1));
  memset(&TIA_ch2 , 0x00, sizeof(TIA_ch2));
  memset(&TIA_ch1_i[0], 0x00, sizeof(TIA_ch1_i));
  memset(&TIA_ch2_i[0], 0x00, sizeof(TIA_ch2_i));
  memset(&DC0_LEDcurrent[0], 0x00, sizeof(DC0_LEDcurrent));
  memset(&gain_factor, 0x00, sizeof(gain_factor));
  memset(&DC0_cal, 0x00,sizeof(DC0_cal));
  memset(&DC0_cal_div, 0x00, sizeof(DC0_cal_div));
  memset(&FullScale[0],0x00, sizeof(FullScale));
  memset(&gAFE_Trim[0], 0x00, sizeof(gAFE_Trim));

  stop = false;

#ifdef ENABLE_PPG_APP
if(Ppg_Slot != 0)
  {
  Adpd400xDrvSetOperationMode(ADPD400xDrv_MODE_IDLE);
  hrmInputRate = gAdpd400x_lcfg->hrmInputRate;
  if(hrmInputRate == 0x1F4 )     //500Hz
    adpdFreq = 0x07D0;
  else if(hrmInputRate == 0x64 ) //100Hz
    adpdFreq = 0x2710;
  else                           //50Hz
    adpdFreq = 0x4e20;
#ifndef SLOT_SELECT
  Adpd400xDrvRegWrite(0x000D, adpdFreq);
#endif
  Adpd400xDrvSetOperationMode(ADPD400xDrv_MODE_SAMPLE);
  }
#endif
  //new_current[0] = INIT_AGCLED;
#if 0
  new_current[1] = INIT_AGCLED;
  new_current[2] = INIT_AGCLED;
  new_current[3] = INIT_AGCLED;
#endif

  // Calculating full scale DC based on pulse count
  // Slot F to I
if(gn_led_slot_g!=0 && gb_static_agc_green_en==1)
  {
  g_reg_base = log2(gn_led_slot_g) * 0x20;
  Adpd400xDrvRegRead(ADPD400x_REG_COUNTS_A + g_reg_base, &PulseReg.x1A7);
  PulseValue.x1A7 = (PulseReg.x1A7 & 0x00FF);
  FullScale[0] = PulseValue.x1A7 * 8192;
  }

if(gn_led_slot_r!=0 && gb_static_agc_red_en==1)
  {
  g_reg_base = log2(gn_led_slot_r) * 0x20;
  Adpd400xDrvRegRead(ADPD400x_REG_COUNTS_A + g_reg_base, &PulseReg.x1C7);
  PulseValue.x1C7 = (PulseReg.x1C7 & 0x00FF);
  FullScale[1] = PulseValue.x1C7 * 8192;
  }

if(gn_led_slot_ir!=0 && gb_static_agc_ir_en==1)
  {
  g_reg_base = log2(gn_led_slot_ir) * 0x20;
  Adpd400xDrvRegRead(ADPD400x_REG_COUNTS_A + g_reg_base, &PulseReg.x1E7);
  PulseValue.x1E7 = (PulseReg.x1E7 & 0x00FF);
  FullScale[2] = PulseValue.x1E7 * 8192;
  }

if(gn_led_slot_b!=0 && gb_static_agc_blue_en==1)
  {
  g_reg_base = log2(gn_led_slot_b) * 0x20;
  Adpd400xDrvRegRead(ADPD400x_REG_COUNTS_A + g_reg_base, &PulseReg.x207);
  PulseValue.x207 = (PulseReg.x207 & 0x00FF);
  FullScale[3] = PulseValue.x207 * 8192;
  }

  // Set Initial TIA gain as '12.5K'
  // Slot F-I
if(gn_led_slot_g!=0 && gb_static_agc_green_en==1)
  {
  g_reg_base = log2(gn_led_slot_g) * 0x20;
  /* take backup of default TIA gain set in the DCFG */
  Adpd400xDrvRegRead(ADPD400x_REG_AFE_TRIM_A + g_reg_base, &gAFE_Trim[AGC_LED_SLOT_GREEN]);
  /* set initial TIA gain for agc */
  Adpd400xDrvRegWrite(ADPD400x_REG_AFE_TRIM_A + g_reg_base, Init_gain);
  }

if(gn_led_slot_r!=0 && gb_static_agc_red_en==1)
  {
  g_reg_base = log2(gn_led_slot_r) * 0x20;
  /* take backup of default TIA gain set in the DCFG */
  Adpd400xDrvRegRead(ADPD400x_REG_AFE_TRIM_A + g_reg_base, &gAFE_Trim[AGC_LED_SLOT_RED]);
  /* set initial TIA gain for agc */
  Adpd400xDrvRegWrite(ADPD400x_REG_AFE_TRIM_A + g_reg_base, Init_gain);
  }

if(gn_led_slot_ir!=0 && gb_static_agc_ir_en==1)
  {
  g_reg_base = log2(gn_led_slot_ir) * 0x20;
  /* take backup of default TIA gain set in the DCFG */
  Adpd400xDrvRegRead(ADPD400x_REG_AFE_TRIM_A + g_reg_base, &gAFE_Trim[AGC_LED_SLOT_IR]);
  /* set initial TIA gain for agc */
  Adpd400xDrvRegWrite(ADPD400x_REG_AFE_TRIM_A + g_reg_base, Init_gain);
  }

if(gn_led_slot_b!=0 && gb_static_agc_blue_en==1)
  {
  g_reg_base = log2(gn_led_slot_b) * 0x20;
  /* take backup of default TIA gain set in the DCFG */
  Adpd400xDrvRegRead(ADPD400x_REG_AFE_TRIM_A + g_reg_base, &gAFE_Trim[AGC_LED_SLOT_BLUE]);
  /* set initial TIA gain for agc */
  Adpd400xDrvRegWrite(ADPD400x_REG_AFE_TRIM_A + g_reg_base, Init_gain);
  }

  //Set Initial current as 0x0008
  // Slot F LED 1A Green
if(gn_led_slot_g!=0 && gb_static_agc_green_en==1)
  {
  g_reg_base = log2(gn_led_slot_g) * 0x20;
  Adpd400xDrvRegRead(ADPD400x_REG_LED_POW12_A + g_reg_base, &nRegValue);
  nRegValue = nRegValue & (~BITM_LED_POW12_X_LED_CURRENT1_X);
  nRegValue |= (nLedCurrent) << BITP_LED_POW12_X_LED_CURRENT1_X;
  Adpd400xDrvRegWrite(ADPD400x_REG_LED_POW12_A + g_reg_base, nRegValue);
  }

  // Slot G LED 3A Red
if(gn_led_slot_r!=0 && gb_static_agc_red_en==1)
  {
  g_reg_base = log2(gn_led_slot_r) * 0x20;
  Adpd400xDrvRegRead(ADPD400x_REG_LED_POW34_A + g_reg_base, &nRegValue);
  nRegValue = nRegValue & (~BITM_LED_POW34_X_LED_CURRENT3_X);
  nRegValue |= (nLedCurrent) << BITP_LED_POW34_X_LED_CURRENT3_X;
  Adpd400xDrvRegWrite(ADPD400x_REG_LED_POW34_A + g_reg_base, nRegValue);
  }

  // Slot H LED 2A IR
if(gn_led_slot_ir!=0 && gb_static_agc_ir_en==1)
  {
  g_reg_base = log2(gn_led_slot_ir) * 0x20;
  Adpd400xDrvRegRead(ADPD400x_REG_LED_POW12_A + g_reg_base, &nRegValue);
  nRegValue = nRegValue & (~BITM_LED_POW12_X_LED_CURRENT2_X);
  nRegValue |= (nLedCurrent) << BITP_LED_POW12_X_LED_CURRENT2_X;
  Adpd400xDrvRegWrite(ADPD400x_REG_LED_POW12_A + g_reg_base, nRegValue);
  }

  // Slot I LED 4A Blue
if(gn_led_slot_b!=0 && gb_static_agc_blue_en==1)
  {
  g_reg_base = log2(gn_led_slot_b) * 0x20;
  Adpd400xDrvRegRead(ADPD400x_REG_LED_POW34_A + g_reg_base, &nRegValue);
  nRegValue = nRegValue & (~BITM_LED_POW34_X_LED_CURRENT4_X);
  nRegValue |= (nLedCurrent) << BITP_LED_POW34_X_LED_CURRENT4_X;
  Adpd400xDrvRegWrite(ADPD400x_REG_LED_POW34_A + g_reg_base, nRegValue);
  }
}

/*!****************************************************************************
*
*  \brief       Static AGC deinitialization
*
*  \return       None
*****************************************************************************/
void agc_deinit(void)
{
  //ReLoad register settings after AGC, to default for ppg application
}

/*!****************************************************************************
*
*  \brief       Calculate average of each slot data
*
*  \param[in]   avg_data: pointer to get average of each channel of each slot
*
*  \return       None
*****************************************************************************/
void do_average (agc_data_avg_t *avg_data) {
  for (uint8_t slot = 0; slot < LED ; slot++) {
    for (uint8_t sample = 0; sample < SAMPLE_AVG_NUM ; sample++) {
      avg_data->ch1[slot] = avg_data->ch1[slot] + agc_data[slot].ch1[sample];
      avg_data->ch2[slot] = avg_data->ch2[slot] + agc_data[slot].ch2[sample];
    }
  }

  for(uint8_t slot = 0; slot < LED; slot++) {
    avg_data->ch1[slot] = avg_data->ch1[slot] / SAMPLE_AVG_NUM;
    avg_data->ch2[slot] = avg_data->ch2[slot] / SAMPLE_AVG_NUM;
  }
}

/*!****************************************************************************
*
*  \brief       Find TIA gain and LED current of each slot
*
*  \param[in]   DC0_cal: pointer to get the average of each slot
*
*  \return       None
*****************************************************************************/
void calculate_DC0(agc_DC0_cal_t *DC0_cal){
  for(uint8_t slot = 0; slot <LED ; slot++){
    DC0_cal->ch1[slot] = ((avg_data.ch1[slot]*100)/(float) FullScale[slot]);
    DC0_cal->ch2[slot] = ((avg_data.ch2[slot]*100)/(float) FullScale[slot]);
    if(DC0_cal->ch1[slot]> DC0_cal->ch2[slot]){
      DC0_init_check.ch[slot] = DC0_cal->ch1[slot];
      DC0.ch[slot] = DC0_cal->ch1[slot];
    }
    else{
      DC0_init_check.ch[slot] = DC0_cal->ch2[slot];
      DC0.ch[slot] = DC0_cal->ch2[slot];
    }
    if((DC0_init_check.ch[slot] * 254)<70){
      TIA_ch1_i[slot] = 0;
      TIA_ch2_i[slot] = 0;
      DC0_LEDcurrent[slot] = 127;
    }
    else{
       if(DC0_cal->ch1[slot]> DC0_cal->ch2[slot]){
        if(DC0_cal->ch1[slot] < (2*DC0_cal->ch2[slot])){
          gain_factor.ch1[slot] = 1;
          gain_factor.ch2[slot] = 1;
        }
        else if((DC0_cal->ch1[slot] >= (2*DC0_cal->ch2[slot])) && (DC0_cal->ch1[slot] <= (4*DC0_cal->ch2[slot]))){
          gain_factor.ch1[slot] = 1;
          gain_factor.ch2[slot] = 2;
        }
        else{
          if(DC0_cal->ch1[slot]>= (4*DC0_cal->ch2[slot])){
            gain_factor.ch1[slot] = 1;
            gain_factor.ch2[slot] = 4;
          }
        }
      }
      else{
          if(DC0_cal->ch2[slot] < (2*DC0_cal->ch1[slot])){
          gain_factor.ch1[slot] = 1;
          gain_factor.ch2[slot] = 1;
        }
        else if((DC0_cal->ch2[slot] >= (2*DC0_cal->ch1[slot])) && (DC0_cal->ch2[slot] <= (4*DC0_cal->ch1[slot]))){
          gain_factor.ch1[slot] = 2;
          gain_factor.ch2[slot] = 1;
        }
        else{
          if(DC0_cal->ch2[slot]>= (4*DC0_cal->ch1[slot])){
            gain_factor.ch1[slot] = 4;
            gain_factor.ch2[slot] = 1;
          }
        }
      }

      DC125.ch[slot] = DC0.ch[slot]*12.5;
      DC25.ch[slot] = DC125.ch[slot]*2;
      DC50.ch[slot] = DC25.ch[slot]*2;
      DC100.ch[slot] = DC50.ch[slot]*2;
      DC200.ch[slot] = DC100.ch[slot]*2;
      if((slot == 1) ||(slot == 2))
      {
        if(DC125.ch[slot]>=70){
          DC0_LEDcurrent[slot] = (uint16_t)((70/(DC0.ch[slot]/8)));
          if(gain_factor.ch1[slot] >= 3){
            gain_factor.ch1[slot] = 2;
          }
          if(gain_factor.ch2[slot] >= 3){
            gain_factor.ch2[slot] = 2;
          }
          TIA_ch1.ch[slot] = (float)(12.5 * gain_factor.ch1[slot]);
          TIA_ch2.ch[slot] = (float)(12.5 * gain_factor.ch2[slot]);
        }
        else if(DC25.ch[slot]>=70){
          DC0_LEDcurrent[slot] =(uint16_t)((35/(DC0.ch[slot]/8)));
          TIA_ch1.ch[slot] =(float) (25 * gain_factor.ch1[slot]);
          TIA_ch2.ch[slot] =(float) (25 * gain_factor.ch2[slot]);
        }
        else if(DC50.ch[slot]>=70){
          DC0_LEDcurrent[slot] =(uint16_t)((17.5/(DC0.ch[slot]/8)));
          TIA_ch1.ch[slot] =(float) (50 * gain_factor.ch1[slot]);
          TIA_ch2.ch[slot] =(float) (50 * gain_factor.ch2[slot]);
        }
        else if(DC100.ch[slot]>=70){
          DC0_LEDcurrent[slot] =(uint16_t)((8.75/(DC0.ch[slot]/8)));
          if(gain_factor.ch1[slot] >= 3){
            gain_factor.ch1[slot] = 2;
          }
          if(gain_factor.ch2[slot] >= 3){
            gain_factor.ch2[slot] = 2;
          }
          TIA_ch1.ch[slot] =(float) (100 * gain_factor.ch1[slot]);
          TIA_ch2.ch[slot] =(float) (100 * gain_factor.ch2[slot]);
        }
        else{
          DC0_LEDcurrent[slot] =(uint16_t)((4.375/(DC0.ch[slot]/8)));
          if(gain_factor.ch1[slot] >= 2){
            gain_factor.ch1[slot] = 1;
          }
          if(gain_factor.ch2[slot] >= 2){
            gain_factor.ch2[slot] = 1;
          }
          TIA_ch1.ch[slot] =(float) (200 * gain_factor.ch1[slot]);
          TIA_ch2.ch[slot] =(float) (200 * gain_factor.ch2[slot]);
        }
        if(DC0_LEDcurrent[slot]>=127){
          DC0_LEDcurrent[slot] = 127;
        }
      }
      else
      {
         if(DC125.ch[slot]>=70){
          DC0_LEDcurrent[slot] = (uint16_t)((75/(DC0.ch[slot]/8)));
          if(gain_factor.ch1[slot] >= 3){
            gain_factor.ch1[slot] = 2;
          }
          if(gain_factor.ch2[slot] >= 3){
            gain_factor.ch2[slot] = 2;
          }
          TIA_ch1.ch[slot] =(float) (12.5 * gain_factor.ch1[slot]);
          TIA_ch2.ch[slot] =(float) (12.5 * gain_factor.ch2[slot]);
        }
        else if(DC25.ch[slot]>=70){
          DC0_LEDcurrent[slot] =(uint16_t)((37.5/(DC0.ch[slot]/8)));
          TIA_ch1.ch[slot] =(float) (25 * gain_factor.ch1[slot]);
          TIA_ch2.ch[slot] =(float) (25 * gain_factor.ch2[slot]);
        }
        else if(DC50.ch[slot]>=70){
          DC0_LEDcurrent[slot] =(uint16_t)((18.75/(DC0.ch[slot]/8)));
          TIA_ch1.ch[slot] =(float) (50 * gain_factor.ch1[slot]);
          TIA_ch2.ch[slot] =(float) (50 * gain_factor.ch2[slot]);
        }
        else if(DC100.ch[slot]>=70){
          DC0_LEDcurrent[slot] =(uint16_t)((9.375/(DC0.ch[slot]/8)));
          if(gain_factor.ch1[slot] >= 3){
            gain_factor.ch1[slot] = 2;
          }
          if(gain_factor.ch2[slot] >= 3){
            gain_factor.ch2[slot] = 2;
          }
          TIA_ch1.ch[slot] =(float) (100 * gain_factor.ch1[slot]);
          TIA_ch2.ch[slot] =(float) (100 * gain_factor.ch2[slot]);
        }
        else{
          DC0_LEDcurrent[slot] =(uint16_t)((4.6875/(DC0.ch[slot]/8)));
          if(gain_factor.ch1[slot] >= 2){
            gain_factor.ch1[slot] = 1;
          }
          if(gain_factor.ch2[slot] >= 2){
            gain_factor.ch2[slot] = 1;
          }
          TIA_ch1.ch[slot] =(float) (200 * gain_factor.ch1[slot]);
          TIA_ch2.ch[slot] =(float) (200 * gain_factor.ch2[slot]);
        }
        if(DC0_LEDcurrent[slot]>=127){
          DC0_LEDcurrent[slot] = 127;
        }
      }

      //CH1
      if(TIA_ch1.ch[slot]==12.5){
        TIA_ch1_i[slot]=4;
      }
      else if(TIA_ch1.ch[slot]==25){
        TIA_ch1_i[slot]=3;
      }
      else if(TIA_ch1.ch[slot]==50){
        TIA_ch1_i[slot]=2;
      }
      else if(TIA_ch1.ch[slot]==100){
        TIA_ch1_i[slot]=1;
      }
      else {
        TIA_ch1_i[slot]=0;
      }

      //CH2
      if(TIA_ch2.ch[slot]==12.5){
        TIA_ch2_i[slot]=4;
      }
      else if(TIA_ch2.ch[slot]==25){
        TIA_ch2_i[slot]=3;
      }
      else if(TIA_ch2.ch[slot]==50){
        TIA_ch2_i[slot]=2;
      }
      else if(TIA_ch2.ch[slot]==100){
        TIA_ch2_i[slot]=1;
      }
      else if(TIA_ch2.ch[slot]==200){
        TIA_ch2_i[slot]=0;
      }
    }
  }
}

/*!****************************************************************************
*
*  \brief       Get static agc information
*
*  \param[in]   agc_info: pointer to get static agc information
*
*  \return       None
*****************************************************************************/
void get_agc_info(m2m2_adpd_agc_info_t *agc_info)
{
  int i;

  /* (led_index-1) is used since the CLI and tools use the indices
     1 -> LED 1 -> Green
     2 -> LED 2 -> Red
     3 -> LED 3 -> IR
     4 -> LED 4 -> Blue
  */
  for( i=0; i<SAMPLE_AVG_NUM; i++ )
  {
    agc_info->led_ch1[i] = agc_data[agc_info->led_index-1].ch1[i];
    agc_info->led_ch2[i] = agc_data[agc_info->led_index-1].ch2[i];
  }

  agc_info->DC0_LEDcurrent = DC0_LEDcurrent[agc_info->led_index-1];
  agc_info->TIA_ch1_i = TIA_ch1_i[agc_info->led_index-1];
  agc_info->TIA_ch2_i = TIA_ch2_i[agc_info->led_index-1];
}
