/**
****************************************************************************
* @file     clock_calibration.c
* @author   ADI
* @version  V0.1
* @date     04-Dec-2019
* @brief    This source file is used to calibrate 32M and 1M Clock.
*           This will tune the register to reduce the deviation to a minimum.
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

/* ------------------------- Includes -------------------------------------- */
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "adpd400x_drv.h"
#include "adpd400x_reg.h"
#include "adi_adpd_ssm.h"
#include "clock_calibration.h"
#include <hw_if_config.h>
#include <Common.h>

/** ------------------------- Private Variables ----------------------------- */
#ifdef CAL_BASED_ON_TIMESTAMP_REGISTER
static struct RegisterCl {
  uint16_t x0c;
  uint16_t x22;
  uint16_t x26;
}RegCl;
#else
/*! \struct AdiAdpd400xReg ""
    Backup register structure for clock calibration */
static struct RegisterAdpd400x {
  uint16_t x06;          /*!< Clock selection register */                  
  uint16_t x0D;          /*!< System Core Frequency register */
  uint16_t x0E;          /*!< System Core Frequency register*/
  uint16_t x0F;          /*!< Time Slot selection and Operation mode register */
  uint16_t x10;          /*!< Fifo status byte register */
  uint16_t x1E;          /*!< Gpio configuration register */
  uint16_t x22;          /*!< Gpio configuration register */
  uint16_t x23;          /*!< Gpio1 configuration register */
  uint16_t x0100;        /*!< Time Slot Control register */
  uint16_t x0110;        /*!< Time Slot Data Size register */ 
  uint16_t x0111;        /*!< Time Slot Lit Data Size register */
  uint16_t x0112;        /*!< Time Slot Decimation register */
  uint16_t highslot;
  uint16_t slotformat;
  uint16_t channelnum;
}RegAdpd400x;
#endif //CAL_BASED_ON_TIMESTAMP_REGISTER
/** ------------------------- Private Variables ----------------------------- */
static __IO uint8_t gsIRQFlag = 0;     /*!< IRQ flag variable */
static uint16_t gsFifoWaterMark;
/*! \enum CALIBRATION_STATUS
    Clock calibration error status */
typedef enum CALIBRATION_STATUS_t {
  CALIBRATION_FATAL_ERROR = -2,        /*!< Calibration fatal error status */
  CALIBRATION_FAILED = -1,             /*!< Calibration error status */
  CALIBRATION_SUCCESS = 0              /*!< Calibration success status */
}CALIBRATION_STATUS;

extern uint8_t dvt2; /*!< To check whether dvt2 board version or not */
/*!****************************************************************************
*
*  \brief       To set the IRQ flag for data ready
*
*  \return      None
*****************************************************************************/
void ADPDLib_DataReady(void) {
  gsIRQFlag = 1;
}

#ifndef CAL_BASED_ON_TIMESTAMP_REGISTER
/*!****************************************************************************
*
*  \brief       Initialize the clock calibration
*
*  \return      None
  *****************************************************************************/
