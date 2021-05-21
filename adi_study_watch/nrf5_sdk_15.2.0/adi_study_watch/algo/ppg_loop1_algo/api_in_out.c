/**
    ****************************************************************************
    * @file     APIInOut.c
    * @author   ADI
    * @version  V1.0
    * @date     29-Jan-2016
    ******************************************************************************
    * @attention
    ******************************************************************************
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
* This software is intended for use with the ADPD and derivative parts     *
* only                                                                        *
*                                                                             *
******************************************************************************/
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>
#include "adpd400x_lib.h"
#include "heart_rate_internal.h"
#include "adpd400x_lib_common.h"

#define MODULE ("APIInOut.c")

static const uint32_t gsHrmAdpdLibVersionnumber = HRMPPGLIBVERSIONNUMBER;
static const uint32_t gsHr_alg_versionnumber = HRMALGORITHMVERSIONNUMBER;
static const uint8_t gsHr_alg_type = 1;

/* Global variables */

INT_ERROR_CODE_t Adpd400xStateMachine(LibResult_t *result,
                          uint32_t *slotData,
                          int16_t *acceldata,
                          TimeStamps_t timeStamp);


/**
  * @brief Gets the version number of the library
  * @retval uint32_t Returns the version number of the library in hexdecimal
  *         format.
  *         For example: 0x00010200 is version 1.2.0
  */
uint32_t Adpd400xLibGetVersion() {
  return gsHrmAdpdLibVersionnumber;
}

/**
  * @brief Gets the version number of the HR algorithm embedded in the library
  * @retval uint32_t Returns the version number of the HR algorithm embedded
  *          library.
  */
uint32_t Adpd400xLibGetAlgorithmVersion() {
  return gsHr_alg_versionnumber;
}

/**
  * @brief Gets the type of the HR algorithm embedded in the library
  * @retval uint8_t Returns the type of the HR algorithm embedded library
  * 0 - No Algorithm; 1 - ADI HRM algorithm; 2 - LifeQ HRM algorithm
  */
uint8_t Adpd400xLibGetAlgorithmType() {
  return gsHr_alg_type;
}

/**
  * @brief Gets the vendor name and version number of the HR algorithm
  *        embedded in the library
  * @param pointer to the buffer where the vendor name and version number
  *   are returned
  * @retval uint8_t Returns the size of he string.
  */
uint8_t Adpd400xLibGetAlgorithmVendorAndVersion(uint8_t *nAlgoInfo) {
  uint8_t nAlgoInfoSize;
  strcpy((char *)nAlgoInfo, "ADI HRM;");
  nAlgoInfoSize = strlen("ADI HRM;");
  nAlgoInfo[nAlgoInfoSize++] = ((gsHr_alg_versionnumber & 0xFF0000) >> 16) + '0';
  nAlgoInfo[nAlgoInfoSize++] = '.';
  nAlgoInfo[nAlgoInfoSize++] = ((gsHr_alg_versionnumber & 0xFF00) >> 8) + '0';
  nAlgoInfo[nAlgoInfoSize++] = '.';
  nAlgoInfo[nAlgoInfoSize++] = (gsHr_alg_versionnumber & 0xFF) + '0';
  nAlgoInfo[nAlgoInfoSize++] = '\0';
  return nAlgoInfoSize;
}


/**
  * @brief Initializes the library for use. This must be called before the
  * library AdpdLibGetHr can be called. Before the library can be
  * opened again it must be closed with a call to AdpdLibCloseHr().
  * @retval ERROR_CODE_t indicating whether or not the operation was successful.
  */
ADPDLIB_ERROR_CODE_t Adpd400xLibOpenHr() {
  return (ADPDLIB_ERROR_CODE_t) Adpd400xStateMachineInit();
}

/**
  * @brief Closes the HR library; must be called when shutting down.
  * @retval ERROR_CODE_t indicating whether or not the operation was successful.
  */
ADPDLIB_ERROR_CODE_t Adpd400xLibCloseHr() {
  ADPDLIB_ERROR_CODE_t ret;
  ret = (ADPDLIB_ERROR_CODE_t)Adpd400xStateMachineDeInit();

  return ret;
}

/**
  * @brief Set the device library configuration used by the library.
  *        This must be called before AdpdLibOpenHr is called.
  * @param lcfg pointer to the LCFG array
  * @retval none.
  */
void Adpd400xLibApplyLCFG(Adpd400xLibConfig_t *lcfg)  {
  gAdpd400x_lcfg = lcfg;
}

/**
  * @brief Returns the values used during the Detect On state.
  * @param val returns the current DC level. If there’s no object on sensor,
  *        this is the ambient level.
  * @param valAir returns the current ambient level setting (lcfg41 value).
  * @param var returns the current variance value.
  * @retval uint8_t Current state of the state machine.
  */
