/**
    ***************************************************************************
    * @file    adpd4000_dcfg.c
    * @author  ADI Team
    * @version V0.0.1
    * @date    03-April-2018
    * @brief   ADPD4000_default configuration file
    ***************************************************************************
     * @attention
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

#include "nrf_log_ctrl.h"

/* System Task Module Log settings */
#define NRF_LOG_MODULE_NAME ADPD4000

#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

#include <stdlib.h>
#include <adpd4000_dcfg.h>
#include <adpd400x_drv.h>
#include <adpd400x_reg.h>
#include <sensor_adpd_application_interface.h>
#ifdef DCB
#include "adi_dcb_config.h"
#include "dcb_interface.h"
#endif
#include "adi_calendar.h"

#define MAXADPD4000DCFGSIZE (228) /* Max 4*57 registers */

#ifdef DCB
static volatile bool g_adpd4000_dcb_present = false;
static uint32_t g_current_dcb[MAX_ADPD4000_DCB_PKTS*MAXADPD4000DCBSIZE] = {'\0'};
#endif
extern uint32_t  g_therm_slot_en_bit_mask;
extern uint32_t Ppg_Slot;
extern uint32_t gn_led_slot_g;
extern uint32_t gn_led_slot_r;
extern uint32_t gn_led_slot_ir;
extern uint32_t gn_led_slot_b;
extern uint8_t dvt2;

static uint32_t g_created_dcfg[MAXADPD4000DCFGSIZE] = {'\0'};
bool check_dcfg_created = false;
#ifdef SLOT_SELECT
extern bool check_temp_slot_set;
bool check_ppg_slot_set = false;
#endif
static uint32_t g_current_dcfg[MAXADPD4000DCFGSIZE] = {'\0'};


#ifdef EVTBOARD

#define GENERAL_DCFG_SIZE 14 /* No. of registers common for all the slots */
#define SLOT_DCFG_SIZE 17 /* No. of registers per slot */
uint32_t slot_general_dcfg[SLOT_DCFG_SIZE] = {'\0'};

/* used as a reference to create dcfg for different slots */
const uint32_t slotA_default_dcfg_4000[] = {
  0x01000000,
  0x01010000,
  0x01020000,
  0x01030000,
  0x01040000,
  0x01050000,
  0x01060000,
  0x01070000,
  0x01080000,
  0x01090000,
  0x010A0000,
  0x010B0000,
  0x010C0000,
  0x010D0000,
  0x010E0000,
  0x010F0000,
  0x01100000
};

/* stores general registers common for all the slots */
const uint32_t general_dcfg_4000_g[] = {
    0x00090097,
    0x00078FFF,
    0x000B02F6,
    0x000C0012,
#ifdef SLOT_SELECT
    0x000D0D05,//0x0D05 -> 300Hz
#else
    0x000D4E20, //50Hz
#endif
    0x000E0000,
    0x000F0006,
    0x00148000,
#ifdef SLOT_SELECT
    0x00202022,
#else
    0x00202222,
#endif
#ifdef SLOT_SELECT
    0x00210004,//IN7 and IN8 configured as a differential pair
#else
    0x00210000,
#endif
    0x00220083, //GPIO0 - Output Inverted, GPIO2 - Output Normal
    0x00230302,
    0x00240000, //GPIO2 - Output Logic = 0
    0x00100000, //opmode
    };

/* stores register values for ppg/green_led config */
const uint32_t ppg_dcfg_4000_g[] = {
    0x00004000, //wrt slot-F reg-> 0x01A0
    0x000041DA, //0x01A1
    0x00000005, //0x01A2
    0x00005002, //0x01A3
    0x0000E3D2,//0x01A4E2C1, //0x01A4 //This setting is for DVT1
#ifdef DVT
    0x00000005,//0040 //0x01A5
#elif PCBA
    0x00000500, //0x01A5
#else
    0x00000005,//0040 //0x01A5
#endif
    0x00000000, //0x01A6
    0x00000140, // 64 pulse //0x01A7
    0x00000000, //0x01A8
    0x00000210, //0x01A9
    0x00000003, //0x01AA
    0x00001010, //0x01AB
    0x00000101, //0x01AC
    0x00000099, //0x01AD
    0x00000000, //0x01AE
    0x00000000, //0x01AF
    0x00000004  //signal size = 4 bytes //0x01B0
};

/* stores register values for temp_calibration_resistor config */
const uint32_t temp_cal_dcfg_4000[] = {
    0x00000000, //0
    0x000041DA, //1
    0x00000030, //2
    0x00005A40, //3
    0x0000E281, //4 //This setting is for DVT1
    0x00000000, //5
    0x00000000, //6
    0x00000101, //7
    0x00000000, //8
    0x00000210, //9 //DEFAULT
    0x00000003, //10
    0x0000130F, //11
    0x00000210, //12
    0x00000000, //13
    0x00000000, //14
    0x00000000, //15
    0x00000004 //16 //signal size = 4 bytes
    };

/* stores register values for temp_thermistor config */
const uint32_t temp_thermistor_dcfg_4000[] = {
    0x00000000, //0
    0x000041DA, //1
    0x00000010, //2
    0x00005A40, //3
    0x0000E281, //4 //This setting is for DVT1
    0x00000000, //5
    0x00000000, //6
    0x00000101, //7
    0x00000000, //8
    0x00000210, //9 //DEFAULT
    0x00000003, //10
    0x0000130F, //11
    0x00000210, //12
    0x00000000, //13
    0x00000000, //14
    0x00000000, //15
    0x00000004 //16 //signal size = 4 bytes
    };

/* stores register values for ecg config */
const uint32_t ecg_dcfg_4000[] = {
    0x00000000, //0
    0x000000E6, //1
    0x00000700, //2
    0x00000000, //3
    0x0000E2C1, //4 //This setting is for DVT1
    0x00000000, //5
    0x00000000, //6
    0x00000102, //7
    0x00001000, //8
    0x00000210, //9 //DEFAULT
    0x00000003, //10
    0x0000000D, //11
    0x00000210, //12
    0x00000000, //13
    0x00000000, //14
    0x00000000, //15
    0x00000003 //16 //signal size = 3 bytes
    };

/* stores register values for red_led config */
const uint32_t dcfg_4000_r[] = {
    0x00004000, //0
    0x000041DA, //1
    0x00000005, //2
    0x00005002, //3
    0x0000E3D2, //4 //This setting is for DVT1
    0x00000000, //5
    0x00000005, //6
    0x00000140, //7
    0x00000000, //8
    0x00000210, //9 //DEFAULT
    0x00000003, //10 //DEFAULT
    0x00001010, //11
    0x00000101, //12
    0x00000099, //13
    0x00000000, //14
    0x00000000, //15
    0x00000004 //16 //signal size = 4 bytes
    };

