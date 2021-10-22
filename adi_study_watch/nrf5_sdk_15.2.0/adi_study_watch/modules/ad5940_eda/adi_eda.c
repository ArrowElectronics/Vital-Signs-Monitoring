/**
****************************************************************************
* @file     adi_eda.c
* @author   ADI
* @version  V0.1
* @date     08-June-2021
* @brief    EDA measurement on AD5940
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

#ifdef ENABLE_EDA_APP
#include "eda_application_task.h"
#include <adpd4000_dcfg.h>
#include <ecg_task.h>
#include <includes.h>
#include <limits.h>
#include <power_manager.h>
#include <stdint.h>
#include <string.h>
#include "us_tick.h"
#include "nrf_log_ctrl.h"

extern AppEDACfg_Type AppEDACfg;
extern volatile int16_t eda_user_applied_odr;
extern volatile int16_t eda_user_applied_dftnum;
extern volatile int16_t eda_user_applied_rtia_cal;
extern uint8_t rtia_cal_completed;
static float LFOSCFreq; /* Measured LFOSC frequency */
extern uint8_t measurement_cycle_completed;
extern uint8_t init_flag;
extern uint32_t ResistorForBaseline;
extern uint32_t AppBuff[APPBUFF_SIZE];
#ifdef EDA_DCFG_ENABLE
extern uint8_t eda_load_applied_dcfg;
#endif
#ifndef EXTERNAL_TRIGGER_EDA
uint32_t ad5940_port_Init(void);
#endif

/*!
 ****************************************************************************
 *@brief      This function is provided for upper controllers that want to
 *            change application parameters specially for user defined
 *            parameters
 *@param      pCfg: pointer to configuration structure
 *@return     AD5940Err
 ******************************************************************************/
AD5940Err AppEDAGetCfg(void *pCfg) {
  if (pCfg) {
    *(AppEDACfg_Type **)pCfg = &AppEDACfg;
    return AD5940ERR_OK;
  }
  return AD5940ERR_PARA;
}

/*!
 ****************************************************************************
 * @brief Control application like start, stop.
 * @param EDACtrl The command for this applicaiton, select from below paramters
 *        - APPCTRL_START: start the measurment. Note: the ramp test need
 *            to call function AppRAMPInit() every time before start it.
 *        - APPCTRL_STOPNOW: Stop the measurment immediately.
 *        - APPCTRL_STOPSYNC: Stop the measuremnt when current measured data
 *                             is read back.
 *        - APPCTRL_SHUTDOWN: Stop the measurment immediately and put AFE to
 *           shut down mode(turn off LP loop and enter hibernate).
 *        - EDACTRL_MEASVOLT: Measure voltage once current measurment
 *            is done(Interrupt occured).
 *        - EDACTRL_GETRTIAMAG: Get current RTIA value.
 * @param pPara: pointer to return state
 * @return AD5940Err.
 *******************************************************************************/
AD5940Err AppEDACtrl(int32_t EDACtrl, void *pPara) {
  switch (EDACtrl) {
  case APPCTRL_START: {
#ifndef EXTERNAL_TRIGGER_EDA	
    WUPTCfg_Type wupt_cfg;
    /* Wakeup AFE by read register, read 10 times at most */
    if (AD5940_WakeUp(10) > 10)
      return AD5940ERR_WAKEUP; /* Wakeup Failed */
    if (AppEDACfg.EDAInited == bFALSE)
      return AD5940ERR_APPERROR;
    /* Start it */
    wupt_cfg.WuptEn = bTRUE;
    wupt_cfg.WuptEndSeq = WUPTENDSEQ_A;
    wupt_cfg.WuptOrder[0] = SEQID_0;
    wupt_cfg.SeqxSleepTime[SEQID_0] =
        (uint32_t)(AppEDACfg.LfoscClkFreq / AppEDACfg.EDAODR) - 2 - 4;
    /* The minimum value is 1. Do not set it to zero. Set it to 1 will spend 2
     * 32kHz clock */
    wupt_cfg.SeqxWakeupTime[SEQID_0] = 4;
    AD5940_WUPTCfg(&wupt_cfg);
#endif   
    /* flags to indicate status of eda app and init fifo data count */
    AppEDACfg.FifoDataCount = 0; /* restart */
    AppEDACfg.bRunning = bTRUE;
    break;
  }
  case APPCTRL_STOPNOW: {
    /* Wakeup AFE by read register, read 10 times at most */
    if (AD5940_WakeUp(10) > 10)
      return AD5940ERR_WAKEUP; /* Wakeup Failed */
#ifndef EXTERNAL_TRIGGER_EDA	
    /* Start Wupt right now */
    AD5940_WUPTCtrl(bFALSE);
    /* There is chance this operation will fail because sequencer could put AFE
      back to hibernate mode just after waking up. Use STOPSYNC is better. */
    AD5940_WUPTCtrl(bFALSE);
#endif
    AppEDACfg.bRunning = bFALSE;
    break;
  }
  case APPCTRL_STOPSYNC: {
    AppEDACfg.StopRequired = bTRUE;
    break;
  }
  case APPCTRL_SHUTDOWN: {
    /* Stop the measurement if it's running. */
    AppEDACtrl(APPCTRL_STOPNOW, 0);
    /* Turn off LPLoop related blocks which are not controlled
     * automatically by hibernate operation */
    AFERefCfg_Type aferef_cfg;
    LPLoopCfg_Type lp_loop;
    memset(&aferef_cfg, 0, sizeof(aferef_cfg));
    AD5940_REFCfgS(&aferef_cfg);
    memset(&lp_loop, 0, sizeof(lp_loop));
    AD5940_LPLoopCfgS(&lp_loop);
    AD5940_EnterSleepS(); /* Enter Hibernate */
  } break;
  case EDACTRL_MEASVOLT:
    AppEDACfg.bMeasVoltReq = bTRUE;
    break;
  case EDACTRL_GETRTIAMAG:
    if (pPara == NULL)
      return AD5940ERR_NULLP; /* Null pointer */
    *(float *)pPara = AD5940_ComplexMag(&AppEDACfg.RtiaCurrValue);
    break;
  case EDACTRL_RSTBASE:
    AppEDACfg.ImpEDABase.Real = 0;
    AppEDACfg.ImpEDABase.Image = 0;
    AppEDACfg.ImpSum.Real = 0;
    AppEDACfg.ImpSum.Image = 0;
    AppEDACfg.ImpSumCount = 0;
    break;
  case EDACTRL_SETBASE: {
    /* The impedance used to set base line */
    fImpCar_Type *pImpBase = (fImpCar_Type *)pPara;
    AppEDACfg.ImpEDABase = *pImpBase;
  } break;
  case EDACTRL_GETAVR:
    if (pPara == NULL)
      return AD5940ERR_NULLP;
    {
      fImpCar_Type *pImpAVR = (fImpCar_Type *)pPara;
      pImpAVR->Real = AppEDACfg.ImpSum.Real / AppEDACfg.ImpSumCount;
      pImpAVR->Image = AppEDACfg.ImpSum.Image / AppEDACfg.ImpSumCount;
      break;
    }
  case APPCTRL_RUNNING:
  case EDACTRL_STATUS:
    if (pPara == NULL)
      return AD5940ERR_NULLP; /* Null pointer */
    *(BoolFlag *)pPara = AppEDACfg.bRunning;
    break;
  default:
    break;
  }
  return AD5940ERR_OK;
}