static void Adpd400xClockCalibrationInit(void) {
  uint16_t temp;
  // Read the values from sensor device before doing calibration
  //adi_adpddrv_RegRead(ADPD400x_REG_FIFO_CTL,   &RegAdpd400x.x06);
  adi_adpddrv_RegRead(ADPD400x_REG_TS_FREQ,    &RegAdpd400x.x0D);
  adi_adpddrv_RegRead(ADPD400x_REG_TS_FREQH,   &RegAdpd400x.x0E);
  adi_adpddrv_RegRead(ADPD400x_REG_SYS_CTL,    &RegAdpd400x.x0F);
  adi_adpddrv_RegRead(ADPD400x_REG_OPMODE,     &RegAdpd400x.x10);
  adi_adpddrv_RegRead(ADPD400x_REG_FIFO_STATUS_BYTES, &RegAdpd400x.x1E);
  adi_adpddrv_RegRead(ADPD400x_REG_GPIO_CFG,    &RegAdpd400x.x22);
  adi_adpddrv_RegRead(ADPD400x_REG_GPIO01,     &RegAdpd400x.x23);
  adi_adpddrv_RegRead(ADPD400x_REG_TS_CTRL_A, &RegAdpd400x.x0100);
  adi_adpddrv_RegRead(ADPD400x_REG_DATA1_A,  &RegAdpd400x.x0110);
  adi_adpddrv_RegRead(ADPD400x_REG_DATA2_A,  &RegAdpd400x.x0111);
  adi_adpddrv_RegRead(ADPD400x_REG_DECIMATE_A, &RegAdpd400x.x0112);

  adi_adpdssm_GetParameter(ADPD400x_WATERMARKING, 0, &gsFifoWaterMark);

  adi_adpdssm_GetParameter(ADPD400x_HIGHEST_SLOT_NUM, 0,&RegAdpd400x.highslot);

  gsIRQFlag = 0;
  adi_adpddrv_DataReadyCallback(ADPDLib_DataReady);

  adi_adpdssm_GetParameter(ADPD400x_LATEST_SLOT_DATASIZE, RegAdpd400x.highslot, \
                                                &RegAdpd400x.slotformat);

  adi_adpdssm_GetParameter(ADPD400x_THIS_SLOT_CHANNEL_NUM, RegAdpd400x.highslot,\
      &RegAdpd400x.channelnum);

  adi_adpdssm_slotSetup(0, 1, 0x0002, 1);
  adi_adpddrv_RegWrite(ADPD400x_REG_OPMODE, 0x0000);     // Enable Slot-A alone
  adi_adpddrv_RegWrite(ADPD400x_REG_TS_FREQ, 0x09c4);    // Set freq. 400Hz
  adi_adpddrv_RegWrite(ADPD400x_REG_TS_FREQH, 0x0000);   // Set freq. 400Hz
  adi_adpddrv_RegWrite(ADPD400x_REG_DECIMATE_A, 0x0000); // Decimation as 1
  adi_adpddrv_RegWrite(ADPD400x_REG_TS_CTRL_A, 0x0000);  // Disable CH2
  // Disable Optional bytes
  adi_adpddrv_RegWrite(ADPD400x_REG_FIFO_STATUS_BYTES, 0x0000);
  adi_adpddrv_RegWrite(ADPD400x_REG_GPIO_CFG, 0x0003);
  adi_adpddrv_RegWrite(ADPD400x_REG_GPIO01, 0x0302);
  adi_adpddrv_RegWrite(ADPD400x_REG_INT_STATUS_FIFO, 0x8000); // Clear FIFO
  adi_adpdssm_GetParameter(ADPD400x_LATEST_SLOT_DATASIZE, RegAdpd400x.highslot, \
                                                &temp);
  adi_adpdssm_SetParameter(ADPD400x_WATERMARKING, 0, CAL_WATERMARK);

}

/*!****************************************************************************
*
*  \brief       DeInitialize the clock calibration
*
*  \return      None
  *****************************************************************************/
static void Adpd400xClockCalibrationDeInit() {
  uint16_t temp;

  //adi_adpddrv_RegWrite(ADPD400x_REG_FIFO_CTL, RegAdpd400x.x06);
  adi_adpddrv_RegWrite(ADPD400x_REG_TS_FREQ, RegAdpd400x.x0D);
  adi_adpddrv_RegWrite(ADPD400x_REG_TS_FREQH, RegAdpd400x.x0E);
  adi_adpddrv_RegWrite(ADPD400x_REG_SYS_CTL, RegAdpd400x.x0F);
  adi_adpddrv_RegWrite(ADPD400x_REG_OPMODE, RegAdpd400x.x10);
  adi_adpddrv_RegWrite(ADPD400x_REG_FIFO_STATUS_BYTES, RegAdpd400x.x1E);
  adi_adpddrv_RegWrite(ADPD400x_REG_GPIO_CFG, RegAdpd400x.x22);
  adi_adpddrv_RegWrite(ADPD400x_REG_GPIO01, RegAdpd400x.x23);
  adi_adpddrv_RegWrite(ADPD400x_REG_TS_CTRL_A, RegAdpd400x.x0100);
  adi_adpddrv_RegWrite(ADPD400x_REG_DATA1_A, RegAdpd400x.x0110);
  adi_adpddrv_RegWrite(ADPD400x_REG_DATA2_A, RegAdpd400x.x0111);
  adi_adpddrv_RegWrite(ADPD400x_REG_DECIMATE_A, RegAdpd400x.x0112);

  adi_adpdssm_slotSetup(RegAdpd400x.highslot, 1, RegAdpd400x.slotformat, \
                                                       RegAdpd400x.channelnum);
  adi_adpdssm_GetParameter(ADPD400x_LATEST_SLOT_DATASIZE, RegAdpd400x.highslot, \
                                                &temp);
  adi_adpdssm_SetParameter(ADPD400x_WATERMARKING, 0, gsFifoWaterMark);

  if(dvt2)
  {
    adi_adpddrv_RegRead(ADPD400x_REG_OSC1M, &temp);
    /* Setting to 1 -> disable */
    adi_adpddrv_RegWrite(ADPD400x_REG_OSC1M, (temp| (BITM_OSC1M_OSC_CAL_DISABLE)));
  }
}
#endif

