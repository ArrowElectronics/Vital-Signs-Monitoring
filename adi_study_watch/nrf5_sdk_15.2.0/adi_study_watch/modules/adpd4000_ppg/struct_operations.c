/**
    ***************************************************************************
    * @addtogroup User
    * @{
    * @file         StructOperations.c
    * @author       ADI
    * @version      V1.0.0
    * @date         29-Sept-2015
    * @brief        Structure operations for sample application.
    *
    ***************************************************************************
    * @attention
    ***************************************************************************
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
#ifdef ENABLE_PPG_APP
/* Includes -----------------------------------------------------------------*/
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include "main.h"
//#include "adpd_lib.h"
#include "adpd400x_lib.h"
#include "struct_operations.h"

#ifdef DCB
#include "dcb_interface.h"
#endif

/* ------------------------- Defines  --------------------------------------- */
#define LCFG_SIZE   56
#define ADPD400xLCFG_SIZE   53
/* ------------------------- Public Function Prototypes -------------------- */
/* Functions to do operations of lcfg structure */
int stricmp(char const *a, char const *b);
void ShowLcfgStruct();
void DumpLcfgStruct();


int tolower(int c);

/* ------------------------- Public Variables ------------------------------ */
//AdpdLibConfig_t gAdpdLibCfg;
//AdpdLibConfig_t *aStruct = &gAdpdLibCfg;
Adpd400xLibConfig_t gAdpd400xLibCfg;
Adpd400xLibConfig_t *adpd400xStruct = &gAdpd400xLibCfg;

//uint16_t stroffset_array[LCFG_SIZE];
uint16_t stroffset_Adpd400xarray[ADPD400xLCFG_SIZE];
#if 0
char *str_array[] = {
                      "partNum", "uint16_t",
                      "targetChs", "uint8_t",
                      "devicemode", "uint16_t",
                      "skipstate", "uint16_t",
                      "drTime", "uint16_t",
                      "res32_1", "uint32_t",
                      "hrmInputRate", "uint16_t",
                      "syncmode", "uint8_t",
                      "proximityRate", "uint32_t",
                      "proximityTimeout", "uint16_t",
                      "proximityOnLevel", "uint16_t",

                      "detectionRate", "uint32_t",
                      "detectOntimeout", "uint16_t",
                      "detectOnSettlingCnt", "uint8_t",
                      "triggerOnLevel", "uint16_t",
                      "triggerOnAirLevel", "uint16_t",
                      "triggerOnStablizeVR", "uint32_t",
                      "res16_2", "uint16_t",

                      "detectOffSettlingCnt", "uint8_t",
                      "triggerOffPercentage", "uint8_t",
                      "triggerOffStablizeVR", "uint32_t",
                      "res16_3", "uint16_t",
                      "res8_1", "uint8_t",

                      "driftPercent", "uint8_t",
                      "driftInterval", "uint8_t",

                      "maxSamplingRate", "uint16_t",
                      "saturateAdjustPercent", "uint8_t",
                      "ledMaxCurrent", "uint16_t",
                      "maxPulseNum",  "uint8_t",
                      "floatModeCtr", "uint8_t",
                      "ledB_Vol", "uint8_t",
                      "dcLevelPercentA", "uint8_t",
                      "modIndex", "uint16_t",

                      "motionThreshold", "uint32_t",
                      "motionCheckPeriod", "uint32_t",
                      "motionThresholdHigh", "uint32_t",
                      "motionCheckPeriodHigh", "uint16_t",
                      "ctrTh", "uint8_t",
                      "mt2Th", "uint16_t",
                      "mt3Th", "uint16_t",

                      "ambientChk", "uint16_t",
                      "ambientTh", "uint16_t",
#if ALGO_ADI
                      "spotalgosamplerate","int16_t",
                      "spotalgodecimation","int16_t",
                      "mindifftrackSpot","int16_t",
                      "initialconfidencethreshold","int16_t",
                      "ppgscale","uint32_t",
                      "accelscale","int16_t",
                      "spotstabilitycount","uint8_t",
                      "spothrtimeoutsecs","int16_t",
                      "zeroorderholdnumsamples","int16_t",
                      "trackalgosamplerate","int16_t",
                      "trackhrtimeoutsecs","int16_t",
                      "spotwindowlength","uint32_t",
                      "trackerminheartratebpm","uint32_t",
                      "hrvEnable","uint8_t"
#endif
                      };
