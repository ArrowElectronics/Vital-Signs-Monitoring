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
* This software is intended for use with the ADPD and derivative parts    *
* only                                                                        *
*                                                                             *
******************************************************************************/
#if 1
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "adpd400x_lib.h"
#include "adpd400x_lib_common.h"
//#include "signal_metrics.h"
#include "adi_vsm_sqi.h"

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

/* Public define ------------------------------------------------------------*/
#define MODULE ("OptimizationAC.c")

// #define _TEST_PPROC_
#ifdef _TEST_PPROC_
#include "TestAdpdData.h"
#define TestDataLen     250
SignalMetrics_t TemResult[TestDataLen+1];
static uint16_t gsResultCnt;
#endif

//SM_RetCode_t gAgcRetCode;
#define ADD_PULSE               4
#define SUB_PULSE               3
#define MIN_PULSE               4 //2 //changed from 2 -> 4
#define FUDGE                   0.1
#define MIN_CURRENT_STEP        10
#define ADD_CURRENT_STEP        0.4     // add 40%
#define MAX_SIGNAL_LEVEL_FOR_PULSE_ADJ 600000
#define SINGLE_PULSE_SATURATION_VALUE 8000

#define SQI_SAMPLING_RATE 50
#define STATE_MEM_PER_INSTANCE 3816
#define STATE_SQI_MEM_NUM_CHARS   STATE_MEM_PER_INSTANCE
#define SQI_RESULT_CONVERSION 1024.0
/* Allocate a max amount of memory for the SQI Algo state memory block */
static unsigned char STATE_memory_SQI[STATE_SQI_MEM_NUM_CHARS];

static adi_vsm_sqi_instance_t* sqi_instance;
static adi_vsm_sqi_mem_t adi_vsm_sqi_mem_handle;
static adi_vsm_sqi_config_t config_handle;
static adi_vsm_sqi_output_t adi_vsm_sqi_output;

/* Public function prototypes -----------------------------------------------*/
INT_ERROR_CODE_t Adpd400xACOptInit(uint16_t);
void Adpd400xACOptDeInit(void);
void Adpd400xACOptReset(uint8_t);

/* Public variables --------------------------------------------------------*/
extern uint16_t gnPulseWidth, gnAfeGainNum, gnBpfGainNum;

/* Private function prototypes ----------------------------------------------*/
//static uint8_t SetProfileSetting(SignalQt_t result, uint32_t);
static uint8_t SetProfileSetting(float result, uint32_t);
static uint8_t ACOptSetHighPowerProfile(uint32_t);
static uint8_t ACOptSetLowPowerProfile(uint32_t);

/* Private variables --------------------------------------------------------*/
static uint32_t gsStartTime;

// fs, dec, Led, trim, pulse
static uint16_t gsLow_Pf[] = {0xA0, 0x0, 0x8F, 0x0, MIN_PULSE}; /* AGC minimum current 15mA */
static uint16_t gsDcOptimumCurrent, gsAcMinimumCurrent;

/**
  * @brief  Initialization rountine for Setting Optimization.
  * @param  None.
  * @retval SUCCESS = done
  */
INT_ERROR_CODE_t Adpd400xACOptInit(uint16_t deviceID) { /* deviceID not used during init */
  //if (Preprocess_Metrics_Init(deviceID, gAdpd400x_lcfg->mt2Th, gAdpd400x_lcfg->mt3Th) != 0)
  //  return IERR_FAIL;
  
  config_handle.sampling_freq = SQI_SAMPLING_RATE;
  adi_vsm_sqi_mem_handle.state.block = STATE_memory_SQI;
  adi_vsm_sqi_mem_handle.state.length_numchars = STATE_MEM_PER_INSTANCE;
  
  /* Create the SQI Measurement instance */
  sqi_instance = adi_vsm_sqi_create(&adi_vsm_sqi_mem_handle, &config_handle);
  if (sqi_instance == NULL) {
    return IERR_FAIL;
  } 

#ifdef _TEST_PPROC_
  gsResultCnt = 0;
#endif
  gsStartTime = Adpd400xMwLibGetCurrentTime();
  gsDcOptimumCurrent = Adpd400xUtilGetCurrentFromReg(gAdpd400xOptmVal.ledB_Cur, gAdpd400xOptmVal.ledB_Trim);
  gsAcMinimumCurrent = Adpd400xUtilGetCurrentFromReg(gsLow_Pf[2], gsLow_Pf[3]);
  return IERR_SUCCESS;
}

