/**
    ****************************************************************************
    * @file     Utility.c
    * @author   ADI
    * @version  V1.0
    * @date     22-Jul-2019
    ****************************************************************************
    * @attention
    ****************************************************************************
    */
/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2016 Analog Devices Inc.                                      *
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
* This software is intended for use with the ADPD and derivative parts    *
* only                                                                        *
*                                                                             *
******************************************************************************/
// ToDo:
/*
uint16_t UtilGetCurrentFromReg(uint16_t reg12, uint16_t reg34);
uint8_t UtilGetCurrentRegValue(uint16_t current, uint16_t* coarseReg, uint16_t* fineReg)
void UtilSetLoop1Config()
AdpdSetMode_RegCB

*/

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include "adpd400x_lib.h"
#include "adpd400x_lib_common.h"

//================== LOG LEVELS=============================================//
#define NRF_LOG_MODULE_NAME PPG_LIB

#if PPG_LIB_CONFIG_LOG_ENABLED
#ifdef NRF_LOG_DEFAULT_LEVEL
#undef NRF_LOG_DEFAULT_LEVEL
#define NRF_LOG_DEFAULT_LEVEL  PPG_LIB_CONFIG_LOG_LEVEL
#endif
#define NRF_LOG_INFO_COLOR  PPG_LIB_CONFIG_INFO_COLOR
#define NRF_LOG_DEBUG_COLOR  PPG_LIB_CONFIG_DEBUG_COLOR
#else //PPG_LIB_CONFIG_LOG_ENABLED
#define NRF_LOG_LEVEL       0
#endif //PPG_LIB_CONFIG_LOG_ENABLED
#include "nrf_log.h"

#define MODULE ("Utility.c")

#define SD_BUFFER_SIZE      25      // around 1/2 second data
#define LED_PHASE1_CONST  1.7
#define LED_PHASE2_CONST  1.5 
#define STEP1_CURRENT 16
#define MAX_REG_CURRENT 127

#ifdef TEST_AGC_PULSE
extern uint8_t gSignalFlag;
#endif
//#ifndef RELEASE_PRODUCTION
//#pragma location = "bank2_retained_ram"
//char SBZ[_BUF_SIZE];
//#endif  // RELEASE_PRODUCTION

/* Public function prototypes -----------------------------------------------*/
void Adpd400xUtilGetMeanVarInit(uint8_t avg_size, uint8_t get_var);
INT_ERROR_CODE_t Adpd400xUtilGetMeanVar(uint32_t* rawData, uint32_t* mean_val,
                               uint32_t *varCh1,uint32_t *varCh2);
uint16_t Adpd400xUtilGetCurrentFromSlot(uint8_t);
/* uint16_t Adpd400xUtilGetCurrentValue(uint16_t coarseReg, uint16_t fineReg); */
uint16_t LED_Current_Monotonic_Calculation(uint16_t led_coarse);
void Adpd400xUtilSetLoop1Config(void);
void Adpd400xSetModeCB(ADPD400xDrv_Operation_Mode_t);
void PpgAdpd400xSetModeCB(ADPD400xDrv_Operation_Mode_t);
void Adpd400xSetSlotCb(ADPD400xDrv_Operation_Mode_t, ADPD400xDrv_Operation_Mode_t);
void Adpd400xSetMode_RegCB(int16_t (*pfADPDSetMode)(uint8_t));
void Adpd400xSetSlot_RegCB(int16_t (*pfADPDSetSlot)(uint8_t, uint8_t));
void PpgAdpd400xSetMode_RegCB(int16_t (*pfPpgSetMode)(uint8_t));

/* Private function prototypes ----------------------------------------------*/
static uint32_t get_variance(uint32_t *data, uint32_t n);
static int16_t (*gsfnADPDSetModeCB)(uint8_t);
static int16_t (*gsfnPPGSetModeCB)(uint8_t);
static int16_t (*gsfnADPDSetSlotCB)(uint8_t, uint8_t);
static int8_t (*gsfnADPDSetSampleRateCB)(uint16_t, uint16_t, uint16_t);
static int8_t (*gsfnADPDGetSampleRateCB)(uint16_t*, uint16_t*, uint16_t);

/* Public Variables ---------------------------------------------------------*/
uint32_t gUserDcLevelPerPulse = 0;
extern uint32_t gAdpd400xPpgTimeStamp;
AdpdClSlotOri_t gSlotOri;
uint32_t gnAdpd400xTempData[4*SD_BUFFER_SIZE];      // Temporary buffer

// detection //
Adpd400xDetection_t gAdpd400xDetectVal;

// Led Calibration //
AdpdOptimization_t gAdpd400xOptmVal;

// Status
Adpd400xLibStat_t gAdpd400xPPGLibStatus;

// Float Mode
uint16_t g_Adpd400xdeviceMode,gDVT2 = 0;
uint16_t g_Adpd400xTiaG_Init;
uint8_t g_Adpd400xchannelNums;

#define CFG_REG_NUM   6
#define MAX_SIGNAL_LEVEL_FOR_PULSE_ADJ 600000
#define SINGLE_PULSE_SATURATION_VALUE 8000
#define MIN_PULSE               4 //2 //changed from 2 -> 4

