/********************************************************************************

*/
/*!
*  copyright Analog Devices
* ****************************************************************************
*
* License Agreement
*
* Copyright (c) 2021 Analog Devices Inc.
* All rights reserved.
*
* This source code is intended for the recipient only under the guidelines of
* the non-disclosure agreement with Analog Devices Inc.
*

* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER  LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING  FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER  DEALINGS IN THE SOFTWARE.

 ********************************************************************************
 * File Name     : us_tick.c
 * Date          : 15-06-2021
 * Description   : .Offer a micro second precision counter.
 * Version       : 1.0
*************************************************************************************************************/

#include "us_tick.h"

#include "nrf_log.h"
#include "nrf.h"
#include "nrf_drv_timer.h"
#include "Adxl362.h"
#if NRF_LOG_ENABLED
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#endif
#include <rtc.h>
#include "ad5940.h"
#include "ecg_task.h"
#ifdef EXTERNAL_TRIGGER_EDA
#include "eda_application_task.h"
#endif

#if defined(PROFILE_TIME_ENABLED) || defined(BLE_NUS_PROFILE_TIME_ENABLED)


const nrfx_timer_t TIMER_INC_MICRO_SEC = NRFX_TIMER_INSTANCE(4);

static void inc_micro_sec(nrf_timer_event_t event_type, void* p_context)
{
}
/*****************************************************************************
 * Function      : get_micro_sec
 * Description   : get micro second time
 * Input         : void
 * Output        : None
 * Return        :
 * Others        :
 * Record
 * 1.Date        : 20200720
 *   Modification: Created function

*****************************************************************************/
uint32_t get_micro_sec(void)
{
    return nrfx_timer_capture(&TIMER_INC_MICRO_SEC,NRF_TIMER_CC_CHANNEL0);
}

/*****************************************************************************
 * Function      : us_timer_init
 * Description   : micro second timer initialization
 * Input         : void
 * Output        : None
 * Return        :
 * Others        :
 * Record
 * 1.Date        : 20200720
 *   Modification: Created function

*****************************************************************************/
uint32_t us_timer_init(void)
{
    uint32_t err_code = NRF_SUCCESS;

    nrfx_timer_config_t timer_cfg  = NRFX_TIMER_DEFAULT_CONFIG;

       timer_cfg.frequency          = (nrf_timer_frequency_t)NRF_TIMER_FREQ_1MHz;
       timer_cfg.mode               =  (nrf_timer_mode_t)NRF_TIMER_MODE_TIMER;
       timer_cfg.bit_width          = (nrf_timer_bit_width_t)NRF_TIMER_BIT_WIDTH_32;
       timer_cfg.interrupt_priority = APP_IRQ_PRIORITY_LOWEST;

     err_code = nrfx_timer_init(&TIMER_INC_MICRO_SEC,&timer_cfg,inc_micro_sec);
     if(NRF_SUCCESS != err_code)
    {
        NRF_LOG_INFO("nrf_drv_timer_init failue,err_code = %d",err_code);
        return err_code;
    }
    nrfx_timer_enable(&TIMER_INC_MICRO_SEC);
    return 0;
}

void us_timer_deinit(void)
{
   nrfx_timer_uninit(&TIMER_INC_MICRO_SEC);
}

void us_timer_reset(void)
{
     // de init timer structure
      nrfx_timer_clear(&TIMER_INC_MICRO_SEC);
}
#endif

#define ADXL_EXT_TRIGGER
#ifdef ADXL_EXT_TRIGGER

const nrf_drv_timer_t TIMER_ADXL = NRF_DRV_TIMER_INSTANCE(1);

void ppg_adjust_adpd_ext_trigger(uint16_t nOdr);
extern void invert_adxl_trigger_signal();
extern void enable_adxl_trigger_pin();
extern void disable_adxl_trigger_pin();

extern void invert_adpd_trigger_signal();
extern void enable_adpd_trigger_pin();
extern void disable_adpd_trigger_pin();
extern void reset_trigger_pulses();

volatile uint32_t gTriggerTimerCnt =0, gAdpdTimerCnt=0, gAdxlTimerCnt=0;
volatile uint8_t gAdxlExtTimerRunning = 0;
volatile uint8_t gAdpdRunning=0, gAdxlRunning=0,gTimerRunning=0;
volatile uint8_t gResetTriggerPulseStates = 0;
#ifdef EXTERNAL_TRIGGER_EDA
extern uint8_t ecg_start_req;
extern uint8_t eda_start_req;
#endif

