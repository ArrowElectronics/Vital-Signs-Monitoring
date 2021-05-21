/**
******************************************************************************
* @file     adp5360.h
* @author   
* @version  
* @date     
* @brief    Include file for the ADP5360 device driver
******************************************************************************
* @attention
******************************************************************************
*/
/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2019 Analog Devices Inc.                                 *
* All rights reserved.                                                        *
*                                                                             *
* This source code is intended for the recipient only under the guidelines of *
* the non-disclosure agreement with Analog Devices Inc.                       *
*                                                                             *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,    *
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE *
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      *
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING     *
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER         *
* DEALINGS IN THE SOFTWARE.                                                   *
*                                                                             *
*                                                                             *
* This software is intended for use with the ADPD and derivative parts     *
* only                                                                        *
*                                                                             *
******************************************************************************/
#ifndef __ADP5360_H
#define __ADP5360_H

#include <stdint.h>
#include "sdk_common.h"
#include "post_office.h"

#define ADP5360_DISABLE (0)                        /*disable */
#define ADP5360_ENABLE  (1)                      /*enable*/

/* ============================== REGISTER MAP ===============================*/
#define ADP5360_REG_DEV_ID                0x00
#define ADP5360_REG_REV_ID                0x01
#define ADP5360_REG_VBUS_ILIM             0x02
#define ADP5360_REG_TERM_SET              0x03
#define ADP5360_REG_CURRENT_SET           0x04
#define ADP5360_REG_V_THRESHOLD           0x05
#define ADP5360_REG_TIMER_SETT            0x06
#define ADP5360_REG_FUNC_SETT             0x07
#define ADP5360_REG_CHG_STATUS1           0x08
#define ADP5360_REG_CHG_STATUS2           0x09
#define ADP5360_REG_BATT_THERM_CTRL       0x0A
#define ADP5360_REG_THERMISTOR_60C        0x0B
#define ADP5360_REG_THERMISTOR_45C        0x0C
#define ADP5360_REG_THERMISTOR_10C        0x0D
#define ADP5360_REG_THERMISTOR_0C         0x0E
#define ADP5360_REG_THR_VOLTAGE_LOW       0x0F
#define ADP5360_REG_THR_VOLTAGE_HIGH      0x10
#define ADP5360_REG_BAT_PROC_CONTROL      0x11
#define ADP5360_REG_BAT_DISCHG_UV_SETT    0x12
#define ADP5360_REG_BAT_DISCHG_OC_SETT    0x13
#define ADP5360_REG_BAT_CHG_OV_SETT       0x14
#define ADP5360_REG_BAT_CHG_OC_SETT       0x15
#define ADP5360_REG_V_SOC_0               0x16
#define ADP5360_REG_V_SOC_5               0x17
#define ADP5360_REG_V_SOC_11              0x18
#define ADP5360_REG_V_SOC_19              0x19
#define ADP5360_REG_V_SOC_28              0x1A
#define ADP5360_REG_V_SOC_41              0x1B
#define ADP5360_REG_V_SOC_55              0x1C
#define ADP5360_REG_V_SOC_69              0x1D
#define ADP5360_REG_V_SOC_84              0x1E
#define ADP5360_REG_V_SOC_100             0x1F
#define ADP5360_REG_BAT_CAP               0x20
#define ADP5360_REG_BAT_SOC               0x21
#define ADP5360_REG_BAT_SOCACM_CTL        0x22
#define ADP5360_REG_BAT_SOCACM_H          0x23
#define ADP5360_REG_BAT_SOCACM_L          0x24
#define ADP5360_REG_BAT_READ_H            0x25
#define ADP5360_REG_BAT_READ_L            0x26
#define ADP5360_REG_FUEL_GAUGE_MODE       0x27
#define ADP5360_REG_SOC_RESET             0x28
#define ADP5360_REG_BUCK_CFG              0x29
#define ADP5360_REG_BUCK_OUT_VOL_SETT     0x2A
#define ADP5360_REG_BUCK_B_CFG            0x2B
#define ADP5360_REG_BUCK_B_OUT_VOL_SETT   0x2C
#define ADP5360_REG_SUPERVISORY_SETT      0x2D
#define ADP5360_REG_FAULTS                0x2E
#define ADP5360_REG_PGOOD_STATUS          0x2F
#define ADP5360_REG_PGOOD1_MASK           0x30
#define ADP5360_REG_PGOOD2_MASK           0x31
#define ADP5360_REG_INT_EN1               0x32
#define ADP5360_REG_INT_EN2               0x33
#define ADP5360_REG_INT_FLAG1             0x34
#define ADP5360_REG_INT_FLAG2             0x35
#define ADP5360_REG_SHIPMODE              0x36
//#define ADP5360_REG_DEFAULT_SET           0x37