uint16_t gAdpd400xOpModeUsedReg[CFG_REG_NUM] = {
  ADPD400x_REG_AFE_TRIM_A,        // 104
  ADPD400x_REG_LED_POW12_A,       // 105
  ADPD400x_REG_LED_POW34_A,       // 106
  ADPD400x_REG_COUNTS_A,          // 107
  ADPD400x_REG_PERIOD_A,          // 108
  ADPD400x_REG_LED_PULSE_A,       // 109
  // ADPD400x_REG_INTEG_WIDTH_A,     // 10A
  };  // 10 regs each row

AdpdClConfig_t gAdpd400xFloatModeCfg[CFG_REG_NUM];
AdpdClConfig_t gAdpd400xNormalModeCfg[CFG_REG_NUM];

// Power calculation //
AdpdPowerUsage_t gAdpd400xPowerVal;

/* Private Variables --------------------------------------------------------*/
static uint8_t AvgSize, DataNum, GetVariance;
static uint32_t *Ch1Data,*Ch2Data;

/* Extern function prototypes */

/**
    * @brief    getSignal mean and variance initialization
    * @param    avg_size: amount of data for calculation
    * @param    get_var: get variance value
    * @retval   none
    */
void Adpd400xUtilGetMeanVarInit(uint8_t avg_size, uint8_t get_var) {
    if (avg_size > SD_BUFFER_SIZE || avg_size == 0)
        avg_size = SD_BUFFER_SIZE;
    AvgSize = avg_size;
    GetVariance = get_var;
    DataNum = 0;
    Ch1Data = &gnAdpd400xTempData[0];
    Ch2Data = &gnAdpd400xTempData[SD_BUFFER_SIZE];
}

/**
    * @brief    getSignal mean and variance
    * @param    rawData: channel data
    * @param    mean:    mean of all channels
    * @param    var:     variance of all channels
    * @retval   ADPDMW_ERR_PROC = processing,  ADPDMW_ERR_FAIL = invalid,
    * @retval   ADPDMW_ERR_PASS = Success.
    */
INT_ERROR_CODE_t Adpd400xUtilGetMeanVar(uint32_t *rawData,
                                uint32_t *mean,
                                uint32_t *varCh1,uint32_t *varCh2) {
    static uint32_t rawTotal = 0;
    uint32_t vr_ch1,vr_ch2;

    if (DataNum == 0) {
        rawTotal = 0;
    }

    if (AvgSize == 0 || rawData == 0)
        return IERR_FAIL;

    Ch1Data[DataNum] = rawData[0];
    Ch2Data[DataNum] = rawData[1];
    rawTotal += rawData[0];

    if (++DataNum >=  AvgSize) {
        DataNum = 0;
        *mean = rawTotal/AvgSize;
        if (GetVariance != 0) {
            vr_ch1 = get_variance(Ch1Data, AvgSize);
            vr_ch2 = get_variance(Ch2Data, AvgSize);
            *varCh1 = vr_ch1;
            *varCh2 = vr_ch2;
        }
        Adpd400xUtilGetMeanVarInit(0, 1);   // in case next use didn't do initilization
        return IERR_SUCCESS;
    }
    return IERR_IN_PROGRESS;
}

/**
  * @internal
  * @brief    getSignal variance*16
  * @param    data: channel data
  * @param    n: window size
  * @retval   variance*16
  */
static uint32_t get_variance(uint32_t *data, uint32_t n) {
    uint32_t mean, variance_sum;
    uint32_t i;

    for (i = 0, mean = 0; i < n; i++)
        mean += data[i];
    mean = (uint32_t)(mean/n);

    for (i = 0, variance_sum = 0; i < n; i++) {
        variance_sum += (data[i] - mean) * (data[i] - mean);
    }

    variance_sum <<= 4;

    return (variance_sum/n);
}

int32_t Adpd400xMwLibLoadConfigFromArray(uint32_t *in) {
    uint32_t index = 0;

    if (in == 0)
        return -1;

    while (in[index] != 0xFFFFFFFF) {
        uint16_t address = (in[index] >> 16) & 0x00FF;
        uint16_t value   = (in[index]) & 0xFFFF;

        AdpdDrvRegWrite(address, value);
        /*
        if (funcptr_debugLog)
            funcptr_debugLog("DCFG: %c %02X = %04X",
                  (err < 0)?'E':' ',
                  (uint8_t)address,
                  (uint16_t)value);*/


        index++;
        if (index > 128) {
          return -2;
        }
    }

    return 0;
}

int32_t Adpd400xMwLibReadConfigToArray(uint32_t *in) {
  int32_t ret = 0;
  uint32_t index = 0;

  if (in == 0) {
    ret =  -1;
  } else {
    while (in[index] != 0xFFFFFFFF) {
      uint16_t address = (in[index] >> 16) & 0x00FF;
      uint16_t value   = (in[index]) & 0xFFFF;

      // printf("F: %08X\n", in[index]);

      AdpdDrvRegRead(address, &value);

      in[index] = (address << 16) + (value & 0x0000FFFF);
      /*
      if (funcptr_debugLog)
        funcptr_debugLog("DCFG: %c %02X = %04X",
                         (err < 0)?'E':' ',
                         (uint8_t)address,
                         (uint16_t)value);*/

      index++;
      if (index > 128) {
        ret = -2;
      }
    }
    in[index] = 0xFFFFFFFF;
  }
  return ret;
}

uint32_t Adpd400xMwLibGetCurrentTime() {  // in ms
    return (uint32_t)(AdpdLibGetTick());
}

void Adpd400xLibSetTriggerOnAirLevel(uint32_t in) {
    gAdpd400xDetectVal.detectOnAirLevel = in;
}