/*!
                              ADPD ODR: Index to Frequency(Hz) to Timer Ticks(16Mhz clk ticks) Mapping Table
  --------------------------------------------------------------------------------------------------------------
  | Index     |   0      |   1    |   2   |    3     |   4     |   5     |   6     |   7    |   8     |   9    |
  --------------------------------------------------------------------------------------------------------------
  | Freq(Hz)  |   12.5   |   25   |   50  |    100   |   200   |   250   |   300   |  400   |   500   |  1000  |
  --------------------------------------------------------------------------------------------------------------
  --------------------------------------------------------------------------------------------------------------
  | Period(ms)|    80    |   40   |   20  |    10    |    5    |    4    |   3.33  |   2.5  |    2    |    1   |
  --------------------------------------------------------------------------------------------------------------
  | Pulse     |    40    |   20   |   10  |    5     |   2.5   |    2    |  1.66   |  1.25  |    1    |   0.5  |
  | width(ms) |          |        |       |          |         |         |         |        |         |        |
  --------------------------------------------------------------------------------------------------------------

            Frequency to Ticks Conversion assuming 16Mhz clk and Prescaler value of 0.
                  # Ticks = Pulse Width (ms)  * 16000(ticks per ms)
*/

#define TRIGGER_FREQ_STEPS  10u
uint16_t gaAdpdTriggerFreq[TRIGGER_FREQ_STEPS]={12,25,50,100,200,250,300,400,500,1000};
uint8_t gAdpdFreqIndex = 2, gAdpdDecFactor = 1;
uint8_t gAdxlFreqIndex = 2, gAdxlDecFactor = 1;

uint32_t gaExtTriggerTicks[TRIGGER_FREQ_STEPS] = {640000,320000,160000,80000,40000,32000,26667,20000,16000,8000};
uint8_t gExtTriggerFreqIndex = 2;

#ifdef ENABLE_ECG_APP
uint8_t gAd5940FreqIndex = 2, gAd5940DecFactor = 1;
uint8_t gExtad5940TriggerFreqIndex = 2;
#endif

uint8_t map_adpd_freq_to_index(uint16_t nOdr)
{
  int i;
  for(i=0; i<TRIGGER_FREQ_STEPS; i++)
  {
    if(gaAdpdTriggerFreq[i] >= nOdr)
    {
      return i;
    }
  }
  if(i==TRIGGER_FREQ_STEPS)
    return 2; /*! Wrong ODR chosen, set ODR to 50Hz */
}

#define MAX_ADXL_ODR_INDEX    5
uint8_t map_adxl_freq_to_index(uint8_t nOdrIndex)
{
  /*! Offset of +2 added because of 250,300Hz in between 200 and 400Hz in Ticks Conversion Table*/
  return(( nOdrIndex>=MAX_ADXL_ODR_INDEX)?MAX_ADXL_ODR_INDEX+2:nOdrIndex);
}

#ifdef DEBUG_TIMER_TICKS
uint32_t gaTrigTimings[100];
uint16_t gArrIndex = 0;

uint32_t gaTrigDiff[100];
uint16_t gDiffIndex = 0;
uint32_t gTimestampCp  = 0;
#endif

/**
 * @brief Handler for timer events.
 */
void adxl_timer_event_handler(nrf_timer_event_t event_type, void* p_context)
{
    switch (event_type)
    {
        case NRF_TIMER_EVENT_COMPARE0:
            if( gAdpdRunning ==1 && gAdxlRunning ==0)
            {
              invert_adpd_trigger_signal();
              gAdpdTimerCnt++;
            }
            else if( gAdpdRunning ==0 && gAdxlRunning ==1)
            {
              invert_adxl_trigger_signal();
              gAdxlTimerCnt++;
            }
            else if( gAdpdRunning ==1 && gAdxlRunning ==1)
            {
              gTriggerTimerCnt++;
              if(gResetTriggerPulseStates)
              {
                reset_trigger_pulses();
                gResetTriggerPulseStates = 0;
              }
              else
              {
                if((gTriggerTimerCnt%gAdxlDecFactor) == 0)
                {
                  invert_adxl_trigger_signal();
#ifdef DEBUG_TIMER_TICKS
                  gTimestampCp = get_sensor_time_stamp();
#endif
                }
                if((gTriggerTimerCnt%gAdpdDecFactor) == 0)
                {
                  invert_adpd_trigger_signal();
#ifdef DEBUG_TIMER_TICKS
                  gaTrigDiff[gDiffIndex] = get_sensor_time_stamp() - gTimestampCp;
                  if(++gDiffIndex >= 100)
                    gDiffIndex = 0;
#endif
                }
              }
            }
#ifdef DEBUG_TIMER_TICKS
              gaTrigTimings[gArrIndex] = get_sensor_time_stamp();
              if(++gArrIndex >= 100)
                gArrIndex = 0;
#endif
            break;

        default:
            //Do nothing.
            break;
    }
}

