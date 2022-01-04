/***************************************************************************//**
 *   @file   AD7156.c
 *   @brief  Implementation of AD7156 Driver.
 *   @author DNechita(Dan.Nechita@analog.com)
********************************************************************************
 * Copyright 2019(c) Analog Devices, Inc.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  - Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  - Neither the name of Analog Devices, Inc. nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *  - The use of this software may or may not infringe the patent rights
 *    of one or more patent holders.  This license does not release you
 *    from the requirement that you obtain separate licenses from these
 *    patent holders to use this software.
 *  - Use of the software either in source or binary form, must be run
 *    on or directly connected to an Analog Devices Inc. component.
 *
 * THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-INFRINGEMENT,
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ANALOG DEVICES BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, INTELLECTUAL PROPERTY RIGHTS, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
********************************************************************************
 *   SVN Revision: 793
*******************************************************************************/

/******************************************************************************/
/***************************** Include Files **********************************/
/******************************************************************************/
#include "ad7156.h"

#ifdef VSM_MBOARD
#include "watch_vsm_motherboard_pin_config.h"
#elif PCBA
#include "watch_board_pcba_pin_config.h"
#else
#include "watch_board_evt_pin_config.h"
#endif
#include "nrf_drv_twi.h"

#include "sdk_common.h"
#include "nrf_drv_gpiote.h"

#include <adi_osal.h>
#include <app_cfg.h>
#include <task_includes.h>
#include "hal_twi0.h"

#include "nrf_log.h"
#if NRF_LOG_ENABLED
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#endif

#define AD7156_ENERGY_SAVING_ENABLE (0)

#ifdef DVT
#define AD7156_DETECT_METHOD (AD7156_ADAPTIVE_THRESHOLD)//use adaptive mode
#else
#define AD7156_DETECT_METHOD (AD7156_FIXED_THRESHOLD)//use adaptive mode
#endif

#if (AD7156_ENERGY_SAVING_ENABLE != 0)
ADI_OSAL_STATIC_THREAD_ATTR ad7156_task_attributes;
uint8_t ad7156_task_stack[APP_OS_CFG_AD7156_TASK_STK_SIZE];
StaticTask_t ad7156_task_tcb;
ADI_OSAL_THREAD_HANDLE ad7156_task_handler;
#endif

#define OUT_PIN_USER_MAX (2)
static out_pin_detect_func out1_user_handle[OUT_PIN_USER_MAX] = {NULL};
static out_pin_detect_func out2_user_handle[OUT_PIN_USER_MAX] = {NULL};

void Register_out1_pin_detect_func(out_pin_detect_func hander)
{
    for(int i = 0;i<OUT_PIN_USER_MAX;i++)
    {
        if(NULL == out1_user_handle[i])
        {
            out1_user_handle[i] = hander;
            break;
        }
    }
}

void Unregister_out1_pin_detect_func(out_pin_detect_func hander)
{
    for(int i = 0;i<OUT_PIN_USER_MAX;i++)
    {
        if(hander == out1_user_handle[i])
        {
            out1_user_handle[i] = NULL;
            break;
        }
    }
}

void Register_out2_pin_detect_func(out_pin_detect_func hander)
{
    for(int i = 0;i<OUT_PIN_USER_MAX;i++)
    {
        if(NULL == out2_user_handle[i])
        {
            out2_user_handle[i] = hander;
            break;
        }
    }
}

void Unregister_out2_pin_detect_func(out_pin_detect_func hander)
{
    for(int i = 0;i<OUT_PIN_USER_MAX;i++)
    {
        if(hander == out2_user_handle[i])
        {
            out2_user_handle[i] = NULL;
            break;
        }
    }
}

/******************************************************************************/
/************************ Variables Declarations ******************************/
/******************************************************************************/
uint16_t ad7156Channel1Range = 2000;//use the fF to replace it.
uint16_t ad7156Channel2Range = 2000;//use the fF to replace it.
//unsigned char thrMode = 0;
/******************************************************************************/
/************************ Functions Definitions *******************************/
/******************************************************************************/

/***************************************************************************//**
 * @brief Performs a burst read of a specified number of registers.
 *
 * @param pReadData - The read values are stored in this buffer.
 * @param registerAddress - The start address of the burst read.
 * @param bytesNumber - Number of bytes to read.
 *
 * @return None.
*******************************************************************************/
void AD7156_GetRegisterValue(unsigned char* pReadData,
                             unsigned char registerAddress,
                             unsigned char bytesNumber)
{
    twi0_read_register(AD7156_ADDRESS,registerAddress,pReadData,bytesNumber);
}