/******************************************************************************/


//typedef enum {
//    ADP5360_SUCCESS,                          /*!< Successfully Completed */
//    ADP5360_ERR_INVALID_PARAMETER,            /*!< Input parameter to the function is invalid */
//    ADP5360_ERR_I2C                           /*!< Error during I2C transaction */
//};
typedef uint32_t ADP5360_RESULT;
#define ADP5360_SUCCESS 0
#define ADP5360_ERR_I2C 1
/* ============================= VBUS SETTINGS ==============================*/

#define ADP5360_mV_TO_VADPICHG(mV) ((uint8_t)((mV-4400)/100)+2)

typedef enum {
    ADP5360_VBUS_ILIM_50mA = 0,
    ADP5360_VBUS_ILIM_100mA,
    ADP5360_VBUS_ILIM_150mA,
    ADP5360_VBUS_ILIM_200mA,
    ADP5360_VBUS_ILIM_250mA,
    ADP5360_VBUS_ILIM_300mA,
    ADP5360_VBUS_ILIM_400mA,
    ADP5360_VBUS_ILIM_500mA,
    ADP5360_VBUS_ILIM_MAX_ENTRIES
} ADP5360_VBUS_ILIM;

typedef struct{
    uint8_t ilim:3;
    uint8_t vsystem:1;//0=Vtrm+100mV,1:5V.
    uint8_t not_used:1;//
    uint8_t vadpichg:3;//    
}ADP5360_CHG_VBUS_ILIM;


/* ============================= BATTERY SETTINGS ==============================*/
#define ADP5360_mV_TO_VTRM(mV) (((mV-3560)/20))//3.56V~4.66V,interval:0.02V
#define ADP5360_mA_TO_IEND(mA)  (((mA) >= 7.5)?((((mA))-7.5)/5+2):(1))//5mA.7.5mA~32.5mA,interval:5mA
#define ADP5360_mA_TO_ICHG(mA) ((((mA)-10)/10))//10mA~320mA,interval:10mA
#define ADP5360_mV_TO_VWEAK(mV) (((mV-2700)/100))//2.7V~3.4V,interval:0.1V


typedef struct{
	struct{
	    uint8_t itrk_dead:2;//00 = 1mA.01 = 2.5mA.10 = 5 mA.11 = 10 mA
		uint8_t vtrm:6;//Termination voltage programming bus		
	}chg_termination_set;
    struct{
        uint8_t ichg:5;//Fast charge current programming bus
        uint8_t iend:3;//Termination current programming bus		
	}chg_current_set;
	struct{
	    uint8_t vweak:3;//Weak battery voltage rising threshold
	    uint8_t vtrk_dead:2;//00 = 2.0 V.01 = 2.5 V.10 = 2.6 V.11 = 2.9 V.
	    uint8_t vrch:2;//01 = 120 mV.10 = 180 mV.11 = 240 mV
		uint8_t dis_rch:1;//0 = recharge enable.1 = recharge disable.
	}chg_voltage_threshold;
}ADP5360_BATTERY_THRESHOLDS;


/* ========================== TIMER SETTINGS ============================*/
typedef struct {
    uint8_t chg_tmr_period:2;//00 = 15 min/150 min.01 = 30 min/300 min.10 = 45 min/450 min.11 = 60 min/600 min.
    uint8_t chg_timer_en:1;//When high, the trickle/fast charge timer is enabled
    uint8_t tend_en:1;
    uint8_t not_used:4;
	
}ADP5360_CHG_TIMER_SET;

/* ========================== FUNCTIONAL SETTINGS ============================*/
typedef struct {
    uint8_t chg_en:1;//When low, the charging is disabled
    uint8_t adpichg_en:1;//When high, VBUS adaptive current limit function is enabled during charge.
    uint8_t eoc_en:1;//When high, end of charge is allowed.
    uint8_t ldo_en:1;//When low the charge LDO is disabled. When high the charge LDO is enabled
    uint8_t off_isofet:1;//When high, the ISOFET is forced turn off and VSYS shut down even when only battery present
    uint8_t not_used:1;
    uint8_t ilim_jeita_cool:1;//0: Approximately 50% of programmed charge current.1: Approximately 10%//Select battery charging current when in temperature cool
    uint8_t jeita_en:1;//When low, this bit disables the JEITA Li-Ion temperature battery charging specification.	
} ADP5360_CHG_FUNC_SET;