void Adpd400xACOptReset(uint8_t hardReset) {
  (void)hardReset;
  //Preprocess_Metrics_Reset();
  adi_vsm_sqi_frontend_reset(sqi_instance);
  return;
}

/**
  * @brief  Deinitialization rountine for Setting Optimization.
  * @param  None.
  * @retval SUCCESS = done
  */
void Adpd400xACOptDeInit()  {

}


/**
  * @brief  Do Seting adjustment.
  * @param  Input adpd Data
  * @param  Output status: 0=Setting not change, 1=setting changed
  * @retval In progress, Fail or Success
  */
INT_ERROR_CODE_t Adpd400xACOptDoOptimization(uint32_t* rData, uint8_t* status ) {
  INT_ERROR_CODE_t errCode;
  //SignalMetrics_t sMetrics;
  //SignalQt_t result;
  float sqi_input, sqi_result;
  uint32_t adpdData, tempTime, curTime, adpdDataOri;
  //SM_RetCode_t AGCrtc;
  adi_vsm_sqi_return_code_t SQIAGCrtc;
  g_reg_base = log2(gAdpd400x_lcfg->targetSlots) * SLOT_REG_OFFSET;
#ifdef _TEST_PPROC_
  if (gsResultCnt == TestDataLen) {
    for (gsResultCnt=0; gsResultCnt<TestDataLen; gsResultCnt++)  {
      debug(MODULE, "%d, %d, %d, %d, %d", \
            gsResultCnt,
            TemResult[gsResultCnt].Mt1,
            TemResult[gsResultCnt].Mt2,
            (TemResult[gsResultCnt].Mt2*100+128)/256,
            TemResult[gsResultCnt].Mt3
      );
    }
    gsResultCnt = TestDataLen+10;
  } else {
    adpdData = input_adpd[gsResultCnt];
    //adpdData *= gAdpd400x_lcfg->ppgscale;
  }
  gsResultCnt++;
  sqi_input = (float)adpdData;
  //AGCrtc = Adpd400xGetSignalMetrics(&adpdData, &sMetrics, &result);
  SQIAGCrtc = adi_vsm_sqi_process(sqi_instance, sqi_input, &adi_vsm_sqi_output);

  if (gsResultCnt <= TestDataLen)
    memcpy(&TemResult[gsResultCnt-1], &sMetrics, sizeof(sMetrics));

  return IERR_IN_PROGRESS;   // don't do next
#else
    adpdData = rData[0];/* + rData[1] */
    adpdDataOri = adpdData;
    sqi_input = (float)adpdData;
    //adpdData *= gAdpd400x_lcfg->ppgscale;
    //AGCrtc = GetSignalMetrics(&adpdData, &sMetrics, &result);
    //gSqiSampleCnt++;
    SQIAGCrtc = adi_vsm_sqi_process(sqi_instance, sqi_input, &adi_vsm_sqi_output);
#endif

  //if (AGCrtc == -1) {
    //Preprocess_Metrics_Reset();   // do get metrics again
  if (SQIAGCrtc != ADI_VSM_SQI_SUCCESS && SQIAGCrtc != ADI_VSM_SQI_BUFFERING)
  {
    adi_vsm_sqi_frontend_reset(sqi_instance);
    return IERR_FAIL;
  } else if (SQIAGCrtc != ADI_VSM_SQI_SUCCESS)  {
    return IERR_IN_PROGRESS;
  }

  //gAdpd400xOptmVal.Mt2 = sMetrics.Mt2;
  //gAdpd400xOptmVal.Mt3 = sMetrics.Mt3;
  //gAdpd400xOptmVal.Mt1 = sMetrics.Mt1 / gAdpd400x_lcfg->ppgscale;
  //gAdpd400xOptmVal.sigQuality = (uint8_t)result;
  //adi_printf("\nMetrics %x, %x, %x", gAdpd400xOptmVal.Mt1, gAdpd400xOptmVal.Mt2, gAdpd400xOptmVal.Mt3);

  if (SQIAGCrtc == ADI_VSM_SQI_SUCCESS) {  // Cal Done
    //gSqiSampleCnt = 0;
    curTime = Adpd400xMwLibGetCurrentTime();
    tempTime = AdpdMwLibDeltaTime(gsStartTime, curTime);
    gsStartTime = curTime;

    uint16_t ledPulse, ledCurrent, ledTrim, sampleRate, decimation;   
    sqi_result = adi_vsm_sqi_output.sqi;
    gAdpd400xOptmVal.sigQuality = (uint16_t)(sqi_result * SQI_RESULT_CONVERSION);
    *status = SetProfileSetting(sqi_result, adpdDataOri);
    AdpdDrvRegRead((ADPD400x_REG_COUNTS_A  + g_reg_base), &ledPulse);
    AdpdDrvRegRead((ADPD400x_REG_LED_POW12_A  + g_reg_base), &ledCurrent);
    AdpdDrvRegRead((ADPD400x_REG_LED_POW34_A  + g_reg_base), &ledTrim);
    PpgLibGetSampleRate(&sampleRate, &decimation,gAdpd400x_lcfg->targetSlots);


    gAdpd400xOptmVal.ledB_Cur2 = ledCurrent;
    gAdpd400xOptmVal.ledB_Trim2 = ledTrim;
    gAdpd400xOptmVal.ledB_CurVal2 = Adpd400xUtilGetCurrentFromReg(ledCurrent, ledTrim);
    gAdpd400xOptmVal.ledB_Pulse2 = ledPulse;
    gAdpd400xOptmVal.sampleRate2 = sampleRate;   // sampleRate is not a reg value
    gAdpd400xOptmVal.decimation2 = decimation;   // sampleRate is not a reg value

    errCode = IERR_SUCCESS;
  }
  //Preprocess_Metrics_Reset();   // do get metrics again
  adi_vsm_sqi_frontend_reset(sqi_instance);
  return errCode;
}

