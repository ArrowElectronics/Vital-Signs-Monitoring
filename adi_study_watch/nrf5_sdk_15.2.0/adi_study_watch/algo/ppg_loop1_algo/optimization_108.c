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
* This software is intended for use with the ADPD and derivative parts    *
* only                                                                        *
*                                                                             *
******************************************************************************/
#include <stdio.h>
#include <math.h>
#include "adpd400x_lib.h"
#include "adpd400x_lib_common.h"
#include "variables.h"

/* Public define ------------------------------------------------------------*/
#define MODULE ("Opt108.c")

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

#define ADPD_VOL                3.3
#define LED_CURVE_CUTOFF_PTR    50      // 30 for 50% cut off. 50 for 75%
#define PULSE_CTR_START_PTR     7       // 0-19 for 50% 7-26 for 75%
#define PULSE_CTR_END_PTR       26      // 19 for 50% cut off. 26 for 75%
#define SATURATED_PER_PULSE     8000    // around 8k per pulse for Normal mode
#define SKIP_NUM                3       // skip the first few data for process

/* Public variables ---------------------------------------------------------*/
extern Adpd400xLibConfig_t *gAdpd400x_lcfg;
extern uint16_t gnPulseWidth, gnAfeGainNum, gnBpfGainNum;

/* Public function prototypes -----------------------------------------------*/
void Adpd400xOptimizationInit_108(void);
void Adpd400xOptimizationDeInit_108(void);
INT_ERROR_CODE_t Adpd400xOptimizeSetting_108(uint32_t *rawData);

/* Private function prototypes ----------------------------------------------*/

/* Private variables --------------------------------------------------------*/
// static uint8_t gsChNum;
static uint16_t *gsSkipCnt;
static uint8_t *gsDeviceIndex, *gsPulseNum;
static uint16_t *gsAfePower, *gsLedPower, *gsTotalPower;
static double *gsFitCurveBase1, *gsFitCurveExponent1;
static double *gsFitCurveBase2, *gsFitCurveExponent2;

static void Op_FindPulseNum_108(void);
static void Op_FindPower_108(void);
static void Op_VerifyLedResult(uint32_t *rawData);

/* Constant variables -------------------------------------------------------*/
// constant from Optimzation Method2 Excel sheet

// 83db, 75% LED
static const double LED_FitCurve[][4] = {
        {52777, -1.63, 11271, -1.24},       // Adpd108 LED/AFE = 2/3
        {25979, -1.56, 4918.5, -1.13},      // Adpd108 LED/AFE = 3/4
        {138153, -1.824, 26602,  -1.41},    // Adpd188 LED/AFE = 2/3
        {61440, -1.734, 10262, -1.275},     // Adpd188 LED/AFE = 3/4
};

// Array starts from CTR=6 to CTR = 26
// 83db, 75% LED
static const uint8_t LED_Pulse[][20] = {  // index = 0 for ctr = 1, fs = 50
    {124, 124, 124, 124, 124, 94, 74, 60, 50, 44, \
      38, 32, 30, 26, 26, 26, 26, 26, 26, 26 },        // Adpd108 LED/AFE = 2/3
    {132, 132, 132, 132, 98, 78, 64, 52, 44, 42, \
      42, 42, 42, 42, 42, 42, 42, 42, 42, 42, },        // Adpd108 LED/AFE = 3/4
    {130, 130, 130, 130, 112, 84, 68, 56, 48, 40, \
      36, 30, 28, 24, 22, 20, 18, 16, 16, 14, },        // Adpd188 LED/AFE = 2/3
    {112, 112, 112, 112, 86, 68, 56, 46, 40, 34, \
      30, 26, 24, 24, 24, 24, 24, 24, 24, 24, },        // Adpd188 LED/AFE = 3/4
};

static struct Register {
  uint16_t x104;    // tia
  uint16_t x105;    // LED12
  uint16_t x106;    // LED34
  uint16_t x107;    // LED pulses
  uint16_t x109;    // LED width
}Reg;

// static uint16_t gsSampleRateVal, gsDecimationVal;
/**
  * @brief  Initialization rountine for Setting Optimization.
  * @param  None.
  * @retval SUCCESS = done
  */
