/**
****************************************************************************
* @file     EDA_App_Test.c
* @author   ADI
* @version  V0.1
* @date     04-Dec-2019
* @brief    This source file is used as a test application for eda
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
#include "EDA_App_Test.h"

#include "sdk_config.h"
#include "nrf_log_ctrl.h"
#include "nrf_delay.h"
#define NRF_LOG_MODULE_NAME EDA_App_Task

#if EDA_APP_CONFIG_LOG_ENABLED
#ifdef NRF_LOG_DEFAULT_LEVEL
#undef NRF_LOG_DEFAULT_LEVEL
#define NRF_LOG_DEFAULT_LEVEL EDA_APP_CONFIG_LOG_LEVEL
#endif      
#define NRF_LOG_INFO_COLOR  EDA_APP_CONFIG_INFO_COLOR
#define NRF_LOG_DEBUG_COLOR  EDA_APP_CONFIG_DEBUG_COLOR
#else //LFS_CONFIG_LOG_ENABLED
#define NRF_LOG_LEVEL       0
#endif //LFS_CONFIG_LOG_ENABLED

#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

uint32_t ResistorForBaseline = 0;

/** @addtogroup AD5940_System_Examples
 * @{
 *    @defgroup EDA_Example
 *    @brief This example is used to measure skin impedance. The main feature of this example is ultra low power consumption.
 *    @details
 * @note Need to update code when runs at S2 silicon.
 * 
 * 
 * 
 * @{
 * */
AppEDACfg_Type AppEDACfg;
void InitCfg()
{
    memset(&AppEDACfg,0,sizeof(AppEDACfg));

/* Common configurations for all kinds of Application. */
  AppEDACfg.bParaChanged = bTRUE;
  AppEDACfg.SeqStartAddr = 0;
  AppEDACfg.MaxSeqLen = 0;
  AppEDACfg.SeqStartAddrCal = 0;
  AppEDACfg.MaxSeqLenCal = 0;

/* Application related parameters */ 
  AppEDACfg.bBioElecBoard = bTRUE;
  AppEDACfg.ReDoRtiaCal = bFALSE;
  AppEDACfg.SysClkFreq = 16000000.0;
  AppEDACfg.LfoscClkFreq = 32000.0;
  AppEDACfg.AdcClkFreq = 16000000.0;
  AppEDACfg.FifoThresh = 4;
  AppEDACfg.EDAODR = 4.0; /* 20.0 Hz*/
  AppEDACfg.NumOfData = -1;
  AppEDACfg.VoltCalPoints = 8;
  AppEDACfg.RcalVal = 10000.0; /* 10kOhm */
  AppEDACfg.SinFreq = 100.0; /* 100Hz */
  AppEDACfg.SampleFreq = 400.0;    /* 400Hz */
  AppEDACfg.SinAmplitude = 1100.0f/2; /* 1100mV peak */
  AppEDACfg.DacUpdateRate = 7;
  AppEDACfg.LptiaRtiaSel = LPTIARTIA_10K;
  //.LptiaRtiaSel = LPTIARTIA_100K,
  AppEDACfg.DftNum = DFTNUM_16;
  AppEDACfg.HanWinEn = bTRUE;

  AppEDACfg.RtiaAutoScaleEnable = bFALSE;
  //.RtiaAutoScaleEnable = bTRUE,
  AppEDACfg.RtiaAutoScaleMax = LPTIARTIA_512K;
  AppEDACfg.RtiaAutoScaleMin = LPTIARTIA_1K;

  AppEDACfg.RtiaIndexCurr = 0;
  AppEDACfg.RtiaIndexNext = 0;
  AppEDACfg.bChangeRtia = bFALSE;

  /* private varaibles */
  AppEDACfg.SeqPatchInfo.BuffLen = 32;
  AppEDACfg.SeqPatchInfo.pSeqCmd = NULL;

  AppEDACfg.ImpEDABase.Real = 0;
  AppEDACfg.ImpEDABase.Image = 0;
 
  AppEDACfg.ImpSum.Real = 0;
  AppEDACfg.ImpSum.Real = 0;
  AppEDACfg.EDAInited = bFALSE;
  AppEDACfg.StopRequired = bFALSE;
  AppEDACfg.bMeasVoltReq = bFALSE;
  AppEDACfg.EDAStateCurr = EDASTATE_INIT;
  AppEDACfg.EDAStateNext = EDASTATE_INIT;
};

/**
 * @brief This function is provided for upper controllers that want to change
 *        application parameters specially for user defined parameters.
 * @param pCfg: The pointer used to store application configuration structure pointer.
 * @return none.
*/
AD5940Err AppEDAGetCfg(void *pCfg)
{
  if(pCfg){
    *(AppEDACfg_Type**)pCfg = &AppEDACfg;
    return AD5940ERR_OK;
  }
  return AD5940ERR_PARA;
}

/**
 * @brief Control application like start, stop.
 * @param Command: The command for this applicaiton, select from below paramters
 *        - APPCTRL_START: start the measurment. Note: the ramp test need firslty call function AppRAMPInit() every time before start it.
 *        - APPCTRL_STOPNOW: Stop the measurment immediately.
 *        - APPCTRL_STOPSYNC: Stop the measuremnt when current measured data is read back.
 *        - APPCTRL_SHUTDOWN: Stop the measurment immediately and put AFE to shut down mode(turn off LP loop and enter hibernate).
 *        - EDACTRL_MEASVOLT: Measure voltage once current measurment is done(Interrupt occured).
 *        - EDACTRL_GETRTIAMAG: Get current RTIA value.
 * @return none.
*/
AD5940Err AppEDACtrl(int32_t EDACtrl, void *pPara)
{
  switch (EDACtrl)
  {
    case APPCTRL_START:
    {
      WUPTCfg_Type wupt_cfg;

      if(AD5940_WakeUp(10) > 10)  /* Wakeup AFE by read register, read 10 times at most */
        return AD5940ERR_WAKEUP;  /* Wakeup Failed */
      if(AppEDACfg.EDAInited == bFALSE)
        return AD5940ERR_APPERROR;
      /* Start it */
      wupt_cfg.WuptEn = bTRUE;
      wupt_cfg.WuptEndSeq = WUPTENDSEQ_A;
      wupt_cfg.WuptOrder[0] = SEQID_0;
      wupt_cfg.SeqxSleepTime[SEQID_0] = (uint32_t)(AppEDACfg.LfoscClkFreq/AppEDACfg.EDAODR)-2-4;
      wupt_cfg.SeqxWakeupTime[SEQID_0] = 4; /* The minimum value is 1. Do not set it to zero. Set it to 1 will spend 2 32kHz clock. */
      AD5940_WUPTCfg(&wupt_cfg);
      AppEDACfg.FifoDataCount = 0;  /* restart */
      break;
    }
    case APPCTRL_STOPNOW:
    {
      if(AD5940_WakeUp(10) > 10)  /* Wakeup AFE by read register, read 10 times at most */
        return AD5940ERR_WAKEUP;  /* Wakeup Failed */
      /* Start Wupt right now */
      AD5940_WUPTCtrl(bFALSE);
      AD5940_WUPTCtrl(bFALSE);
      break;
    }
    case APPCTRL_STOPSYNC:
    {
      AppEDACfg.StopRequired = bTRUE;
      break;
    }
    case APPCTRL_SHUTDOWN:
    {
      AppEDACtrl(APPCTRL_STOPNOW, 0);  /* Stop the measurement if it's running. */
      /* Turn off LPLoop related blocks which are not controlled automatically by hibernate operation */
      AFERefCfg_Type aferef_cfg;
      LPLoopCfg_Type lp_loop;
      memset(&aferef_cfg, 0, sizeof(aferef_cfg));
      AD5940_REFCfgS(&aferef_cfg);
      memset(&lp_loop, 0, sizeof(lp_loop));
      AD5940_LPLoopCfgS(&lp_loop);
      AD5940_EnterSleepS();  /* Enter Hibernate */
    }
    break;
    case EDACTRL_MEASVOLT:
      AppEDACfg.bMeasVoltReq = bTRUE;
    break;
    case EDACTRL_GETRTIAMAG:
      if(pPara == NULL)
        return AD5940ERR_NULLP; /* Null pointer */
      *(float*)pPara = AD5940_ComplexMag(&AppEDACfg.RtiaCurrValue);
    break;
    case EDACTRL_RSTBASE:
      AppEDACfg.ImpEDABase.Real = 0;
      AppEDACfg.ImpEDABase.Image = 0;
      AppEDACfg.ImpSum.Real = 0;
      AppEDACfg.ImpSum.Image = 0;
      AppEDACfg.ImpSumCount = 0;
    break;
    case EDACTRL_SETBASE:
    {
      fImpCar_Type *pImpBase = (fImpCar_Type *)pPara; /* The impedance used to set base line */
      AppEDACfg.ImpEDABase = *pImpBase;
    }
    break;
    case EDACTRL_GETAVR:
    if(pPara == NULL) return AD5940ERR_NULLP;
    {
      fImpCar_Type *pImpAVR = (fImpCar_Type *)pPara;
      pImpAVR->Real = AppEDACfg.ImpSum.Real/AppEDACfg.ImpSumCount;
      pImpAVR->Image = AppEDACfg.ImpSum.Image/AppEDACfg.ImpSumCount;
    }
    default:
    break;
  }
  return AD5940ERR_OK;
}