#endif

char *Adpd400x_str_array[] = {
                      "partNum", "uint16_t",
                      "targetSlots", "uint16_t",
                      "targetChs", "uint8_t",
                      "deviceMode", "uint16_t",
                      "featureSelect", "uint16_t",
                      "drTime", "uint16_t",
                      "DutyCycle", "uint32_t",
                      "hrmInputRate", "uint16_t",
                      "syncMode", "uint8_t",
                      "proximityRate", "uint32_t",
                      "proximityTimeout", "uint16_t",
                      "proximityOnLevel", "uint16_t",

                      "detectionRate", "uint32_t",
                      "detectOntimeout", "uint16_t",
                      "detectOnSettlingCnt", "uint8_t",
                      "triggerOnLevel", "uint16_t",
                      "triggerOnAirLevel", "uint16_t",
                      "triggerOnStablizeVR", "uint32_t",
                      "res16_2", "uint16_t",

                      "detectOffSettlingCnt", "uint8_t",
                      "triggerOffPercentage", "uint8_t",
                      "triggerOffStablizeVR", "uint32_t",
                      "res16_3", "uint16_t",
                      "res8_1", "uint8_t",

                      "maxSamplingRate", "uint16_t",
                      "targetDcPercent", "uint8_t",
                      "maxLedCurrent", "uint16_t",
                      "maxPulseNum",  "uint8_t",
                      "floatModeCtr", "uint8_t",
                      "ledG_Voltage", "uint8_t",
                      "modIndex", "uint16_t",

                      "motionThreshold", "uint32_t",
                      "motionCheckPeriod", "uint32_t",
                      "motionThresholdHigh", "uint32_t",
                      "motionCheckPeriodHigh", "uint32_t",
                      "mt2Th", "uint16_t",
                      "mt3Th", "uint16_t",

                      "ambientChk", "uint16_t",
                      "ambientTh", "uint16_t",
#if ALGO_ADI
                      "spotalgosamplerate","int16_t",
                      "spotalgodecimation","int16_t",
                      "mindifftrackSpot","int16_t",
                      "initialconfidencethreshold","int16_t",
                      "ppgscale","uint32_t",
                      "accelscale","int16_t",
                      "spotstabilitycount","uint8_t",
                      "spothrtimeoutsecs","int16_t",
                      "zeroorderholdnumsamples","int16_t",
                      "trackalgosamplerate","int16_t",
                      "trackhrtimeoutsecs","int16_t",
                      "spotwindowlength","uint32_t",
                      "trackerminheartratebpm","uint32_t",
                      "hrvEnable","uint8_t"
#endif
                      };
/**
* @brief    Case insensitive string comparison
* @param    *a string1 for comparison
* @param    *b string2 for comparison
* @return   Returns 0 if both strings are identical
*/
int stricmp(char const *a, char const *b)  {
    for (;; a++, b++) {
        int d = tolower(*a) - tolower(*b);
        if (d != 0 || !*a)
            return d;
    }
}