/**
  * @brief  Set to use high power profile
  * @param  None.
  * @retval: 0=Setting not change, 1=setting changed
  */
static uint8_t SetProfileSetting(float result, uint32_t adpdData)  {
  uint8_t retCode = 0;
#if 0
  switch (result) {
  case SignalQt_Highest:
  case SignalQt_Excellent:
  case SignalQt_Great:
    // load a fixed profile
    retCode = ACOptSetLowPowerProfile(adpdData);
    break;
  case SignalQt_Bad:
    retCode = ACOptSetHighPowerProfile(adpdData);
    break;
  case SignalQt_Good:
  default:
    break;
  }
#endif
  if(result > (gAdpd400x_lcfg->sqiLowPowerThreshold/SQI_RESULT_CONVERSION)){
    retCode = ACOptSetLowPowerProfile(adpdData);
  }
  else if(result <= (gAdpd400x_lcfg->sqiHighPowerThreshold/SQI_RESULT_CONVERSION)) {
    retCode = ACOptSetHighPowerProfile(adpdData);
  }
  else{
    //do nothing;
  }
  gAdpd400xOptmVal.SelectedLoop = 2;

  return retCode;
}

/**
  * @brief  Set to use high power profile
  * @param  None.
  * @retval 0 = No change. 1 = change
  */