/**
  * @brief Set the used or unused Channel DC offset
  * @param slotX_num is the slot number
  * @param offset value for channel being used. Negative means don't set it
  * @retval None
  */
void Adpd400xSetChannelFilter(uint16_t slotNum, int16_t usedChValue) {
    uint8_t i;
    uint16_t addr;

    if (slotNum == 0)
      slotNum = 1;

    addr = PpgGetRegAddr(slotNum, ADPD400x_REG_ADC_OFF1_A);
    for (i=0; i<2; i++)  {
        if ((gAdpd400x_lcfg->targetChs & (i+1)) == 0)  {
            // write 0x3fff for unused channel
            AdpdDrvRegWrite(addr, 0x3FFF);
        } else {
            // if usedChValue is negative, don't set it.
            if (usedChValue >= 0)
              AdpdDrvRegWrite(addr, usedChValue);
        }
        addr ++;
    }
}

/**
  * @brief store configurationsetting for each pre-configuration modules
  * @param slotNum:  slot number
  * @retval none
  */
void Adpd400xStoreInitSetting(uint16_t slotNum)  {
  uint8_t i;
  AdpdClConfig_t *pConFig;

  if (slotNum < 1)
    return;
  for (i = 0; i < CFG_REG_NUM; i++)  {
    pConFig[i].addr = gAdpd400xOpModeUsedReg[i] + log2(slotNum) * SLOT_REG_OFFSET;
    AdpdDrvRegRead(pConFig[i].addr, &pConFig[i].value);
  }
}

/**
  * @brief apply configurationsetting for different AFE mode
  * @param slotNum:  slot number
  * @retval none
  */
void Adpd400xApplyInitSetting(uint16_t slotNum)  {
  uint8_t i;
  AdpdClConfig_t *pConFig;

  for (i=0; i<CFG_REG_NUM; i++)  {
    pConFig[i].addr = gAdpd400xOpModeUsedReg[i] + log2(slotNum) * SLOT_REG_OFFSET;
    AdpdDrvRegWrite(pConFig[i].addr, pConFig[i].value);
  }
}

/**
  * @brief store configurationsetting for different AFE mode
  * @param AFE operation mode
  * @retval none
  */
void Adpd400xStoreOpModeSetting(void)  {
  uint8_t i;
  g_reg_base = log2(gAdpd400x_lcfg->targetSlots) * SLOT_REG_OFFSET;
  AdpdClConfig_t *pConFig;
  if (gAdpd400xPPGLibStatus.AFE_OpMode == ADPD400xLIB_AFE_OPMODE_FLOAT)
    pConFig = gAdpd400xFloatModeCfg;
  else
    pConFig = gAdpd400xNormalModeCfg;

  for (i = 0; i < CFG_REG_NUM; i++)  {
    pConFig[i].addr = gAdpd400xOpModeUsedReg[i];
    AdpdDrvRegRead(g_reg_base + gAdpd400xOpModeUsedReg[i], &pConFig[i].value);
  }
}

/**
  * @brief apply configurationsetting for different AFE mode
  * @param AFE operation mode
  * @retval none
  */
void Adpd400xApplyOpModeSetting(void)  {
  uint8_t i;
  AdpdClConfig_t *pConFig;
  g_reg_base = log2(gAdpd400x_lcfg->targetSlots) * SLOT_REG_OFFSET;
  if (gAdpd400xPPGLibStatus.AFE_OpMode == ADPD400xLIB_AFE_OPMODE_FLOAT)  {
    pConFig = gAdpd400xFloatModeCfg;
  } else {
    pConFig = gAdpd400xNormalModeCfg;
    gAdpd400xOptmVal.SelectedLoop =1;
  }

  for (i=0; i<CFG_REG_NUM; i++)  {
    pConFig[i].addr = gAdpd400xOpModeUsedReg[i];
    AdpdDrvRegWrite(g_reg_base + gAdpd400xOpModeUsedReg[i], pConFig[i].value);
  }
}

/**
  * @brief Conver LED current register setting to LED current value
  * @param Slot Number, starts from 1
  * @retval uint16_t LED current value
  */
uint16_t Adpd400xUtilGetCurrentFromSlot(uint8_t slotNum)  {
  uint16_t led12_c, led34_c, led12_addr, led34_addr;

  // assuming ch1, g_lcfg->targetChs
  led12_addr = PpgGetRegAddr(slotNum, ADPD400x_REG_LED_POW12_A);
  led34_addr = PpgGetRegAddr(slotNum, ADPD400x_REG_LED_POW34_A);
  AdpdDrvRegRead(led12_addr, &led12_c);
  AdpdDrvRegRead(led34_addr, &led34_c);

  return Adpd400xUtilGetCurrentFromReg(led12_c, led34_c);
  /*
  led12_c = (led12_c & BITM_LED_POW12_A_LED_CURRENT1_A) + \
            ((led12_c & BITM_LED_POW12_A_LED_CURRENT2_A) >> 8);
  led34_c = (led34_c & BITM_LED_POW34_A_LED_CURRENT3_A) + \
            ((led34_c & BITM_LED_POW34_A_LED_CURRENT4_A) >> 8);

  led12_c = (uint16_t)((led12_c + led34_c) * 3 + 0.5);

  return led12_c;     // round to nearest digit
  */
}