/*****************************************************************************
 * @brief Writes data into one or two registers.
 *
 * @param registerValue - Data value to write.
 * @param registerAddress - Address of the register.
 * @param bytesNumber - Number of bytes. Accepted values: 0 - 1.
 *
 * @return None.
*******************************************************************************/
void AD7156_SetRegisterValue(unsigned short registerValue,
                             unsigned char  registerAddress,
                             unsigned char  bytesNumber)
{
    unsigned char dataBuffer[2] = {0,0};

    dataBuffer[0] = (registerValue >> 8);
    dataBuffer[bytesNumber-1] = (uint8_t )(registerValue & 0x00FF);
    twi0_write_register(AD7156_ADDRESS,registerAddress,dataBuffer,bytesNumber);
}

/***************************************************************************//**
 * @brief Resets the device.
 *
 * @return None.
*******************************************************************************/
void AD7156_Reset(void)
{
    unsigned char addressPointer = 0;

    addressPointer = AD7156_RESET_CMD;
    twi0_send(AD7156_ADDRESS, &addressPointer, 1);
}

/***************************************************************************//**
 * @brief Sets the converter mode of operation.
 *
 * @oaram pwrMode - Mode of operation option.
 *		    Example: AD7156_CONV_MODE_IDLE - Idle
 *                           AD7156_CONV_MODE_CONT_CONV  - Continuous conversion
 *                           AD7156_CONV_MODE_SINGLE_CONV - Single conversion
 *                           AD7156_CONV_MODE_PWR_DWN - Power-down
 *
 * @return None.
*******************************************************************************/
void AD7156_SetPowerMode(unsigned char pwrMode)
{
    unsigned char oldConfigReg = 0;
    unsigned char newConfigReg = 0;

    AD7156_GetRegisterValue(&oldConfigReg, AD7156_REG_CONFIG, 1);
    oldConfigReg &= ~AD7156_CONFIG_MD(0x3);
    newConfigReg = oldConfigReg| AD7156_CONFIG_MD(pwrMode);
//    thrMode = pwrMode;
    AD7156_SetRegisterValue(newConfigReg, AD7156_REG_CONFIG, 1);
}

/***************************************************************************//**
 * @brief Enables or disables conversion on the selected channel.
 *
 * @param channel - Channel option.
 *                  Example: AD7156_CHANNEL1
 *                           AD7156_CHANNEL2
 * @param enableConv - The state of channel activity.
 *                      Example: 0 - disable conversion on selected channel.
 *                               1 - enable conversion on selected channel.
 *
 * @return None.
*******************************************************************************/
void AD7156_ChannelState(unsigned char channel, unsigned char enableConv)
{
    unsigned char oldConfigReg = 0;
    unsigned char newConfigReg = 0;
    unsigned char channelMask  = 0;

    channelMask = (channel == 1) ? AD7156_CONFIG_EN_CH1 : AD7156_CONFIG_EN_CH2;

    AD7156_GetRegisterValue(&oldConfigReg, AD7156_REG_CONFIG, 1);
    oldConfigReg &= ~channelMask;
    newConfigReg = oldConfigReg | (channelMask * enableConv);
    AD7156_SetRegisterValue(newConfigReg, AD7156_REG_CONFIG, 1);
}

/***************************************************************************//**
 * @brief Sets the input range of the specified channel.
 *
 * @param channel - Channel option.
 *                  Example: AD7156_CHANNEL1
 *                           AD7156_CHANNEL2
 * @param range - Input range option.
 *                Example: AD7156_CDC_RANGE_2_PF   - 2pF input range.
 *                         AD7156_CDC_RANGE_0_5_PF - 0.5pF input range.
 *                         AD7156_CDC_RANGE_1_PF   - 1pF input range.
 *                         AD7156_CDC_RANGE_4_PF   - 4pF input range.
 *
 * @return None.
*******************************************************************************/
void AD7156_SetRange(unsigned channel, unsigned char range)
{
    unsigned char oldSetupReg = 0;
    unsigned char newSetupReg = 0;
    unsigned char regAddress  = 0;

    regAddress = (channel == 1) ? AD7156_REG_CH1_SETUP : AD7156_REG_CH2_SETUP;
    AD7156_GetRegisterValue(&oldSetupReg, regAddress, 1);
    oldSetupReg &= ~AD7156_CH1_SETUP_RANGE(0x3);
    newSetupReg = oldSetupReg | AD7156_CH1_SETUP_RANGE(range);
    AD7156_SetRegisterValue(newSetupReg, regAddress, 1);
    /* Update global variables that hold range information. */
    if(channel == 1)
    {
        ad7156Channel1Range = AD7156_GetRange(channel);
    }
    else
    {
        ad7156Channel2Range = AD7156_GetRange(channel);
    }
}