/*!
 ****************************************************************************
 *@brief      Eda application sequencer configuration generate
 *@param      None
 *@return     AD5940Err
 ******************************************************************************/
AD5940Err AppEDASeqCfgGen(void) {
  AD5940Err error = AD5940ERR_OK;
  const uint32_t *pSeqCmd;
  uint32_t SeqLen;
  AFERefCfg_Type aferef_cfg;
  /* Waveform Generator uses some parameter(DAC update rate)
   * from HSDAC config registers */
  HSDACCfg_Type hsdac_cfg;
  LPLoopCfg_Type lp_loop;
  WGCfg_Type wg_cfg;
  DSPCfg_Type dsp_cfg;
  SWMatrixCfg_Type sw_cfg;

  AD5940_SEQGenCtrl(bTRUE);
#ifdef EDA_DCFG_ENABLE
  write_ad5940_seqcfg();
#else
  /* Sequence starts here */
  AD5940_SEQGpioCtrlS(AGPIO_Pin6 | AGPIO_Pin1);
  AD5940_StructInit(&aferef_cfg, sizeof(aferef_cfg));
  /* Turn off all references, only enable it when needed */
  AD5940_REFCfgS(&aferef_cfg);
  /* Disable everything, configure them during measurement */
  AD5940_StructInit(&lp_loop, sizeof(lp_loop));
  AD5940_LPLoopCfgS(&lp_loop);
  AD5940_StructInit(&wg_cfg, sizeof(wg_cfg));
  /* Maximum amplitude is 1100mV */
  wg_cfg.SinCfg.SinAmplitudeWord =
      (uint32_t)(AppEDACfg.SinAmplitude / 1100.0f * 2047);
  wg_cfg.SinCfg.SinFreqWord = AD5940_WGFreqWordCal(AppEDACfg.SinFreq, AppEDACfg.LfoscClkFreq);
  wg_cfg.SinCfg.SinPhaseWord = 0;
  wg_cfg.WgType = WGTYPE_SIN;
  AD5940_WGCfgS(&wg_cfg);
  AD5940_AFECtrlS(AFECTRL_ALL, bFALSE);
  /* Switch configuration for BioElec board */
  sw_cfg.Dswitch = SWD_OPEN; /* Open all switch D */
  sw_cfg.Pswitch = SWP_AIN2 | SWP_SE0;
  sw_cfg.Nswitch = SWN_OPEN;
  sw_cfg.Tswitch = SWT_AIN0 | SWT_AFE3LOAD;
  AD5940_SWMatrixCfgS(&sw_cfg);
  AD5940_StructInit(&dsp_cfg, sizeof(dsp_cfg));
  dsp_cfg.ADCBaseCfg.ADCMuxN = ADCMUXN_VSET1P1;
  dsp_cfg.ADCBaseCfg.ADCMuxP = ADCMUXP_AIN4;
  dsp_cfg.ADCBaseCfg.ADCPga = ADCPGA_1;
  dsp_cfg.ADCFilterCfg.ADCRate = ADCRATE_800KHZ;
  /* Use averaged SINC3 output as DFT input source */
  dsp_cfg.ADCFilterCfg.ADCAvgNum = ADCAVGNUM_4;
  dsp_cfg.ADCFilterCfg.ADCSinc2Osr = ADCSINC2OSR_22;
  dsp_cfg.ADCFilterCfg.ADCSinc3Osr = ADCSINC3OSR_5;
  dsp_cfg.ADCFilterCfg.BpNotch = bTRUE;
  dsp_cfg.ADCFilterCfg.BpSinc3 = bFALSE;
  dsp_cfg.ADCFilterCfg.Sinc2NotchEnable = bFALSE;
  dsp_cfg.DftCfg.DftNum = AppEDACfg.DftNum;
  /* Use averaged SINC3 data */
  dsp_cfg.DftCfg.DftSrc = DFTSRC_AVG;
  dsp_cfg.DftCfg.HanWinEn = AppEDACfg.HanWinEn;
  AD5940_DSPCfgS(&dsp_cfg);
  /* (n+2)*osr + 1, n=4,osr=5 */
  AD5940_ADCRepeatCfgS(5 * (4 + 2) + 1);
  hsdac_cfg.ExcitBufGain = EXCITBUFGAIN_2;
  hsdac_cfg.HsDacGain = HSDACGAIN_1;
  /* Note: the DAC update rate is decided by register DACON.RATE */
  hsdac_cfg.HsDacUpdateRate = AppEDACfg.DacUpdateRate;
  AD5940_HSDacCfgS(&hsdac_cfg);
#endif //EDA_DCFG_ENABLE
  /* GP6->endSeq, GP5 -> AD8233=OFF, GP1->RLD=OFF */
  AD5940_SEQGpioCtrlS(0);
  /* Sequence end. */
  /* Add one extra command to disable sequencer for initialization
   * sequence because it is needed only at run one time. */
  AD5940_SEQGenInsert(SEQ_STOP());
  /* Stop here */
  /* Stop sequencer generator */
  AD5940_SEQGenCtrl(bFALSE);
  error = AD5940_SEQGenFetchSeq(&pSeqCmd, &SeqLen);
  if (error == AD5940ERR_OK) {
    AppEDACfg.InitSeqInfo.SeqId = SEQID_1;
    AppEDACfg.InitSeqInfo.SeqRamAddr = AppEDACfg.SeqStartAddr;
    AppEDACfg.InitSeqInfo.pSeqCmd = pSeqCmd;
    AppEDACfg.InitSeqInfo.SeqLen = SeqLen;
    AppEDACfg.InitSeqInfo.WriteSRAM = bTRUE;
    /* Write command to SRAM */
    AD5940_SEQInfoCfg(&AppEDACfg.InitSeqInfo);
  } else {
    return error; /* Error */
  }
  return AD5940ERR_OK;
}

/*!
 ****************************************************************************
 * @brief Generate patch sequence according to current measurement
 *            type(Voltage or Current).
 * @details The patch is used to adjust sequencer commands already
 *   stored in SRAM of AD5940 in order to perform different measurments.
 *   The reason is that the sequences need to be adjusted. Using the patch
 *   method will make things easiy and we won't need to modify
 *   sequences in register level.
 * @param   pPatch: pointer to patch information include the measurement
 *                   type, Rtia selection and buffers.
 * @return  AD5940Err: error code.
 ******************************************************************************/