/**
 * @brief Generate initialization sequence and write the commands to SRAM.
 * @return return error code.
*/
static AD5940Err AppEDASeqCfgGen(void)
{
  AD5940Err error = AD5940ERR_OK;
  const uint32_t *pSeqCmd;
  uint32_t SeqLen;

  AFERefCfg_Type aferef_cfg;
  HSDACCfg_Type hsdac_cfg; /* Waveform Generator uses some parameter(DAC update rate) from HSDAC config registers */
  LPLoopCfg_Type lp_loop;
  WGCfg_Type wg_cfg;
  DSPCfg_Type dsp_cfg;
  SWMatrixCfg_Type sw_cfg;
  
  AD5940_SEQGenCtrl(bTRUE);
  /* Sequence starts here */
  AD5940_SEQGpioCtrlS(AGPIO_Pin6/*|AGPIO_Pin5*/|AGPIO_Pin1);
  AD5940_StructInit(&aferef_cfg, sizeof(aferef_cfg));
  AD5940_REFCfgS(&aferef_cfg);  /* Turn off all references, we only enable it when we need it. */
  
  AD5940_StructInit(&lp_loop, sizeof(lp_loop)); /* Disable everything, configure them during measurment */
  AD5940_LPLoopCfgS(&lp_loop);

  AD5940_StructInit(&wg_cfg, sizeof(wg_cfg));
  wg_cfg.SinCfg.SinAmplitudeWord = (uint32_t)(AppEDACfg.SinAmplitude/1100.0f*2047); /* Maximum amplitude is 1100mV */
  wg_cfg.SinCfg.SinFreqWord = AD5940_WGFreqWordCal(AppEDACfg.SinFreq, AppEDACfg.LfoscClkFreq);
  wg_cfg.SinCfg.SinPhaseWord = 0;
  wg_cfg.WgType = WGTYPE_SIN;
  AD5940_WGCfgS(&wg_cfg);
  AD5940_AFECtrlS(AFECTRL_ALL, bFALSE);

  /* Switch configuration for BioElec board */
  sw_cfg.Dswitch = SWD_OPEN;  /* Open all switch D */
  //sw_cfg.Dswitch = SWD_CE0; 
  sw_cfg.Pswitch = SWP_AIN2|SWP_SE0;
  sw_cfg.Nswitch = SWN_OPEN;
  sw_cfg.Tswitch = SWT_AIN0|SWT_AFE3LOAD;
  AD5940_SWMatrixCfgS(&sw_cfg);

  AD5940_StructInit(&dsp_cfg, sizeof(dsp_cfg));
  dsp_cfg.ADCBaseCfg.ADCMuxN = ADCMUXN_VSET1P1;
  dsp_cfg.ADCBaseCfg.ADCMuxP = ADCMUXP_AIN4;//commented
  //dsp_cfg.ADCBaseCfg.ADCMuxN = ADCMUXN_AIN2;
  //dsp_cfg.ADCBaseCfg.ADCMuxP = ADCMUXP_VCE0;

  dsp_cfg.ADCBaseCfg.ADCPga = ADCPGA_1;
  dsp_cfg.ADCFilterCfg.ADCRate = ADCRATE_800KHZ;
  dsp_cfg.ADCFilterCfg.ADCAvgNum = ADCAVGNUM_4; /* We use averaged SINC3 output as DFT input source */
  dsp_cfg.ADCFilterCfg.ADCSinc2Osr = ADCSINC2OSR_22;  /* Don't care */
  dsp_cfg.ADCFilterCfg.ADCSinc3Osr = ADCSINC3OSR_5;
  dsp_cfg.ADCFilterCfg.BpNotch = bTRUE;
  dsp_cfg.ADCFilterCfg.BpSinc3 = bFALSE;
  dsp_cfg.ADCFilterCfg.DFTClkEnable = bTRUE;
  dsp_cfg.ADCFilterCfg.Sinc2NotchClkEnable = bFALSE;  /* disable SINC2 since we don't need it. */
  dsp_cfg.ADCFilterCfg.Sinc2NotchEnable = bFALSE;
  dsp_cfg.ADCFilterCfg.Sinc3ClkEnable = bTRUE;
  dsp_cfg.ADCFilterCfg.WGClkEnable = bTRUE;
  dsp_cfg.DftCfg.DftNum = AppEDACfg.DftNum;
  dsp_cfg.DftCfg.DftSrc = DFTSRC_AVG; /* Use averaged SINC3 data */
  dsp_cfg.DftCfg.HanWinEn = AppEDACfg.HanWinEn;
  AD5940_DSPCfgS(&dsp_cfg);
  AD5940_ADCRepeatCfgS(5*(4+2)+1);  /* (n+2)*osr + 1, n=4,osr=5*/
  hsdac_cfg.ExcitBufGain = EXCITBUFGAIN_2;
  hsdac_cfg.HsDacGain = HSDACGAIN_1;
  hsdac_cfg.HsDacUpdateRate = AppEDACfg.DacUpdateRate;  /* Note: the DAC update rate is decided by register DACON.RATE */
  AD5940_HSDacCfgS(&hsdac_cfg);

  AD5940_SEQGpioCtrlS(0/*AGPIO_Pin6|AGPIO_Pin5|AGPIO_Pin1*/);        //GP6->endSeq, GP5 -> AD8233=OFF, GP1->RLD=OFF .

  /* Sequence end. */
  AD5940_SEQGenInsert(SEQ_STOP()); /* Add one extra command to disable sequencer for initialization sequence because we only want it to run one time. */
  /* Stop here */
  AD5940_SEQGenCtrl(bFALSE); /* Stop sequencer generator */
  error = AD5940_SEQGenFetchSeq(&pSeqCmd, &SeqLen);
  if(error == AD5940ERR_OK)
  {
    AppEDACfg.InitSeqInfo.SeqId = SEQID_1;
    AppEDACfg.InitSeqInfo.SeqRamAddr = AppEDACfg.SeqStartAddr;
    AppEDACfg.InitSeqInfo.pSeqCmd = pSeqCmd;
    AppEDACfg.InitSeqInfo.SeqLen = SeqLen;
    AppEDACfg.InitSeqInfo.WriteSRAM = bTRUE;
    AD5940_SEQInfoCfg(&AppEDACfg.InitSeqInfo); /* Write command to SRAM */
  }
  else
    return error; /* Error */
  return AD5940ERR_OK;
}