/***************************************************************************//**
 * @brief Reads the range bits from the device and returns the range in fF.
 *
 * @param channel - Channel option.
 *                  Example: AD7156_CHANNEL1
 *                           AD7156_CHANNEL2
 *
 * @return The capacitive input range(fF).
*******************************************************************************/
uint16_t AD7156_GetRange(unsigned channel)
{
    unsigned char setupReg    = 0;
    unsigned char regAddress  = 0;
    uint16_t range = 0;

    regAddress = (channel == 1) ? AD7156_REG_CH1_SETUP : AD7156_REG_CH2_SETUP;
    AD7156_GetRegisterValue(&setupReg, regAddress, 1);
    setupReg = (setupReg & AD7156_CH1_SETUP_RANGE(0x3)) >> 6;
    switch(setupReg)
    {
        case AD7156_CDC_RANGE_2_PF:
            range =  2000;
            break;
        case AD7156_CDC_RANGE_0_5_PF:
            range = 500;
            break;
        case AD7156_CDC_RANGE_1_PF:
            range =  1000;
            break;
        case AD7156_CDC_RANGE_4_PF:
            range =  4000;
            break;
        default:
            range = 2000;
            break;
    }
    /* Update global variables that hold range information. */
    if(channel == 1)
    {
        ad7156Channel1Range = range;
    }
    else
    {
        ad7156Channel2Range = range;
    }

    return range;
}

/***************************************************************************//**
 * @brief Selects the threshold mode of operation.
 *
 * @param thrMode - Output comparator mode.
 *                  Example: AD7156_THR_MODE_NEGATIVE
 *                           AD7156_THR_MODE_POSITIVE
 *                           AD7156_THR_MODE_IN_WINDOW
 *                           AD7156_THR_MODE_OU_WINDOW
 * @param thrFixed - Selects the threshold mode.
 *                   Example: AD7156_ADAPTIVE_THRESHOLD
 *                            AD7156_FIXED_THRESHOLD
 *
 * @return None.
*******************************************************************************/
void AD7156_SetThresholdMode(unsigned char thrMode, unsigned char thrFixed)
{
    unsigned char oldConfigReg = 0;
    unsigned char newConfigReg = 0;

    AD7156_GetRegisterValue(&oldConfigReg, AD7156_REG_CONFIG, 1);
    oldConfigReg &= ~(AD7156_CONFIG_THR_FIXED | AD7156_CONFIG_THR_MD(0x3));
    newConfigReg = oldConfigReg |
                   (AD7156_CONFIG_THR_FIXED * thrFixed) |
                   (AD7156_CONFIG_THR_MD(thrMode));
    AD7156_SetRegisterValue(newConfigReg, AD7156_REG_CONFIG, 1);
}
/***************************************************************************//**
 * @brief set the power down timeout time.
 *
 * @param time_4h - 0:disable power-down timeout
                    interval:4h.
 * @return None.
*******************************************************************************/

void AD7156_SetPowerDownTimeout(unsigned char time_4h)
{
    AD7156_SetRegisterValue(AD7156_PWR_DWN_TMR_TIMEOUT(time_4h), AD7156_REG_PWR_DWN_TMR, 1);
}
/***************************************************************************//**
 * @brief set CAPDAC function.
 *
 * @param channel - Channel option.
 *                  Example: AD7156_CHANNEL1
 *                           AD7156_CHANNEL2
 * @param dacEn - enables capacitive the DAC
 * @param dacAutoEn -enables the auto-DAC function in the adaptive threshold mode.
 * @param dacValue -CAPDAC value.Code 0x00 ~= 0 pF, Code 0x3F ~= CAPDAC full range(12.6pF),so the interval is 0.2pF
 * @return None.
*******************************************************************************/