/*
  Frequency to  ticks conversion table; assuming 16MHz clock source and Prescaler value of 0.
  TODO: Check with Prescaler value of 16; accordingly change the table

                              Frequecny(Hz) to Ticks Conversion Table
  -------------------------------------------------------------------------------------------
  | Index     |   0      |   1    |   2   |    3     |   4     |   5     |   6     |   7    |
  -------------------------------------------------------------------------------------------
  | Freq(Hz)  |   12.5   |   25   |   50  |    100   |   200   |   400   |   400   |   400  |
  -------------------------------------------------------------------------------------------
  -------------------------------------------------------------------------------------------
  | Period(ms)|    80    |   40   |   20  |    10    |    5    |   2.5   |   2.5   |   2.5  |
  -------------------------------------------------------------------------------------------
  | Pulse     |    40    |   20   |   10  |    5     |   2.5   |   1.25  |  1.25   |  1.25  |
  | width(ms) |          |        |       |          |         |         |         |        |
  -------------------------------------------------------------------------------------------

            Frequency to Ticks Conversion assuming 16Mhz clk and Prescaler value of 0.
                  # Ticks = Pulse Width (ms)  * 16000(ticks per ms)

*/

uint32_t gaAdxlTriggerTicks[8] = {640000,320000,160000,80000,40000,20000,20000,20000};

/*!
                              ECG ODR: Index to Frequency(Hz) to Timer Ticks(16Mhz clk ticks) Mapping Table
  ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  | Index     |   0   | 1   |   2     |   3    |   4   |    5     |   6     |   7     |   8     |   9    |   10     |   11   |  12  |  13   |   14  |   15   | 16     |
  ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  | Freq(Hz)  |   4   | 8   |   12    |  16   |   30   |    50    |  100    | 200     |   250   |   300   |  400    |   500   |  600  |  700 |  800  |  900  |  1000  |
  ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  -----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  | Period(ms)|    250   |  125     | 83.33        |   62.5       |   33.33    |    20    | 10     |   5       |    4    |   3.33  |   2.5      |    2    |    1.66 |  1.42  |  1.25   |  1.11   |  1  |
  -----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  | Pulse     |    125   |   62.5   |   41.665     |    31.25     |   16.665   |    10    |  5     |  2.5      |   2     |  1.665  |    1.25    |   1     |  0.83   |  0.71  |  0.625   |  0.555 |  1  |
  | width(ms) |      |        |            |          |               |             |         |        |         |         |       |         |         |       |
  -----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

            Frequency to Ticks Conversion assuming 16Mhz clk and Prescaler value of 0.
                  # Ticks = Pulse Width (ms)  * 16000(ticks per ms)
*/

#ifdef EXTERNAL_TRIGGER_EDA
#define TRIGGER_AD5940_FREQ_STEPS 17
uint16_t gaAd5940TriggerFreq[TRIGGER_AD5940_FREQ_STEPS]={4,8,12,16,30,50,100,200,250,300,400,500,600,700,800,900,1000};
uint32_t gaExtAd5940TriggerTicks[TRIGGER_AD5940_FREQ_STEPS] = {2000000,100000,666640,500000,\
                                                            266640,320000,160000,80000,\
                                                            40000,32000,26640,20000,\
                                                            13280,11360,10000,8800,8000};
#else
#define TRIGGER_AD5940_FREQ_STEPS 14
uint16_t gaAd5940TriggerFreq[TRIGGER_AD5940_FREQ_STEPS]={12,25,50,100,200,250,300,400,500,600,700,800,900,1000};
uint32_t gaExtAd5940TriggerTicks[TRIGGER_AD5940_FREQ_STEPS] = {666640,320000,160000,80000,40000,32000,26667,20000,\
								16000,13280,11360,10000,8800,8000};