uint8_t Adpd400xLibGetDetectOnValues(uint32_t *val,
                                  uint32_t *valAir,
                                  uint32_t *var) {
  if (val != 0)
    *val = gAdpd400xDetectVal.detectOnValue;

  if (valAir != 0)
    *valAir = gAdpd400xDetectVal.detectOnAirLevel;
  if (var != 0)
    *var = gAdpd400xDetectVal.detectOnVariance;

  return Adpd400xLibGetState();
}

/**
  * @brief Returns the current mode of the ADPD device as seen by ADPDLib
  * @retval int32_t current ADPD device mode; see ADPDDrv
  */
int32_t Adpd400xLibGetMode(uint16_t *mode) {
  *mode = g_Adpd400xdeviceMode;

  return 0;
}


/**
  * @brief Set function points for library instrumentation, aka logging support
  * @param result pointer to where the results will be stored.
  * @param slotACh pointer to an array containing slot A channel data;
  *                where index 0, 1, 2 and 3 are used for channels
  *                1, 2, 3 and 4 respectively.
  * @param slotData pointer to an array containing slot A channel data;
  *                where index 0, 1, 2 and 3 are used for channels
  *                1, 2, 3 and 4 respectively.
  * @param acceldata pointer to an array containing the accelerometer data;
  *                  where index 0, 1 and 2 are used for x, y and z
  *                  respectively. Please see the section Library Configuration
  *                  for details on configuring the input type for the
  *                  accelerometer data.
  * @param timeStamp a milliseconds resolution time stamp.
  * @retval ERROR_CODE_t from the state machine
  */
ADPDLIB_ERROR_CODE_t Adpd400xLibGetHr(LibResultX_t *result,
                                   uint32_t     *slotData,
                                   int16_t      *acceldata,
                                   TimeStamps_t  timeStamp) {
  LibResult_t output;
  INT_ERROR_CODE_t ret;
  ADPDLIB_ERROR_CODE_t libErrCode;

  ret = Adpd400xStateMachine(&output, slotData, acceldata, timeStamp);

  if (ret == IERR_SUCCESS_WITH_RESULT) {
    if (output.HR_Type >= 0) {              // in HearRate show time
      result->HR = output.HR;
      result->confidence = output.confidence;
      result->HR_Type = output.HR_Type;
      result->RRinterval = output.RRinterval;
      result->IsHrvValid = output.IsHrvValid;
    } else {
      result->confidence = 0;
      result->HR_Type = 0;
      ret = (INT_ERROR_CODE_t)output.HR;    // Note this is a failure code
    }
  }
  libErrCode = (ADPDLIB_ERROR_CODE_t)ret;

  return libErrCode;
}

/**
  * @brief Returns the AGC state and other AGC related information.
  * @param AGC state pointer.
  * @retval none.
  */
void Adpd400xLibGetAgcState(AGCStat_t* agcInfo) {
  memcpy(agcInfo, &gAdpd400xAGCStatInfo, sizeof(AGCStat_t));
  gAdpd400xAGCStatInfo.setting[0] = 0;    // Reset State
}

/**
  * @brief Returns the ppg lib AFE operation mode of the device.
  * @retval none
  */
void Adpd400xLibGetLibStat_AFE_OP_MODE(Adpd400xLib_AFEMODE_t *afe_mode) {
  *afe_mode = gAdpd400xPPGLibStatus.AFE_OpMode;
}

/**
  * @brief Returns the ppg lib CTR value of the detected object.
  * @retval none
  */
void Adpd400xLibGetLibStat_CTR_Value(uint16_t *ctr_val) {
  *ctr_val = gAdpd400xPPGLibStatus.CtrValue;
}

/**
  * @brief Returns the ppg lib AGC Signal metrics of the device.
  * @retval none
  */
void Adpd400xLibGetLibStat_AGC_SIGM(uint16_t *sig_val) {
  sig_val[0] = gAdpd400xAGCStatInfo.mts[0];
  sig_val[1] = gAdpd400xAGCStatInfo.mts[1];
  sig_val[2] = gAdpd400xAGCStatInfo.mts[2];
  sig_val[3] = gAdpd400xAGCStatInfo.mts[3];
}


/**
  * @brief Returns the current stage of the library stage machine.
  * @retval uint8_t state machine stage
  *                 0 - Start
  *                 2 - Detection
  *                 4,6,9 - Calibration
  *                 7 - Heart Rate
  */
uint8_t Adpd400xLibGetState() {
  return (uint8_t)gAdpd400xPpgLibState;
}

/**
  * @brief Returns an input state's info of the library state machine.
  * @param input, the state of interest.
  * @param output, the state's related infomation.
  * @retval ADPDLIB_ERROR_CODE_t Error or Fail
  */