/*!****************************************************************************
*
*  \brief       32K OSC calibration
*
*  \return      CALIBRATION_SUCCESS - Success; CALIBRATION_FAILED - Failure
*****************************************************************************/
static int8_t CalibrateCL32KClock(void) {
  uint32_t nCurrentRate;
  int16_t nRateDiff, nRateDiffPre;
  int8_t nTrimCodeNew, i;
  uint16_t nCaptureTs, nClockRegVal, nClockRegValPre;
  uint16_t trimSlop;

  adi_adpddrv_RegWrite(ADPD400x_REG_GPIO_EXT, 0x50); // Enable Timestamp ON
  nClockRegVal = 0x15;
  adi_adpddrv_RegWrite(ADPD400x_REG_OSC32K, nClockRegVal);
  i = NTRIAL_32K;
  trimSlop = TRIM_SLOP;
  nRateDiff = 30000;   /*! initially set to a large difference value */

  while (i-->0) {
    adi_adpdssm_setOperationMode(E_ADI_ADPD_MODE_SAMPLE);
    adi_adpddrv_RegRead(ADPD400x_REG_OSC32K, &nCaptureTs);
    adi_adpddrv_RegWrite(ADPD400x_REG_OSC32K, nCaptureTs|0x8000); // Enable Timestamp ON
    GPIO_Clock_Cal_TriggerTS();
    adi_adpddrv_RegRead32B(ADPD400x_REG_STAMP_L, &nCurrentRate);
    nRateDiffPre = nRateDiff;
    nRateDiff = (nCurrentRate<<1) - 32000;

    adi_printf("Current Rate=%d, Setting=%04x\r\n", nCurrentRate, nClockRegVal);
    if (i == 0)
      break;
    if (nRateDiff < trimSlop && nRateDiff > (-1 * trimSlop)) {
      i = 1;
      nClockRegValPre = nClockRegVal;
      if (nRateDiff < 0)  {
        nClockRegVal -= 1;
      } else {
        nClockRegVal += 1;
      }
      adi_adpddrv_RegWrite(ADPD400x_REG_OSC32K, nClockRegVal);
      continue;
    }
    if (ABS(nRateDiffPre) < ABS(nRateDiff))  /*! if not converge, reduce the slop */
      trimSlop = TRIM_SLOP - (TRIM_SLOP/NTRIAL_32K);
    nTrimCodeNew = nRateDiff / trimSlop;
    nTrimCodeNew = (nClockRegVal & 0x3F) + nTrimCodeNew;
    if (nTrimCodeNew < 0)
      nTrimCodeNew = 1;
    if (nTrimCodeNew > 0x3F)
      nTrimCodeNew = 0x3E;

    nClockRegVal = (nClockRegVal & 0xFFC0) | nTrimCodeNew;
    adi_adpddrv_RegWrite(ADPD400x_REG_OSC32K, nClockRegVal);
    adi_adpdssm_setOperationMode(E_ADI_ADPD_MODE_IDLE);
  }

  if (ABS(nRateDiffPre) < ABS(nRateDiff))  {  // choose previous
    adi_adpddrv_RegWrite(ADPD400x_REG_OSC32K, nClockRegValPre);
  }

  return 0;
}