#endif															


/*
  Pointer used for choosing values from the Frequency to Ticks conversion table
  By default set to 50hz;
*/
uint8_t gAdxlFreqSelected = 2;
uint8_t gsTrigTimerStartCnt =0, gsTrigTimerStopCnt =0;

void  enable_trigger_timer(uint8_t nOdrIndex)
{
    uint32_t time_ticks;
    uint32_t err_code = NRF_SUCCESS;

    //nrf_drv_timer_config_t timer_cfg = NRF_DRV_TIMER_DEFAULT_CONFIG;
    nrf_drv_timer_config_t timer_cfg = {                                                                                    \
    .frequency          = (nrf_timer_frequency_t)NRFX_TIMER_DEFAULT_CONFIG_FREQUENCY,\
    .mode               = (nrf_timer_mode_t)NRFX_TIMER_DEFAULT_CONFIG_MODE,          \
    .bit_width          = (nrf_timer_bit_width_t)NRFX_TIMER_DEFAULT_CONFIG_BIT_WIDTH,\
    .interrupt_priority = APP_IRQ_PRIORITY_HIGHEST,                    \
    .p_context          = NULL                                                       \
    };
    err_code = nrf_drv_timer_init(&TIMER_ADXL, &timer_cfg, adxl_timer_event_handler);
    APP_ERROR_CHECK(err_code);

    time_ticks = gaExtTriggerTicks[nOdrIndex];
    gExtTriggerFreqIndex = nOdrIndex;

    /*! Update the decimation factor with new timer frequency*/
    gAdpdDecFactor = gaAdpdTriggerFreq[nOdrIndex]/gaAdpdTriggerFreq[gAdpdFreqIndex];//1 << (nOdrIndex - gAdpdFreqIndex);
    gAdxlDecFactor = gaAdpdTriggerFreq[nOdrIndex]/gaAdpdTriggerFreq[gAdxlFreqIndex];//1 << (nOdrIndex - gAdxlFreqIndex);

    nrf_drv_timer_extended_compare(&TIMER_ADXL, NRF_TIMER_CC_CHANNEL0, time_ticks,\
                                  NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);
    nrf_drv_timer_enable(&TIMER_ADXL);
    gTimerRunning = 1;
}

void restart_trigger_timer(uint8_t nOdrIndex)
{
    uint32_t time_ticks;
    uint32_t err_code = NRF_SUCCESS;

    nrf_drv_timer_uninit(&TIMER_ADXL);    /*! Stop the Timer */

    //nrf_drv_timer_config_t timer_cfg = NRF_DRV_TIMER_DEFAULT_CONFIG;
    nrf_drv_timer_config_t timer_cfg = {                                                                                    \
    .frequency          = (nrf_timer_frequency_t)NRFX_TIMER_DEFAULT_CONFIG_FREQUENCY,\
    .mode               = (nrf_timer_mode_t)NRFX_TIMER_DEFAULT_CONFIG_MODE,          \
    .bit_width          = (nrf_timer_bit_width_t)NRFX_TIMER_DEFAULT_CONFIG_BIT_WIDTH,\
    .interrupt_priority = APP_IRQ_PRIORITY_HIGHEST,                    \
    .p_context          = NULL                                                       \
    };
    err_code = nrf_drv_timer_init(&TIMER_ADXL, &timer_cfg, adxl_timer_event_handler);
    APP_ERROR_CHECK(err_code);

    time_ticks = gaExtTriggerTicks[nOdrIndex];    /*! Restart the timer with new frequency */
    gExtTriggerFreqIndex = nOdrIndex;

    /*! Update the decimation factor with new timer frequency*/
    gAdpdDecFactor = gaAdpdTriggerFreq[nOdrIndex]/gaAdpdTriggerFreq[gAdpdFreqIndex];//1 << (nOdrIndex - gAdpdFreqIndex);
    gAdxlDecFactor = gaAdpdTriggerFreq[nOdrIndex]/gaAdpdTriggerFreq[gAdxlFreqIndex];//1 << (nOdrIndex - gAdxlFreqIndex);

    nrf_drv_timer_extended_compare(
         &TIMER_ADXL, NRF_TIMER_CC_CHANNEL0, time_ticks, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);
    nrf_drv_timer_enable(&TIMER_ADXL);
}