/* ============================= STATUS REGISTERS ==============================*/

typedef enum {
    ADP5360_THERM_OFF = 0,
    ADP5360_THERM_COLD,
    ADP5360_THERM_COOL,
    ADP5360_THERM_WARM,
    ADP5360_THERM_HOT,
    ADP5360_THERM_OK = 7
} ADP5360_THERM_STATUS;

typedef enum {
    ADP5360_CHG_OFF = 0,
    ADP5360_CHG_TRICKLE,
    ADP5360_CHG_FAST_CC,
    ADP5360_CHG_FAST_CV,
    ADP5360_CHG_COMPLETE,
    ADP5360_CHG_LDO_MODE,
    ADP5360_CHG_TIMER_EXPIRED,
    ADP5360_CHG_BAT_DETECTION
} ADP5360_CHARGER_STATUS;

typedef enum {
    ADP5360_BATT_NORMAL = 0,
    ADP5360_BATT_NO_BAT,
    ADP5360_BATT_RANGE1, //BAT_SNS < Vtrk when in charge
    ADP5360_BATT_RANGE2, //Vtrk < BAT_SNS < Vweak when in charge
    ADP5360_BATT_RANGE3, //BAT_SNS >= Vweak when in charge
    ADP5360_BATT_RANGE4  //BAT_SNS < Vovchg when in charge
} ADP5360_BAT_CHG_STATUS;

typedef struct {
    ADP5360_CHARGER_STATUS chager_status:3;//refer ADP5360_CHG_STATUS
    uint8_t not_used:2;
    uint8_t vbus_ilim:1;/*When high, this bit indicates that the current into a VBUSx pin is
                            limited by the high voltage blocking FET and the charger is not
                            running at the full programmed ICHG*/
    uint8_t adpichg:1;//When high, this bit indicates that the adaptive input current limit active and VBUS voltage regulator to V_ADPICHG
    uint8_t vubs_ov:1;//When high, this bit indicate that the VBUS voltage over than threshold	
} ADP5360_CHG_STATUS1;

typedef struct {
    ADP5360_BAT_CHG_STATUS bat_chg_status:3;//refer ADP5360_CHG_STATUS
    uint8_t bat_uv_stat:1;//1 = battery under voltage protection
    uint8_t bat_ov_stat:1;//1 = Battery over voltage protection
    uint8_t thr_status:3;//refer ADP5360_THERM_STATUS
} ADP5360_CHG_STATUS2;


/* ============================= THERMAL SETTINGS ==============================*/

typedef enum {
    ADP5360_ITHR_60UA = 0,
    ADP5360_ITHR_12UA,
    ADP5360_ITHR_6UA,
    ADP5360_ITHR_MAX_ENTRIES
}ADP5360_ITHR;


typedef struct {
    struct{
        uint8_t thr_en:1;//When high, the THR current source is enabled even when the voltage at the VBUS pins is below VVBUS_OK.
        uint8_t not_used:5;
        uint8_t ithr:2;//refer ADP5360_ITHR
    }thermistor_ctrl;
    uint8_t temp_high_60;
    uint8_t temp_high_45;
    uint8_t temp_low_10;
    uint8_t temp_low_0;
} ADP5360_THERMAL_CTRL;

/* ======================set battery protection=============================*/
#define ADP5360_mV_TO_UV_DISCH(mV) (((mV-2050)/50))//2.05V~2.80V,interval:0.05V
#define ADP5360_mA_TO_OC_DISCH(mA)  (((mA) > 200)?(((mA)-200)/100+3):(((mA)-50)/50))//50~200mA,interval:50mA.200~600mA,interval:100mA
#define ADP5360_mV_TO_OV_CHG(mV) (((mV-3550)/50))//3.55~4.8V,interval:0.05V

typedef enum {
    ADP5360_DGT_OC_DISCH_0_5MS = 1,
    ADP5360_DGT_OC_DISCH_1MS,
    ADP5360_DGT_OC_DISCH_5MS,
    ADP5360_DGT_OC_DISCH_10MS,
    ADP5360_DGT_OC_DISCH_20MS,
    ADP5360_DGT_OC_DISCH_50MS,
    ADP5360_DGT_OC_DISCH_100MS
}ADP5360_DGT_OC_DISCH;

