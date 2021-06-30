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
* This software is intended for use with the ADPD and derivative parts     *
* only                                                                        *
*                                                                             *
******************************************************************************/
#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "adpd400x_lib.h"
#include "adpd400x_lib_common.h"
#include "heart_rate_internal.h"
#include "timers.h"

#define MODULE ("HeartRate.c")

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

/************************************************************
 Macros
 ************************************************************/

#define CIRBUFF (32)
#define HISTDEPTH (4)  /* DO NOT CHANGE!!!! */

/************************************************************
 Type Definitions
 ************************************************************/
typedef enum {NOTSTARTED, STARTED} HRState_t;
typedef struct _prev_sensorInfo_t {
  uint32_t slotBCh;
  int16_t acc[3];
  int32_t timeStamp;
} Prev_sensorInfo_t;

/************************************************************
 External and Pulbice Variables
 ************************************************************/
uint16_t gAdpd400x_dataRateRatio;
HR_Log_Result_t gAdpd400x_resultlog;
extern Adpd400xLibConfig_t *gAdpd400x_lcfg;

/************************************************************
 Internal Variables
 ************************************************************/
static HRState_t HrStage = NOTSTARTED;
static uint8_t gsFrontEndReset;
static Prev_sensorInfo_t gsPrevRawData;

/************************************************************
 Function Prototypes
 ************************************************************/
static uint8_t measureDataRate(uint16_t *datarate, int32_t timeStamp);

/************************************************************
 External Function Prototypes
 ************************************************************/
extern int16_t Adpd400xAlgHRInit();

extern INT_ERROR_CODE_t AlgHRPostProcess(LibResult_t *result,
                                         HR_Log_Result_t *resultlog,
                                         int32_t status);
extern void AdpdLibSetTriggerLevel(uint32_t in);
/************************************************************
 Functions
 ************************************************************/

/**
  * @brief Initializes the heart rate wrapper routine
  * @retval SUCCESS = Initialization successful
  */
INT_ERROR_CODE_t Adpd400xheartRateInit() {
    HrStage = NOTSTARTED;
    memset(&gAdpd400x_resultlog, 0, sizeof(gAdpd400x_resultlog));
    return IERR_SUCCESS;
}

/**
  * @brief Wrapper function to do algorithm initialization, measure the data rate
  *                slippage from set value and call the algorithm processing and
  *                post-processing functions.
  * @param slotBCh pointer to an array of uint32_t that contains the slotB
  *                input data for algorithm processing
  * @param acceldata pointer to an array of int16_t that contains the data
  *                from the 3 axes of acclerometer for algorithm processing
  * @param timeStamp not used currently
  * @param timeStampADPD timestamp of ADPD sample used for calculating data rate
  * @param result structure holding results of library processing
  * @retval  indicating success in processing the raw data or
  *          if the algorithm is in progress processing the data
  */
