/**
****************************************************************************
* @file     adi_bcm.c
* @author   ADI
* @version  V0.1
* @date     11-Jun-2021
* @brief    BIA Measurement using AD5940.
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
#ifdef ENABLE_BIA_APP
#include "ad5940.h"
#include "us_tick.h"
#include <stdio.h>
#include "string.h"
#include "math.h"
#include "rtc.h"
#include <ecg_task.h>
#include "bia_application_task.h"
#include "app_bia.h"
#include "dcb_interface.h"

/* BIA parameter Configurations */
extern AppBIACfg_Type AppBIACfg;
extern volatile int8_t bia_user_applied_odr;
extern volatile int8_t bia_user_applied_dft_num;
extern uint32_t AppBuff[APPBUFF_SIZE];
uint32_t ad5940_port_Init(void);

  /*!
  ****************************************************************************
 *  @brief Control application like start, stop

 *  @param  BIACtrl The command for this application, select from below paramters
 *            - APPCTRL_START: start the measurement. Note: the ramp test need
 *              to call function AppRAMPInit() every time before start it.
 *            - APPCTRL_STOPNOW: Stop the measurment immediately.
 *            - APPCTRL_STOPSYNC: Stop the measurement when current measured data
 *                     is read back.
 *            - BIACTRL_GETFREQ: Get Sine frequency.
 *            - APPCTRL_SHUTDOWN: Stop the measurement immediately and put AFE to
 *                    shut down mode(turn off LP loop and enter hibernate).
 *            - APPCTRL_RUNNING: Read state if running.
  *@return            AD5940ERR_OK/AD5940ERR_PARA
******************************************************************************/
AD5940Err AppBIACtrl(int32_t BcmCtrl, void *pPara) {
  switch (BcmCtrl) {
    case APPCTRL_START:
    {
      WUPTCfg_Type wupt_cfg;
      if(AD5940_WakeUp(10) > 10)  /* Wakeup AFE by read register, read 10 times at most */
        return AD5940ERR_WAKEUP;  /* Wakeup Failed */
      if(AppBIACfg.BIAInited == bFALSE)
        return AD5940ERR_APPERROR;
      /* Start it */
      wupt_cfg.WuptEn = bTRUE;
      wupt_cfg.WuptEndSeq = WUPTENDSEQ_A;
      wupt_cfg.WuptOrder[0] = SEQID_0;
      wupt_cfg.SeqxSleepTime[SEQID_0] = (uint32_t)(AppBIACfg.WuptClkFreq/AppBIACfg.BiaODR)-2-1;
      /* The minimum value is 1. Do not set it to zero. Set it to 1 will spend 2 32kHz clock. */
      wupt_cfg.SeqxWakeupTime[SEQID_0] = 1;
      AD5940_WUPTCfg(&wupt_cfg);

      AppBIACfg.FifoDataCount = 0;  /* restart */
	  AppBIACfg.bRunning = bTRUE;
      break;
    }
    case APPCTRL_STOPNOW:
    {
      if(AD5940_WakeUp(10) > 10)  /* Wakeup AFE by read register, read 10 times at most */
        return AD5940ERR_WAKEUP;  /* Wakeup Failed */
      /* Start Wupt right now */
      AD5940_WUPTCtrl(bFALSE);
      AD5940_WUPTCtrl(bFALSE);  /* @todo is it sure this will stop Wupt? */
	  AppBIACfg.bRunning = bFALSE;
      break;
    }
    case APPCTRL_STOPSYNC:
    {
      AppBIACfg.StopRequired = bTRUE;
      break;
    }
    case BIACTRL_GETFREQ:
    if(pPara)
    {
      if(AppBIACfg.SweepCfg.SweepEn == bTRUE)
        *(float*)pPara = AppBIACfg.FreqofData;
      else
        *(float*)pPara = AppBIACfg.SinFreq;
    }
    break;
    case APPCTRL_SHUTDOWN:
    {
      /* Stop the measurment if it's running. */
      AppBIACtrl(APPCTRL_STOPNOW, 0);
      /* Turn off LPloop related blocks which are not controlled
       * automatically by sleep operation */
      AFERefCfg_Type aferef_cfg;
      LPLoopCfg_Type lp_loop;
      memset(&aferef_cfg, 0, sizeof(aferef_cfg));
      AD5940_REFCfgS(&aferef_cfg);
      memset(&lp_loop, 0, sizeof(lp_loop));
      AD5940_LPLoopCfgS(&lp_loop);
      AD5940_EnterSleepS();  /* Enter Hibernate */
    }
    break;
    case APPCTRL_RUNNING:
      if(pPara == NULL)
        return AD5940ERR_NULLP; /* Null pointer */
    *(BoolFlag*)pPara = AppBIACfg.bRunning;
    break;
   default:
    break;
  }
  return AD5940ERR_OK;
}