#if 0
/**
* @brief    To initilize Lcfg Offset array
* @return   None
*/
void InitOffsetsLcfgStruct()  {
    uint8_t i = 0;
    stroffset_array[i++] = offsetof(AdpdLibConfig_t, partNum);
    stroffset_array[i++] = offsetof(AdpdLibConfig_t, targetChs);
    stroffset_array[i++] = offsetof(AdpdLibConfig_t, devicemode);
    stroffset_array[i++] = offsetof(AdpdLibConfig_t, skipstate);
    stroffset_array[i++] = offsetof(AdpdLibConfig_t, drTime);
    stroffset_array[i++] = offsetof(AdpdLibConfig_t, res32_1);
    stroffset_array[i++] = offsetof(AdpdLibConfig_t, hrmInputRate);
    stroffset_array[i++] = offsetof(AdpdLibConfig_t, syncmode);
    stroffset_array[i++] = offsetof(AdpdLibConfig_t, proximityRate);
    stroffset_array[i++] = offsetof(AdpdLibConfig_t, proximityTimeout);
    stroffset_array[i++] = offsetof(AdpdLibConfig_t, proximityOnLevel);

    stroffset_array[i++] = offsetof(AdpdLibConfig_t, detectionRate);
    stroffset_array[i++] = offsetof(AdpdLibConfig_t, detectOntimeout);
    stroffset_array[i++] = offsetof(AdpdLibConfig_t, detectOnSettlingCnt);
    stroffset_array[i++] = offsetof(AdpdLibConfig_t, triggerOnLevel);
    stroffset_array[i++] = offsetof(AdpdLibConfig_t, triggerOnAirLevel);
    stroffset_array[i++] = offsetof(AdpdLibConfig_t, triggerOnStablizeVR);
    stroffset_array[i++] = offsetof(AdpdLibConfig_t, res16_2);

    stroffset_array[i++] = offsetof(AdpdLibConfig_t, detectOffSettlingCnt);
    stroffset_array[i++] = offsetof(AdpdLibConfig_t, triggerOffPercentage);
    stroffset_array[i++] = offsetof(AdpdLibConfig_t, triggerOffStablizeVR);
    stroffset_array[i++] = offsetof(AdpdLibConfig_t, res16_3);
    stroffset_array[i++] = offsetof(AdpdLibConfig_t, res8_1);

    stroffset_array[i++] = offsetof(AdpdLibConfig_t, driftPercent);
    stroffset_array[i++] = offsetof(AdpdLibConfig_t, driftInterval);

    stroffset_array[i++] = offsetof(AdpdLibConfig_t, maxSamplingRate);
    stroffset_array[i++] = offsetof(AdpdLibConfig_t, saturateAdjustPercent);
    stroffset_array[i++] = offsetof(AdpdLibConfig_t, ledMaxCurrent);
    stroffset_array[i++] = offsetof(AdpdLibConfig_t, maxPulseNum);
    stroffset_array[i++] = offsetof(AdpdLibConfig_t, floatModeCtr);
    stroffset_array[i++] = offsetof(AdpdLibConfig_t, ledB_Vol);
    stroffset_array[i++] = offsetof(AdpdLibConfig_t, dcLevelPercentA);
    stroffset_array[i++] = offsetof(AdpdLibConfig_t, modIndex);

    stroffset_array[i++] = offsetof(AdpdLibConfig_t, motionThreshold);
    stroffset_array[i++] = offsetof(AdpdLibConfig_t, motionCheckPeriod);
    stroffset_array[i++] = offsetof(AdpdLibConfig_t, motionThresholdHigh);
    stroffset_array[i++] = offsetof(AdpdLibConfig_t, motionCheckPeriodHigh);

    stroffset_array[i++] = offsetof(AdpdLibConfig_t, ctrTh);
    stroffset_array[i++] = offsetof(AdpdLibConfig_t, mt2Th);
    stroffset_array[i++] = offsetof(AdpdLibConfig_t, mt3Th);

    stroffset_array[i++] = offsetof(AdpdLibConfig_t, ambientChk);
    stroffset_array[i++] = offsetof(AdpdLibConfig_t, ambientTh);

#ifdef ALGO_ADI
    stroffset_array[i++] = offsetof(AdpdLibConfig_t, spotalgosamplerate);
    stroffset_array[i++] = offsetof(AdpdLibConfig_t, spotalgodecimation);
    stroffset_array[i++] = offsetof(AdpdLibConfig_t, mindifftrackSpot);
    stroffset_array[i++] = offsetof(AdpdLibConfig_t, initialconfidencethreshold);
    stroffset_array[i++] = offsetof(AdpdLibConfig_t, ppgscale);
    stroffset_array[i++] = offsetof(AdpdLibConfig_t, accelscale);
    stroffset_array[i++] = offsetof(AdpdLibConfig_t, spotstabilitycount);
    stroffset_array[i++] = offsetof(AdpdLibConfig_t, spothrtimeoutsecs);
    stroffset_array[i++] = offsetof(AdpdLibConfig_t, zeroorderholdnumsamples);
    stroffset_array[i++] = offsetof(AdpdLibConfig_t, trackalgosamplerate);
    stroffset_array[i++] = offsetof(AdpdLibConfig_t, trackhrtimeoutsecs);
    stroffset_array[i++] = offsetof(AdpdLibConfig_t, spotwindowlength);
    stroffset_array[i++] = offsetof(AdpdLibConfig_t, trackerminheartratebpm);
    stroffset_array[i++] = offsetof(AdpdLibConfig_t, hrvEnable);
#endif
}
#endif

