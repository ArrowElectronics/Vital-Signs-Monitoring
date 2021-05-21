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
#if 0
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "adpd400x_lib.h"
#include "adpd400x_lib_common.h"
#include "signal_metrics.h"

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

#define ADD_PULSE               4
#define SUB_PULSE               3
#define MIN_PULSE               4 //2 //changed from 2 -> 4
#define FUDGE                   0.1
#define MIN_CURRENT_STEP        10
#define ADD_CURRENT_STEP        0.4     // add 40%


/* Public function prototypes -----------------------------------------------*/
INT_ERROR_CODE_t Adpd400xACOptInit(uint16_t);
void Adpd400xACOptDeInit(void);
void Adpd400xACOptReset(uint8_t);

/* Private function prototypes ----------------------------------------------*/
static uint8_t SetProfileSetting(SignalQt_t result, uint32_t);
static uint8_t ACOptSetHighPowerProfile(uint32_t);
static uint8_t ACOptSetLowPowerProfile(void);

/* Private variables --------------------------------------------------------*/
static uint32_t gsStartTime;

// fs, dec, Led, trim, pulse
static uint16_t gsLow_Pf[] = {0xA0, 0x0, 0x0000, 0x1F, MIN_PULSE};
static uint16_t gsDcOptimumCurrent, gsAcMinimumCurrent;

/**
  * @brief  Initialization rountine for Setting Optimization.
  * @param  None.
  * @retval SUCCESS = done
  */
INT_ERROR_CODE_t Adpd400xACOptInit(uint16_t deviceID) {
  if (Adpd400xPreprocess_Metrics_Init(deviceID, gAdpd400x_lcfg->mt2Th, gAdpd400x_lcfg->mt3Th) != 0)
    return IERR_FAIL;

#ifdef _TEST_PPROC_
  gsResultCnt = 0;
#endif
  gsStartTime = AdpdMwLibGetCurrentTime();
  gsDcOptimumCurrent = Adpd400xUtilGetCurrentValue(gAdpd400xOptmVal.ledB_Cur, gAdpd400xOptmVal.ledB_Trim);
  gsAcMinimumCurrent = Adpd400xUtilGetCurrentValue(gsLow_Pf[2], gsLow_Pf[3]);
  return IERR_SUCCESS;
}

void Adpd400xACOptReset(uint8_t hardReset) {
  (void)hardReset;
  Adpd400xPreprocess_Metrics_Reset();
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
  SignalMetrics_t sMetrics;
  SignalQt_t result;
  uint32_t adpdData, tempTime, curTime, adpdDataOri;
  SM_RetCode_t AGCrtc;

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
    adpdData *= gAdpd400x_lcfg->ppgscale;
  }
  gsResultCnt++;
  AGCrtc = Adpd400xGetSignalMetrics(&adpdData, &sMetrics, &result);

  if (gsResultCnt <= TestDataLen)
    memcpy(&TemResult[gsResultCnt-1], &sMetrics, sizeof(sMetrics));

  return IERR_IN_PROGRESS;   // don't do next
#else
    adpdData = rData[0] + rData[1] + rData[2] + rData[3];
    adpdDataOri = adpdData;
    adpdData *= gAdpd400x_lcfg->ppgscale;
    AGCrtc = Adpd400xGetSignalMetrics(&adpdData, &sMetrics, &result);