/*!
  ****************************************************************************
  *@brief      BIA sequence generation
  *@param      None
  *@return     AD5940Err/AD5940ERR_OK
******************************************************************************/
static AD5940Err AppBIASeqCfgGen(void)
{
  AD5940Err error = AD5940ERR_OK;
  uint32_t const *pSeqCmd;
  uint32_t SeqLen;

  AFERefCfg_Type aferef_cfg;
  HSLoopCfg_Type hs_loop;
  LPLoopCfg_Type lp_loop;
  DSPCfg_Type dsp_cfg;
  float sin_freq;

  /* Start sequence generator here */
  AD5940_SEQGenCtrl(bTRUE);

  /* analog front end configuration */
  aferef_cfg.HpBandgapEn = bTRUE;
  aferef_cfg.Hp1V1BuffEn = bTRUE;
  aferef_cfg.Hp1V8BuffEn = bTRUE;
  aferef_cfg.Disc1V1Cap = bFALSE;
  aferef_cfg.Disc1V8Cap = bFALSE;
  aferef_cfg.Hp1V8ThemBuff = bFALSE;
  aferef_cfg.Hp1V8Ilimit = bFALSE;
  aferef_cfg.Lp1V1BuffEn = bFALSE;
  aferef_cfg.Lp1V8BuffEn = bFALSE;

  /* LP reference control - turn off them to save powr*/
  aferef_cfg.LpBandgapEn = bTRUE;
  aferef_cfg.LpRefBufEn = bTRUE;
  aferef_cfg.LpRefBoostEn = bFALSE;
  AD5940_REFCfgS(&aferef_cfg);

  /* high power loop configuration */
  hs_loop.HsDacCfg.ExcitBufGain = AppBIACfg.ExcitBufGain;
  hs_loop.HsDacCfg.HsDacGain = AppBIACfg.HsDacGain;
  hs_loop.HsDacCfg.HsDacUpdateRate = AppBIACfg.HsDacUpdateRate;
  hs_loop.HsTiaCfg.DiodeClose = bFALSE;
  hs_loop.HsTiaCfg.HstiaBias = HSTIABIAS_1P1;
  hs_loop.HsTiaCfg.HstiaCtia = AppBIACfg.CtiaSel; /* 31pF + 2pF */
  hs_loop.HsTiaCfg.HstiaDeRload = HSTIADERLOAD_OPEN;
  hs_loop.HsTiaCfg.HstiaDeRtia = HSTIADERTIA_OPEN;
  hs_loop.HsTiaCfg.HstiaRtiaSel = AppBIACfg.HstiaRtiaSel;

  /* Configure switches */
  hs_loop.SWMatCfg.Dswitch = SWD_OPEN;
  hs_loop.SWMatCfg.Pswitch = SWP_PL|SWP_PL2;
  hs_loop.SWMatCfg.Nswitch = SWN_NL|SWN_NL2;
  hs_loop.SWMatCfg.Tswitch = SWT_TRTIA;

  /* high power loop wave form generator configuration */
  hs_loop.WgCfg.WgType = WGTYPE_SIN;
  hs_loop.WgCfg.GainCalEn = bFALSE;
  hs_loop.WgCfg.OffsetCalEn = bFALSE;

   /* If sweep enabled, perform sweep operation */
  if(AppBIACfg.SweepCfg.SweepEn == bTRUE) {
    AppBIACfg.SweepCfg.SweepIndex = 0;
    AppBIACfg.FreqofData = AppBIACfg.SweepCfg.SweepStart;
    AppBIACfg.SweepCurrFreq = AppBIACfg.SweepCfg.SweepStart;
    AD5940_SweepNext(&AppBIACfg.SweepCfg, &AppBIACfg.SweepNextFreq);
    sin_freq = AppBIACfg.SweepCurrFreq;
  } else {
    sin_freq = AppBIACfg.SinFreq;
    AppBIACfg.FreqofData = sin_freq;
  }

  /* high power loop sine signal settings */
  hs_loop.WgCfg.SinCfg.SinFreqWord = AD5940_WGFreqWordCal(sin_freq, AppBIACfg.SysClkFreq);
  hs_loop.WgCfg.SinCfg.SinAmplitudeWord = (uint32_t)(AppBIACfg.DacVoltPP/800.0f*2047 + 0.5f);
  hs_loop.WgCfg.SinCfg.SinOffsetWord = 0;
  hs_loop.WgCfg.SinCfg.SinPhaseWord = 0;
  AD5940_HSLoopCfgS(&hs_loop);

  /* low power loop dac configuration settings */
  lp_loop.LpDacCfg.LpdacSel = LPDAC0;
  lp_loop.LpDacCfg.LpDacSrc = LPDACSRC_MMR;
  lp_loop.LpDacCfg.LpDacSW = LPDACSW_VBIAS2LPPA|LPDACSW_VBIAS2PIN|LPDACSW_VZERO2LPTIA|LPDACSW_VZERO2PIN;
  lp_loop.LpDacCfg.LpDacVzeroMux = LPDACVZERO_6BIT;
  lp_loop.LpDacCfg.LpDacVbiasMux = LPDACVBIAS_12BIT;
  lp_loop.LpDacCfg.LpDacRef = LPDACREF_2P5;
  lp_loop.LpDacCfg.DataRst = bFALSE;
  lp_loop.LpDacCfg.PowerEn = bTRUE;
  lp_loop.LpDacCfg.DacData12Bit = (uint32_t)((1100-200)/2200.0*4095);
  lp_loop.LpDacCfg.DacData6Bit = 31;

  /* low power loop amplifier settings */
  lp_loop.LpAmpCfg.LpAmpSel = LPAMP0;
  lp_loop.LpAmpCfg.LpAmpPwrMod = LPAMPPWR_NORM;
  lp_loop.LpAmpCfg.LpPaPwrEn = bTRUE;
  lp_loop.LpAmpCfg.LpTiaPwrEn = bTRUE;
  lp_loop.LpAmpCfg.LpTiaRf = LPTIARF_20K;
  lp_loop.LpAmpCfg.LpTiaRload = LPTIARLOAD_SHORT;
  lp_loop.LpAmpCfg.LpTiaRtia = LPTIARTIA_OPEN;
  lp_loop.LpAmpCfg.LpTiaSW = LPTIASW(5)|LPTIASW(6)|LPTIASW(7)|LPTIASW(8)|\
                             LPTIASW(9)|LPTIASW(12)|LPTIASW(13);
  AD5940_LPLoopCfgS(&lp_loop);

  /* dsp configuration ADC settings */
  dsp_cfg.ADCBaseCfg.ADCMuxN = ADCMUXN_HSTIA_N;
  dsp_cfg.ADCBaseCfg.ADCMuxP = ADCMUXP_HSTIA_P;
  dsp_cfg.ADCBaseCfg.ADCPga = AppBIACfg.ADCPgaGain;

  memset(&dsp_cfg.ADCDigCompCfg, 0, sizeof(dsp_cfg.ADCDigCompCfg));
  /* Don't care because it's disabled */
  dsp_cfg.ADCFilterCfg.ADCAvgNum = ADCAVGNUM_16;
  /* Tell filter block clock rate of ADC*/
  dsp_cfg.ADCFilterCfg.ADCRate = ADCRATE_800KHZ;
  dsp_cfg.ADCFilterCfg.ADCSinc2Osr = AppBIACfg.ADCSinc2Osr;
  dsp_cfg.ADCFilterCfg.ADCSinc3Osr = AppBIACfg.ADCSinc3Osr;
  dsp_cfg.ADCFilterCfg.BpSinc3 = bFALSE;
  dsp_cfg.ADCFilterCfg.BpNotch = bTRUE;
  dsp_cfg.ADCFilterCfg.Sinc2NotchEnable = bTRUE;
  dsp_cfg.DftCfg.DftNum = AppBIACfg.DftNum;
  dsp_cfg.DftCfg.DftSrc = AppBIACfg.DftSrc;
  dsp_cfg.DftCfg.HanWinEn = AppBIACfg.HanWinEn;

  memset(&dsp_cfg.StatCfg, 0, sizeof(dsp_cfg.StatCfg));
  AD5940_DSPCfgS(&dsp_cfg);

  /* Enable all of them. They are automatically turned off during hibernate mode to save power */
  AD5940_AFECtrlS(AFECTRL_HPREFPWR|AFECTRL_HSTIAPWR|AFECTRL_INAMPPWR|AFECTRL_EXTBUFPWR|\
                AFECTRL_WG|AFECTRL_DACREFPWR|AFECTRL_HSDACPWR|\
                AFECTRL_SINC2NOTCH, bTRUE);
  /* GP6->endSeq, GP5 -> AD8233=OFF, GP1->RLD=OFF */
  AD5940_SEQGpioCtrlS(0);

  /* Sequence end. */
  /* Add one external command to disable sequencer for
   * initialization sequence because we only want it to run one time. */
  AD5940_SEQGenInsert(SEQ_STOP());

  /* Stop here */
  error = AD5940_SEQGenFetchSeq(&pSeqCmd, &SeqLen);
  AD5940_SEQGenCtrl(bFALSE); /* Stop sequencer generator */
  if(error == AD5940ERR_OK) {
    AppBIACfg.InitSeqInfo.SeqId = SEQID_1;
    AppBIACfg.InitSeqInfo.SeqRamAddr = AppBIACfg.SeqStartAddr;
    AppBIACfg.InitSeqInfo.pSeqCmd = pSeqCmd;
    AppBIACfg.InitSeqInfo.SeqLen = SeqLen;
    /* Write command to SRAM */
    AD5940_SEQCmdWrite(AppBIACfg.InitSeqInfo.SeqRamAddr, pSeqCmd, SeqLen);
  } else {
    return error; /* Error */
  }
  return AD5940ERR_OK;
}