#ifdef CAL_BASED_ON_TIMESTAMP_REGISTER
/*!****************************************************************************
*
*  \brief       1M Osc calibration
*
*  \return       CALIBRATION_SUCCESS - success; CALIBRATION_FAILED - Failure
*****************************************************************************/
static int8_t CalibrateCL1MClock() {
  uint32_t nCurrentRate;
  int32_t nRateDiff;
  int16_t nTrimCodeNew, i;
  uint16_t nClockRegVal;
  uint16_t trimSlop;

  adi_adpddrv_RegWrite(ADPD400x_REG_GPIO_EXT, 0xD0); // Enable Timestamp ON
  adi_adpddrv_RegRead(ADPD400x_REG_OSC32K, &RegCl.x0c);
  nClockRegVal = 0x2B2; // Initial value
  adi_adpddrv_RegWrite(ADPD400x_REG_OSC1M, nClockRegVal);
  i = NTRIAL_1M;
  trimSlop = TRIM_SLOP_1M;

  while (i-->0) {
    adi_adpdssm_setOperationMode(E_ADI_ADPD_MODE_SAMPLE);
    adi_adpddrv_RegWrite(ADPD400x_REG_OSC32K, 0x8000); // Enable Timestamp Captute ON
    GPIO_Clock_Cal_TriggerTS();
    adi_adpddrv_RegRead32B(ADPD400x_REG_STAMP_L, &nCurrentRate);
    nRateDiff = 1000000 - (nCurrentRate<<1);
    nTrimCodeNew = nRateDiff / trimSlop;
    nTrimCodeNew = (nClockRegVal & 0x3FF) + nTrimCodeNew;
    if (nTrimCodeNew < 0)
      nTrimCodeNew = 1;
    if (nTrimCodeNew > 0x3FF)
      nTrimCodeNew = 0x3FE;

    nClockRegVal = (nClockRegVal & 0xFC00) | nTrimCodeNew;
    adi_adpddrv_RegWrite(ADPD400x_REG_OSC1M, nClockRegVal);
    adi_adpddrv_RegWrite(0xC, 0x0); // Disable Timestamp Captute ON
    adi_adpdssm_setOperationMode(E_ADI_ADPD_MODE_IDLE);
  }
  adi_adpddrv_RegWrite(ADPD400x_REG_OSC32K, RegCl.x0c);

  return 0;
}
#else
/*!****************************************************************************
*
*  \brief       1M Osc calibration
*
*  \return       CALIBRATION_SUCCESS - success; CALIBRATION_FAILED - Failure
*****************************************************************************/
static int8_t CalibrateCL1MClock(void) {
  int32_t nRateDiff;
  int16_t nTrimCodeNew, i;
  uint16_t nClockRegVal;
  uint32_t nTimeVal, nTimeVal1;
  uint16_t nTemp = 0;
  uint8_t nCalibrationCompleted = 0;
  ADI_OSAL_PRIORITY back_priority;

  nClockRegVal = 0x2E2; /*! Initial value */
  nTrimCodeNew = 0;     /*! Initial value */

  adi_adpddrv_RegWrite(ADPD400x_REG_OSC1M, nClockRegVal);

  i = NTRIAL_1M;
  while (i-->0) {
    adi_adpddrv_RegWrite(ADPD400x_REG_INT_STATUS_FIFO, 0x8000); // Clear FIFO
    // set GO
    nTemp |= ADPD400x_OP_RUN_MODE;
    adi_adpddrv_RegWrite(ADPD400x_REG_OPMODE, nTemp);

    adi_osal_ThreadGetPrio(NULL, &back_priority );
    adi_osal_ThreadSetPrio(NULL, configMAX_PRIORITIES - 1 );
    /*! Take initial time tick */
    nTimeVal = MCU_HAL_GetTick();
    do {
        MCU_HAL_Delay(1);
      /*! 1ms clk */
      nTimeVal1 = MCU_HAL_GetTick();
      if ((ABS(nTimeVal1 - nTimeVal)) > CAL_TIMEOUT)
        return -1;
    } while (gsIRQFlag == 0);  
    adi_osal_ThreadSetPrio(NULL,back_priority);
    // Set Idle Mode
    adi_adpdssm_setOperationMode(E_ADI_ADPD_MODE_IDLE);
    // set the IRQ flag to zero
    gsIRQFlag = 0;
    /*! Calculate the time trial difference */
    nRateDiff = nTimeVal1 - nTimeVal;
    
    if(nRateDiff > 0) {
      if ((nRateDiff > CALIBRATION_TIME) && (nRateDiff > ALLOWED_DEVIATION_MAX)) {
        nRateDiff = nRateDiff % CALIBRATION_TIME;
        if(dvt2)
        {
          nTrimCodeNew = (nRateDiff * 1U);
        }
        else
        {
          if(nRateDiff > 0) {
            nTrimCodeNew = 1;
          }
        }
      } else if ((nRateDiff < CALIBRATION_TIME) && (nRateDiff < ALLOWED_DEVIATION_MIN)) {
        nRateDiff = nRateDiff - CALIBRATION_TIME;
        if(dvt2)
        {
          nTrimCodeNew = nRateDiff * 1;
        }
        else
        {
          if(nRateDiff > 0) {
            nTrimCodeNew = -1;
          }
        }
      } else {
        nTrimCodeNew = 0;
        nCalibrationCompleted = 1;
      }
      if(!dvt2)
      {
        nTrimCodeNew = nTrimCodeNew * TRIM_1MHZ;
      }
    } else {
      /*! Shouldn't appear here */
      return (int8_t)CALIBRATION_FATAL_ERROR;
    }

    nTrimCodeNew = (nClockRegVal & 0x3FF) + nTrimCodeNew;
    if (nTrimCodeNew < 0)
      nTrimCodeNew = 1;
    if (nTrimCodeNew > 0x3FF)
      nTrimCodeNew = 0x3FE;

    nClockRegVal = (nClockRegVal & 0xFC00) | nTrimCodeNew;
    adi_adpddrv_RegWrite(ADPD400x_REG_OSC1M, nClockRegVal);
    if (nCalibrationCompleted)
      break;
  }
  if (nCalibrationCompleted) {
      return (int8_t)CALIBRATION_SUCCESS;
  } else {
      return (int8_t)CALIBRATION_FAILED;
  }
}
#endif