/**
* @brief    To initilize Lcfg Offset array
* @return   None
*/
void InitOffsetsAdpd400xLcfgStruct()  {
    uint8_t i = 0;
    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, partNum);
    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, targetSlots);
    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, targetChs);
    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, deviceMode);
    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, featureSelect);
    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, drTime);
    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, DutyCycle);
    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, hrmInputRate);
    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, syncMode);
    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, proximityRate);
    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, proximityTimeout);
    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, proximityOnLevel);

    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, detectionRate);
    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, detectOntimeout);
    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, detectOnSettlingCnt);
    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, triggerOnLevel);
    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, triggerOnAirLevel);
    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, triggerOnStablizeVR);
    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, res16_2);

    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, detectOffSettlingCnt);
    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, triggerOffPercentage);
    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, triggerOffStablizeVR);
    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, res16_3);
    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, res8_1);

    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, maxSamplingRate);
    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, targetDcPercent);
    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, maxLedCurrent);
    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, maxPulseNum);
    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, floatModeCtr);
    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, ledG_Voltage);
    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, modIndex);

    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, motionThreshold);
    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, motionCheckPeriod);
    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, motionThresholdHigh);
    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, motionCheckPeriodHigh);

    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, mt2Th);
    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, mt3Th);

    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, ambientChk);
    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, ambientTh);

#ifdef ALGO_ADI
    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, spotalgosamplerate);
    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, spotalgodecimation);
    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, mindifftrackSpot);
    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, initialconfidencethreshold);
    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, ppgscale);
    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, accelscale);
    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, spotstabilitycount);
    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, spothrtimeoutsecs);
    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, zeroorderholdnumsamples);
    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, trackalgosamplerate);
    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, trackhrtimeoutsecs);
    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, spotwindowlength);
    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, trackerminheartratebpm);
    stroffset_Adpd400xarray[i++] = offsetof(Adpd400xLibConfig_t, hrvEnable);
#endif
}

#if 0
/**
* @brief    To modify Lcfg Offset array structure based on field string
* @param    *field Lcfg Flield that needs to be modified
* @param    val Value of Lcfg Flield that needs to be modified
* @return   StructOpErrorStatus returns error status
*/
StructOpErrorStatus ModifyLcfgStructure(char *field, int32_t val) {
    int32_t i = 0;
    while (i < LCFG_SIZE) {
        if (stricmp(field, str_array[2*i]) == 0) {
          if (strcmp(str_array[2*i+1], "uint32_t") == 0)
            *(uint32_t *)((char *)aStruct + stroffset_array[i]) = (uint32_t)val;
          else if (strcmp(str_array[2*i+1], "uint8_t") == 0)
            *(uint8_t *)((char *)aStruct + stroffset_array[i]) = (uint8_t)val;
          else if (strcmp(str_array[2*i+1], "int8_t") == 0)
            *(int8_t *)((char *)aStruct + stroffset_array[i]) = (int8_t)val;
          else if (strcmp(str_array[2*i+1], "uint16_t") == 0)
            *(uint16_t *)((char *)aStruct + stroffset_array[i]) = (uint16_t)val;
          else if (strcmp(str_array[2*i+1], "int16_t") == 0)
            *(int16_t *)((char *)aStruct + stroffset_array[i]) = (int16_t)val;
	  else if (strcmp(str_array[2*i+1],"float") == 0)
	     *(float *)((char *)aStruct + stroffset_array[i]) = (float)val;
          return STRUCTOPSUCCESS;
        }
        i++;
    }
    return STRUCTOPERROR;
}

