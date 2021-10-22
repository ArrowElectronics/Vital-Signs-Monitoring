/**
    ****************************************************************************
    * @file     APIInOut.c
    * @author   ADI
    * @version  V1.1
    * @date     08-June-2021
    ******************************************************************************
    * @attention
    ******************************************************************************
    */
/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2021 Analog Devices Inc.                                      *
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
  * @brief Initializes the HRM library for use. This must be called before the
  * library AdpdLibGetHr can be called. Initial pulse from lcfg will be updated to ADPD. 
  * Before the library can be opened again it must be closed with a call to Adpd400xLibCloseHr().
  * @retval ERROR_CODE_t indicating whether or not the operation was successful.
  */
ADPDLIB_ERROR_CODE_t Adpd400xLibOpenHr() {
  return (ADPDLIB_ERROR_CODE_t) Adpd400xStateMachineInit();
}

/**
  * @brief Deinit the HRM library; Restore the sample rate changed by dynamic AGC
           ,clears the saturation detection bit and clears the library state.
  * @retval ERROR_CODE_t indicating whether or not the operation was successful.
  */
ADPDLIB_ERROR_CODE_t Adpd400xLibCloseHr() {
  ADPDLIB_ERROR_CODE_t ret;
  ret = (ADPDLIB_ERROR_CODE_t)Adpd400xStateMachineDeInit();

  return ret;
}

/**
  * @brief Set the device library configuration used by the library.
  *        User can change the library configuration from application using this API 
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
  * @brief Function to get HRM result from library, After Adpd400xLibOpenHr function called, 
           this function has to be called continuously to get HR result.This is the wrapper 
           to call statemachine process of library.If the statemachine returns
           IERR_SUCCESS_WITH_RESULT,the result structure from statemachine will be 
           copied to the application result structure 
  * @param result pointer to where the results will be stored.
  * @param slotData pointer to an array containing ADPD data;
  *                where index 0 and 1 are used for channel 1 and 2 respectively.
  * @param acceldata pointer to an array containing the accelerometer data;
  *                  where index 0, 1 and 2 are used for x, y and z
  *                  respectively. 
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

  if ((ret == IERR_SUCCESS_WITH_RESULT)  || (ret == IERR_ALGO_INPUT_OVERFLOW)) {
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
  * @brief Returns the AGC state and other AGC related information
  *        Whenever the Dynamic/static AGC change power settings, new LED power settings captured in the
  *        gAdpd400xAGCStatInfo structure
  * @param AGC state pointer gets the updated settings from AGC info structure.
  * @retval none.
  */
void Adpd400xLibGetAgcState(AGCStat_t* agcInfo) {
  gAdpd400xAGCStatInfo.setting[6] = log2(gAdpd400x_lcfg->targetSlots);
  memcpy(agcInfo, &gAdpd400xAGCStatInfo, sizeof(AGCStat_t));
  gAdpd400xAGCStatInfo.setting[0] = 0;    // Reset State
}

/**
  * @brief Function to update the AGC info structure 
  *        when there is change in LED power happens, eg This function called in AGC recalibration,
  *        AFE saturation etc
  * @param ENUM for AGC LOG indicator,reason to update the AGC info struct.
  *        eg, ADPD400xLIB_AGCLOG_STATIC_AGC_RECAL 
  * @retval none
  */
void Adpd400xLibUpdateAGCStateInfo(ADPD400xLIB_AGCLOG_Indicator_t agcindicator) {
    gAdpd400xAGCStatInfo.setting[0] = (uint16_t)agcindicator;
    Adpd400xUpdateAGCInfoSettings();    
}

/**
  * @brief Returns the Algo raw data captured in the HRM algorithm wrapper.
  *        Not all the data from ADPD data passed to HRM algorithm wrapper, sometimes ADPD data skipped
  *        when there is power change happens.The data captures at HRM algorithm wrapper for simulation purpose.
  * @param Algo raw data struct pointer.
  * @retval none.
  */
void Adpd400xLibGetAlgoRawData(AlgoRawDataStruct_t* algoInfo) {
  memcpy(algoInfo, &gGetAlgoInfo, sizeof(AlgoRawDataStruct_t));
}

/**
  * @brief Returns the ppg lib CTR value of the detected object.
  * @retval none
  */
void Adpd400xLibGetLibStat_CTR_Value(uint16_t *ctr_val) {
  *ctr_val = gAdpd400xPPGLibStatus.CtrValue;
}

/**
  * @brief Returns the signal quality for the PPG signal.
  *        This value will be updated from SQI library whenever the Dynamic AGC checks the signal quality
  * @param uint16_t pointer gets the signal quality value.
  * @retval none
  */
void Adpd400xLibGetLibStat_AGC_SIGM(uint16_t *sig_val) {
  *sig_val = gAdpd400xAGCStatInfo.mts[0];
}

/**
  * @brief Returns the current state of the library state machine.
  * @retval uint8_t state machine stage
            ADPDLIB_STAGE_START = 1,
            ADPDLIB_STAGE_DARKOFFSET_CALIBRATION = 3,
            ADPDLIB_STAGE_DETECT_PERSON = 5,
            ADPDLIB_STAGE_HEART_RATE_INIT = 6,
            ADPDLIB_STAGE_HEART_RATE = 7,
            ADPDLIB_STAGE_END_CALIBRATION = 15,
            ADPDLIB_STAGE_DETECT_OFF = 20
  */
uint8_t Adpd400xLibGetState() {
  return (uint8_t)gAdpd400xPpgLibState;
}

/**
  * @brief Returns an input state's info of the library state machine.
  *        The debug info captures the required information like current,gain,pulse in each stage of library
  *        The first 10 bytes captures the loop1 settings(AGC) and 
  *        second 10 bytes captures whenever power change happens on loop2(dynamic AGC) 
  * @param input, library state for details required.
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
    debugData[4] = (uint16_t)(gAdpd400xOptmVal.tiaB_Gain);

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