/**
  * @brief Calculate LED current from two LED current setting registers
  * @param LED12 Register, LED34 Register
  * @retval uint16_t LED current value
  * TODO: Caller of this func should pass addresses appending it with g_reg_base
  */
uint16_t Adpd400xUtilGetCurrentFromReg(uint16_t led12_r, uint16_t led34_r)  {
  uint16_t led12_c, led34_c;

  led12_c = (led12_r & BITM_LED_POW12_A_LED_CURRENT1_A) + \
            ((led12_r & BITM_LED_POW12_A_LED_CURRENT2_A) >> 8);
  led34_c = (led34_r & BITM_LED_POW34_A_LED_CURRENT3_A) + \
            ((led34_r & BITM_LED_POW34_A_LED_CURRENT4_A) >> 8);

  /* led12_c = (uint16_t)((led12_c + led34_c) * 3 + 0.5); */
  led12_c = (uint16_t)((led12_c + led34_c)); 
  return led12_c;     // round to nearest digit
}

/**
  * @brief Conver LED current value to register setting
  * @param led12_reg: LED12 register setting
  * @param led34_reg: LED34 register setting
  * @retval uint8_t result, pass or fail
  */
uint8_t Adpd400xUtilGetCurrentRegValue(uint16_t current, uint16_t* led12_reg, uint16_t* led34_reg)  {
  uint16_t ledCurrent_each, remain_current;
  uint8_t active_leds;
  
  // assumming LED 1,2 and 3 are used
  /*
  AdpdDrvRegRead(g_reg_base + ADPD400x_REG_LED_POW12_A, led12_reg);
  AdpdDrvRegRead(g_reg_base + ADPD400x_REG_LED_POW34_A, led34_reg); 
  */
  g_reg_base = log2(gAdpd400x_lcfg->targetSlots) * SLOT_REG_OFFSET;
  AdpdDrvRegRead((ADPD400x_REG_LED_POW12_A  + g_reg_base), led12_reg);
  AdpdDrvRegRead((ADPD400x_REG_LED_POW34_A  + g_reg_base), led34_reg);

  active_leds = 0;
  if ((*led12_reg & BITM_LED_POW12_A_LED_CURRENT1_A) != 0)
    active_leds++;
  if ((*led12_reg & BITM_LED_POW12_A_LED_CURRENT2_A) != 0)
    active_leds++;
  if ((*led34_reg & BITM_LED_POW34_A_LED_CURRENT3_A) != 0)
    active_leds++;
  if ((*led34_reg & BITM_LED_POW34_A_LED_CURRENT4_A) != 0)
    active_leds++;

  /* ledCurrent_each = (uint16_t)(current / active_leds / 3); */
  if(active_leds == 0)
  {
    return 0;
  }
  ledCurrent_each = (uint16_t)(current / active_leds);

#if 0
  remain_current = current - (ledCurrent_each * 3 * active_leds);
  remain_current /= 3;
  remain_current += ledCurrent_each;        // 1st LED current
  if (ledCurrent_each > 0x7F)
    ledCurrent_each = 0x7F;
  if (remain_current > 0x7F)
    remain_current = 0x7F;
  // Todo: if (ledCurrent_each == 0)

  // equally distributed
  if ((*led12_reg & BITM_LED_POW12_A_LED_CURRENT1_A) != 0)     {
    *led12_reg = (*led12_reg & 0xFF80) | remain_current;
  }
#else
  if(ledCurrent_each > 0x7F)
  {
    ledCurrent_each = 0x7F;
  }
  if(active_leds * ledCurrent_each > 0xFF){
    return 0;
  }
  
  if ((*led12_reg & BITM_LED_POW12_A_LED_CURRENT1_A) != 0)     {
    *led12_reg = (*led12_reg & 0xFF80) | ledCurrent_each;
  }
#endif
  if ((*led12_reg & BITM_LED_POW12_A_LED_CURRENT2_A) != 0)     {
    *led12_reg = (*led12_reg & 0x80FF) | (ledCurrent_each<<8);
  }

  if ((*led34_reg & BITM_LED_POW34_A_LED_CURRENT3_A) != 0)     {
    *led34_reg = (*led34_reg & 0xFF80) | ledCurrent_each;
  }
  if ((*led34_reg & BITM_LED_POW34_A_LED_CURRENT4_A) != 0)     {
    *led34_reg = (*led34_reg & 0x80FF) | (ledCurrent_each<<8);
  }

  return 0;
}

void Adpd400xUtilSetLoop1Config()   {
  g_reg_base = log2(gAdpd400x_lcfg->targetSlots) * SLOT_REG_OFFSET;
  AdpdDrvRegWrite(g_reg_base + ADPD400x_REG_LED_POW12_A, gAdpd400xOptmVal.ledB_Cur);
  AdpdDrvRegWrite(g_reg_base + ADPD400x_REG_LED_POW34_A, gAdpd400xOptmVal.ledB_Trim);
  AdpdDrvRegWrite(g_reg_base + ADPD400x_REG_COUNTS_A, gAdpd400xOptmVal.ledB_Pulse);
  AdpdDrvRegWrite(g_reg_base + ADPD400x_REG_AFE_TRIM_A, gAdpd400xOptmVal.tiaB_Gain);
  PpgLibSetSampleRate(gAdpd400xOptmVal.sampleRate, gAdpd400xOptmVal.decimation,gAdpd400x_lcfg->targetSlots);//restore sample rate also from loop1
  gAdpd400xOptmVal.SelectedLoop =1;
}

