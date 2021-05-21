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
#include <stdio.h>
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

#define MODULE ("GetCTR.c")

#define BPF_GAIN2           797     // scale up by 1k
#define BPF_GAIN3           750     // Band pass filter gain with 3uS LED
#define SKIP_NUM            3       // skip the first few data for process
#define PULSE_NUM           4
#define A_SATURATE          0x1F00*PULSE_NUM  // should less than 0x2000/pulse
#define CURRENT_USE         25      // use this current to get ctr
#define LED_DEGRADATION     0.00156

/* Public function prototypes -----------------------------------------------*/
void Adpd400xGetCtrInit(void);
void Adpd400xGetCtrDeInit(void);
INT_ERROR_CODE_t Adpd400xGetCtrValue(uint32_t *rawDataB);

/* Public or external variables ---------------------------------------------*/
extern uint16_t g_Adpd400xTiaG_Init;

/* Private function prototypes ----------------------------------------------*/
/* Private Variables --------------------------------------------------------*/

static struct Register {
  uint16_t x104;    // tia
  uint16_t x105;    // LED12
  uint16_t x106;    // LED34
  uint16_t x107;    // LED pulses
  uint16_t x109;    // LED width
  uint16_t x10A;    // AFE width
}Reg;

// static uint16_t gsSampleRate, gsDecimation;
static uint8_t gsOperateState;
static uint8_t gsSkipCnt;
static uint16_t gsTiaGain;
static uint16_t gsLedCurrentB;
static uint16_t gsLedWidthB;
static uint16_t gsPreDataVal;

/**
  * @internal
  * @brief Get CTR Init routine
  * @param none
  * @retval None
  */
void Adpd400xGetCtrInit() {
  // PpgLibGetSampleRate(&gsSampleRate, &gsDecimation);  // backup sampling rate
  g_reg_base = log2(gAdpd400x_lcfg->targetSlots) * 0x20;
  AdpdDrvRegRead(g_reg_base + ADPD400x_REG_AFE_TRIM_A, &Reg.x104);
  AdpdDrvRegRead(g_reg_base + ADPD400x_REG_LED_POW12_A, &Reg.x105);
  AdpdDrvRegRead(g_reg_base + ADPD400x_REG_LED_POW34_A, &Reg.x106);
  AdpdDrvRegRead(g_reg_base + ADPD400x_REG_COUNTS_A, &Reg.x107);
  AdpdDrvRegRead(g_reg_base + ADPD400x_REG_LED_PULSE_A, &Reg.x109);
  AdpdDrvRegRead(g_reg_base + ADPD400x_REG_INTEG_WIDTH_A, &Reg.x10A);
  gsTiaGain = Reg.x104 & 0xFFF8;   // 200k
  gsLedWidthB = Reg.x109 >> 8;
  // gsAfeWidthB = Reg.x10A & 0x1F;
  gsOperateState = 0;
  gsSkipCnt = 0;
}

/**
  * @internal
  * @brief Get CTR DeInit routine
  * @param None
  * @retval None
  */