/*!
  ****************************************************************************
  *@brief      BIA sequence measurement generation
  *@param      None
  *@return     AD5940Err/AD5940ERR_OK
******************************************************************************/
static AD5940Err AppBIASeqMeasureGen(void) {
  AD5940Err error = AD5940ERR_OK;
  uint32_t const *pSeqCmd;
  uint32_t SeqLen;

  uint32_t WaitClks;
  SWMatrixCfg_Type sw_cfg;
  ClksCalInfo_Type clks_cal;

  clks_cal.DataType = DATATYPE_DFT;
  clks_cal.DftSrc = AppBIACfg.DftSrc;
  /* Enable DFT data count 2^(DFTNUMBER+2) */
  clks_cal.DataCount = 1L<<(AppBIACfg.DftNum+2);
  clks_cal.ADCSinc2Osr = AppBIACfg.ADCSinc2Osr;
  clks_cal.ADCSinc3Osr = AppBIACfg.ADCSinc3Osr;
  clks_cal.ADCAvgNum = 0;
  clks_cal.BpNotch = bFALSE;

  clks_cal.RatioSys2AdcClk = AppBIACfg.SysClkFreq/AppBIACfg.AdcClkFreq;
  AD5940_ClksCalculate(&clks_cal, &WaitClks);

  /* Start sequence generator here */
  AD5940_SEQGenCtrl(bTRUE);
  /* GP6->endSeq, GP5 -> AD8233=OFF, GP1->RLD=OFF */
  AD5940_SEQGpioCtrlS(AGPIO_Pin6);

  /* wait 250*16 micro sec*/
  AD5940_SEQGenInsert(SEQ_WAIT(16*250));
  sw_cfg.Dswitch = SWD_CE0;
  sw_cfg.Pswitch = SWP_CE0;
  sw_cfg.Nswitch = SWN_AIN1;
  sw_cfg.Tswitch = SWT_AIN1|SWT_TRTIA;
  AD5940_SWMatrixCfgS(&sw_cfg);
  AD5940_ADCMuxCfgS(ADCMUXP_HSTIA_P, ADCMUXN_HSTIA_N);
  /* Enable Waveform generator, ADC power */
  AD5940_AFECtrlS(AFECTRL_WG|AFECTRL_ADCPWR, bTRUE);
  /* Enable sequencer wait */
  AD5940_SEQGenInsert(SEQ_WAIT(16*50));
  /* Start ADC convert and DFT */
  AD5940_AFECtrlS(AFECTRL_ADCCNV|AFECTRL_DFT, bTRUE);
  /* wait for first data ready */
  AD5940_SEQGenInsert(SEQ_WAIT(WaitClks));
  AD5940_AFECtrlS(AFECTRL_ADCCNV|AFECTRL_DFT|AFECTRL_WG|\
                  AFECTRL_ADCPWR, bFALSE);  /* Stop ADC convert and DFT */

  AD5940_ADCMuxCfgS(ADCMUXP_AIN3, ADCMUXN_AIN2);
  /* Enable Waveform generator, ADC power */
  AD5940_AFECtrlS(AFECTRL_WG|AFECTRL_ADCPWR, bTRUE);
  /* delay for signal settling DFT_WAIT */
  AD5940_SEQGenInsert(SEQ_WAIT(16*50));
  /* Start ADC convert and DFT */
  AD5940_AFECtrlS(AFECTRL_ADCCNV|AFECTRL_DFT, bTRUE);
  /* wait for first data ready */
  AD5940_SEQGenInsert(SEQ_WAIT(WaitClks));
  /* Stop ADC convert and DFT */
  AD5940_AFECtrlS(AFECTRL_ADCCNV|AFECTRL_DFT|AFECTRL_WG|AFECTRL_ADCPWR, bFALSE);

  /* Configure float switches */
  sw_cfg.Dswitch = SWD_OPEN;
  sw_cfg.Pswitch = SWP_PL|SWP_PL2;
  sw_cfg.Nswitch = SWN_NL|SWN_NL2;
  sw_cfg.Tswitch = SWT_TRTIA;
  AD5940_SWMatrixCfgS(&sw_cfg);
  /* GP6->endSeq, GP5 -> AD8233=OFF, GP1->RLD=OFF .*/
  AD5940_SEQGpioCtrlS(0);
  /* Goto hibernate */
  AD5940_EnterSleepS();
  /* Sequence end. */
  error = AD5940_SEQGenFetchSeq(&pSeqCmd, &SeqLen);
  /* Stop sequencer generator */
  AD5940_SEQGenCtrl(bFALSE);
  AppBIACfg.MeasSeqCycleCount = AD5940_SEQCycleTime();
  AppBIACfg.MaxODR = 1/(((AppBIACfg.MeasSeqCycleCount + 10) / 16.0)* 1E-6)  ;
  if(AppBIACfg.BiaODR > AppBIACfg.MaxODR) {
    /* We have requested a sampling rate that cannot be achieved with the time it
       takes to acquire a sample.
    */
    AppBIACfg.BiaODR = AppBIACfg.MaxODR;
  }

  if(error == AD5940ERR_OK) {
    /* Sequencer configuration */
    AppBIACfg.MeasureSeqInfo.SeqId = SEQID_0;
    AppBIACfg.MeasureSeqInfo.SeqRamAddr = AppBIACfg.InitSeqInfo.SeqRamAddr + AppBIACfg.InitSeqInfo.SeqLen ;
    AppBIACfg.MeasureSeqInfo.pSeqCmd = pSeqCmd;
    AppBIACfg.MeasureSeqInfo.SeqLen = SeqLen;
    /* Write command to SRAM */
    AD5940_SEQCmdWrite(AppBIACfg.MeasureSeqInfo.SeqRamAddr, pSeqCmd, SeqLen);
  } else {
    return error; /* Error */
  }
  return AD5940ERR_OK;
}

