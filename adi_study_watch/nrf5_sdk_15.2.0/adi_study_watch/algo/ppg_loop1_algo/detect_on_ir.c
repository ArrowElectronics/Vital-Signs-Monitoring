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
#if 0
#include <stdio.h>
#include <time.h>
//#include "adpd_lib.h"
#include "adpd_lib_common.h"

#define MODULE ("DetectObject.c")

#define SKIP_NUM            2       // must less than 50
#define AVG_SIZE            8       // size of data to do averaging

/* Public function prototypes -----------------------------------------------*/
void Adpd400xDetectObjectOnInitIR(void);
void Adpd400xDetectObjectOnDeInitIR(void);
INT_ERROR_CODE_t DetectObjectOnIR(uint32_t *rawDataB);

/* Public or external variables ---------------------------------------------*/
extern AdpdLibConfig_t *gAdpd400x_lcfg;

/* Private function prototypes ----------------------------------------------*/
/* Private Variables --------------------------------------------------------*/
static struct Register {
  uint16_t x24;
  uint16_t x25;
  uint16_t x31;
  uint16_t x34;
  uint16_t x42;
}Reg;

static uint16_t gsSampleRate, gsDecimation;
static uint8_t gsOccurCounter, gsSkipCnt;
static uint32_t gsLevelThresholdON;    // reference ON/OFF TH
static uint32_t gsDetectStartTime;
static uint8_t gsDetectState;
static uint32_t gsDetectDataA1, gsDetectDataA2;

/**
  * @internal
  * @brief Object detect Init routine
  * @param Number of sample points for averaging
  * @retval None
  */
void Adpd400xDetectObjectOnInitIR() {
  PpgLibGetSampleRate(&gsSampleRate, &gsDecimation,gAdpd400x_lcfg->targetSlots);  // backup sampling rate
  AdpdDrvRegRead(REG_LED2_DRV, &Reg.x24);
  AdpdDrvRegRead(REG_LED_TRIM, &Reg.x25);
  AdpdDrvRegRead(REG_PULSE_PERIOD_A, &Reg.x31);
  AdpdDrvRegRead(REG_PULSE_MASK, &Reg.x34);
  AdpdDrvRegRead(REG_AFE_TRIM_A, &Reg.x42);

  debug(MODULE, "Trigger On Level= %d\t Stable=%u\n",
      gAdpd400x_lcfg->triggerOnLevel, gAdpd400x_lcfg->triggerOnStablizeVR);
  gsDetectState = 0;
  gsSkipCnt = 0;
  gsDetectStartTime = AdpdMwLibGetCurrentTime();
}

/**
  * @internal
  * @brief Object detect DeInit routine
  * @param None
  * @retval None
  */
void Adpd400xDetectObjectOnDeInitIR() {
  AdpdMwLibSetMode(ADPDDrv_MODE_IDLE, ADPDDrv_SLOT_OFF, ADPDDrv_SLOT_OFF);
  PpgLibSetSampleRate(gsSampleRate, gsDecimation,gAdpd400x_lcfg->targetSlots);    // restore sampling rate
  AdpdDrvRegWrite(REG_LED2_DRV, Reg.x24);
  AdpdDrvRegWrite(REG_LED_TRIM, Reg.x25);
  AdpdDrvRegWrite(REG_PULSE_PERIOD_A, Reg.x31);
  AdpdDrvRegWrite(REG_PULSE_MASK, Reg.x34);
  AdpdDrvRegWrite(REG_AFE_TRIM_A, Reg.x42);
  AdpdMwLibSetMode(ADPDDrv_MODE_SAMPLE, ADPDDrv_SUM_32 , ADPDDrv_SLOT_OFF);
}

/**
  * @internal
  * @brief Check if object is detected
  * @param rawDataB: 4 channel rawData
  * @retval IN_PROGRESS = checking, SUCCESS = detected, FAIL = Not detected
  */