AD5940Err ApPEDASeqPatchGen(SeqPatchInfo_Type *pPatch) {
  AD5940Err err;
  const uint32_t *pSeqCmd;
  uint32_t SeqLen;
  LPAmpCfg_Type lpamp_cfg;
  AD5940_SEQGenInit(pPatch->Buffer, pPatch->BuffLen);
  AD5940_SEQGenCtrl(bTRUE);
  lpamp_cfg.LpAmpSel = LPAMP0;
  /* Use normal power mode is enough */
  lpamp_cfg.LpAmpPwrMod = LPAMPPWR_NORM;
  /* Enable Potential amplifier */
  lpamp_cfg.LpPaPwrEn = bTRUE;
  /* Enable TIA amplifier */
  lpamp_cfg.LpTiaPwrEn = bTRUE;
  /* Rf resistor controls cut off frequency. */
  lpamp_cfg.LpTiaRf = LPF_RF;
  /** @note Use 100Ohm Rload. */
  lpamp_cfg.LpTiaRload = LPTIARLOAD_100R;
  /* If autoscaling is enabled, use selected value. */
  lpamp_cfg.LpTiaRtia = pPatch->RtiaSel;
  /* Swtich settings for voltage measurement */
  if (pPatch->Type == PATCHTYPE_VOLT)
    lpamp_cfg.LpTiaSW = LPTIASW_VOLT;
  /* Switch settings for current measurement */
  else if (pPatch->Type == PATCHTYPE_CURR)
    lpamp_cfg.LpTiaSW = LPTIASW_CURR;
  AD5940_LPAMPCfgS(&lpamp_cfg);
  AD5940_SEQGenCtrl(bFALSE);
  err = AD5940_SEQGenFetchSeq(&pSeqCmd, &SeqLen);
  if (err != AD5940ERR_OK)
    return err;
  pPatch->pSeqCmd = pSeqCmd;
  pPatch->SeqLen = SeqLen;
  return AD5940ERR_OK;
}

/*!
 ****************************************************************************
 *@brief      Eda application sequencer Measurement mode command generate
 *             and write to SRAM
 *@param      None
 *@return     AD5940Err
 ******************************************************************************/
AD5940Err AppEDASeqMeasureGen(void) {
  AD5940Err error = AD5940ERR_OK;
  const uint32_t *pSeqCmd;
  uint32_t SeqLen;
  uint32_t i;
  uint32_t DFTNumber;
  LPDACCfg_Type lpdac_cfg;
  LPAmpCfg_Type lpamp_cfg;
  /* Start sequence generator here */
  AD5940_SEQGenCtrl(bTRUE);
#ifdef EDA_DCFG_ENABLE
  write_ad5940_seqmeasurement();
#else
  /* Stage I: Initialization */
#ifdef EXTERNAL_TRIGGER_EDA	
  /* GP6->endSeq, GP5 -> AD8233=OFF, GP1->RLD=OFF */
  AD5940_SEQGpioCtrlS(AGPIO_Pin6);
#else
  /* GP6->endSeq, GP5 -> AD8233=OFF,GP3 ->ELECTRODESWITCH=ON, GP1->RLD=OFF */
  AD5940_SEQGpioCtrlS(AGPIO_Pin3 | AGPIO_Pin6);
#endif  
  /* LP loop configure: LPDAC and LPAMP */
  lpdac_cfg.LpdacSel = LPDAC0;
  lpdac_cfg.DataRst = bFALSE;
  lpdac_cfg.LpDacSW =
      LPDACSW_VBIAS2LPPA | LPDACSW_VZERO2LPTIA | LPDACSW_VZERO2PIN;
  /* Use internal 2.5V reference */
  lpdac_cfg.LpDacRef = LPDACREF_2P5;
  /* Use data from waveform generator */
  lpdac_cfg.LpDacSrc = LPDACSRC_WG;
  lpdac_cfg.LpDacVbiasMux = LPDACVBIAS_12BIT;
  /* Use 6bit LPDAC for Vzero */
  lpdac_cfg.LpDacVzeroMux = LPDACVZERO_6BIT;
  /* Enable LPDAC */
  lpdac_cfg.PowerEn = bTRUE;
  /* Don't care, 12bit DAC data is from WG */
  lpdac_cfg.DacData12Bit = 0;
  /* Set it to middle scale of LPDAC. Vzero is the
   * bias voltage of LPTIA amplifier */
  lpdac_cfg.DacData6Bit = 32;
  AD5940_LPDACCfgS(&lpdac_cfg);
  /* Voltage and current measurment need different switch settings,
   *  record the difference and only modify this part
   *  for different purpose */
  /* Record the start address of below commands */
  error = AD5940_SEQGenFetchSeq(NULL, &AppEDACfg.SeqPatchInfo.SRAMAddr);
  if (error != AD5940ERR_OK)
    return error;
  lpamp_cfg.LpAmpSel = LPAMP0;
  /* Use normal power mode is enough */
  lpamp_cfg.LpAmpPwrMod = LPAMPPWR_NORM;
  /* Enable Potential amplifier */
  lpamp_cfg.LpPaPwrEn = bTRUE;
  /* Enable TIA amplifier */
  lpamp_cfg.LpTiaPwrEn = bTRUE;
  /* Rf resistor controls cut off frequency. */
  lpamp_cfg.LpTiaRf = LPF_RF;
  /** @note Use 100Ohm Rload. */
  lpamp_cfg.LpTiaRload = LPTIARLOAD_100R;
  /* If autoscaling is enabled, then use selected value. */
  lpamp_cfg.LpTiaRtia = AppEDACfg.LptiaRtiaSel;
  /* Switch settings for voltage measurement */
  lpamp_cfg.LpTiaSW = LPTIASW_VOLT;
  AD5940_LPAMPCfgS(&lpamp_cfg);
  /* Enable low power bandgap and 2.5V reference buffer */
  AD5940_WriteReg(REG_AFE_LPREFBUFCON, 0);
  /* Switch Off everything */
  AD5940_AFECtrlS(AFECTRL_ALL, bFALSE);
  /* Enter LP control mode. The registers are summarized to
   * LPMODECON, so we can control some blocks conveniently */
  AD5940_LPModeEnS(bTRUE);
  /* Trigger switching system clock to 32kHz */
  AD5940_LPModeClkS(LPMODECLK_LFOSC);
  /* Disable all */
  AD5940_LPModeCtrlS(LPMODECTRL_NONE);
  /* Start waveform generator */
  AD5940_AFECtrlS(AFECTRL_WG, bTRUE);
  /* wait for stable state */
  AD5940_SEQGenInsert(SEQ_WAIT(LPF_TIME * 32));
  /* Enable DFT engine */
  AD5940_AFECtrlS(AFECTRL_DFT, bTRUE);
  /* Stage II: ADC Run to sample enough data(DFT number) */
  DFTNumber = (1 << (AppEDACfg.DftNum + 2));
  for (i = 0; i < DFTNumber; i++) {
#define EDA_LPMODCTRLSETS                                                      \
  LPMODECTRL_GLBBIASZ | LPMODECTRL_GLBBIASP | LPMODECTRL_HPREFPWR |            \
      LPMODECTRL_BUFHP1P8V | LPMODECTRL_BUFHP1P1V | LPMODECTRL_HFOSCEN
    /* Turn ON HPREF */
    AD5940_LPModeCtrlS(EDA_LPMODCTRLSETS);
    AD5940_SEQGenInsert(SEQ_WAIT(4));
    /* Set RepeatEN will enable ADC power */
    AD5940_LPModeCtrlS(EDA_LPMODCTRLSETS | LPMODECTRL_REPEATEN);
    /* Wait 50us at least to allow ADC settiling. one NOP commands
     * consumes two system clock(32kHz) before ADCCNV = 61.5us. */
    AD5940_SEQGenInsert(SEQ_NOP());
    /* Start ADC conversion. !!Clear REPEATEN bit */
    AD5940_LPModeCtrlS(EDA_LPMODCTRLSETS | LPMODECTRL_ADCCNV);
    AD5940_SEQGenInsert(SEQ_NOP());
    /* One command need 31.25us because the system clock is 32kHz now. */
    /* There is no need to wait such long time for last point, only
     * enough clock for DFT calculation before disable it. */
    if (i != DFTNumber - 1) {
      /* Disable all */
      AD5940_LPModeCtrlS(LPMODECTRL_NONE);
      AD5940_SEQGenInsert(
          SEQ_WAIT(AppEDACfg.LfoscClkFreq / AppEDACfg.SampleFreq - 12));
    } else {
      /* Disable all except 16MHz HFOSC */
      AD5940_LPModeCtrlS(LPMODECTRL_HFOSCEN);
      /* wait another 21 clocks. DFT need it to calculte last input data */
      AD5940_SEQGenInsert(SEQ_WAIT(21));
    }
  }
  /* Stage III: Turn off all we can */
  /* Switching back to 16MHz */
  AD5940_LPModeClkS(LPMODECLK_HFOSC);
  /* Disable waveform generator */
  AD5940_AFECtrlS(AFECTRL_ALL, bFALSE);
  lpamp_cfg.LpAmpSel = LPAMP0;
  lpamp_cfg.LpPaPwrEn = bFALSE;
  lpamp_cfg.LpTiaPwrEn = bFALSE;
  lpamp_cfg.LpTiaRf = LPTIARF_OPEN;
  lpamp_cfg.LpTiaRtia = LPTIARTIA_OPEN;
  lpamp_cfg.LpTiaSW = 0;
  AD5940_LPAMPCfgS(&lpamp_cfg);
  AD5940_WriteReg(REG_AFE_LPREFBUFCON,
      BITM_AFE_LPREFBUFCON_LPBUF2P5DIS | BITM_AFE_LPREFBUFCON_LPREFDIS);
  /* Disconnect all switch */
  lpdac_cfg.LpDacSW = 0;
  lpdac_cfg.PowerEn = bFALSE;
  AD5940_LPDACCfgS(&lpdac_cfg);
  /* GP6->endSeq, GP5 -> AD8233=OFF, GP1->RLD=OFF */
  AD5940_SEQGpioCtrlS(0);
#ifndef EXTERNAL_TRIGGER_EDA	
  /* Go to hibernate */
  AD5940_EnterSleepS(); /* we cannot enter ad5940 to sleep state while taking measurements */
  /* Sequence end. */
#endif
#endif // EDA_DCFG_ENABLE

  /* Stop sequencer generator */
  AD5940_SEQGenCtrl(bFALSE);
  error = AD5940_SEQGenFetchSeq(&pSeqCmd, &SeqLen);
  if (error == AD5940ERR_OK) {
    /* Buffer limited */
    if (AppEDACfg.MaxSeqLen < (SeqLen + AppEDACfg.InitSeqInfo.SeqLen))
      return AD5940ERR_BUFF;
    AppEDACfg.MeasureSeqInfo.SeqId = SEQID_0;
    AppEDACfg.MeasureSeqInfo.SeqRamAddr =
        AppEDACfg.InitSeqInfo.SeqRamAddr + AppEDACfg.InitSeqInfo.SeqLen;
    AppEDACfg.MeasureSeqInfo.pSeqCmd = pSeqCmd;
    AppEDACfg.MeasureSeqInfo.SeqLen = SeqLen;
    AppEDACfg.MeasureSeqInfo.WriteSRAM = bTRUE;
    /* Write command to SRAM */
    AD5940_SEQInfoCfg(&AppEDACfg.MeasureSeqInfo);
    /* Record where the patch should be applied. */
    /* The start address in AD5940 SRAM */
    AppEDACfg.SeqPatchInfo.SRAMAddr += AppEDACfg.MeasureSeqInfo.SeqRamAddr;
  } else {
    return error; /* Error */
  }
  return AD5940ERR_OK;
}