/*!
  ****************************************************************************
  *@brief      BIA rtia calibration
  *@param      None
  *@return     AD5940Err/AD5940ERR_OK
******************************************************************************/
static AD5940Err AppBIARtiaCal(void) {
  HSRTIACal_Type hsrtia_cal;
  hsrtia_cal.AdcClkFreq = AppBIACfg.AdcClkFreq;
  hsrtia_cal.ADCSinc2Osr = AppBIACfg.ADCSinc2Osr;
  hsrtia_cal.ADCSinc3Osr = AppBIACfg.ADCSinc3Osr;
  /* Enable magnitude and phase */
  hsrtia_cal.bPolarResult = bTRUE;
  hsrtia_cal.DftCfg.DftNum = AppBIACfg.DftNum;
  hsrtia_cal.DftCfg.DftSrc = AppBIACfg.DftSrc;
  hsrtia_cal.DftCfg.HanWinEn = AppBIACfg.HanWinEn;
  hsrtia_cal.fRcal= AppBIACfg.RcalVal;
  hsrtia_cal.HsTiaCfg.DiodeClose = bFALSE;
  hsrtia_cal.HsTiaCfg.HstiaBias = HSTIABIAS_1P1;
  hsrtia_cal.HsTiaCfg.HstiaCtia = AppBIACfg.CtiaSel;
  hsrtia_cal.HsTiaCfg.HstiaDeRload = HSTIADERLOAD_OPEN;
  hsrtia_cal.HsTiaCfg.HstiaDeRtia = HSTIADERTIA_TODE;
  hsrtia_cal.HsTiaCfg.HstiaRtiaSel = AppBIACfg.HstiaRtiaSel;
  hsrtia_cal.SysClkFreq = AppBIACfg.SysClkFreq;
  hsrtia_cal.fFreq = AppBIACfg.SweepCfg.SweepStart;

  /* If sweep is enabled, consider update of Rtia cal table based on sweep points */
  if(AppBIACfg.SweepCfg.SweepEn == bTRUE) {
    uint32_t i;
    AppBIACfg.SweepCfg.SweepIndex = 0;  /* Reset index */
    for(i = 0; i < AppBIACfg.SweepCfg.SweepPoints;i++) {
      AD5940_SweepNext(&AppBIACfg.SweepCfg, &hsrtia_cal.fFreq);
    }
    AppBIACfg.SweepCfg.SweepIndex = 0;  /* Reset index */
    AppBIACfg.RtiaCurrValue[0] = AppBIACfg.RtiaCalTable[AppBIACfg.SweepCfg.SweepIndex][0];
    AppBIACfg.RtiaCurrValue[1] = AppBIACfg.RtiaCalTable[AppBIACfg.SweepCfg.SweepIndex][1];
  } else {
    hsrtia_cal.fFreq = AppBIACfg.SinFreq;
    AD5940_HSRtiaCal(&hsrtia_cal, AppBIACfg.RtiaCurrValue);
  }
  return AD5940ERR_OK;
}