void Adpd400xUtilRecordCriticalSetting()   {
  g_reg_base = log2(gAdpd400x_lcfg->targetSlots) * SLOT_REG_OFFSET;
  AdpdDrvRegRead(g_reg_base + ADPD400x_REG_AFE_TRIM_A, &gSlotOri.tia_Gain);     // R 104
  AdpdDrvRegRead(g_reg_base + ADPD400x_REG_LED_POW12_A, &gSlotOri.led12_Cur);   // R 105
  AdpdDrvRegRead(g_reg_base + ADPD400x_REG_LED_POW34_A, &gSlotOri.led34_Cur);   // R 106
  AdpdDrvRegRead(g_reg_base + ADPD400x_REG_COUNTS_A, &gSlotOri.led_Pulse);      // R 107
  AdpdDrvRegRead(g_reg_base + ADPD400x_REG_LED_PULSE_A, &gSlotOri.led_Width);   // R 109
  AdpdDrvRegRead(g_reg_base + ADPD400x_REG_ADC_OFF1_A, &gSlotOri.adc_Offset1);  // R 10E
  AdpdDrvRegRead(g_reg_base + ADPD400x_REG_ADC_OFF2_A, &gSlotOri.adc_Offset2);  // R 10F
  AdpdDrvRegRead(g_reg_base + ADPD400x_REG_DECIMATE_A, &gSlotOri.decimate_val); // R 112

  AdpdDrvRegRead(ADPD400x_REG_TS_FREQ, &gSlotOri.fs_low);         // R 0D
  AdpdDrvRegRead(ADPD400x_REG_TS_FREQH, &gSlotOri.fs_high);       // R 0E
}

void Adpd400xUtilRestoreCriticalSetting()   {
  g_reg_base = log2(gAdpd400x_lcfg->targetSlots) * SLOT_REG_OFFSET;
  AdpdDrvRegWrite(g_reg_base + ADPD400x_REG_AFE_TRIM_A, gSlotOri.tia_Gain);     // R 104
  AdpdDrvRegWrite(g_reg_base + ADPD400x_REG_LED_POW12_A, gSlotOri.led12_Cur);   // R 105
  AdpdDrvRegWrite(g_reg_base + ADPD400x_REG_LED_POW34_A, gSlotOri.led34_Cur);   // R 106
  AdpdDrvRegWrite(g_reg_base + ADPD400x_REG_COUNTS_A, gSlotOri.led_Pulse);      // R 107
  AdpdDrvRegWrite(g_reg_base + ADPD400x_REG_LED_PULSE_A, gSlotOri.led_Width);   // R 109
  AdpdDrvRegWrite(g_reg_base + ADPD400x_REG_ADC_OFF1_A, gSlotOri.adc_Offset1);  // R 10E
  AdpdDrvRegWrite(g_reg_base + ADPD400x_REG_ADC_OFF2_A, gSlotOri.adc_Offset2);  // R 10F
  AdpdDrvRegWrite(g_reg_base + ADPD400x_REG_DECIMATE_A, gSlotOri.decimate_val); // R 112

  AdpdDrvRegWrite(ADPD400x_REG_TS_FREQ, gSlotOri.fs_low);         // R 0D
  AdpdDrvRegWrite(ADPD400x_REG_TS_FREQH, gSlotOri.fs_high);       // R 0E
}

/**
  * @brief Find number of pulses
  * @param Slot number
  * @retval uint16_t number of pulses
  */
uint16_t Adpd400xUtilGetPulseNum(uint8_t slotNum)   {
  uint8_t repeatNum, integrateNum;
  uint16_t addr, pulseNum;

  addr = PpgGetRegAddr(slotNum,ADPD400x_REG_COUNTS_A)
  AdpdDrvRegRead(addr, &pulseNum);
  repeatNum = pulseNum & 0xFF;
  integrateNum = pulseNum >> 8;

  pulseNum = repeatNum * integrateNum;

  return pulseNum;
}

/**
  * @brief Set number of pulses
  * @param Slot number
  * @param total number of pulses
  * @param with ppg restriction
  * @retval uint16_t number of pulses
  */
INT_ERROR_CODE_t Adpd400xUtilSetPulseNum(uint8_t slotNum, uint16_t pulseNum, uint8_t res)   {
  uint8_t repeatNum, integrateNum;
  uint16_t addr;

  addr = PpgGetRegAddr(slotNum,ADPD400x_REG_COUNTS_A)
  if (pulseNum <= 0xFF)  {
    integrateNum = 1;
    repeatNum = pulseNum & 0xFF;
  } else if (pulseNum <= 0x1FF)  {
    integrateNum = 2;
    repeatNum = pulseNum / 2;
  } else if (pulseNum <= 0x2FF)  {
    integrateNum = 3;
    repeatNum = slotNum / 3;
  }

  if (repeatNum == 0)
    repeatNum = 1;

  if (res == 1) {       // with PPG restriction
    if (repeatNum > gAdpd400x_lcfg->maxPulseNum)
      repeatNum = gAdpd400x_lcfg->maxPulseNum - 1;
    if (repeatNum == 0xFF)
      repeatNum &= 0xFE;    // 255 pulse -> 254
    if ((repeatNum & 0x1) == 1)
      repeatNum++;          // +1 to make it even number
  }

  pulseNum = (uint16_t)(integrateNum << 8 | repeatNum);

  AdpdDrvRegWrite(addr, pulseNum);

  return IERR_SUCCESS;
}