/**
* @brief    To modify Lcfg Offset array structure based on field index
* @param    *field Lcfg field index that needs to be modified
* @param    val Value of Lcfg field index that needs to be modified
* @return   StructOpErrorStatus returns error status
*/
StructOpErrorStatus ModifyLcfgStructureRaw(uint32_t field, int32_t val) {
  if (field < LCFG_SIZE) {
    if (strcmp(str_array[2*field+1], "uint32_t") == 0)
      *(uint32_t *)((char *)aStruct + stroffset_array[field]) = (uint32_t)val;
    else if (strcmp(str_array[2*field+1], "uint8_t") == 0)
      *(uint8_t *)((char *)aStruct + stroffset_array[field]) = (uint8_t)val;
    else if (strcmp(str_array[2*field+1], "int8_t") == 0)
      *(int8_t *)((char *)aStruct + stroffset_array[field]) = (int8_t)val;
    else if (strcmp(str_array[2*field+1], "uint16_t") == 0)
      *(uint16_t *)((char *)aStruct + stroffset_array[field]) = (uint16_t)val;
    else if (strcmp(str_array[2*field+1], "int16_t") == 0)
      *(int16_t *)((char *)aStruct + stroffset_array[field]) = (int16_t)val;
    else if (strcmp(str_array[2*field+1],"float") == 0)
      *(float *)((char *)aStruct + stroffset_array[field]) = (float)val;
    return STRUCTOPSUCCESS;
  } else {
    return STRUCTOPERROR;
  }
}
#endif

StructOpErrorStatus ModifyAdpd400xLcfgStructureRaw(uint32_t field, int32_t val) {
  if (field < ADPD400xLCFG_SIZE) {
    if (strcmp(Adpd400x_str_array[2*field+1], "uint32_t") == 0)
      *(uint32_t *)((char *)adpd400xStruct + stroffset_Adpd400xarray[field]) = (uint32_t)val;
    else if (strcmp(Adpd400x_str_array[2*field+1], "uint8_t") == 0)
      *(uint8_t *)((char *)adpd400xStruct + stroffset_Adpd400xarray[field]) = (uint8_t)val;
    else if (strcmp(Adpd400x_str_array[2*field+1], "int8_t") == 0)
      *(int8_t *)((char *)adpd400xStruct + stroffset_Adpd400xarray[field]) = (int8_t)val;
    else if (strcmp(Adpd400x_str_array[2*field+1], "uint16_t") == 0)
      *(uint16_t *)((char *)adpd400xStruct + stroffset_Adpd400xarray[field]) = (uint16_t)val;
    else if (strcmp(Adpd400x_str_array[2*field+1], "int16_t") == 0)
      *(int16_t *)((char *)adpd400xStruct + stroffset_Adpd400xarray[field]) = (int16_t)val;
    else if (strcmp(Adpd400x_str_array[2*field+1],"float") == 0)
      *(float *)((char *)adpd400xStruct + stroffset_Adpd400xarray[field]) = (float)val;
    return STRUCTOPSUCCESS;
  } else {
    return STRUCTOPERROR;
  }
}
#if 0
/**
* @brief    To read Lcfg Offset array structure
* @param    *field Lcfg field index that needs to be read
* @param    value Value of Lcfg
* @return   StructOpErrorStatus returns error status
*/
StructOpErrorStatus ReadLcfgStructureRaw(uint32_t field, int32_t *value) {
  int32_t val = 0;
  float fval;
  if (field < LCFG_SIZE) {
    if (strcmp(str_array[2*field+1], "uint32_t") == 0) {
      val = *(uint32_t *)((char *)aStruct + stroffset_array[field]);
    } else if (strcmp(str_array[2*field+1], "uint8_t") == 0) {
      val = *(uint8_t *)((char *)aStruct + stroffset_array[field]);
    } else if (strcmp(str_array[2*field+1], "int8_t") == 0) {
      val = *(int8_t *)((char *)aStruct + stroffset_array[field]);
    } else if (strcmp(str_array[2*field+1], "uint16_t") == 0) {
      val = *(uint16_t *)((char *)aStruct + stroffset_array[field]);
    } else if (strcmp(str_array[2*field+1], "int16_t") == 0) {
      val = *(int16_t *)((char *)aStruct + stroffset_array[field]);
    } else if (strcmp(str_array[2*field+1], "float") == 0) {
      fval = *(float *)((char *)aStruct + stroffset_array[field]);
      val = (int32_t)fval;
    }
    *value = (int32_t)val;
    return STRUCTOPSUCCESS;
  } else {
    return STRUCTOPERROR;
  }
}
#endif