ADPDLIB_ERROR_CODE_t Adpd400xLibGetStateInfo(uint8_t state, uint16_t* debugData) {
  if (debugData == 0)
    return ADPDLIB_ERR_FAIL;
  switch (state)  {
  case ADPDLIB_STAGE_START:
    debugData[0] = (uint16_t)(gAdpd400x_lcfg->featureSelect);
    debugData[1] = (uint16_t)(gAdpd400xOptmVal.ledB_Cur);
    debugData[2] = (uint16_t)(gAdpd400xOptmVal.sampleRate);
    debugData[3] = (uint16_t)(gAdpd400xDetectVal.detectOnValue);
    debugData[4] = (uint16_t)(0xAABB);
    break;
  // case ADPDLIB_STAGE_DARKOFFSET_CALIBRATION:
  // case ADPDLIB_STAGE_DETECT_PERSON:
  // break;
  case ADPDLIB_STAGE_GETCTR:
    debugData[0] = (uint16_t)(gAdpd400xDetectVal.proxOnLevel);
    debugData[1] = (uint16_t)(gAdpd400xDetectVal.detectOnValue);
    debugData[2] = (uint16_t)(gAdpd400xDetectVal.detectOnVariance);
    debugData[3] = (uint16_t)(0xAABB);
    break;
  case ADPDLIB_STAGE_OPTIMIZATION:
    debugData[0] = (uint16_t)gAdpd400xPPGLibStatus.CtrValue;
    debugData[1] = (uint16_t)(0xAABB);
    break;
  case ADPDLIB_STAGE_END_CALIBRATION:
    debugData[0] = (uint16_t)(gAdpd400xOptmVal.ledB_Cur);
    debugData[1] = (uint16_t)(gAdpd400xOptmVal.ledB_Trim);
    debugData[2] = (uint16_t)(gAdpd400xOptmVal.ledB_Pulse);
    debugData[3] = (uint16_t)(gAdpd400xOptmVal.sampleRate);
    debugData[4] = (uint16_t)(gAdpd400xOptmVal.tiaB_Gain);
    debugData[5] = (uint16_t)(gAdpd400xOptmVal.ledB_FltWid);
    debugData[6] = (uint16_t)(0xAABB);
    debugData[7] = (uint16_t)(gAdpd400xPPGLibStatus.AFE_OpMode);
    debugData[8] = (uint16_t)(gAdpd400xPPGLibStatus.CtrValue);
    break;
  case ADPDLIB_STAGE_HEART_RATE_INIT:
    debugData[0] = (uint16_t)(gAdpd400xPowerVal.ledB_PW);
    debugData[1] = (uint16_t)(gAdpd400xPowerVal.total_PW);
    debugData[2] = (uint16_t)(gAdpd400xOptmVal.sampleRate);
    debugData[3] = (uint16_t)(gAdpd400xOptmVal.decimation);
    break;
  case ADPDLIB_STAGE_HEART_RATE:
    // Loop1 setting
    debugData[0] = (uint16_t)(gAdpd400xOptmVal.ledB_Cur);
    debugData[1] = (uint16_t)(gAdpd400xOptmVal.ledB_Trim);
    debugData[2] = (uint16_t)(gAdpd400xOptmVal.ledB_Pulse);
    debugData[3] = (uint16_t)(gAdpd400xOptmVal.sampleRate);
    debugData[4] = (uint16_t)(gAdpd400xPPGLibStatus.CtrValue);

    // if AGC, record loop2 setting
    if (gAdpd400xOptmVal.SelectedLoop == 1)  {
      debugData[5] = (uint16_t)(gAdpd400xOptmVal.ledB_Cur);
      debugData[6] = (uint16_t)(gAdpd400xOptmVal.ledB_Trim);
      debugData[7] = (uint16_t)(gAdpd400xOptmVal.ledB_Pulse);
      debugData[8] = (uint16_t)(gAdpd400xOptmVal.sampleRate);
      debugData[9] = (uint16_t)(gAdpd400xOptmVal.tiaB_Gain);
    } else {
      debugData[5] = (uint16_t)(gAdpd400xOptmVal.ledB_Cur2);
      debugData[6] = (uint16_t)(gAdpd400xOptmVal.ledB_Trim2);
      debugData[7] = (uint16_t)(gAdpd400xOptmVal.ledB_Pulse2);
      debugData[8] = (uint16_t)(gAdpd400xOptmVal.sampleRate2);
      debugData[9] = (uint16_t)(gAdpd400xOptmVal.tiaB_Gain);
    }
    break;
  default:
    debugData[0] = (uint16_t)(0xABCD);
    debugData[1] = (uint16_t)(1234);
    break;
  }
  return ADPDLIB_ERR_SUCCESS;
}
#if 0
void Adpd400xLibAdjestAmbient()  {
  Adpd400xFloatModeAdjestAmbient();
}
#endif