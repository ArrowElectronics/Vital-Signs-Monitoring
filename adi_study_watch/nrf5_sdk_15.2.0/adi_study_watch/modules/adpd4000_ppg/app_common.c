/**
*******************************************************************************
* @file         common.c
* @author       ADI
* @version      V1.0.0
* @date         12-August-2016
* @brief        Common function for HRM application.
*
*******************************************************************************
* @attention
*******************************************************************************
*/
/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2019 Analog Devices Inc.                                      *
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
* This software is intended for use with the ADPD and derivative parts        *
* only                                                                        *
*                                                                             *
******************************************************************************/
//#include "AdpdDrv.h"
#include "adpd400x_drv.h"
#include "adpd400x_reg.h"
#include "Adxl362.h"
#include "app_common.h"
#include "math.h"
#include "adi_adpd_ssm.h"

uint16_t reg_base;
uint16_t gPrevSamplerate = 0;
extern volatile uint8_t gbAdpdSamplingRateChanged;
extern void ppg_adjust_adpd_ext_trigger(uint16_t nOdr);
/* Static variable and function prototypes */
//uint8_t gnSynxDataSetSize = 0;
/**
* @brief    Get Data Rate
* @param    Output of sampling rate
* @param    Output of decimation value
* @return   -1=fail, 0=success
*/
/* int8_t GetAdpdOutputRate(uint16_t* sampleRate, uint16_t* decimation) {

  uint16_t samplefreq;
  uint16_t decrate;

  if (sampleRate == 0 || decimation == 0)
    return -1;

  AdpdDrvRegRead(REG_SAMPLING_FREQ, &samplefreq);
  AdpdDrvRegRead(REG_DEC_MODE, &decrate);

  if (samplefreq != 0) {
    *sampleRate = 8000 / samplefreq;
    *decimation = 1 << ((decrate >> 8) & 0x7);
  } else {
    return -1;
  }
  return 0;
} */

/**
* @brief    Set Data Rate
* @param    sampling rate
* @param    decimation value
* @return   -1=fail, 0=success
*/
/* int8_t SetAdpdOutputRate(uint16_t sampleRate, uint16_t decimation) {
  uint16_t sampleRateReg;
  uint16_t decReg;
  uint16_t decReg_proxBits;
  uint8_t i;

  if (sampleRate == 0 || decimation == 0)
    return -1;

  for (i=0; i<8; i++)  {
    decReg = 1 << i;
    if (decReg == decimation)
      break;
    if (decReg > decimation)
      return -1;
  }
  decReg = (i << 4 | i << 8);
  sampleRateReg = 8000/sampleRate;

  AdpdDrvRegRead(REG_DEC_MODE, &decReg_proxBits);
  decReg |= (decReg_proxBits & 0x7);

#ifdef __ADUCM4050__
  setAdpdSSTriggerSampleRate(sampleRate);
  setAdxlSSTriggerSampleRate((sampleRate >> i));
#endif

  AdpdDrvRegWrite(REG_DEC_MODE, decReg);
  AdpdDrvRegWrite(REG_SAMPLING_FREQ, sampleRateReg);

  return 0;
} */

/**
* @brief    Get adxl Data Rate
* @param    Output of sampling rate
* @return   -1=fail, 0=success
*/
int8_t GetAdxlOutputRate(uint16_t* sampleRate) {

  if (sampleRate == 0)
    return -1;

  // This driver call needs to be replaced with an m2m2 message as soon as the PS is running full m2m2
  AdxlDrvGetParameter(ADXL_ODRRATE, sampleRate);
  *sampleRate = (*sampleRate<5)  ? (((1 << (*sampleRate)) * 25 ) >> 1) : 400;

  return 0;
}


 // int16_t AdpdSelectSlot(uint8_t eSlotAMode,uint8_t eSlotBMode) {
    // uint16_t nFifoWatermark;
    // uint16_t  slot_A_data_sz = 0;
    // uint16_t  slot_B_data_sz = 0;
    // if (AdpdDrvGetParameter( ADPD_WATERMARKING, &nFifoWatermark) != ADPDDrv_SUCCESS) {
        // return ADPDDrv_ERROR;
    // }
    // /* Select operation slot for ADPD Device */
    // if (AdpdDrvSetSlot(eSlotAMode, eSlotBMode) != ADPDDrv_SUCCESS) {
        // return ADPDDrv_ERROR;
    // }
    // /* Set FIFO level based on select slot and fifowatermark */
    // if (AdpdDrvSetParameter( ADPD_WATERMARKING, nFifoWatermark) != ADPDDrv_SUCCESS) {
        // return ADPDDrv_ERROR;
    // }

    // AdpdDrvGetParameter(ADPD_DATASIZEA, &slot_A_data_sz);
    // AdpdDrvGetParameter(ADPD_DATASIZEB, &slot_B_data_sz);
    // gnSynxDataSetSize = slot_A_data_sz + slot_B_data_sz;
    // return ADPDDrv_SUCCESS;
// }

/* int16_t AdpdSetOperationMode(uint8_t eOpState) {
//  if (eOpState == ADPDDrv_MODE_SAMPLE)  {
//    adpd_buff_reset(0);
//  }
  if (AdpdDrvSetOperationMode(eOpState) != ADPDDrv_SUCCESS) {
    return -1;
  }

  return 0;
} */

/** @brief  callback function to Set AdpdCl operating mode, clear FIFO if needed
  *
  * @param  nOpMode 8-bit operating mode
  * @return int16_t A 16-bit integer: 0 - success; < 0 - failure
  */