fImpCar_Type RtiaCalValue; /* Calibration result - imaginary and real */
/*!
 ****************************************************************************
 * @brief Calibrate LPTIA internal RTIA resistor(s).
 * @details This function will do calibration using
 * parameters stored in @ref AppEDACfg structure.
 * @param None
 * @return AD5940Err: return error code.
 ******************************************************************************/
AD5940Err AppEDARtiaCal(void) {
  LPRTIACal_Type lprtia_cal;
  AD5940_StructInit(&lprtia_cal, sizeof(lprtia_cal));
  /* low power amplifier (LPTIA + potentiostat amplifier) */
  lprtia_cal.LpAmpSel = LPAMP0;
  /* Real + Image */
  lprtia_cal.bPolarResult = bFALSE;
  lprtia_cal.AdcClkFreq = AppEDACfg.AdcClkFreq;
  lprtia_cal.SysClkFreq = AppEDACfg.SysClkFreq;
  lprtia_cal.ADCSinc3Osr = ADCSINC3OSR_4;
  lprtia_cal.ADCSinc2Osr = ADCSINC2OSR_22;
  /* Maximum DFT number */
  lprtia_cal.DftCfg.DftNum = DFTNUM_2048;
  lprtia_cal.DftCfg.DftSrc = DFTSRC_SINC2NOTCH;
  lprtia_cal.DftCfg.HanWinEn = bTRUE;
  lprtia_cal.fFreq = AppEDACfg.SinFreq;
  lprtia_cal.fRcal = AppEDACfg.RcalVal;
  lprtia_cal.bWithCtia = bTRUE;
  lprtia_cal.LpAmpPwrMod = LPAMPPWR_NORM;
  lprtia_cal.bWithCtia = bTRUE;
  lprtia_cal.LpTiaRtia = AppEDACfg.LptiaRtiaSel;

  /* if autoscale enable */
  if (AppEDACfg.RtiaAutoScaleEnable == bTRUE) {
    int i = AppEDACfg.RtiaAutoScaleMin;
    for (; i <= AppEDACfg.RtiaAutoScaleMax; i++) {
      lprtia_cal.LpTiaRtia = i;
      AD5940_LPRtiaCal(&lprtia_cal, &RtiaCalValue);
      AppEDACfg.RtiaCalTable[i] = RtiaCalValue;
    }
    AppEDACfg.RtiaCurrValue = AppEDACfg.RtiaCalTable[AppEDACfg.RtiaIndexCurr];
    rtia_cal_completed = 1;
  } else {
    AD5940_LPRtiaCal(&lprtia_cal, &RtiaCalValue);
    AppEDACfg.RtiaCurrValue = RtiaCalValue;
    rtia_cal_completed = 0;
  }
  return AD5940ERR_OK;
}

