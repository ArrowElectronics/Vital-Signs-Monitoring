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
#include <math.h>
#include "adpd400x_lib.h"
#include "adpd400x_lib_common.h"

#define MODULE ("DetectObject.c")

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

void Adpd400xDetectObjectOnInitGreen(void) {}
void Adpd400xDetectObjectOnDeInitGreen() {}
INT_ERROR_CODE_t Adpd400xDetectObjectOnGreen(uint32_t *rawDataB) {return (INT_ERROR_CODE_t)0;}
#if 0


#define SKIP_NUM        2   // Skip the first SKIP_NUM data for averaging
#define AVG_SIZE        8   // size of data to do averaging

/* Public function prototypes -----------------------------------------------*/
void DetectObjectOnInitGreen(void);
void DetectObjectOnDeInitGreen();
INT_ERROR_CODE_t DetectObjectOnGreen(uint32_t *rawDataB);

/* Public or external variables ---------------------------------------------*/
extern AdpdLibConfig_t *gAdpd400x_lcfg;

/* Private function prototypes ----------------------------------------------*/
/* Private Variables --------------------------------------------------------*/
static struct Register {
  uint16_t x23;
  uint16_t x25;
  uint16_t x36;
  uint16_t x34;
  uint16_t x44;
}Reg;

static uint16_t gsSampleRate, gsDecimation;
static uint8_t gsSkipCnt;
static uint8_t gsOccurCounter;
static uint8_t gsDetectState;
static uint32_t gsDetectStartTime;

/**
  * @brief Initializes the Detect Object routine for calling
  *        DetectObjectOn.
  * @retval none
  */
void DetectObjectOnInitGreen() {
  AdpdDrvRegRead(REG_LED1_DRV, &Reg.x23);
  AdpdDrvRegRead(REG_LED_TRIM, &Reg.x25);
  AdpdDrvRegRead(REG_PULSE_PERIOD_B, &Reg.x36);
  AdpdDrvRegRead(REG_PULSE_MASK, &Reg.x34);
  AdpdDrvRegRead(REG_AFE_TRIM_B, &Reg.x44);
  PpgLibGetSampleRate(&gsSampleRate, &gsDecimation,gAdpd400x_lcfg->targetSlots);  // backup sampling rate

  debug(MODULE, "Air Level= %d\t Trigger Level= %d\t Stable=%u", \
      gAdpd400x_lcfg->triggerOnAirLevel, gAdpd400x_lcfg->triggerOnLevel, gAdpd400x_lcfg->triggerOnStablizeVR);

  gsDetectState = 0;
  gsSkipCnt = 0;
  gsDetectStartTime = AdpdMwLibGetCurrentTime();
}

/**
  * @brief DeInitializes the Detect Object routine for calling
  *        DetectObjectOn.
  * @retval none
  */
void DetectObjectOnDeInitGreen() {
  AdpdMwLibSetMode(ADPDDrv_MODE_IDLE, ADPDDrv_SLOT_OFF, ADPDDrv_SLOT_OFF);
  PpgLibSetSampleRate(gsSampleRate, gsDecimation,gAdpd400x_lcfg->targetSlots);    // restore sampling rate
  AdpdDrvRegWrite(REG_LED1_DRV, Reg.x23);
  AdpdDrvRegWrite(REG_LED_TRIM, Reg.x25);
  AdpdDrvRegWrite(REG_PULSE_PERIOD_B, Reg.x36);
  AdpdDrvRegWrite(REG_PULSE_MASK, Reg.x34);
  AdpdDrvRegWrite(REG_AFE_TRIM_B, Reg.x44);
  AdpdMwLibSetMode(ADPDDrv_MODE_SAMPLE, ADPDDrv_SLOT_OFF, ADPDDrv_SUM_32);
}

/**
  * @internal
  * @brief Check if object is detected using the Green LED
  * @param rawDataB : slotB channel rawData
  * @retval IN_PROGRESS=checking, SUCCESS=detected, FAIL=Not detected
  */
