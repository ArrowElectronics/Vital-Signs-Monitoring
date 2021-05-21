/**
***************************************************************************
* @file         AdiFixedPointAlgo.c
* @author       ADI
* @version      V1.2.0
* @date         07-Apr-2018
* @brief        Wrapper for ADI fixed point algorithm.
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
* This software is intended for use with the ADPD and derivative parts        *
* only                                                                        *
*                                                                             *
******************************************************************************/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <math.h>
#include "adpd400x_lib.h"
#include "adpd400x_lib_common.h"
#include "heart_rate_internal.h"
/* Include API header */
#include "adi_vsm_hrm.h"

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

#ifdef HRV
#include "adi_vsm_hrv.h"
#define STATE_HRV_MEM_NUM_CHARS     3000 // requirement in 0.1.0 = 2163
#define SCRATCH_HRV_MEM_NUM_CHARS   1200

static unsigned char STATE_memory_HRV[STATE_HRV_MEM_NUM_CHARS];

adi_hrv_ppg_b2b_mem_t Adpd400xvsmhrv_memory_setup;

adi_hrv_ppg_b2b_config_t Adpd400xvsmhrv_config_params;

adi_hrv_ppg_b2b_instance_t *Adpd400xvsmhrv_instance;

adi_hrv_ppg_b2b_return_code_t Adpd400xvsmhrv_return_code;

adi_hrv_ppg_b2b_output_status_t Adpd400xvsmhrv_output;
static uint32_t gnHrvCycleCountStart = 0;
static uint32_t gnHrvCycleCountEnd = 0;
static uint32_t gnHrvCycleCount = 0;
int16_t  gnAdpd400xHrvAlgoSampleRate;
#define HRV_CYCLE_AVERAGE 50
static uint8_t gnHrvCycleAverage = HRV_CYCLE_AVERAGE;
#endif

#define MODULE ("AdiFixedPointAlgo.c")

static int16_t AlgHrSelection(LibResult_t *result);
void Adpd400xAlgHrFrontEnd_Reset(void);

extern Adpd400xLibConfig_t *gAdpd400x_lcfg;
int16_t gnAdpd400xSpotAlgoSampleRate;
int16_t gnAdpd400xSpotAlgoDecimation;
int16_t gnAdpd400xMinDiffTrackSpot;

/* initial threshold for tracking HR confidence in Q6.10 format
   in deciding to stop the SpotHR instance
*/
int16_t gnAdpd400xInitialConfidenceThreshold;/* 70% */
/* Scalimg factor for PPG signal */
uint32_t gnAdpd400xPpgScale;
/* Scalimg factor for accelerometer value */
int16_t gnAdpd400xAccelScale;

uint8_t gnAdpd400xSpotStabilityCountMac;
/* timeout for stopping the spotHR instance */
int16_t  gnAdpd400xSpotHrTimeoutSecs;
int16_t  gnAdpd400xZeroOrderHoldNumSamples;
int16_t  gnAdpd400xTrackAlgoSampleRate;
int16_t  gnAdpd400xTrackHrTimeoutSecs;
uint32_t  gnAdpd400xSpotWindowlength;
uint32_t  gnAdpd400xTrackerMinHeartrateBpm;
int16_t  gnAdpd400xHrmPlusTimeoutSecs;
int16_t  gnAdpd400xHrmPlusAlgoSampleRate;


/******************************************************************************
**          #defines
******************************************************************************/

/* convert from 12.4 fract to float */
// #define FIXED16Q4_TO_FLOAT(x) (((float)(x))/16.0f)
/* convert from 6.10 fract to float */
// #define FIXED16Q10_TO_FLOAT(x) (((float)(x))/1024.0f)
/* convert from 12.20 fract to float */
#define FIXED12Q20_TO_FLOAT(x) (((float)(x))/1048576.0f)
/* convert from float to 12.20 fract */
#define FLOAT_TO_FIXED12Q20(x) (int)((float)(x)*1048576.0f)
/* convert from 2.30 fract to float */
#define FIXED2Q30_TO_FLOAT(x) (((float)(x))/(float)(1<<30))

