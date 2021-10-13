/*! *****************************************************************************
    @file:    adp5360.c
    @brief:   ADP5360 Power IC driver code file.
    @details: This file will have Power IC specific code related to LDO settings and
             Battery Charger Enable and Bateery status Info.
    @version: $Revision: 1p0
    @date:    $Date: 05.10.2016
    -----------------------------------------------------------------------------
    Copyright (c) 2019 Analog Devices, Inc.

    All rights reserved.

    Redistribution and use in source and binary forms, with or without modification,
    are permitted provided that the following conditions are met:
    - Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
    - Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
    - Modified versions of the software must be conspicuously marked as such.
    - This software is licensed solely and exclusively for use with processors
    manufactured by or for Analog Devices, Inc.
    - This software may not be combined or merged with other code in any manner
    that would cause the software to become subject to terms and conditions
    which differ from those listed here.
    - Neither the name of Analog Devices, Inc. nor the names of its
    contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.
    - The use of this software may or may not infringe the patent rights of one
    or more patent holders.  This license does not release you from the
    requirement that you obtain separate licenses from these patent holders
    to use this software.

    THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES, INC. AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-
    INFRINGEMENT, TITLE, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL ANALOG DEVICES, INC. OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, PUNITIVE OR
    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, DAMAGES ARISING OUT OF
    CLAIMS OF INTELLECTUAL PROPERTY RIGHTS INFRINGEMENT; PROCUREMENT OF
    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.

*****************************************************************************/
#include "adp5360.h"
#include <stdbool.h>
#ifdef VSM_MBOARD
#include "watch_vsm_motherboard_pin_config.h"
#elif PCBA
#include "watch_board_pcba_pin_config.h"
#else
#include "watch_board_evt_pin_config.h"
#endif
#include "nrf_drv_gpiote.h"
#include "common_sensor_interface.h"
#include "hal_twi0.h"
#ifdef TEMP_PROTECT_FUNC_EN
#include "battery_temp_detect.h"
#endif
#include "low_voltage_protect.h"
#include "nrf_log.h"
#if NRF_LOG_ENABLED

#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#endif

#define ADP5360_I2C_ADDRESS (0x46)


static uint8_t gnBatteryDetectionFlag;
static uint8_t  gn_ChargerStatusCheck;

ADP5360_RESULT Adp5360_getAndClearInterrupt(ADP5360_INTERRUPT *inter);

#define PGOOD_USER_MAX (2)
static pgood_detect_func pgood_user_handle[PGOOD_USER_MAX] = {NULL};

void Register_pgood_detect_func(pgood_detect_func hander)
{
    for(int i = 0;i<PGOOD_USER_MAX;i++)
    {
        if(NULL == pgood_user_handle[i])
        {
            pgood_user_handle[i] = hander;
            break;
        }
    }
}

void Unregister_pgood_detect_func(pgood_detect_func hander)
{
    for(int i = 0;i<PGOOD_USER_MAX;i++)
    {
        if(hander == pgood_user_handle[i])
        {
            pgood_user_handle[i] = NULL;
            break;
        }
    }
}

#define INT_USER_MAX (2)
static int_detect_func int_user_handle[INT_USER_MAX] = {NULL};

void Register_int_detect_func(int_detect_func hander)
{
    for(int i = 0;i<INT_USER_MAX;i++)
    {
        if(NULL == int_user_handle[i])
        {
            int_user_handle[i] = hander;
            break;
        }
    }
}

void Unregister_int_detect_func(int_detect_func hander)
{
    for(int i = 0;i<INT_USER_MAX;i++)
    {
        if(hander == int_user_handle[i])
        {
            int_user_handle[i] = NULL;
            break;
        }
    }
}

static void adp5360_int1_event_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    uint8_t value = 0;
    ADP5360_INTERRUPT interrupt;

    value = nrf_drv_gpiote_in_is_set(pin);
//    if(0 == value)
    {
        Adp5360_getAndClearInterrupt(&interrupt);
        for(int i = 0;i<INT_USER_MAX;i++)
        {
            if(NULL != int_user_handle[i])
            {
                int_user_handle[i](value,interrupt);
            }
        }

    }
}


static void adp5360_pgood_event_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    ADP5360_PGOOD_STATUS pgood;
    uint8_t value = 0;

    value = nrf_drv_gpiote_in_is_set(pin);
    if(0 == value)
    {
        //NRF_LOG_INFO("the status of out1 PIN = %d,vbus = %d",value,pgood.vbusok);
        Adp5360_getPgoodStatus(&pgood);
        for(int i = 0;i<PGOOD_USER_MAX;i++)
        {
            if(NULL != pgood_user_handle[i])
            {
                pgood_user_handle[i](value,pgood);
            }
        }
    }
}


uint8_t Adp5360_pgood_pin_status_get(void)
{
    return nrf_drv_gpiote_in_is_set(ADP5360_PGOOD_PIN);
}
ret_code_t Adp5360_port_init(void)
{
    uint32_t err_code = NRF_SUCCESS;

    nrf_drv_gpiote_in_config_t config = GPIOTE_CONFIG_IN_SENSE_TOGGLE(false);
    if (!nrf_drv_gpiote_is_init())
    {
        err_code = nrf_drv_gpiote_init();
        if (err_code != NRF_SUCCESS)
        {
            return err_code;
        }
    }
    err_code = nrf_drv_gpiote_in_init(ADP5360_INT1_PIN, &config, adp5360_int1_event_handler);
    VERIFY_SUCCESS(err_code);
    //nrf_drv_gpiote_in_event_enable(ADP5360_INT1_PIN, true);

    err_code = nrf_drv_gpiote_in_init(ADP5360_PGOOD_PIN, &config, adp5360_pgood_event_handler);
    VERIFY_SUCCESS(err_code);
    //nrf_drv_gpiote_in_event_enable(ADP5360_PGOOD_PIN, true);

    return err_code;
}