#endif

  if (AGCrtc == -1) {
    Adpd400xPreprocess_Metrics_Reset();   // do get metrics again
    return IERR_FAIL;
  } else if (AGCrtc != SMrtc_Pass)  {
    return IERR_IN_PROGRESS;
  }

  gAdpd400xOptmVal.Mt2 = sMetrics.Mt2;
  gAdpd400xOptmVal.Mt3 = sMetrics.Mt3;
  gAdpd400xOptmVal.Mt1 = sMetrics.Mt1 / gAdpd400x_lcfg->ppgscale;
  gAdpd400xOptmVal.sigQuality = (uint8_t)result;
  adi_printf("\nMetrics %x, %x, %x", gAdpd400xOptmVal.Mt1, gAdpd400xOptmVal.Mt2, gAdpd400xOptmVal.Mt3);

  if (AGCrtc == SMrtc_Pass) {  // Cal Done
    curTime = AdpdMwLibGetCurrentTime();
    tempTime = AdpdMwLibDeltaTime(gsStartTime, curTime);
    gsStartTime = curTime;

    uint16_t ledPulse, ledCurrent, ledTrim, sampleRate, decimation;
    AdpdDrvRegRead(REG_PULSE_PERIOD_B, &ledPulse);
    AdpdDrvRegRead(REG_LED1_DRV, &ledCurrent);
    AdpdDrvRegRead(REG_LED_TRIM, &ledTrim);
    // kfkf 1120 AdpdDrvRegRead(REG_SAMPLING_FREQ, &sampleRate);
    PpgLibGetSampleRate(&sampleRate, &decimation);

    adi_printf("\nbefore %4x, %4x, %4x, %4x, result=%d", ledCurrent, ledTrim, ledPulse, sampleRate, result);
    *status = SetProfileSetting(result, adpdDataOri);

    AdpdDrvRegRead(REG_PULSE_PERIOD_B, &ledPulse);
    AdpdDrvRegRead(REG_LED1_DRV, &ledCurrent);
    AdpdDrvRegRead(REG_LED_TRIM, &ledTrim);
    // kfkf 1120 AdpdDrvRegRead(REG_SAMPLING_FREQ, &sampleRate);
    PpgLibGetSampleRate(&sampleRate, &decimation);
    adi_printf("\nafter  %4x, %4x, %4x, %4x, time=%u", ledCurrent, ledTrim, ledPulse, sampleRate, tempTime);


    gAdpd400xOptmVal.ledB_Cur2 = ledCurrent;
    gAdpd400xOptmVal.ledB_Trim2 = ledTrim;
    gAdpd400xOptmVal.ledB_Pulse2 = ledPulse;
    gAdpd400xOptmVal.sampleRate2 = sampleRate;   // sampleRate is not a reg value
    gAdpd400xOptmVal.decimation2 = decimation;   // sampleRate is not a reg value
    gAdpd400xOptmVal.ledB_CurVal2 = Adpd400xUtilGetCurrentFromReg(ledCurrent, ledTrim);
    // gOptmVal.ledB_Pulse2 = (ledPulse & 0xFF00) | (result & 0xff) | 0x8000; // for Gen2

    errCode = IERR_SUCCESS;
  }
  Adpd400xPreprocess_Metrics_Reset();   // do get metrics again
  return errCode;
}

/**
  * @brief  Set to use high power profile
  * @param  None.
  * @retval: 0=Setting not change, 1=setting changed
  */