void Adpd400xGetCtrDeInit() {
  // AdpdMwLibSetMode(ADPDDrv_MODE_IDLE, ADPDDrv_SLOT_OFF, ADPDDrv_SLOT_OFF);

  // PpgLibSetSampleRate(gsSampleRate, gsDecimation);    // restore sampling rate
  g_reg_base = log2(gAdpd400x_lcfg->targetSlots) * 0x20;
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
  * @internal
  * @brief Get CTR value
  * @param 4 channel rawData
  * @retval IN_PROGRESS=in progress, SUCCESS=done, FAIL=error
  */
INT_ERROR_CODE_t Adpd400xGetCtrValue(uint32_t *rawDataB) {
  uint32_t dataVal, tempVal32, temp32;
  uint16_t afeGainNum, bpfGainNum, temp16;
  uint16_t led12, led34;
  g_reg_base = log2(gAdpd400x_lcfg->targetSlots) * 0x20;

  if (gsSkipCnt > 0)  {
    gsSkipCnt--;
    return IERR_IN_PROGRESS;
  }
  if (gsOperateState == 0) {
    AdpdMwLibSetMode(ADPD400xDrv_MODE_IDLE, ADPD400xDrv_SIZE_0, ADPD400xDrv_SIZE_0);

    // PpgLibSetSampleRate(800, 32);                // use 800/32=25Hz
    gsLedCurrentB = CURRENT_USE;                    // use 50mA
    Adpd400xUtilGetCurrentRegValue(gsLedCurrentB, &led12, &led34);
    AdpdDrvRegWrite(g_reg_base + ADPD400x_REG_LED_POW12_A, led12);
    AdpdDrvRegWrite(g_reg_base + ADPD400x_REG_LED_POW34_A, led34);

    temp16 = 0x100 + PULSE_NUM;
    AdpdDrvRegWrite(g_reg_base + ADPD400x_REG_COUNTS_A, temp16);   // 4 pulse

    gsTiaGain = Reg.x104&0xFFC0;                    // 200k
    AdpdDrvRegWrite(g_reg_base + ADPD400x_REG_AFE_TRIM_A, gsTiaGain);

    gsOperateState = 1;
  }

  if (gsOperateState == 1) {
    AdpdMwLibSetMode(ADPD400xDrv_MODE_IDLE, ADPD400xDrv_SIZE_0, ADPD400xDrv_SIZE_0);
    AdpdDrvRegWrite(g_reg_base + ADPD400x_REG_AFE_TRIM_A, gsTiaGain);
    AdpdMwLibSetMode(ADPD400xDrv_MODE_SAMPLE,
                   ADPD400xDrv_SIZE_0,
                   ADPD400xDrv_SIZE_32);
    gsOperateState = 2;
    gsSkipCnt = SKIP_NUM;
    return IERR_IN_PROGRESS;
  }

  if (gsOperateState == 2) {
    dataVal = rawDataB[0];
    gsPreDataVal = dataVal;

    if (dataVal < A_SATURATE)  {   // First check is not staturated
      gsOperateState = 3;
    }  // else stay in this state
    gsTiaGain++;
    if ((gsTiaGain & 0x3) == 0x3)  { // 25K, should not saturated with 25mA
      gsOperateState = 10;
    }

    AdpdMwLibSetMode(ADPD400xDrv_MODE_IDLE, ADPD400xDrv_SIZE_0, ADPD400xDrv_SIZE_0);
    AdpdDrvRegWrite(g_reg_base + ADPD400x_REG_AFE_TRIM_A, gsTiaGain);
    AdpdMwLibSetMode(ADPD400xDrv_MODE_SAMPLE,
                   ADPD400xDrv_SIZE_0,
                   ADPD400xDrv_SIZE_32);
    gsSkipCnt = SKIP_NUM;
    return IERR_IN_PROGRESS;
  }

  if (gsOperateState == 3) {  // Check Saturation
    dataVal = rawDataB[0];
    temp32 = (uint32_t)(dataVal*1.8); // should ~= 2x of the previous tia
    if (dataVal >= A_SATURATE || temp32 > gsPreDataVal) {
      gsPreDataVal = dataVal;
      gsTiaGain++;            // gain to half. Bit[1:0] is the gain setting
      if ((gsTiaGain & 0x3) == 0)  {
        debug(MODULE, "getCTR Saturated with 25k");
        return IERR_FAIL;     // saturated in 25K
      }
      AdpdMwLibSetMode(ADPD400xDrv_MODE_IDLE, ADPD400xDrv_SIZE_0, ADPD400xDrv_SIZE_0);
      AdpdDrvRegWrite(g_reg_base + ADPD400x_REG_AFE_TRIM_A, gsTiaGain);
      AdpdMwLibSetMode(ADPDDrv_MODE_SAMPLE,
                   ADPD400xDrv_SIZE_0,
                   ADPD400xDrv_SIZE_32);
      return IERR_IN_PROGRESS;
    } else {
      g_Adpd400xTiaG_Init = gsTiaGain;      // proper gain value would not saturate
      gsOperateState = 10;
    }
  }

  if (gsOperateState == 10) {
    //  0=processing 1=with result 2=invalid
    if (gsLedWidthB == 2)
      bpfGainNum = BPF_GAIN2;
    else
      bpfGainNum = BPF_GAIN3;
    afeGainNum = 1 << (gsTiaGain&0x3);         // R4/Rf=200/x.
    // should x2 here for R5. Double dynamic range
    afeGainNum *= 2;                // R4/Rf=400/x.
    // Effective LED current = I*(1-degradation(~0.00156)*I)
    temp16 = 24;

    // Equation: dominator = bpfGainNum*pulseWidth*LED_Current*PULSE_NUM;
    tempVal32 = bpfGainNum*gsLedWidthB*temp16*PULSE_NUM;  // afeGainNum is inverted
    // should use sum of all ch to calculate ctr.
    dataVal = rawDataB[0];
    temp32 = dataVal*460*afeGainNum;
    // Equation: g_CtrValue = dataVal*460*afeGainNum/tempVal32;
    gAdpd400xPPGLibStatus.CtrValue = (uint16_t)(temp32/tempVal32);
    //debug(MODULE, "CTR=%d, mean=%u\r\n", gAdpd400xPPGLibStatus.CtrValue, tempVal32);
    //printf("CTR=%u \r\n", gAdpd400xPPGLibStatus.CtrValue);
    NRF_LOG_DEBUG("CTR Pass\n");
    NRF_LOG_DEBUG("*******************CTR=%u **********\r\n", gAdpd400xPPGLibStatus.CtrValue);

    return IERR_SUCCESS;
  }
  return IERR_IN_PROGRESS;
}