void AD7156_SetCAPDAC(unsigned char channel,unsigned char dacEn,unsigned char dacAutoEn,unsigned char dacValue)
{
    unsigned char  regAddress  = 0;
    unsigned short value         = 0;

    regAddress = (channel == 1) ? AD7156_REG_CH1_CAPDAC :AD7156_REG_CH2_CAPDAC;
    value = AD7156_CAPDAC_DAC_EN(dacEn)|AD7156_CAPDAC_DAC_AUTO(dacAutoEn)|AD7156_CAPDAC_DAC_VAL(dacValue);
    AD7156_SetRegisterValue(value, regAddress, 1);
}

/***************************************************************************//**
 * @brief Writes to the threshold register when threshold fixed mode is enabled.
 *
 * @param channel - Channel option.
 *                  Example: AD7156_CHANNEL1
 *                           AD7156_CHANNEL2
 * @param pFthr - The threshold value in femto farads(fF). The value must not be
 *                out of the selected input range.
 *
 * @return None.
*******************************************************************************/
void AD7156_SetThreshold(unsigned char channel, uint16_t fFthr)
{
    unsigned char  thrRegAddress  = 0;
    unsigned short rawThr         = 0;
    uint16_t  range               = 0;

    thrRegAddress = (channel == 1) ? AD7156_REG_CH1_SENS_THRSH_H :
                                     AD7156_REG_CH2_SENS_THRSH_H;
    range = AD7156_GetRange(channel); //range in fF
    rawThr = (uint16_t)((fFthr * 0xA000 / range) + 0x3000);
    if(rawThr > 0xD000)
    {
        rawThr = 0xD000;
    }
    else if(rawThr < 0x3000)
    {
        rawThr = 0x3000;
    }
    AD7156_SetRegisterValue(rawThr, thrRegAddress, 2);
}

/***************************************************************************//**
 * @brief Writes a value(fF) to the sensitivity register. This functions
 * should be used when adaptive threshold mode is selected.
 *
 * @param channel - Channel option.
 *                  Example: AD7156_CHANNEL1
 *                           AD7156_CHANNEL2
 * @param pFsensitivity - The sensitivity value in femtofarads(fF).
 *
 * @return None.
*******************************************************************************/
void AD7156_SetSensitivity(unsigned char channel, uint16_t fFsensitivity)
{
    unsigned char  sensitivityRegAddr = 0;
    unsigned short rawSensitivity     = 0;
    uint16_t range = 0;

    sensitivityRegAddr = (channel == 1) ? AD7156_REG_CH1_SENS_THRSH_H :
                                          AD7156_REG_CH2_SENS_THRSH_H;
    range = (channel == 1) ? ad7156Channel1Range : ad7156Channel2Range;
    rawSensitivity = (unsigned short)(fFsensitivity * 0xA00 / range);
//    rawSensitivity = (rawSensitivity << 4) & 0x0FF0;//remove//20190505
//    AD7156_SetRegisterValue(rawSensitivity, sensitivityRegAddr, 2);//remove//20190505
    AD7156_SetRegisterValue(rawSensitivity, sensitivityRegAddr, 1);//add//20190505
}

/***************************************************************************//**
 * @brief Writes timeout approaching and timout receding to a specific channel.
 *        This functions
 *        should be used when adaptive threshold mode is selected.
 *
 * @param channel - Channel option.
 *                  Example: AD7156_CHANNEL1
 *                           AD7156_CHANNEL2
 * @param TimeOutApr - Timeout approaching
 * @param TimeOutRec - Timeout receding
 *
 * @return None.
*******************************************************************************/
void AD7156_SetTimeout(unsigned char channel, unsigned char TimeOutApr,unsigned char TimeOutRec)
{
    unsigned char  RegAddr = 0;
    unsigned short value     = 0;


    RegAddr = (channel == 1) ? AD7156_REG_CH1_TMO_THRSH_L :AD7156_REG_CH2_TMO_THRSH_L;

    value = (TimeOutApr << 4)|(TimeOutRec&0x0f);
    AD7156_SetRegisterValue(value, RegAddr, 1);
}