/* stores register values for ir_led config */
const uint32_t dcfg_4000_ir[] = {
    0x00004000, //0
    0x000041DA, //1
    0x00000005, //2
    0x00005002, //3
    0x0000E3D2, //4 //This setting is for DVT1
#ifdef DVT
    0x00000500, //5
#elif PCBA
    0x00000005, //5
#else
    0x00000500, //5
#endif
    0x00000000, //6
    0x00000140, //7
    0x00000000, //8
    0x00000210, //9 //DEFAULT
    0x00000003, //10 //DEFAULT
    0x00001010, //11
    0x00000101, //12
    0x00000099, //13
    0x00000000, //14
    0x00000000, //15
    0x00000004 //16 //signal size = 4 bytes
    };

/* stores register values for blue_led config */
const uint32_t dcfg_4000_b[] = {
    0x00004000, //0
    0x000041DA, //1
    0x00000005, //2
    0x00005002, //3
    0x0000E3D2, //4 //This setting is for DVT1
    0x00000000, //5
    0x00000500, //6
    0x00000140, //7
    0x00000000, //8
    0x00000210, //9 //DEFAULT
    0x00000003, //10 //DEFAULT
    0x00001010, //11
    0x00000101, //12
    0x00000099, //13
    0x00000000, //14
    0x00000000, //15
    0x00000004 //16 //signal size = 4 bytes
    };

/*
* ppg_dcfg_4000_g at idx = 1 -> specifies app id for ppg
* ppg_dcfg_4000_g at idx = 4 -> specifies app id for adpd_g raw data stream
* ppg_dcfg_4000_g at both the indices in the array needs to be maintained
*/
const uint32_t* app_dcfg_list[] = {ecg_dcfg_4000, ppg_dcfg_4000_g, temp_thermistor_dcfg_4000, temp_cal_dcfg_4000, ppg_dcfg_4000_g, dcfg_4000_r, dcfg_4000_ir, dcfg_4000_b};

const uint32_t dcfg_org_4000_g[] = {
    //0x000F8000,
    0x00090097,
    0x00078FFF,
    0x000B02F6,
    0x000C0012,
    0x000D4E20,//50Hz
    0x000E0000,
    0x000F0006,
    0x00148000,
    0x00200022, //IN5+IN6 & IN7+IN8 floating during sleep
    0x00210004, //IN5/IN6 configured as a differential pair  //0x00210000
    0x00220083, //GPIO0 - Output Inverted, GPIO2 - Output Normal
    0x00230302,
    0x00240000, //GPIO2 - Output Logic = 0
    0x01310000, // DATA2_B
    0x01510000, // DATA2_C
    0x01710000, // DATA2_D
    0x01910000, // DATA2_E
    0x01B10000, // DATA2_F
    0x01D10000, // DATA2_G
#if 1
// Slots config
/// Timeslot A - Sleep float mode ECG with multiple charge tranfers
/// IN5 and IN6 as differential pair
  0x01000000,  //  CH2 Disabled, Input resistor 500 ohms 0000, 6.25k ohms 0400
  0x010100E6,  //  skip preconditioning, No bpf
  0x01020700,  // IN5&IN6 differential pair to CH 1
  0x01030000,  //
  0x0104E2C1,  // TIA gain, 2C0 200k, 2C1 100k, Vref = 0.88V  //This setting is for DVT1
  0x01070102,  // number of pulses = 2
  0x01081000,  // float mode, min period
  0x010A0003, // Int width
  0x010B000D, // Integrator timing offset
  0x010C0210, // Mod pulse width, mod offset 16us
  0x010D0000,// No Chop
  0x010E0000,
  0x010F0000,
  0x01100003,  // 3 bytes signal
  0x01050000,  // LEDs off
  0x01060000,  // LEDs off
#endif
/*Configuration for Time slot F*/
    0x01A04000, //CH2 enable
    //0x01A00000,
    0x01A141DA,
    0x01A20005,
    0x01A35002,
    //0x01A4E2C1,
    0x01A4E3D2, //TIA GAIN, CH1 = CH2 = 50k //This setting is for DVT1
#ifdef DVT
    0x01A50005,
#elif PCBA
    0x01A50500,
#else
    0x01A50005,//0040
#endif
    0x01A60000,
    0x01A70140, // 64 pulse
#if 0
    0x01AB0E10,
    0x01AD0000, //  No chop
    0x01AE1F00,
    0x01AF1F00,
#endif
    //0x01A90000,//added
#if 1
    0x01A80000, // PERIOD_F
    0x01A90210, // LED_PULSE_F
    0x01AA0003, // INTEG_SETUP_F
    0x01AB1010, // INTEG_OS_F
    0x01AC0101, // MOD_PULSE_F
    0x01AD0099, // PATTERN_F Reset value, chop -++-
    0x01AE0000,
    0x01AF0000,
    0x01B00004,  // DATA_FORMAT_F
    0x01B20000,  // DECIMATE_F
    0x01B30026,  // DIGINT_LIT_F
    0x01B42306,  // DIGINT_DARK_F
    0x01B50000,  // THRESH_CFG_F
    0x01B60000,  // THRESH0_F
    0x01B70000,  // THRESH1_F
#endif
    //0x01200010,
    0x00100500, /* Enable slot F for green LED data*/

/*Moving thermistor sensor to slot D*/
#if 1
    0x01600000,   /*INPUT_RESISTOR set ot 500 ohm*/
    0x016141DA,   /*pre conditioning period of 8us, TIA+INT+BPF+ADC signal path selected for Slot A */
    0x01620010,   /*IN3 connected to CH1 of slot A*/
    0x01635A40,   /*Pre condition with TIA V_ref,VC2_pulse changes between active state and alternate state,
                    VC2_Active = V-ref and VC2_alternate = V_ref + 250mV*/
    0x0164E281,   /* R_int = 400K, R_tia_c h1 = 100k, TIA_Vref = 0.8855  V, Vref_pulse = 0.8855 V; No v-ref pulsing */ //This setting is for DVT1
    0x01650000,
    0x01660000,
    0x01670101,   /*single pulse and single repition is used*/
    0x01680000,   /*MOD_TYPE set to mode 0; normal mode*/
    //0x01690419,   /*LED pulse width and offset*/
    0x016A0003,   /*Integrator clock width set to 3us*/
    0x016B130F,   /*Integrator offset set to 15.595us*/
    0x016C0210,   /*Modulation pulse width set to 2us, with offset of 16us*/
    0x016E0000,   /*ADC CH1 offset set to 0*/
    0x016F0000,   /*ADC CH2 offset set to 0*/
    0x01700004,   /* signal size Data format set to 4 bytes;*/
    //0x01720310,   /* Decimation set to 50 */
#endif //slot D

/*Moving calibration resistor to slot E*/
#if 1
    0x01800000,   /*INPUT_RESISTOR set ot 500 ohm*/
    0x018141DA,   /*pre conditioning period of 8us, TIA+INT+BPF+ADC signal path selected for Slot B */
    0x01820030,   /*IN4 connected to CH1 of slot B*/
    0x01835A40,   /*Pre condition with TIA V_ref,VC2_pulse changes between active state and alternate state,
                    VC2_Active = V-ref and VC2_alternate = V_ref + 250mV*/
    0x0184E281,   /* R_int = 400K, R_tia_c h1 = 100k, TIA_Vref = 0.8855  V, Vref_pulse = 0.8855 V; No v-ref pulsing */ //This setting is for DVT1
    0x01850000,
    0x01860000,
    0x01870101,   /*single pulse and single repition is used*/
    0x01880000,   /*MOD_TYPE set to mode 0; normal mode*/
    //0x01890419,   /*LED pulse width and offset*/
    0x018A0003,   /*Integrator clock width set to 3us*/
    0x018B130F,   /*Integrator offset set to 15.595us*/
    0x018C0210,   /*Modulation pulse width set to 2us, with offset of 16us*/
    0x018E0000,   /*ADC CH1 offset set to 0*/
    0x018F0000,   /*ADC CH2 offset set to 0*/
    0x01900004,   /*signal size data format set to 4 bytes; */
    //0x01920310,   /* Decimation set to 50 */
#endif //slot E

/* Set the signal and dark sample data size of the slots from A to E to zero*/
    0x01300000,
    0x01500000,
    //0x01700000,
    //0x01900000,
    0x01B00004,
    0xFFFFFFFF,
};