/*!
  ****************************************************************************
  *@brief      BIA Initialization
  *@param      None
  *@return     AD5940Err/AD5940ERR_OK
******************************************************************************/
AD5940Err AppBIAInit(uint32_t *pBuffer, uint32_t BufferSize) {
  AD5940Err error = AD5940ERR_OK;
  SEQCfg_Type seq_cfg;
  FIFOCfg_Type fifo_cfg;

  if(AD5940_WakeUp(10) > 10)  /* Wakeup AFE by read register, read 10 times at most */
    return AD5940ERR_WAKEUP;  /* Wakeup Failed */

  /* Add mux enable for BCM measurement */
  /* output enable */
  uint32_t tempreg;
  tempreg = AD5940_ReadReg(REG_AGPIO_GP0OEN);
  tempreg &= 0xFFE7;
  tempreg |= 0x0018;
  AD5940_AGPIOOen(tempreg);
  /* pull up / pull down register */
  tempreg = AD5940_ReadReg(REG_AGPIO_GP0PE);
  tempreg &= 0xFFE7;
  tempreg |= 0x0018;
  AD5940_AGPIOPen(tempreg);
  /* data set register */
  tempreg =  AD5940_ReadReg(REG_AGPIO_GP0SET);
  tempreg &= 0xFFE7;
  tempreg |= 0x0008;
  AD5940_AGPIOSet(tempreg);
  /* Configure sequencer and stop it */
  /* 2kB SRAM is used for sequencer, others for data FIFO */
  seq_cfg.SeqMemSize = SEQMEMSIZE_2KB;
  seq_cfg.SeqBreakEn = bFALSE;
  seq_cfg.SeqIgnoreEn = bFALSE;
  seq_cfg.SeqCntCRCClr = bTRUE;
  seq_cfg.SeqEnable = bFALSE;
  seq_cfg.SeqWrTimer = 0;
  AD5940_SEQCfg(&seq_cfg);

  /* Do RTIA calibration */
  if((AppBIACfg.ReDoRtiaCal == bTRUE) || \
      AppBIACfg.BIAInited == bFALSE)  /* Do calibration on the first initialization */
  {
    AppBIARtiaCal();
    AppBIACfg.ReDoRtiaCal = bFALSE;
  }
  /* Reconfigure FIFO */
  /* Disable FIFO firstly */
  AD5940_FIFOCtrlS(FIFOSRC_DFT, bFALSE);
  fifo_cfg.FIFOEn = bTRUE;
  /* fifo mode set */
  fifo_cfg.FIFOMode = FIFOMODE_FIFO;
  /* 4kB for FIFO, The reset 2kB for sequencer */
  fifo_cfg.FIFOSize = FIFOSIZE_4KB;
  fifo_cfg.FIFOSrc = FIFOSRC_DFT;
  /* DFT result. One pair for RCAL, another for Rz.
   * One DFT result have real part and imaginary part */
  fifo_cfg.FIFOThresh = AppBIACfg.FifoThresh;
  AD5940_FIFOCfg(&fifo_cfg);
  AD5940_INTCClrFlag(AFEINTSRC_ALLINT);
  /* Start sequence generator */
  /* Initialize sequencer generator */
  if((AppBIACfg.BIAInited == bFALSE)||\
       (AppBIACfg.bParaChanged == bTRUE)){
    if(pBuffer == 0)  return AD5940ERR_PARA;
    if(BufferSize == 0) return AD5940ERR_PARA;
    AD5940_SEQGenInit(pBuffer, BufferSize);
    /* Generate initialize sequence */
    /* Application initialization sequence using either MCU or sequencer */
    error = AppBIASeqCfgGen();
    if(error != AD5940ERR_OK) return error;
    /* Generate measurement sequence */
    error = AppBIASeqMeasureGen();
    if(error != AD5940ERR_OK) return error;
    /* Clear this flag as we already implemented the new configuration */
    AppBIACfg.bParaChanged = bFALSE;
  }
  /* Initialization sequencer  */
  AppBIACfg.InitSeqInfo.WriteSRAM = bFALSE;
  AD5940_SEQInfoCfg(&AppBIACfg.InitSeqInfo);
  seq_cfg.SeqEnable = bTRUE;
  /* Enable sequencer */
  AD5940_SEQCfg(&seq_cfg);
  AD5940_SEQMmrTrig(AppBIACfg.InitSeqInfo.SeqId);
  while(AD5940_INTCTestFlag(AFEINTC_1, AFEINTSRC_ENDSEQ) == bFALSE);
  /* Measurment sequence  */
  AppBIACfg.MeasureSeqInfo.WriteSRAM = bFALSE;
  AD5940_SEQInfoCfg(&AppBIACfg.MeasureSeqInfo);
  seq_cfg.SeqEnable = bTRUE;
  /* Enable sequencer, and wait for trigger */
  AD5940_SEQCfg(&seq_cfg);
  /* Clear interrupt flag generated before */
  AD5940_ClrMCUIntFlag();
  AD5940_AFEPwrBW(AppBIACfg.PwrMod, AFEBW_250KHZ);
  AD5940_WriteReg(REG_AFE_SWMUX, 1<<3);
  /* BIA application has been initialized. */
  AppBIACfg.BIAInited = bTRUE;
  return AD5940ERR_OK;
}