void Adpd400xOptimizationInit_108() {
  gsPulseNum = (uint8_t*)&gnAdpd400xTempData[0];
  // gsLedCurrent = (uint16_t*)&gnTempData[1];
  gsSkipCnt = (uint16_t*)&gnAdpd400xTempData[2];
  gsDeviceIndex = (uint8_t*)&gnAdpd400xTempData[3];
  gsLedPower = (uint16_t*)&gnAdpd400xTempData[4];
  gsAfePower = (uint16_t*)&gnAdpd400xTempData[5];
  gsTotalPower = (uint16_t*)&gnAdpd400xTempData[6];

  gsFitCurveBase1 = (double*)&gnAdpd400xTempData[10];
  gsFitCurveExponent1 = (double*)&gnAdpd400xTempData[14];
  gsFitCurveBase2 = (double*)&gnAdpd400xTempData[18];
  gsFitCurveExponent2 = (double*)&gnAdpd400xTempData[22];
  
  g_reg_base = log2(gAdpd400x_lcfg->targetSlots) * SLOT_REG_OFFSET;
  AdpdDrvRegRead(g_reg_base + ADPD400x_REG_AFE_TRIM_A, &Reg.x104);
  AdpdDrvRegRead(g_reg_base + ADPD400x_REG_LED_POW12_A, &Reg.x105);
  AdpdDrvRegRead(g_reg_base + ADPD400x_REG_LED_POW34_A, &Reg.x106);
  AdpdDrvRegRead(g_reg_base + ADPD400x_REG_COUNTS_A, &Reg.x107);
  AdpdDrvRegRead(g_reg_base + ADPD400x_REG_LED_PULSE_A, &Reg.x109);

  //PpgLibGetSampleRate(&gsSampleRateVal, &gsDecimationVal,gAdpd400x_lcfg->targetSlots);  // backup fs

  *gsDeviceIndex = (Reg.x109 >> 8);     // get LED width as array index
  if (gAdpd400x_lcfg->partNum == 108 || gAdpd400x_lcfg->partNum == 0x00C0)  {
    *gsDeviceIndex = *gsDeviceIndex - 2;    // 108 index=0,1. 188 index = 2,3
  }

  *gsSkipCnt = SKIP_NUM;
  return;
}

/**
  * @brief  Main Setting Optimization control routine.
  * @param  None.
  * @retval SUCCESS = done, IERR_IN_PROGRESS = in progress
  */
INT_ERROR_CODE_t Adpd400xOptimizeSetting_108(uint32_t *rawData) {
  // find pulses
  // calculate power
  // convert sampling rate
  if (*gsSkipCnt == SKIP_NUM)  {
    Op_FindPulseNum_108();
    AdpdMwLibSetMode(ADPD400xDrv_MODE_IDLE, ADPD400xDrv_SIZE_0, ADPD400xDrv_SIZE_0);
    Adpd400xUtilSetLoop1Config();     // Apply Loop1 optimum setting
    AdpdMwLibSetMode(ADPD400xDrv_MODE_SAMPLE,
                    ADPD400xDrv_SIZE_0,
                    (ADPD400xDrv_Operation_Mode_t)(gAdpd400x_lcfg->deviceMode));
    *gsSkipCnt -= 1;
    return IERR_IN_PROGRESS;
  }

  if (*gsSkipCnt > 0)  {
    *gsSkipCnt -= 1;
    return IERR_IN_PROGRESS;
  }

  AdpdMwLibSetMode(ADPD400xDrv_MODE_IDLE, ADPD400xDrv_SIZE_0, ADPD400xDrv_SIZE_0);
  Op_VerifyLedResult(rawData);
  Op_FindPower_108();
  Adpd400xUtilSetLoop1Config();     // Apply Loop1 optimum setting
  AdpdMwLibSetMode(ADPD400xDrv_MODE_SAMPLE,
                   ADPD400xDrv_SIZE_0,
                   (ADPD400xDrv_Operation_Slot_t)(gAdpd400x_lcfg->deviceMode));
  return IERR_SUCCESS;
}

/**
  * @brief  Deinitialization rountine for Setting Optimization.
  * @param  None.
  * @retval SUCCESS = done
  */