/*!
 ****************************************************************************
 * @brief Initialize the EDA measurement.
 * @details This function must be called before start measurement.
 *  It will initialize all needed hardwares and put AD5940 to ready state.
 *  The application parameters stored in @ref AppEDACfg can be changed.
 *  Call this function to re-initialize AD5940 with new parameters.
 * @param pBuffer: the buffer for sequencer generator. Only need to
 *                 provide it for the first time.
 * @param BufferSize: The buffer size start from pBuffer.
 * @return AD5940Err: return error code.
 ******************************************************************************/
AD5940Err AppEDAInit(uint32_t *pBuffer, uint32_t BufferSize) {
  AD5940Err error = AD5940ERR_OK;
  /* sequencer configuration */
  SEQCfg_Type seq_cfg;
  /* FIFO configuration */
  FIFOCfg_Type fifo_cfg;
  ADI_OSAL_PRIORITY default_priority;
  /* State of EDA App, init/voltage mesaurement / current / RTIA Calibration */
  AppEDACfg.EDAStateCurr = EDASTATE_INIT;
  /* Wakeup AFE by read register, read 10 times at most */
  if (AD5940_WakeUp(10) > 10)
    return AD5940ERR_WAKEUP;

#ifndef EDA_DCFG_ENABLE

  /*For now keeping the external trigger code as it is till it gets in working mode */
#ifdef EXTERNAL_TRIGGER_EDA	
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
  tempreg = AD5940_ReadReg(REG_AGPIO_GP0SET);
  tempreg &= 0xFFE7;
  /* set only eda gpio pin as high */
  tempreg |= 0x0008;
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
#endif //EDA_DCFG_ENABLE

#ifdef PROFILE_TIME_ENABLED
  uint32_t eda_rtia_cal_start_time = get_micro_sec();
#endif
  /* Do RTIA calibration */
  /* Do calibration on the first initialization */
  if ((AppEDACfg.ReDoRtiaCal == bTRUE) || AppEDACfg.EDAInited == bFALSE) {
    AppEDACfg.EDAStateCurr = EDASTATE_RTIACAL;
    adi_osal_ThreadGetPrio(NULL, &default_priority);
    adi_osal_ThreadSetPrio(NULL, configMAX_PRIORITIES - 1);
    AppEDARtiaCal();
    adi_osal_ThreadSetPrio(NULL, default_priority);
    AppEDACfg.ReDoRtiaCal = bFALSE;
  }
#ifdef PROFILE_TIME_ENABLED
  eda_rtia_cal_diff_time = get_micro_sec() - eda_rtia_cal_start_time;
#endif
  /* Reconfigure FIFO */
  AD5940_FIFOCtrlS(FIFOSRC_DFT, bFALSE);
  fifo_cfg.FIFOEn = bTRUE;
  fifo_cfg.FIFOMode = FIFOMODE_FIFO;
  /* 4kB for FIFO, The reset 2kB for sequencer */
  fifo_cfg.FIFOSize = FIFOSIZE_4KB;
  fifo_cfg.FIFOSrc = FIFOSRC_DFT;
  /* The first measurment is for excitation voltage. */
  fifo_cfg.FIFOThresh = AppEDACfg.VoltCalPoints * 2;
  AD5940_FIFOCfg(&fifo_cfg);
  AD5940_INTCClrFlag(AFEINTSRC_ALLINT);
  /* Start sequence generator */
  /* Initialize sequencer generator */
  if ((AppEDACfg.EDAInited == bFALSE) || (AppEDACfg.bParaChanged == bTRUE)) {
    if (pBuffer == 0)
      return AD5940ERR_PARA;
    if (BufferSize == 0)
      return AD5940ERR_PARA;
    AD5940_SEQGenInit(pBuffer, BufferSize);
    /* Generate initialize sequence */
    /* Application initialization sequence using either MCU or sequencer */
    error = AppEDASeqCfgGen();
    if (error != AD5940ERR_OK)
      return error;
    /* Generate measurement sequence */
    error = AppEDASeqMeasureGen();
    if (error != AD5940ERR_OK)
      return error;
    AppEDACfg.bParaChanged = bFALSE; /* Clear this flag as we already
                                        implemented the new configuration */
  }
  /* Initialization sequence */
  /* Enable sequencer, run initialization sequence */
  AD5940_SEQCtrlS(bTRUE);
  AD5940_SEQMmrTrig(AppEDACfg.InitSeqInfo.SeqId);
  while (AD5940_INTCTestFlag(AFEINTC_1, AFEINTSRC_ENDSEQ) == bFALSE)
    ;
  /* Apply patch for voltage measurment */
  /* After initialization, the first thing is to measure excitation voltage */
  AppEDACfg.EDAStateCurr = EDASTATE_VOLT;
  /* Init with a value */
  AppEDACfg.RtiaIndexCurr = AppEDACfg.RtiaIndexNext = AppEDACfg.LptiaRtiaSel;
  AppEDACfg.SeqPatchInfo.RtiaSel = LPTIARTIA_OPEN;
  AppEDACfg.SeqPatchInfo.Type = PATCHTYPE_VOLT;
  error = ApPEDASeqPatchGen(&AppEDACfg.SeqPatchInfo);
  if (error != AD5940ERR_OK)
    return error;
  /* Apply the patch to SRAM */
  AD5940_SEQCmdWrite(AppEDACfg.SeqPatchInfo.SRAMAddr,
      AppEDACfg.SeqPatchInfo.pSeqCmd, AppEDACfg.SeqPatchInfo.SeqLen);
  /* Enable sequencer, and wait for trigger */
  AD5940_SEQCtrlS(bTRUE);
  /* Clear interrupt flag generated before */
  AD5940_ClrMCUIntFlag();
  AD5940_AFEPwrBW(AFEPWR_LP, AFEBW_250KHZ);
  /**@todo remove it? close switch SW1  */
  AD5940_WriteReg(REG_AFE_SWMUX, 0x01);
  if (AppEDACfg.RtiaAutoScaleMin > AppEDACfg.RtiaAutoScaleMax) {
    uint32_t temp;
    temp = AppEDACfg.RtiaAutoScaleMin;
    AppEDACfg.RtiaAutoScaleMin = AppEDACfg.RtiaAutoScaleMax;
    AppEDACfg.RtiaAutoScaleMax = temp;
  }
  /* EDA application has been initialized. */
  AppEDACfg.EDAInited = bTRUE;
  return AD5940ERR_OK;
}