#ifdef HRMPLUS
/* Allocate a max amount of memory for the HRMPlus Algo state memory block */
#define STATE_HRMPLUS_MEM_NUM_CHARS   9200 // requirement in 1.2.0 = 9096
#define SCRATCH_HRMPLUS_MEM_NUM_CHARS 1500 // requirement in 1.2.0 = 1139

static unsigned char STATE_memory_HRMPLUS[STATE_HRMPLUS_MEM_NUM_CHARS];

static unsigned char SCRATCH_memory_HRMPLUS[SCRATCH_HRMPLUS_MEM_NUM_CHARS];

adi_vsm_hrm_plus_mem_t Adpd400xvsmhrm_plus_memory_setup;

adi_vsm_hrm_plus_instance_t *Adpd400xvsmhrm_plus_instance;

adi_vsm_hrm_plus_return_code_t Adpd400xvsmhrm_plus_return_code;

adi_vsm_hrm_plus_output_status_t Adpd400xvsmhrm_plus_output;
int32_t gAdpd400xnHrmPlusConfFlag;
static uint32_t gnHrmPlusCycleCountStart = 0;
static uint32_t gnHrmPlusCycleCountEnd = 0;
static uint32_t gnHrmPlusCycleCount = 0;

#define HRMPLUS_CYCLE_AVERAGE 50
static uint8_t gnHrmPlusCycleAverage = HRMPLUS_CYCLE_AVERAGE;
#else
/* Allocate a max amount of memory for the Track Algo state memory block */
/* Actual value 4712 for (with min HR = 30 and max HR = 220) */
#define STATE_TRACK_MEM_NUM_CHARS   6800 // requirement in 1.2.0 = 6607

/* Allocate a max amount of memory for the Spot Algo state memory block */
/* Actual value 2828 for (with min HR =30 and max HR = 220) */
#define STATE_SPOT_MEM_NUM_CHARS   2400  // requirement in 1.2.0 = 2227
#define SCRATCH_SPOT_MEM_NUM_CHARS 1500 // requirement in 1.2.0 = 1139

/* Memory for tracking HR */

/* Define state memory block */
static unsigned char STATE_memory[STATE_TRACK_MEM_NUM_CHARS];

adi_vsm_hrm_mem_t vsmhrm_memory_setup;

adi_vsm_hrm_instance_t *vsmhrm_instance;

adi_vsm_hrm_config_t vsmhrm_config_params;

adi_vsm_hrm_parameters_t vsmhrm_tuning_params;

adi_vsm_hrm_return_code_t vsmhrm_return_code;

adi_vsm_hrm_output_status_t vsmhrm_output;

/* Memory for spot HR */

/* Define state memory block for a spot HR instance */
static unsigned char STATE_memory_spotHR[STATE_SPOT_MEM_NUM_CHARS];

/* Define scratch spot memory block */
static unsigned char SCRATCH_memory_spotHR[SCRATCH_SPOT_MEM_NUM_CHARS];

adi_vsm_spothr_mem_t spothr_memory_setup;

adi_vsm_spothr_instance_t *spothr_instance;

adi_vsm_spothr_config_t spothr_config_params;

adi_vsm_spothr_return_code_t spothr_return_code;

adi_vsm_spothr_output_status_t spothr_output;
int32_t gnRunSpotHrFlag, gnSpotConfFlag;

//float modulation_index;

static uint32_t gnSpotCycleCountStart = 0;
static uint32_t gnSpotCycleCountEnd = 0;
static uint32_t gnSpotCycleCount = 0;

#define SPOT_CYCLE_AVERAGE 50
static uint8_t gnSpotCycleAverage = SPOT_CYCLE_AVERAGE;

static uint32_t gnTrackCycleCountStart = 0;
static uint32_t gnTrackCycleCountEnd = 0;
static uint32_t gnTrackCycleCount = 0;
#define TRACK_CYCLE_AVERAGE 50
static uint8_t gnTrackCycleAverage = TRACK_CYCLE_AVERAGE;
static uint8_t gnEnableTrackHRConf, gnEnableSpotTrackHR, gnSpotStabilityCount;
#endif  // HRMPLUS
int32_t gnAdpd400xTotalNumSamples;