void ADP5360_gpio_interrupt_enable()
{
    nrf_drv_gpiote_in_event_enable(ADP5360_INT1_PIN, true);
    nrf_drv_gpiote_in_event_enable(ADP5360_PGOOD_PIN, true);
}

__STATIC_INLINE ret_code_t write_I2C_ADP5360(unsigned char RegisterAddr,unsigned char* dataBuffer,unsigned char bytesNumber)
{
    return (twi0_write_register(ADP5360_I2C_ADDRESS,RegisterAddr,dataBuffer,bytesNumber));
}

__STATIC_INLINE ret_code_t read_I2C_ADP5360(unsigned char RegisterAddr,unsigned char* dataBuffer,unsigned char bytesNumber)
{
    return (twi0_read_register(ADP5360_I2C_ADDRESS,RegisterAddr,dataBuffer,bytesNumber));
}
/*!
    @brief                     This function is used to get devide ID of ADP5360.
    @param devID               Devide ID of ADP5360.
    @return ADP5360_RESULT     0-Successfully Completed/ 1- Input parameter to the function is invalid / 2-Error during I2C transaction.
*/
ADP5360_RESULT Adp5360_getDevID(unsigned char* devID)
{
    return (read_I2C_ADP5360(ADP5360_REG_DEV_ID, devID, 1));
}

/*!
    @brief                     This function is used to get revision ID of ADP5360.
    @param RevID               revision ID of ADP5360.
    @return ADP5360_RESULT     0-Successfully Completed/ 1- Input parameter to the function is invalid / 2-Error during I2C transaction.
*/
ADP5360_RESULT Adp5360_getRevID(unsigned char* revID)
{
    return (read_I2C_ADP5360(ADP5360_REG_REV_ID, revID, 1));
}

ADP5360_RESULT Adp5360_setVbusIlim(ADP5360_CHG_VBUS_ILIM *vbus_ilim)
{
    return (write_I2C_ADP5360(ADP5360_REG_VBUS_ILIM, (uint8_t*)vbus_ilim, sizeof(ADP5360_CHG_VBUS_ILIM)));
}

ADP5360_RESULT Adp5360_setBatteryThresholds(ADP5360_BATTERY_THRESHOLDS* b_thr)
{
    return (write_I2C_ADP5360(ADP5360_REG_TERM_SET, (uint8_t *)b_thr, sizeof(ADP5360_BATTERY_THRESHOLDS)));
}

ADP5360_RESULT Adp5360_setChgTimerSet(ADP5360_CHG_TIMER_SET *timer_set)
{
    return (write_I2C_ADP5360(ADP5360_REG_TIMER_SETT, (uint8_t *)timer_set, sizeof(ADP5360_CHG_TIMER_SET)));
}

ADP5360_RESULT Adp5360_setChgFunctionalSet(ADP5360_CHG_FUNC_SET * func_set)
{
    return (write_I2C_ADP5360(ADP5360_REG_FUNC_SETT, (uint8_t *)func_set, sizeof(ADP5360_CHG_FUNC_SET)));
}
ADP5360_RESULT Adp5360_getChgFunctionalSet(ADP5360_CHG_FUNC_SET *func_settings)
{
    return (read_I2C_ADP5360(ADP5360_REG_FUNC_SETT,(uint8_t *)func_settings, sizeof(ADP5360_CHG_FUNC_SET)));
}
ADP5360_RESULT Adp5360_getChargerStatus1(ADP5360_CHG_STATUS1 * chg_status)
{
    return (read_I2C_ADP5360(ADP5360_REG_CHG_STATUS1,(uint8_t *)chg_status, sizeof(ADP5360_CHG_STATUS1)));
}

ADP5360_RESULT Adp5360_getChargerStatus2(ADP5360_CHG_STATUS2 * chg_status)
{
    return (read_I2C_ADP5360(ADP5360_REG_CHG_STATUS2,(uint8_t *)chg_status, sizeof(ADP5360_CHG_STATUS2)));
}

ADP5360_RESULT Adp5360_setThermalControl(ADP5360_THERMAL_CTRL *thrm_ctrl)
{
    return (write_I2C_ADP5360(ADP5360_REG_BATT_THERM_CTRL,(uint8_t *)thrm_ctrl, sizeof(ADP5360_THERMAL_CTRL)));
}

ADP5360_RESULT Adp5360_get_THR_Voltage(uint16_t *thr_v)
{
    uint8_t buffer[2];
    ADP5360_RESULT ret;
    ret = read_I2C_ADP5360(ADP5360_REG_THR_VOLTAGE_LOW, buffer, 2);
    *thr_v = (((uint16_t) (buffer[1]&0x0f))<<8)|buffer[0];
    return ret;
}

ADP5360_RESULT Adp5360_setBatProtecton(ADP5360_BAT_PROTECTION *bat_proc)
{
    return (write_I2C_ADP5360(ADP5360_REG_BAT_PROC_CONTROL, (uint8_t *)bat_proc, sizeof(ADP5360_BAT_PROTECTION)));
}

ADP5360_RESULT Adp5360_getBatProtecton(ADP5360_BAT_PROTECTION *bat_proc)
{
    return (read_I2C_ADP5360(ADP5360_REG_BAT_PROC_CONTROL, (uint8_t *)bat_proc, sizeof(ADP5360_BAT_PROTECTION)));
}

ADP5360_RESULT Adp5360_setBatCapacity(ADP5360_BAT_CAP *bat_cap)
{
    return (write_I2C_ADP5360(ADP5360_REG_V_SOC_0, (uint8_t *)bat_cap, sizeof(ADP5360_BAT_CAP)));
}

ADP5360_RESULT Adp5360_getBatSoC(uint8_t *bat_soc)
{
    return (read_I2C_ADP5360(ADP5360_REG_BAT_SOC, bat_soc, sizeof(uint8_t)));
}

ADP5360_RESULT Adp5360_setBatSocacmCtl(ADP5360_BAT_SOCACM_CTL *socacm_ctl)
{
    return (write_I2C_ADP5360(ADP5360_REG_BAT_SOCACM_CTL, (uint8_t *)socacm_ctl, sizeof(ADP5360_BAT_SOCACM_CTL)));
}