/**
 * @brief Generate patch sequence according to current measurement type(Voltage or Current). 
 * @details The patch is used to adjust sequencer commands already stored in SRAM of AD5940 in order to perform different measurments. 
 *          The reason is that the sequences need to be adjusted. Using the patch method will make things easiy and we won't need to modify 
 *          sequences in register level.
 * @param pPatch: pointer to patch information include the measurement type, Rtia selection and buffers.
 * @return return error code.
*/
static AD5940Err ApPEDASeqPatchGen(SeqPatchInfo_Type *pPatch)
{
  AD5940Err err;
  const uint32_t *pSeqCmd;
  uint32_t SeqLen;
  LPAmpCfg_Type lpamp_cfg;
  AD5940_SEQGenInit(pPatch->Buffer, pPatch->BuffLen);
  AD5940_SEQGenCtrl(bTRUE);
  lpamp_cfg.LpAmpSel = LPAMP0;
  lpamp_cfg.LpAmpPwrMod = LPAMPPWR_NORM;       /* Use normal power mode is enough */
  lpamp_cfg.LpPaPwrEn = bTRUE;                 /* Enable Potential amplifier */
  lpamp_cfg.LpTiaPwrEn = bTRUE;                /* Enable TIA amplifier */
  lpamp_cfg.LpTiaRf = LPF_RF;                  /* Rf resistor controls cut off frequency. */
  lpamp_cfg.LpTiaRload = LPTIARLOAD_100R;      /** @note Use 100Ohm Rload. */
  lpamp_cfg.LpTiaRtia = pPatch->RtiaSel;  /* If autoscaling is enabled, use seleted value. */
  if(pPatch->Type == PATCHTYPE_VOLT)
    lpamp_cfg.LpTiaSW = LPTIASW_VOLT;            /* Swtich settings for voltage measurement */
  else if(pPatch->Type == PATCHTYPE_CURR)
    lpamp_cfg.LpTiaSW = LPTIASW_CURR;            /* Swtich settings for current measurement */
  AD5940_LPAMPCfgS(&lpamp_cfg);
  AD5940_SEQGenCtrl(bFALSE);
  err = AD5940_SEQGenFetchSeq(&pSeqCmd, &SeqLen);
  if(err != AD5940ERR_OK)
    return err;
  pPatch->pSeqCmd = pSeqCmd;
  pPatch->SeqLen = SeqLen;
  return AD5940ERR_OK;
}