/*!
 ****************************************************************************
 *@brief      AD5940 structure initialization
 *@param      None
 *@return     None
 ******************************************************************************/
void AD5940EDAStructInit(void) {
  AppEDACfg_Type *pCfg;

  AppEDAGetCfg(&pCfg);
  pCfg->MaxSeqLen = 512;
  /* Don't do LFOSC calibration now. We assume the default LFOSC is trimmed. */
  pCfg->LfoscClkFreq = 32000;
  /* We manually select resistor value */
  if (!eda_user_applied_rtia_cal) {
    pCfg->RtiaAutoScaleEnable = bTRUE;
    pCfg->RtiaAutoScaleMax = LPTIARTIA_512K;
    pCfg->RtiaAutoScaleMin = LPTIARTIA_100K;
    pCfg->LptiaRtiaSel = LPTIARTIA_100K;
  } else {
    eda_user_applied_rtia_cal = 0;
  }

#ifndef EDA_DCFG_ENABLE
  if (!eda_user_applied_dftnum) {
    /* DFNUM_16 = 2 */
    AppEDACfg.DftNum = DFTNUM_16;
  } else {
    eda_user_applied_dftnum = 0;
  }
  /* if odr is > 16 use dft num = 8 */
  if (eda_user_applied_odr) {
    if (pCfg->EDAODR > 16) {
      /* set dft num to 8 */
      /* DFTNUM_8 = 1 */
      AppEDACfg.DftNum = DFTNUM_8;
    }
  }
#endif
  /* Set excitation voltage to 0.75 times of full range */
  pCfg->SinAmplitude = 1100 * 3 / 4;
  pCfg->SinFreq = 100.0f;
  /* Do not change sample frequency unless you know how it works */
  pCfg->SampleFreq = 400.0f;
  if (!eda_user_applied_odr) {
    /* ODR decides how freuquently to start the engine to measure impedance */
    pCfg->EDAODR = 4.0f;
  } else {
    eda_user_applied_odr = 0;
  }
  /* The minimum threshold value is 4, and should always be 4*N, where N is
   * 1,2,3... */
  pCfg->FifoThresh = 4;
}

/*!
 ****************************************************************************
 *@brief      Initialize AD5940 basic blocks like clock. It will first reset
 *             AD5940 using reset pin
 *@param      None
 *@return     AD5940Err
 ******************************************************************************/
static AD5940Err AD5940PlatformCfg(void) {
  CLKCfg_Type clk_cfg;
  FIFOCfg_Type fifo_cfg;
  SEQCfg_Type seq_cfg;
  AGPIOCfg_Type gpio_cfg;
  LFOSCMeasure_Type LfoscMeasure;
#ifdef EXTERNAL_TRIGGER_EDA	
  /* enable external trigger within ad5940 */
  SeqGpioTrig_Cfg cfg;
#endif
  /* Use hardware reset */
  AD5940_HWReset();
  /* Platform configuration */
  AD5940_Initialize();
#ifdef EDA_DCFG_ENABLE
  /* load dcfg here for test */
  write_ad5940_init();
#else
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
  /* DFT result. One pair for RCAL, another for Rz. One DFT result have
   * real part and imaginary part */
  fifo_cfg.FIFOThresh = 2;
  /* Disable to reset FIFO */
  AD5940_FIFOCfg(&fifo_cfg);
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
  /* Enable all interrupt in Interrupt Controller 1, so we can check INTC flags
   */
  AD5940_INTCCfg(AFEINTC_1, AFEINTSRC_ALLINT, bTRUE);
  /* Interrupt Controller 0 will control GP0 to generate interrupt to MCU */
  AD5940_INTCCfg(AFEINTC_0, AFEINTSRC_DATAFIFOTHRESH, bTRUE);
  AD5940_INTCClrFlag(AFEINTSRC_ALLINT);
#ifdef EXTERNAL_TRIGGER_EDA	
 /* Step4: Reconfigure GPIO */
  gpio_cfg.FuncSet = GP0_TRIG | GP6_SYNC | GP5_SYNC | GP4_SYNC | GP2_EXTCLK | GP1_SYNC ;
  gpio_cfg.InputEnSet = AGPIO_Pin0;
  gpio_cfg.OutputEnSet =  AGPIO_Pin1 | AGPIO_Pin4 | AGPIO_Pin5 | AGPIO_Pin6;
  gpio_cfg.OutVal = 0;
  gpio_cfg.PullEnSet = AGPIO_Pin0;
  AD5940_AGPIOCfg(&gpio_cfg);

  cfg.bEnable = bTRUE;
  cfg.PinSel = AGPIO_Pin0;
  cfg.SeqPinTrigMode = SEQPINTRIGMODE_FALLING;
  AD5940_SEQGpioTrigCfg(&cfg);
#else
  /*Adding electrode switch on measurement sequence only using GPIO3*/
  /* Step4: Configure GPIO */
  gpio_cfg.FuncSet =  GP6_SYNC | GP5_SYNC | GP4_SYNC | GP3_SYNC | GP2_EXTCLK | GP1_SYNC | GP0_INT;
  gpio_cfg.InputEnSet = AGPIO_Pin2;
  gpio_cfg.OutputEnSet =  AGPIO_Pin0 | AGPIO_Pin1 | AGPIO_Pin3 | AGPIO_Pin4 | AGPIO_Pin5 | AGPIO_Pin6;
  gpio_cfg.OutVal = 0;
  gpio_cfg.PullEnSet = 0;
  AD5940_AGPIOCfg(&gpio_cfg);
#endif


  /* Enable AFE to enter sleep mode. */
  AD5940_SleepKeyCtrlS(SLPKEY_UNLOCK);
#endif //EDA_DCFG_ENABLE

  /* Measure LFOSC frequency */
  /**@note Calibrate LFOSC using system clock. The system clock accuracy
   * decides measurment accuracy. Use XTAL to get better result. */
  /* 1000ms used for calibration. */
  LfoscMeasure.CalDuration = 1000.0;
  /* Put sequence commands from start address of SRAM */
  LfoscMeasure.CalSeqAddr = 0;
  /* 16MHz in this firmware */
  LfoscMeasure.SystemClkFreq = 16000000.0f;
  AD5940_LFOSCMeasure(&LfoscMeasure, &LFOSCFreq);
  return AD5940ERR_OK;
}

/*!
  ****************************************************************************
  *@brief      Eda application modify registers when AFE wakeup. It is called
               in ISR when AFE has been wakeup and we can access registers
  *@param      pData: the buffer points to data read back from FIFO
  *@param      pDataCount: The data count in pData buffer
  *@return     AD5940Err
  ******************************************************************************/