ADP5360_RESULT Adp5360_getSocacm(uint16_t *socacm)
{
    uint8_t buffer_rx[2];
    ADP5360_RESULT ret;
    ret = read_I2C_ADP5360(ADP5360_REG_BAT_SOCACM_H, buffer_rx, sizeof(uint16_t));
    *socacm = ((uint16_t)buffer_rx[0] << 4)|(buffer_rx[1] >> 4);
    return ret;
}

ADP5360_RESULT Adp5360_getBatVoltage(uint16_t *bat_v)
{
    uint8_t buffer_rx[2];
    ADP5360_RESULT ret;
    ret = read_I2C_ADP5360(ADP5360_REG_BAT_READ_H, buffer_rx, sizeof(uint16_t));
    *bat_v = ((uint16_t)buffer_rx[0] << 5)|(buffer_rx[1] >> 3);
    return ret;
}

ADP5360_RESULT Adp5360_setFuelGaugeMode(ADP5360_FG_MODE *fg_mode)
{
    return (write_I2C_ADP5360(ADP5360_REG_FUEL_GAUGE_MODE, (uint8_t *)fg_mode, sizeof(ADP5360_FG_MODE)));
}

ADP5360_RESULT Adp5360_Reset_SoC(void)
{
    uint8_t buffer_tx[2] = {0x80,0};
    ADP5360_RESULT ret;
    ret = write_I2C_ADP5360(ADP5360_REG_SOC_RESET, &buffer_tx[0], 1);
    if(ret != ADP5360_SUCCESS)
    {
        return ret;
    }
    return (write_I2C_ADP5360(ADP5360_REG_SOC_RESET, &buffer_tx[1], 1));
}

ADP5360_RESULT Adp5360_setBuckCfg(ADP5360_BUCK_CFG *buck_cfg_t)
{
    return (write_I2C_ADP5360(ADP5360_REG_BUCK_CFG, (uint8_t *)buck_cfg_t, sizeof(ADP5360_BUCK_CFG)));
}

ADP5360_RESULT Adp5360_getBuckCfg(ADP5360_BUCK_CFG *buck_cfg_t)
{
    return (read_I2C_ADP5360(ADP5360_REG_BUCK_CFG, (uint8_t *)buck_cfg_t, sizeof(ADP5360_BUCK_CFG)));
}

ADP5360_RESULT Adp5360_setBuckBstCfg(ADP5360_BUCK_BOOST_CFG *buckbst_cfg_t)
{
    return (write_I2C_ADP5360(ADP5360_REG_BUCK_B_CFG, (uint8_t *)buckbst_cfg_t, sizeof(ADP5360_BUCK_BOOST_CFG)));
}

ADP5360_RESULT Adp5360_getBuckBstCfg(ADP5360_BUCK_BOOST_CFG *buckbst_cfg_t)
{
    return (read_I2C_ADP5360(ADP5360_REG_BUCK_B_CFG, (uint8_t *)buckbst_cfg_t, sizeof(ADP5360_BUCK_BOOST_CFG)));
}

ADP5360_RESULT Adp5360_setSupervisory(ADP5360_SUPERVISORY_SET *supervisory)
{
    return (write_I2C_ADP5360(ADP5360_REG_SUPERVISORY_SETT, (uint8_t *)supervisory, sizeof(ADP5360_SUPERVISORY_SET)));
}



ADP5360_RESULT Adp5360_getFaults(ADP5360_FAULTS *fault)
{
    return (read_I2C_ADP5360(ADP5360_REG_FAULTS, (uint8_t *)fault, sizeof(ADP5360_FAULTS)));
}

ADP5360_RESULT Adp5360_clearFaults(ADP5360_FAULTS *fault)
{
    return (write_I2C_ADP5360(ADP5360_REG_FAULTS, (uint8_t *)fault, sizeof(ADP5360_FAULTS)));
}

ADP5360_RESULT Adp5360_getPgoodStatus(ADP5360_PGOOD_STATUS *pgood)
{
    return (read_I2C_ADP5360(ADP5360_REG_PGOOD_STATUS, (uint8_t *)pgood, sizeof(ADP5360_PGOOD_STATUS)));
}

ADP5360_RESULT Adp5360_setPgood1Mask(ADP5360_PGOOD_MASK *pgood)
{
    return (write_I2C_ADP5360(ADP5360_REG_PGOOD1_MASK, (uint8_t *)pgood, sizeof(ADP5360_PGOOD_MASK)));
}

ADP5360_RESULT Adp5360_setPgood2Mask(ADP5360_PGOOD_MASK *pgood)
{
    return (write_I2C_ADP5360(ADP5360_REG_PGOOD2_MASK, (uint8_t *)pgood, sizeof(ADP5360_PGOOD_MASK)));
}

ADP5360_RESULT Adp5360_enableInterrupt(ADP5360_INTERRUPT *inter)
{
    return (write_I2C_ADP5360(ADP5360_REG_INT_EN1, (uint8_t *)inter, sizeof(ADP5360_INTERRUPT)));
}

/*When read the register, interrupt bit will reset automatically*/
ADP5360_RESULT Adp5360_getAndClearInterrupt(ADP5360_INTERRUPT *inter)
{
    return (read_I2C_ADP5360(ADP5360_REG_INT_FLAG1, (uint8_t *)inter, sizeof(ADP5360_INTERRUPT)));
}

//ADP5360_RESULT Adp5360_DefaultReset(void) //reset all register to default value
//{
//    uint8_t write_buffer = 0x7F;
//    return (write_I2C_ADP5360(ADP5360_REG_DEFAULT_SET, &write_buffer, 1));
//}