/**
 * @brief Generate measurement sequence and write the commands to SRAM.
 * @return return error code.
*/
static AD5940Err AppEDASeqMeasureGen(void)
{
  AD5940Err error = AD5940ERR_OK;
  const uint32_t *pSeqCmd;
  uint32_t SeqLen;
  uint32_t i;
  uint32_t DFTNumber; 

  LPDACCfg_Type lpdac_cfg;
  LPAmpCfg_Type lpamp_cfg;
  /* Start sequence generator here */
  AD5940_SEQGenCtrl(bTRUE);
  
  /* Stage I: Initialization */
  AD5940_SEQGpioCtrlS(AGPIO_Pin6/*|AGPIO_Pin5|AGPIO_Pin1*/);//GP6->endSeq, GP5 -> AD8233=OFF, GP1->RLD=OFF .
  /* LP loop configure: LPDAC and LPAMP */
  lpdac_cfg.LpdacSel = LPDAC0;
  lpdac_cfg.DataRst = bFALSE;
  lpdac_cfg.LpDacSW = LPDACSW_VBIAS2LPPA/*|LPDACSW_VBIAS2PIN*/|LPDACSW_VZERO2LPTIA|LPDACSW_VZERO2PIN;
  lpdac_cfg.LpDacRef = LPDACREF_2P5;           /* Use internal 2.5V reference */
  lpdac_cfg.LpDacSrc = LPDACSRC_WG;            /* Use data from waveform generator */
  lpdac_cfg.LpDacVbiasMux = LPDACVBIAS_12BIT;
  lpdac_cfg.LpDacVzeroMux = LPDACVZERO_6BIT;   /* Use 6bit LPDAC for Vzero */
  lpdac_cfg.PowerEn = bTRUE;                   /* Enable LPDAC */
  lpdac_cfg.DacData12Bit = 0;                  /* Don't care, 12bit DAC data is from WG */
  lpdac_cfg.DacData6Bit = 32;                  /* Set it to middle scale of LPDAC. Vzero is the bias voltage of LPTIA amplifire */ 
  AD5940_LPDACCfgS(&lpdac_cfg);

  /* Voltage and current measurment need different switch settings, record the difference and only modify this part for different purpose */
  error = AD5940_SEQGenFetchSeq(NULL, &AppEDACfg.SeqPatchInfo.SRAMAddr); /* Record the start address of below commands */
  if(error != AD5940ERR_OK)
    return error;
  
  lpamp_cfg.LpAmpSel = LPAMP0;
  lpamp_cfg.LpAmpPwrMod = LPAMPPWR_NORM;       /* Use normal power mode is enough */
  lpamp_cfg.LpPaPwrEn = bTRUE;                 /* Enable Potential amplifier */
  lpamp_cfg.LpTiaPwrEn = bTRUE;                /* Enable TIA amplifier */
  lpamp_cfg.LpTiaRf = LPF_RF;                  /* Rf resistor controls cut off frequency. */
  lpamp_cfg.LpTiaRload = LPTIARLOAD_100R;      /** @note Use 100Ohm Rload. */
  lpamp_cfg.LpTiaRtia = AppEDACfg.LptiaRtiaSel;  /* If autoscaling is enabled, use seleted value. */
  lpamp_cfg.LpTiaSW = LPTIASW_VOLT;            /* Swtich settings for voltage measurement */
  AD5940_LPAMPCfgS(&lpamp_cfg);

  AD5940_WriteReg(REG_AFE_LPREFBUFCON, 0);    /* Enable low power bandgap and 2.5V reference buffer  */  
  AD5940_AFECtrlS(AFECTRL_ALL, bFALSE);     /* Off everything */

  AD5940_LPModeEnS(bTRUE);  /* Enter LP control mode. The registers are summarized to LPMODECON, so we can control some blocks convenniently */
  AD5940_LPModeClkS(LPMODECLK_LFOSC); /* Trigger switching system clock to 32kHz */
  AD5940_LPModeCtrlS(LPMODECTRL_NONE);    /* Disable all */
  AD5940_AFECtrlS(AFECTRL_WG, bTRUE);     /* Start waveform generator */
  AD5940_SEQGenInsert(SEQ_WAIT(LPF_TIME*32));   /* wait for stable */
  AD5940_AFECtrlS(AFECTRL_DFT, bTRUE);     /* Enable DFT engine */

  /* Stage II: ADC Run to sample enough data(DFT number) */
  DFTNumber = (1<<(AppEDACfg.DftNum +2));
  for(i=0;i<DFTNumber;i++)
  {
    #define EDA_LPMODCTRLSETS LPMODECTRL_GLBBIASZ|LPMODECTRL_GLBBIASP|LPMODECTRL_HPREFPWR|LPMODECTRL_BUFHP1P8V|LPMODECTRL_BUFHP1P1V|LPMODECTRL_HFOSCEN

    AD5940_LPModeCtrlS(EDA_LPMODCTRLSETS);                        /* Turn ON HPREF. */
    AD5940_SEQGenInsert(SEQ_WAIT(4));
    AD5940_LPModeCtrlS(EDA_LPMODCTRLSETS|LPMODECTRL_REPEATEN);    /* Set RepeatEN will enable ADC power */
    AD5940_SEQGenInsert(SEQ_NOP());   /* Wait 50us at least to allow ADC settiling. one NOP commands consumes two system clock(32kHz) before ADCCNV = 61.5us. */
    AD5940_LPModeCtrlS(EDA_LPMODCTRLSETS|LPMODECTRL_ADCCNV);   /* Start ADC conversion. !!Clear REPEATEN bit */
    AD5940_SEQGenInsert(SEQ_NOP());
    /* One command need 31.25us because the system clock is 32kHz now. */
    if(i != DFTNumber-1)   /* There is no need to wait such long time for last point, only enough clock for DFT calculation before disable it. */
    {
      AD5940_LPModeCtrlS(LPMODECTRL_NONE);      /* Disable all */
      AD5940_SEQGenInsert(SEQ_WAIT(AppEDACfg.LfoscClkFreq/AppEDACfg.SampleFreq - 12));
    }
    else
    {
      AD5940_LPModeCtrlS(LPMODECTRL_HFOSCEN);   /* Disable all except 16MHz HFOSC */
      AD5940_SEQGenInsert(SEQ_WAIT(21));      /* wait another 21 clocks. DFT need it to calculte last input data */
    }
  }
  /* Stage III: Turn off all we can */
  AD5940_LPModeClkS(LPMODECLK_HFOSC);     /* Switching back to 16MHz */
  AD5940_AFECtrlS(AFECTRL_ALL, bFALSE);   /* Disable waveform generator */

  lpamp_cfg.LpAmpSel = LPAMP0;
  lpamp_cfg.LpPaPwrEn = bFALSE;
  lpamp_cfg.LpTiaPwrEn = bFALSE;
  lpamp_cfg.LpTiaRf = LPTIARF_OPEN;
  lpamp_cfg.LpTiaRtia = LPTIARTIA_OPEN;
  lpamp_cfg.LpTiaSW = 0;  
  AD5940_LPAMPCfgS(&lpamp_cfg);
  AD5940_WriteReg(REG_AFE_LPREFBUFCON, BITM_AFE_LPREFBUFCON_LPBUF2P5DIS|BITM_AFE_LPREFBUFCON_LPREFDIS);
  lpdac_cfg.LpDacSW = 0;    /* Disconnect all switch */
  lpdac_cfg.PowerEn = bFALSE;
  AD5940_LPDACCfgS(&lpdac_cfg);
  AD5940_SEQGpioCtrlS(0/*AGPIO_Pin6|AGPIO_Pin5|AGPIO_Pin1*/);        //GP6->endSeq, GP5 -> AD8233=OFF, GP1->RLD=OFF .
  AD5940_EnterSleepS();/* Go to hibernate */

  /* Sequence end. */
  AD5940_SEQGenCtrl(bFALSE); /* Stop sequencer generator */
  error = AD5940_SEQGenFetchSeq(&pSeqCmd, &SeqLen);
  if(error == AD5940ERR_OK)
  {
    if(AppEDACfg.MaxSeqLen < (SeqLen + AppEDACfg.InitSeqInfo.SeqLen))
      return AD5940ERR_BUFF;    /* Buffer limited */
    AppEDACfg.MeasureSeqInfo.SeqId = SEQID_0;
    AppEDACfg.MeasureSeqInfo.SeqRamAddr = AppEDACfg.InitSeqInfo.SeqRamAddr + AppEDACfg.InitSeqInfo.SeqLen ;
    AppEDACfg.MeasureSeqInfo.pSeqCmd = pSeqCmd;
    AppEDACfg.MeasureSeqInfo.SeqLen = SeqLen;
    AppEDACfg.MeasureSeqInfo.WriteSRAM = bTRUE;
    AD5940_SEQInfoCfg(&AppEDACfg.MeasureSeqInfo); /* Write command to SRAM */
    /* Record where the patch should be applied. */
    AppEDACfg.SeqPatchInfo.SRAMAddr += AppEDACfg.MeasureSeqInfo.SeqRamAddr;  /* The start address in AD5940 SRAM */
  }
  else
    return error; /* Error */
  return AD5940ERR_OK;
}


  fImpCar_Type RtiaCalValue;  /* Calibration result */ // imaginary and real
/**
 * @brief Calibrate LPTIA internal RTIA resistor(s).
 * @details This function will do calibration using parameters stored in @ref AppEDACfg structure.
 * @return return error code.
*/
AD5940Err AppEDARtiaCal(void)
{

  LPRTIACal_Type lprtia_cal;
  AD5940_StructInit(&lprtia_cal, sizeof(lprtia_cal));

  lprtia_cal.LpAmpSel = LPAMP0; // low power amplifier (LPTIA + potentiostat amplifier)
  lprtia_cal.bPolarResult = bFALSE;                /* Real + Image */
  lprtia_cal.AdcClkFreq = AppEDACfg.AdcClkFreq;
  lprtia_cal.SysClkFreq = AppEDACfg.SysClkFreq;
  lprtia_cal.ADCSinc3Osr = ADCSINC3OSR_4;
  lprtia_cal.ADCSinc2Osr = ADCSINC2OSR_22;        /* We don't use SINC2 for now. */
  lprtia_cal.DftCfg.DftNum = DFTNUM_2048;        /* Maximum DFT number */
  lprtia_cal.DftCfg.DftSrc = DFTSRC_SINC2NOTCH;
  lprtia_cal.DftCfg.HanWinEn = bTRUE;
  lprtia_cal.fFreq = AppEDACfg.SinFreq;
  lprtia_cal.fRcal = AppEDACfg.RcalVal;
  lprtia_cal.bWithCtia = bTRUE;
  lprtia_cal.LpAmpPwrMod = LPAMPPWR_NORM;
  lprtia_cal.bWithCtia = bTRUE;
  lprtia_cal.LpTiaRtia = AppEDACfg.LptiaRtiaSel;
  //NRF_LOG_DEBUG("Auto scale parameter = %d\n", AppEDACfg.RtiaAutoScaleEnable );
  
  // if autoscale enable
  if(AppEDACfg.RtiaAutoScaleEnable == bTRUE)
  {
    int i = AppEDACfg.RtiaAutoScaleMin;
    for(;i<=AppEDACfg.RtiaAutoScaleMax; i++)
    {
      lprtia_cal.LpTiaRtia = i;
      AD5940_LPRtiaCal(&lprtia_cal, &RtiaCalValue);
      AppEDACfg.RtiaCalTable[i] = RtiaCalValue;
      //NRF_LOG_DEBUG("Rtia%d,%f,%f\n", i, RtiaCalValue.Real, RtiaCalValue.Image);
    }
    AppEDACfg.RtiaCurrValue = AppEDACfg.RtiaCalTable[AppEDACfg.RtiaIndexCurr];
  }
  else
  {  
    AD5940_LPRtiaCal(&lprtia_cal, &RtiaCalValue);
    AppEDACfg.RtiaCurrValue = RtiaCalValue;
    //NRF_LOG_DEBUG("Rtia,%f,%f\n", RtiaCalValue.Real, RtiaCalValue.Image);
    //NRF_LOG_DEBUG("Rtia calibration done\n");
  }
  return AD5940ERR_OK;
}