void disable_trigger_timer(uint8_t nOdrIndex)
{
  if(gsTrigTimerStartCnt == 1)
  {
    gsTrigTimerStartCnt = 0;
    nrf_drv_timer_uninit(&TIMER_ADXL);  /*! stop timer if only one stream is running*/
    /*! Reset all variables to default values*/
    gTriggerTimerCnt = 0;
    gTimerRunning = 0;
    gAdxlFreqIndex = 2;
    gAdpdFreqIndex = 2;
    gExtTriggerFreqIndex = 2;
    gAdpdDecFactor = 1;
    gAdxlDecFactor = 1;
  }
  else
  {
    gsTrigTimerStartCnt--;
    /*! If other stream runs at lower freqeuncy; then restart the timer with lower frequency required*/
    if(nOdrIndex == gExtTriggerFreqIndex)
    {
      if(nOdrIndex != gAdpdFreqIndex)
      {
         restart_trigger_timer(gAdpdFreqIndex);
         gAdpdDecFactor = 1;
      }
      else if(nOdrIndex != gAdxlFreqIndex)
      {
         restart_trigger_timer(gAdxlFreqIndex);
         gAdxlDecFactor = 1;
      }
    }
  }
}

/*
  Flag to indicate if Timer is currently running
  0 --> Timer is Off
  1 --> Timer is On

  By default flag is set to '0'
*/

void enable_adxl_ext_trigger(uint8_t nOdrIndex)
{
    uint32_t time_ticks;
    uint32_t err_code = NRF_SUCCESS;
    enable_adxl_trigger_pin();
    nOdrIndex = map_adxl_freq_to_index(nOdrIndex);
    if((nOdrIndex > gExtTriggerFreqIndex) || (!gTimerRunning))
    {
      //restart timer; adjust the dec factors
      gAdxlFreqIndex = nOdrIndex;
      if(gTimerRunning)
        restart_trigger_timer(nOdrIndex);
      else
        enable_trigger_timer(nOdrIndex);

    }
    else
    {
      gAdxlFreqIndex = nOdrIndex;
      gAdxlDecFactor = gaAdpdTriggerFreq[gExtTriggerFreqIndex]/gaAdpdTriggerFreq[nOdrIndex];//1 << (nOdrIndex - gAdxlFreqIndex);
    }
    gsTrigTimerStartCnt++;
    gAdxlExtTimerRunning = 1;
    gResetTriggerPulseStates = 1;
    gAdxlRunning = 1;
}


void disable_adxl_ext_trigger(uint8_t nOdrIndex)
{
    nOdrIndex = map_adxl_freq_to_index(nOdrIndex);
    gAdxlRunning = 0;
    disable_trigger_timer(nOdrIndex);
    disable_adxl_trigger_pin();
    gAdxlExtTimerRunning = 0;
}

void set_adxl_trigger_freq(uint8_t nFreq)
{
  nFreq = map_adxl_freq_to_index(nFreq);
  gAdxlFreqSelected = nFreq;
  if(gAdxlRunning && (nFreq != gAdxlFreqIndex))
  {
    /*
      If Timer is currently running, first stop the timer.
      Then restart the timer with new Frequency requested
    */
    disable_adxl_ext_trigger(gAdxlFreqIndex);
//    AdxlDrvExtSampleMode(PRM_EXT_SAMPLE_DISABLE);
//    AdxlDrvSetOperationMode(PRM_MEASURE_STANDBY_MODE);
//
//    AdxlDrvSetOperationMode(PRM_MEASURE_MEASURE_MODE);
//    AdxlDrvExtSampleMode(PRM_EXT_SAMPLE_ENABLE);
    gAdxlFreqIndex = nFreq;
    enable_adxl_ext_trigger(nFreq);
  }
}