const uint32_t dcfg_org_4000_r[] = {
    0x00090097,
    0x00078FFF,
    0x000B02F6,
    0x000C0012,
    0x000D4E20,//0x2710
    0x000E0000,
    0x000F0006,
    0x00148000,
    0x00202222,
    0x00210000,
    0x00220003,
    0x00230302,
    0x00240001,

/*Configuration for Time slot G*/
    0x01C04000, //CH2 enable
    0x01C141DA,
    0x01C20005,
    0x01C35002,
    0x01C403D2, //TIA GAIN, CH1 = CH2 = 50k
    0x01C50000,
    0x01C60005, //Red 3A
    0x01C70140,
    0x01C80000, // PERIOD_G
    0x01C90210, // LED_PULSE_G
    0x01CA0003, // INTEG_SETUP_G
    0x01CB1010, // INTEG_OS_G
    0x01CC0101, // MOD_PULSE_G
    0x01CD0099, // PATTERN_G Reset value, chop -++-
    0x01CE0000,
    0x01CF0000,
    0x01D00004,  // DATA_FORMAT_G
    0x01D20000,  // DECIMATE_G
    0x00100600, /* Enable slot G for RED LED data*/

/* Set the signal and dark sample data size of the slots from A to F to zero*/
    0x01100000,
    0x01300000,
    0x01500000,
    0x01700000,
    0x01900000,
    0x01B00000,
};

const uint32_t dcfg_org_4000_ir[] = {
    0x00090097,
    0x00078FFF,
    0x000B02F6,
    0x000C0012,
    0x000D4E20,//0x2710
    0x000E0000,
    0x000F0006,
    0x00148000,
    0x00202222,
    0x00210000,
    0x00220003,
    0x00230302,
    0x00240001,

/*Configuration for Time slot H*/
    0x01E04000, //CH2 enable
    0x01E141DA,
    0x01E20005,
    0x01E35002,
    0x01E403D2, //TIA GAIN, CH1 = CH2 = 50k
#ifdef DVT
    0x01E50500, //IR 2A
#elif PCBA
    0x01E50005,
#else
    0x01E50500,
#endif
    0x01E60000,
    0x01E70140,
    0x01E80000, // PERIOD_H
    0x01E90210, // LED_PULSE_H
    0x01EA0003, // INTEG_SETUP_H
    0x01EB1010, // INTEG_OS_H
    0x01EC0101, // MOD_PULSE_H
    0x01ED0099, // PATTERN_H Reset value, chop -++-
    0x01EE0000,
    0x01EF0000,
    0x01F00004,  // DATA_FORMAT_H
    0x01F20000,  // DECIMATE_H
    0x00100700, /* Enable slot H for IR LED data*/

/* Set the signal and dark sample data size of the slots from A to G to zero*/
    0x01100000,
    0x01300000,
    0x01500000,
    0x01700000,
    0x01900000,
    0x01B00000,
    0x01D00000,
};

const uint32_t dcfg_org_4000_b[] = {
    0x00090097,
    0x00078FFF,
    0x000B02F6,
    0x000C0012,
    0x000D4E20,//0x2710
    0x000E0000,
    0x000F0006,
    0x00148000,
    0x00202222,
    0x00210000,
    0x00220003,
    0x00230302,
    0x00240001,

/*Configuration for Time slot I*/
    0x02004000, //CH2 enable
    0x020141DA,
    0x02020005,
    0x02035002,
    0x020403D2, //TIA GAIN, CH1 = CH2 = 50k
    0x02050000,
    0x02060500, //Blue 4A
    0x02070140,
    0x02080000, // PERIOD_I
    0x02090210, // LED_PULSE_I
    0x020A0003, // INTEG_SETUP_I
    0x020B1010, // INTEG_OS_I
    0x020C0101, // MOD_PULSE_I
    0x020D0099, // PATTERN_I Reset value, chop -++-
    0x020E0000,
    0x020F0000,
    0x02100004,  // DATA_FORMAT_I
    0x02120000,  // DECIMATE_I
    0x00100800, /* Enable slot I for bLUE LED data*/

/* Set the signal and dark sample data size of the slots from A to H to zero*/
    0x01100000,
    0x01300000,
    0x01500000,
    0x01700000,
    0x01900000,
    0x01B00000,
    0x01D00000,
    0x01F00000,
};

