/**
***************************************************************************
* @file         agc.c
* @author       ADI
* @version      V1.0.0
* @date         10-June-2019
* @brief        Source file contains the static agc functions.
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
#include "adpd4000_dcfg.h"
#include "sensor_adpd_application_interface.h"
#include "adi_adpd_ssm.h"
#include "user0_config_app_task.h"
#include <math.h>
#define MODULE ("Agc.c")
/*================== LOG LEVELS=============================================*/
#include "nrf_log.h"

/*--------------------------- Defines ----------------------------------------*/
#define INIT_AGCLED_PHASE2 0x08
#define INIT_AGCLED_PHASE1 0x0D
//Initial TIA gain 12.5k
#define INIT_TIA_GAIN_PHASE2 (0x03E4)
#define INIT_TIA_GAIN_PHASE1 (0xE3E4)

 /* 127(0x7F) -> 200mA-> Max. Current per LED Driver */
#define MAX_200MA_LED_REG_VAL 127
 /* 254 -> 400mA ->  Max. Current for all 4 LED drivers combined together */
#define MAX_400MA_LED_REG_VAL 254

/* 
Max factor by which a signal can be amplified considering 
a single LED driver enabled with max current limit of 200mA

200k [max TIA gain]/12.5k [measurement TIA gain]*(127 [max LED current]/8 [measurement LED current]) = 254
*/
#define MAX_AMP_FACTOR_200MA 254

/* 
Max factor by which a signal can be amplified considering
multiple LED drivers enabled with max current limit of 400mA

200k [max TIA gain]/12.5k [measurement TIA gain]*(254 [max LED current]/8 [measurement LED current]) = 508
*/
#define MAX_AMP_FACTOR_400MA 508

/* 5 Different values of TIA Gain
3-bits used in 0x01X4 register to specify TIA gain
for each channel in the slot
0(000) -> 200k
1(001) -> 100k
2(010) -> 50k
3(011) -> 25k
4(100) -> 12.5k
*/
#define NUM_TIA_GAINS 5
/*--------------------------- Typedef ----------------------------------------*/

/*--------------------------- Public Variable --------------------------------*/
agc_data_t agc_data[SLOT_NUM];
static uint16_t gHighestSlotActive;
static uint16_t hrmInputRate;
#ifdef STATIC_AGC
extern uint8_t agc_samples;
extern uint8_t skip_first_10_samples;
#endif
extern uint8_t dvt2;/* flag to check if phase-2 chip is used */
extern uint32_t gn_led_slot_g;
extern uint32_t gn_led_slot_r;
extern uint32_t gn_led_slot_ir;
extern uint32_t gn_led_slot_b;
extern uint32_t gn_agc_active_slots;
#ifdef ENABLE_PPG_APP
extern uint32_t  Ppg_Slot;
#endif
/*--------------------------- Public Function Prototype --------------------- */

/*--------------------------- Private variables ------------------------------*/

typedef struct SlotRegister {
  uint16_t pulse[SLOT_NUM];
}SlotRegisters_t;

static uint16_t g_agc_reg_base;
static uint16_t Init_gain = INIT_TIA_GAIN_PHASE2;
//LED full scale calculation with no of pulses
uint32_t FullScale[SLOT_NUM]={0};
static SlotRegisters_t SlotRegisters;
//DC level calculation for TIA gain 12.5_25_50_100_200k
agc_DC_level_cal_t DC0_init_check, dc_normal_diff, dc_div_diff, DC0, DC125, DC25, DC50, DC100, DC200, TIA_ch1, TIA_ch2;
//DC0 calculation with initial TIA gain and LED current
agc_DC0_cal_t DC0_cal, DC0_cal_div;
//AGC final LED current and TIA gains
uint16_t DC0_LEDcurrent[SLOT_NUM]={0},TIA_ch1_i[SLOT_NUM]={0},TIA_ch2_i[SLOT_NUM]={0};
//Backup initial TIA Gain
uint16_t gAFE_Trim[SLOT_NUM] = {0};
//No. of LED drivers used per slot
uint8_t gNum_led_drivers[SLOT_NUM] = {0};
agc_data_avg_t avg_data, gain_factor;