void Adpd400xOptimizationDeInit_108()  {
  AdpdMwLibSetMode(ADPDDrv_MODE_IDLE, ADPD400xDrv_SIZE_0, ADPD400xDrv_SIZE_0);
  // PpgLibSetSampleRate(gsSampleRateVal, gsDecimationVal,gAdpd400x_lcfg->targetSlots);    // restore fs
  
  g_reg_base = log2(gAdpd400x_lcfg->targetSlots) * SLOT_REG_OFFSET;
  AdpdDrvRegWrite(g_reg_base + ADPD400x_REG_AFE_TRIM_A, Reg.x104);
  AdpdDrvRegWrite(g_reg_base + ADPD400x_REG_LED_POW12_A, Reg.x105);
  AdpdDrvRegWrite(g_reg_base + ADPD400x_REG_LED_POW34_A, Reg.x106);
  AdpdDrvRegWrite(g_reg_base + ADPD400x_REG_COUNTS_A, Reg.x107);
  AdpdDrvRegWrite(g_reg_base + ADPD400x_REG_LED_PULSE_A, Reg.x109);

  AdpdMwLibSetMode(ADPD400xDrv_MODE_SAMPLE,
                  ADPD400xDrv_SIZE_0,
                  (ADPDDrv_Operation_Slot_t)(gAdpd400x_lcfg->deviceMode));
}

/**
  * @brief  Fine pulses, sampling rate, LED current
  * @param  None.
  * @retval None.
  */
static void Op_FindPulseNum_108(void) {
  uint16_t ctr, deviceI, sampleRate, decimateVal;
  double temp_d;
  uint16_t ledCur, coarseReg, fineReg;
  
  uint16_t temp_16 = 0;
  uint8_t retVal;
  uint16_t div_factor;
  uint32_t dc_level;
  
  PpgLibGetSampleRate(&sampleRate, &decimateVal,gAdpd400x_lcfg->targetSlots);  // backup sampling rate
  deviceI = *gsDeviceIndex;
  *gsFitCurveBase1 = LED_FitCurve[deviceI][0];
  *gsFitCurveExponent1 = LED_FitCurve[deviceI][1];
  *gsFitCurveBase2 = LED_FitCurve[deviceI][2];
  *gsFitCurveExponent2 = LED_FitCurve[deviceI][3];
  
  ctr = gAdpd400xPPGLibStatus.CtrValue;
  if (ctr == 0)
    ctr = 1;
  if (ctr <= LED_CURVE_CUTOFF_PTR)  {
    temp_d = pow(ctr, (*gsFitCurveExponent1));
    ledCur = (uint16_t)(temp_d * (*gsFitCurveBase1));
	NRF_LOG_ERROR("CTR power " NRF_LOG_FLOAT_MARKER "\r\n", NRF_LOG_FLOAT(temp_d));
  } else {    // ctr > FixCurveCutOffPt
    temp_d = pow(ctr, (*gsFitCurveExponent2));
    ledCur = (uint16_t)(temp_d * (*gsFitCurveBase2));
	NRF_LOG_ERROR("CTR Power " NRF_LOG_FLOAT_MARKER "\r\n", NRF_LOG_FLOAT(temp_d));
  }
  if (ledCur > gAdpd400x_lcfg->maxLedCurrent)
    ledCur = gAdpd400x_lcfg->maxLedCurrent;

  NRF_LOG_DEBUG("Calculated Led current = %d\r\n",ledCur);

  if (ctr <= PULSE_CTR_START_PTR)           // ctr = 0 - 7
    *gsPulseNum = LED_Pulse[deviceI][0];
  else if (ctr >= PULSE_CTR_END_PTR)        // ctr >= 26
    *gsPulseNum = LED_Pulse[deviceI][19];
  else
    *gsPulseNum = LED_Pulse[deviceI][ctr-PULSE_CTR_START_PTR];

  /*if (*gsPulseNum > gAdpd400x_lcfg->maxPulseNum)
      *gsPulseNum = gAdpd400x_lcfg->maxPulseNum - 1;*/

  dc_level =((ctr * gnBpfGainNum * gnPulseWidth * ledCur * (*gsPulseNum)) / (460 * gnAfeGainNum));
  // check sampling rate
#if 0
  // adjust by the expected percentage
  temp_16 = ((gAdpd400x_lcfg->postLoop1PulseAdjust + 100) * (*gsPulseNum)) / 100;
#else
  temp_16 =   *gsPulseNum; 
#endif
  // adjust temp_16 for the sampling rate
  div_factor = sampleRate/50;           // above array tuned for 50HZ  
  if(div_factor != 0){
    temp_16 = (temp_16/div_factor);
  }
  retVal = ADPDLibPostPulseIncreaseAdjust(&temp_16,gsPulseNum,&sampleRate, &decimateVal,&dc_level);
  
  // invalid pulse range and revert back to gsPulseNum
  if(retVal == IERR_FAIL) {
    temp_16 = *gsPulseNum; 
  }
  *gsPulseNum = temp_16;
#if 0 
   // adjust by the expected percentage
  temp_16 = ((gAdpd400x_lcfg->postLoop1CurrAdjust + 100) * (ledCur)) / 100;
#endif
  if (temp_16 > gAdpd400x_lcfg->maxLedCurrent) { 
    ledCur = gAdpd400x_lcfg->maxLedCurrent - 1; 
  } else {
    ledCur = temp_16;
  }
  /* *gsPulseNum = (*gsPulseNum * 50) / gAdpd400xOptmVal.sampleRate;
  if ((*gsPulseNum & 0x1) != 0)   // odd value
      *gsPulseNum = *gsPulseNum + 1; */

  Adpd400xUtilGetCurrentRegValue(ledCur, &coarseReg, &fineReg);
  gAdpd400xOptmVal.ledB_Cur = coarseReg;
  gAdpd400xOptmVal.ledB_Trim = fineReg;
  gAdpd400xOptmVal.ledB_Pulse = (Reg.x107 & 0xFF00) | *gsPulseNum;  // num repeat
  gAdpd400xOptmVal.tiaB_Gain = Reg.x104 & 0xFFF8;                   // always 200k
  gAdpd400xOptmVal.ledB_CurVal = Adpd400xUtilGetCurrentFromReg(coarseReg, fineReg);

  //debug(MODULE, "Current_I=%d, Pulse=%d \r\n", \
    ledCur, *gsPulseNum);
	
  NRF_LOG_DEBUG("Current_I=%d, Pulse=%d \r\n",
   ledCur, *gsPulseNum);

  // OptimizationDoFloatCheck(selected);   // check if FM should be used
}