#define LOOPCOUNT 2
#define CORE_FREQUENCY_MULTIPLIER 26000  // cycles per millisecond

void Adpd400xAlgHRCfg() {
    gnAdpd400xSpotAlgoSampleRate =  gAdpd400x_lcfg->spotalgosamplerate;
    gnAdpd400xSpotAlgoDecimation =  gAdpd400x_lcfg->spotalgodecimation;
    gnAdpd400xMinDiffTrackSpot =   gAdpd400x_lcfg->mindifftrackSpot;
    gnAdpd400xInitialConfidenceThreshold = gAdpd400x_lcfg->initialconfidencethreshold;
    gnAdpd400xPpgScale = gAdpd400x_lcfg->ppgscale;
    gnAdpd400xAccelScale =  gAdpd400x_lcfg->accelscale;
    gnAdpd400xSpotStabilityCountMac =  gAdpd400x_lcfg->spotstabilitycount;
    gnAdpd400xSpotHrTimeoutSecs = gAdpd400x_lcfg->spothrtimeoutsecs;
    gnAdpd400xZeroOrderHoldNumSamples = 50 / gAdpd400x_lcfg->hrmInputRate;
    gnAdpd400xTrackAlgoSampleRate = gAdpd400x_lcfg->trackalgosamplerate;
    gnAdpd400xTrackHrTimeoutSecs =  gAdpd400x_lcfg->trackhrtimeoutsecs;
    gnAdpd400xSpotWindowlength = gAdpd400x_lcfg->spotwindowlength;
    gnAdpd400xTrackerMinHeartrateBpm = gAdpd400x_lcfg->trackerminheartratebpm;
    gnAdpd400xHrmPlusTimeoutSecs = 10;
    gnAdpd400xHrmPlusAlgoSampleRate = 50;
#ifdef HRV
    gnAdpd400xHrvAlgoSampleRate = 50;
#endif
}
int16_t Adpd400xAlgHRInit() {

    Adpd400xAlgHRCfg();

#ifdef HRMPLUS
    /* initialize the memory object for the HRMPlus instance */
    Adpd400xvsmhrm_plus_memory_setup.state.block = STATE_memory_HRMPLUS;
    Adpd400xvsmhrm_plus_memory_setup.state.length_numchars =
        adi_vsm_hrm_plus_numchars_state_memory();
    Adpd400xvsmhrm_plus_memory_setup.scratch.block = SCRATCH_memory_HRMPLUS;
    Adpd400xvsmhrm_plus_memory_setup.scratch.length_numchars =
        adi_vsm_hrm_plus_numchars_scratch_memory();

    /* Create the Heart Rate Measurement instance */
    Adpd400xvsmhrm_plus_instance = adi_vsm_hrm_plus_create(&Adpd400xvsmhrm_plus_memory_setup);
    if (Adpd400xvsmhrm_plus_instance == NULL) {
        debug(MODULE, "Cannot create Heart Rate Measurement HRMPlus instance\n");
        exit(0);
    }
    gnAdpd400xTotalNumSamples = 0;
    gAdpd400xnHrmPlusConfFlag = 0;
#else
   // int16_t sample_rate;
    int16_t min_heart_rate_bpm = gAdpd400xnTrackerMinHeartrateBpm;
    //int16_t internal_decimation = gnSpotAlgoDecimation;

    // Uncomment the following to enable low sample rate
    /* if (getSetDataRate((unsigned int)&sample_rate) != 0)
           return -1;  // Error reading sample rate
    */

    /* Create-time Configuration of the Heart Rate Measurement instance */
    vsmhrm_config_params.sample_rate         =  gAdpd400xnTrackAlgoSampleRate;
    vsmhrm_config_params.min_heart_rate_bpm  = min_heart_rate_bpm;
    //vsmhrm_config_params.internal_decimation = internal_decimation;

    /* initialize the memory object for the Heart Rate Measurement instance */
    vsmhrm_memory_setup.state.block = STATE_memory;
    vsmhrm_memory_setup.state.length_numchars =
        adi_vsm_hrm_numchars_state_memory(&vsmhrm_config_params);

    /* Create the Heart Rate Measurement instance */
    vsmhrm_instance = adi_vsm_hrm_create(&vsmhrm_memory_setup,
                                         &vsmhrm_config_params);
    if (vsmhrm_instance == NULL) {
      debug(MODULE, "Cannot create Heart Rate Measurement instance\n");
      exit(0);
    }
    /* Get the default algorithm parameters */
    adi_vsm_hrm_get_params(&vsmhrm_tuning_params, vsmhrm_instance);
    /* Tune algorithm parameters */
    vsmhrm_tuning_params.max_heart_rate_bpm = 220;
    adi_vsm_hrm_set_params(vsmhrm_instance, &vsmhrm_tuning_params);

    /* reset all state variables within the algorithm to initial conditions */
    adi_vsm_hrm_reset(vsmhrm_instance);

    /* Create-time Configuration of the Spot Heart Rate Measurement instance
    */
    spothr_config_params.sample_rate         = gnSpotAlgoSampleRate;
    spothr_config_params.min_heart_rate_bpm  = min_heart_rate_bpm;
    spothr_config_params.max_heart_rate_bpm  = vsmhrm_tuning_params.max_heart_rate_bpm;
    spothr_config_params.data_window_length_ms = gAdpd400xnSpotWindowlength;
    /* initialize the memory object for the Heart Rate Measurement instance
    */
    spothr_memory_setup.state.block = STATE_memory_spotHR;
    spothr_memory_setup.state.length_numchars =
        adi_vsm_spothr_numchars_state_memory(&spothr_config_params);
    spothr_memory_setup.scratch.block = SCRATCH_memory_spotHR;
     spothr_memory_setup.scratch.length_numchars =
       adi_vsm_spothr_numchars_scratch_memory(&spothr_config_params);

    /* Create the Spot Heart Rate Measurement instance
    */
    spothr_instance = adi_vsm_spothr_create(&spothr_memory_setup,
                                          &spothr_config_params);
    if (spothr_instance == NULL) {
      debug(MODULE, "Cannot create Spot Heart Rate Measurement instance\n");
      exit(0);
    }

    gnRunSpotHrFlag = 1;
    gnSpotConfFlag = 0;

    /*
     * Tracking algorithm confidence initialised to 0. Needed to ensure
     * Spot HR doesn't immediately time out.
     */
    vsmhrm_output.confidence = 0;

    gnAdpd400xTotalNumSamples = gnEnableTrackHRConf = gnEnableSpotTrackHR = gnSpotStabilityCount = 0;
#endif  // HRMPLUS
#ifdef HRV
    if (gAdpd400x_lcfg->hrvEnable) {
      /* Create-time Configuration of the Heart Rate Variability instance */
      Adpd400xvsmhrv_config_params.sample_rate           = gnAdpd400xHrvAlgoSampleRate;
      Adpd400xvsmhrv_config_params.max_reported_gap_ms   = 5000;
      /* Initialize the memory object for the Heart Rate Variability instance */
      Adpd400xvsmhrv_memory_setup.state.block = STATE_memory_HRV;
      Adpd400xvsmhrv_memory_setup.state.length_numchars =
         adi_hrv_ppg_b2b_numchars_state_memory(&Adpd400xvsmhrv_config_params);
      /* Create the Heart Rate Variability instance */
      Adpd400xvsmhrv_instance = adi_hrv_ppg_b2b_create(
                                  &Adpd400xvsmhrv_memory_setup, &Adpd400xvsmhrv_config_params);
    }
#endif
    return 0;
}