#ifdef CUST4_SM
uint16_t agc_up_threshold,agc_low_threshold;
#endif
#ifdef TEST_AGC
uint16_t gAFE_Trim_init[SLOT_NUM] = {0};
uint16_t gInteg_offset[SLOT_NUM] = {0};
uint16_t gInteg_offset_init[SLOT_NUM] = {0};
uint16_t gLedCurrInit[SLOT_NUM][2] = {0};
#endif
/*--------------------------- Private Function Prototype ---------------------*/
void set_led_current(uint16_t *current);
void do_average (agc_data_avg_t *avg_data);
void calculate_DC0(agc_DC0_cal_t *DC0_cal);
void set_TIA_gain(uint16_t *slot_ch1_i, uint16_t *slot_ch2_i);
#ifdef CUST4_SM
uint16_t get_Max_Led_Current(uint16_t led_current,uint8_t num_drivers);
#endif

/************************************************************************/
/* ----// LED Driver Current Max Limit //----
*
* If num_driver == 1:
*     led_curr_val <= 127 (200 mA)
*
* If num_driver >= 2:
*     led_curr_val <= 254 (400 mA)
*
* ----// ------------------------------// ----
*/

/* 
  Integrator offset at different TIA gains for each of the wavelengths,
  to align zero-crossing of BPF ouput response with integerator sequence
  to improve SNR 
*/


/* Green LED - Integrator offset register val for {phase-1, phase2} chip */
const uint16_t g_integ_offset_reg_val[NUM_TIA_GAINS][2] = {
#ifdef VSM_MBOARD
  {0x1310, 0x0213}, /* 16593.75ns at 200k */
  {0x0710, 0x0207}, /* 16218.75ns at 100k */
  {0x1F0F, 0x01FF}, /* 15968.75ns at 50k */
  {0x1B0F, 0x01FB}, /* 15843.75ns at 25k */
  {0x1A0F, 0x01FA} /* 15812.5ns at 12.5k */
#else /* STUDYWATCH */
  {0x1D10, 0x021D}, /* 16906.25ns at 200k */
  {0x1110, 0x0211}, /* 16531.25ns at 100k */
  {0x0810, 0x0208}, /* 16250.00ns at 50k */
  {0x0710, 0x0207}, /* 16218.75ns at 25k */
  {0x0510, 0x0205} /* 16156.25ns at 12.5k */
#endif
};

/* Red LED - Integrator offset register val for {phase-1, phase2} chip */
const uint16_t r_integ_offset_reg_val[NUM_TIA_GAINS][2] = {
#ifdef VSM_MBOARD
  {0x0D10, 0x020D}, /* 16406.25ns at 200k */
  {0x0110, 0x0201}, /* 16031.25ns at 100k */
  {0x1B0F, 0x01FB}, /* 15843.75ns at 50k */
  {0x180F, 0x01F8}, /* 15750ns at 25k */
  {0x160F, 0x01F6} /* 15687.5ns at 12.5k */
#else /* STUDYWATCH */
  {0x0E10, 0x020E}, /* 16437.50ns at 200k */
  {0x0310, 0x0203}, /* 16093.75ns at 100k */
  {0x1E0F, 0x01FE}, /* 15937.50ns at 50k */
  {0x1C0F, 0x01FC}, /* 15875.00ns at 25k */
  {0x170F, 0x01F7} /* 15718.75ns at 12.5k */
#endif
};

/* IR LED - Integrator offset register val for {phase-1, phase2} chip */
const uint16_t ir_integ_offset_reg_val[NUM_TIA_GAINS][2] = {
#ifdef VSM_MBOARD
  {0x1010, 0x0210}, /* 16500ns at 200k */
  {0x0410, 0x0204}, /* 16125ns at 100k */
  {0x1E0F, 0x01FE}, /* 15937.5ns at 50k */
  {0x1B0F, 0x01FB}, /* 15843.75ns at 25k */
  {0x190F, 0x01F9} /* 15781.25ns at 12.5k */
#else /* STUDYWATCH */
  {0x1110, 0x0211}, /* 16531.25ns at 200k */
  {0x0610, 0x0206}, /* 16187.50ns at 100k */
  {0x0010, 0x0200}, /* 16000.00ns at 50k */
  {0x1C0F, 0x01FC}, /* 15875.00ns at 25k */
  {0x180F, 0x01F8} /* 15750.00ns at 12.5k */
#endif
};