const uint32_t dcfg_4000_temperature[] = {
    //0x000F8000,
    0x00090097,
    0x00078FFF,
    0x000B02F6,
    0x000C0012,
    0x000D2710,
    0x000E0000,
    0x000F0006,
    0x00148000,
    0x00200000,
    0x00210010,
    0x00220003,
    0x00230302,
    0x00240001,

    /*Moving thermistor sensor to slot D*/
#if 1
    0x01600000,   /*INPUT_RESISTOR set ot 500 ohm*/
    0x016141DA,   /*pre conditioning period of 8us, TIA+INT+BPF+ADC signal path selected for Slot A */
    0x01620010,   /*IN3 connected to CH1 of slot A*/
    0x01635A40,   /*Pre condition with TIA V_ref,VC2_pulse changes between active state and alternate state,
                    VC2_Active = V-ref and VC2_alternate = V_ref + 250mV*/
    0x01640281,   /* R_int = 400K, R_tia_c h1 = 100k, TIA_Vref = 0.8855  V, Vref_pulse = 0.8855 V; No v-ref pulsing */
    0x01650000,
    0x01660000,
    0x01670101,   /*single pulse and single repition is used*/
    0x01680000,   /*MOD_TYPE set to mode 0; normal mode*/
    //0x01690419,   /*LED pulse width and offset*/
    0x016A0003,   /*Integrator clock width set to 3us*/
    0x016B130F,   /*Integrator offset set to 15.595us*/
    0x016C0210,   /*Modulation pulse width set to 2us, with offset of 16us*/
    0x016E0000,   /*ADC CH1 offset set to 0*/
    0x016F0000,   /*ADC CH2 offset set to 0*/
    0x01700004,   /* signal size Data format set to 4 bytes;*/
#endif //slot A

#if 1
    /*Moving calibration resistor to slot E*/
    0x01800000,   /*INPUT_RESISTOR set ot 500 ohm*/
    0x018141DA,   /*pre conditioning period of 8us, TIA+INT+BPF+ADC signal path selected for Slot B */
    0x01820030,   /*IN4 connected to CH1 of slot B*/
    0x01835A40,   /*Pre condition with TIA V_ref,VC2_pulse changes between active state and alternate state,
                    VC2_Active = V-ref and VC2_alternate = V_ref + 250mV*/
    0x01840281,   /* R_int = 400K, R_tia_c h1 = 100k, TIA_Vref = 0.8855  V, Vref_pulse = 0.8855 V; No v-ref pulsing */
    0x01850000,
    0x01860000,
    0x01870101,   /*single pulse and single repition is used*/
    0x01880000,   /*MOD_TYPE set to mode 0; normal mode*/
    //0x01890419,   /*LED pulse width and offset*/
    0x018A0003,   /*Integrator clock width set to 3us*/
    0x018B130F,   /*Integrator offset set to 15.595us*/
    0x018C0210,   /*Modulation pulse width set to 2us, with offset of 16us*/
    0x018E0000,   /*ADC CH1 offset set to 0*/
    0x018F0000,   /*ADC CH2 offset set to 0*/
    0x01900004,   /*signal size data format set to 4 bytes; */
#endif //slot B
    0xFFFFFFFF,
};

#else
/* Default configurations for slotA and slotB */
/* TODO: slotC to slotL*/
const uint32_t dcfg_org_4000_g[] = {
      //0x000F8000,
    0x00090097,
    0x00078FFF,
    0x000B02F6,
    0x000C0012,
    0x000D4E20,//0x2710
    0x000E0000,
    0x000F0006,
    0x00148000,
    0x00202222,
    0x00210000,
    0x00220003,
    0x00230302,
    0x00240001,
/*Configuration for Time slot F*/
    0x01A00000,
    0x01A141DA,
    0x01A20003,
    0x01A35002,
    //0x01A4E2C1,
    0x01A50002,//0040
    0x01A60000,


    0x01A70101,
    //0x01A90000,//added
    0x01AB0E10,
    0x01AE1F00,
    0x01AF0000,
    //0x01100004,
    //0x01200010,
    0x00100500, /* Enable slot F for green LED data*/

/* Set the signal and dark sample data size of the slots from A to E to zero*/
    0x01100000,
    0x01300000,
    0x01500000,
    0x01700000,
    0x01900000,
    0x01B00004,
};
#endif //EVTBOARD

const uint32_t dcfg_org_4000_g_r_ir_b[] = {
    0x00090097,
    0x00078FFF,
    0x000B02F6,
    0x000C0012,
    0x000D4E20,//0x2710
    0x000E0000,
    0x000F0006,
    0x00148000,
    0x00202222,
    0x00210000,
    0x00220003,
    0x00230302,
    0x00240001,
    0x01310000, // DATA2_B
    0x01510000, // DATA2_C
    0x01710000, // DATA2_D
    0x01910000, // DATA2_E
    0x01B10000, // DATA2_F
    0x01D10000, // DATA2_G
/*Configuration for Time slot F*/
    0x01A04000, //CH2 enable
    0x01A141DA,
    0x01A20005,
    0x01A35002,
    0x01A403D2, //TIA GAIN, CH1 = CH2 = 50k
    0x01A50005, //Green - 1A
    0x01A60000,
    0x01A70140, // 64 pulse
    0x01A80000, // PERIOD_F
    0x01A90210, // LED_PULSE_F
    0x01AA0003, // INTEG_SETUP_F
    0x01AB1010, // INTEG_OS_F
    0x01AC0101, // MOD_PULSE_F
    0x01AD0099, // PATTERN_F Reset value, chop -++-
    0x01AE0000,
    0x01AF0000,
    0x01B00004,  // DATA_FORMAT_F
    0x01B20000,  // DECIMATE_F
    //0x00100500, /* Enable slot F for green LED data*/

/*Configuration for Time slot G*/
    0x01C04000, //CH2 enable
    0x01C141DA,
    0x01C20005,
    0x01C35002,
    0x01C403D2, //TIA GAIN, CH1 = CH2 = 50k
    0x01C50000,
    0x01C60005, //Red 3A
    0x01C70140,
    0x01C80000, // PERIOD_G
    0x01C90210, // LED_PULSE_G
    0x01CA0003, // INTEG_SETUP_G
    0x01CB1010, // INTEG_OS_G
    0x01CC0101, // MOD_PULSE_G
    0x01CD0099, // PATTERN_G Reset value, chop -++-
    0x01CE0000,
    0x01CF0000,
    0x01D00004,  // DATA_FORMAT_G
    0x01D20000,  // DECIMATE_G
    //0x00100600, /* Enable slot G for RED LED data*/

/*Configuration for Time slot H*/
    0x01E04000, //CH2 enable
    0x01E141DA,
    0x01E20005,
    0x01E35002,
    0x01E403D2, //TIA GAIN, CH1 = CH2 = 50k
    0x01E50500, //IR 2A
    0x01E60000,
    0x01E70140,
    0x01E80000, // PERIOD_H
    0x01E90210, // LED_PULSE_H
    0x01EA0003, // INTEG_SETUP_H
    0x01EB1010, // INTEG_OS_H
    0x01EC0101, // MOD_PULSE_H
    0x01ED0099, // PATTERN_H Reset value, chop -++-
    0x01EE0000,
    0x01EF0000,
    0x01F00004,  // DATA_FORMAT_H
    0x01F20000,  // DECIMATE_H
    //0x00100700, /* Enable slot H for IR LED data*/

/*Configuration for Time slot I*/
    0x02004000, //CH2 enable
    0x020141DA,
    0x02020005,
    0x02035002,
    0x020403D2, //TIA GAIN, CH1 = CH2 = 50k
    0x02050000,
    0x02060500, //Blue 4A
    0x02070140,
    0x02080000, // PERIOD_I
    0x02090210, // LED_PULSE_I
    0x020A0003, // INTEG_SETUP_I
    0x020B1010, // INTEG_OS_I
    0x020C0101, // MOD_PULSE_I
    0x020D0099, // PATTERN_I Reset value, chop -++-
    0x020E0000,
    0x020F0000,
    0x02100004,  // DATA_FORMAT_I
    0x02120000,  // DECIMATE_I
    0x00100800, /* Enable slot I for bLUE LED data*/

/* Set the signal and dark sample data size of the slots from A to E to zero*/
    0x01100000,
    0x01300000,
    0x01500000,
    0x01700000,
    0x01900000,
};