ADPDLIB_ERROR_CODE_t Adpd400xAlgHRProcess(uint32_t nSlotSumA,
                                  uint32_t nSlotSumB,
                                  int16_t *acceldata,
                                  LibResult_t *result,
                                  HR_Log_Result_t *g_resultlog) {

    int32_t nPpgSignal;
    int32_t nAccelX, nAccelY, nAccelZ;
    uint32_t nVariable;
    int32_t nLoop;

    nSlotSumA = nSlotSumA;  // to remove warning

    nPpgSignal = (nSlotSumB * gnAdpd400xPpgScale);

    // debug(MODULE, "\r\nIR = %d, nPpgSignal = %d", IR, nPpgSignal);
    nAccelX = (acceldata[0] * gnAdpd400xAccelScale);
    nAccelY = (acceldata[1] * gnAdpd400xAccelScale);
    nAccelZ = (acceldata[2] * gnAdpd400xAccelScale);
    // debug(MODULE, " X = %d, Y = %d Z = %d", nAccelX, nAccelY, nAccelZ);
    g_resultlog->PPG_SCALED = nPpgSignal;
    g_resultlog->ACCEL_SCALED[0] = nAccelX;
    g_resultlog->ACCEL_SCALED[1] = nAccelY;
    g_resultlog->ACCEL_SCALED[2] = nAccelZ;
    g_resultlog->CYCLE_T1 = g_resultlog->CYCLE_T2 = 0;
    g_resultlog->CYCLE_S1 = g_resultlog->CYCLE_S2 = 0;
    g_resultlog->MIPS_T = 0;
    g_resultlog->MIPS_S = 0;
    g_resultlog->DEV_RATIO = gAdpd400x_dataRateRatio;

#ifdef HRMPLUS
    /* Perform the HRMPlus Algorithm */
    for (nLoop = 0; nLoop < gnAdpd400xZeroOrderHoldNumSamples; nLoop++) {
    gnHrmPlusCycleCountStart = AdpdLibGetTick() >> 3;
    Adpd400xvsmhrm_plus_return_code = adi_vsm_hrm_plus_process(Adpd400xvsmhrm_plus_instance,
                                                       nPpgSignal,
                                                       nAccelX,
                                                       nAccelY,
                                                       nAccelZ,
                                                       &Adpd400xvsmhrm_plus_output);
    if (Adpd400xvsmhrm_plus_return_code == ADI_VSM_HRM_PLUS_INVALID_POINTER) {
      debug(MODULE, "\nHeart Rate Measurement processing error: Invalid pointer encountered\n");
      exit(-1);
    }

    gnHrmPlusCycleCountEnd = AdpdLibGetTick() >> 3;
    gnHrmPlusCycleCount += (gnHrmPlusCycleCountEnd - gnHrmPlusCycleCountStart);
    g_resultlog->CYCLE_S1 = (gnHrmPlusCycleCountEnd - gnHrmPlusCycleCountStart) * CORE_FREQUENCY_MULTIPLIER;
    if (--gnHrmPlusCycleAverage == 0) {
      g_resultlog->MIPS_S = ((gnHrmPlusCycleCount * CORE_FREQUENCY_MULTIPLIER * gnAdpd400xHrmPlusAlgoSampleRate) / (HRMPLUS_CYCLE_AVERAGE));
      // debug(MODULE, "Sample No: %d HrmPlus Avg Cycles per call = %u ", gnTotalNumSamples, g_resultlog->MIPS_S);
      gnHrmPlusCycleAverage = HRMPLUS_CYCLE_AVERAGE;
      gnHrmPlusCycleCount = 0;
    }
    } // end of loop counter of 2
#else
    /* Perform the Track HRM Algorithm */
    for (nLoop = 0; nLoop < gAdpd400xnZeroOrderHoldNumSamples; nLoop++) {
      gnTrackCycleCountStart = AdpdLibGetTick() >> 3;
      vsmhrm_return_code = adi_vsm_hrm_process(vsmhrm_instance,
                                               nPpgSignal,
                                               nAccelX,
                                               nAccelY,
                                               nAccelZ,
                                               &vsmhrm_output);
          /*
          * Check the return code.  Abort if it's an error or timeout reached;
          * display a message and continue if it's a warning.
          */
      switch (vsmhrm_return_code)
      {
      case ADI_VSM_HRM_VALID:
        /* Nothing */
        break;

      case ADI_VSM_HRM_INVALID_POINTER:
        debug(MODULE, "Heart Rate Measurement processing error: Invalid pointer encountered\r\n");
        exit(-1);
      }
      gnTrackCycleCountEnd = AdpdLibGetTick() >> 3;
      gnTrackCycleCount += (gnTrackCycleCountEnd - gnTrackCycleCountStart);
      if (nLoop == 0) {
        g_resultlog->CYCLE_T1 = (gnTrackCycleCountEnd - gnTrackCycleCountStart) * CORE_FREQUENCY_MULTIPLIER;
      } else {
        g_resultlog->CYCLE_T2 = (gnTrackCycleCountEnd - gnTrackCycleCountStart) * CORE_FREQUENCY_MULTIPLIER;
      }
      if (gnTrackCycleAverage-- == 0) {

        g_resultlog->MIPS_T = ((gnTrackCycleCount * CORE_FREQUENCY_MULTIPLIER * vsmhrm_config_params.sample_rate) / (TRACK_CYCLE_AVERAGE));
        // debug(MODULE, "Tracking Avg Cycles per call = %u", g_resultlog->MIPS_T);
             gnTrackCycleAverage = TRACK_CYCLE_AVERAGE;
             gnTrackCycleCount = 0;
      }
    }  // nLoop for Track HR Algo


    /* Always call spothr_submit so we can get signal quality info from it */

    gnSpotCycleCountStart = AdpdLibGetTick() >> 3;
    for (nLoop = 0; nLoop < gAdpd400xnZeroOrderHoldNumSamples; nLoop++) {
    	spothr_return_code = adi_vsm_spothr_submit(spothr_instance,
        	                                      nPpgSignal,
            	                                  nAccelX,
                	                              nAccelY,
                    	                          nAccelZ,
                        	                      &spothr_output);

    	/* record the modulation index returned from the Tracker instance only if
       		the Spot HR instance does not report high_motion
    	*/
    	if (spothr_output.high_motion) {
      	//  modulation_index = 0.0;
    	} else {
      	// modulation_index = FIXED12Q20_TO_FLOAT(vsmhrm_output.modulation_index);
    	}
    }
    if (gnRunSpotHrFlag) {

        if (spothr_return_code == ADI_VSM_SPOTHR_READY) {
          spothr_return_code = adi_vsm_spothr_get_hr(spothr_instance,
                                                     &spothr_output);
        }
        gnSpotCycleCountEnd = AdpdLibGetTick() >> 3;
        gnSpotCycleCount += (gnSpotCycleCountEnd - gnSpotCycleCountStart);
        g_resultlog->CYCLE_S1 = (gnSpotCycleCountEnd - gnSpotCycleCountStart) * CORE_FREQUENCY_MULTIPLIER;
        if (--gnSpotCycleAverage == 0) {
          g_resultlog->MIPS_S = ((gnSpotCycleCount * CORE_FREQUENCY_MULTIPLIER * spothr_config_params.sample_rate) / (SPOT_CYCLE_AVERAGE));
          // debug(MODULE, "Sample No: %d Spot Avg Cycles per call = %u ", gnTotalNumSamples, g_resultlog->MIPS_S);
          gnSpotCycleAverage = SPOT_CYCLE_AVERAGE;
          gnSpotCycleCount = 0;
       }

       if (spothr_return_code == ADI_VSM_SPOTHR_READY) {
                /* Spot HR processing succeeded and returned a valid HR estimate
                    use it to update the tracking HR instance
                */
                /* round Spot HR estimate to nearest integer */

                /* New API for feeding the current HR estimate */
                //adi_vsm_hrm_adjust_hr(vsmhrm_instance, (spothr_output.heart_rate_bpm + 0x8) >> 4);
                 adi_vsm_hrm_adjust_hr(vsmhrm_instance,spothr_output.heart_rate_bpm);
                gnRunSpotHrFlag = 0; /* no longer need to run Spot HR alg */
                gnSpotConfFlag = 1;
                debug(MODULE, "Switching to Tracking algo based on a valid spot heart rate measure \r\n");
       } else if (spothr_return_code != ADI_VSM_SPOTHR_NOT_READY) {
         debug(MODULE, "Got bad return code %8x from the Spot HR instance.\r\n",spothr_return_code);
       }

       /* For now we only allow Spot HR to run for the first so many
          seconds or until the tracking instance reports a high confidence
          in its HR estimate
       */
       if ((gnAdpd400xTotalNumSamples > gnSpotAlgoSampleRate * gAdpd400xnSpotHrTimeoutSecs)
           || (vsmhrm_output.confidence > gnAdpd400xInitialConfidenceThreshold)
       ) {
           gnRunSpotHrFlag = 0;
           debug(MODULE, "Switching to Tracking algo based on timeout\r\n");
       }
     }
#endif
#ifdef HRV
    if (gAdpd400x_lcfg->hrvEnable) {
      /* Perform the HRV Algorithm */
        gnHrvCycleCountStart = AdpdLibGetTick() >> 3;
        Adpd400xvsmhrv_return_code = adi_hrv_ppg_b2b_process(Adpd400xvsmhrv_instance,
                                                           nPpgSignal,
                                                           &Adpd400xvsmhrv_output);
        switch(Adpd400xvsmhrv_return_code) {
          /* expecting one of these return codes */
          case ADI_HRV_PPG_B2B_VALID_RR:
            if (Adpd400xvsmhrv_output.interval_ms != 0) { // added due to bug in HRV algo
              g_resultlog->RRinterval = Adpd400xvsmhrv_output.interval_ms;
              result->RRinterval = Adpd400xvsmhrv_output.interval_ms;
              g_resultlog->IsHrvValid = 1;
              result->IsHrvValid = 1;
            } else {
              g_resultlog->RRinterval = 0;
              result->RRinterval = 0;
              g_resultlog->IsHrvValid = 0;
              result->IsHrvValid = 0;
            }
            break;
          case ADI_HRV_PPG_B2B_GAP:
            g_resultlog->RRinterval = Adpd400xvsmhrv_output.interval_ms;
            result->RRinterval = Adpd400xvsmhrv_output.interval_ms;
            g_resultlog->IsHrvValid = 0;
            result->IsHrvValid = 0;
            break;
          case ADI_HRV_PPG_B2B_NEED_MORE_INPUT:
            g_resultlog->RRinterval = 0;
            result->RRinterval = 0;
            g_resultlog->IsHrvValid = 0;
            result->IsHrvValid = 0;
            break;
          case ADI_HRV_PPG_B2B_INVALID_POINTER:
            debug(MODULE, "Heart Rate Variability processing error code %d\r\n",Adpd400xvsmhrv_return_code);
            break;
        }
        gnHrvCycleCountEnd = AdpdLibGetTick() >> 3;
        gnHrvCycleCount += (gnHrvCycleCountEnd - gnHrvCycleCountStart);
        g_resultlog->CYCLE_S1 = (gnHrvCycleCountEnd - gnHrvCycleCountStart) * CORE_FREQUENCY_MULTIPLIER;
         if (--gnHrvCycleAverage == 0) {
           g_resultlog->MIPS_S = ((gnHrvCycleCount * CORE_FREQUENCY_MULTIPLIER * gnAdpd400xHrvAlgoSampleRate) / (HRV_CYCLE_AVERAGE));
           // debug(MODULE, "Sample No: %d Hrv Avg Cycles per call = %u ", gnTotalNumSamples, g_resultlog->MIPS_S);
           gnHrvCycleAverage = HRV_CYCLE_AVERAGE;
           gnHrvCycleCount = 0;
         }
    }
#endif
        gnAdpd400xTotalNumSamples++;

#ifdef HRMPLUS
        result->HR = AlgHrSelection(result); /*HR in 12.4 fromat*/
        nVariable  = result->HR * gAdpd400x_dataRateRatio;
        result->HR = (nVariable) >> 10;
        g_resultlog->HR_SELECTED = result->HR;
        g_resultlog->CONFIDENCE = result->confidence;
        g_resultlog->HR_TYPE = result->HR_Type;
#else
        result->HR = AlgHrSelection(result);
        result->HR = (result->HR * g_dataRateRatio) >> 10;
        g_resultlog->CONFIDENCE = result->confidence;
        g_resultlog->HR_TYPE = result->HR_Type;
        g_resultlog->HR_SELECTED = result->HR;
        g_resultlog->HR_SPOT = (uint32_t)((spothr_output.heart_rate_bpm + 0x8) >> 4);
        // g_resultlog->CONFIDENCE_S = (int16_t)spothr_output.confidence;

        g_resultlog->HR_TRACK = (uint32_t)(vsmhrm_output.heart_rate_bpm >> 4);
        g_resultlog->CONFIDENCE_T = (vsmhrm_output.confidence >> 10);
        /* Convert HR output to float */
        // HR_alg = FIXED16Q4_TO_FLOAT(vsmhrm_output.heart_rate_bpm);
#endif
        return ADPDLIB_ERR_SUCCESS_WITH_RESULT;
}