static uint8_t ACOptSetHighPowerProfile(uint32_t adpdData)  {
  uint16_t pulse_num, ori_pulse_num, sampleRate, decimateVal;
  uint16_t led_cur, led_coast, led_trim, led_coast_pre, led_trim_pre;
  uint16_t temp16 = 0, diffPercent;
  uint32_t maxVal;
  uint8_t retVal,ori_pulse_value; 
  uint32_t dc_level;

  // Increase LED current upto the gOptmValDC.ledB_Cur value
  // Increase Pulses
  // Increase Sampling rate if pulse_num reach max
  g_reg_base = log2(gAdpd400x_lcfg->targetSlots) * SLOT_REG_OFFSET;
  AdpdDrvRegRead((ADPD400x_REG_LED_POW12_A  + g_reg_base), &led_coast);
  AdpdDrvRegRead((ADPD400x_REG_LED_POW34_A  + g_reg_base), &led_trim);
  AdpdDrvRegRead((ADPD400x_REG_COUNTS_A  + g_reg_base), &pulse_num);
  //led_cur = Adpd400xUtilGetCurrentValue(led_coast, led_trim);
  led_cur = Adpd400xUtilGetCurrentFromSlot(gAdpd400x_lcfg->targetSlots);

  temp16 = 0;
 // if (gAdpd400x_lcfg->partNum == 188 || gAdpd400x_lcfg->partNum == 108) {  // exclude 107
    // chNum = UtilGetChannelNum();
     //maxVal = (pulse_num >> 8)*gAdpd400x_lcfg->targetDcPercent*80;  // Saturate at 8k
     maxVal = (pulse_num & 0xFF)*gAdpd400x_lcfg->targetDcPercent*80;  // Saturate at 8k
    if (adpdData < maxVal)  {
      diffPercent = (maxVal - adpdData) * 100 / adpdData;
      diffPercent *= ADD_CURRENT_STEP;          // 40% of the difference
      temp16 = led_cur * diffPercent / 100;     // increase LED current amount
      if (temp16 < MIN_CURRENT_STEP) {          //check if change is too small
        temp16 = MIN_CURRENT_STEP;
      }
      led_cur += temp16;
    }
    // else don't change current

  /*} else {
    // 107 may use 2ch, sum after ADC
    temp16 = 0;
    if (led_cur < gsDcOptimumCurrent*(1-FUDGE)) {
      temp16 = (gsDcOptimumCurrent - led_cur)/4; // incr by 25% of the difference
      if (temp16 < MIN_CURRENT_STEP) { //check if 25% current change is too small
        temp16 = MIN_CURRENT_STEP;
        if((led_cur + temp16) > gsDcOptimumCurrent) {
          temp16 = gsDcOptimumCurrent - led_cur;
        }
      }
      led_cur += temp16;   // increase by the difference
    }
    if (led_cur > gsDcOptimumCurrent*(1+FUDGE)) {
      led_cur = gsDcOptimumCurrent;
      temp16 = 1;
    }
  }*/

  // apply new cuurent
  if (temp16 != 0)  {
    if (led_cur > gAdpd400x_lcfg->maxLedCurrent)
      led_cur = gAdpd400x_lcfg->maxLedCurrent;
    led_coast_pre = led_coast;
    led_trim_pre = led_trim;
    Adpd400xUtilGetCurrentRegValue(led_cur, &led_coast, &led_trim);
    if (led_coast_pre != led_coast || led_trim_pre != led_trim)  {
     PpgSetOperationMode(ADPDDrv_MODE_IDLE);
     AdpdDrvRegWrite((ADPD400x_REG_LED_POW12_A  + g_reg_base), led_coast);
     AdpdDrvRegWrite((ADPD400x_REG_LED_POW34_A  + g_reg_base), led_trim);
     PpgSetOperationMode(ADPDDrv_MODE_SAMPLE);
     return 1;
    }
  }

  // adpdlib 4.1.0
  // Add pulses upto the g_lcfg->maxPulseNum
  PpgLibGetSampleRate(&sampleRate, &decimateVal,gAdpd400x_lcfg->targetSlots);  // backup sampling rate
  ori_pulse_num = pulse_num;
 // temp16 = pulse_num >> 8;  
  temp16 = (pulse_num & 0xFF);
  temp16 = (uint16_t)(temp16 * 1.30) * decimateVal;
  temp16 += ADD_PULSE;   // increase by 30% pulses and + ADD_PULSE
  temp16 /= (uint8_t)decimateVal;
  dc_level = adpdData;
  
  //ori_pulse_value = ori_pulse_num >> 8;
  ori_pulse_value = (ori_pulse_num & 0xFF);
  retVal = ADPDLibPostPulseIncreaseAdjust(&temp16,&ori_pulse_value,&sampleRate,&decimateVal,&dc_level);
   
  if(retVal == IERR_FAIL) {
      temp16 = (pulse_num & 0xFF);
  }
  
 // pulse_num = (pulse_num & 0xFF) | (temp16 << 8);
  pulse_num = (pulse_num & 0xFF00) | (temp16); 
  if (pulse_num == ori_pulse_num){
    return 0;   // no change, both pulse and Fs are max out
  }

  PpgSetOperationMode(ADPDDrv_MODE_IDLE);
  AdpdDrvRegWrite((ADPD400x_REG_COUNTS_A  + g_reg_base), pulse_num);
  PpgLibSetSampleRate(sampleRate, decimateVal,gAdpd400x_lcfg->targetSlots);    // in case Fs get changed
  PpgSetOperationMode(ADPDDrv_MODE_SAMPLE);
  return 1;
}