///////////////////////////////////////
typedef struct
{
/* Application related parameter */
  float ECGODR;                 /* Must be less than 1500Hz. Sample frequency in Hz, this value is used to set Sleep Wakeup Timer period */
}AppECG4kCfg_Type;

AppECG4kCfg_Type AppECG4kCfg =
{
  .ECGODR = 300.0,           /* 300.0 Hz*/
};
////////////////////////////////////////

/* Function to patch the dcfg with register address-value pairs that have


** changed in DVT2 board with ADPD4100
** @param None
*/
void patch_dvt2_adpd4100_reg()
{
  //Slot A
  Adpd400xDrvRegWrite(0x010B, 0x01A0);//INTEG_OS_A
  Adpd400xDrvRegWrite(0x0104, 0x02C1);//AFE_TRIM_A
  //Slot D
  Adpd400xDrvRegWrite(0x0161, 0x40DA);//TS_PATH_D
  Adpd400xDrvRegWrite(0x0164, 0x0281);//AFE_TRIM_D
  Adpd400xDrvRegWrite(0x016B, 0x01F3);//INTEG_OS_D
  //Slot E
  Adpd400xDrvRegWrite(0x0181, 0x40DA);//TS_PATH_E
  Adpd400xDrvRegWrite(0x0184, 0x0281);//AFE_TRIM_E
  Adpd400xDrvRegWrite(0x018B, 0x01F3);//INTEG_OS_E
  //Slot F
  Adpd400xDrvRegWrite(0x01A1, 0x40DA);//TS_PATH_F
  Adpd400xDrvRegWrite(0x01A4, 0x03D2);//AFE_TRIM_F
  Adpd400xDrvRegWrite(0x01AB, 0x0210);//INTEG_OS_F
  //Slot G
  Adpd400xDrvRegWrite(0x01C1, 0x40DA);//TS_PATH_G
  Adpd400xDrvRegWrite(0x01C4, 0x03D2);//AFE_TRIM_G
  Adpd400xDrvRegWrite(0x01CB, 0x0210);//INTEG_OS_G
  //Slot H
  Adpd400xDrvRegWrite(0x01E1, 0x40DA);//TS_PATH_H
  Adpd400xDrvRegWrite(0x01E4, 0x03D2);//AFE_TRIM_H
  Adpd400xDrvRegWrite(0x01EB, 0x0210);//INTEG_OS_H
  //Slot I
  Adpd400xDrvRegWrite(0x0201, 0x40DA);//TS_PATH_I
  Adpd400xDrvRegWrite(0x0204, 0x03D2);//AFE_TRIM_I
  Adpd400xDrvRegWrite(0x020B, 0x0210);//INTEG_OS_I
}

/**
* @brief    Load ADPD4000 Default configuration
* @param    device_id - Adpd4000 device index/type
* @retval   Status
*/
ADPD4000_DCFG_STATUS_t load_adpd4000_cfg(uint16_t device_id)
{
    ADPD4000_DCFG_STATUS_t cfg_status = ADPD4000_DCFG_STATUS_ERR;
    ADPD4000_DCB_STATUS_t ret;
#ifdef DCB
    bool dcb_cfg = false;
    dcb_cfg = adpd4000_get_dcb_present_flag();
    if(dcb_cfg == true)
    {
        //Load dcb Settings
        ret = load_adpd4000_dcb(device_id);
        cfg_status = (!ret) ? ADPD4000_DCFG_STATUS_OK : ADPD4000_DCFG_STATUS_ERR;
        //NRF_LOG_INFO("Load DCB dcfg");
    }
    else
    {
#endif
        // Load dcfg Settings
        cfg_status = load_adpd4000_dcfg(device_id);
        //NRF_LOG_INFO("Load Default f/w dcfg");

        //Check if its DVT2 chip, if so apply register patches
        if( dvt2 )
        {
          patch_dvt2_adpd4100_reg();
        }

#ifdef DCB
    }
#endif

    return cfg_status;
}

#ifdef EVTBOARD
ADPD4000_DCFG_STATUS_t load_temperature_dcfg ()
{
  Adpd400xDrvSoftReset();
  Adpd400xDrvOpenDriver();
  memcpy(&g_current_dcfg[0], &dcfg_4000_temperature[0], sizeof(dcfg_4000_temperature));
  if (write_adpd4000_dcfg(&g_current_dcfg[0]) != ADPD4000_DCFG_STATUS_OK) {
    return ADPD4000_DCFG_STATUS_ERR;
  }
  return ADPD4000_DCFG_STATUS_OK;
}
#endif

/**
* @brief    Load ADPD4000 Default configuration
* @param    device_id - Adpd4000 device index/type
* @retval   Status
*/
ADPD4000_DCFG_STATUS_t load_adpd4000_dcfg(uint16_t device_id) {
  if (device_id == 0) {
    Adpd400xDrvOpenDriver();
    return ADPD4000_DCFG_STATUS_OK;
  }
  if (device_id != 0) {
    stage_adpd4000_dcfg(&device_id);
  }
  Adpd400xDrvSoftReset();
  //Adpd400xDrvOpenDriver();
  if (write_adpd4000_dcfg(&g_current_dcfg[0]) != ADPD4000_DCFG_STATUS_OK) {
    return ADPD4000_DCFG_STATUS_ERR;
  }
  return ADPD4000_DCFG_STATUS_OK;
}