void Adpd400xSetMode_RegCB(int16_t (*pfADPDSetMode)(uint8_t)) {
  gsfnADPDSetModeCB = pfADPDSetMode;
}

void Adpd400xSetModeCB(ADPD400xDrv_Operation_Mode_t op_mode) {
  (*gsfnADPDSetModeCB)((uint8_t)op_mode);
}

void PpgAdpd400xSetMode_RegCB(int16_t (*pfPpgSetMode)(uint8_t)) {
  gsfnPPGSetModeCB = pfPpgSetMode;    // register CB function
}

void PpgAdpd400xSetModeCB(ADPD400xDrv_Operation_Mode_t op_mode) {
  (*gsfnPPGSetModeCB)((uint8_t)op_mode);
}

void Adpd400xSetSlot_RegCB(int16_t (*pfADPDSetSlot)(uint8_t, uint8_t)) {
  gsfnADPDSetSlotCB = pfADPDSetSlot;
}

void Adpd400xSetSlotCb(ADPD400xDrv_Operation_Mode_t slotA_mode, \
                   ADPD400xDrv_Operation_Mode_t slotB_mode) {
  (*gsfnADPDSetSlotCB)((uint8_t)slotA_mode, (uint8_t)slotB_mode);
}

// set Sampling rate call back function
void PpgAdpd400xLibSetSampleRate_RegCB(int8_t (*pfADPDSetSampleRate)(uint16_t, uint16_t, uint16_t)) {
  gsfnADPDSetSampleRateCB = pfADPDSetSampleRate;
}

void PpgAdpd400xLibSetSampleRateCb(uint16_t rate, uint16_t decimation,uint16_t slotnum)  {
  (*gsfnADPDSetSampleRateCB)((uint16_t)rate, (uint16_t)decimation, slotnum);
}

// get Sampling rate call back function
void PpgAdpd400xLibGetSampleRate_RegCB(int8_t (*pfADPDGetSampleRate)(uint16_t*, uint16_t*, uint16_t)) {
  gsfnADPDGetSampleRateCB = pfADPDGetSampleRate;
}

void PpgAdpd400xLibGetSampleRateCb(uint16_t* rate, uint16_t* decimation, uint16_t slotnum)  {
  (*gsfnADPDGetSampleRateCB)(rate, decimation, slotnum);
}

uint32_t AdpdMwLibDeltaTime(uint32_t start, uint32_t current) {
    if (start > current)
        return current + (UINT_MAX - start) + 1;
    else
        return current - start;
}

#if 0
/**
  * @brief Conver LED current register setting to LED current value
  * @param LED Coase register setting
  * @param LED fine register setting
  * @retval uint16_t LED current value
  */
uint16_t Adpd400xUtilGetCurrentValue(uint16_t coarseReg, uint16_t fineReg)  {
  uint16_t temp16_c, temp16_f;

  if (gAdpd400x_lcfg->partNum != 108 && gAdpd400x_lcfg->partNum != 188)  {
    temp16_c = (uint16_t)((coarseReg & 0xF) * 19.8 + 50.3);
    temp16_f = (uint16_t)((fineReg & 0x1F) * 2.2 + 74);
    temp16_c *= temp16_f;

    if ((coarseReg & 0x2000) == 0)
      temp16_c *= 0.4;
  } else {
    temp16_c = (uint16_t)((coarseReg & 0xF) * 19.8 + 50.3);
    temp16_f = (uint16_t)((fineReg & 0x1F) * 2.2 + 74);
    temp16_c *= temp16_f;

    if ((coarseReg & 0x2000) == 0)
      temp16_c *= 0.1;
  }
  return (temp16_c+50)/100;     // round to nearest digit
}
#endif
/***
  This function does a pulse adjustment, following a pulse decrease setting and 
   it is called under two scenarios:
    1. AGC detects a need to decrease pulse --> ACOptSetLowerPowerProfile()
    3. Due to ADC Saturation, a pulse adjustment is done --> Saturation Adjust() 

    ** Function brief **
     
     The design adjusts pulse and maintains it above the minimum pulse setting.
     The factors determining the pulse adjustment include sampling frequency and
     current pulse value. The latter is not allowed to go below MIN_PULSE. The
     sampling rate is also not allowed to go below the ODR requirement for HRM
     ** Input parameters **
      npulse --> The pulse setting from calling code
      nsamplerate --> current sampling rate
      ndecimation --> current decimation factor
     ** Pseudo code description**
      a. If pulse is less than  MIN_PULSE and sampling rate > hrmODR, then
          halve sampling rate & decimation while doubling the pulse
     
      b. If pulse is an odd number, then make it to the lower even pulse number
     
      c. Clip the final pulse to the minimum pulse setting

       ** Pseudo code **    
      
      if((pulse < MIN_PULSE ) && (samplingrate > hrmODR)) {
         halve sampling rate
         halve decimation factor
         double pulse
       }
      
       pulse = (pulse is odd)?(pulse -1):pulse
       if(pulse<MIN_PULSE), then  pulse = MIN_PULSE
       return IERR_SUCCESS if pulse changed or IERR_FAIL if no change in pulse
 ***/