/**
 * @brief Initialize the EDA measurement.
 * @details This function must be called before start measurement. It will initialize all needed hardwares and put AD5940 to ready state.
 *          The application parameters stored in @ref AppEDACfg can be changed. Call this function to re-initialize AD5940 with new parameters.
 * @param pBuffer: the buffer for sequencer generator. Only need to provide it for the first time.
 * @param BufferSize: The buffer size start from pBuffer.
 * @return return error code.
*/
AD5940Err AppEDAInit(uint32_t *pBuffer, uint32_t BufferSize)
{
  AD5940Err error = AD5940ERR_OK;
  SEQCfg_Type seq_cfg;// sequencer configuration
  FIFOCfg_Type fifo_cfg;// FIFO configuration

  AppEDACfg.EDAStateCurr = EDASTATE_INIT; // which state is EDA App is in , init/voltage mesaurement / current / RTIA Calibration
  if(AD5940_WakeUp(10) > 10)  /* Wakeup AFE by read register, read 10 times at most */
    return AD5940ERR_WAKEUP;  /* Wakeup Failed */

  #if 1
  // add mux enable for eda measurement            
  // output enable
  uint32_t tempreg;
  tempreg = AD5940_ReadReg(REG_AGPIO_GP0OEN);
  tempreg &= 0xFFE7;
  tempreg |= 0x0018;
  AD5940_AGPIOOen(tempreg);
  
  //pull up / pull down register
  tempreg = AD5940_ReadReg(REG_AGPIO_GP0PE);
  tempreg &= 0xFFE7;
  tempreg |= 0x0018;
  AD5940_AGPIOPen(tempreg);

  //data set register
  tempreg =  AD5940_ReadReg(REG_AGPIO_GP0SET);
  tempreg &= 0xFFE7;
  tempreg |= 0x0008;// set only eda gpio pin as high
  AD5940_AGPIOSet(tempreg);
#endif

  /* Configure sequencer and stop it */
  seq_cfg.SeqMemSize = SEQMEMSIZE_2KB;
  seq_cfg.SeqBreakEn = bFALSE;
  seq_cfg.SeqIgnoreEn = bFALSE;
  seq_cfg.SeqCntCRCClr = bTRUE;
  seq_cfg.SeqEnable = bFALSE;
  seq_cfg.SeqWrTimer = 0;
  AD5940_SEQCfg(&seq_cfg);
  /* Do RTIA calibration */
  if((AppEDACfg.ReDoRtiaCal == bTRUE) || \
      AppEDACfg.EDAInited == bFALSE)  /* Do calibration on the first initialization */
  {
    AppEDACfg.EDAStateCurr = EDASTATE_RTIACAL;
    AppEDARtiaCal();
    AppEDACfg.ReDoRtiaCal = bFALSE;
    //AppEDAMeasureRserial();
  }
  /* Reconfigure FIFO */
  AD5940_FIFOCtrlS(FIFOSRC_DFT, bFALSE);									/* Disable FIFO firstly */
  fifo_cfg.FIFOEn = bTRUE;
  fifo_cfg.FIFOMode = FIFOMODE_FIFO;
  fifo_cfg.FIFOSize = FIFOSIZE_4KB;                       /* 4kB for FIFO, The reset 2kB for sequencer */
  fifo_cfg.FIFOSrc = FIFOSRC_DFT;
  fifo_cfg.FIFOThresh = AppEDACfg.VoltCalPoints*2;       /* The first measurment is for excitation voltage. */
  AD5940_FIFOCfg(&fifo_cfg);

  AD5940_INTCClrFlag(AFEINTSRC_ALLINT);
  
  /* Start sequence generator */
  /* Initialize sequencer generator */
  if((AppEDACfg.EDAInited == bFALSE)||\
       (AppEDACfg.bParaChanged == bTRUE))
  {
    if(pBuffer == 0)  return AD5940ERR_PARA;
    if(BufferSize == 0) return AD5940ERR_PARA;   
    AD5940_SEQGenInit(pBuffer, BufferSize);

    /* Generate initialize sequence */
    error = AppEDASeqCfgGen(); /* Application initialization sequence using either MCU or sequencer */
    if(error != AD5940ERR_OK) return error;

    /* Generate measurement sequence */
    error = AppEDASeqMeasureGen();
    if(error != AD5940ERR_OK) return error;

    AppEDACfg.bParaChanged = bFALSE; /* Clear this flag as we already implemented the new configuration */
  }
  
  /* Initialization sequence  */
  AD5940_SEQCtrlS(bTRUE);  /* Enable sequencer, run initialization sequence */
  AD5940_SEQMmrTrig(AppEDACfg.InitSeqInfo.SeqId);
  while(AD5940_INTCTestFlag(AFEINTC_1, AFEINTSRC_ENDSEQ) == bFALSE);
  
  /* Apply patch for voltage measurment */
  AppEDACfg.EDAStateCurr = EDASTATE_VOLT; /* After initialization, the first thing is to measure excitation voltage */
  AppEDACfg.RtiaIndexCurr = AppEDACfg.RtiaIndexNext = AppEDACfg.LptiaRtiaSel;   /* Init with a value */
  AppEDACfg.SeqPatchInfo.RtiaSel = LPTIARTIA_OPEN;//AppEDACfg.RtiaIndexCurr;
  //AppEDACfg.SeqPatchInfo.bMeasureVolt = bTRUE;
  AppEDACfg.SeqPatchInfo.Type = PATCHTYPE_VOLT;
  error = ApPEDASeqPatchGen(&AppEDACfg.SeqPatchInfo);
  if(error != AD5940ERR_OK)
    return error;
  AD5940_SEQCmdWrite(AppEDACfg.SeqPatchInfo.SRAMAddr, \
                      AppEDACfg.SeqPatchInfo.pSeqCmd, AppEDACfg.SeqPatchInfo.SeqLen); /* Apply the patch to SRAM */

  AD5940_SEQCtrlS(bTRUE);   /* Enable sequencer, and wait for trigger */
  AD5940_ClrMCUIntFlag();   /* Clear interrupt flag generated before */
  AD5940_AFEPwrBW(AFEPWR_LP, AFEBW_250KHZ);

  AD5940_WriteReg(REG_AFE_SWMUX, 0x01);  /**@todo remove it? close switch SW1  */

  if(AppEDACfg.RtiaAutoScaleMin > AppEDACfg.RtiaAutoScaleMax)
  {
    uint32_t temp;
    temp = AppEDACfg.RtiaAutoScaleMin;
    AppEDACfg.RtiaAutoScaleMin = AppEDACfg.RtiaAutoScaleMax;
    AppEDACfg.RtiaAutoScaleMax = temp;
  }
  AppEDACfg.EDAInited = bTRUE;  /* EDA application has been initialized. */
  return AD5940ERR_OK;
}