/**
  * @brief  Set to use low power profile.
  * @param  None.
  * @retval 0 = No change. 1 = change
  */
static uint8_t ACOptSetLowPowerProfile(uint32_t adpdData)  {
  uint16_t pulse_num, new_pulseNum, sampleRate, decimateVal;
  uint16_t led_cur, led_trim, led_coast, ori_led_cur;
  uint16_t temp16 = 0;
  uint8_t retVal,ori_pulseNum;
  uint32_t dc_level;
  // Decrease Sampling rate if Fs>ODR, double Pulse num.
  // Decrease Pulses
  // Decrease LED current upto the gsAcMinimumCurrent
  g_reg_base = log2(gAdpd400x_lcfg->targetSlots) * SLOT_REG_OFFSET;

  AdpdDrvRegRead((ADPD400x_REG_COUNTS_A  + g_reg_base), &pulse_num);
  ori_pulseNum = (pulse_num & 0xFF);
  new_pulseNum = ori_pulseNum;
  PpgLibGetSampleRate(&sampleRate, &decimateVal,gAdpd400x_lcfg->targetSlots);  // backup sampling rate

  // adpdlib 4.1.0
  new_pulseNum = (uint16_t)(new_pulseNum * decimateVal * 0.8);
  if (new_pulseNum > SUB_PULSE)
    new_pulseNum -= SUB_PULSE;
  new_pulseNum /= (uint8_t)decimateVal;
  dc_level = adpdData;
  retVal = ADPDLibPostPulseDecreaseAdjust(&new_pulseNum,&ori_pulseNum,&sampleRate, &decimateVal, &dc_level);
  
  if (new_pulseNum != ori_pulseNum)  {   // Pulse num changed
    //pulse_num = (pulse_num & 0xFF) | (new_pulseNum << 8);
    pulse_num = (pulse_num & 0xFF00) | (new_pulseNum);
    PpgSetOperationMode(ADPDDrv_MODE_IDLE);
    AdpdDrvRegWrite((ADPD400x_REG_COUNTS_A  + g_reg_base), pulse_num);
    PpgLibSetSampleRate(sampleRate, decimateVal,gAdpd400x_lcfg->targetSlots);     // in case Fs get changed
    PpgSetOperationMode(ADPDDrv_MODE_SAMPLE);
    return 1;

  } else {  // pulse num, Fs reach min
    // Decrease LED current by x%.
    AdpdDrvRegRead((ADPD400x_REG_LED_POW12_A  + g_reg_base), &led_coast);
    AdpdDrvRegRead((ADPD400x_REG_LED_POW34_A  + g_reg_base), &led_trim);
    //led_cur = Adpd400xUtilGetCurrentValue(led_coast, led_trim);
    led_cur = Adpd400xUtilGetCurrentFromSlot(gAdpd400x_lcfg->targetSlots);
    ori_led_cur = led_cur;
    temp16 = 0;         // delta decrement
    // if led_cur is close to gsAcMin, do nothing
    if (led_cur > gsAcMinimumCurrent*(1+FUDGE)) {
      temp16 = (led_cur-gsAcMinimumCurrent)/4; // decr by 25% of the difference
      // check if 25% delta is too small
      if (temp16 < MIN_CURRENT_STEP)
        temp16 = MIN_CURRENT_STEP;
      // check if final current will be less than gsAcMinimumCurrent
      if ( (led_cur - temp16) < gsAcMinimumCurrent )
        led_cur = gsAcMinimumCurrent;
      else
        led_cur -= temp16;

      if (led_cur == ori_led_cur)
        return 0;       // no change of LED current

      Adpd400xUtilGetCurrentRegValue(led_cur, &led_coast, &led_trim);
      PpgSetOperationMode(ADPDDrv_MODE_IDLE);
      AdpdDrvRegWrite((ADPD400x_REG_LED_POW12_A  + g_reg_base), led_coast);
      AdpdDrvRegWrite((ADPD400x_REG_LED_POW34_A  + g_reg_base), led_trim);
      PpgSetOperationMode(ADPDDrv_MODE_SAMPLE);
      return 1;
    }
  }
  return 0;
}
#endif