/*!****************************************************************************
*
*  \brief       32M Osc calibration
*
*  \return      CALIBRATION_SUCCESS - success; CALIBRATION_FAILED - Failure
*****************************************************************************/
static int8_t CalibrateCL32MClock(void) {
  uint16_t nReadReg, nVcoTrim, nVcoTrimPre;
  uint32_t nCur32Mcount = 0;
  uint32_t nPre32Mcount = 0;
  int32_t nCurCountnDiff;
  int32_t nPreCountnDiff;
  int8_t i;

  adi_adpddrv_RegRead(ADPD400x_REG_OSC32M, &nVcoTrim);

  for ( i = 0; i < NTRIAL_32M; i++ ) {
    adi_adpddrv_RegWrite(ADPD400x_REG_OSC32M_CAL, 0x8000);
    MCU_HAL_Delay(5);
    adi_adpddrv_RegRead(ADPD400x_REG_OSC32M_CAL, &nReadReg);
    nCur32Mcount = nReadReg & 0x7FFF;
    nCurCountnDiff = ABS((int32_t)(32000 - nCur32Mcount));
    nPreCountnDiff = ABS((int32_t)(32000 - nPre32Mcount));
    if ((nCurCountnDiff > nPreCountnDiff) && (nPreCountnDiff < 640)) { // TODO: set the proper threshold after testing it
      adi_adpddrv_RegWrite(ADPD400x_REG_OSC32M, nVcoTrimPre);
      return 0;   // success
    }
    nVcoTrimPre = nVcoTrim;
    if (nCur32Mcount < 32000) {
      nVcoTrim += ((32000 - nCur32Mcount) >> 8) + 1;
    } else {
      nVcoTrim -= ((nCur32Mcount - 32000) >> 8) + 1;
    }
    adi_adpddrv_RegWrite(ADPD400x_REG_OSC32M, nVcoTrim);

    nPre32Mcount = nCur32Mcount;
  }

  return -1;
}

/*!****************************************************************************
*
*  \brief       32M Osc calibration
*
*  \return       CALIBRATION_SUCCESS - success; CALIBRATION_FAILED - Failure
*****************************************************************************/
static int8_t CalibrateCL1M32MClock(void) {
  uint16_t nReadReg, nVcoTrim, nVcoTrimPre;
  uint32_t nCur32Mcount = 0;
  uint32_t nPre32Mcount = 0;
  int32_t nCurCountnDiff;
  int32_t nPreCountnDiff;
  int8_t i;

  adi_adpddrv_RegRead(ADPD400x_REG_OSC32M, &nVcoTrim);

  for ( i = 0; i < NTRIAL_32M; i++ ) {
    adi_adpddrv_RegWrite(ADPD400x_REG_OSC32M_CAL, 0x8000);
    MCU_HAL_Delay(5);
    adi_adpddrv_RegRead(ADPD400x_REG_OSC32M_CAL, &nReadReg);
    nCur32Mcount = nReadReg & 0x7FFF;
    nCurCountnDiff = ABS((int32_t)(nCur32Mcount - 4096));
    nPreCountnDiff = ABS((int32_t)(nPre32Mcount - 4096));
    if ((nCurCountnDiff > nPreCountnDiff) && (nPreCountnDiff < 80)) { // TODO: set the proper threshold after testing it
      adi_adpddrv_RegWrite(ADPD400x_REG_OSC32M, nVcoTrimPre);
      return 0;   // success
    }
    nVcoTrimPre = nVcoTrim;
    if (nCur32Mcount < 4096) {
      nVcoTrim += ((4096 - nCur32Mcount) >> 5) + 1;
    } else {
      nVcoTrim -= ((nCur32Mcount - 4096) >> 5) + 1;
    }
    adi_adpddrv_RegWrite(ADPD400x_REG_OSC32M_CAL, 0x0);
    adi_adpddrv_RegWrite(ADPD400x_REG_OSC32M, nVcoTrim);

    nPre32Mcount = nCur32Mcount;
  }

  return -1;
}