void Adp5360_output_vol_init(void)
{
    ADP5360_BUCK_CFG buck_config;
    ADP5360_BUCK_BOOST_CFG buck_bst_config;

    buck_config.buck_cfg.buck_ss = 0;
    buck_config.buck_cfg.buck_imin = 3;
    buck_config.buck_cfg.buck_mode = 0;
    buck_config.buck_cfg.stp_buck_en = ADP5360_DISABLE;
    buck_config.buck_cfg.dischg_buck_en = ADP5360_DISABLE;
    buck_config.buck_cfg.buck_output_en = ADP5360_ENABLE;

    buck_config.output_vol_set.buck_dly = 0;
    buck_config.output_vol_set.vout_buck = ADP5360_BUCK_OUTPUT_VOL(2100);//2.1V

    Adp5360_setBuckCfg(&buck_config);

    buck_bst_config.buckbst_cfg.buckbst_ss = 0;
    buck_bst_config.buckbst_cfg.buckbst_imin = 4;
    buck_bst_config.buckbst_cfg.stp_buckbst_en = ADP5360_DISABLE;
    buck_bst_config.buckbst_cfg.dischg_buckbst_en = ADP5360_DISABLE;
    buck_bst_config.buckbst_cfg.buckbst_output_en = ADP5360_ENABLE;

    buck_bst_config.output_vol_set.buckbst_dly = 0;
    buck_bst_config.output_vol_set.vout_buckbst = ADP5360_BUCK_BOOST_OUTPUT_VOL(4500);//4.5V
    Adp5360_setBuckBstCfg(&buck_bst_config);
}

ret_code_t Adp5360_pgood_init(void)
{
    ADP5360_PGOOD_MASK pgood1;
    pgood1.pg_rev = ADP5360_ENABLE;
    pgood1.chg_cmplt_mask = ADP5360_DISABLE;
    pgood1.vbusok_mask = ADP5360_ENABLE;
    pgood1.batok_mask = ADP5360_DISABLE;
    pgood1.vout1ok_mask = ADP5360_DISABLE;
    pgood1.vout2ok_mask = ADP5360_DISABLE;
    Adp5360_setPgood1Mask(&pgood1);//pgood1 used to monitor Vbus pin.

    ADP5360_PGOOD_MASK pgood2;
    pgood2.pg_rev = ADP5360_DISABLE;
    pgood2.chg_cmplt_mask = ADP5360_DISABLE;
    pgood2.vbusok_mask = ADP5360_DISABLE;
    pgood2.batok_mask = ADP5360_DISABLE;
    pgood2.vout1ok_mask = ADP5360_DISABLE;
    pgood2.vout2ok_mask = ADP5360_DISABLE;
    Adp5360_setPgood2Mask(&pgood2);

    return ADP5360_SUCCESS;
}
ret_code_t Adp5360_interrupt_init(void)
{
    ADP5360_INTERRUPT interrupt;
    interrupt.int1.soclow_int = ADP5360_ENABLE;//enable the low voltage interrupt
    interrupt.int1.socacm_int = ADP5360_DISABLE;
    interrupt.int1.adpichg_int = ADP5360_DISABLE;
    interrupt.int1.batpro_int = ADP5360_ENABLE;//enable the battery fault interrupt
    interrupt.int1.thr_int = ADP5360_ENABLE;//enable the temperature threthold interrupt.
    interrupt.int1.bat_int = ADP5360_DISABLE;
    interrupt.int1.chg_int = ADP5360_DISABLE;
    interrupt.int1.vbus_int = ADP5360_DISABLE;

    interrupt.int2.mr_int = ADP5360_DISABLE;
    interrupt.int2.wd_int = ADP5360_DISABLE;
    interrupt.int2.buckpg_int = ADP5360_DISABLE;
    interrupt.int2.bckbstpg_int = ADP5360_DISABLE;
    interrupt.int2.thrwrn_int = ADP5360_DISABLE;

    Adp5360_enableInterrupt(&interrupt);
    Adp5360_getAndClearInterrupt(&interrupt);
    return ADP5360_SUCCESS;
}

ret_code_t Adp5360_reset_init(void)
{
    ADP5360_SUPERVISORY_SET supervisory;
    supervisory.vout1_rst = 1;
    supervisory.vout2_rst = 0;
    supervisory.reset_time = 0;
    supervisory.wd_time = 1; //25.6sec  /*0->12.5s; 1->25.6s; 2->50s; 3->100s*/
#ifdef USE_ADP5360_WDT
    supervisory.wd_en = ADP5360_ENABLE;
#else
    supervisory.wd_en = ADP5360_DISABLE;
#endif
    supervisory.mr_sd_en = ADP5360_DISABLE;
    supervisory.reset_wd = 1;
    Adp5360_setSupervisory(&supervisory);

    return ADP5360_SUCCESS;
}


ADP5360_RESULT Adp5350_wdt_set(uint8_t en)
{
    ADP5360_SUPERVISORY_SET supervisory;
    if (read_I2C_ADP5360(ADP5360_REG_SUPERVISORY_SETT, (uint8_t *)&supervisory, sizeof(ADP5360_SUPERVISORY_SET))) {
        return ADP5360_ERR_I2C;
    }
    supervisory.wd_en = en;
    if (write_I2C_ADP5360(ADP5360_REG_SUPERVISORY_SETT, (uint8_t *)&supervisory, sizeof(ADP5360_SUPERVISORY_SET))) {
        return ADP5360_ERR_I2C;
    }
    return ADP5360_SUCCESS;
}
/*add this func to the lowest priority task to clear watchdog timer*/
ADP5360_RESULT Adp5350_resetWdTimer()
{
    ADP5360_SUPERVISORY_SET supervisory;
    if (read_I2C_ADP5360(ADP5360_REG_SUPERVISORY_SETT, (uint8_t *)&supervisory, sizeof(ADP5360_SUPERVISORY_SET))) {
        return ADP5360_ERR_I2C;
    }
    supervisory.reset_wd = 1;
    if (write_I2C_ADP5360(ADP5360_REG_SUPERVISORY_SETT, (uint8_t *)&supervisory, sizeof(ADP5360_SUPERVISORY_SET))) {
        return ADP5360_ERR_I2C;
    }
    return ADP5360_SUCCESS;
}

