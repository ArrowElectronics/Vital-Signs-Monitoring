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

#define MODULE ("DetectOff.c")
#define SKIP_NUM            5
#define AVG_SIZE            6       // size of data to do averaging

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
void Adpd400xDetectObjectOffInit(void);
INT_ERROR_CODE_t Adpd400xDetectObjectOFF(uint32_t mean_val, uint32_t var);

/* External global variables ------------------------------------------------*/

/* Internal variables -------------------------------------------------------*/
static uint8_t OccurCounter1, OccurCounter2, DetectState;
static uint32_t LevelThresholdOFF;    // reference ON/OFF TH

/**
  * @brief Initialization rountine for object detach check.
  * @param  Number of data to do averaging.
  * @retval none
  */
#if 0
void Adpd400xDetectObjectOffInit() {
    LevelThresholdOFF = gAdpd400xDetectVal.curDcLevelG * gAdpd400x_lcfg->triggerOffPercentage;
    LevelThresholdOFF /= 100;

    //debug(MODULE, "OFF Level= %u\t Stable=%u", \
        LevelThresholdOFF, gAdpd400x_lcfg->triggerOffStablizeVR);
	NRF_LOG_DEBUG("OFF Level= %u\t Stable=%u", \
         LevelThresholdOFF, gAdpd400x_lcfg->triggerOffStablizeVR);

    // UtilGetMeanVarInit(AVG_SIZE, 1);
    DetectState = 0;
    OccurCounter1 = OccurCounter2 = 0;
    return;
}
#endif

/**
  * @brief Check if object is detached
  * @param1 mean current signal's mean value
  * @param1 mean current signal's variance value
  * @retval SUCCESS = detect OFF, IN_PROGRESS = detect OFF fail, keep checking
  */
#if 0
INT_ERROR_CODE_t Adpd400xDetectObjectOFF(uint32_t mean_val, uint32_t var) {
    uint8_t off_true;

    if (DetectState == 0) {
        //  0=processing 1=with result 2=invalid
        // if (UtilGetMeanVar(rawDataB, &mean_val, &var) == IERR_IN_PROGRESS)
        //    return IERR_IN_PROGRESS;
        // UtilGetMeanVarInit(AVG_SIZE, 1);

        off_true = 0;
        if (mean_val < LevelThresholdOFF)  {  // Object detached
            off_true = 1;
            OccurCounter1++;
        }
        if (var > gAdpd400x_lcfg->triggerOffStablizeVR)  {  // Not stable
            off_true = 1;
            OccurCounter2++;
        }
        if (var == 0)  {   // Saturated
            off_true = 1;
            OccurCounter2++;
        }

        if (off_true == 0) {
            OccurCounter1 = OccurCounter2 = 0;
            return IERR_IN_PROGRESS;
        }

        NRF_LOG_DEBUG("Mean = %u\t VAR = %u", mean_val, var);
        // Event continue occur
        if (OccurCounter1 >= gAdpd400x_lcfg->detectOffSettlingCnt) {
            OccurCounter1 = OccurCounter2 = 0;
            DetectState = 1;        // Low signal
        }
        if (OccurCounter2 >= gAdpd400x_lcfg->detectOffSettlingCnt) {
            OccurCounter1 = OccurCounter2 = 0;
            DetectState = 0;        // unstable
            return IERR_SUCCESS;
        }
        return IERR_IN_PROGRESS;
    }

    if (DetectState == 1)
        return IERR_SUCCESS;

    return IERR_IN_PROGRESS;

}
#endif