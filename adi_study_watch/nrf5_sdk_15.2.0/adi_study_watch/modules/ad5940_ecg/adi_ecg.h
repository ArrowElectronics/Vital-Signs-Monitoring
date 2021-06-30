/**
****************************************************************************
* @file     adi_ecg.h
* @author   ADI
* @version  V0.1
* @date     04-Dec-2019
* @brief    ECG Measurement using AD5940.
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
#ifndef _ELETROCARDIOAGRAPH_H_
#define _ELETROCARDIOAGRAPH_H_
#include "ad5940.h"
#include <stdio.h>
#include "string.h"
#include "math.h"

typedef struct
{
/* Common configurations for all kinds of Application. */
  BoolFlag bParaChanged;        /* Indicate to generate sequence again. It's auto cleared by AppBIAInit */
  BoolFlag bBioElecBoard;       /* Indicate if the board is Bioelec board. 0: AD5941Sens1 board, 1: AD5940-BioElec */
  uint32_t SeqStartAddr;        /* Initialaztion sequence start address in SRAM of AD5940  */
  uint32_t MaxSeqLen;           /* Limit the maximum sequence.   */
  uint32_t SeqStartAddrCal;     /* Calibration sequence start address in SRAM of AD5940 */
  uint32_t MaxSeqLenCal;
/* Application related parameters */ 
  float ECGODR;                 /* Must be less than 1500Hz. Sample frequency in Hz, this value is used to set Sleep Wakeup Timer period */
  int32_t NumOfData;            /* By default it's '-1'. If you want the engine stops after get NumofData, then set the value here. Otherwise, set it to '-1' which means never stop. */
  uint32_t FifoThresh;          /* FIFO threshold. Should be N*4 */

  float LfoscClkFreq;           /* The clock frequency of internal LFOSC in Hz. Typically it's 32kHz. Leave it here in case we calibrate clock in software method */
  float SysClkFreq;             /* The real frequency of system clock */
  float AdcClkFreq;             /* The real frequency of ADC clock */
  uint32_t PwrMod;              /* Control Chip power mode(LP/HP) */

  uint32_t AdcPgaGain;          /* PGA Gain select from GNPGA_1, GNPGA_1_5, GNPGA_2, GNPGA_4, GNPGA_9 !!! We must ensure signal is in range of +-1.5V which is limited by ADC input stage */   
  uint32_t ADCSinc3Osr;
  uint32_t ADCSinc2Osr; 
  BoolFlag bRunning;            /**< status of if app is running. Useful when send STOP_SYNC to detect if it's actually stopped. */ 
/* Private variables for internal usage */
  BoolFlag ECGInited;           /* If the program run firstly, generated sequence commands */
  BoolFlag StopRequired;        /* After FIFO is ready, stop the measurement sequence */
  uint32_t FifoDataCount;       /* How many data we have got from start. */
  SEQInfo_Type InitSeqInfo;
  SEQInfo_Type MeasureSeqInfo;
  uint16_t PacketizationEn;
}AppECGCfg_Type;
/* Common application control message */
#define APPCTRL_START          0      /**< Start the measurement by starting Wakeup Timer */
#define APPCTRL_STOPNOW        1      /**< Stop immediately by stop Wakeup Timer*/
#define APPCTRL_STOPSYNC       2      /**< Stop the measurement when interrupt occured */
#define APPCTRL_SHUTDOWN       3      /**< Note: shutdown here means turn off everything and put AFE to hibernate mode. The word 'SHUT DOWN' is only used here. */
#define APPCTRL_RUNNING        4      /**< Is application running? */

AD5940Err AppECGGetCfg(void *pCfg);
AD5940Err AppECGInit(uint32_t *pBuffer, uint32_t BufferSize);
AD5940Err AppECGISR(void *pBuff, uint32_t *pCount);
AD5940Err AppECGCtrl(int32_t Command, void *pPara);
AD5940Err AppECGDataProcess(int32_t * const pData, uint32_t *pDataCount);
#endif