/* Blue LED - Integrator offset register val for {phase-1, phase2} chip */
const uint16_t b_integ_offset_reg_val[NUM_TIA_GAINS][2] = {
#ifdef VSM_MBOARD
/* Note: Blue LED not present in default/alternate cfg board
 and for Rob's sensor board Integ. offeset LUT is not avail.
 Hence, setting same Integ offset for all the diff. TIA gains */
  {0x1A0F, 0x01FA}, /* 15812.5ns at 200k */
  {0x1A0F, 0x01FA}, /* 15812.5ns at 100k */
  {0x1A0F, 0x01FA}, /* 15812.5ns at 50k */
  {0x1A0F, 0x01FA}, /* 15812.5ns at 25k */
  {0x1A0F, 0x01FA} /* 15812.5ns at 12.5k */
#else /* STUDYWATCH */
  {0x1010, 0x0210}, /* 16500.00ns at 200k */
  {0x0510, 0x0205}, /* 16156.25ns at 100k */
  {0x1E0F, 0x01FE}, /* 15937.50ns at 50k */
  {0x1A0F, 0x01FA}, /* 15812.50ns at 25k */
  {0x190F, 0x01F9} /* 15781.25ns at 12.5k */
#endif
};

/************************************************************************/

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

/*!***************************************************************************
*
*  \brief       Setting Integrator offset
*
*  \param[in]   slot_id: specify slot idx (0 to 12 -> A to L)
*
*  \param[in]   tia_gain_idx: specify TIA gain idx (0 to 4 -> 200k to 12.5k)
*
*  \return      None
*****************************************************************************/
void set_integ_offset(uint8_t slot_id, uint8_t tia_gain_idx)
{
  g_agc_reg_base = slot_id * ADPD400x_SLOT_BASE_ADDR_DIFF;
  /* if dvt2 = 1 -> set integ offset for phase2 chip else phase1 chip */
  if(gn_led_slot_g & (1 << slot_id))
  {
    adi_adpddrv_RegWrite(ADPD400x_REG_INTEG_OFFSET_A + g_agc_reg_base, g_integ_offset_reg_val[tia_gain_idx][dvt2]);
  }
  else if(gn_led_slot_r & (1 << slot_id))
  {
    adi_adpddrv_RegWrite(ADPD400x_REG_INTEG_OFFSET_A + g_agc_reg_base, r_integ_offset_reg_val[tia_gain_idx][dvt2]);
  }
  else if(gn_led_slot_ir & (1 << slot_id))
  {
    adi_adpddrv_RegWrite(ADPD400x_REG_INTEG_OFFSET_A + g_agc_reg_base, ir_integ_offset_reg_val[tia_gain_idx][dvt2]);
  }
  else if(gn_led_slot_b & (1 << slot_id))
  {
    adi_adpddrv_RegWrite(ADPD400x_REG_INTEG_OFFSET_A + g_agc_reg_base, b_integ_offset_reg_val[tia_gain_idx][dvt2]);
  }
}