m2m2_hdr_t *adp5360_app_reg_access(m2m2_hdr_t *p_pkt) {
  M2M2_APP_COMMON_STATUS_ENUM_t status = M2M2_APP_COMMON_STATUS_ERROR;
  // Declare a pointer to access the input packet payload
  PYLD_CST(p_pkt, m2m2_sensor_common_reg_op_16_hdr_t, p_in_payload);
  // Allocate a response packet with space for the correct number of operations
  PKT_MALLOC(p_resp_pkt, m2m2_sensor_common_reg_op_16_hdr_t, p_in_payload->num_ops * sizeof(p_in_payload->ops[0]));
  if(NULL != p_resp_pkt)
  {
      PYLD_CST(p_resp_pkt, m2m2_sensor_common_reg_op_16_hdr_t, p_resp_payload);
      uint8_t  reg_data = 0;

      switch (p_in_payload->command) {
      case M2M2_SENSOR_COMMON_CMD_READ_REG_16_REQ:
        for (int i = 0; i < p_in_payload->num_ops; i++) {
          if (NRFX_SUCCESS == read_I2C_ADP5360((uint8_t)p_in_payload->ops[i].address,(uint8_t *)&reg_data, sizeof(reg_data))) {
            status = M2M2_APP_COMMON_STATUS_OK;
          } else {
            status = M2M2_APP_COMMON_STATUS_ERROR;
          }
          p_resp_payload->ops[i].address = p_in_payload->ops[i].address;
          p_resp_payload->ops[i].value = reg_data;
        }
        p_resp_payload->command = (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_SENSOR_COMMON_CMD_READ_REG_16_RESP;
        break;
      case M2M2_SENSOR_COMMON_CMD_WRITE_REG_16_REQ:
        for (int i = 0; i < p_in_payload->num_ops; i++) {
          //if ( NRFX_SUCCESS == write_I2C_ADP5360((uint8_t)p_in_payload->ops[i].address, (uint8_t *)&p_in_payload->ops[i].value, sizeof(p_in_payload->ops[i].value))) {
          uint16_t dataBuffer = p_in_payload->ops[i].value;
          if ( NRFX_SUCCESS == write_I2C_ADP5360((uint8_t)p_in_payload->ops[i].address, (uint8_t *)&dataBuffer, sizeof(p_in_payload->ops[i].value))) {
            status = M2M2_APP_COMMON_STATUS_OK;
          } else {
            status = M2M2_APP_COMMON_STATUS_ERROR;
          }
          p_resp_payload->ops[i].address = p_in_payload->ops[i].address;
          p_resp_payload->ops[i].value = p_in_payload->ops[i].value;
        }
        p_resp_payload->command = (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_SENSOR_COMMON_CMD_WRITE_REG_16_RESP;
        break;
      default:
        // Something has gone horribly wrong.
        post_office_consume_msg(p_resp_pkt);
        return NULL;
      }
      p_resp_pkt->dest = p_pkt->src;
      p_resp_pkt->src = p_pkt->dest;
      p_resp_payload->num_ops = p_in_payload->num_ops;
      p_resp_payload->status = status;
  }

  return p_resp_pkt;
}

ret_code_t Adp5360_bat_soc_init(void)
{
    ADP5360_BAT_CAP bat_cap_param;

#ifdef VSM_MBOARD
#ifdef RJD2450
    bat_cap_param.v_soc_0 = ADP5360_mV_TO_VSOC(3000);
    bat_cap_param.v_soc_5 = ADP5360_mV_TO_VSOC(3390);
    bat_cap_param.v_soc_11 = ADP5360_mV_TO_VSOC(3480);
    bat_cap_param.v_soc_19 = ADP5360_mV_TO_VSOC(3550);
    bat_cap_param.v_soc_28 = ADP5360_mV_TO_VSOC(3610);
    bat_cap_param.v_soc_41 = ADP5360_mV_TO_VSOC(3690);
    bat_cap_param.v_soc_55 = ADP5360_mV_TO_VSOC(3760);
    bat_cap_param.v_soc_69 = ADP5360_mV_TO_VSOC(3850);
    bat_cap_param.v_soc_84 = ADP5360_mV_TO_VSOC(3990);
    bat_cap_param.v_soc_100 = ADP5360_mV_TO_VSOC(4200);
    bat_cap_param.bat_cap = 200/2;//200mAh,the battery parameter.
#else
    bat_cap_param.v_soc_0 = ADP5360_mV_TO_VSOC(2750);
    bat_cap_param.v_soc_5 = ADP5360_mV_TO_VSOC(3000);
    bat_cap_param.v_soc_11 = ADP5360_mV_TO_VSOC(3250);
    bat_cap_param.v_soc_19 = ADP5360_mV_TO_VSOC(3400);
    bat_cap_param.v_soc_28 = ADP5360_mV_TO_VSOC(3550);
    bat_cap_param.v_soc_41 = ADP5360_mV_TO_VSOC(3590);
    bat_cap_param.v_soc_55 = ADP5360_mV_TO_VSOC(3600);
    bat_cap_param.v_soc_69 = ADP5360_mV_TO_VSOC(3700);
    bat_cap_param.v_soc_84 = ADP5360_mV_TO_VSOC(3800);
    bat_cap_param.v_soc_100 = ADP5360_mV_TO_VSOC(4200);
    bat_cap_param.bat_cap = 120/2;//120mA,the battery parameter.
#endif
#else
    bat_cap_param.v_soc_0 = ADP5360_mV_TO_VSOC(3000);
    bat_cap_param.v_soc_5 = ADP5360_mV_TO_VSOC(3565);
    bat_cap_param.v_soc_11 = ADP5360_mV_TO_VSOC(3637);
    bat_cap_param.v_soc_19 = ADP5360_mV_TO_VSOC(3682);
    bat_cap_param.v_soc_28 = ADP5360_mV_TO_VSOC(3717);
    bat_cap_param.v_soc_41 = ADP5360_mV_TO_VSOC(3763);
    bat_cap_param.v_soc_55 = ADP5360_mV_TO_VSOC(3835);
    bat_cap_param.v_soc_69 = ADP5360_mV_TO_VSOC(3936);
    bat_cap_param.v_soc_84 = ADP5360_mV_TO_VSOC(4100);
    bat_cap_param.v_soc_100 = ADP5360_mV_TO_VSOC(4306);//all was the battery parameter
    bat_cap_param.bat_cap = 200/2;//200mA,the battery parameter.
#endif

    Adp5360_setBatCapacity(&bat_cap_param);

    ADP5360_BAT_SOCACM_CTL socacm_control;

    socacm_control.batcap_age = 1;//1.5%,the battery parameter.
    socacm_control.batcap_age_en = ADP5360_ENABLE;
    socacm_control.batcap_temp = 0;//wait the battery data.
    socacm_control.batcap_temp_en = ADP5360_DISABLE;

    Adp5360_setBatSocacmCtl(&socacm_control);

    ADP5360_FG_MODE fuel_gauge_mode;
    fuel_gauge_mode.fg_func_en = ADP5360_ENABLE;
    fuel_gauge_mode.fg_mode_en = ADP5360_ENABLE;
    fuel_gauge_mode.slp_curr = 0;
    fuel_gauge_mode.slp_time = 0;
    fuel_gauge_mode.soc_low_th = 0;//under 6%, alarm to charge.

    Adp5360_setFuelGaugeMode(&fuel_gauge_mode);

    return ADP5360_SUCCESS;
}

ret_code_t Adp5360_bat_proc_init(void)
{
    ADP5360_THERMAL_CTRL thrm_control;
    thrm_control.thermistor_ctrl.ithr = ADP5360_ITHR_60UA;//60uA->10K NTC,the battery parameter.
    thrm_control.thermistor_ctrl.thr_en = ADP5360_ENABLE;//detect temperature even in discharge status. but its rate slow down to 30s.
    thrm_control.temp_high_60 = 90;//resistance value = 3.019k,v = 0.18.
    thrm_control.temp_high_45 = 147; //resistance value = 4.911k,v = 0.29.
    thrm_control.temp_low_10 = 107;//resistance value = 17.96k,v = 1.077.
    thrm_control.temp_low_0 = 163;//resistance value = 27.28k,v = 1.63.

    Adp5360_setThermalControl(&thrm_control);

    ADP5360_BAT_PROTECTION bat_protection;

    bat_protection.bat_proc_ctrl.batpro_en = ADP5360_ENABLE;
    bat_protection.bat_proc_ctrl.chglb_en = ADP5360_ENABLE;
    bat_protection.bat_proc_ctrl.isofet_ovchg = 0;
    bat_protection.bat_proc_ctrl.oc_dis_hiccup = 0;
    bat_protection.bat_proc_ctrl.oc_chg_hiccup = 0;

#ifdef VSM_MBOARD
#ifdef RJD2450
    bat_protection.bat_dischg_uv_set.uv_disch = ADP5360_mV_TO_UV_DISCH(2800);//3.3V,electric quantity only remain 1%
    bat_protection.bat_dischg_uv_set.hys_uv_disch = 0;
    bat_protection.bat_dischg_uv_set.dgt_uv_disch = 0;

    bat_protection.bat_dischg_oc_set.oc_disch = ADP5360_mA_TO_OC_DISCH(350); // set maximum discharge to 350mA (RJD2450 max is 2C = 400mA)
    bat_protection.bat_dischg_oc_set.dgt_oc_disch = ADP5360_DGT_OC_DISCH_5MS;

    bat_protection.bat_chg_ov_set.ov_chg = ADP5360_mV_TO_OV_CHG(4250);//4.25V,the battery parameter was (4.2±0.030)V
    bat_protection.bat_chg_ov_set.hys_ov_chg = 0;
    bat_protection.bat_chg_ov_set.dgt_ov_chg = 0;

    bat_protection.bat_chg_oc_set.oc_chg = ADP5360_OC_CHG_200MA;//200mA,the battery parameter was 200mA(20~45),100(10~20),40ma(0~10)
    bat_protection.bat_chg_oc_set.dgt_oc_chg = 1;
#else
    //bat_protection.bat_dischg_uv_set.uv_disch = ADP5360_mV_TO_UV_DISCH(2600);//2.6V,the battery parameter was 2.5V
    bat_protection.bat_dischg_uv_set.uv_disch = ADP5360_mV_TO_UV_DISCH(2800);//3.3V,electric quantity only remain 1%
    bat_protection.bat_dischg_uv_set.hys_uv_disch = 0;
    bat_protection.bat_dischg_uv_set.dgt_uv_disch = 0;

    bat_protection.bat_dischg_oc_set.oc_disch = ADP5360_mA_TO_OC_DISCH(600);
    bat_protection.bat_dischg_oc_set.dgt_oc_disch = ADP5360_DGT_OC_DISCH_0_5MS;

    bat_protection.bat_chg_ov_set.ov_chg = ADP5360_mV_TO_OV_CHG(4400);//4.4V,the battery parameter was (4.425±0.020)V
    bat_protection.bat_chg_ov_set.hys_ov_chg = 0;
    bat_protection.bat_chg_ov_set.dgt_ov_chg = 0;

    bat_protection.bat_chg_oc_set.oc_chg = ADP5360_OC_CHG_200MA;//200ma,the battery parameter was 200mA(20~45),100(10~20),40ma(0~10)
    bat_protection.bat_chg_oc_set.dgt_oc_chg = 1;
#endif
#else
    //bat_protection.bat_dischg_uv_set.uv_disch = ADP5360_mV_TO_UV_DISCH(2600);//2.6V,the battery parameter was 2.5V
    bat_protection.bat_dischg_uv_set.uv_disch = ADP5360_mV_TO_UV_DISCH(2800);//3.3V,electric quantity only remain 1%
    bat_protection.bat_dischg_uv_set.hys_uv_disch = 0;
    bat_protection.bat_dischg_uv_set.dgt_uv_disch = 0;

    bat_protection.bat_dischg_oc_set.oc_disch = ADP5360_mA_TO_OC_DISCH(600);
    bat_protection.bat_dischg_oc_set.dgt_oc_disch = ADP5360_DGT_OC_DISCH_0_5MS;

    bat_protection.bat_chg_ov_set.ov_chg = ADP5360_mV_TO_OV_CHG(4400);//4.4V,the battery parameter was (4.425±0.020)V
    bat_protection.bat_chg_ov_set.hys_ov_chg = 0;
    bat_protection.bat_chg_ov_set.dgt_ov_chg = 0;

    bat_protection.bat_chg_oc_set.oc_chg = ADP5360_OC_CHG_200MA;//200ma,the battery parameter was 200mA(20~45),100(10~20),40ma(0~10)
    bat_protection.bat_chg_oc_set.dgt_oc_chg = 1;
#endif
    Adp5360_setBatProtecton(&bat_protection);

    return ADP5360_SUCCESS;
}

ret_code_t Adp5360_bat_charger_init(void)
{
    ADP5360_CHG_VBUS_ILIM chg_vbus_ilim;

    chg_vbus_ilim.vadpichg = ADP5360_mV_TO_VADPICHG(4600);
    chg_vbus_ilim.vsystem = 0;
    chg_vbus_ilim.ilim = ADP5360_VBUS_ILIM_400mA;

    Adp5360_setVbusIlim(&chg_vbus_ilim);

    ADP5360_BATTERY_THRESHOLDS bat_thr;
#ifdef VSM_MBOARD
#ifdef RJD2450
    /*
      The charging requirements are as follows:

      Nom capacity 200mAh
      Charging voltage 4.2V
      Nominal Voltage 3.7V
      Max discharge 200mAh
      End Discharge voltage 3.0V

      Charging scheme:
      Constant  charging current of 100mAh until battery voltage reaches 4.2V then, constant voltage of 4.2V until current reaches ~2.4mA.
    */
    bat_thr.chg_termination_set.vtrm = ADP5360_mV_TO_VTRM(4200);//Charging voltage is 4.2V
    bat_thr.chg_termination_set.itrk_dead = 2;
    bat_thr.chg_current_set.iend = ADP5360_mA_TO_IEND(5);//2mA constant voltage phase
    bat_thr.chg_current_set.ichg = ADP5360_mA_TO_ICHG(100);//use the 0.5C charge.the battery is 200mAh
    bat_thr.chg_voltage_threshold.dis_rch = 0;//enable charge.
    bat_thr.chg_voltage_threshold.vrch = 1;
    bat_thr.chg_voltage_threshold.vtrk_dead = 3;//2.9V,the battery parameter
    bat_thr.chg_voltage_threshold.vweak = ADP5360_mV_TO_VWEAK(3050);//2.7V, the battery parameter, better to go higher than lower
#else
    /*
      The charging requirements are as follows:

      Nom capacity 120mAh
      Charging voltage 4.2V
      Nominal Voltage 3.6V
      Max discharge 120mAh
      End Discharge voltage 2.75V

      Charging scheme:
      Constant  charging current of 60mAh until battery voltage reaches 4.2V then, constant voltage of 4.2V until current reaches ~2.4mA.
    */
    bat_thr.chg_termination_set.vtrm = ADP5360_mV_TO_VTRM(4200);//Charging voltage is 4.2V
    bat_thr.chg_termination_set.itrk_dead = 2;
    bat_thr.chg_current_set.iend = ADP5360_mA_TO_IEND(5);//5 mAh constant voltage phase
    bat_thr.chg_current_set.ichg = ADP5360_mA_TO_ICHG(60);//use the 0.5C charge.the battery is 120mAh
    bat_thr.chg_voltage_threshold.dis_rch = 0;//enable charge.
    bat_thr.chg_voltage_threshold.vrch = 1;
    bat_thr.chg_voltage_threshold.vtrk_dead = 2;//2.6V,the battery parameter
    bat_thr.chg_voltage_threshold.vweak = ADP5360_mV_TO_VWEAK(2800);//2.7V, the battery parameter, better to go higher than lower
#endif
#else
    bat_thr.chg_termination_set.vtrm = ADP5360_mV_TO_VTRM(4340);//the battery parameter was 4.35V,need to confirm select 4.34 or 4.36V
    bat_thr.chg_termination_set.itrk_dead = 2;//wait confirm
    bat_thr.chg_current_set.iend = ADP5360_mA_TO_IEND(7.5);//need to confirm use 7.5 or 5V?
    bat_thr.chg_current_set.ichg = ADP5360_mA_TO_ICHG(100);//use the 0.5C charge.the batter was 200mA
    bat_thr.chg_voltage_threshold.dis_rch = 0;//enable charge.
    bat_thr.chg_voltage_threshold.vrch = 1;
    bat_thr.chg_voltage_threshold.vtrk_dead = 3;//2.9V,the battery parameter
    bat_thr.chg_voltage_threshold.vweak = ADP5360_mV_TO_VWEAK(3400);//3.4V,the battery parameter.
#endif
    Adp5360_setBatteryThresholds(&bat_thr);

    ADP5360_CHG_TIMER_SET timer_set;
    timer_set.tend_en = ADP5360_DISABLE;
    timer_set.chg_timer_en = ADP5360_ENABLE;
    timer_set.chg_tmr_period = 1;//160min,the battery parameter
    Adp5360_setChgTimerSet(&timer_set);

    ADP5360_CHG_FUNC_SET func_set;
    func_set.jeita_en = ADP5360_ENABLE;
    func_set.ilim_jeita_cool = 1;
    func_set.off_isofet = 0;
    func_set.ldo_en = ADP5360_ENABLE;
    func_set.eoc_en = ADP5360_ENABLE;
    func_set.adpichg_en = ADP5360_ENABLE;
    func_set.chg_en = ADP5360_ENABLE;
    Adp5360_setChgFunctionalSet(&func_set);

    return ADP5360_SUCCESS;
}

#ifdef TO_BE_USED
ret_code_t Adp5360_enable_batt_charging( bool bEnable ) {

  static ADP5360_CHG_FUNC_SET func_settings;

  // Configure Charger_Functional Settings
  if (Adp5360_getChgFunctionalSet(&func_settings) != ADP5360_SUCCESS) {
    return ADP5360_ERR_I2C;
  }

  func_settings.chg_en = bEnable ? ADP5360_ENABLE : ADP5360_DISABLE;
  if (Adp5360_setChgFunctionalSet(&func_settings) != ADP5360_SUCCESS) {
    return ADP5360_ERR_I2C;
  }

  ADP5360_BAT_PROTECTION bat_protection;

  Adp5360_getBatProtecton(&bat_protection);

  bat_protection.bat_proc_ctrl.chglb_en = bEnable ? ADP5360_ENABLE : ADP5360_DISABLE;
  Adp5360_setBatProtecton(&bat_protection);

  return ADP5360_SUCCESS;
}
#endif

void Adp5360_exit_shipment_mode(void)
{
    uint8_t value;

//    nrf_gpio_pin_clear(ADP5360_ENSD_PIN);//
    value = 0x00;
    write_I2C_ADP5360(ADP5360_REG_SHIPMODE, &value, sizeof(uint8_t));
}
void Adp5360_enter_shipment_mode(void)
{
    uint8_t value;

//    nrf_gpio_pin_set(ADP5360_ENSD_PIN);//
    value = 0x01;
    write_I2C_ADP5360(ADP5360_REG_SHIPMODE, &value, sizeof(uint8_t));
}

void Adp5360_init(void)
{
    Adp5360_pgood_init();
    Adp5360_interrupt_init();
    Adp5360_bat_charger_init();
    Adp5360_bat_proc_init();
    Adp5360_bat_soc_init();
    Adp5360_reset_init();
    Adp5360_output_vol_init();
    Adp5360_exit_shipment_mode();
    Adp5360_port_init();
#ifdef TEMP_PROTECT_FUNC_EN
    battery_temp_detect_task_init();
#endif
    battery_low_voltage_protect_init();
}

/** @brief This function is does check of battery status information
  *
  * @param p_battery_stat pointer to structure of battery status information
  * @return PWR_CTRL_STATUS_t power control return status
  */
ADP5360_RESULT Adp5360_get_battery_details(BATTERY_STATUS_t *p_battery_stat) {
  uint8_t  bat_level = 0;
  //short degreesC;
  uint16_t vBat;
  ADP5360_CHG_STATUS1 nCharger_Status1;
  ADP5360_CHG_STATUS2 nCharger_Status2;

  /* Getting charger status from ADP5360 */
  if (Adp5360_getChargerStatus1(&nCharger_Status1) != ADP5360_SUCCESS) {
    return ADP5360_ERR_I2C;
  }

  /* Getting Battery status from ADP5360 */
  if (Adp5360_getChargerStatus2(&nCharger_Status2) != ADP5360_SUCCESS) {
    return ADP5360_ERR_I2C;
  }

  gnBatteryDetectionFlag = ((nCharger_Status2.bat_chg_status) == ADP5360_BATT_NO_BAT) ? 1: 0; //0-> Battery Detected, 1-> not detected
  // charger mode status
  gn_ChargerStatusCheck = nCharger_Status1.chager_status;

  /*
  0 - Battery Available - Charging starts
  1- no battery */
  if ((gnBatteryDetectionFlag == 1) && (gn_ChargerStatusCheck != ADP5360_CHG_OFF)) {
    p_battery_stat->chrg_status         = BATTERY_NOT_AVAILABLE;
    p_battery_stat->level               = 0;
    p_battery_stat->voltage_mv          = 0;
    //p_battery_stat->temp_c              = 0;
    return ADP5360_SUCCESS;
  }

  switch (gn_ChargerStatusCheck) {
    case ADP5360_CHG_OFF: {
      p_battery_stat->chrg_status        = BATTERY_NOT_CHARGING;
      break;
    } case ADP5360_CHG_TRICKLE:
    case ADP5360_CHG_FAST_CC:
    case ADP5360_CHG_FAST_CV: {
      p_battery_stat->chrg_status        = BATTERY_CHARGING;
      break;
    } case ADP5360_CHG_COMPLETE: {
      p_battery_stat->chrg_status        = BATTERY_CHARGE_COMPLETE;
      break;
    }
    case ADP5360_CHG_LDO_MODE: {
      p_battery_stat->chrg_status        = BATTERY_CHARGER_LDO_MODE;
      break;
    }
    case ADP5360_CHG_TIMER_EXPIRED: {
      p_battery_stat->chrg_status        = BATTERY_CHARGER_TIMER_EXPIRED;
      break;
    } case ADP5360_CHG_BAT_DETECTION: {
      p_battery_stat->chrg_status        = BATTERY_DETECTION;
      break;
    } default: {
      p_battery_stat->chrg_status        = BATTERY_UNKNOWN;
      break;
    }
  }

  if (Adp5360_getBatVoltage(&vBat) != ADP5360_SUCCESS) {
    return ADP5360_ERR_I2C;
  }
  if (Adp5360_getBatSoC(&bat_level) != ADP5360_SUCCESS) {
    return ADP5360_ERR_I2C;
  }
  /*if (adp5350_getSocTemperature(&degreesC) != ADP5360_SUCCESS) {
    return ADP5360_ERR_I2C;
  }*/
  p_battery_stat->level         = bat_level;
  p_battery_stat->voltage_mv       = vBat;

  /* error check for initial zero battery level */
  if (bat_level == 0 && vBat == 0 && gn_ChargerStatusCheck == ADP5360_CHG_OFF) {
    return ADP5360_ERR_I2C;
  }

  //p_battery_stat->temp_c          = degreesC;
  return ADP5360_SUCCESS;
}