uint8_t ADPDLibPostPulseDecreaseAdjust(uint16_t *npulse,uint8_t *oripulse,
                                       uint16_t *nsamplerate,
                                       uint16_t *ndecimation,
                                       uint32_t *dc_level)
{
  uint16_t in_pulse = *oripulse;
  uint32_t NewSignalLevel;
  if(*nsamplerate > gAdpd400x_lcfg->hrmInputRate) {
       gUserDcLevelPerPulse = (*dc_level/(*oripulse)); 
       NewSignalLevel = (*oripulse * 2) * (gUserDcLevelPerPulse * 1.1);

     if (NewSignalLevel <= MAX_SIGNAL_LEVEL_FOR_PULSE_ADJ)  {
      *nsamplerate >>= 1;         // half sampling rate
      *ndecimation >>= 1;         // half decimation
      *npulse = *oripulse * 2;               // double the current pulses
     }
  }

  if ((*npulse & 0x1) == 1) // make it even number
    *npulse -= 1;
  if (*npulse < MIN_PULSE){
    *npulse = MIN_PULSE;    
  }

  return (*npulse == in_pulse)?IERR_FAIL:IERR_SUCCESS;  // settings changed 
}

/***
  This function does a pulse adjustment, following a pulse increase setting and 
   it is called under three scenarios:
    1. Loop1 --> where the pulse setting is done --> Op_FindPulseNum_108()
    2. AGC detects a need to increase pulse --> ACOptSetHighPowerProfile()
    3. Due to AFE Saturation, a pulse adjustment is done --> Saturation Adjust() 

    ** Function brief **
     The design is to monitor the signal level such that it does not cross a
     maximum  allowable level for the HRM algorithm. The factors determining the
     pulse adjustment include sampling frequency and signal level. 

     A global variable/value gUserDcLevelPerPulse will be calculated to check 
      how much will be the code contribution from a single original pulse. 
      A margin of 10% will be added to it.  This gUserDcLevelPerPulse will be
      used instead of a fixed value of code per pulse. By using this, for the
      same signal level, higher number of pulses can be allowed. This will be 
      especially applicable for skin tones requiring higher pulses to get a good
      signal quality. There are 3 places that this global variable will be 
      calculated:
       - From loop1, this will be a theoretical value based on CTR. 
          Here the dc signal level is not known
       - In SateMachine, in HEART_RATE_INIT stage. In this case, we will be 
          knowing the signal level
       - In this function, so that contribution per pulse can be calculated 
          before using it in adjusting the pulse

     Sampling frequency is not allowed to cross the maxSamplingRate in lcfg. The
     decimation will be adjusted to maintain the ODR
     ** Input parameters **
      newpulse --> The pulse setting from calling code
      oripulse --> pulse before increase from calling code
      nsamplerate --> current sampling rate
      ndecimation --> current decimation factor

     ** Pseudo code description**

      a. NewSignal is calculated based on each ori pulse contribution and limited
      to a level set by the saturate adjust percent level. A 10% margin is added 
      to per pulse contribution to account for any minor variations.

       NewSignal = newpulse*(gUserDcLevelPerPulse*1.1)*saturatelevel/100)
      
       b. This Newsignal is checked whether it is greater than 
          MAX_SIGNAL_LEVEL_FOR_PULSE_ADJ 
 
          i. If above is true, then sampling rate is checked whether it is 
	      less than maxSamplingRate. If true, then double sampling rate & 
	       decimation while halving the pulse
         ii. Else, then no more adjustment of pulse and the functions returns 9
             to indicate that npulse itself is high
      
      c. The incoming new pulse is also compared for its signal contribution and
         if more than MAX_SIGNAL_LEVEL_FOR_PULSE_ADJ, then return 9   

      d. Clip the newpulse to 255(g_lcfg->maxPulseNum), if it goes beyond 255
 
      e.If newpulse is an odd number, then make it to the lower even pulse number
     
      f. Clip the final newpulse to the minimum pulse setting

       ** Pseudo code **    
      Calculate gUserDcLevelPerPulse from equation. use 10% margin
      NewSignal = newpulse* (gUserDcLevelPerPulse*1.1)*targetDcPercent/100)
      if(NewSignal>MAX_SIGNAL_LEVEL_FOR_PULSE_ADJ) {
       if (samprate < maxSamplingRate) {
         double sampling rate
         double decimation factor
         halve pulse
       }
       else{
         return IERR_FAIL; // newpulse itself is very high 
           }
      }
      if((*newpulse * (gUserDcLevelPerPulse * 1.1)) > MAX_SIGNAL_LEVEL_FOR_PULSE_ADJ){
      return IERR_FAIL; // revert back to original pulse 
      }
       newpulse = (newpulse>255)?255:newpulse
       newpulse = (newpulse is odd)?(newpulse -1):newpulse
       if(newpulse<MIN_PULSE), then  pulse = MIN_PULSE
 ***/