/*!
  ****************************************************************************
  *@brief      BIA register modification
  *@param      None
  *@return     AD5940Err/AD5940ERR_OK
******************************************************************************/
AD5940Err AppBIARegModify(int32_t * const pData, uint32_t *pDataCount) {
  if(AppBIACfg.NumOfData > 0) {
    AppBIACfg.FifoDataCount += *pDataCount/4;
    if(AppBIACfg.FifoDataCount >= AppBIACfg.NumOfData) {
      AD5940_WUPTCtrl(bFALSE);
      return AD5940ERR_OK;
    }
  }
  if(AppBIACfg.StopRequired == bTRUE) {
    AD5940_WUPTCtrl(bFALSE);
    return AD5940ERR_OK;
  }
  /* Need to set new frequency and set power mode */
  if(AppBIACfg.SweepCfg.SweepEn) {
    AD5940_WGFreqCtrlS(AppBIACfg.SweepNextFreq, AppBIACfg.SysClkFreq);
  }
  return AD5940ERR_OK;
}


iImpCar_Type *pDftVolt, *pDftCurr;

/*!
  ****************************************************************************
  *@brief      Depending on the data type, do appropriate data pre-process
               before return back to controller
               AD5940_BIOZ-4Wire driver example code has data conversion for 
               voltage phase and magnitude, for BCM algo we need cartesian form of 
               Real and imaginary from DFT results.
               Reference code for calculating cartesian form of real and 
               imaginary taken from AD5940_BIOZ-2Wire driver example code 
  *@param      pointer to data buffer, pointer to data count
  *@return     AD5940Err/AD5940ERR_OK
*******************************************************************************/
AD5940Err AppBIADataProcess(int32_t * const pData, uint32_t *pDataCount)  {
 uint32_t DataCount = *pDataCount;
  uint32_t ImpResCount = DataCount/4;
  
  fImpCar_Type * pOut = (fImpCar_Type*)pData;
  iImpCar_Type * pSrcData = (iImpCar_Type*)pData;
  
  *pDataCount = 0;
  
  DataCount = (DataCount/4)*4; /* One DFT result has two data in FIFO, real part and imaginary part. Each measurement has 2 DFT results, one for voltage measurement, one for current */
  
  /* Convert DFT result to int32_t type */
  for(uint32_t i=0; i<DataCount; i++)
  {
    pData[i] &= 0x3ffff;
    if(pData[i]&(1<<17)) /* Bit17 is sign bit */
    {
      pData[i] |= 0xfffc0000; /* Data is 18bit in two's complement, bit17 is the sign bit */
    }
  }
  for(uint32_t i=0; i<ImpResCount; i++)
  {
    fImpCar_Type DftCurr, DftVolt;
    fImpCar_Type res;
    
    DftCurr.Real = (float)pSrcData[i].Real;
    DftCurr.Image = (float)pSrcData[i].Image;
    DftVolt.Real = (float)pSrcData[i+1].Real;
    DftVolt.Image = (float)pSrcData[i+1].Image;
    
    DftCurr.Real =  DftCurr.Real;
    DftCurr.Image = -DftCurr.Image;
    DftVolt.Real = DftVolt.Real;
    DftVolt.Image = -DftVolt.Image;
    res = AD5940_ComplexDivFloat(&DftCurr,(fImpCar_Type*)&AppBIACfg.RtiaCurrValue);           /* I=Vrtia/Zrtia */
    res = AD5940_ComplexDivFloat(&DftVolt, &res);
    pOut[i] = res;
  }
  *pDataCount = ImpResCount; 

  /* Calculate next frequency point */
  if(AppBIACfg.SweepCfg.SweepEn == bTRUE){
    AppBIACfg.FreqofData = AppBIACfg.SweepCurrFreq;
    AppBIACfg.SweepCurrFreq = AppBIACfg.SweepNextFreq;
    AppBIACfg.RtiaCurrValue[0] = AppBIACfg.RtiaCalTable[AppBIACfg.SweepCfg.SweepIndex][0];
    AppBIACfg.RtiaCurrValue[1] = AppBIACfg.RtiaCalTable[AppBIACfg.SweepCfg.SweepIndex][1];
    AD5940_SweepNext(&AppBIACfg.SweepCfg, &AppBIACfg.SweepNextFreq);
  }
  return AD5940ERR_OK;
}