#ifdef HRMPLUS
static int16_t AlgHrSelection(LibResult_t *result) {
    int16_t nOutHr = 0;
    int16_t nHrPlus;

    nHrPlus = (Adpd400xvsmhrm_plus_output.heart_rate_bpm);
    /* gnInitialConfidenceThreshold is set at 75%  ~= 768 */
    if ((gnAdpd400xTotalNumSamples > (gnAdpd400xHrmPlusAlgoSampleRate * gnAdpd400xHrmPlusTimeoutSecs)) ||
        (gAdpd400xnHrmPlusConfFlag == 1) ||
        (Adpd400xvsmhrm_plus_output.confidence > ((gnAdpd400xInitialConfidenceThreshold * 750)/700))) {
                  nOutHr = nHrPlus;
                  result->HR = nHrPlus;
                  result->HR_Type = 1;
                  result->confidence = (Adpd400xvsmhrm_plus_output.confidence); /*confidence in 6.10 fromat*/
                  gAdpd400xnHrmPlusConfFlag = 1;
    } else {
      nOutHr = result->HR = result->confidence = result->HR_Type = 0;
    }

    return (nOutHr);
}

void Adpd400xAlgHrFrontEnd_Reset()  {
  adi_vsm_hrm_plus_frontend_reset(Adpd400xvsmhrm_plus_instance);
}