/***************************************************************************//**
 * @brief Reads the 12-bit samples form both channels.
 *
 * @param ch1 - The 12-bit sample of channel 1 is stored in this variable. The
 *              12 bits are left justified.
 * @param ch2 - The 12-bit sample of channel 2 is stored in this variable. The
 *              12 bits are left justified.
 *
 * @return None.
*******************************************************************************/
void AD7156_ReadChannels(uint16_t* ch1, uint16_t* ch2)
{
    unsigned char regData[4] = {0, 0, 0, 0};
    unsigned char status     = 0;

    do
    {
        AD7156_GetRegisterValue(&status, AD7156_REG_STATUS, 1);
    }while((status & AD7156_STATUS_RDY2) == 0);
    AD7156_GetRegisterValue(regData, AD7156_REG_CH1_DATA_H, 4);
    *ch1 = (uint16_t)(regData[0] << 8) + regData[1];
    *ch2 = (uint16_t)(regData[2] << 8) + regData[3];
}

/***************************************************************************//**
 * @brief Reads a 12-bit sample from the selected channel.
 *
 * @param channel - Channel option.
 *                  Example: AD7156_CHANNEL1
 *                           AD7156_CHANNEL2
 * @return Conversion result form the selected channel.
*******************************************************************************/
unsigned short AD7156_ReadOneChannel(unsigned char channel)
{
    unsigned short chResult   = 0;
    unsigned char  regData[2] = {0, 0};
    unsigned char  status     = 0;
    unsigned char  chRdyMask  = 0;
    unsigned char  chAddress  = 0;

    if(channel == 1)
    {
        chRdyMask = AD7156_STATUS_RDY1;
        chAddress = AD7156_REG_CH1_DATA_H;
    }
    else
    {
        chRdyMask = AD7156_STATUS_RDY2;
        chAddress = AD7156_REG_CH2_DATA_H;
    }
    do
    {
        AD7156_GetRegisterValue(&status, AD7156_REG_STATUS, 1);
    }while((status & chRdyMask) == 0);
    AD7156_GetRegisterValue(regData, chAddress, 2);
    chResult = (regData[0] << 8) + regData[1];

    return chResult;
}

/***************************************************************************//**
 * @brief Read data form both channels and converts it to femtofarads(fF).
 *
 * @param ch1 - Stores the read capacitance(fF) from channel 1.
 * @param ch2 - Stores the read capacitance(fF) from channel 2.
 *
 * @return None.
*******************************************************************************/
void AD7156_ReadCapacitance(uint16_t* ch1, uint16_t* ch2)
{
    unsigned short rawCh1   = 0;
    unsigned short rawCh2   = 0;

    AD7156_ReadChannels(&rawCh1, &rawCh2);
    if(rawCh1 < 0x3000)
    {
        rawCh1 = 0x3000;
    }
    else if(rawCh1 > 0xD000)
    {
        rawCh1 = 0xD000;
    }
    if(rawCh2 < 0x3000)
    {
        rawCh2 = 0x3000;
    }
    else if(rawCh2 > 0xD000)
    {
        rawCh2 = 0xD000;
    }
    *ch1 = (((rawCh1) - 0x3000) * ad7156Channel1Range) / 0xA000;
    *ch2 = (((rawCh2) - 0x3000) * ad7156Channel2Range) / 0xA000;
}

/***************************************************************************//**
 * @brief Reads a sample and converts the data to femtofarads(fF).
 *
 * @param channel - Channel option.
 *                  Example: AD7156_CHANNEL1
 *                           AD7156_CHANNEL2
 * @return Conversion result form the selected channel as femtofarads(fF).
*******************************************************************************/
uint16_t AD7156_ReadChannelCap(unsigned char channel)
{
    unsigned short rawCh = 0;
    uint16_t chRange = 0;
    uint16_t fFdata = 0;

    chRange = (channel == 1) ? ad7156Channel1Range : ad7156Channel2Range;
    rawCh = AD7156_ReadOneChannel(channel);
    if(rawCh < 0x3000)
    {
        rawCh= 0x3000;
    }
    else if(rawCh > 0xD000)
    {
        rawCh = 0xD000;
    }
    fFdata = (((rawCh) - 0x3000) * chRange) / 0xA000;

    return fFdata;
}