INT_ERROR_CODE_t DetectObjectOnGreen(uint32_t *rawDataB) {
  uint32_t mean_val2;
  uint32_t var2;
  uint32_t detectCurrentTime;

  detectCurrentTime = AdpdMwLibGetCurrentTime();
  if (AdpdMwLibDeltaTime(gsDetectStartTime, detectCurrentTime) >
      (uint32_t)(gAdpd400x_lcfg->detectOntimeout * 1000)) {
    return IERR_TIMEOUT;
  }

  if (gsSkipCnt > 0)  {
    gsSkipCnt--;
    return IERR_IN_PROGRESS;
  }

  if (gsDetectState == 0) {
    AdpdMwLibSetMode(ADPDDrv_MODE_IDLE, ADPDDrv_SLOT_OFF, ADPDDrv_SLOT_OFF);
    PpgLibSetSampleRate((gAdpd400x_lcfg->detectionRate)>>16, \
                        (gAdpd400x_lcfg->detectionRate)&0xFFFF,gAdpd400x_lcfg->targetSlots);
    AdpdDrvRegWrite(REG_PULSE_MASK, 0x0100);              // Turn on Gr LED
    AdpdDrvRegWrite(REG_LED1_DRV, (Reg.x23&0xFFF0)|0x1);  // change to 40mA
    AdpdDrvRegWrite(REG_LED_TRIM, 0xC);                   // set to default
    AdpdDrvRegWrite(REG_PULSE_PERIOD_B, Reg.x36&0x00FF | 0x100);  // 1 pul
    AdpdDrvRegWrite(REG_AFE_TRIM_B, Reg.x44&0xFFFC | 0x01);       // 100k
    AdpdMwLibSetMode(ADPDDrv_MODE_SAMPLE, ADPDDrv_SLOT_OFF, ADPDDrv_SUM_32);
    gsDetectState = 1;
    return IERR_IN_PROGRESS;
  }

  if (gsDetectState == 1) {
    Adpd400xUtilGetMeanVarInit(AVG_SIZE, 1);    // average size, get variance
    gsOccurCounter = 0;
    gsDetectState = 2;
    gsSkipCnt = SKIP_NUM;
    return IERR_IN_PROGRESS;
  }

  if (gsDetectState == 2) {
    //  0=processing 1=with result 2=invalid
    if (Adpd400xUtilGetMeanVar(rawDataB, &mean_val2, &var2) == IERR_IN_PROGRESS)
      return IERR_IN_PROGRESS;
    if (var2 > gAdpd400x_lcfg->triggerOnStablizeVR) {   // should be sqrt3 of vr
      // not stable
      gsDetectState = 1;

      debug(MODULE, "!Device Unstable %04x %04x %04x %04x, Var=%08x!", \
                rawDataB[0], rawDataB[1], rawDataB[2], rawDataB[3], var2);
      return IERR_DEVICE_ON_MOTION;
    }
    debug(MODULE, "Detected ON Level=%u\r\n", mean_val2);
    gDetectVal.detectOnValue = mean_val2;
    if (gDetectVal.detectOnValue < (uint32_t)(gAdpd400x_lcfg->triggerOnLevel +
        gAdpd400x_lcfg->triggerOnAirLevel)) {
      gsDetectState = 1;
      return IERR_OFF_SENSOR;
    }
    gsDetectState = 0xFF;
    Adpd400xUtilGetMeanVarInit(AVG_SIZE, 1);
    return IERR_IN_PROGRESS;
  }

  if (Adpd400xUtilGetMeanVar(rawDataB, &mean_val2, &var2) == IERR_IN_PROGRESS)
    return IERR_IN_PROGRESS;
  Adpd400xUtilGetMeanVarInit(AVG_SIZE, 1);     // for next time

  gDetectVal.detectOnVariance = var2;

  debug(MODULE, "%04x %04x %04x %04x, mean=%u Var=%u", rawDataB[0], \
        rawDataB[1], rawDataB[2], rawDataB[3], \
        mean_val2, gDetectVal.detectOnVariance);
  if (mean_val2 < (uint32_t)(gAdpd400x_lcfg->triggerOnLevel +
                               gAdpd400x_lcfg->triggerOnAirLevel))  {
    gsDetectState = 1;
    return IERR_OFF_SENSOR;
  }

  if (var2 > gAdpd400x_lcfg->triggerOnStablizeVR) {  // Not stable
    gsDetectState = 1;
    return IERR_DEVICE_ON_MOTION;
  }

  if (++gsOccurCounter >= gAdpd400x_lcfg->detectOnSettlingCnt) {
    return IERR_SUCCESS;
  }

  return IERR_IN_PROGRESS;
}

#endif