void enable_adpd_ext_trigger(uint16_t nOdr)
{
    uint8_t nOdrIndex;
    enable_adpd_trigger_pin();
    nOdrIndex = map_adpd_freq_to_index(nOdr);
    if((nOdrIndex > gExtTriggerFreqIndex) || (!gTimerRunning))
    {
      //restart timer; adjust the dec factors
      gAdpdFreqIndex = nOdrIndex;
      if(gTimerRunning)
        restart_trigger_timer(nOdrIndex);
      else
        enable_trigger_timer(nOdrIndex);
    }
    else
    {
      gAdpdFreqIndex = nOdrIndex;
      gAdpdDecFactor = gaAdpdTriggerFreq[gExtTriggerFreqIndex]/gaAdpdTriggerFreq[nOdrIndex];//1 << (nOdrIndex - gAdpdFreqIndex);
    }
    gsTrigTimerStartCnt++;
    gResetTriggerPulseStates = 1;
    gAdpdRunning = 1;
}
/*
 Function to update the ADPD/ADXL trigger frequency at run time for PPG application
 This will be called from Library through callback PpgAdpd400xSetModeCB
*/
void ppg_adjust_adpd_ext_trigger(uint16_t nOdr)
{
    uint8_t nOdrIndex;
    nOdrIndex = map_adpd_freq_to_index(nOdr);
    gAdpdFreqIndex = nOdrIndex;
    if(nOdrIndex > gExtTriggerFreqIndex)
    {
      //restart timer; adjust the dec factors
      restart_trigger_timer(nOdrIndex);
    }
    else
    {
      gAdpdDecFactor = gaAdpdTriggerFreq[gExtTriggerFreqIndex]/gaAdpdTriggerFreq[nOdrIndex];
      gAdxlDecFactor = gaAdpdTriggerFreq[gExtTriggerFreqIndex]/gaAdpdTriggerFreq[gAdxlFreqIndex];  
    }
}

void disable_adpd_ext_trigger(uint16_t nOdr)
{
    uint8_t nOdrIndex;
    gAdpdRunning = 0;
    nOdrIndex = map_adpd_freq_to_index(nOdr);
    disable_trigger_timer(nOdrIndex);
    disable_adpd_trigger_pin();
}
#endif //ADXL_EXT_TRIGGER

#ifdef ENABLE_ECG_APP
const nrf_drv_timer_t TIMER_AD5940 = NRF_DRV_TIMER_INSTANCE(3);

extern void invert_ad5940_trigger_signal();
extern void disable_ad5940_trigger_pin();
extern void reset_ad5940_trigger_signal(bool n_state);
extern void enable_ad5940_trigger_pin();
extern void Ad5940FifoCallBack(void);

volatile uint8_t gAd5940Running = 0, gAd5940TimerRunning=0;
volatile uint32_t gad5940TriggerTimerCnt =0,gad5940TriggerPulseCnt = 0;

#define ECG_WATER_MARK_LEVEL    16U

/*FIFO_THRESHOLD_TRIGGERS_CNT = 2 * ECG_WATER_MARK_LEVEL/EDA_WATER_MARK_LEVEL */
#define FIFO_THRESHOLD_TRIGGERS_CNT       32U

#ifdef DEBUG_ECG
uint32_t timer_start=0,timer_end=0,timer_diff=0,time_gap_based_on_odr;
extern uint32_t ecgodr;
uint32_t time_stamps_deviated[50]={0};
uint32_t ts_cnt=0;
uint16_t fifo_level_timer=0;
uint16_t Fifotimer_count=0;
uint32_t ecg_tick=0,ecg_tick_prev=0,ecg_load_time_diff=0;
#endif

extern uint8_t gnAd5940TimeIrqSet, gnAd5940TimeIrqSet1;
extern uint32_t gn_ad5940_isr_cnt;
extern uint16_t fifo_read_flag;
extern uint32_t gnAd5940TimeCurVal;
extern uint32_t gnAd5940TimeCurVal_copy;
extern volatile uint8_t gnAd5940DataReady;
extern ADI_OSAL_SEM_HANDLE   ecg_task_evt_sem;
#ifdef EXTERNAL_TRIGGER_EDA
extern ADI_OSAL_SEM_HANDLE eda_task_evt_sem;
#endif
volatile uint16_t fifo_threshold_not_four=0;
volatile uint8_t debug_flag=0;
uint8_t gResetAd5940TriggerPulse = 0;

#ifdef ECG_POLLING
struct gTrigInfo{
uint32_t gaTrigTime[100];
uint32_t gaTrigCnt[100];
uint16_t nIndex;
}TrigArr;
#endif

/*!
 ****************************************************************************
 *@brief      Timer event handler to handle AD5940 data collection, events
              are generated from Nordic Internal timer and driven through 
              External trigger pin connected to AD5940
 *@param      event_type: Event type of type 'nrf_timer_event_t'
              p_context: pointer to context and is defined according to Nordic
              timer event call back
 *@return     None
 ******************************************************************************/