AD5940Err AppEDARegModify(int32_t *const pData, uint32_t *pDataCount) {
  AD5940Err error = AD5940ERR_OK;
  if (AppEDACfg.EDAStateCurr == EDASTATE_VOLT) {
    SWMatrixCfg_Type sw_cfg;
    /* Next step is to measure current */
    AppEDACfg.EDAStateNext = EDASTATE_CURR;
    /* Need change some registers in order to measure current */
    /* Stop it for now. */
    AD5940_SEQCtrlS(bFALSE);
    /* Disable FIFO firstly because we are going to change FIFO threshold */
    AD5940_FIFOCtrlS(FIFOSRC_DFT, bFALSE);
    AD5940_FIFOThrshSet(AppEDACfg.FifoThresh);
    /* Enable FIFO */
    AD5940_FIFOCtrlS(FIFOSRC_DFT, bTRUE);
    /* Change Switch matrix settings to connect AIN2(body) to SE0 */
    /* Open all switch D */
    sw_cfg.Dswitch = SWD_OPEN;
    sw_cfg.Pswitch = SWP_AIN2 | SWP_SE0;
    sw_cfg.Nswitch = SWN_OPEN;
    /* This switch is for ECG. */
    sw_cfg.Tswitch = SWT_AIN0 | SWT_AFE3LOAD;
    AD5940_SWMatrixCfgS(&sw_cfg);
    /* Apply patch for current measurment */
    AppEDACfg.SeqPatchInfo.Type = PATCHTYPE_CURR;
    AppEDACfg.SeqPatchInfo.RtiaSel = AppEDACfg.RtiaIndexNext;
    error = ApPEDASeqPatchGen(&AppEDACfg.SeqPatchInfo);
    if (error != AD5940ERR_OK)
      return error;
    /* Apply the patch to SRAM */
    AD5940_SEQCmdWrite(AppEDACfg.SeqPatchInfo.SRAMAddr,
        AppEDACfg.SeqPatchInfo.pSeqCmd, AppEDACfg.SeqPatchInfo.SeqLen);
    /* Enable sequencer. Sequencer will run when next valid trigger comes */
    AD5940_SEQCtrlS(bTRUE);
  } else if (AppEDACfg.EDAStateCurr == EDASTATE_CURR) {
    if (AppEDACfg.bChangeRtia == bTRUE) {
      AppEDACfg.bChangeRtia = bFALSE;
      /* Apply patch for next RTIA selection */
      AppEDACfg.SeqPatchInfo.Type = PATCHTYPE_CURR;
      AppEDACfg.SeqPatchInfo.RtiaSel = AppEDACfg.RtiaIndexNext;
      error = ApPEDASeqPatchGen(&AppEDACfg.SeqPatchInfo);
      if (error != AD5940ERR_OK)
        return error;
      /* Apply the patch to SRAM */
      AD5940_SEQCmdWrite(AppEDACfg.SeqPatchInfo.SRAMAddr,
          AppEDACfg.SeqPatchInfo.pSeqCmd, AppEDACfg.SeqPatchInfo.SeqLen);
    }
    measurement_cycle_completed = 1;
  }

  if (AppEDACfg.bMeasVoltReq == bTRUE) {
    SWMatrixCfg_Type sw_cfg;
    /* Clear this request */
    AppEDACfg.bMeasVoltReq = bFALSE;
    /* Next step is to measure voltage */
    AppEDACfg.EDAStateNext = EDASTATE_VOLT;
    /* Change Switch matrix settings to connect AIN2(body) to SE0 */
    /* Open all switch D */
    sw_cfg.Dswitch = SWD_OPEN;
    sw_cfg.Pswitch = SWP_OPEN;
    sw_cfg.Nswitch = SWN_OPEN;
    /* This switch is for ECG */
    sw_cfg.Tswitch = SWT_AIN0 | SWT_AFE3LOAD;
    AD5940_SWMatrixCfgS(&sw_cfg);
    /* Need change some registers in order to measure current */
    /* Stop it for now. */
    AD5940_SEQCtrlS(bFALSE);
    /* Disable FIFO first because we are going to change FIFO threshold */
    AD5940_FIFOCtrlS(FIFOSRC_DFT, bFALSE);
    AD5940_FIFOThrshSet(AppEDACfg.VoltCalPoints * 2);
    /* Enable FIFO*/
    AD5940_FIFOCtrlS(FIFOSRC_DFT, bTRUE);
    /* Apply patch for current measurment */
    AppEDACfg.SeqPatchInfo.Type = PATCHTYPE_VOLT;
    AppEDACfg.SeqPatchInfo.RtiaSel = LPTIARTIA_OPEN;
    error = ApPEDASeqPatchGen(&AppEDACfg.SeqPatchInfo);
    if (error != AD5940ERR_OK)
      return error;
    /* Apply the patch to SRAM */
    AD5940_SEQCmdWrite(AppEDACfg.SeqPatchInfo.SRAMAddr,
        AppEDACfg.SeqPatchInfo.pSeqCmd, AppEDACfg.SeqPatchInfo.SeqLen);
    /* Enable sequencer. Sequencer will run when next valid trigger comes */
    AD5940_SEQCtrlS(bTRUE);
  }
  if (AppEDACfg.NumOfData > 0) {
    AppEDACfg.FifoDataCount += *pDataCount / 4;
    if (AppEDACfg.FifoDataCount >= AppEDACfg.NumOfData) {
      AD5940_WUPTCtrl(bFALSE);
      return AD5940ERR_OK;
    }
  }
  if (AppEDACfg.StopRequired == bTRUE) {
    AD5940_WUPTCtrl(bFALSE);
    AppEDACfg.StopRequired = bFALSE;
    AppEDACfg.bRunning = bFALSE;
    return AD5940ERR_OK;
  }
  return AD5940ERR_OK;
}

/*!
 ****************************************************************************
 * @brief Depending on the data type, do appropriate data pre-process
 *       before return back to controller
 * @param pImpedance: the buffer points to pre-processed data.
 * The impedance magnitude value is used to decide new RTIA settings.
 * @param uiDataCount: The data count in pData buffer.
 * @return uint32_t: return the next appropriate RTIA index value.
 ******************************************************************************/
uint32_t EDARtiaAutoScaling(
    fImpCar_Type *const pImpedance, uint32_t uiDataCount) {
  uint32_t OptRtiaIndex;
  float MagMean = 0;
  fImpCar_Type SumImp = {0, 0};
  /* Get Averaged Magnitude Result */
  for (int i = 0; i < uiDataCount; i++) {
    SumImp.Real += pImpedance[i].Real;
    SumImp.Image += pImpedance[i].Image;
  }
  SumImp.Real /= uiDataCount;
  SumImp.Image /= uiDataCount;
  /* Impedance under test is sum of changed value and baseline */
  SumImp = AD5940_ComplexAddFloat(&SumImp, &AppEDACfg.ImpEDABase);
  MagMean = AD5940_ComplexMag(&SumImp);
  OptRtiaIndex = AppEDACfg.RtiaAutoScaleMin;
  /* This is much easier because although the RTIA is not the best value,
   * the results are still reliable. We can directly choose the RTIA matched */
  for (; OptRtiaIndex < AppEDACfg.RtiaAutoScaleMax;) {
    float mag;
    mag = AD5940_ComplexMag(&AppEDACfg.RtiaCalTable[OptRtiaIndex + 1]);
    if (MagMean < mag * 0.97f)
      break;
    OptRtiaIndex++;
  }
  return OptRtiaIndex;
}