#else

void Adpd400xAlgHrFrontEnd_Reset()  {
  adi_vsm_hrm_frontend_reset(vsmhrm_instance);
}

static int16_t AlgHrSelection(LibResult_t *result) {
    int16_t nOutHr = 0;

    int16_t nTrackHr, nSpotHr;

    nTrackHr = (vsmhrm_output.heart_rate_bpm >> 4);
    nSpotHr  = (int16_t)((spothr_output.heart_rate_bpm + 0x8) >> 4);

    if (gnSpotConfFlag) {
                nOutHr = nSpotHr;
                result->HR = nSpotHr;
                result->HR_Type = 2;
                // result->confidence = (int16_t)spothr_output.confidence;
                gnSpotConfFlag = 0;
    } else if ((gnAdpd400xTotalNumSamples > gnSpotAlgoSampleRate * gAdpd400xnSpotHrTimeoutSecs) ||
               (vsmhrm_output.confidence > gnAdpd400xInitialConfidenceThreshold) ||
                gnEnableTrackHRConf) {
                  nOutHr = nTrackHr;
                  result->HR = nTrackHr;
                  result->HR_Type = 1;
                  result->confidence = (vsmhrm_output.confidence >> 10);
                gnEnableTrackHRConf = 1;
    } else if (gnEnableSpotTrackHR) {
                nOutHr = nTrackHr;
                result->HR = nTrackHr;
                result->HR_Type = 3;
                result->confidence = (vsmhrm_output.confidence >> 10);
    } else if ((abs(nTrackHr - nSpotHr) <= gnMinDiffTrackSpot)) {
         if (++gnSpotStabilityCount == gAdpd400xnSpotStabilityCountMac) {
                nOutHr = nTrackHr;
                gnEnableSpotTrackHR = 1;
                gnSpotStabilityCount = 0;
                result->HR = nTrackHr;
                result->HR_Type = 3;
                result->confidence = (vsmhrm_output.confidence >> 10);
         } else {
           nOutHr = result->HR = result->confidence = result->HR_Type = 0;
         }
    } else {
                nOutHr = result->HR = result->confidence = result->HR_Type = 0;
                gnSpotStabilityCount = 0;
    }

    return (nOutHr);
}
#endif