static void ad7156_out1_event_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    uint8_t value = 0;

    value = nrf_drv_gpiote_in_is_set(pin);//0:release.1:press

    for(int i = 0;i<OUT_PIN_USER_MAX;i++)
    {
        if(NULL != out1_user_handle[i])
        {
            out1_user_handle[i](value);
        }
    }
}

static void ad7156_out2_event_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    uint8_t value = 0;

    value = nrf_drv_gpiote_in_is_set(pin);//0:release.1:press

    for(int i = 0;i<OUT_PIN_USER_MAX;i++)
    {
        if(NULL != out2_user_handle[i])
        {
            out2_user_handle[i](value);
        }
    }
}

/***************************************************************************//**
 * @brief Initializes the communication peripheral and checks if the device is
 *        present.
 *
 * @return status - The result of the initialization procedure.
 *                  Example: 0x0 - I2C peripheral was not initialized or the
 *                                 device is not present.
 *                           0x1 - I2C peripheral was initialized and the
 *                                 device is present.
*******************************************************************************/
uint32_t ad7156_out1_pin_status_get(void)
{
    return nrf_gpio_pin_read(AD7156_OUT1_PIN);
}

uint32_t ad7156_out2_pin_status_get(void)
{
    return nrf_gpio_pin_read(AD7156_OUT2_PIN);
}

uint32_t AD7156_detect_pin_init(void)
{
    uint32_t err_code = NRF_SUCCESS;

    /* GPIO output trigger for high to low transition */
    nrf_drv_gpiote_in_config_t config = GPIOTE_CONFIG_IN_SENSE_TOGGLE(false);
    if (!nrf_drv_gpiote_is_init())
    {
        err_code = nrf_drv_gpiote_init();
        if (err_code != NRF_SUCCESS)
        {
            return err_code;
        }
    }
    err_code = nrf_drv_gpiote_in_init(AD7156_OUT1_PIN, &config, ad7156_out1_event_handler);
    if (err_code != NRF_SUCCESS)
    {
        NRF_LOG_INFO("AD7156_OUT1_PIN init failue,err_code = %d",err_code);
        return err_code;
    }

    err_code = nrf_drv_gpiote_in_init(AD7156_OUT2_PIN, &config, ad7156_out2_event_handler);
    if (err_code != NRF_SUCCESS)
    {
        NRF_LOG_INFO("AD7156_OUT2_PIN init failue,err_code = %d",err_code);
        return err_code;
    }
    //nrf_drv_gpiote_in_event_enable(AD7156_OUT1_PIN, true);
    //nrf_drv_gpiote_in_event_enable(AD7156_OUT2_PIN, true);
    return err_code;
}

void Ad7156_detect_set(uint8_t channel,uint8_t en)
{
    if(AD7156_CHANNEL1 == channel)
    {
        if(AD7156_DISABLE != en)
        {
            nrf_drv_gpiote_in_event_enable(AD7156_OUT1_PIN, true);
        }
        else
        {
            nrf_drv_gpiote_in_event_enable(AD7156_OUT1_PIN, false);
        }
    }
    else if(AD7156_CHANNEL2 == channel)
    {
        if(AD7156_DISABLE != en)
        {
            nrf_drv_gpiote_in_event_enable(AD7156_OUT2_PIN, true);
        }
        else
        {
            nrf_drv_gpiote_in_event_enable(AD7156_OUT2_PIN, false);
        }
    }
    else
    {
    }
}
void Ad7156DrvDevDisable(void)
{
  AD7156_SetPowerMode(AD7156_CONV_MODE_PWR_DWN);
}

void Ad7156DrvDevEnable(void)
{
  AD7156_SetPowerMode(AD7156_CONV_MODE_CONT_CONV);
}
#if (AD7156_ENERGY_SAVING_ENABLE != 0)
static void ad7156_thread(void * arg)
{
    UNUSED_PARAMETER(arg);
    while(1)
    {
        AD7156_SetPowerMode(AD7156_CONV_MODE_CONT_CONV);

        vTaskDelay(20);

//        float value1,value2;
//        uint32_t val1,val2;
//        AD7156_ReadCapacitance(&value1,&value2);
//        val1 = (uint32_t) (value1*1000);
//        val2 = (uint32_t) (value2*1000);
//        NRF_LOG_INFO("val1 = %d;val2=%d",val1,val2);
        AD7156_SetPowerMode(AD7156_CONV_MODE_PWR_DWN);

//        AD7156_GetRegisterValue(data, 0, 24);
//        NRF_LOG_HEXDUMP_INFO(data,24);
        vTaskDelay(980);
    }
}