void AD5940EDAStructInit(void)
{
  AppEDACfg_Type *pCfg;
  
  AppEDAGetCfg(&pCfg);
  pCfg->MaxSeqLen = 512;
  
  pCfg->LfoscClkFreq = 32000;             /* Don't do LFOSC calibration now. We assume the default LFOSC is trimmed. */
  pCfg->RtiaAutoScaleEnable = bFALSE;     /* We manually select resistor value. */
  pCfg->LptiaRtiaSel = LPTIARTIA_10K;    
  pCfg->SinAmplitude = 1100*3/4;          /* Set excitation voltage to 0.75 times of full range. */
  pCfg->SinFreq = 100.0f;                 
  pCfg->SampleFreq = 400.0f;              /* Do not change sample frequency unless you know how it works. */
  pCfg->EDAODR = 16.0f;                    /* ODR decides how freuquently to start the engine to measure impedance. */
  pCfg->FifoThresh = 4;                   /* The minimum threshold value is 4, and should always be 4*N, where N is 1,2,3... */
}


#define APPBUFF_SIZE 1024
static uint32_t AppBuff[APPBUFF_SIZE];
static float LFOSCFreq;    /* Measured LFOSC frequency */

/**
 * @brief The general configuration to AD5940 like FIFO/Sequencer/Clock. 
 * @note This function will firstly reset AD5940 using reset pin.
 * @return return 0.
*/
static int32_t AD5940PlatformCfg(void)
{
  CLKCfg_Type clk_cfg;
  FIFOCfg_Type fifo_cfg;
  SEQCfg_Type seq_cfg;
  AGPIOCfg_Type gpio_cfg;
  LFOSCMeasure_Type LfoscMeasure;

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
  fifo_cfg.FIFOSrc = FIFOSRC_DFT;
  fifo_cfg.FIFOThresh = 4;//AppBIACfg.FifoThresh;        /* DFT result. One pair for RCAL, another for Rz. One DFT result have real part and imaginary part */
  AD5940_FIFOCfg(&fifo_cfg);                             /* Disable to reset FIFO. */
  fifo_cfg.FIFOEn = bTRUE;  
  AD5940_FIFOCfg(&fifo_cfg);
                               /* Enable FIFO here */
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
  


  /* Step4: Configure GPIO */
  gpio_cfg.FuncSet = GP6_SYNC|GP5_SYNC|GP4_SYNC|GP2_EXTCLK|GP1_SYNC|GP0_INT;
  gpio_cfg.InputEnSet = AGPIO_Pin2;
  gpio_cfg.OutputEnSet = AGPIO_Pin0|AGPIO_Pin1|AGPIO_Pin4|AGPIO_Pin5|AGPIO_Pin6;
  gpio_cfg.OutVal = 0;
  gpio_cfg.PullEnSet = 0;
  AD5940_AGPIOCfg(&gpio_cfg);
	
  AD5940_SleepKeyCtrlS(SLPKEY_UNLOCK);  /* Enable AFE to enter sleep mode. */
  
  /* Measure LFOSC frequency */
  /**@note Calibrate LFOSC using system clock. The system clock accuracy decides measurment accuracy. Use XTAL to get better result. */
  LfoscMeasure.CalDuration = 1000.0;  /* 1000ms used for calibration. */
  LfoscMeasure.CalSeqAddr = 0;        /* Put sequence commands from start address of SRAM */
  LfoscMeasure.SystemClkFreq = 16000000.0f; /* 16MHz in this firmware. */
  AD5940_LFOSCMeasure(&LfoscMeasure, &LFOSCFreq);
  
  return 0;
}

int gErrCnt = 0;

#define AD5940_ADIID              0x4144      /**< ADIID is fixed to 0x4144 */



/**
 * @brief Register modification function.
 * @details This function is called in ISR when AFE has been wakeup and we can access registers.
 * @param pData: the buffer points to data read back from FIFO. Not needed for this application-RAMP
 * @param pDataCount: The data count in pData buffer.
 * @return return error code.
*/
static AD5940Err AppEDARegModify(int32_t * const pData, uint32_t *pDataCount)
{
  AD5940Err error = AD5940ERR_OK;
  if(AppEDACfg.EDAStateCurr == EDASTATE_VOLT)
  {
    SWMatrixCfg_Type sw_cfg;
    /* Next step is to measure current */
    AppEDACfg.EDAStateNext = EDASTATE_CURR;
    /* Need change some registers in order to measure current */
    AD5940_SEQCtrlS(bFALSE);                  /* Stop it for now. */
    AD5940_FIFOCtrlS(FIFOSRC_DFT, bFALSE);    /* Disable FIFO firstly because we are going to change FIFO threshold */
    AD5940_FIFOThrshSet(AppEDACfg.FifoThresh);
    AD5940_FIFOCtrlS(FIFOSRC_DFT, bTRUE);     /* Enable FIFO. */

    //arpitha code change
    /* Change Switch matrix settings to connect AIN2(body) to SE0 */
    sw_cfg.Dswitch = SWD_OPEN;  /* Open all switch D */
    //sw_cfg.Dswitch = SWD_CE0;  /* Open all switch D */
    sw_cfg.Pswitch = SWP_AIN2|SWP_SE0;
    sw_cfg.Nswitch = SWN_OPEN;
    sw_cfg.Tswitch = SWT_AIN0|SWT_AFE3LOAD;    /* This switch is for ECG. */
    AD5940_SWMatrixCfgS(&sw_cfg);
    /* Apply patch for current measurment */
    //AppEDACfg.SeqPatchInfo.bMeasureVolt = bFALSE;
    AppEDACfg.SeqPatchInfo.Type = PATCHTYPE_CURR;
    AppEDACfg.SeqPatchInfo.RtiaSel = AppEDACfg.RtiaIndexNext;
    error = ApPEDASeqPatchGen(&AppEDACfg.SeqPatchInfo);
    if(error != AD5940ERR_OK)
      return error;
    AD5940_SEQCmdWrite(AppEDACfg.SeqPatchInfo.SRAMAddr, \
                        AppEDACfg.SeqPatchInfo.pSeqCmd, AppEDACfg.SeqPatchInfo.SeqLen); /* Apply the patch to SRAM */

    AD5940_SEQCtrlS(bTRUE);                   /* Enable sequencer. Sequencer will run when next valid trigger comes */
  }
  else if(AppEDACfg.EDAStateCurr == EDASTATE_CURR)
  {
    if(AppEDACfg.bChangeRtia == bTRUE)
    {
      AppEDACfg.bChangeRtia = bFALSE;
      /* Apply patch for next RTIA selection */
      AppEDACfg.SeqPatchInfo.Type = PATCHTYPE_CURR;
      AppEDACfg.SeqPatchInfo.RtiaSel = AppEDACfg.RtiaIndexNext;
      error = ApPEDASeqPatchGen(&AppEDACfg.SeqPatchInfo);
      if(error != AD5940ERR_OK)
        return error;
      AD5940_SEQCmdWrite(AppEDACfg.SeqPatchInfo.SRAMAddr, \
                        AppEDACfg.SeqPatchInfo.pSeqCmd, AppEDACfg.SeqPatchInfo.SeqLen); /* Apply the patch to SRAM */
    }
  }
  
  if(AppEDACfg.bMeasVoltReq == bTRUE)
  {
    SWMatrixCfg_Type sw_cfg;
    AppEDACfg.bMeasVoltReq = bFALSE;    /* Clear this request */
    /* Next step is to measure voltage */
    AppEDACfg.EDAStateNext = EDASTATE_VOLT;

    /* Change Switch matrix settings to connect AIN2(body) to SE0 */
    sw_cfg.Dswitch = SWD_OPEN;  /* Open all switch D */
    sw_cfg.Pswitch = SWP_OPEN;
    sw_cfg.Nswitch = SWN_OPEN;
    sw_cfg.Tswitch = SWT_AIN0|SWT_AFE3LOAD;    /* This switch is for ECG. */
    AD5940_SWMatrixCfgS(&sw_cfg);

    /* Need change some registers in order to measure current */
    AD5940_SEQCtrlS(bFALSE);                  /* Stop it for now. */
    AD5940_FIFOCtrlS(FIFOSRC_DFT, bFALSE);    /* Disable FIFO firstly because we are going to change FIFO threshold */
    AD5940_FIFOThrshSet(AppEDACfg.VoltCalPoints*2);
    AD5940_FIFOCtrlS(FIFOSRC_DFT, bTRUE);     /* Enable FIFO. */

    /* Apply patch for current measurment */
    AppEDACfg.SeqPatchInfo.Type = PATCHTYPE_VOLT;
    AppEDACfg.SeqPatchInfo.RtiaSel = LPTIARTIA_OPEN;//AppEDACfg.RtiaIndexNext;
    error = ApPEDASeqPatchGen(&AppEDACfg.SeqPatchInfo);
    if(error != AD5940ERR_OK)
      return error;
    AD5940_SEQCmdWrite(AppEDACfg.SeqPatchInfo.SRAMAddr, \
                        AppEDACfg.SeqPatchInfo.pSeqCmd, AppEDACfg.SeqPatchInfo.SeqLen); /* Apply the patch to SRAM */

    AD5940_SEQCtrlS(bTRUE);                   /* Enable sequencer. Sequencer will run when next valid trigger comes */
  }

  if(AppEDACfg.NumOfData > 0)
  {
    AppEDACfg.FifoDataCount += *pDataCount/4;
    if(AppEDACfg.FifoDataCount >= AppEDACfg.NumOfData)
    {
      AD5940_WUPTCtrl(bFALSE);
      return AD5940ERR_OK;
    }
  }
  if(AppEDACfg.StopRequired == bTRUE)
  {
    AD5940_WUPTCtrl(bFALSE);
    return AD5940ERR_OK;
  }
  return AD5940ERR_OK;
}

