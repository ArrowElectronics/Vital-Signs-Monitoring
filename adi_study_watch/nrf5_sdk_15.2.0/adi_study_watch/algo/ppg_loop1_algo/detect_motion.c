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
#include <time.h>
#include "adpd400x_lib.h"
#include "adpd400x_lib_common.h"
#include "signal_metrics.h"

#ifndef ABS
#define ABS(i_x) (((i_x) > 0) ? (i_x): -(i_x))
#endif

#define MODULE ("DetectMotion.c")

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

/* Public function prototypes -----------------------------------------------*/
void Adpd400xMDCheckMotionInit();
INT_ERROR_CODE_t Adpd400xMDInstantCheck(int16_t *acceldata, uint8_t*);
INT_ERROR_CODE_t Adpd400xMDDurationCheck();
void Adpd400xMDPeriodicCheckStart(void);
void Adpd400xMDResetTimers(void);

/* Private function prototypes ----------------------------------------------*/
/* Private Variables --------------------------------------------------------*/
static uint32_t gsRestStartTime, gsPeriodicStartTime;
static uint8_t gsRestStartTimeExpired, gsPeriodicTimeExpired;

void Adpd400xMDCheckMotionInit() {
  gsRestStartTime = Adpd400xMwLibGetCurrentTime();
  gsPeriodicStartTime = gsRestStartTime;
  gsPeriodicTimeExpired = 1;

  Preprocess_Activity_Init(SAMPLE_RATE);
}

/**
  * @brief Check if motion is happening
  * @param Adxl data
  * @retval Success = rest, Fail = motion
  */
INT_ERROR_CODE_t Adpd400xMDInstantCheck(int16_t *acceldata, uint8_t* motionLevel)  {
  int32_t accelx, accely, accelz;
  int32_t activity;

  accelx = acceldata[0];
  accely = acceldata[1];
  accelz = acceldata[2];

  accelx *= gAdpd400x_lcfg->accelscale;
  accely *= gAdpd400x_lcfg->accelscale;
  accelz *= gAdpd400x_lcfg->accelscale;
  // get result from algorithm as well. motion = getCostFunction();
  // activity_function. 0x19999=104857 -> 104857/2^20 = 0.10
  Preprocess_Activity_Run(accelx,
                          accely,
                          accelz,
                          &activity);

  *motionLevel = 0;
  if (activity > (int)(gAdpd400x_lcfg->motionThresholdHigh)) {
    gsRestStartTime = Adpd400xMwLibGetCurrentTime();
    gsRestStartTimeExpired = 0;
    //debug(MODULE, "!!High Motion=%d", activity);
	NRF_LOG_INFO("!!High Motion=%d", activity);
    *motionLevel = HIGH_MOTION;
    return IERR_FAIL;
  }
  if (activity > (int)(gAdpd400x_lcfg->motionThreshold)) {
    gsRestStartTime = Adpd400xMwLibGetCurrentTime();
    gsRestStartTimeExpired = 0;
    //debug(MODULE, "!!Motion=%d", activity);
	NRF_LOG_INFO("!!Motion=%d", activity);
    *motionLevel = LOW_MOTION;
    return IERR_FAIL;
  }
  *motionLevel = NO_MOTION;
  return IERR_SUCCESS;
}

/**
  * @brief Check if motion happens in last M second druing the N second period
  * @param none
  * @retval Success = rest, Fail = motion
  */
INT_ERROR_CODE_t Adpd400xMDDurationCheck()  {
  uint32_t curTime = Adpd400xMwLibGetCurrentTime();
  uint32_t temp16;

  if (gsPeriodicTimeExpired)  {
    if (gsRestStartTimeExpired)  {
      return IERR_SUCCESS;
    } else {
      // check if rested for M second after priodic N second
      temp16 = (gAdpd400x_lcfg->motionCheckPeriod&0xFFFF)*1000;
      if (AdpdMwLibDeltaTime(gsRestStartTime, curTime) > temp16)  {
        gsRestStartTimeExpired = 1;
        gsRestStartTime = curTime;
        //debug(MODULE, "AGC time start");
		NRF_LOG_DEBUG("AGC time start");
      }
      return IERR_FAIL;
    }
  } else {
    // Ok, time for motion checking (Every N second)
    temp16 = (gAdpd400x_lcfg->motionCheckPeriod>>16)*1000;
    if (AdpdMwLibDeltaTime(gsPeriodicStartTime, curTime) > temp16) {
      gsPeriodicStartTime = curTime;
      gsPeriodicTimeExpired = 1;
      gsRestStartTime = curTime;
    }
    return IERR_FAIL;
  }

}

void Adpd400xMDResetTimers(){
  uint32_t curTime = Adpd400xMwLibGetCurrentTime();
  gsRestStartTime = curTime;
  gsPeriodicStartTime = curTime;
  gsRestStartTimeExpired = 0;
  gsPeriodicTimeExpired = 0;
}

void Adpd400xMDPeriodicCheckStart() {
  gsPeriodicTimeExpired = 0;
  gsRestStartTimeExpired = 0;
}