#pragma diag_suppress = Pe826
INT_ERROR_CODE_t Adpd400xheartRate(uint32_t *slotBCh,
                           int16_t *acceldata,
                           int32_t timeStamp,
                           int32_t timeStampADPD,
                           LibResult_t *result)  {
    uint32_t levelSlotB;
    uint32_t levelSlotA;
    uint32_t hrStatus = 0;
    uint32_t tempU32;
    uint16_t dataRxInterval;
    uint32_t currtimediff;
    int16_t numSamplesMissing;
    int16_t accDelta[3];
    int16_t simacceldata[3];
    int16_t i;

#ifndef NDEBUG
    #pragma diag_suppress = Pe550
#endif
    if (result == 0) {
        debug(MODULE, "Unexpected null error");
        return (INT_ERROR_CODE_t)IERR_NULL_ERROR;
    }

    /* Initialization */
    if (HrStage == NOTSTARTED) {
        gAdpd400x_dataRateRatio = 1024;     // default to 1024 means ratio is 1.

        gAdpd400x_resultlog.StepCount=0;
        gAdpd400x_resultlog.RRinterval=0;

        if (Adpd400xAlgHRInit() != 0)
            return IERR_FAIL;
        measureDataRate(0, timeStampADPD);  // Init
        HrStage = STARTED;
        // upTime = timeStamp;
        // CbIndex = 0;
    }

    /* Measure data rate for linear scaling */
    if (gAdpd400x_lcfg->drTime == 0) {
      gAdpd400x_dataRateRatio = 1024;   // no compensation
    } else if (measureDataRate(&dataRxInterval, timeStampADPD) == 0) {
        tempU32 = 1024000000/gAdpd400x_lcfg->hrmInputRate;  // interval shifted by 10 in uS
        gAdpd400x_dataRateRatio = tempU32 / dataRxInterval;
        // debug(MODULE, "Ratio = %d", g_dataRateRatio);
		NRF_LOG_DEBUG("Ratio = %d", gAdpd400x_dataRateRatio);
    }

    levelSlotA = 0;
    //levelSlotB = slotBCh[0] + slotBCh[1] + slotBCh[2] + slotBCh[3];
    levelSlotB = slotBCh[0];

    // linear intepolation data for missing samples
    if (gsFrontEndReset == 1) { // make sure there's previous data saved
      gsFrontEndReset = 0;
      // get number of missing samples
#ifndef STM32F405xx
      currtimediff = (timeStampADPD - gsPrevRawData.timeStamp) >> 5;
#else
      currtimediff = (timeStampADPD - gsPrevRawData.timeStamp);
#endif
      currtimediff += 500 / gAdpd400x_lcfg->hrmInputRate;
      numSamplesMissing = currtimediff * gAdpd400x_lcfg->hrmInputRate / 1000 - 1;
      gAdpd400xAGCStatInfo.setting[7] = numSamplesMissing;
      debug(MODULE, "Missing=%d, newTs=%d, oldTs=%d",
            numSamplesMissing, timeStampADPD, gsPrevRawData.timeStamp);
      if (numSamplesMissing > 0) {  // missing samples
        accDelta[0] = (acceldata[0] - gsPrevRawData.acc[0])/numSamplesMissing;
        accDelta[1] = (acceldata[1] - gsPrevRawData.acc[1])/numSamplesMissing;
        accDelta[2] = (acceldata[2] - gsPrevRawData.acc[2])/numSamplesMissing;
        // simSlotB = gsPrevRawData.slotBCh;
      }

      // linear intepolation for adxl data
      for( i = 0; i < numSamplesMissing; i++ ) {
        simacceldata[0] = accDelta[0] + gsPrevRawData.acc[0];
        simacceldata[1] = accDelta[1] + gsPrevRawData.acc[1];
        simacceldata[2] = accDelta[2] + gsPrevRawData.acc[2];

        gsPrevRawData.acc[0] = simacceldata[0];
        gsPrevRawData.acc[1] = simacceldata[1];
        gsPrevRawData.acc[2] = simacceldata[2];

        hrStatus = Adpd400xAlgHRProcess(levelSlotA,
                                levelSlotB,
                                simacceldata,
                                result,
                                &gAdpd400x_resultlog);
        if (hrStatus != ADPDLIB_ERR_SUCCESS_WITH_RESULT)
          return (INT_ERROR_CODE_t)hrStatus;
      }
      // debug(MODULE, "%d Missing Samples were Linearly Interpolated", numSamplesMissing);
	  NRF_LOG_DEBUG("%d Missing Samples were Linearly Interpolated", numSamplesMissing);
    }

    /* Start doing algorithm */
    hrStatus = Adpd400xAlgHRProcess(levelSlotA,
                            levelSlotB,
                            acceldata,
                            result,
                            &gAdpd400x_resultlog);

    return (INT_ERROR_CODE_t)hrStatus;

}

void Adpd400xHeartRateSignalAlgNewSetting()  {
  if(HrStage == STARTED){
    Adpd400xAlgHrFrontEnd_Reset();
    gsFrontEndReset = 1;
  }

  //gsPrevRawData.slotBCh = gAdpd400xSlotBChOrg[0] + gAdpd400xSlotBChOrg[1] + gAdpd400xSlotBChOrg[2] + gAdpd400xSlotBChOrg[3];
  gsPrevRawData.slotBCh = gAdpd400xSlotBChOrg[0];
  gsPrevRawData.acc[0] = gAdpd400xAdxlOrg[0];
  gsPrevRawData.acc[1] = gAdpd400xAdxlOrg[1];
  gsPrevRawData.acc[2] = gAdpd400xAdxlOrg[2];
  gsPrevRawData.timeStamp = gAdpd400xPpgTimeStamp;
}

/************************************************************
 Internal Functions
 ************************************************************/
/**
  * @brief Function to measure the data rate based on the ADPD sample timestamp
  * @param datarate : Pointer to variable which is set to the new data rate
  * @param timeStamp : Pointer to ADPD sample timestamp
  * @retval SUCCESS = Variable set to current data rate
  */
static uint8_t measureDataRate(uint16_t *datarate, int32_t currentTime) {
    static uint32_t startTime;
    static uint32_t sampleCount;
    uint32_t deltaTime;

    if (datarate == 0) {
        startTime = (uint32_t)currentTime;
        sampleCount = 0;
        return 2;
    }

    deltaTime = AdpdMwLibDeltaTime(startTime, (uint32_t)currentTime);
    deltaTime >>= 5;
    sampleCount++;

    if (deltaTime > gAdpd400x_lcfg->drTime) {
        *datarate = (deltaTime * 1000) / sampleCount;
        startTime = (uint32_t)currentTime;

        /* debug(MODULE, "******* DataRate T = %d %u %u %i ******* ", \
              *datarate,   \
              sampleCount, \
              deltaTime,   \
              currentTime);*/
	    NRF_LOG_DEBUG( "******* DataRate T = %d %u %u %i ******* \n",
         *datarate,
         sampleCount,
         deltaTime,
         currentTime);
        sampleCount = 0;
        return 0;
    }

    return 1;
}