/*!
  ****************************************************************************
  *@brief      Initialize AD5940 basic blocks like clock
  *@param      void
  *@return     AD5940Err/AD5940ERR_OK
******************************************************************************* */
int32_t AD5940PlatformCfg(void)
{
  CLKCfg_Type clk_cfg;
  FIFOCfg_Type fifo_cfg;
  AGPIOCfg_Type gpio_cfg;
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
   /* 4kB for FIFO, The reset 2kB for sequencer */
  fifo_cfg.FIFOSize = FIFOSIZE_4KB;
  fifo_cfg.FIFOSrc = FIFOSRC_DFT;
  /* DFT result. One pair for RCAL, another for Rz.
   * One DFT result have real part and imaginary part */
  fifo_cfg.FIFOThresh = 4;
  AD5940_FIFOCfg(&fifo_cfg);
  /* Enable FIFO here */
  fifo_cfg.FIFOEn = bTRUE;
  AD5940_FIFOCfg(&fifo_cfg);

  /* Step3. Interrupt controller */
  /* Enable all interrupt in Interrupt Controller 1, so we can check INTC flags */
  AD5940_INTCCfg(AFEINTC_1, AFEINTSRC_ALLINT, bTRUE);
  /* Interrupt Controller 0 will control GP0 to generate interrupt to MCU */
  AD5940_INTCCfg(AFEINTC_0, AFEINTSRC_DATAFIFOTHRESH, bTRUE);
  AD5940_INTCClrFlag(AFEINTSRC_ALLINT);

  /* Step4: Reconfigure GPIO */
  gpio_cfg.FuncSet = GP6_SYNC|GP5_SYNC|GP4_SYNC|GP2_TRIG|GP1_SYNC|GP0_INT;
  gpio_cfg.InputEnSet = AGPIO_Pin2;
  gpio_cfg.OutputEnSet = AGPIO_Pin0|AGPIO_Pin1|AGPIO_Pin4|AGPIO_Pin5|AGPIO_Pin6;
  gpio_cfg.OutVal = 0;
  gpio_cfg.PullEnSet = 0;

  AD5940_AGPIOCfg(&gpio_cfg);
  /* Allow AFE to enter sleep mode. */
  AD5940_SleepKeyCtrlS(SLPKEY_UNLOCK);
  return 0;
}