/**
* @brief    Gets the entire ADPD current device configuration
* @param    pDcfg - pointer to dcfg register/value pairs
* @retval   Status
*/
ADPD4000_DCFG_STATUS_t read_adpd4000_dcfg(uint32_t *p_dcfg, uint16_t *p_dcfg_size) {
  uint16_t reg_addr;
  uint16_t reg_data;
  if (p_dcfg == NULL) {
    return ADPD4000_DCFG_STATUS_NULL_PTR;
  }

  for (int i = 0; g_current_dcfg[i]!= 0xFFFFFFFF; i++) {
    reg_addr = (uint16_t) (g_current_dcfg[i] >> 16);
    if (Adpd400xDrvRegRead(reg_addr, &reg_data) != ADPD400xDrv_SUCCESS) {
      return ADPD4000_DCFG_STATUS_ERR;
    }
    *p_dcfg = (reg_addr << 16) | reg_data;
    p_dcfg++;
    *p_dcfg_size = i + 1;
  }
  return ADPD4000_DCFG_STATUS_OK;
}


/**
* @brief    Write a dcfg to device
* @param    p_dcfg - pointer to Dcfg array
* @retval   Status
*/
ADPD4000_DCFG_STATUS_t write_adpd4000_dcfg(uint32_t *p_dcfg) {
  uint16_t reg_addr;
  uint16_t reg_data;

  if (p_dcfg == NULL) {
    return ADPD4000_DCFG_STATUS_NULL_PTR;
  }
  // clear FIFO and IRQs
  Adpd400xDrvSetOperationMode(ADPD400xDrv_MODE_IDLE);
  //Adpd400xDrvSetOperationMode(ADPD400xDrv_MODE_PAUSE);

  for (int i = 0; p_dcfg[i]!= 0xFFFFFFFF; i++) {
    reg_addr = (uint16_t) (p_dcfg[i] >> 16);
    reg_data = (uint16_t)(p_dcfg[i]);

    if (Adpd400xDrvRegWrite(reg_addr, reg_data) != ADPD400xDrv_SUCCESS) {
      //Adpd400xDrvSetOperationMode(ADPD400xDrv_MODE_IDLE);
      return ADPD4000_DCFG_STATUS_ERR;
    }
  }
  //Adpd400xDrvSetOperationMode(ADPD400xDrv_MODE_IDLE);
  return ADPD4000_DCFG_STATUS_OK;
}


static uint8_t g_used_slots[SLOT_NUM] = {'\0'};
static uint8_t gNumUsedSlots = 0;
static uint8_t gDcfgMaxSlot = 0;
/***********************************REFERENCE TABLE FOR SLOT-ID and APP-ID ********************************************
        ----------------------------------------   ------------------------------  ------------------------------
        | App-ID |          Apps               |   | Slot- ID  |  ADPD4k_SLOTS  |  | Slot- ID   |  ADPD4k_SLOTS  |
        ----------------------------------------   ------------------------------   ------------------------
        |   0    |          ECG4K              |   |    1      |       A        |   |    9       |     I    |
         ---------------------------------------   ------------------------------   ------------------------
        |   1    |          PPG                |   |    2      |       B        |   |    10      |     J    |
        ----------------------------------------   ------------------------------   ------------------------
        |   2    |    Temp. (Thermistor)       |   |    3      |       C        |   |    11      |     K    |
        ------------------------- --------------   ------------------------------   ------------------------
        |   3    |Temp. (Calibration Resistor) |   |    4      |       D        |   |    12      |     L    |
        ----------------------------------------   ------------------------------   --------------------------
        |   4    |          ADPD4K_G           |   |    5      |       E        |
        ----------------------------------------   ------------------------------
        |   5    |          ADPD4K_R           |   |    6      |       F        |
        ----------------------------------------   ------------------------------
        |   6    |          ADPD4K_IR          |   |    7      |       G        |
        ----------------------------------------   ------------------------------
        |   7    |          ADPD4K_B           |   |    8      |       H        |
        ----------------------------------------  -------------------------------
  *****************************************************************************************************************/
/*!
 ****************************************************************************
 * @brief get the ADPD4k Dcfg based on selected Slot and Application
 *
 * @param[in]          slot id : specifies slot
 * @param[in]          app id  : specifies application
 * @param[in]          index   : iterates over 0 to num_slots
 * @param[in]          num slots: total number of slots required in dcfg
 *
 * @return              status
 *****************************************************************************/
ADPD4000_DCFG_STATUS_t get_adpd4k_dcfg(uint16_t slot_id, uint16_t app_id, uint8_t index, uint8_t num_slots)
{
  uint8_t i = 0, j = 0;
  slot_id = slot_id - 1;/* referring slot id from 0 to 11 */

  if(index == 0)
  {
      memset(&g_created_dcfg, 0xff, sizeof(g_created_dcfg));
      memcpy(&g_created_dcfg, &general_dcfg_4000_g, sizeof(general_dcfg_4000_g));
      gDcfgMaxSlot = slot_id;
  }

  for(i = 0; i < SLOT_DCFG_SIZE ;i++)
  {
   slot_general_dcfg[i] = ((slotA_default_dcfg_4000[i] >> 16) + (ADPD400x_SLOT_BASE_ADDR_DIFF * slot_id)) << 16;
  }

  uint8_t idx = GENERAL_DCFG_SIZE + SLOT_DCFG_SIZE*index;
  for(i = idx; i < (GENERAL_DCFG_SIZE + SLOT_DCFG_SIZE*(index+1)) ;i++)
  {
    g_created_dcfg[i] = slot_general_dcfg[abs(idx- i)] | app_dcfg_list[app_id][abs(idx - i)];
  }
  g_used_slots[gNumUsedSlots++] = slot_id;
  if(slot_id > gDcfgMaxSlot)
  {
    gDcfgMaxSlot = slot_id;/* store the max slot in dcfg to set the opmode */
  }

  if(index == num_slots - 1)
  {
   /* Enable Slots */
   g_created_dcfg[GENERAL_DCFG_SIZE-1] = ((ADPD400x_REG_OPMODE << 16) | (gDcfgMaxSlot << 8));
   check_dcfg_created = true;

   /* set signal size = 0 for slots that are enabled but not in use */
   idx = (GENERAL_DCFG_SIZE + SLOT_DCFG_SIZE*(index+1));
   uint16_t reg_base = 0;
   for(i = 0; i <= slot_id ;i++)
   {
     for(j = 0; j < gNumUsedSlots; j++)
     {
        if(i == g_used_slots[j])
        {
          break;
        }
     }
     if(j == gNumUsedSlots)
     {
       reg_base = i * ADPD400x_SLOT_BASE_ADDR_DIFF;
       g_created_dcfg[idx++] = (ADPD400x_REG_DATA1_A + reg_base) << 16;
     }
   }

   memset(&g_used_slots, '\0', sizeof(g_used_slots));
   gNumUsedSlots = 0;
  }

#ifdef SLOT_SELECT
  //set Temp. Slot
  if(app_id == 2)
  {
    g_therm_slot_en_bit_mask = (1 << slot_id) | ( 1 << (slot_id+1));
    check_temp_slot_set = true;
  }

  //set PPG Slot
  if(app_id == 1)
  {
    Ppg_Slot = 1 << slot_id;
    check_ppg_slot_set = true;
  }

  if(app_id == 4) //adpd4000_g
  {
    gn_led_slot_g |= 1 << slot_id;
  }

  if(app_id == 5) //adpd4000_r
  {
    gn_led_slot_r |= 1 << slot_id;
  }

  if(app_id == 6) //adpd4000_ir
  {
    gn_led_slot_ir |= 1 << slot_id;
  }

  if(app_id == 7) //adpd4000_b
  {
    gn_led_slot_b |= 1 << slot_id;
  }
#endif
 return ADPD4000_DCFG_STATUS_OK;
}