typedef enum {
    ADP5360_OC_CHG_25MA = 0,
    ADP5360_OC_CHG_50MA,
    ADP5360_OC_CHG_100MA,
    ADP5360_OC_CHG_150MA,
    ADP5360_OC_CHG_200MA,
    ADP5360_OC_CHG_250MA,
    ADP5360_OC_CHG_300MA,
    ADP5360_OC_CHG_400MA
}ADP5360_OC_CHG;

typedef struct {
    struct{
        uint8_t batpro_en:1;//When low, the battery protection function is disabled
        uint8_t chglb_en:1;//When low, the battery charge is not allowed when battery UV protection
        uint8_t oc_chg_hiccup:1;//Battery over charge current protection .0 = Latch up,1 = Hiccup
        uint8_t oc_dis_hiccup:1;//Battery discharge over current protection .0 = Latch up,1 = Hiccup
        uint8_t isofet_ovchg:1;//When low, ISOFET turn on when battery charge over voltage protetction
        uint8_t not_used:3;      
    }bat_proc_ctrl;
    struct{
        uint8_t dgt_uv_disch:2;//00 = 30 mS.01 = 60 mS.10 = 120 mS.11 = 240 mS.
        uint8_t hys_uv_disch:2;//00 = 2%.01 = 4%.10 = 6%.11 = 8% UV_DISCH voltage threshold
        uint8_t uv_disch:4;//refer ADP5360_mV_TO_UV_DISCH     
    }bat_dischg_uv_set;
    struct{
        uint8_t not_use2:1;
        uint8_t dgt_oc_disch:3;//refer ADP5360_DGT_OC_DISCH
        uint8_t not_use1:1;
        uint8_t oc_disch:3;//refer ADP5360_mA_TO_OC_DISCH  
    }bat_dischg_oc_set;
    struct{
        uint8_t dgt_ov_chg:1;//0 = 0.5s.1 = 1s Battery over voltage protection deglitch time.
        uint8_t hys_ov_chg:2;//00 = 2%.01 = 4%.10 = 6%.11 = 8% Battery over voltage protection for charge hysteresis.
        uint8_t ov_chg:5;//refer ADP5360_mV_TO_OV_CHG
    }bat_chg_ov_set;
    struct{
        uint8_t not_uset:3;
        uint8_t dgt_oc_chg:2;//00 = 5 mS.01 = 10 mS.10 = 20 mS.11 = 40 mS.Battery charge over current protection deglitch time setting
        uint8_t oc_chg:3;//refer ADP5360_OC_CHG 
    }bat_chg_oc_set;
  
} ADP5360_BAT_PROTECTION;



/* ============================= FUEL GAUGE SETTINGS ==============================*/
#define ADP5360_mV_TO_VSOC(mV) ((uint8_t)((mV-2500)/8))
#define ADP5360_mV_MAX            (4540)//mV
typedef struct{
    uint8_t v_soc_0;//battery voltage = (2.5+v_soc_0*0.008)V
    uint8_t v_soc_5;
    uint8_t v_soc_11;
    uint8_t v_soc_19;
    uint8_t v_soc_28;
    uint8_t v_soc_41;
    uint8_t v_soc_55;
    uint8_t v_soc_69;
    uint8_t v_soc_84;
    uint8_t v_soc_100;
    uint8_t bat_cap;//battery capacity = (bat_cap*2)mAh
}ADP5360_BAT_CAP;

/* ============================= SOCACM control and read ==============================*/
typedef struct{
    uint8_t batcap_age_en:1;//Battery capacity aging compensation function selection.0 = Disable,1 = Enable
    uint8_t batcap_temp_en:1;//Battery capacity temperature compensation function selection.0 = Disable,1 = Enable
    uint8_t not_used:2;
    uint8_t batcap_temp:2;//00=0.2 %/°C,01=0.4 %/°C,10=0.6 %/°C,00=0.8 %/°C,
    uint8_t batcap_age:2;//00=0.8%,01=1.5%,10=3.1%,11=6.3%.  
}ADP5360_BAT_SOCACM_CTL;