void ad5940_timer_event_handler(nrf_timer_event_t event_type, void* p_context)
{
    switch (event_type)
    {
        case NRF_TIMER_EVENT_COMPARE0:
              if(gAd5940Running == 1)
              {
                gad5940TriggerTimerCnt++;
                if(gResetAd5940TriggerPulse)
                {
                  gResetAd5940TriggerPulse = 0;
                  gad5940TriggerTimerCnt = 0;
                  gad5940TriggerPulseCnt = 0;
                  reset_ad5940_trigger_signal(0);  /* set the trigger signal high*/
                }
                else
                {
                  if((gad5940TriggerTimerCnt%gAd5940DecFactor) == 0)
                  {
                    invert_ad5940_trigger_signal();
                    gad5940TriggerPulseCnt++;
                    if(((gad5940TriggerPulseCnt) % FIFO_THRESHOLD_TRIGGERS_CNT) ==0)
                    {
#ifdef DEBUG_ECG
                      fifo_read_flag = 1;
#endif

                      /* Now there should be fifo data ; number of samples to read is being set in
                      ecg app which FIFO threshold variable*/
                      /* Read current time */
                      gnAd5940TimeCurVal =  get_sensor_time_stamp();
                      /* Increment isr count */
                      gn_ad5940_isr_cnt++;
#ifdef EXTERNAL_TRIGGER_EDA							
                      if(ecg_start_req== 1){
                        /*Wake up the ecg task to read the samples from FIFO*/
                        adi_osal_SemPost(ecg_task_evt_sem);
                      }
                      else if(eda_start_req== 1){
                        /*Wake up the ecg task to read the samples from FIFO*/
                        adi_osal_SemPost(eda_task_evt_sem);
                      }
#else
                      adi_osal_SemPost(ecg_task_evt_sem);						   
#endif						   
                    }
#ifdef DEBUG_ECG
                    else{
                      if(fifo_read_flag == 1 && (((gad5940TriggerPulseCnt-4)%2) ==0) ){
                      gnAd5940TimeCurVal_copy = gnAd5940TimeCurVal;
                      gnAd5940TimeCurVal = get_sensor_time_stamp();
                      if(gnAd5940TimeCurVal_copy != gnAd5940TimeCurVal){
                        fifo_threshold_not_four += 1;
                        debug_flag=1;
                      }
                    }
                      /* post ecg semaphore */
                     adi_osal_SemPost(ecg_task_evt_sem);
                  }
#endif
                 }
               }
#ifdef ECG_POLLING
                TrigArr.gaTrigTime[TrigArr.nIndex] = get_sensor_time_stamp();
                TrigArr.gaTrigCnt[TrigArr.nIndex] = gad5940TriggerTimerCnt;
                if(++TrigArr.nIndex >= 99)
                  TrigArr.nIndex = 0;
#endif
              }
          default:
            //Do nothing.
            break;
     }
}

void  enable_ad5940_trigger_timer(uint8_t nOdrIndex)
{
    uint32_t time_ticks;
    uint32_t err_code = NRF_SUCCESS;

    nrf_drv_timer_config_t timer_cfg = NRF_DRV_TIMER_DEFAULT_CONFIG;
    err_code = nrf_drv_timer_init(&TIMER_AD5940, &timer_cfg, ad5940_timer_event_handler);
    APP_ERROR_CHECK(err_code);
    
    time_ticks = gaExtAd5940TriggerTicks[nOdrIndex];
    gExtad5940TriggerFreqIndex = nOdrIndex;
    gAd5940DecFactor = gaAd5940TriggerFreq[nOdrIndex]/gaAd5940TriggerFreq[gAd5940FreqIndex];//1 << (nOdrIndex - gAdxlFreqIndex);
    nrf_drv_timer_extended_compare(&TIMER_AD5940, NRF_TIMER_CC_CHANNEL0, time_ticks, \
                                  NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);
    nrf_drv_timer_enable(&TIMER_AD5940);
    gAd5940TimerRunning = 1;
}