/*!**************************************************************************
*
*  \brief       Clock calibration routine for Adpd41x sensor
*
*  \param[in]   cal_id: Clock ID to select RC OSC to calibrate
*
*  \return      CALIBRATION_SUCCESS - success; CALIBRATION_FAILED - Failure
****************************************************************************/
#ifdef CAL_BASED_ON_TIMESTAMP_REGISTER
int8_t AdpdClDoClockCalibration(uint8_t cal_id) {
  int8_t nResult = 0;
  uint16_t nLfOsc;

  adi_adpddrv_RegRead(ADPD400x_REG_GPIO_CFG, &RegCl.x22);
  adi_adpddrv_RegRead(ADPD400x_REG_GPIO_EXT, &RegCl.x26);

  adi_adpddrv_RegWrite(ADPD400x_REG_GPIO_CFG, 0x8); // ADPD4000 GPIO1 in input mode

  adi_adpddrv_RegRead(ADPD400x_REG_SYS_CTL, &nLfOsc);
  if(cal_id & 0x1) {
     adi_adpddrv_RegWrite(ADPD400x_REG_SYS_CTL, (nLfOsc&0xFFF8)|0x1);
     nResult = CalibrateCL32KClock();
  } else if(cal_id & 0x2) {
     adi_adpddrv_RegWrite(ADPD400x_REG_SYS_CTL, (nLfOsc&0xFFF8)|0x6);
     nResult = CalibrateCL1MClock();
  }

  adi_adpddrv_RegRead(ADPD400x_REG_SYS_CTL, &nLfOsc);
  if(cal_id & 0x4) {
     if (nResult == 0) {
        if(nLfOsc & 0x4) {
           nResult =  CalibrateCL1M32MClock();
        } else {
           nResult =  CalibrateCL32MClock();
        }
     }
  }

  adi_adpddrv_RegWrite(ADPD400x_REG_GPIO_CFG, RegCl.x22);
  adi_adpddrv_RegWrite(ADPD400x_REG_GPIO_EXT, RegCl.x26);

  return nResult;
}
#else
int8_t Adpd4000DoClockCalibration(uint8_t cal_id) {
  int8_t nResult = 0;
  uint16_t nLfOsc;
  /*! Take the register backup before going to do calibration */
  Adpd400xClockCalibrationInit();
  // Read the Oscilator selection for LOFC
  adi_adpddrv_RegRead(ADPD400x_REG_SYS_CTL, &nLfOsc);
  if(cal_id & 0x1) {
     adi_adpddrv_RegWrite(ADPD400x_REG_SYS_CTL, (nLfOsc&0xFFF8)|0x1);
     nResult = CalibrateCL32KClock();
  } else if(cal_id & 0x2) {
     adi_adpddrv_RegWrite(ADPD400x_REG_SYS_CTL, (nLfOsc&0xFFF8)|0x6);
     nResult = CalibrateCL1MClock();
  }

  if(cal_id & 0x4) {
     if (nResult == 0) {
        if(nLfOsc & 0x4) {
           nResult =  CalibrateCL1M32MClock();
        } else {
           nResult =  CalibrateCL32MClock();
        }
     }
  }
  Adpd400xClockCalibrationDeInit();
  return nResult;
}
#endif // CAL_BASED_ON_TIMESTAMP_REGISTER

/*!**************************************************************************
*
*  \brief       Can be called after ADPDMwLib_DoClockCalibration to obtain the
*               accuracy of the clock calibration.
*
*  \return      0 - success; 1 - Failure
****************************************************************************/
uint32_t ADPDGetClockCalResults() {
    return 0;
}