/* ============================= fuel gauge mode ==============================*/
typedef struct{
    uint8_t fg_func_en:1;//Fuel Gauge function selection 0: Disable Fuel gauge 1: Enable Fuel gauge
    uint8_t fg_mode_en:1;//Fuel Gauge Operation Mode selection 0: Disable Sleep mode 1: Enable Sleep mode
    uint8_t slp_time:2;//00=1min,01=4min,10=8min,00=16min,//Fuel gauge sleep mode SOC update rate
    uint8_t slp_curr:2;//00=5mA,01=10mA,10=20mA,00=40mA,//fuel gauge sleep mode current threshold
    uint8_t soc_low_th:2;//00=6%,01=11%,10=21%,11=31%.//indicate of low SoC threshold 
}ADP5360_FG_MODE;

/* ============================= buck configure ==============================*/
#define ADP5360_BUCK_OUTPUT_VOL(mV)  (((mV)-600)/50)//0.6-3.75V
typedef struct{
    struct{
        uint8_t buck_output_en:1;//Buck ouptut control
        uint8_t dischg_buck_en:1;//Configure the output discharge functionality for buck
        uint8_t stp_buck_en:1;//Enable stop feature to buck regulator
        uint8_t buck_mode:1;//0=hystersis,1=force PWM//buck operate mode selection
        uint8_t buck_imin:2;//00=100mA,01=200mA,10=300mA,11=400mA//Buck regulator peak current limit
        uint8_t buck_ss:2;//00=1mS,01=8mS,10=64mS,11=512mS//Buck regulator output soft start time  
    }buck_cfg;
    struct{
        uint8_t vout_buck:6;//Buck ouptut voltage setting
        uint8_t buck_dly:2;//00=0uS,01=5uS,10=10uS,11=20uS,//Buck switch delay time in hystersys.
    }output_vol_set;
}ADP5360_BUCK_CFG;

/* ============================= buck-boost configure ==============================*/
#define ADP5360_BUCK_BOOST_OUTPUT_VOL(mV)  (((mV) > 2900)?(((mV)-2900)/50+11):(((mV)-1800)/100))//1.8-5.5V
typedef struct{
    struct{
        uint8_t buckbst_output_en:1;//Buck-boost ouptut control
        uint8_t dischg_buckbst_en:1;//Configure the output discharge functionality for buck-boost
        uint8_t stp_buckbst_en:1;//Enable stop feature to buck-boost regulator
        uint8_t buckbst_imin:3;//000=100mA,001=200mA,010=300mA,...,111=800mA//Buck-boost regulator peak current limit
        uint8_t buckbst_ss:2;//00=1mS,01=8mS,10=64mS,11=512mS//Buck-boost regulator output soft start time 
    }buckbst_cfg;
    struct{
        uint8_t vout_buckbst:6;//Buck boost ouptut voltage setting
        uint8_t buckbst_dly:2;//00=0uS,01=5uS,10=10uS,11=20uS,//Buckbst switch delay time in hystersys.  
    }output_vol_set;
}ADP5360_BUCK_BOOST_CFG;


/* =============================supervisory ==============================*/
typedef struct{
    uint8_t reset_wd:1;//High resets the watchdog safety timer. Bit is reset automatically.
    uint8_t mr_sd_en:1;//When high, enter to shipment mode after nMR press low for 12s
    uint8_t wd_en:1;//When high, watchdog timer function is enable
    uint8_t wd_time:2;//Watchdog timeout period selection.00 = 12.5s,01 = 25.6s,10 = 50s,11 = 100 s
    uint8_t reset_time:1;//0:200ms,1=1.6sec.
    uint8_t vout2_rst:1;//Buck-boost output voltage monitor to RESET selection
    uint8_t vout1_rst:1;//Buck output voltage monitor to RESET selection
}ADP5360_SUPERVISORY_SET;

/* ============================faults ==============================*/
/*To reset the fault bits in the fault register, cycle power on VBUSx or write high to the corresponding I2C bit*/
typedef struct{
    uint8_t tsd110:1;
    uint8_t tsd90:1;
    uint8_t wd_timeout:1;
    uint8_t not_used:1;
    uint8_t bat_chgov:1;
    uint8_t bat_chgoc:1;//
    uint8_t bat_oc:1;//
    uint8_t bat_uv:1;// 
}ADP5360_FAULTS;