/*!****************************************************************************
*
*  \brief       Static AGC processing
*
*  \return       None
*****************************************************************************/
void agc_data_process() {

  adi_adpdssm_setOperationMode(ADPD400xDrv_MODE_IDLE);
  memset(&avg_data, 0x00, sizeof(avg_data));
  // Find average of each slot data
  do_average(&avg_data);

  // Find TIA gain and LED current of each slot
  calculate_DC0(&DC0_cal);

  //Apply AGC final LED current settings
  set_led_current(&DC0_LEDcurrent[0]);

  //Apply AGC final TIA gain settings
  set_TIA_gain(&TIA_ch1_i[0], &TIA_ch2_i[0]);

  adi_adpdssm_setOperationMode(ADPD400xDrv_MODE_SAMPLE);

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
  uint16_t total_led_current = 0, led_curr_per_driver = 0;
  uint8_t i, num_drivers = 0;
  uint8_t lut_index[SLOT_NUM]={0};
  static uint8_t new_reg_val[SLOT_NUM];

  //Check if its phase1 chip, if so do corresponding mapping from LUT
  if(!dvt2)
  {
    for(i=0; i <= gHighestSlotActive; i++)
    {
      if (gn_agc_active_slots & (0x01 << i)){	
        lut_index[i] = current[i];
        new_reg_val[i] =  gan_lut_phase1_reg[ lut_index[i] ];
        NRF_LOG_INFO("LED Slot:%d lut_index:%d current[0]:%d lut val:%d", i, lut_index[i], current[i], gan_lut_phase1_reg[ lut_index[i] ]);
      }
    }
  }
  else
  {
    for(i=0; i <= gHighestSlotActive; i++)
    {
      if (gn_agc_active_slots & (0x01 << i)){	
        new_reg_val[i] = current[i];
      }
    }
  }

if(gn_agc_active_slots != 0)
  {
    for(int slot = 0; slot <= gHighestSlotActive; slot++){
      if (gn_agc_active_slots & (0x01 << slot))
      {
#ifdef CUST4_SM
        if (new_reg_val[slot] > agc_up_threshold) {   //Checking condition for Maximum LED threshold value
           new_reg_val[slot] = agc_up_threshold;
        } if (new_reg_val[slot] < agc_low_threshold) {  // Checking condition for Minimum LED threshold value
           new_reg_val[slot]= agc_low_threshold;
        }                 
#endif
        total_led_current = new_reg_val[slot];
        num_drivers = gNum_led_drivers[slot];
        if(num_drivers == 0)
        {
          return;
        }
#ifdef CUST4_SM
        total_led_current = get_Max_Led_Current(total_led_current,num_drivers);  //set Maximum LED current if total_led_current is out of the valid range.
#endif    
        led_curr_per_driver = total_led_current / num_drivers;
        g_agc_reg_base = slot * ADPD400x_SLOT_BASE_ADDR_DIFF;

        /*Led current setting for LED1A or LED1B */
        adi_adpddrv_RegRead(ADPD400x_REG_LED_POW12_A + g_agc_reg_base, &nRegValue);
        if((nRegValue & BITM_LED_POW12_X_LED_CURRENT1_X) != 0){
          if(num_drivers == 1)
          {
            led_curr_per_driver = total_led_current - ((gNum_led_drivers[slot] - 1) * led_curr_per_driver);
          }
          nRegValue = nRegValue & (~BITM_LED_POW12_X_LED_CURRENT1_X);
          nRegValue |= led_curr_per_driver << BITP_LED_POW12_X_LED_CURRENT1_X;
          adi_adpddrv_RegWrite(ADPD400x_REG_LED_POW12_A + g_agc_reg_base, nRegValue);
          num_drivers--;
        }

        /*Led current setting for LED2A or LED2B */
        adi_adpddrv_RegRead(ADPD400x_REG_LED_POW12_A + g_agc_reg_base, &nRegValue);
        if((nRegValue & BITM_LED_POW12_X_LED_CURRENT2_X) != 0){
          if(num_drivers == 1)
          {
            led_curr_per_driver = total_led_current - ((gNum_led_drivers[slot] - 1) * led_curr_per_driver);
          }
          nRegValue = nRegValue & (~BITM_LED_POW12_X_LED_CURRENT2_X);
          nRegValue |= led_curr_per_driver << BITP_LED_POW12_X_LED_CURRENT2_X;
          adi_adpddrv_RegWrite(ADPD400x_REG_LED_POW12_A + g_agc_reg_base, nRegValue);
          num_drivers--;
        }

        /*Led current setting for LED3A or LED3B */
        adi_adpddrv_RegRead(ADPD400x_REG_LED_POW34_A + g_agc_reg_base, &nRegValue); 
        if((nRegValue & BITM_LED_POW34_X_LED_CURRENT3_X) != 0){
          if(num_drivers == 1)
          {
            led_curr_per_driver = total_led_current - ((gNum_led_drivers[slot] - 1) * led_curr_per_driver);
          }
          nRegValue = nRegValue & (~BITM_LED_POW34_X_LED_CURRENT3_X);
          nRegValue |= led_curr_per_driver << BITP_LED_POW34_X_LED_CURRENT3_X;
          adi_adpddrv_RegWrite(ADPD400x_REG_LED_POW34_A + g_agc_reg_base, nRegValue);
          num_drivers--;
        }

        /*Led current setting for LED4A or LED4B */
        adi_adpddrv_RegRead(ADPD400x_REG_LED_POW34_A + g_agc_reg_base, &nRegValue); 
        if((nRegValue & BITM_LED_POW34_X_LED_CURRENT4_X) != 0){
          if(num_drivers == 1)
          {
            led_curr_per_driver = total_led_current - ((gNum_led_drivers[slot] - 1) * led_curr_per_driver);
          }
          nRegValue = nRegValue & (~BITM_LED_POW34_X_LED_CURRENT4_X);
          nRegValue |= led_curr_per_driver << BITP_LED_POW34_X_LED_CURRENT4_X;
          adi_adpddrv_RegWrite(ADPD400x_REG_LED_POW34_A + g_agc_reg_base, nRegValue);
          num_drivers--;
        }      
      }
    }
 }
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
  uint8_t nTiaGainIdx = 0x00; /* Find idx of Min(CH1_TIA_GAIN, CH2_TIA_GAIN) */
  if(gn_agc_active_slots != 0)
   {
    for(int slot = 0; slot <= gHighestSlotActive; slot++){
      if (gn_agc_active_slots & (0x01 << slot))
      { 
        g_agc_reg_base = slot * ADPD400x_SLOT_BASE_ADDR_DIFF;
        /* check if ch2 is enabled */
        adi_adpddrv_RegRead(ADPD400x_REG_TS_CTRL_A + g_agc_reg_base, &nTsCtrl);
        nCh2Enable = (nTsCtrl & BITM_TS_CTRL_A_CH2_EN_A) >> BITP_TS_CTRL_A_CH2_EN_A;

        adi_adpddrv_RegRead(ADPD400x_REG_AFE_TRIM_A + g_agc_reg_base, &nTIAgain);
        nTIAgain = nTIAgain & (~BITM_AFE_TRIM_X_TIA_GAIN_CH1_X);
        nTIAgain |= (slot_ch1_i[slot]) << BITP_AFE_TRIM_X_TIA_GAIN_CH1_X;
        if(nCh2Enable)
        {
          nTIAgain = nTIAgain & (~BITM_AFE_TRIM_X_TIA_GAIN_CH2_X);
          nTIAgain |= (slot_ch2_i[slot]) << BITP_AFE_TRIM_X_TIA_GAIN_CH2_X;
        }
        else
        {
          /* if ch2 not enabled - restore default value */
          nTIAgain = nTIAgain & (~BITM_AFE_TRIM_X_TIA_GAIN_CH2_X);
          nTIAgain |= gAFE_Trim[slot] & BITM_AFE_TRIM_X_TIA_GAIN_CH2_X;
        }
        adi_adpddrv_RegWrite(ADPD400x_REG_AFE_TRIM_A + g_agc_reg_base, nTIAgain);
        if(nCh2Enable)
        {
          /* get idx of lowest gain among ch1 and ch2,
          higher the gain idx, lower will be the gain value */
          nTiaGainIdx = (slot_ch1_i[slot] >= slot_ch2_i[slot]) ? slot_ch1_i[slot] : slot_ch2_i[slot];
          /* set the integrator offset as per TIA gain */
          set_integ_offset(slot, nTiaGainIdx);
        }
        else
        {
          /* set the integrator offset as per TIA gain */
          set_integ_offset(slot, slot_ch1_i[slot]);
        }
#ifdef TEST_AGC
        /* read the integrator offset */
        adi_adpddrv_RegRead(ADPD400x_REG_INTEG_OFFSET_A + g_agc_reg_base, &gInteg_offset[slot]);
#endif
      }
    }
  }
}