/**
 * @brief Depending on the data type, do appropriate data pre-process before return back to controller
 * @param pImpedance: the buffer points to pre-processed data. We use the impedance magnitude value to decide new RTIA settings.
 * @param uiDataCount: The data count in pData buffer.
 * @return return the next appropriate RTIA index value.
*/
static uint32_t EDARtiaAutoScaling(fImpCar_Type * const pImpedance, uint32_t uiDataCount)
{
  uint32_t OptRtiaIndex;
  float MagMean = 0;
  fImpCar_Type SumImp={0,0};

  /* Get Averaged Magnitude Result */
  for(int i=0;i<uiDataCount;i++)
  {
    SumImp.Real += pImpedance[i].Real;
    SumImp.Image += pImpedance[i].Image;
  }
  SumImp.Real /= uiDataCount;
  SumImp.Image /= uiDataCount;
  SumImp = AD5940_ComplexAddFloat(&SumImp, &AppEDACfg.ImpEDABase); /* Impedance under test is sum of changed value and baseline */
  MagMean = AD5940_ComplexMag(&SumImp);
  OptRtiaIndex = AppEDACfg.RtiaAutoScaleMin;
  /* This is much easier because although the RTIA is not the best value, the results are still reliable. We can directly choose the RTIA matched */
  for(;OptRtiaIndex < AppEDACfg.RtiaAutoScaleMax;)
  {
    float mag;
    mag = AD5940_ComplexMag(&AppEDACfg.RtiaCalTable[OptRtiaIndex+1]);
    if(MagMean < mag*0.97f) /* @todo add threshold?? */
      break;
    OptRtiaIndex ++;
  }
  return OptRtiaIndex;
}


/**
 * @brief Data pre-process
 * @details Depending on the data type, do appropriate data pre-process before return back to controller
 * @param pData: the buffer points to data read back from FIFO. Not needed for this application-RAMP
 * @param pDataCount: The data count in pData buffer.
 * @return return error code.
*/
int32_t SumReal = 0, SumImage = 0;
static AD5940Err AppEDADataProcess(int32_t * const pData, uint32_t *pDataCount)
{
  uint32_t DataCount = *pDataCount;
  
  *pDataCount = 0;

  /* EDA results are DFT results */
  for(uint32_t i=0; i<DataCount; i++)
  {
    pData[i] &= 0x3ffff; /* @todo option to check ECC */
    if(pData[i]&(1<<17)) /* Bit17 is sign bit */
      pData[i] |= 0xfffc0000; /* Data is 18bit in two's complement, bit17 is the sign bit */
    
    //NRF_LOG_DEBUG("Data[%] = %d",i,pData[i]);
  }

  if(AppEDACfg.EDAStateCurr == EDASTATE_VOLT)
  {
    uint32_t DftResCnt;
    iImpCar_Type *pDftRes = (iImpCar_Type*)pData;
	SumReal = 0, SumImage = 0;
    /* Get average excitation voltage */
    if(DataCount != AppEDACfg.VoltCalPoints*2)
      return EDAERR_VOLTMEASURE;
    DftResCnt = DataCount/2;
    if(DftResCnt > 4)
    {
      DftResCnt -= 4;
      pDftRes += 4; /* Discard the first 4 results */
    }
    for(uint32_t i=0;i<DftResCnt;i++)
    {
      SumReal += pDftRes[i].Real;
      SumImage += pDftRes[i].Image;
      //NRF_LOG_DEBUG("Volt, %d, %d\n", pDftRes[i].Real, pDftRes[i].Image);
    }
    SumReal /= (int32_t)DftResCnt;
    SumImage /= (int32_t)DftResCnt; /* Get average result */
    SumImage = -SumImage;           /* Fix sign of imaginary part of DFT result. */

    AppEDACfg.ExcitVolt.Real = SumReal;
    AppEDACfg.ExcitVolt.Image = SumImage;
    //NRF_LOG_DEBUG("Volt:%f,%f\n", AppEDACfg.ExcitVolt.Real, AppEDACfg.ExcitVolt.Image);
    /* Done */
    *pDataCount = 0; /* Don't return voltage result */
  }
  else if(AppEDACfg.EDAStateCurr == EDASTATE_CURR)/* The FIFO data is current result. We need to calculate impedance, Z=V/I */
  {
    iImpCar_Type * const pSrc = (iImpCar_Type*)pData;
    fImpCar_Type * const pOut = (fImpCar_Type*)pData;
    for(uint32_t i=0; i<DataCount/2; i++)
    {
      fImpCar_Type DftCurr;
      fImpCar_Type res;

      DftCurr.Real = (float)pSrc[i].Real;
      DftCurr.Image = (float)pSrc[i].Image;
      DftCurr.Image = -DftCurr.Image;
      DftCurr.Real = -DftCurr.Real;
      DftCurr.Image = -DftCurr.Image;
      res = AD5940_ComplexDivFloat(&DftCurr, &AppEDACfg.RtiaCurrValue);           /* I=Vrtia/Zrtia */
      res = AD5940_ComplexDivFloat(&AppEDACfg.ExcitVolt, &res);
      AppEDACfg.ImpSum = AD5940_ComplexAddFloat(&AppEDACfg.ImpSum ,&res);
      AppEDACfg.ImpSumCount ++;
      res = AD5940_ComplexSubFloat(&res, &AppEDACfg.ImpEDABase);
      pOut[i] = res;
    }
    *pDataCount = DataCount/2; /* Impedance result */

    /* Process RTIA autoscaling calculation */
    if(AppEDACfg.RtiaAutoScaleEnable)
    {
      static uint32_t rtia_pre = (uint32_t)-1; /* Init to invalid value */
      uint32_t rtia_index;

      AppEDACfg.RtiaIndexCurr = AppEDACfg.RtiaIndexNext;
      AppEDACfg.RtiaCurrValue = AppEDACfg.RtiaCalTable[AppEDACfg.RtiaIndexCurr];

      rtia_index = EDARtiaAutoScaling(pOut,*pDataCount);
      if(rtia_index != rtia_pre)
      {
        AppEDACfg.bChangeRtia = bTRUE;
        rtia_pre = rtia_index;
        AppEDACfg.RtiaIndexNext = rtia_index;
      }
    }
  }
  AppEDACfg.EDAStateCurr = AppEDACfg.EDAStateNext;    /* We have to move forward the state at end of data processing */
  return AD5940ERR_OK;
}