/**
  * @brief  Verify LED current, reduced if it is saturated
  * @param  None.
  * @retval None.
  */
static void Op_VerifyLedResult(uint32_t *rawData) {
  uint16_t ledCurrent, ledReg12, ledReg34;
  uint32_t saturatedValue;
  
  g_reg_base = log2(gAdpd400x_lcfg->targetSlots) * SLOT_REG_OFFSET;

  saturatedValue = *gsPulseNum * SATURATED_PER_PULSE;
  if (rawData[0] >= saturatedValue)  {      // reduce LED current
    //ledCurrent = Adpd400xUtilGetCurrentFromSlot(PPG_SLOTA);
    ledCurrent = Adpd400xUtilGetCurrentFromSlot(gAdpd400x_lcfg->targetSlots);
    ledCurrent *= gAdpd400x_lcfg->targetDcPercent;
    ledCurrent /= 100;  // Adjust to valid range

    Adpd400xUtilGetCurrentRegValue(ledCurrent, &ledReg12, &ledReg34);
    AdpdDrvRegWrite(g_reg_base + ADPD400x_REG_LED_POW12_A, ledReg12);
    AdpdDrvRegWrite(g_reg_base + ADPD400x_REG_LED_POW34_A, ledReg34);

    gAdpd400xOptmVal.ledB_Cur = ledReg12;
    gAdpd400xOptmVal.ledB_Trim = ledReg34;
    gAdpd400xOptmVal.ledB_CurVal = Adpd400xUtilGetCurrentFromReg(ledReg12, ledReg34);
    //debug(MODULE, "New Current=%d, reg=0x%04x, 0x%04x", \
          ledCurrent, ledReg12, ledReg34);
	NRF_LOG_DEBUG("New Current=%d, reg=0x%04x, 0x%04x \r\n",ledCurrent, ledReg12, ledReg34);
  }
}

/**
  * @brief  Fine pulses, sampling rate, LED current
  * @param  None.
  * @retval None.
  */
static void Op_FindPower_108(void) {
#if 0
  *gsLedPower = (uint16_t) (gAdpd400x_lcfg->ledG_Voltage) * (gAdpd400xOptmVal.ledB_CurVal);
#endif
  *gsAfePower = (uint16_t) (ADPD_VOL * 1234);
  *gsTotalPower = (*gsLedPower) + (*gsAfePower);
}