/*!
  ****************************************************************************
  *@brief      !!Change the application parameters here if you want to change it
  *@param      void
  *@return     void
******************************************************************************* */
void AD5940BIAStructInit(void) {
  AppBIACfg_Type *pBIACfg;
  AppBIAGetCfg(&pBIACfg);
  pBIACfg->SeqStartAddr = 0;
  pBIACfg->MaxSeqLen = 512; /** @todo add checker in function */
  pBIACfg->RcalVal = 10000.0;
  if(!bia_user_applied_dft_num) {
    pBIACfg->DftNum = DFTNUM_8192;
  } else {
    bia_user_applied_dft_num = 0;
  }
  /* Never stop until you stop it mannually by AppBIACtrl() function */
  pBIACfg->NumOfData = -1;

 if(!bia_user_applied_odr) {
    /* ODR(Sample Rate) 20Hz */
   /* ODR decides how freuquently to start the engine to measure impedance. */
    pBIACfg->BiaODR = 20;
  } else {
    bia_user_applied_odr = 0;
  }
  pBIACfg->FifoThresh = 4;      /* 4 */
  pBIACfg->ADCSinc3Osr = ADCSINC3OSR_2;
}


/*!
 ****************************************************************************
 *@brief      AD5940 BCM App initialization
 *@param      None
 *@return     None
 ******************************************************************************/
void ad5940_bia_start(void){
     /* Interrupts setting */
  ad5940_port_Init();
   /* configure call back */
  Ad5940DrvDataReadyCallback(Ad5940FifoCallBack);

  /* clear AD5940 soft buffer */
  ClearDataBufferAd5940();
  /* default parameters setting */
  AD5940PlatformCfg();
  /* BIA Initialization */
  /* Configure your parameters in this function */
  AD5940BIAStructInit();

  /* Initialize BIA application. Provide a buffer, which is used to store sequencer commands */
  AppBIAInit(AppBuff, APPBUFF_SIZE);
}
#endif