/**
* @brief    Stage default DCFG to buffer
* @param    p_device_id - pointer to a device ID
* @retval   Success/Error
*/
ADPD4000_DCFG_STATUS_t stage_adpd4000_dcfg(uint16_t *p_device_id) {
  if (p_device_id == NULL) {
    return ADPD4000_DCFG_STATUS_NULL_PTR;
  }

  adpd4000_fw_dcfg_clear();

  if (*p_device_id == 0) {
    // If we didn't receive a device ID, try to read it from the device
    // AdpdClDrvEfuseModuleTypeRead(p_device_id);
  }
  switch (*p_device_id) {
  case M2M2_SENSOR_ADPD4000_DEVICE_4000_G:
#ifdef DCB
    if( adpd4000_get_dcb_present_flag() ) {
        uint16_t dcb_sz = (MAXADPD4000DCBSIZE*MAX_ADPD4000_DCB_PKTS);
        adpd4000_dcb_dcfg_clear();
	if(read_adpd4000_dcb(g_current_dcb, &dcb_sz) == ADPD4000_DCB_STATUS_OK)
        {
        memcpy(&g_current_dcfg[0], &g_current_dcb[0], sizeof(g_current_dcb));
        }
        else
        {
        return ADPD4000_DCFG_STATUS_ERR;
        }
    }
    else
    {
        if(check_dcfg_created == true)
          {
            memcpy(&g_current_dcfg[0], &g_created_dcfg[0], sizeof(g_created_dcfg));
            check_dcfg_created = false;
          }
        else
          {
            memcpy(&g_current_dcfg[0], &dcfg_org_4000_g[0], sizeof(dcfg_org_4000_g));
          }
    }
#else
    memcpy(&g_current_dcfg[0], &dcfg_org_4000_g[0], sizeof(dcfg_org_4000_g));
#endif
    *p_device_id = 4000;
    break;
#ifdef EVTBOARD
  case M2M2_SENSOR_ADPD4000_DEVICE_4000_R:
    memcpy(&g_current_dcfg[0], &dcfg_org_4000_r[0], sizeof(dcfg_org_4000_r));
    *p_device_id = 4000;
    break;
  case M2M2_SENSOR_ADPD4000_DEVICE_4000_IR:
    memcpy(&g_current_dcfg[0], &dcfg_org_4000_ir[0], sizeof(dcfg_org_4000_ir));
    *p_device_id = 4000;
    break;
  case M2M2_SENSOR_ADPD4000_DEVICE_4000_B:
    memcpy(&g_current_dcfg[0], &dcfg_org_4000_b[0], sizeof(dcfg_org_4000_b));
    *p_device_id = 4000;
    break;
  case M2M2_SENSOR_ADPD4000_DEVICE_4000_G_R_IR_B:
    memcpy(&g_current_dcfg[0], &dcfg_org_4000_g_r_ir_b[0], sizeof(dcfg_org_4000_g_r_ir_b));
    *p_device_id = 4000;
    break;
#endif
  default:
  // Fallthrough to a default dcfg if we didn't get one and couldn't read it from the device
//  case M2M2_SENSOR_ADPD4000_DEVICE_4000_G:
    memcpy(&g_current_dcfg[0], &dcfg_org_4000_g[0], sizeof(dcfg_org_4000_g));
    *p_device_id = 4000;
    break;
  }
  return ADPD4000_DCFG_STATUS_OK;
}

void adpd4000_fw_dcfg_clear(void) {
  memset(&g_current_dcfg[0], 0xFF, sizeof(g_current_dcfg));
}

#ifdef DCB
void adpd4000_dcb_dcfg_clear(void) {
  memset(&g_current_dcb[0], 0xFF, sizeof(g_current_dcb));
}

// ================== ADPD_DCB Section ====================== //

/**
* @brief    Load ADPD4000 Default DCB configuration
* @param    device_id - Adpd4000 device index/type
* @retval   Status
*/
ADPD4000_DCB_STATUS_t load_adpd4000_dcb(uint16_t device_id)
{
    if (device_id == 0)
    {
        Adpd400xDrvOpenDriver();
        return ADPD4000_DCB_STATUS_OK;
    }
    if (device_id != 0)
    {
        stage_adpd4000_dcfg(&device_id);
    }

    Adpd400xDrvSoftReset();
    //Adpd400xDrvOpenDriver();

    if(write_adpd4000_dcfg(&g_current_dcfg[0]) != ADPD4000_DCFG_STATUS_OK)
    {
         NRF_LOG_INFO("Write adpd4000_dcfg Error");
         return ADPD4000_DCB_STATUS_ERR;
    }

    return ADPD4000_DCB_STATUS_OK;
}

/**@brief   Gets the entire ADPD DCB configuration written in flash
 *
 * @param adpd4000_dcb_data - pointer to dcb struct variable,
 * @param read_size: size of adpd4000_dcb_data array filled from FDS(length in Double Word (32-bits))
 * @return return value of type ADPD4000_DCB_STATUS_t
 */