int16_t AdpdClSetOperationMode(uint8_t eOpState) {
  uint16_t total_slot_size;
  if (eOpState == ADPD400xDrv_MODE_SAMPLE)  {
    // ClearDataBufferAdpdCL(); // ori
    // ResetTimeGapAdpdCL();    // ori
    adi_adpdssm_GetParameter(ADPD400x_SUM_SLOT_DATASIZE, 0, &total_slot_size);
    //adpdCl_buff_reset(total_slot_size);
  }
  if (adi_adpdssm_setOperationMode(eOpState) != ADPD400xDrv_SUCCESS) {
    return -1;
  }
  return 0;
}

/** @brief  callback function to Select operation time nSlot
  *
  * @param  nSlotA 8-bit time nSlot
  * @param  nSlotB 8-bit time nSlot
  * @return int16_t A 16-bit integer: 0 - success; < 0 - failure
  */
int16_t AdpdClSelectSlot(uint8_t eSlotAMode,uint8_t eSlotBMode) {
    /* Select operation slot B of ADPDCL Device */
    // Todo, error kfkfkf 0310
    // if(AdpdClDrvSlotSetup(1, 1, 4) != ADPD400xDrv_SUCCESS) {
    //    return ADPD400xDrv_ERROR;
    // }
    return ADPD400xDrv_SUCCESS;
}

/**
* @brief    Get Data Rate
* @param    Output of sampling rate
* @param    Output of decimation value, 1 = no decimation
* @return   -1=fail, 0=success
*/
int8_t GetAdpdClOutputRate(uint16_t* sampleRate, uint16_t* decimation, uint16_t slotN) {

    uint16_t nOsc = 0;
    uint16_t reg_base = 0;
    uint32_t nTimeSlotPeriod= 0;

  if (sampleRate == 0 || decimation == 0)
    return -1;

    /* Read SYS-CTL register (0x00F)
     *  Reg0x00F = 0x0006 = LFOSC = 1MHz enabled
     *  Reg0x00F = 0x0001 = LFOSC = 32KHz enabled
     */
    if(adi_adpddrv_RegRead(ADPD400x_REG_SYS_CTL, &nOsc) != ADPD400xDrv_SUCCESS)
    {
      return -1;
    }

    /* Read TS_FREQ/TS_FREQH registers (0x00D/0x00E)
     *  Reg0x00D 15:0 - TIMESLOT_PERIOD_L
     *  Reg0x00E  6:0 - TIMESLOT_PERIOD_H
     */
    
    if(adi_adpddrv_RegRead32B(ADPD400x_REG_TS_FREQ, &nTimeSlotPeriod) != ADPD400xDrv_SUCCESS)
    {
      return -1;
    }

    if(nOsc == 0x0006) {
      *sampleRate = 1000000/nTimeSlotPeriod; // LFOSC = 1MHz
    } else if (nOsc == 0x0001) {
      *sampleRate = 32000/nTimeSlotPeriod;   // LFOSC = 32KHz
    }
    reg_base = log2(slotN) * ADPD400x_SLOT_BASE_ADDR_DIFF;
    if(adi_adpddrv_RegRead(reg_base + ADPD400x_REG_DECIMATE_A, decimation) != ADPD400xDrv_SUCCESS)
    {
      return -1;
    }
    *decimation = (*decimation >> 4) & 0x7F;
    *decimation += 1;       // 1 means no decimation

  return 0;
}

/**
* @brief    Set Data Rate
* @param    sampling rate
* @param    decimation value
* @return   -1=fail, 0=success
*/
int8_t SetAdpdClOutputRate(uint16_t sampleRate, uint16_t decimation, uint16_t slotNum) {

     uint16_t nOsc = 0, temp16,reg_base = 0;
     uint32_t nTimeSlotPeriod = 0;

     // TODO there is separate decimate register for each slot.
     if (sampleRate == 0 || decimation == 0)
        return -1;

     /* Read SYS-CTL register (0x00F)
     *  Reg0x00F = 0x0006 = LFOSC = 1MHz enabled
     *  Reg0x00F = 0x0001 = LFOSC = 32KHz enabled
     */
    adi_adpddrv_RegRead(ADPD400x_REG_SYS_CTL, &nOsc);

    /* Read TS_FREQ/TS_FREQH registers (0x00D/0x00E)
     *  Reg0x00D 15:0 - TIMESLOT_PERIOD_L
     *  Reg0x00E  6:0 - TIMESLOT_PERIOD_H
     */
    if(nOsc == 0x0006) {
      nTimeSlotPeriod = 1000000/sampleRate; // LFOSC = 1MHz
    } else if (nOsc == 0x0001) {
      nTimeSlotPeriod = 32000/sampleRate;   // LFOSC = 32KHz
    }

    adi_adpddrv_RegWrite(ADPD400x_REG_TS_FREQ, (nTimeSlotPeriod&0xFFFF));
    adi_adpddrv_RegWrite(ADPD400x_REG_TS_FREQH,(nTimeSlotPeriod>>16));


    reg_base = log2(slotNum) * ADPD400x_SLOT_BASE_ADDR_DIFF;
    adi_adpddrv_RegRead(ADPD400x_REG_DECIMATE_A + reg_base, &temp16);
    temp16 = temp16 & 0xF80F;
    temp16 |= (decimation - 1) << 4;//subract one becuase DECIMATE_FACTOR_x + 1 in register.
    adi_adpddrv_RegWrite(ADPD400x_REG_DECIMATE_A + reg_base, temp16);

    if(sampleRate != gPrevSamplerate){
      ppg_adjust_adpd_ext_trigger(sampleRate);// Adjusting the external trigger timer
      gPrevSamplerate = sampleRate;
      gbAdpdSamplingRateChanged = 1;
    }
  return 0;
}