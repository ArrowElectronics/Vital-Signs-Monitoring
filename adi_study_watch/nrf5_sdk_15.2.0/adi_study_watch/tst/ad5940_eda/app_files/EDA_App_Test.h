/**
****************************************************************************
* @file     EDA_App_Test.h
* @author   ADI
* @version  V0.1
* @date     04-Dec-2019
* @brief    This header file is used as a test application for eda
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

#ifndef _EDA_H_
#define _EDA_H_
#include "ad5940.h"
#include "stdio.h"
#include "string.h"
#include "math.h"

/* Do not modify following parameters */
#define LPTIAPA_PMOD    0                 /* Power Mode of PA and LPTIA, Set to Half Power Mode is better for power consumption, 0: normal. 0x18: boost power. BITM_AFE_ULPTIACON0_HALFPWR: half power */
#define LPF_RF          LPTIARF_20K       /* Set RF resistor of Low Pass Filter */
#define LPF_TIME        10.0              /* Unit is ms. Low Pass Filter need time to settle. 10ms is OK for now */

#define LPTIASW_VOLT    LPTIASW(5)|LPTIASW(6)|LPTIASW(7)|LPTIASW(8)|LPTIASW(9)|LPTIASW(13)
#define LPTIASW_CURR    LPTIASW(2)|LPTIASW(5)|LPTIASW(10)|LPTIASW(13)

/**
 * @brief The structure for sequencer patch.
*/
typedef struct
{
  enum __PatchType
  {
    PATCHTYPE_VOLT = 0,         /**< Generate patch for measuring voltage */
    PATCHTYPE_CURR,             /**< Generate patch for measuring current of body */
  }Type;                          
  uint32_t RtiaSel;             /**< LPTIA RTIA selection */
  const uint32_t *pSeqCmd;      /**< The sequence to measure voltage and current is similar. The difference is stored in a command patch. */
  uint32_t SeqLen;              /**< Length of patch sequence  */
  uint32_t SRAMAddr;            /**< Start address of the sequence command patch  */
  uint32_t Buffer[32];          /**< 32Byte should be enough for sequence generator */
  uint32_t BuffLen;       /**< The buffer length of Buffer */
}SeqPatchInfo_Type;