StructOpErrorStatus ReadAdpd400xLcfgStructureRaw(uint32_t field, int32_t *value) {
  int32_t val = 0;
  float fval;
  if (field < ADPD400xLCFG_SIZE) {
    if (strcmp(Adpd400x_str_array[2*field+1], "uint32_t") == 0) {
      val = *(uint32_t *)((char *)adpd400xStruct + stroffset_Adpd400xarray[field]);
    } else if (strcmp(Adpd400x_str_array[2*field+1], "uint8_t") == 0) {
      val = *(uint8_t *)((char *)adpd400xStruct + stroffset_Adpd400xarray[field]);
    } else if (strcmp(Adpd400x_str_array[2*field+1], "int8_t") == 0) {
      val = *(int8_t *)((char *)adpd400xStruct + stroffset_Adpd400xarray[field]);
    } else if (strcmp(Adpd400x_str_array[2*field+1], "uint16_t") == 0) {
      val = *(uint16_t *)((char *)adpd400xStruct + stroffset_Adpd400xarray[field]);
    } else if (strcmp(Adpd400x_str_array[2*field+1], "int16_t") == 0) {
      val = *(int16_t *)((char *)adpd400xStruct + stroffset_Adpd400xarray[field]);
    } else if (strcmp(Adpd400x_str_array[2*field+1], "float") == 0) {
      fval = *(float *)((char *)stroffset_Adpd400xarray + stroffset_Adpd400xarray[field]);
      val = (int32_t)fval;
    }
    *value = (int32_t)val;
    return STRUCTOPSUCCESS;
  } else {
    return STRUCTOPERROR;
  }
}

#if 0
/**
* @brief    To read entire Lcfg array structure
* @param    *pointer to buffer  where Lcfg is returned
* @return   StructOpErrorStatus returns error status
*/
StructOpErrorStatus GetLcfgStructure(int32_t *pValue, uint8_t *nSize) {
  int32_t val = 0;
  uint8_t i = 0;

  while (i < LCFG_SIZE) { // current LCFG array size is 52 elements including ADI algo
    if (strcmp(str_array[2*i+1], "uint32_t") == 0) {
      val = *(uint32_t *)((char *)aStruct + stroffset_array[i]);
    } else if (strcmp(str_array[2*i+1], "uint8_t") == 0) {
      val = *(uint8_t *)((char *)aStruct + stroffset_array[i]);
    } else if (strcmp(str_array[2*i+1], "int8_t") == 0) {
      val = *(int8_t *)((char *)aStruct + stroffset_array[i]);
    } else if (strcmp(str_array[2*i+1], "uint16_t") == 0) {
      val = *(uint16_t *)((char *)aStruct + stroffset_array[i]);
    } else if (strcmp(str_array[2*i+1], "int16_t") == 0) {
      val = *(int16_t *)((char *)aStruct + stroffset_array[i]);
    }
    *pValue++ = (int32_t)val;
    i++;
  }
  *nSize = i;
  return STRUCTOPSUCCESS;
}
#endif

StructOpErrorStatus GetAdpd400xLcfgStructure(int32_t *pValue, uint8_t *nSize) {
  int32_t val = 0;
  uint8_t i = 0;

  while (i < ADPD400xLCFG_SIZE) { // current LCFG array size is 52 elements including ADI algo
    if (strcmp(Adpd400x_str_array[2*i+1], "uint32_t") == 0) {
      val = *(uint32_t *)((char *)adpd400xStruct + stroffset_Adpd400xarray[i]);
    } else if (strcmp(Adpd400x_str_array[2*i+1], "uint8_t") == 0) {
      val = *(uint8_t *)((char *)adpd400xStruct + stroffset_Adpd400xarray[i]);
    } else if (strcmp(Adpd400x_str_array[2*i+1], "int8_t") == 0) {
      val = *(int8_t *)((char *)adpd400xStruct + stroffset_Adpd400xarray[i]);
    } else if (strcmp(Adpd400x_str_array[2*i+1], "uint16_t") == 0) {
      val = *(uint16_t *)((char *)adpd400xStruct + stroffset_Adpd400xarray[i]);
    } else if (strcmp(Adpd400x_str_array[2*i+1], "int16_t") == 0) {
      val = *(int16_t *)((char *)adpd400xStruct + stroffset_Adpd400xarray[i]);
    }
    *pValue++ = (int32_t)val;
    i++;
  }
  *nSize = i;
  return STRUCTOPSUCCESS;
}
//get structure from the array
StructOpErrorStatus GetAdpd400xLcfgarray_to_struct(Adpd400xLibConfig_t* dcblcfg, uint32_t* array) {

  #ifdef DCB
  if(ADPD400xLCFG_SIZE > MAXPPGDCBSIZE)
  {
  return STRUCTOPERROR;
  }
  #endif

  uint8_t i = 0;
  while (i < ADPD400xLCFG_SIZE) { // current LCFG array size is 53 elements including ADI algo
    if (strcmp(Adpd400x_str_array[2*i+1], "uint32_t") == 0) {
       *(uint32_t *)((char *)dcblcfg + stroffset_Adpd400xarray[i]) = (uint32_t)array[i];
    } else if (strcmp(Adpd400x_str_array[2*i+1], "uint8_t") == 0) {
      *(uint8_t *)((char *)dcblcfg + stroffset_Adpd400xarray[i]) = (uint8_t)array[i];
    } else if (strcmp(Adpd400x_str_array[2*i+1], "int8_t") == 0) {
      *(int8_t *)((char *)dcblcfg + stroffset_Adpd400xarray[i]) = (int8_t)array[i];
    } else if (strcmp(Adpd400x_str_array[2*i+1], "uint16_t") == 0) {
      *(uint16_t *)((char *)dcblcfg + stroffset_Adpd400xarray[i]) = (uint16_t)array[i];
    } else if (strcmp(Adpd400x_str_array[2*i+1], "int16_t") == 0) {
      *(int16_t *)((char *)dcblcfg + stroffset_Adpd400xarray[i]) = (int16_t)array[i];
    }
    i++;
  }
  return STRUCTOPSUCCESS;
}