/* ============================pgood status and mask ==============================*/
typedef struct{
    uint8_t vout1ok:1;
    uint8_t vout2ok:1;
    uint8_t batok:1;//This bit shows real-time status of battery voltage
    uint8_t vbusok:1;//This bit shows real-time status of VBUSx voltage
    uint8_t chg_cmplt:1;//This bit shows charge complete
    uint8_t mr_press:1;//nMR pin status,can not masked
    uint8_t not_used:2;// 
}ADP5360_PGOOD_STATUS;

typedef struct{
    uint8_t vout1ok_mask:1;//This bit configures external PGOOD pin. Not effective if buck is configured as load-switch mode:
    uint8_t vout2ok_mask:1;//This bit configures external PGOOD pin for buck-boost output
    uint8_t batok_mask:1;//This bit shows real-time status of battery voltage
    uint8_t vbusok_mask:1;//This bit shows real-time status of VBUSx voltage
    uint8_t chg_cmplt_mask:1;//This bit shows charge complete
    uint8_t not_used:2;//
    uint8_t pg_rev:1;//This bit configure PGOOD pin output active low output  
}ADP5360_PGOOD_MASK;//1 and 2

/* ============================interrupt enable and flag==============================*/
/*When read flag register, interrupt bit will reset automatically*/
typedef struct{
    struct{
        uint8_t vbus_int:1;
        uint8_t chg_int:1;
        uint8_t bat_int:1;
        uint8_t thr_int:1;
        uint8_t batpro_int:1;
        uint8_t adpichg_int:1;
        uint8_t socacm_int:1;
        uint8_t soclow_int:1; 
    }int1;
    struct{
        uint8_t not_used:3;
        uint8_t thrwrn_int:1;
        uint8_t bckbstpg_int:1;
        uint8_t buckpg_int:1;
        uint8_t wd_int:1;
        uint8_t mr_int:1;  
    }int2;
}ADP5360_INTERRUPT;


typedef enum {
  BATTERY_NOT_AVAILABLE    = 0x00,
  BATTERY_NOT_CHARGING     = 0x01,
  BATTERY_CHARGING         = 0x02,
  BATTERY_CHARGE_COMPLETE  = 0x03,
  BATTERY_CHARGER_LDO_MODE = 0x04,
  BATTERY_CHARGER_TIMER_EXPIRED = 0x05,
  BATTERY_DETECTION        = 0x06,
  BATTERY_UNKNOWN          = 0xFF,
} BATTERY_STATUS_ENUM_t;

typedef struct _chrg_status_t {
  BATTERY_STATUS_ENUM_t chrg_status;
  uint8_t               level;
  uint16_t              voltage_mv;
  //int16_t               temp_c;
} BATTERY_STATUS_t;

typedef void (*pgood_detect_func)(uint8_t value,ADP5360_PGOOD_STATUS status);
typedef void (*int_detect_func)(uint8_t value,ADP5360_INTERRUPT status);

/* =========================== FUNCTION PROTOTYPES ===========================*/
void Register_pgood_detect_func(pgood_detect_func hander);
void Unregister_pgood_detect_func(pgood_detect_func hander);
void Register_int_detect_func(int_detect_func hander);
void Unregister_int_detect_func(int_detect_func hander);

uint8_t Adp5360_pgood_pin_status_get(void);

ADP5360_RESULT Adp5360_getPgoodStatus(ADP5360_PGOOD_STATUS *pgood);
ADP5360_RESULT Adp5360_get_THR_Voltage(uint16_t *thr_v);

ADP5360_RESULT Adp5360_getBatVoltage(uint16_t *bat_v);
ADP5360_RESULT Adp5360_getChargerStatus1(ADP5360_CHG_STATUS1 * chg_status);
ADP5360_RESULT Adp5360_getChargerStatus2(ADP5360_CHG_STATUS2 * chg_status);
ADP5360_RESULT Adp5360_getBatSoC(uint8_t *bat_soc) ;
ADP5360_RESULT Adp5360_getDevID(unsigned char* devID);
/*global function*/
void Adp5360_output_vol_init(void);
void Adp5360_init(void);

void Adp5360_exit_shipment_mode(void);
void Adp5360_enter_shipment_mode(void);
ADP5360_RESULT Adp5360_get_battery_details(BATTERY_STATUS_t *p_battery_stat);
ADP5360_RESULT Adp5350_resetWdTimer();
ADP5360_RESULT Adp5350_wdt_set(uint8_t en);
m2m2_hdr_t *adp5360_app_reg_access(m2m2_hdr_t *p_pkt);
#endif