uint8_t ADPDLibPostPulseIncreaseAdjust(uint16_t *newpulse,
                                       uint8_t *oripulse,
                                       uint16_t *nsamplerate, 
                                       uint16_t *ndecimation,
                                       uint32_t *dc_level){
uint32_t NewSignalLevel;  
    // calculate pulse contribution from original pulse and dc signal
   gUserDcLevelPerPulse = (*dc_level/(*oripulse));
   NewSignalLevel = *newpulse * (((gUserDcLevelPerPulse * 1.1) * gAdpd400x_lcfg->targetDcPercent)/100);  //gUserDcLevelPerPulse + 10% Margin
   
   if (NewSignalLevel > MAX_SIGNAL_LEVEL_FOR_PULSE_ADJ){
    if (*nsamplerate < gAdpd400x_lcfg->maxSamplingRate) {// sampleRate >= max
      *nsamplerate *= 2;         // double sampling rate
      *ndecimation *= 2;         // double decimation
      *newpulse = *oripulse/2;        // halve pulses   
#ifdef TEST_AGC_PULSE
      gSignalFlag = 1;
#endif 
    }
    else {
      return IERR_FAIL; /* return a code to indicate that the newpulse is too high */
    }
  }
  if((*newpulse * (gUserDcLevelPerPulse * 1.1)) > MAX_SIGNAL_LEVEL_FOR_PULSE_ADJ){
      return IERR_FAIL; /* revert back to original pulse */
  }
  /* Compare the newpulse with maximum of 255 and clip to 255 */
  if(*newpulse > gAdpd400x_lcfg->maxPulseNum) {
     *newpulse = gAdpd400x_lcfg->maxPulseNum;
  }
   
  if (*newpulse & 0x1 == 1) /* make even pulse */
    *newpulse -= 1;
  if (*newpulse < MIN_PULSE) /* if value < min, set min */
    *newpulse = MIN_PULSE;
  
  return IERR_SUCCESS; /* settings changed */
}

uint16_t LED_Current_Monotonic_Calculation(uint16_t led_coarse)
{
  uint16_t led_current = 0;

   if(!gDVT2)
  {
    if (led_coarse >= STEP1_CURRENT && led_coarse <= MAX_REG_CURRENT)
    {
      led_current = (LED_PHASE1_CONST * (led_coarse - 15)) + STEP1_CURRENT;
    }
    else if (led_coarse < STEP1_CURRENT && led_coarse > 0)
    {
      led_current = led_coarse + 1;
    }
    else if (led_coarse == 0)
    {
      led_current = 0;
    }
  }
  else 
  {
    if (led_coarse == 0)
    {
      led_current = 0;
    }
    else
    {
      led_current = LED_PHASE2_CONST * led_coarse;
    }
  }
  
  return led_current;
} 

uint16_t Adpd400xUtilGetRegFromMonotonicCurrent(uint8_t led_coarse)
{
  uint16_t ledRegValue = 0;
   if(!gDVT2)
  {
    if (led_coarse >= STEP1_CURRENT)
    {
      ledRegValue = ((led_coarse - STEP1_CURRENT)/LED_PHASE1_CONST) + 15;
      if(ledRegValue > BITM_LED_POW12_X_LED_CURRENT1_X) //check for max register value
        ledRegValue = BITM_LED_POW12_X_LED_CURRENT1_X;
    }
    else if (led_coarse < STEP1_CURRENT && led_coarse > 0)
    {
      ledRegValue = led_coarse - 1;
    }
    else if (led_coarse == 0)
    {
      ledRegValue = 0;
    }
  }
  else 
  {
    if (led_coarse == 0)
    {
      ledRegValue = 0;
    }
    else
    {
      ledRegValue = led_coarse/LED_PHASE2_CONST;
      if(ledRegValue > BITM_LED_POW12_X_LED_CURRENT1_X) //check for max register value
        ledRegValue = BITM_LED_POW12_X_LED_CURRENT1_X;

    }
  }
  
  return ledRegValue;
} 

void Adpd400xUpdateAGCInfoSettings(void)   {
  g_reg_base = log2(gAdpd400x_lcfg->targetSlots) * SLOT_REG_OFFSET;
  AdpdDrvRegRead(g_reg_base + ADPD400x_REG_LED_POW12_A, &gAdpd400xOptmVal.ledB_Cur2);
  AdpdDrvRegRead(g_reg_base + ADPD400x_REG_LED_POW34_A, &gAdpd400xOptmVal.ledB_Trim2);
  AdpdDrvRegRead(g_reg_base + ADPD400x_REG_COUNTS_A, &gAdpd400xOptmVal.ledB_Pulse2);
  AdpdDrvRegRead(g_reg_base + ADPD400x_REG_AFE_TRIM_A, &gAdpd400xOptmVal.tiaB_Gain2);
  PpgLibGetSampleRate(&gAdpd400xOptmVal.sampleRate2, &gAdpd400xOptmVal.decimation2,gAdpd400x_lcfg->targetSlots);//restore sample rate also from loop1
  gAdpd400xOptmVal.ledB_CurVal2 = Adpd400xUtilGetCurrentFromReg(gAdpd400xOptmVal.ledB_Cur2,gAdpd400xOptmVal.ledB_Trim2);
  gAdpd400xAGCStatInfo.timestamp = AdpdLibGetSensorTimeStamp();
  gAdpd400xAGCStatInfo.mts[0] = 0xFFFF;// No signal quality check for static AGC calibration and saturation case
  gAdpd400xAGCStatInfo.setting[1] = gAdpd400xOptmVal.ledB_Cur2;
  gAdpd400xAGCStatInfo.setting[2] = gAdpd400xOptmVal.ledB_Trim2;
  gAdpd400xAGCStatInfo.setting[3] = gAdpd400xOptmVal.ledB_Pulse2;
  gAdpd400xAGCStatInfo.setting[4] = gAdpd400xOptmVal.tiaB_Gain2;
  gAdpd400xAGCStatInfo.setting[5] = gAdpd400xOptmVal.sampleRate2;
  gAdpd400xOptmVal.SelectedLoop = 2;
}