/* 
  Note: this example will use SEQID_0 as measurment sequence, and use SEQID_1 as init sequence. 
  SEQID_3 is used for calibration if there is need.
*/
typedef struct
{
/* Common configurations for all kinds of Application. */
  BoolFlag bParaChanged;        /**< Indicate to generate sequence again. It's auto cleared by AppEDAInit */
  uint32_t SeqStartAddr;        /**< Initialaztion sequence start address in SRAM of AD5940  */
  uint32_t MaxSeqLen;           /**< Limit the maximum sequence.   */
  uint32_t SeqStartAddrCal;     /**< Measurment sequence start address in SRAM of AD5940 */
  uint32_t MaxSeqLenCal;
/* Application related parameters */ 
  BoolFlag bBioElecBoard;       /**< Select between AD5941Sens1 board and BioElec board */
  BoolFlag ReDoRtiaCal;         /**< Set this flag to bTRUE when there is need to do calibration. */
  float SysClkFreq;             /**< The real frequency of system clock */
  float LfoscClkFreq;           /**< The clock frequency of Wakeup Timer in Hz. Typically it's 32kHz. Leave it here in case we calibrate clock in software method */
  float AdcClkFreq;             /**< The real frequency of ADC clock */
  uint32_t FifoThresh;          /**< FIFO threshold. Should be N*4 */   
  float EDAODR;                 /**< in Hz. ODR decides the period of WakeupTimer who will trigger sequencer periodically. DFT number and sample frequency decides the maxim ODR. */
  int32_t NumOfData;            /**< By default it's '-1'. If you want the engine stops after get NumofData, then set the value here. Otherwise, set it to '-1' which means never stop. */
  uint32_t VoltCalPoints;       /**< Use how many points to calculate average excitation voltage */
  float RcalVal;                /**< Rcal value in Ohm */
  float SinFreq;                /**< Frequency of excitation signal */
  float SampleFreq;             /**< Sample Frequency in Hz. Clock source is 32kHz.*/  
  float SinAmplitude;           /**< Signal in amplitude in mV unit. Range: 0Vp to 1100mVp (0Vpp to 2.2Vpp) */
  uint32_t DacUpdateRate;       /**< DAC update rate is SystemCLoock/Divider. The available value is 7 to 255. */
  uint32_t LptiaRtiaSel;        /**< Use internal RTIA, Select from LPTIARTIA_OPEN, LPTIARTIA_200R, ... , LPTIARTIA_512K */
  
  uint32_t DftNum;              /**< DFT number */
  BoolFlag HanWinEn;            /**< Enable Hanning window */
  
  BoolFlag RtiaAutoScaleEnable; /**< Automatically change RTIA value according to measurement results. 0: Set RTIA with RTIA_SEL. 1: Automatically choose RTIA in software */
  uint32_t RtiaAutoScaleMax;    /**< Limit the maximum RTIA value that auto scale function can use. Select from LPTIARTIA_OPEN, LPTIARTIA_200R, ... , LPTIARTIA_512K */
  uint32_t RtiaAutoScaleMin;    /**< Limit the minimum RTIA value that auto scale function can use. Select from LPTIARTIA_OPEN, LPTIARTIA_200R, ... , LPTIARTIA_512K */

/* Private variables for internal usage */
  fImpCar_Type  RtiaCurrValue;   /**< Calibrated Rtia value of current frequency */
  fImpCar_Type  RtiaCalTable[LPTIARTIA_512K+1];   /**< Calibrated Rtia Value table */
  fImpCar_Type  ImpEDABase;      /**< Impedance of EDA base line */
  fImpCar_Type  ImpSum;          /**< Sum of all measured results. Used to calculate base line of EDA */
  uint32_t      ImpSumCount;     /**< Count of data added to 'ImpSum' */ 
  uint32_t      RtiaIndexCurr;   /**< Index value 0 to 26 means Open, 200Ohm, to 512kOhm */
  uint32_t      RtiaIndexNext;
  BoolFlag      bChangeRtia;     /**< Auto scaling method says we need to change RTIA */
  
  SeqPatchInfo_Type SeqPatchInfo; /**< The sequence patch for different RTIA and both voltage/current measurement */
  fImpCar_Type ExcitVolt;       /**< Measured excitation voltage result */
  BoolFlag bDataIsVolt;         /**< Current DFT result is voltage */
  BoolFlag bMeasVoltReq;        /**< User says we need to measure voltage */

  BoolFlag EDAInited;           /**< If the program run firstly, generated sequence commands */
  SEQInfo_Type InitSeqInfo;
  SEQInfo_Type MeasureSeqInfo;
  BoolFlag StopRequired;        /**< After FIFO is ready, stop the measurment sequence */
  uint32_t FifoDataCount;       /**< Count how many times impedance have been measured */

  enum __EDAState{
    EDASTATE_INIT = 0,          /**< Initializing */
    EDASTATE_RTIACAL,           /**< Internal RTIA resistor calibrating. */
    EDASTATE_VOLT,              /**< Measuring excitation voltage */
    EDASTATE_CURR,              /**< Measuring respond current */
  }EDAStateCurr, EDAStateNext;  /**< When interrupt happens, the state is EDACurrState. At the end of interrupt function, go to EDANextState */
/* End */
}AppEDACfg_Type;

/* Common application control message */
#define APPCTRL_START          0      /**< Start the measurement by starting Wakeup Timer */
#define APPCTRL_STOPNOW        1      /**< Stop immediately by stop Wakeup Timer*/
#define APPCTRL_STOPSYNC       2      /**< Stop the measurement when interrupt occured */
#define APPCTRL_SHUTDOWN       3      /**< Note: shutdown here means turn off everything and put AFE to hibernate mode. The word 'SHUT DOWN' is only used here. */

#define EDACTRL_MEASVOLT       100    /**< Measure Exciation voltage now */
#define EDACTRL_GETRTIAMAG     101    /**< Get the rtia magnitude for current measured data */

#define EDACTRL_RSTBASE        102    /**< Reset base line of EDA result. */
#define EDACTRL_SETBASE        103    /**< Set base line of EDA result */
#define EDACTRL_GETAVR         104    /**< Get average value of all measured impedance */

/* Error message */
#define EDAERR_ERROR            AD5940ERR_APPERROR    /**< General error */
#define EDAERR_VOLTMEASURE      AD5940ERR_APPERROR-1  /**< Excitation voltage measurment error. Points not match */

AD5940Err AppEDAGetCfg(void *pCfg);
AD5940Err AppEDAInit(uint32_t *pBuffer, uint32_t BufferSize);
AD5940Err AppEDAISR(void *pBuff, uint32_t *pCount);
AD5940Err AppEDACtrl(int32_t EDACtrl, void *pPara);
AD5940Err AppEDARtiaCal(void);
void AD5940EDAStructInit(void);
AD5940Err AppEDAInit(uint32_t *pBuffer, uint32_t BufferSize);
void Eda_App_init();
void eda_measurements();
AD5940Err EDAShowResult(void *pData, uint32_t DataCount);
void AD5940_Main_EDA(void);
void InitCfg();
#endif