static uint8_t SetProfileSetting(SignalQt_t result, uint32_t adpdData)  {
  uint8_t retCode = 0;
  switch (result) {
  case SignalQt_Highest:
  case SignalQt_Excellent:
  case SignalQt_Great:
    // load a fixed profile
    retCode = ACOptSetLowPowerProfile();
    break;
  case SignalQt_Bad:
    retCode = ACOptSetHighPowerProfile(adpdData);
    break;
  case SignalQt_Good:
  default:
    break;
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
  uint16_t pulse_num, ori_pulse_num, sampleRate, decimateVal, ori_decimateVal;
  uint16_t led_cur, led_coast, led_trim, led_coast_pre, led_trim_pre;
  uint16_t temp16 = 0, diffPercent;
  uint32_t maxVal;

  // Increase LED current upto the gOptmValDC.ledB_Cur value
  // Increase Pulses
  // Increase Sampling rate if pulse_num reach max
  AdpdDrvRegRead(REG_LED1_DRV, &led_coast);
  AdpdDrvRegRead(REG_LED_TRIM, &led_trim);
  AdpdDrvRegRead(REG_PULSE_PERIOD_B, &pulse_num);
  led_cur = Adpd400xUtilGetCurrentValue(led_coast, led_trim);

  temp16 = 0;
  if (gAdpd400x_lcfg->partNum == 188 || gAdpd400x_lcfg->partNum == 108) {  // exclude 107
    // chNum = UtilGetChannelNum();
    maxVal = (pulse_num >> 8)*gAdpd400x_lcfg->targetDcPercent*80;  // Saturate at 8k
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

  } else {
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
  }

  // apply new cuurent
  if (temp16 != 0)  {
    if (led_cur > gAdpd400x_lcfg->maxLedCurrent)
      led_cur = gAdpd400x_lcfg->maxLedCurrent;
    led_coast_pre = led_coast;
    led_trim_pre = led_trim;
    Adpd400xUtilGetCurrentRegValue(led_cur, &led_coast, &led_trim);
    if (led_coast_pre != led_coast || led_trim_pre != led_trim)  {
      AdpdMwLibSetMode(ADPDDrv_MODE_IDLE, ADPDDrv_SLOT_OFF, ADPDDrv_SLOT_OFF);
      AdpdDrvRegWrite(REG_LED1_DRV, led_coast);
      AdpdDrvRegWrite(REG_LED_TRIM, led_trim);
      PpgLibSetMode(ADPDDrv_MODE_SAMPLE,
                  (ADPDDrv_Operation_Slot_t)gSlotAmode,
                  (ADPDDrv_Operation_Slot_t)(gAdpd400x_lcfg->deviceMode));
      return 1;
    }
  }

#if 1   // adpdlib 4.1.0
  // Add pulses upto the g_lcfg->maxPulseNum
  PpgLibGetSampleRate(&sampleRate, &decimateVal);  // backup sampling rate
  ori_pulse_num = pulse_num;
  ori_decimateVal = decimateVal;
  temp16 = pulse_num >> 8;
  temp16 = (uint16_t)(temp16 * 1.30) * decimateVal;
  temp16 += ADD_PULSE;   // increase by 30% pulses and + ADD_PULSE
  temp16 /= (uint8_t)decimateVal;

  if (temp16 > gAdpd400x_lcfg->maxPulseNum)  {
    if (sampleRate >= gAdpd400x_lcfg->maxSamplingRate) {   // sampleRate >= max
      temp16 = gAdpd400x_lcfg->maxPulseNum;
    } else {
      // double sampling rate, decimation
      sampleRate *= 2;          // double Fs
      decimateVal *= 2;         // double decimation
      temp16 /= 2;              // half pulses
    }
  }

  // if odd value, subtract by 1.
  if (temp16 & 0x1 == 1)
    temp16 -= 1;
  // if value < min, set min.
  if (temp16 < MIN_PULSE)
    temp16 = MIN_PULSE;

  pulse_num = (pulse_num & 0xFF) | (temp16 << 8);
  if ((pulse_num == ori_pulse_num) && (decimateVal == ori_decimateVal))  {
    return 0;   // no change, both pulse and Fs are max out
  }

#else
  // Add pulses upto the g_lcfg->maxPulseNum
  PpgLibGetSampleRate(&sampleRate, &decimateVal);  // backup sampling rate
  AdpdDrvRegRead(REG_PULSE_PERIOD_B, &pulse_num);
  ori_pulse_num = pulse_num;
  temp16 = pulse_num >> 8;
  temp16 += ADD_PULSE;      // add 4 pulses

  if (gAdpd400x_lcfg->partNum == 107)  {
    if (temp16 <= gAdpd400x_lcfg->maxPulseNum)  {
      pulse_num = (pulse_num & 0xFF) | (temp16 << 8);
    } else {
      if (sampleRate >= gAdpd400x_lcfg->maxSamplingRate) {   // sampleRate >= max
        pulse_num = (pulse_num & 0xFF) | (gAdpd400x_lcfg->maxPulseNum << 8);
        if (ori_pulse_num == pulse_num)
          return 0;   // no change, both pulse and Fs are max out
      } else {  // sampleRate < max
        adi_printf(MODULE, "double the fs!!");
        sampleRate *= 2;        // double Fs
        decimateVal *= 2;       // double decimation
        temp16 /= 2;            // half pulses
        if (temp16 < MIN_PULSE)
          temp16 = MIN_PULSE;
        pulse_num = (pulse_num & 0xFF) | (temp16 << 8);
      }
    }
  } else {   // adpdlib 4.0.1
    temp16 *= decimateVal;
    if (temp16 <= gAdpd400x_lcfg->maxPulseNum)  {
      temp16 /= decimateVal;
      pulse_num = (pulse_num & 0xFF) | (temp16 << 8);
    } else {
      if (sampleRate >= gAdpd400x_lcfg->maxSamplingRate) {   // sampleRate >= max
        temp16 = gAdpd400x_lcfg->maxPulseNum / decimateVal;
        pulse_num = (pulse_num & 0xFF) | (temp16 << 8);
        if (ori_pulse_num == pulse_num)
          return 0;   // no change, both pulse and Fs are max out
      } else {  // sampleRate < max
        adi_printf(MODULE, "double the fs!!");
        sampleRate *= 2;        // double Fs
        decimateVal *= 2;       // double decimation
        temp16 /= decimateVal;
        if (temp16 < MIN_PULSE)
          temp16 = MIN_PULSE;
        pulse_num = (pulse_num & 0xFF) | (temp16 << 8);
      }
    }
  }
#endif

  AdpdMwLibSetMode(ADPDDrv_MODE_IDLE, ADPDDrv_SLOT_OFF, ADPDDrv_SLOT_OFF);
  AdpdDrvRegWrite(REG_PULSE_PERIOD_B, pulse_num);
  PpgLibSetSampleRate(sampleRate, decimateVal);    // in case Fs get changed
  PpgLibSetMode(ADPDDrv_MODE_SAMPLE,
               (ADPDDrv_Operation_Slot_t)gSlotAmode,
               (ADPDDrv_Operation_Slot_t)(gAdpd400x_lcfg->deviceMode));
  return 1;
}

/**
  * @brief  Set to use low power profile.
  * @param  None.
  * @retval 0 = No change. 1 = change
  */
static uint8_t ACOptSetLowPowerProfile()  {
  uint16_t pulse_num, ori_pulseNum, new_pulseNum, sampleRate, decimateVal;
  uint16_t led_cur, led_trim, led_coast, ori_led_cur;
  uint16_t temp16 = 0;

  // Decrease Sampling rate if Fs>ODR, double Pulse num.
  // Decrease Pulses
  // Decrease LED current upto the gsAcMinimumCurrent

  AdpdDrvRegRead(REG_PULSE_PERIOD_B, &pulse_num);
  ori_pulseNum = pulse_num >> 8;
  new_pulseNum = ori_pulseNum;
  PpgLibGetSampleRate(&sampleRate, &decimateVal);  // backup sampling rate

#if 1   // adpdlib 4.1.0
  new_pulseNum = (uint16_t)(new_pulseNum * decimateVal * 0.8);
  if (new_pulseNum > SUB_PULSE)
    new_pulseNum -= SUB_PULSE;
  new_pulseNum /= (uint8_t)decimateVal;

  for (temp16 = 0; temp16 < 2; temp16++)  {     // check 2 times
    if (sampleRate > gAdpd400x_lcfg->hrmInputRate) {    // Fs>ODR
      if (new_pulseNum >= 64)  {
        break;          // don't want more than 64 pulse to avoid adc sat
      }
      if (new_pulseNum *2 <= gAdpd400x_lcfg->maxPulseNum)  {
        sampleRate >>= 1;         // half sampling rate
        decimateVal >>= 1;        // half decimation
        new_pulseNum *= 2;        // double pulses
      }
    } else {
      break;
    }
  }

  // make it even number
  if (new_pulseNum & 0x1 == 1)
    new_pulseNum -= 1;
  if (new_pulseNum < MIN_PULSE)  {
    new_pulseNum = MIN_PULSE;
  }

#else
  if (sampleRate > gAdpd400x_lcfg->hrmInputRate) {    // Fs>ODR
    sampleRate >>= 1;        // half sampling rate
    decimateVal >>= 1;       // half decimation
    new_pulseNum *= 2;      // double pulses
    adi_printf(MODULE, "half the fs!!");
    ori_pulseNum = 0;       // Needed. indication of changed.
  }
  if (new_pulseNum <= SUB_PULSE + MIN_PULSE)
    new_pulseNum = MIN_PULSE;
  else
    new_pulseNum -= SUB_PULSE;
  if (new_pulseNum > gAdpd400x_lcfg->maxPulseNum)   // for 107. Won't apply to 108
    new_pulseNum = gAdpd400x_lcfg->maxPulseNum;

#endif

  if (new_pulseNum != ori_pulseNum)  {   // Pulse num changed
    pulse_num = (pulse_num & 0xFF) | (new_pulseNum << 8);
    AdpdMwLibSetMode(ADPDDrv_MODE_IDLE, ADPDDrv_SLOT_OFF, ADPDDrv_SLOT_OFF);
    AdpdDrvRegWrite(REG_PULSE_PERIOD_B, pulse_num);
    PpgLibSetSampleRate(sampleRate, decimateVal);     // in case Fs get changed
    PpgLibSetMode(ADPDDrv_MODE_SAMPLE,
                 (ADPDDrv_Operation_Slot_t)gSlotAmode,
                 (ADPDDrv_Operation_Slot_t)(gAdpd400x_lcfg->deviceMode));
    return 1;

  } else {  // pulse num, Fs reach min
    // Decrease LED current by x%.
    AdpdDrvRegRead(REG_LED1_DRV, &led_coast);
    AdpdDrvRegRead(REG_LED_TRIM, &led_trim);
    led_cur = Adpd400xUtilGetCurrentValue(led_coast, led_trim);
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
      AdpdMwLibSetMode(ADPDDrv_MODE_IDLE, ADPDDrv_SLOT_OFF, ADPDDrv_SLOT_OFF);
      AdpdDrvRegWrite(REG_LED1_DRV, led_coast);
      AdpdDrvRegWrite(REG_LED_TRIM, led_trim);
      PpgLibSetMode(ADPDDrv_MODE_SAMPLE,
                   (ADPDDrv_Operation_Slot_t)gSlotAmode,
                   (ADPDDrv_Operation_Slot_t)(gAdpd400x_lcfg->deviceMode));
      return 1;
    }
  }
  return 0;
}
#endif