INT_ERROR_CODE_t Adpd400xDetectObjectOnIR(uint32_t *rawDataB) {
  uint32_t mean_val1, var1, detectCurrentTime;

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
    AdpdDrvRegWrite(REG_PULSE_MASK, 0x0200);              // Turn on IR LED
    AdpdDrvRegWrite(REG_LED2_DRV, (Reg.x24&0xFFF0)|0x3);  // change to 70mA
    AdpdDrvRegWrite(REG_LED_TRIM, 0x630C);                // set to default
    AdpdDrvRegWrite(REG_PULSE_PERIOD_A, (Reg.x31&0x00FF)|0x100);  // 1 pul
    AdpdDrvRegWrite(REG_AFE_TRIM_B, (Reg.x42&0xFFFC)| 0x3);       // 25k
    AdpdMwLibSetMode(ADPDDrv_MODE_SAMPLE, ADPDDrv_SUM_32, ADPDDrv_SLOT_OFF);
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
    if (Adpd400xUtilGetMeanVar(rawDataB, &mean_val1, &var1) == IERR_IN_PROGRESS)
      return IERR_IN_PROGRESS;
    if (var1 > gAdpd400x_lcfg->triggerOnStablizeVR) {   // should be sqrt3 of vr
      // not stable
      gsDetectState = 1;

      debug(MODULE, "!Device Unstable %04x %04x %04x %04x, Var=%08x!", \
            rawDataB[0], rawDataB[1], rawDataB[2], rawDataB[3], var1);
      return IERR_DEVICE_ON_MOTION;
    }
    gsDetectDataA1 = mean_val1;
    AdpdMwLibSetMode(ADPDDrv_MODE_IDLE, ADPDDrv_SLOT_OFF, ADPDDrv_SLOT_OFF);
    AdpdDrvRegWrite(REG_LED2_DRV, (Reg.x24&0xFFF0)|0x0);  // change to 25mA
    AdpdMwLibSetMode(ADPDDrv_MODE_SAMPLE, ADPDDrv_SUM_32, ADPDDrv_SLOT_OFF);
    gsDetectState = 51;
    gsSkipCnt = SKIP_NUM;
    Adpd400xUtilGetMeanVarInit(AVG_SIZE, 1);    // average size, get variance
    return IERR_IN_PROGRESS;
  }

  if (gsDetectState == 51) {
    if (Adpd400xUtilGetMeanVar(rawDataB, &mean_val1, &var1) == IERR_IN_PROGRESS)
      return IERR_IN_PROGRESS;
    if (var1 > gAdpd400x_lcfg->triggerOnStablizeVR) {     // should be sqrt3 of vr
      // not stable
      gsDetectState = 1;

      debug(MODULE, "Not Stable %04x %04x %04x %04x, CurVar=%08x", \
                rawDataB[0], rawDataB[1], rawDataB[2], rawDataB[3], var1);
      return IERR_DEVICE_ON_MOTION;
    }
    gsLevelThresholdON = mean_val1;
    gsDetectDataA2 = mean_val1;
    if (gsDetectDataA1 < gsDetectDataA2) {
      debug(MODULE, "ATTN1!! A1=%04x A2=%04x", \
            gsDetectDataA1, gsDetectDataA2);
      gsDetectDataA1 = 0;
    } else {
      gsDetectDataA1 = (gsDetectDataA1-gsDetectDataA2)/45;
    }

    debug(MODULE, "DetectON Level=%u, DC Value=%u", \
          gsDetectDataA1, gsLevelThresholdON);

    gDetectVal.detectOnValue = gsDetectDataA1;
    if (gDetectVal.detectOnValue < gAdpd400x_lcfg->triggerOnLevel) {  // slop compare fail
      gsDetectState = 1;
      return IERR_OFF_SENSOR;
    }
    gsDetectState = 0xFF;
    Adpd400xUtilGetMeanVarInit(AVG_SIZE, 1);    // average size, get variance
    return IERR_IN_PROGRESS;
  }

  if (Adpd400xUtilGetMeanVar(rawDataB, &mean_val1, &var1) == IERR_IN_PROGRESS)
    return IERR_IN_PROGRESS;

  Adpd400xUtilGetMeanVarInit(AVG_SIZE, 1);    // average size, get variance
  gDetectVal.detectOnVariance = var1;

  debug(MODULE, "%04x %04x %04x %04x, CurVar=%08x", \
      rawDataB[0], rawDataB[1], rawDataB[2], rawDataB[3], gDetectVal.detectOnVariance);


  if (var1 == 0)  {                          // Saturated
    gsDetectState = 1;
    return IERR_SATURATED;
  }
  if (var1 > gAdpd400x_lcfg->triggerOnStablizeVR) {   // Not stable
    gsDetectState = 1;
    return IERR_DEVICE_ON_MOTION;
  }
  if (++gsOccurCounter >= gAdpd400x_lcfg->detectOnSettlingCnt) {
    return IERR_SUCCESS;
  }
  return IERR_IN_PROGRESS;
}
#endif