uint16_t gad5940TriggerTimerStartCnt =0;
void disable_ad5940_trigger_timer(uint8_t nOdrIndex)
{
  if(gad5940TriggerTimerStartCnt == 1)
  {
    gad5940TriggerTimerStartCnt = 0;
    nrf_drv_timer_uninit(&TIMER_AD5940);  /*! stop timer if only one stream is running*/
    /*! Reset all variables to default values*/
    gad5940TriggerTimerCnt = 0;
    gAd5940TimerRunning = 0;
    gAd5940FreqIndex = 2;
    gExtad5940TriggerFreqIndex = 2;
    gAd5940DecFactor = 1;
  }
  /*This case is not required as single stream is running on the timer*/
//  else
//  {
//    gad5940TriggerTimerStartCnt--;
//    /*! If other stream runs at lower freqeuncy; then restart the timer with lower frequency required*/
//    if(nOdrIndex == gExtad5940TriggerFreqIndex)
//    {
//        restart_ad5940_trigger_timer(gAd5940FreqIndex);
//    }
//  }
}

void restart_ad5940_trigger_timer(uint8_t nOdrIndex)
{
    uint32_t time_ticks;
    uint32_t err_code = NRF_SUCCESS;

    nrf_drv_timer_uninit(&TIMER_AD5940);    /*! Stop the Timer */

    nrf_drv_timer_config_t timer_cfg = NRF_DRV_TIMER_DEFAULT_CONFIG;
    err_code = nrf_drv_timer_init(&TIMER_AD5940, &timer_cfg, ad5940_timer_event_handler);
    APP_ERROR_CHECK(err_code);

    time_ticks = gaExtAd5940TriggerTicks[nOdrIndex];    /*! Restart the timer with new frequency */
    gExtad5940TriggerFreqIndex = nOdrIndex;
    gAd5940DecFactor = gaAd5940TriggerFreq[nOdrIndex]/gaAd5940TriggerFreq[gAd5940FreqIndex];//1 << (nOdrIndex - gAdxlFreqIndex);
    nrf_drv_timer_extended_compare(&TIMER_AD5940, NRF_TIMER_CC_CHANNEL0,\
                                  time_ticks, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);
    nrf_drv_timer_enable(&TIMER_AD5940);
}

uint8_t map_ad5940_freq_to_index(uint16_t nOdr)
{
  int i;
  for(i=0; i<TRIGGER_AD5940_FREQ_STEPS; i++)
  {
    if(gaAd5940TriggerFreq[i] >= nOdr)
    {
        return i;
    }
  }
  if(i==TRIGGER_AD5940_FREQ_STEPS)
    return 2; /*! Wrong ODR chosen, set ODR to 50Hz */
}

void enable_ad5940_ext_trigger(uint16_t nOdr)
{
    uint8_t nOdrIndex;
    enable_ad5940_trigger_pin();
    reset_ad5940_trigger_signal(1); /* set the trigger signal low*/
    nOdrIndex = map_ad5940_freq_to_index(nOdr);
#ifdef EXTERNAL_TRIGGER_EDA		
    NRF_LOG_INFO("ODR=%d,ODR Index=%d",nOdr,nOdrIndex);
    if(ecg_start_req== 1){
      if((nOdrIndex == 0)||(nOdrIndex == 1)){
        nOdrIndex = ((nOdrIndex == 0)? (nOdrIndex+2):(nOdrIndex+1));
       }
     }
     else if(eda_start_req == 1){
        if(nOdrIndex > 4){
          return; /* eda doesnt support greater than 30 Hz*/ 
        }
     }
#endif
    if((nOdrIndex > gExtad5940TriggerFreqIndex) || (!gAd5940TimerRunning))
    {
      //restart timer; adjust the dec factors
      gAd5940FreqIndex = nOdrIndex;
      if(gAd5940TimerRunning)
        restart_ad5940_trigger_timer(nOdrIndex);
      else
        enable_ad5940_trigger_timer(nOdrIndex);
    }
    else
    {
      gAd5940FreqIndex = nOdrIndex;
      gAd5940DecFactor = gaAd5940TriggerFreq[gExtad5940TriggerFreqIndex]/gaAd5940TriggerFreq[nOdrIndex];//1 << (nOdrIndex - gAdpdFreqIndex);
    }
    gad5940TriggerTimerStartCnt++;
    gResetAd5940TriggerPulse = 1;
    gAd5940Running = 1;
}

void disable_ad5940_ext_trigger(uint16_t nOdr)
{
    uint8_t nOdrIndex;
    nOdrIndex = map_ad5940_freq_to_index(nOdr);
    gAd5940Running = 0;
    disable_ad5940_trigger_timer(nOdrIndex);
    disable_ad5940_trigger_pin();
}
#endif //ENABLE_ECG_APP