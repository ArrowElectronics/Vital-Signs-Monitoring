/**
****************************************************************************
* @file     adi_ad5940_main.c
* @author   ADI
* @version  V0.1
* @date     04-Dec-2019
* @brief    Used to control specific application and process data.
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

/**
* @addtogroup AD5940_System_Examples
* @{
*  @defgroup BioElec_Example
*  @{
*/
#ifdef ENABLE_ECG_APP
#include "ad5940.h"
#include "AD5940.h"
#include <stdio.h>
#include "string.h"
#include "math.h"
#include "adi_ecg.h"
#include <rtc.h>

#define APPBUFF_SIZE 1024
static uint32_t AppBuff[APPBUFF_SIZE];
static float LFOSCFreq;    /* Measured LFOSC frequency */
extern AppECGCfg_Type AppECGCfg;

/* print ECG result to uart */
AD5940Err ECGShowResult(void *pData, uint32_t DataCount)
{
  /*Process data*/
  for(int i=0;i<DataCount;i++)
  {
    printf("%d \n", ((uint32_t *)pData)[i]);
  }
  return 0;
}

/* Initialize AD5940 basic blocks like clock */
static int32_t AD5940PlatformCfg(void)
{
  CLKCfg_Type clk_cfg;
  FIFOCfg_Type fifo_cfg;
  SEQCfg_Type seq_cfg;
  AGPIOCfg_Type gpio_cfg;
  LFOSCMeasure_Type LfoscMeasure;
  SeqGpioTrig_Cfg cfg;

  /* Use hardware reset */
  AD5940_HWReset();
  /* Platform configuration */
  AD5940_Initialize();
  /* Step1. Configure clock */
  clk_cfg.ADCClkDiv = ADCCLKDIV_1;
  clk_cfg.ADCCLkSrc = ADCCLKSRC_HFOSC;
  clk_cfg.SysClkDiv = SYSCLKDIV_1;
  clk_cfg.SysClkSrc = SYSCLKSRC_HFOSC;
  clk_cfg.HfOSC32MHzMode = bFALSE;
  clk_cfg.HFOSCEn = bTRUE;
  clk_cfg.HFXTALEn = bFALSE;
  clk_cfg.LFOSCEn = bTRUE;
  AD5940_CLKCfg(&clk_cfg);
  /* Step2. Configure FIFO and Sequencer*/
  fifo_cfg.FIFOEn = bFALSE;
  fifo_cfg.FIFOMode = FIFOMODE_FIFO;
  fifo_cfg.FIFOSize = FIFOSIZE_4KB;                       /* 4kB for FIFO, The reset 2kB for sequencer */
  fifo_cfg.FIFOSrc = FIFOSRC_SINC3;
  fifo_cfg.FIFOThresh = 256;//AppBIACfg.FifoThresh;
  AD5940_FIFOCfg(&fifo_cfg);                             /* Disable to reset FIFO. */
  fifo_cfg.FIFOEn = bTRUE;
  AD5940_FIFOCfg(&fifo_cfg);                             /* Enable FIFO here */
  /* Configure sequencer and stop it */
  seq_cfg.SeqMemSize = SEQMEMSIZE_2KB;
  seq_cfg.SeqBreakEn = bFALSE;
  seq_cfg.SeqIgnoreEn = bFALSE;
  seq_cfg.SeqCntCRCClr = bTRUE;
  seq_cfg.SeqEnable = bFALSE;
  seq_cfg.SeqWrTimer = 0;
  AD5940_SEQCfg(&seq_cfg);

  /* Step3. Interrupt controller */
  AD5940_INTCCfg(AFEINTC_1, AFEINTSRC_ALLINT, bTRUE);           /* Enable all interrupt in Interrupt Controller 1, so we can check INTC flags */
  AD5940_INTCCfg(AFEINTC_0, AFEINTSRC_DATAFIFOTHRESH, bTRUE);   /* Interrupt Controller 0 will control GP0 to generate interrupt to MCU */
  AD5940_INTCClrFlag(AFEINTSRC_ALLINT);
  /* Step4: Reconfigure GPIO */
  gpio_cfg.FuncSet = GP0_TRIG;
  gpio_cfg.InputEnSet = AGPIO_Pin0;
  gpio_cfg.OutputEnSet = 0;
  gpio_cfg.OutVal = 0;
  gpio_cfg.PullEnSet = AGPIO_Pin0;
  AD5940_AGPIOCfg(&gpio_cfg);

  cfg.bEnable = bTRUE;
  cfg.PinSel = AGPIO_Pin0;
  cfg.SeqPinTrigMode = SEQPINTRIGMODE_FALLING;
  AD5940_SEQGpioTrigCfg(&cfg);

  AD5940_SleepKeyCtrlS(SLPKEY_UNLOCK);  /* Enable AFE to enter sleep mode. */

  /* Measure LFOSC frequency */
  LfoscMeasure.CalDuration = 1000.0;  /* 1000ms used for calibration. */
  LfoscMeasure.CalSeqAddr = 0;
  LfoscMeasure.SystemClkFreq = 16000000.0f; /* 16MHz in this firmware. */
  AD5940_LFOSCMeasure(&LfoscMeasure, &LFOSCFreq);
  //printf("Freq:%f\n", LFOSCFreq);

  return 0;
}