/**
 * @brief The interrupt service routine for EDA.
 * @param pBuff: The buffer provides by host, used to store data read back from FIFO.
 * @param pCount: The available buffer size starts from pBuff.
 * @return return error code.
*/
 uint32_t FifoCnt;
AD5940Err AppEDAISR(void *pBuff, uint32_t *pCount)
{
  uint32_t BuffCount;
  BuffCount = *pCount;
  if(AppEDACfg.EDAInited == bFALSE)
    return AD5940ERR_APPERROR;
  if(AD5940_WakeUp(10) > 10)  /* Wakeup AFE by read register, read 10 times at most */
    return AD5940ERR_WAKEUP;  /* Wakeup Failed */
  AD5940_SleepKeyCtrlS(SLPKEY_LOCK);  /* Don't enter hibernate */
  *pCount = 0;

  if(AD5940_INTCTestFlag(AFEINTC_0, AFEINTSRC_DATAFIFOTHRESH) == bTRUE)
  {
    /* Now there should be 4 data in FIFO */
    FifoCnt = (AD5940_FIFOGetCnt()/4)*4;
    
    if(FifoCnt > BuffCount)
    {
      //@todo buffer is limited.
    }
    AD5940_FIFORd((int32_t *)pBuff, FifoCnt);
    AD5940_INTCClrFlag(AFEINTSRC_DATAFIFOTHRESH);
    AppEDARegModify(pBuff, &FifoCnt);   /* If there is need to do AFE re-configure, do it here when AFE is in active state */
    //AD5940_EnterSleepS();  /* Manually put AFE back to hibernate mode. This operation only takes effect when register value is ACTIVE previously */
    AD5940_SleepKeyCtrlS(SLPKEY_UNLOCK);  /* Don't enter hibernate */
    /* Process data */ 
    AppEDADataProcess((int32_t*)pBuff,&FifoCnt); 
    *pCount = FifoCnt;
    return 0;
  }
  //NRF_LOG_DEBUG("Leave Int.\n");
  
  return 0;
} 
//static int ind=0;
#define NO_OF_EDA_MEASUREMENTS 20
  float mag, phase;
int flag=0;
int count=0;

/* print EDA result to uart */
AD5940Err EDAShowResult(void *pData, uint32_t DataCount)
{
  float RtiaMag;
  /*Process data*/
  fImpCar_Type *pImp = (fImpCar_Type*)pData;
  AppEDACtrl(EDACTRL_GETRTIAMAG, &RtiaMag);
 // NRF_LOG_INFO("Data count = %d",DataCount);
  mag=phase=0;
  /*Process data*/
  for(int i=0;i<DataCount;i++)
  {
    fImpCar_Type res;
    res = pImp[i];
    res.Real += ResistorForBaseline;    /* Show the real result of impedance under test(between F+/S+) */
    mag = AD5940_ComplexMag(&res);
    phase = AD5940_ComplexPhase(&res)*180/MATH_PI;
    //NRF_LOG_DEBUG("Rtia:%.2f,(Real,Image):(%.2f,%.2f)Ohm---Mag:%.2fOhm,Phase:%.2f\n",RtiaMag, res.Real, res.Image, mag, phase);
    flag = 1;
    count++;
  }
  return 0;
}

void AD5940_Main_EDA(void)
{

  uint32_t temp;
  fImpCar_Type EDABase = 
  {
    .Real = 24368.6,
    .Image = -16.696,
  };
  InitCfg();
  AD5940PlatformCfg();
  
  #if 0
   AGPIOCfg_Type gpio_cfg;
    gpio_cfg.FuncSet = GP6_SYNC|GP5_SYNC|GP4_SYNC|GP2_EXTCLK|GP1_SYNC|GP0_INT;
   gpio_cfg.InputEnSet = AGPIO_Pin2;
   gpio_cfg.OutputEnSet = AGPIO_Pin0|AGPIO_Pin1|AGPIO_Pin4|AGPIO_Pin5|AGPIO_Pin6;
   gpio_cfg.OutVal = 0;
   gpio_cfg.PullEnSet = 0;
   AD5940_AGPIOCfg(&gpio_cfg);
#endif 

  AD5940EDAStructInit(); /* Configure your parameters in this function */
  
  AppEDAInit(AppBuff, APPBUFF_SIZE);    /* Initialize BIA application. Provide a buffer, which is used to store sequencer commands */
  AppEDACtrl(APPCTRL_START, 0);         /* Control BIA measurment to start. Second parameter has no meaning with this command. */
  AppEDACtrl(EDACTRL_SETBASE, &EDABase);
  
  /* Step4: Configure GPIO */
  ResistorForBaseline = 19900;          /* Above result is obtained using 20kOhm resistor on BioElec Rev C board. */
  mag=phase=0;
  while(!flag)
  {
    /* Check if interrupt flag which will be set when interrupt occured. */
    if(AD5940_GetMCUIntFlag())
    {
       AD5940_ClrMCUIntFlag(); /* Clear this flag */
      temp = APPBUFF_SIZE;
      AppEDAISR(AppBuff, &temp); /* Deal with it and provide a buffer to store data we got */
      EDAShowResult(AppBuff, temp); /* Show the results to UART */
      
    }
  }
  if(flag == 1)
  {
    AD5940_LPModeClkS(LPMODECLK_HFOSC); /* Trigger switching system clock to 32kHz */
    //issue stop
    AppEDACtrl(APPCTRL_STOPSYNC, 0);  /* Stop the measurement if it's running. */
    nrf_delay_ms(2000);
    flag = 0;
    count = 0;
    AD5940_Main_EDA(); //call start again
  }
  //NRF_LOG_DEBUG("Main exit");
}


uint32_t temp;