void AD7156_task_init(void) {
    ADI_OSAL_STATUS eOsStatus;

    /* Create USBD tx thread */
    ad7156_task_attributes.pThreadFunc = ad7156_thread;
    ad7156_task_attributes.nPriority = APP_OS_CFG_TOUCH_TASK_PRIO;
    ad7156_task_attributes.pStackBase = &ad7156_task_stack[0];
    ad7156_task_attributes.nStackSize = APP_OS_CFG_TOUCH_TASK_STK_SIZE;
    ad7156_task_attributes.pTaskAttrParam = NULL;
    /* Thread Name should be of max 10 Characters */
    ad7156_task_attributes.szThreadName = "ad7156";
    ad7156_task_attributes.pThreadTcb = &ad7156_task_tcb;

    eOsStatus = adi_osal_ThreadCreateStatic(&ad7156_task_handler,
                                &ad7156_task_attributes);
    if (eOsStatus != ADI_OSAL_SUCCESS)
    {
        Debug_Handler();
    }
}
#endif
uint32_t AD7156_Init(void)
{
    uint32_t err_code = NRF_SUCCESS;
    unsigned char chipID   = 0;

    AD7156_GetRegisterValue(&chipID, AD7156_REG_CHIP_ID, 1);

    if(chipID != AD7156_DEFAULT_ID)
    {
        NRF_LOG_INFO("AD7156 chip ID=0x%x",chipID);
        return 0;
    }

#if (AD7156_ENERGY_SAVING_ENABLE != 0)
    AD7156_SetPowerMode(AD7156_CONV_MODE_PWR_DWN);
#else
    AD7156_SetPowerMode(AD7156_CONV_MODE_CONT_CONV);
#endif
    /* AD7156_THR_MODE_IN_WINDOW - Output Active when
         Data > average - sensitivity
          and
         Data < average + sensitivity
    */
    AD7156_SetThresholdMode(AD7156_THR_MODE_IN_WINDOW,AD7156_DETECT_METHOD);
    AD7156_SetPowerDownTimeout(0);

    AD7156_SetCAPDAC(AD7156_CHANNEL1,AD7156_ENABLE,AD7156_ENABLE,0);
    AD7156_SetCAPDAC(AD7156_CHANNEL2,AD7156_ENABLE,AD7156_ENABLE,0);

    AD7156_SetRange(AD7156_CHANNEL2,AD7156_CDC_RANGE_4_PF);
    AD7156_SetRange(AD7156_CHANNEL1,AD7156_CDC_RANGE_4_PF);

#if (AD7156_ADAPTIVE_THRESHOLD == AD7156_DETECT_METHOD)
    AD7156_SetTimeout(AD7156_CHANNEL1,0x05,0x06);//conversion time is 10 or 20ms,TimeOutRec=(2^6)*20ms=1.28s,TimeOutApr=(2^5)*20ms=0.64s
    AD7156_SetTimeout(AD7156_CHANNEL2,0x07,0x06);//conversion time is 10 or 20ms,TimeOutRec=(2^6)*20ms=1.28s,TimeOutApr=(2^7)*20ms=2.56s

    AD7156_SetSensitivity(AD7156_CHANNEL1,40);//max change value is 0.06pF
    AD7156_SetSensitivity(AD7156_CHANNEL2,25);//use the finger to touch, the max change value is 0.03pF, but wear to arm, the change will become small.
#else
    AD7156_SetThresholdMode(AD7156_THR_MODE_NEGATIVE,AD7156_DETECT_METHOD);
    AD7156_SetThreshold(AD7156_CHANNEL1,1150);
    AD7156_SetThreshold(AD7156_CHANNEL2, 450);
#endif
    AD7156_ChannelState(AD7156_CHANNEL1,1);
    AD7156_ChannelState(AD7156_CHANNEL2,1);
#if (AD7156_ENERGY_SAVING_ENABLE != 0)
    AD7156_task_init();
#endif
    err_code = AD7156_detect_pin_init();
    return err_code;
}

uint8_t getAD7156ChipID()
{
  uint8_t chipID = 0;
  AD7156_GetRegisterValue(&chipID, AD7156_REG_CHIP_ID, 1);

  return chipID;
}