void enable_ecg_ext_trigger(uint16_t nOdr);
extern volatile int16_t user_applied_odr;
extern volatile int16_t user_applied_adcPgaGain;
extern volatile int16_t user_applied_ecg_pwr_mod;

/*!
 ****************************************************************************
 *@brief      AD5940 structure initialization
 *@param      None
 *@return     None
 ******************************************************************************/
void AD5940ECGStructInit(void) {
  AppECGCfg_Type *pCfg;
  AppECGGetCfg(&pCfg);
  pCfg->LfoscClkFreq = LFOSCFreq;

  if (!user_applied_odr) {
    pCfg->ECGODR = 100.0;
  } else {
    user_applied_odr = 0;
  }
  if (!user_applied_adcPgaGain) {
    pCfg->AdcPgaGain = ADCPGA_1;
  } else {
    user_applied_adcPgaGain = 0;
  }
}

void ad5940_ecg_start(void)
{
  AD5940PlatformCfg();

   /* Configure your parameters in this function */
  AD5940ECGStructInit();
  AppECGInit(AppBuff, APPBUFF_SIZE);    /* Initialize BIA application. Provide a buffer, which is used to store sequencer commands */
  enable_ecg_ext_trigger(AppECGCfg.ECGODR);
}

//#define ECG_POLLING
#ifndef ECG_POLLING
struct gTrigInfo{
uint32_t gaTrigTime[100];
uint32_t gaTrigCnt[100];
uint16_t nIndex;
}IntArr;
extern uint32_t gad5940TriggerTimerCnt;
#endif

uint8_t nIntFlag = 0;
void ad5940_fetch_data(void)
{
uint32_t temp;
nIntFlag = 0;
IntArr.nIndex = 0;
#ifdef ECG_POLLING
  while(1)
#else
//  while(!nIntFlag)
#endif
  {
    /* Check if interrupt flag which will be set when interrupt occurred. */
    if(AD5940_INTCTestFlag(AFEINTC_0, AFEINTSRC_DATAFIFOTHRESH))
    {
      IntArr.gaTrigTime[IntArr.nIndex] = get_sensor_time_stamp();
      IntArr.gaTrigCnt[IntArr.nIndex] = gad5940TriggerTimerCnt;
      if(++IntArr.nIndex >= 99)
        IntArr.nIndex = 0;
      nIntFlag = 1;
      temp = APPBUFF_SIZE;
      AppECGISR(AppBuff, &temp); /* Deal with it and provide a buffer to store data we got */
      ECGShowResult(AppBuff, temp); /* Show the results to UART */
    }
  }
}

void AD5940_Main(void)
{
  void AD5940_TrigSequence(void);
  uint32_t temp;

  AD5940PlatformCfg();

  AD5940ECGStructInit(); /* Configure your parameters in this function */

  AppECGInit(AppBuff, APPBUFF_SIZE);    /* Initialize BIA application. Provide a buffer, which is used to store sequencer commands */
  //AppECGCtrl(APPCTRL_START, 0);         /* Control BIA measurement to start. Second parameter has no meaning with this command. */
  enable_ecg_ext_trigger (100);
  while(1)
  {
    /* Check if interrupt flag which will be set when interrupt occurred. */
    if(AD5940_INTCTestFlag(AFEINTC_0, AFEINTSRC_DATAFIFOTHRESH))
    {
      temp = APPBUFF_SIZE;
      AppECGISR(AppBuff, &temp); /* Deal with it and provide a buffer to store data we got */
      ECGShowResult(AppBuff, temp); /* Show the results to UART */
    }
    //AD5940_Delay10us(10000);
    //AD5940_TrigSequence();
  }
}
#endif
/**
* @}
* @}
* */