/*!***************************************************************************
*
*  \brief       Get the count of LED drivers enabled for the slot specified
*
*  \param[in]   slot_id: specify slot idx (0 to 12 -> A to L)
*
*  \return      None
*****************************************************************************/
void get_led_drv_count(uint8_t slot_id)
{
    uint16_t nRegValue = 0x0000;
    g_agc_reg_base = slot_id * ADPD400x_SLOT_BASE_ADDR_DIFF;

    /* check if LED1A or LED1B enabled */
    adi_adpddrv_RegRead(ADPD400x_REG_LED_POW12_A + g_agc_reg_base, &nRegValue);
    if((nRegValue & BITM_LED_POW12_X_LED_CURRENT1_X) != 0){
    gNum_led_drivers[slot_id]++;
    }

    /* check if LED2A or LED2B enabled */
    adi_adpddrv_RegRead(ADPD400x_REG_LED_POW12_A + g_agc_reg_base, &nRegValue);
    if((nRegValue & BITM_LED_POW12_X_LED_CURRENT2_X) != 0){
    gNum_led_drivers[slot_id]++;
    }

    /* check if LED3A or LED3B enabled */
    adi_adpddrv_RegRead(ADPD400x_REG_LED_POW34_A + g_agc_reg_base, &nRegValue); 
    if((nRegValue & BITM_LED_POW34_X_LED_CURRENT3_X) != 0){
    gNum_led_drivers[slot_id]++;
    }
         
    /* check if LED4A or LED4B enabled */
    adi_adpddrv_RegRead(ADPD400x_REG_LED_POW34_A + g_agc_reg_base, &nRegValue); 
    if((nRegValue & BITM_LED_POW34_X_LED_CURRENT4_X) != 0){
    gNum_led_drivers[slot_id]++;
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
  uint16_t adpdFreq;
  uint16_t total_led_current = 0, led_curr_per_driver = 0;
  uint8_t num_drivers = 0;
  
  /* Highest active slot */
  adi_adpdssm_GetParameter(ADPD400x_HIGHEST_SLOT_NUM, 0, &gHighestSlotActive);

  //Check if its phase1 chip and set corresponding INIT_AGCLED
  if(!dvt2)
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
  memset(&gNum_led_drivers[0], 0x00, sizeof(gNum_led_drivers));
#ifdef TEST_AGC
  memset(&gAFE_Trim_init[0], 0x00, sizeof(gAFE_Trim_init));
  memset(&gInteg_offset[0], 0x00, sizeof(gInteg_offset));
  memset(&gInteg_offset_init[0], 0x00, sizeof(gInteg_offset_init));
#endif
#ifdef CUST4_SM
    get_agc_led_current_threshold_from_user0_config_app_lcfg(&agc_up_threshold,&agc_low_threshold);    
#endif
#ifdef ENABLE_PPG_APP
if(Ppg_Slot != 0)
  {
  adi_adpdssm_setOperationMode(ADPD400xDrv_MODE_IDLE);
  hrmInputRate = gAdpd400x_lcfg->hrmInputRate;
  if(hrmInputRate == 0x1F4 )     //500Hz
    adpdFreq = 0x07D0;
  else if(hrmInputRate == 0x64 ) //100Hz
    adpdFreq = 0x2710;
  else                           //50Hz
    adpdFreq = 0x4e20;
#ifndef SLOT_SELECT
  adi_adpddrv_RegWrite(0x000D, adpdFreq); /*TODO: handle slot switching case */
#endif
  adi_adpdssm_setOperationMode(ADPD400xDrv_MODE_SAMPLE);
  }
#endif

if(gn_agc_active_slots != 0){ 
  for(int slot = 0; slot <= gHighestSlotActive; slot++){
    if (gn_agc_active_slots & (0x01 << slot)){ 
      g_agc_reg_base = slot * ADPD400x_SLOT_BASE_ADDR_DIFF;
      adi_adpddrv_RegRead(ADPD400x_REG_COUNTS_A + g_agc_reg_base, &SlotRegisters.pulse[slot]);
      SlotRegisters.pulse[slot] = (SlotRegisters.pulse[slot] & 0x00FF);
      FullScale[slot] = SlotRegisters.pulse[slot] * 8192;

      /* take backup of default TIA gain set in the DCFG */
      adi_adpddrv_RegRead(ADPD400x_REG_AFE_TRIM_A + g_agc_reg_base, &gAFE_Trim[slot]);
      /* set initial TIA gain for agc */
      adi_adpddrv_RegRead(ADPD400x_REG_AFE_TRIM_A + g_agc_reg_base, &nRegValue);
      nRegValue = nRegValue & (~BITM_AFE_TRIM_X_TIA_GAIN_CH1_X);
      nRegValue |= (Init_gain & BITM_AFE_TRIM_X_TIA_GAIN_CH1_X);
      nRegValue = nRegValue & (~BITM_AFE_TRIM_X_TIA_GAIN_CH2_X);
      nRegValue |= (Init_gain & BITM_AFE_TRIM_X_TIA_GAIN_CH2_X);
      adi_adpddrv_RegWrite(ADPD400x_REG_AFE_TRIM_A + g_agc_reg_base, nRegValue);

      /* set the integrator offset as per TIA gain */
      set_integ_offset(slot, Init_gain & BITM_AFE_TRIM_X_TIA_GAIN_CH1_X);/* ch1 and ch2 TIA gain is same */

#ifdef TEST_AGC
      /* take backup of TIA gain set during Init */
      adi_adpddrv_RegRead(ADPD400x_REG_AFE_TRIM_A + g_agc_reg_base, &gAFE_Trim_init[slot]);
      /* take backup of Integrator offset set during Init */
      adi_adpddrv_RegRead(ADPD400x_REG_INTEG_OFFSET_A + g_agc_reg_base, &gInteg_offset_init[slot]);
#endif
      /* get the count of led drivers enabled per slot */
      get_led_drv_count(slot);
      total_led_current = nLedCurrent;
      num_drivers = gNum_led_drivers[slot];
      if(num_drivers == 0)
      {
        return;
      }
      led_curr_per_driver = total_led_current/ num_drivers;

      /*Led current setting for LED1A or LED1B */
      adi_adpddrv_RegRead(ADPD400x_REG_LED_POW12_A + g_agc_reg_base, &nRegValue);
      if((nRegValue & BITM_LED_POW12_X_LED_CURRENT1_X) != 0){
        if(num_drivers == 1)
        {
          led_curr_per_driver = total_led_current - ((gNum_led_drivers[slot] - 1) * led_curr_per_driver);
        }
        nRegValue = nRegValue & (~BITM_LED_POW12_X_LED_CURRENT1_X);
        nRegValue |= (led_curr_per_driver) << BITP_LED_POW12_X_LED_CURRENT1_X;
        adi_adpddrv_RegWrite(ADPD400x_REG_LED_POW12_A + g_agc_reg_base, nRegValue);
        num_drivers--;
      }
      /*Led current setting for LED2A or LED2B */
      adi_adpddrv_RegRead(ADPD400x_REG_LED_POW12_A + g_agc_reg_base, &nRegValue);
      if((nRegValue & BITM_LED_POW12_X_LED_CURRENT2_X) != 0){
        if(num_drivers == 1)
        {
          led_curr_per_driver = total_led_current - ((gNum_led_drivers[slot] - 1) * led_curr_per_driver);
        }
        nRegValue = nRegValue & (~BITM_LED_POW12_X_LED_CURRENT2_X);
        nRegValue |= (led_curr_per_driver) << BITP_LED_POW12_X_LED_CURRENT2_X;
        adi_adpddrv_RegWrite(ADPD400x_REG_LED_POW12_A + g_agc_reg_base, nRegValue);
        num_drivers--;
      }
      /*Led current setting for LED3A or LED3B */
      adi_adpddrv_RegRead(ADPD400x_REG_LED_POW34_A + g_agc_reg_base, &nRegValue);
      if((nRegValue & BITM_LED_POW34_X_LED_CURRENT3_X) != 0){
        if(num_drivers == 1)
        {
          led_curr_per_driver = total_led_current - ((gNum_led_drivers[slot] - 1) * led_curr_per_driver);
        }
        nRegValue = nRegValue & (~BITM_LED_POW34_X_LED_CURRENT3_X);
        nRegValue |= (led_curr_per_driver) << BITP_LED_POW34_X_LED_CURRENT3_X;
        adi_adpddrv_RegWrite(ADPD400x_REG_LED_POW34_A + g_agc_reg_base, nRegValue);
        num_drivers--;
      }      
      /*Led current setting for LED4A or LED4B */
      adi_adpddrv_RegRead(ADPD400x_REG_LED_POW34_A + g_agc_reg_base, &nRegValue);
      if((nRegValue & BITM_LED_POW34_X_LED_CURRENT4_X) != 0){
        if(num_drivers == 1)
        {
          led_curr_per_driver = total_led_current - ((gNum_led_drivers[slot] - 1) * led_curr_per_driver);
        }
        nRegValue = nRegValue & (~BITM_LED_POW34_X_LED_CURRENT4_X);
        nRegValue |= (led_curr_per_driver) << BITP_LED_POW34_X_LED_CURRENT4_X;
        adi_adpddrv_RegWrite(ADPD400x_REG_LED_POW34_A + g_agc_reg_base, nRegValue);
        num_drivers--;
      }
#ifdef TEST_AGC
     adi_adpddrv_RegRead(ADPD400x_REG_LED_POW12_A + g_agc_reg_base, &gLedCurrInit[slot][0]);
     adi_adpddrv_RegRead(ADPD400x_REG_LED_POW34_A + g_agc_reg_base, &gLedCurrInit[slot][1]); 
#endif
    }
  }
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
  gn_agc_active_slots = 0;
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
  for (uint8_t slot = 0; slot <= gHighestSlotActive ; slot++) {
    if (gn_agc_active_slots & (0x01 << slot)){
      for (uint8_t sample = 0; sample < SAMPLE_AVG_NUM ; sample++) {
        avg_data->ch1[slot] = avg_data->ch1[slot] + agc_data[slot].ch1[sample];
        avg_data->ch2[slot] = avg_data->ch2[slot] + agc_data[slot].ch2[sample];
      }
    }
  }

  for(uint8_t slot = 0; slot <= gHighestSlotActive; slot++) {
    if (gn_agc_active_slots & (0x01 << slot)){
      avg_data->ch1[slot] = avg_data->ch1[slot] / SAMPLE_AVG_NUM;
      avg_data->ch2[slot] = avg_data->ch2[slot] / SAMPLE_AVG_NUM;
    }
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
  uint16_t mul_factor = 0x0000;
  for(uint8_t slot = 0; slot <= gHighestSlotActive ; slot++){
   if (gn_agc_active_slots & (0x01 << slot)){
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
    /* In case no. of active led driver is more than 1, 
           - use multiplication factor corresponding to max current of 400mA, 
       else 
           - use multiplication factor corresponding to max current of 200mA 
    */
    mul_factor = (gNum_led_drivers[slot] > 1) ? MAX_AMP_FACTOR_400MA : MAX_AMP_FACTOR_200MA;
    if((DC0_init_check.ch[slot] * mul_factor)<70){
      TIA_ch1_i[slot] = 0;
      TIA_ch2_i[slot] = 0;
      DC0_LEDcurrent[slot] = (gNum_led_drivers[slot] > 1) ? MAX_400MA_LED_REG_VAL : MAX_200MA_LED_REG_VAL;
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
      if((gn_led_slot_r & (0x01 << slot)) || (gn_led_slot_ir & (0x01 << slot)))
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
        if(gNum_led_drivers[slot] > 1)
        {
           if(DC0_LEDcurrent[slot] >= MAX_400MA_LED_REG_VAL){
            DC0_LEDcurrent[slot] = MAX_400MA_LED_REG_VAL;
          }
        }
        else{
          if(DC0_LEDcurrent[slot] >= MAX_200MA_LED_REG_VAL){
            DC0_LEDcurrent[slot] = MAX_200MA_LED_REG_VAL;
          }
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
        if(gNum_led_drivers[slot] > 1)
        {
           if(DC0_LEDcurrent[slot] >= MAX_400MA_LED_REG_VAL){
            DC0_LEDcurrent[slot] = MAX_400MA_LED_REG_VAL;
          }
        }
        else{
          if(DC0_LEDcurrent[slot] >= MAX_200MA_LED_REG_VAL){
            DC0_LEDcurrent[slot] = MAX_200MA_LED_REG_VAL;
          }
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

#ifdef CUST4_SM
/*!****************************************************************************
*
*  \brief       Get Max LED Driver current if input current value is out of the valid range. 
*
*  \param[in]   led_current: this variable to get current led Value
*
*  \param[in]   num_drivers: this variable to get number LED drivers
*
*  \return      returning Maximum LED current based on LED drivers
*****************************************************************************/

uint16_t get_Max_Led_Current(uint16_t led_current,uint8_t num_drivers) {

    if(num_drivers > 1)
    {
       if(led_current >= MAX_400MA_LED_REG_VAL){
        led_current = MAX_400MA_LED_REG_VAL;
      }
    }
    else{
      if(led_current >= MAX_200MA_LED_REG_VAL){
        led_current = MAX_200MA_LED_REG_VAL;
      }
    }
   return led_current;
}
#endif