ADPD4000_DCB_STATUS_t read_adpd4000_dcb(uint32_t *adpd4000_dcb_data, uint16_t* read_size)
{
    ADPD4000_DCB_STATUS_t dcb_status = ADPD4000_DCB_STATUS_ERR;

    if(adi_dcb_read_from_fds(ADI_DCB_ADPD4000_BLOCK_IDX, adpd4000_dcb_data, read_size) == DEF_OK)
    {
        dcb_status = ADPD4000_DCB_STATUS_OK;
    }
    return dcb_status;
}

/**@brief   Sets the entire ADPD DCB configuration in flash
 *
 * @param adpd4000_dcb_data - pointer to dcb struct variable,
 * @param in_size: size of adpd4000_dcb_data array(length in Double Word (32-bits))
 * @return return value of type ADPD4000_DCB_STATUS_t
 */
ADPD4000_DCB_STATUS_t write_adpd4000_dcb(uint32_t *adpd4000_dcb_data, uint16_t in_size)
{
    ADPD4000_DCB_STATUS_t dcb_status = ADPD4000_DCB_STATUS_ERR;

    if(adi_dcb_write_to_fds(ADI_DCB_ADPD4000_BLOCK_IDX, adpd4000_dcb_data, in_size) == DEF_OK)
    {
        dcb_status = ADPD4000_DCB_STATUS_OK;
    }

    return dcb_status;
}

/**
* @brief    Delete the entire ADPD4000 DCB configuration in flash
* @param    void
* @retval   Status
*/
ADPD4000_DCB_STATUS_t delete_adpd4000_dcb(void)
{
    ADPD4000_DCB_STATUS_t dcb_status = ADPD4000_DCB_STATUS_ERR;

    if(adi_dcb_delete_fds_settings(ADI_DCB_ADPD4000_BLOCK_IDX) == DEF_OK)
    {
        adpd4000_fw_dcfg_clear();
        adpd4000_dcb_dcfg_clear();
        dcb_status = ADPD4000_DCB_STATUS_OK;
    }

    return dcb_status;
}

void adpd4000_set_dcb_present_flag(bool set_flag)
{
    g_adpd4000_dcb_present = set_flag;
    NRF_LOG_INFO("Setting..ADPD4000 DCB present: %s",(g_adpd4000_dcb_present == true ? "TRUE" : "FALSE"));
}

bool adpd4000_get_dcb_present_flag(void)
{
    NRF_LOG_INFO("ADPD4000 DCB present: %s", (g_adpd4000_dcb_present == true ? "TRUE" : "FALSE"));
    return g_adpd4000_dcb_present;
}

void adpd4000_update_dcb_present_flag(void)
{
    g_adpd4000_dcb_present = adi_dcb_check_fds_entry(ADI_DCB_ADPD4000_BLOCK_IDX);
    NRF_LOG_INFO("Updated. ADPD4000 DCB present: %s", (g_adpd4000_dcb_present == true ? "TRUE" : "FALSE"));
}
#endif

/**@brief  Function to control the GPIO, which enables/disables the DG2502 switch
 *         that connects ECG electrodes to ADPD4000 - ecg application
 *
 * @param  sw_enable: 1/0 to enable/disable switch
 * @return None
 */
void DG2502_SW_control_ADPD4000(uint8_t sw_enable)
{
    if(sw_enable)
        Adpd400xDrvRegWrite(ADPD400x_REG_GPIO23, 1);//4K_SW_EN_1V8
    else
        Adpd400xDrvRegWrite(ADPD400x_REG_GPIO23,0);//4K_SW_EN_1V8
}

////////////////////////////////////////////////////////
/* Function to set the Sampling Frequency for the adpd4k*
** @param value-> odr value for the sampling frequency to be set for adpd4000 (in Hz)
*/

ADPD4000_DCFG_STATUS_t Set_adpd4000_SamplingFreq(uint16_t odr) {

  ADPD4000_DCFG_STATUS_t sts = ADPD4000_DCFG_STATUS_OK;

  uint16_t fs_reg_addr = ADPD400x_REG_TS_FREQ; //0X000D-> register address for setting sampling frequency in adpd4000
  uint16_t fs_reg_data;
  uint16_t temp16;
  uint32_t lfOSC;

  Adpd400xDrvRegRead(ADPD400x_REG_SYS_CTL, &temp16);
  temp16 &= BITM_SYS_CTL_LFOSC_SEL;
  temp16 >>= BITP_SYS_CTL_LFOSC_SEL;
  if (temp16 == 1)
    lfOSC = 1000000;  // 1M clock
  else
    lfOSC = 32000;    // 32k clock

  fs_reg_data = lfOSC / odr;  //samplingFrequency = lfOSC/ODR

  if (Adpd400xDrvRegWrite(fs_reg_addr, fs_reg_data) != ADPD400xDrv_SUCCESS)
    {
            sts = ADPD4000_DCFG_STATUS_ERR;
    }
  else
    {
            sts = ADPD4000_DCFG_STATUS_OK;
    }
  return sts;
}

/* Function to set the ecg lcfg values for adpd4k
** @param field-> refers to each entry in ecg lcfg (field = ECG4k_LCFG_FS for ecgODR)
** @param value-> odr value for the sampling frequency to be set in the lcfg (in Hz)
*/

ADPD4000_DCFG_STATUS_t Ecg4kSetLCFG(uint8_t field, uint16_t value) {

  AppECG4kCfg_Type *pCfg = &AppECG4kCfg;

  if(field < ECG4k_LCFG_MAX){
    switch(field){

    case ECG4k_LCFG_FS:
        pCfg->ECGODR = value;
        Set_adpd4000_SamplingFreq(pCfg->ECGODR);
       break;

    default:
        break;
    }
    return ADPD4000_DCFG_STATUS_OK;
  }
  return ADPD4000_DCFG_STATUS_ERR;
}


/* Function to get the ecg lcfg values for adpd4k
** @param field-> refers to each entry in ecg lcfg (field = ECG4k_LCFG_FS for ecgODR)
** @param value-> odr value to be read from the lcfg (in Hz)
*/
ADPD4000_DCFG_STATUS_t Ecg4kgGetLCFG(uint8_t field, uint16_t *value) {

  AppECG4kCfg_Type *pCfg = &AppECG4kCfg;
  if(field < ECG4k_LCFG_MAX){
    switch(field){

    case ECG4k_LCFG_FS:
      *value = pCfg->ECGODR;
      break;

    default:
      break;
   }
    return ADPD4000_DCFG_STATUS_OK;
  }
  return ADPD4000_DCFG_STATUS_ERR;
}