int32_t SumReal = 0, SumImage = 0;
/*!
 ****************************************************************************
 * @brief Data pre-process
 * @details Depending on the data type, do appropriate data pre-process
 *    before return back to controller
 * @param pData: the buffer points to data read back from FIFO.
 * @param pDataCount: The data count in pData buffer.
 * @return AD5940Err: return error code.
 ******************************************************************************/
AD5940Err AppEDADataProcess(int32_t *const pData, uint32_t *pDataCount) {
#ifdef PROFILE_TIME_ENABLED
  delay_in_first_measurement =
      abs(get_micro_sec() - eda_init_start_time) / 1000000;
#endif
  uint32_t DataCount = *pDataCount;
  *pDataCount = 0;
  /* EDA results are DFT results */
  for (uint32_t i = 0; i < DataCount; i++) {
    pData[i] &= 0x3ffff;
    /* Bit17 is sign bit */
    /* Data is 18bit in two's complement, bit17 is the sign bit */
    if (pData[i] & (1 << 17))
      pData[i] |= 0xfffc0000;
  }
  if (AppEDACfg.EDAStateCurr == EDASTATE_VOLT) {
    uint32_t DftResCnt;
    iImpCar_Type *pDftRes = (iImpCar_Type *)pData;
    SumReal = 0, SumImage = 0;
    /* Get average excitation voltage */
    if (DataCount != AppEDACfg.VoltCalPoints * 2)
      return EDAERR_VOLTMEASURE;
    DftResCnt = DataCount / 2;
    /* Discard the first 4 results */
    if (DftResCnt > 4) {
      DftResCnt -= 4;
      pDftRes += 4;
    }
    for (uint32_t i = 0; i < DftResCnt; i++) {
      SumReal += pDftRes[i].Real;
      SumImage += pDftRes[i].Image;
    }
    SumReal /= (int32_t)DftResCnt;
    /* Get average result */
    SumImage /= (int32_t)DftResCnt;
    /* Fix sign of imaginary part of DFT result. */
    SumImage = -SumImage;
    AppEDACfg.ExcitVolt.Real = SumReal;
    AppEDACfg.ExcitVolt.Image = SumImage;
    /* Done */
    /* Don't return voltage result */
    *pDataCount = 0;
#ifdef PROFILE_TIME_ENABLED
    voltage_measurement_diff_time =
        abs(get_sensor_time_stamp() - gnAd5940TimeCurVal) / 1000;
    voltage_cycles_diff_time =
        abs(get_micro_sec() - gnAd5940TimeCurValInMicroSec);
#endif
    /* The FIFO data is current result. We need to calculate impedance, Z=V/I */
  } else if (AppEDACfg.EDAStateCurr == EDASTATE_CURR) {
    iImpCar_Type *const pSrc = (iImpCar_Type *)pData;
    fImpCar_Type *const pOut = (fImpCar_Type *)pData;
    for (uint32_t i = 0; i < DataCount / 2; i++) {
      fImpCar_Type DftCurr;
      fImpCar_Type res;
      DftCurr.Real = (float)pSrc[i].Real;
      DftCurr.Image = (float)pSrc[i].Image;
      DftCurr.Image = -DftCurr.Image;
      DftCurr.Real = -DftCurr.Real;
      DftCurr.Image = -DftCurr.Image;
      /* I=Vrtia/Zrtia */
      res = AD5940_ComplexDivFloat(&DftCurr, &AppEDACfg.RtiaCurrValue);
      res = AD5940_ComplexDivFloat(&AppEDACfg.ExcitVolt, &res);
      AppEDACfg.ImpSum = AD5940_ComplexAddFloat(&AppEDACfg.ImpSum, &res);
      AppEDACfg.ImpSumCount++;
      res = AD5940_ComplexSubFloat(&res, &AppEDACfg.ImpEDABase);
      pOut[i] = res;
#ifdef PROFILE_TIME_ENABLED
      current_measurement_diff_time =
          abs(get_sensor_time_stamp() - gnAd5940TimeCurVal) / 1000;
      current_cycles_diff_time =
          abs(get_micro_sec() - gnAd5940TimeCurValInMicroSec);
#endif
    }
    /* Impedance result */
    *pDataCount = DataCount / 2;
    /* Process RTIA autoscaling calculation */
    if (AppEDACfg.RtiaAutoScaleEnable) {
      /* Init to invalid value */
      static uint32_t rtia_pre = (uint32_t)-1;
      uint32_t rtia_index;
      AppEDACfg.RtiaIndexCurr = AppEDACfg.RtiaIndexNext;
      AppEDACfg.RtiaCurrValue = AppEDACfg.RtiaCalTable[AppEDACfg.RtiaIndexCurr];
      rtia_index = EDARtiaAutoScaling(pOut, *pDataCount);
      if (rtia_index != rtia_pre) {
        AppEDACfg.bChangeRtia = bTRUE;
        rtia_pre = rtia_index;
        AppEDACfg.RtiaIndexNext = rtia_index;
      }
    }
  }
  AppEDACfg.EDAStateCurr = AppEDACfg.EDAStateNext;
  return AD5940ERR_OK;
}

#ifdef EDA_DCFG_ENABLE
extern uint64_t default_dcfg_eda[];
#endif
/*!
 ****************************************************************************
 *@brief      AD5940 EDA App initialization
 *@param      None
 *@return     None
 ******************************************************************************/
void ad5940_eda_start(void)
{
#ifndef EXTERNAL_TRIGGER_EDA	
  /* configure call back */
  Ad5940DrvDataReadyCallback(Ad5940FifoCallBack);
   
   /* Interrupts setting */
  ad5940_port_Init();

#endif
/* switch off other switches */
/*
#ifdef ENABLE_ECG_APP
  DG2502_SW_control_AD8233(false);
#endif
  DG2502_SW_control_ADPD4000(false);
*/

#ifdef EDA_DCFG_ENABLE
      if(eda_load_applied_dcfg == 1) {
        /*Have to use it at properly,both rtia and eda calls this if we keep here*/
        load_ad5940_default_config(&default_dcfg_eda[0]);
       }
#endif//EDA_DCFG_ENABLE

if(init_flag == 1){
  InitCfg();
}
  ClearDataBufferAd5940();

  AD5940PlatformCfg();

   /* Configure your parameters in this function */
  AD5940EDAStructInit();
  AppEDAInit(AppBuff, APPBUFF_SIZE);    /* Initialize EDA application. Provide a buffer, which is used to store sequencer commands */
#ifdef EXTERNAL_TRIGGER_EDA	  
  enable_ad5940_ext_trigger(AppEDACfg.EDAODR);
#endif
}

#endif