#if 0
/**
* @brief    To read Lcfg Offset array structure
* @param    *field Lcfg field index that needs to be read
* @param    value Value of Lcfg
* @return   StructOpErrorStatus returns error status
*/

StructOpErrorStatus ReadLcfgStructure(char *field, int32_t *value) {
    int32_t i = 0;
    int32_t val = 0;
    float fval;


    while (i < LCFG_SIZE) {
        if (stricmp(field, str_array[2*i]) == 0) {
            if (strcmp(str_array[2*i+1], "uint32_t") == 0) {
                val = *(uint32_t *)((char *)aStruct + stroffset_array[i]);
            } else if (strcmp(str_array[2*i+1], "uint8_t") == 0) {
                val = *(uint8_t *)((char *)aStruct + stroffset_array[i]);
            } else if (strcmp(str_array[2*i+1], "int8_t") == 0) {
                val = *(int8_t *)((char *)aStruct + stroffset_array[i]);
            } else if (strcmp(str_array[2*i+1], "uint16_t") == 0) {
                val = *(uint16_t *)((char *)aStruct + stroffset_array[i]);
            } else if (strcmp(str_array[2*i+1], "int16_t") == 0) {
                val = *(int16_t *)((char *)aStruct + stroffset_array[i]);
            } else if (strcmp(str_array[2*i+1], "float") == 0) {
                fval = *(float *)((char *)aStruct + stroffset_array[i]);
		val = (int32_t)fval;
            }
            *value = (int32_t)val;
          return STRUCTOPSUCCESS;
        }
        i++;
    }

    return STRUCTOPERROR;
}

/**
* @brief    To clear Lcfg Offset array structure
* @return   None
*/
void ClearLcfgStruct()  {
    int32_t i;
    for(i = 0; i < LCFG_SIZE; i++) {
        if(strcmp(str_array[2*i+1],"uint32_t") == 0) {
            *(uint32_t *)((char *)aStruct + stroffset_array[i]) = (uint32_t)0;
        } else if (strcmp(str_array[2*i+1], "uint8_t") == 0) {
            *(uint8_t *)((char *)aStruct + stroffset_array[i]) = (uint8_t)0;
        } else if (strcmp(str_array[2*i+1], "int8_t") == 0) {
            *(int8_t *)((char *)aStruct + stroffset_array[i]) = (int8_t)0;
        } else if (strcmp(str_array[2*i+1], "uint16_t") == 0) {
            *(uint16_t *)((char *)aStruct + stroffset_array[i]) = (uint16_t)0;
        } else if (strcmp(str_array[2*i+1], "float") == 0) {
            *(float *)((char *)aStruct + stroffset_array[i]) = (float)0;
        }
    }
}
#endif
#endif//ENABLE_PPG_APP
/**@}*/ /* end of group User */
