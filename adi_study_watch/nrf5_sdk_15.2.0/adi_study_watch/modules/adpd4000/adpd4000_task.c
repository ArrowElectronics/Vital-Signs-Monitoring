/**
***************************************************************************
* @addtogroup Tasks
* @{
* @file         adpd4000_task.c
* @author       ADI
* @version      V1.0.0
* @date         17-April-2019
* @brief        Source file contains adpd processing wrapper.
***************************************************************************
* @attention
***************************************************************************
*/
/*!
*  \copyright Analog Devices
* ****************************************************************************
*
* License Agreement
*
* Copyright (c) 2020 Analog Devices Inc.
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
* ****************************************************************************
*/
#include <adpd4000_buffering.h>
#include <adpd4000_dcfg.h>
#include <adpd4000_task.h>
#include <adpd400x_reg.h>
#include <clock_calibration.h>
#include "adpd400x_lib.h"
#include "agc.h"
#include "app_common.h"
#ifdef ENABLE_BIA_APP
#include "bia_application_task.h"
#endif

#ifdef ENABLE_EDA_APP
#include "eda_application_task.h"
#endif
#include <ecg_application_interface.h>
#include <ecg_task.h>
#ifdef ENABLE_SQI_APP
#include "sqi_task.h"
#endif
#ifdef ENABLE_PPG_APP
#include <mw_ppg.h>
#include <struct_operations.h>
#endif
#include "lcm.h"
#include "nrf_log_ctrl.h"
#ifdef DCB
#include "adi_dcb_config.h"
#include <dcb_interface.h>
#endif
#include "power_manager.h"
#include "us_tick.h"
#include "app_sync.h"
#include "low_touch_task.h"
#include "ppg_application_interface.h"
/* System Task Module Log settings */
#include "nrf_log.h"
#ifdef ENABLE_DEBUG_STREAM
#include <debug_interface.h>
#endif
/*---------------------------- Defines --------------------------------------*/
#define MAX_PAYLOAD_BYTE 24
#define TOTAL_SLOT 12 /* total number of slots */
#define SAMPLE_NUM 1
#define ADPD_APP_ROUTING_TBL_SZ                                                \
  (sizeof(adpd4000_app_routing_table) / sizeof(adpd4000_app_routing_table[0]))
#define SECONDS_PER_MIN               60U   //! Number of seconds per minute
#define SKIP_STATIC_AGC_SAMPLES     40
#ifdef ENABLE_DEBUG_STREAM
#define M2M2_DEBUG_INFO_SIZE   4
#define DEBUG_INFO_SIZE    M2M2_DEBUG_INFO_SIZE + 6 // 6 here to avoid overflow   
#endif

/*--------------------------- Typedef ---------------------------------------*/

/*!
 * @brief:  The dictionary type for mapping addresses to callback handlers.
 */
typedef m2m2_hdr_t *(app_cb_function_t)(m2m2_hdr_t *);

typedef struct _app_routing_table_entry_t {
  uint8_t command;
  app_cb_function_t *cb_handler;
} app_routing_table_entry_t;

#ifdef ENABLE_DEBUG_STREAM
static struct _g_adpd_debug_data{
  m2m2_hdr_t   *p_pkt;
  uint8_t num_subs;
  uint16_t  data_pkt_seq_num;
} g_adpd_debug_data;
#endif
/*------------------------------ Public Variable ----------------------------*/
slot_led_reg_t led_reg;
bool gAdpdPause = false;
uint32_t gAdpdIntCnt = 0;
volatile uint8_t gsOneTimeValueWhenReadAdpdData = 0;
uint8_t gsOneTimeValueWhenReadData = 0;
#ifdef ENABLE_SQI_APP
/* flag to check if adpd data is externally fed for sqi stream */
bool gAdpd_ext_data_stream_active = false;
/* odr for externally fed adpd data */
uint16_t gAdpd_ext_data_stream_odr = 50U;
#endif

/* Application status trace */
g_state_t g_state;
g_state_agc g_state_static_agc;
ADPD_TS_DATA_TYPE gADPD4000_dready_ts_prev, g_time_diff;
static uint16_t g_adpd_reg_base;
/****Flags to control AGC for 4 LEDs ****/
uint32_t gn_agc_active_slots;
static uint32_t gsAgcSampleCnt[SLOT_NUM];
static uint8_t gsAgcFlag = 0;
uint32_t gn_led_slot_g = 0;
uint32_t gn_led_slot_r = 0;
uint32_t gn_led_slot_ir = 0;
uint32_t gn_led_slot_b = 0;
bool gRun_agc = false;
static uint32_t gsResetStaticAgcSampleCnt = 0; //!< To count number of samples to recalibrate the signal
/*************************************/

/* flag to check if loadAdpdCfg cmd is used to load the adpd dcfg */
static bool external_load_dcfg = false;
/*
** All chips prior to DVT2 have chipID as 0xC0 corresponding to ADPD4000
** dvt2=0 --> older boards (nRF DK+ sensor board, EVT, DVT1)
** dvt2=1 --> DVT2 upwards
*/
uint8_t dvt2 = 0; /* whether DVT2 or not */

/* counter used to send every kth sample to temp task
   to have temp stream odr = 1Hz, where k = odr of adpd4k temp slot */
uint32_t gsTempSampleCount = 0;
#ifdef SLOT_SELECT
/* counter used to send every kth sample to ppg task
   to have ppg odr = 50Hz, where k = odr of adpd4k ppg slot */
uint32_t gsSampleCount1 = 0;
#endif
/* counter used to send every kth sample to HR algo
   to have ppg odr = 50Hz, where k = odr of adpd4k*/
uint32_t gnHRAdpdSampleCount = 0;
/* flag is set when sampling rate is changed */
volatile uint8_t gbAdpdSamplingRateChanged;
extern uint16_t gAdpdSlotMaxODR; /* Maximum ODR among of all the active slots */
#ifdef ENABLE_PPG_APP
extern Adpd400xLibConfig_t gAdpd400xLibCfg;
extern uint32_t gnHRAdxlSampleCount;
volatile uint8_t gn_uc_hr_enable = 0;
uint16_t gn_uc_hr_slot = ADPD4K_SLOT_F;
#endif

volatile uint8_t gb_adpd_multi_sub_start = 0; /* Flag to handle whether ADPD sensor was 'start'ed and multiple subscribers(more than 1) added to get raw data(CLI_USB/CLI_BLE)*/

/* assign the connected sensor, which can used other application task */
extern ADPD_TS_DATA_TYPE gADPD4000_dready_ts;
extern uint8_t nFifoStatusByte;
extern uint32_t nWriteSequnce;
uint16_t decimation_info[SLOT_NUM] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
extern uint32_t gnLcmValue;
extern uint32_t nReadSequence;
extern uint32_t nInterruptSequence;
extern uint32_t nWriteSequence;
extern adpd400xDrv_slot_t gsSlot[SLOT_NUM];
extern uint8_t gnAdpdFifoWaterMark;
#ifdef ENABLE_PPG_APP
/* flag to check if agc ON for PPG or UC HR */
extern uint8_t gPpg_agc_en;
/*
following flag is set in two cases for PPG or UC HR
    - AGC is ON and AGC calibration done
    - AGC is OFF and Setting Default Curr/Gain from LCFG done
*/
extern uint8_t gPpg_agc_done;
extern uint32_t Ppg_Slot;
extern volatile uint8_t gnAppSyncTimerStarted;
#endif
#ifdef ENABLE_TEMPERATURE_APP
extern uint32_t g_therm_slot_en_bit_mask;
extern uint16_t gsTemperatureStarts;
#endif
extern agc_data_t agc_data[SLOT_NUM];
#ifdef ENABLE_BIA_APP
extern g_state_bia_t g_state_bia;
#endif
#ifdef ENABLE_ECG_APP
extern g_state_ecg_t g_state_ecg;
#endif
#ifdef ENABLE_EDA_APP
extern g_state_eda_t g_state_eda;
#endif
#ifdef ENABLE_SQI_APP
extern uint8_t sqi_event;
extern uint32_t gnSQI_Slot;
#endif
extern uint16_t g_adpd_odr; /**/
extern uint16_t get_adpd_odr();

#ifdef ENABLE_DEBUG_STREAM
uint8_t g_adpdOffset = 0;
extern uint8_t g_adpd_ts_flag_set;
uint32_t g_adpd_debugInfo[DEBUG_INFO_SIZE];
extern uint64_t g_adpd_rtc_info[];
void packetize_adpd_debug_data(void);
#endif
#ifdef DCB
/* Reuse the gsConfigTmpFile[] RAM buffer used in system_task.c for
 * holding copy of either DCB/NAND config file which has LT
 * start/stop sequence of commands.
 * This buffer is to be reused between holding the copy of LT config file
 * as well as temporary storage for DCB content during READ/WRITE DCB block
 * for ADPD, Gen block DCB
 * Buffer to be shared between tasks( General Blk DCB & ADPD4000 DCB ) */
extern uint8_t gsConfigTmpFile[];
#endif

void get_agc_info(m2m2_adpd_agc_info_t *agc_info);
void reset_agc_flags(void);
#ifdef ENABLE_PPG_APP
static void update_ppg_default_current_gain(uint8_t slot_id);
#endif
/*------------------------------ Public Function Prototype ------------------ */
#ifdef ENABLE_PPG_APP
extern void RegisterPpgCB(int16_t (*pfppg_set_opmode)(uint8_t));
extern StructOpErrorStatus GetAdpd400xLcfgStructure(
    int32_t *pValue, uint8_t *nSize);

extern StructOpErrorStatus ReadAdpd400xLcfgStructureRaw(
    uint32_t field, int32_t *value);

extern StructOpErrorStatus ModifyAdpd400xLcfgStructureRaw(
    uint32_t field, int32_t val);

extern void RegisterMwPpgGetLCFGRegCB(
    StructOpErrorStatus (*pfn)(int32_t *, uint8_t *));

extern void RegisterMwPpgReadLCFGRegCB(
    StructOpErrorStatus (*pfn)(uint32_t, int32_t *));

extern void RegisterMwPpgModifyLCFGRegCB(
    StructOpErrorStatus (*pfn)(uint32_t, int32_t));

extern void SyncAppDataSend(uint32_t *p_data, uint32_t adpdtimestamp,
    int16_t *pAdxlData, uint32_t adxltimestamp);
extern void app_sync_timer_init(void);
extern uint16_t Adpd400xUtilGetRegFromMonotonicCurrent(uint8_t ncurrent);
#endif

#ifdef ENABLE_TEMPERATURE_APP
extern void temperatureAppData (uint32_t *pData, uint8_t slot_index);
#endif
extern void ADP5360_gpio_interrupt_enable();
extern void enable_ext_syncmode();
extern void disable_ext_syncmode();
/*---------------------- Private variables -----------------------------------*/

static M2M2_ADDR_ENUM_t gsStream[SLOT_NUM] = {
    M2M2_ADDR_SENSOR_ADPD_STREAM1,
    M2M2_ADDR_SENSOR_ADPD_STREAM2,
    M2M2_ADDR_SENSOR_ADPD_STREAM3,
    M2M2_ADDR_SENSOR_ADPD_STREAM4,
    M2M2_ADDR_SENSOR_ADPD_STREAM5,
    M2M2_ADDR_SENSOR_ADPD_STREAM6,
    M2M2_ADDR_SENSOR_ADPD_STREAM7,
    M2M2_ADDR_SENSOR_ADPD_STREAM8,
    M2M2_ADDR_SENSOR_ADPD_STREAM9,
    M2M2_ADDR_SENSOR_ADPD_STREAM10,
    M2M2_ADDR_SENSOR_ADPD_STREAM11,
    M2M2_ADDR_SENSOR_ADPD_STREAM12,
};

#ifndef ADPD_SEM_CORRUPTION_DEBUG
/* Create the stack for task */
static uint8_t
    sensor_adpd4000_task_stack[APP_OS_CFG_SENSOR_ADPD4000_TASK_STK_SIZE];
#else
/* Create the stack for task */
 uint8_t
    sensor_adpd4000_task_stack[2048];
#endif

/* Create handler for task */
static ADI_OSAL_THREAD_HANDLE sensor_adpd4000_task_handler;

/* Create task attributes variable */
static ADI_OSAL_STATIC_THREAD_ATTR sensor_adpd4000_task_attributes;

/* Create TCB for task */
static StaticTask_t adpd4000TaskTcb;

/* Create semaphores */
ADI_OSAL_SEM_HANDLE adpd4000_task_evt_sem;

#ifdef ADPD_SEM_CORRUPTION_DEBUG
int gAdpdPendFlag = 0;
uint8_t *pAdpdSemPtr = NULL;
#endif

ADI_OSAL_SEM_HANDLE adpd4000_task_floatmode_sem, adpd4000_task_setDarkValue_sem,
    adpd4000_task_getCtr_sem;

/* Create Queue Handler for task */
static ADI_OSAL_QUEUE_HANDLE adpd4000_task_msg_queue = NULL;

const char GIT_ADPD4000_VERSION[] = "TEST ADPD4000_VERSION STRING";
const uint8_t GIT_ADPD4000_VERSION_LEN = sizeof(GIT_ADPD4000_VERSION);

static fifo_data_pattern sReadBufferPattern;
static m2m2_hdr_t *pOptionalPkt;
static m2m2_sensor_fifo_status_bytes_t _PreviousStatus;
static bool check_dcb_erase = false;
/*------------------------ Private Function Prototype -----------------------*/
static m2m2_hdr_t *adpd_app_reg_access(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *adpd_app_stream_config(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *adpd_app_status(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *adpd_app_do_clock_cal(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *adpd_app_load_cfg(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *adpd_app_slotmode(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *adpd_app_slot_active_mode(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *adpd_app_get_dcfg(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *adpd_app_decimation(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *adpd_app_get_version(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *adpd_app_do_general1_cmd(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *adpd_app_do_general2_cmd(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *adpd_app_do_general3_cmd(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *adpd_app_set_pause(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *adpd_app_get_com_mode(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *adpd_app_create_dcfg(m2m2_hdr_t *p_pkt);
#ifdef SLOT_SELECT
static m2m2_hdr_t *adpd_app_disable_slot(m2m2_hdr_t *p_pkt);
#endif
static m2m2_hdr_t *ecg4k_lcfg_access(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *set_adpd4000_fs(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *adpd_app_agc_on_off_cntrl(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *adpd_app_agc_info(m2m2_hdr_t *p_pkt);
#ifdef ENABLE_SQI_APP
static m2m2_hdr_t *adpd_app_ext_adpd_datastream(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *adpd_app_set_ext_adpd_datastream_odr(m2m2_hdr_t *p_pkt);
#endif
#ifdef DCB
static m2m2_hdr_t *adpd_dcb_command_read_config(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *adpd_dcb_command_write_config(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *adpd_dcb_command_delete_config(m2m2_hdr_t *p_pkt);
#endif
#ifdef ENABLE_PPG_APP
static m2m2_hdr_t *adpd_app_uc_hr_enable(m2m2_hdr_t *p_pkt);
#endif
static void adpd_data_ready_cb(void);
static uint8_t fetch_adpd_data(void);
static void sensor_adpd4000_task(void *pArgument);
static void reset_adpd_packetization(void);
static bool _OptionalByteArrange(uint8_t *, uint16_t, uint32_t, m2m2_hdr_t *);
static void Adpd400xUpdateStaticAgcInfo(AGCState_t *pnagcInfo,uint8_t slotnum);
static void packetize_adpd_static_agc_settings(void);
static void static_agc_stream_state_reset(void);
#ifdef OS_TIMER
static OS_TMR gsPollFIFO_Timer;
#endif
static bool bIsStatusStreamEnabled = false;
static uint16_t nStatusRegValue = 0;

/* For NK UC - ADPD@500Hz, ADXL@50Hz, EDA@8Hz, Temp@1Hz it is required to keep
 * ADPD task at higher priority */
/* Flag which holds the status of ADPD task priority change */
static volatile uint8_t gbAdpdHighPrio = 0;

/* ADPD task priority level to be kept when ADPD runs at 500Hz */
#define ADPD_500Hz_TASK_PRIO 7

/*!
 *
 * @brief Array for routing m2m2 pkt to different function in ADPD4k task
 */
app_routing_table_entry_t adpd4000_app_routing_table[] = {
    {M2M2_APP_COMMON_CMD_STREAM_START_REQ, adpd_app_stream_config},
    {M2M2_APP_COMMON_CMD_STREAM_STOP_REQ, adpd_app_stream_config},
    {M2M2_APP_COMMON_CMD_STREAM_SUBSCRIBE_REQ, adpd_app_stream_config},
    {M2M2_APP_COMMON_CMD_STREAM_UNSUBSCRIBE_REQ, adpd_app_stream_config},
    {M2M2_APP_COMMON_CMD_SENSOR_STATUS_QUERY_REQ, adpd_app_status},
    {M2M2_APP_COMMON_CMD_WRITE_LCFG_REQ, ecg4k_lcfg_access},
    {M2M2_APP_COMMON_CMD_READ_LCFG_REQ, ecg4k_lcfg_access},
    {M2M2_SENSOR_ADPD_COMMAND_SET_FS_REQ, set_adpd4000_fs},
    {M2M2_SENSOR_ADPD_COMMAND_LOAD_CFG_REQ, adpd_app_load_cfg},
    {M2M2_SENSOR_ADPD_COMMAND_CLOCK_CAL_REQ, adpd_app_do_clock_cal},
    {M2M2_SENSOR_ADPD_COMMAND_SET_SLOT_REQ, adpd_app_slotmode},
    {M2M2_SENSOR_ADPD_COMMAND_GET_SLOT_REQ, adpd_app_slotmode},
    {M2M2_SENSOR_ADPD_COMMAND_SET_SLOT_ACTIVE_REQ, adpd_app_slot_active_mode},
    {M2M2_SENSOR_ADPD_COMMAND_GET_SLOT_ACTIVE_REQ, adpd_app_slot_active_mode},
    {M2M2_SENSOR_ADPD_COMMAND_SET_PAUSE_REQ, adpd_app_set_pause},
    {M2M2_SENSOR_COMMON_CMD_READ_REG_16_REQ, adpd_app_reg_access},
    {M2M2_SENSOR_COMMON_CMD_WRITE_REG_16_REQ, adpd_app_reg_access},
    {M2M2_SENSOR_COMMON_CMD_GET_DCFG_REQ, adpd_app_get_dcfg},
    {M2M2_SENSOR_COMMON_CMD_GET_STREAM_DEC_FACTOR_REQ, adpd_app_decimation},
    {M2M2_SENSOR_COMMON_CMD_SET_STREAM_DEC_FACTOR_REQ, adpd_app_decimation},
    {M2M2_APP_COMMON_CMD_GET_VERSION_REQ, adpd_app_get_version},
    {M2M2_SENSOR_ADPD_COMMUNICATION_MODE_REQ, adpd_app_get_com_mode},
    {M2M2_SENSOR_ADPD_COMMAND_DO_TEST1_REQ, adpd_app_do_general1_cmd},
    {M2M2_SENSOR_ADPD_COMMAND_DO_TEST2_REQ, adpd_app_do_general2_cmd},
    {M2M2_SENSOR_ADPD_COMMAND_DO_TEST3_REQ, adpd_app_do_general3_cmd},
    {M2M2_SENSOR_ADPD_COMMAND_CREATE_DCFG_REQ, adpd_app_create_dcfg},
#ifdef ENABLE_SQI_APP
    {M2M2_SENSOR_ADPD_COMMAND_EXT_ADPD_DATA_STREAM, adpd_app_ext_adpd_datastream},
    {M2M2_SENSOR_ADPD_COMMAND_SET_EXT_DATA_STREAM_ODR_REQ, adpd_app_set_ext_adpd_datastream_odr},
#endif
#ifdef SLOT_SELECT
    {M2M2_SENSOR_ADPD_COMMAND_DISABLE_SLOTS_REQ, adpd_app_disable_slot},
#endif
    {M2M2_SENSOR_ADPD_COMMAND_AGC_ON_OFF_REQ, adpd_app_agc_on_off_cntrl},
    {M2M2_SENSOR_ADPD_COMMAND_AGC_INFO_REQ, adpd_app_agc_info},
#ifdef DCB
    {M2M2_DCB_COMMAND_READ_CONFIG_REQ, adpd_dcb_command_read_config},
    {M2M2_DCB_COMMAND_WRITE_CONFIG_REQ, adpd_dcb_command_write_config},
    {M2M2_DCB_COMMAND_ERASE_CONFIG_REQ, adpd_dcb_command_delete_config},
#endif
#ifdef ENABLE_PPG_APP
    {M2M2_SENSOR_ADPD_COMMAND_UC_HR_ENAB_REQ, adpd_app_uc_hr_enable},
#endif
};

#ifdef USER0_CONFIG_APP
#include "user0_config_app_task.h"
#include "app_timer.h"
#include "low_touch_task.h"
APP_TIMER_DEF(m_adpd_timer_id);     /**< Handler for repeated timer for ADPD. */
static void adpd_timer_start(void);
static void adpd_timer_stop(void);
static void adpd_timeout_handler(void * p_context);
void start_adpd_app_timer();
user0_config_app_timing_params_t adpd_app_timings = {0};
extern user0_config_app_timing_params_t temp_app_timings;
/**@brief Function for the Timer initialization.
*
* @details Initializes the timer module. This creates and starts application timers.
*
* @param[in]  None
*
* @return     None
*/
static void adpd_timer_init(void)
{
    ret_code_t err_code;

    /*! Create timers */
    err_code =  app_timer_create(&m_adpd_timer_id, APP_TIMER_MODE_REPEATED, adpd_timeout_handler);

    APP_ERROR_CHECK(err_code);
}

/**@brief   Function for starting application timers.
* @details Timers are run after the scheduler has started.
*
* @param[in]  None
*
* @return     None
*/
static void adpd_timer_start(void)
{
    /*! Start repeated timer */
    ret_code_t err_code = app_timer_start(m_adpd_timer_id, APP_TIMER_TICKS(TIMER_ONE_SEC_INTERVAL), NULL);
    APP_ERROR_CHECK(err_code);

    adpd_app_timings.check_timer_started = true;
}

/**@brief   Function for stopping the application timers.
*
* @param[in]  None
*
* @return     None
*/
static void adpd_timer_stop(void)
{
    /*! Stop the repeated timer */
    ret_code_t err_code = app_timer_stop(m_adpd_timer_id);
    APP_ERROR_CHECK(err_code);

    adpd_app_timings.check_timer_started = false;
}

/**@brief   Callback Function for application timer events
*
* @param[in]  p_context: pointer to the callback function arguments
*
* @return     None
*/
static void adpd_timeout_handler(void * p_context)
{
    if(adpd_app_timings.delayed_start && adpd_app_timings.start_time>0)
    {
      adpd_app_timings.start_time_count++; /*! Increment counter every sec., till it is equal to start_time Value in seconds. */
      if(adpd_app_timings.start_time_count == adpd_app_timings.start_time)
      {
        /* if Temperarture stream was previously running,
        then set the correct LED current because it was set to 0
        during temperature stream start */
#ifdef ENABLE_TEMPERATURE_APP
        if ((gsTemperatureStarts > 0)) {
#ifndef SLOT_SELECT
          for (uint8_t i = 0; i < SLOT_NUM; i++) {
            g_adpd_reg_base = i * ADPD400x_SLOT_BASE_ADDR_DIFF;
            Adpd400xDrvRegWrite(ADPD400x_REG_LED_POW12_A + g_adpd_reg_base,
                led_reg.reg_val[i].reg_pow12);
            Adpd400xDrvRegWrite(ADPD400x_REG_LED_POW34_A + g_adpd_reg_base,
                led_reg.reg_val[i].reg_pow34);
          }
#endif
        }
#endif
        //delayed start time expired-turn ON ADPD
        reset_adpd_packetization();
        if (ADPD400xDrv_SUCCESS == Adpd400xDrvSetOperationMode(ADPD400xDrv_MODE_SAMPLE))
        {
        }
        adpd_app_timings.delayed_start = false;
        adpd_app_timings.start_time_count = 0;
        adpd_app_timings.app_mode_on = true;
      }
      return;
    }

    if(adpd_app_timings.app_mode_on && adpd_app_timings.on_time>0)
    {
        adpd_app_timings.on_time_count++; /*! Increment counter every sec. incase of ADPD ON, till it is equal to Ton Value in seconds. */
        if(adpd_app_timings.on_time_count == adpd_app_timings.on_time)
        {
          //on timer expired - turn off ADPD
          if (g_state.num_starts >= 1) {
              if (ADPD400xDrv_SUCCESS == Adpd400xDrvSetOperationMode(ADPD400xDrv_MODE_IDLE)) {
              }
          }
          adpd_app_timings.app_mode_on = false;
          adpd_app_timings.on_time_count = 0;
        }
    }
    else if(!adpd_app_timings.app_mode_on && adpd_app_timings.off_time>0)
    {
      adpd_app_timings.off_time_count++; /*! Increment counter every sec. incase of ADPD OFF, till it is equal to Toff Value in seconds.*/
      if(adpd_app_timings.off_time_count == adpd_app_timings.off_time)
        {
           //off timer expired - turn on ADPD
           if (g_state.num_starts >= 1) {
               if (ADPD400xDrv_SUCCESS == Adpd400xDrvSetOperationMode(ADPD400xDrv_MODE_SAMPLE))
               {
               }
           }
           adpd_app_timings.app_mode_on = true;
           adpd_app_timings.off_time_count = 0;
        }
    }
}

/**@brief   Function to start ADPD app timer
* @details  This is used in either interval based/intermittent LT mode3
*
* @param[in]  None
*
* @return     None
*/
void start_adpd_app_timer()
{
  if(adpd_app_timings.on_time > 0)
  {
    adpd_timer_start();
  }
}
#endif//USER0_CONFIG_APP

/*!
 ****************************************************************************
 * @brief ADPD4k task initialization
 *
 * @param[in]     None
 *
 * @return        None
 *****************************************************************************/
void sensor_adpd4000_task_init(void) {
  /* Initialize app state */
  g_state.num_starts = 0;
  /* Default behaviour is to send every packet */
  g_state.decimation_factor = 1;

  for (uint8_t i = 0; i < SLOT_NUM; i++) {
    g_state.num_subs[i] = 0;
    g_state.data_pkt_seq_num[i][CH1] = 0;
    g_state.data_pkt_seq_num[i][CH2] = 0;
  }
#ifdef ENABLE_DEBUG_STREAM
g_adpd_debug_data.num_subs = 0;
g_adpd_debug_data.data_pkt_seq_num = 0;
#endif
/*Initialize static AGC state*/
  g_state_static_agc.num_subs = 0;
  static_agc_stream_state_reset();

  ADI_OSAL_STATUS eOsStatus = ADI_OSAL_SUCCESS;
  sensor_adpd4000_task_attributes.pThreadFunc = sensor_adpd4000_task;
  sensor_adpd4000_task_attributes.nPriority =
      APP_OS_CFG_SENSOR_ADPD4000_TASK_PRIO;
  sensor_adpd4000_task_attributes.pStackBase = &sensor_adpd4000_task_stack[0];
  sensor_adpd4000_task_attributes.nStackSize =
#ifndef ADPD_SEM_CORRUPTION_DEBUG
      APP_OS_CFG_SENSOR_ADPD4000_TASK_STK_SIZE;
#else
      2048;
#endif
  sensor_adpd4000_task_attributes.pTaskAttrParam = NULL;
  sensor_adpd4000_task_attributes.szThreadName = "ADPD4000 Sensor";
  sensor_adpd4000_task_attributes.pThreadTcb = &adpd4000TaskTcb;
  eOsStatus = adi_osal_MsgQueueCreate(&adpd4000_task_msg_queue, NULL, 25);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
    Debug_Handler();
  } else {
    update_task_queue_list(
        APP_OS_CFG_ADPD4000_TASK_INDEX, adpd4000_task_msg_queue);
  }
  eOsStatus = adi_osal_ThreadCreateStatic(
      &sensor_adpd4000_task_handler, &sensor_adpd4000_task_attributes);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
    Debug_Handler();
  }

  adi_osal_SemCreate(&adpd4000_task_evt_sem, 0);
  adi_osal_SemCreate(&adpd4000_task_setDarkValue_sem, 0);
  adi_osal_SemCreate(&adpd4000_task_getCtr_sem, 0);
  adi_osal_SemCreate(&adpd4000_task_floatmode_sem, 0);
#ifdef ADPD_SEM_CORRUPTION_DEBUG
  pAdpdSemPtr = (uint8_t *)adpd4000_task_evt_sem;
#endif
#ifdef ENABLE_DEBUG_STREAM
  post_office_add_mailbox(M2M2_ADDR_SENSOR_ADPD4000, M2M2_ADDR_SYS_DBG_STREAM);
#endif
}

/*!
 ****************************************************************************
 * @brief Send msg packet to ADPD4k application from the post office
 *
 * @param[in]           p_pkt: input m2m2 packet
 *
 * @return              None
 *****************************************************************************/
void send_message_adpd4000_task(m2m2_hdr_t *p_pkt) {
  ADI_OSAL_STATUS osal_result;
  osal_result = adi_osal_MsgQueuePost(adpd4000_task_msg_queue, p_pkt);
  if (osal_result != ADI_OSAL_SUCCESS)
    post_office_consume_msg(p_pkt);
  adi_osal_SemPost(adpd4000_task_evt_sem);
}

/*!
  ****************************************************************************
* @brief - Configuring ADPD4k during bootup
*        - Interface for sending ADPD4k data to other apps
           and packetizing ADPD4k stream pkt
*
* @param[in]           pArgument not used
*
* @return              None
*****************************************************************************/
#ifdef ADPD_SEM_CORRUPTION_DEBUG
uint16_t stop_resp_sent_from_adpd_task=0;
uint16_t stop_req_adpd_task=0;
uint8_t command_type=0;
#endif
static void sensor_adpd4000_task(void *pArgument) {
  ADI_OSAL_STATUS err;
  uint16_t nDevId = 0;
  uint8_t i;

  for (i = 0; i < SLOT_NUM; i++) {
    post_office_add_mailbox(M2M2_ADDR_SENSOR_ADPD4000,
        (M2M2_ADDR_ENUM_t)(M2M2_ADDR_SENSOR_ADPD_STREAM1 + i));
  }
  post_office_add_mailbox(M2M2_ADDR_SENSOR_ADPD4000, M2M2_ADDR_SYS_STATIC_AGC_STREAM);
  ret_code_t err_code = sd_nvic_SetPriority(GPIOTE_IRQn, APP_IRQ_PRIORITY_LOW);
  APP_ERROR_CHECK(err_code);
  GPIO_IRQ_ADPD_Enable();

  /* Putting ADP5360 interrupt enable in task context.
     Since ADP5360 doesnt have a separate task, its put in adpd4000 task */
  ADP5360_gpio_interrupt_enable();

  /* Init the adpd buffer with a sample size of 4 bytes.
     This size will be automatically updated based on the slot mode. */
  adpd4000_buff_init(8);

  if (Adpd400xDrvOpenDriver() != 0) {
    Debug_Handler();
  }
  Adpd400xDrvDataReadyCallback(adpd_data_ready_cb);
#ifdef ENABLE_PPG_APP
  Adpd400xSetSlot_RegCB(AdpdClSelectSlot);
  Adpd400xSetMode_RegCB(AdpdClSetOperationMode);
  PpgAdpd400xLibGetSampleRate_RegCB(GetAdpdClOutputRate);
  PpgAdpd400xLibSetSampleRate_RegCB(SetAdpdClOutputRate);
  RegisterPpgCB(Adpd400xDrvSetOperationMode);
  RegisterMwPpgGetLCFGRegCB(GetAdpd400xLcfgStructure);
  RegisterMwPpgReadLCFGRegCB(ReadAdpd400xLcfgStructureRaw);
  RegisterMwPpgModifyLCFGRegCB(ModifyAdpd400xLcfgStructureRaw);
#endif

  /*Wait for FDS init to complete*/
  adi_osal_SemPend(adpd4000_task_evt_sem, ADI_OSAL_TIMEOUT_FOREVER);

  Adpd400xDrvRegRead(ADPD400x_REG_CHIP_ID, &nDevId);
  /* Check if its DVT2 chip */
  if (nDevId != 0xc0) {
    NRF_LOG_INFO("** Working with board having ADPD4100 **");
    dvt2 = 1;
  } else {
    NRF_LOG_INFO("** Working with board having ADPD4000 **");
    dvt2 = 0;
  }
  if (load_adpd4000_cfg(M2M2_SENSOR_ADPD4000_DEVICE_4000_G) !=
      ADPD4000_DCFG_STATUS_OK) {
    Debug_Handler();
  }
  Adpd400xDrvSetParameter(ADPD400x_WATERMARKING, 0, 4);
  reset_agc_flags();/**reset AGC flags **/

#ifdef ENABLE_PPG_APP
  //Initilaze the app_timer for sync buffering for UC HR
  app_sync_timer_init();
#endif

#ifdef USER0_CONFIG_APP
  adpd_timer_init();
#endif

  while (1) {
    m2m2_hdr_t *p_in_pkt = NULL;
    m2m2_hdr_t *p_out_pkt = NULL;
#ifdef ADPD_SEM_CORRUPTION_DEBUG
    gAdpdPendFlag = 1;
    if(pAdpdSemPtr != (uint8_t *)adpd4000_task_evt_sem)
	{
      gAdpdPendFlag = 0xFF;
      p_in_pkt =
          post_office_get(ADI_OSAL_TIMEOUT_NONE, APP_OS_CFG_ADPD4000_TASK_INDEX);
      if(p_in_pkt == NULL)
          fetch_adpd_data();
    }
    else
    {
    	adi_osal_SemPend(adpd4000_task_evt_sem, ADI_OSAL_TIMEOUT_FOREVER);
    	gAdpdPendFlag = 0;
	 	p_in_pkt =
        post_office_get(ADI_OSAL_TIMEOUT_NONE, APP_OS_CFG_ADPD4000_TASK_INDEX);
    }
#else
    adi_osal_SemPend(adpd4000_task_evt_sem, ADI_OSAL_TIMEOUT_FOREVER);
	p_in_pkt = post_office_get(ADI_OSAL_TIMEOUT_NONE, APP_OS_CFG_ADPD4000_TASK_INDEX);
#endif

    if (p_in_pkt == NULL) {
      /* No m2m2 messages to process, so fetch some data from the device. */
      /*_nStatus = */ fetch_adpd_data();
      /*send static AGC stream if agc state udpated */
      packetize_adpd_static_agc_settings();
    } else {
      /* We got an m2m2 message from the queue, process it. */
      PYLD_CST(p_in_pkt, _m2m2_app_common_cmd_t, p_in_cmd);

#ifdef ADPD_SEM_CORRUPTION_DEBUG
      if(p_in_cmd->command == M2M2_SENSOR_ADPD_COMMAND_LOAD_CFG_REQ){
        command_type=1;
      }
      else if(p_in_cmd->command == M2M2_SENSOR_COMMON_CMD_WRITE_REG_16_REQ){
        command_type=2;
      }
      else if(p_in_cmd->command == M2M2_SENSOR_ADPD_COMMAND_CLOCK_CAL_REQ){
        command_type=3;
      }
      else if(p_in_cmd->command == M2M2_SENSOR_ADPD_COMMAND_CLOCK_CAL_REQ){
        command_type=4;
      }
       else if(p_in_cmd->command == M2M2_SENSOR_ADPD_COMMAND_AGC_ON_OFF_REQ){
        command_type=5;
      }
      else if(p_in_cmd->command == M2M2_APP_COMMON_CMD_STREAM_START_REQ){
        command_type=5;
      }
      else if(p_in_cmd->command == M2M2_APP_COMMON_CMD_STREAM_STOP_REQ){
        stop_req_adpd_task += 1;
        command_type=7;
        NRF_LOG_INFO("command type = %d",command_type);
      }
#endif
      /* Look up the appropriate function to call in the function table */
      for (int i = 0; i < ADPD_APP_ROUTING_TBL_SZ; i++) {
        if (adpd4000_app_routing_table[i].command == p_in_cmd->command) {
          p_out_pkt = adpd4000_app_routing_table[i].cb_handler(p_in_pkt);
          break;
        }
      }
      post_office_consume_msg(p_in_pkt);
      if (p_out_pkt != NULL) {
        post_office_send(p_out_pkt, &err);
#ifdef ADPD_SEM_CORRUPTION_DEBUG
        if(p_in_cmd->command == M2M2_APP_COMMON_CMD_STREAM_STOP_REQ){
          stop_resp_sent_from_adpd_task +=1;
        }
#endif
      }
    }
  }
}

/*!
 ****************************************************************************
 * @brief Reset ADPD4k Packetization
 *
 * @param[in]      None
 *
 * @return         None
 *****************************************************************************/
static void reset_adpd_packetization(void) {
  uint8_t i;

  for (i = 0; i < SLOT_NUM; i++) {
    g_state.sl_pktizer1[i].packet_max_nsamples = MAX_PAYLOAD_BYTE;
    g_state.sl_pktizer1[i].sample_sz = 0;
    g_state.sl_pktizer1[i].packet_nsamples = 0;
    g_state.sl_pktizer2[i].packet_max_nsamples = MAX_PAYLOAD_BYTE;
    g_state.sl_pktizer2[i].sample_sz = 0;
    g_state.sl_pktizer2[i].packet_nsamples = 0;
    g_state.sl_pktizer1[i].decimation_nsamples = 0;
    g_state.sl_pktizer2[i].decimation_nsamples = 0;
  }
}

/*!
 ****************************************************************************
 * @brief Read ADPD4k data from buffer,
 *        -Send it to other apps (PPG, TEMP, SQI), if active.
 *        -packetize them and send it to postoffice, if ADPD4k app is started.
 *
 * @param[in]     None
 *
 * @return        0- Success, 1- Error
 *****************************************************************************/
#ifdef ADPD_SEM_CORRUPTION_DEBUG
 m2m2_hdr_t    *sempahore_add_channel1=NULL;
 m2m2_hdr_t    *sempahore_add_channel2=NULL;
 uint16_t dataPtr =0,eachSlotSize =0;
 uint32_t gn_sample_size_cp=0;
#endif
static uint8_t fetch_adpd_data(void) {
  ADI_OSAL_STATUS err;
  uint32_t timestamp = 0;
  uint16_t nSampleSize = 0;
  uint32_t ppgData[NUM_CH_PER_SLOT] = {0}, tmpPPGData[NUM_CH_PER_SLOT] = {0};
  uint32_t ch1ch2data[NUM_CH_PER_SLOT] = {0};
  uint16_t nWaterMark=1, nMaximumFifoTh = 0;
#ifdef ENABLE_PPG_APP
  uint8_t nModifyChannelData = 0;
  uint8_t nShiftFactor = 0;
  uint8_t nSkipCh2Packetization = 0;
  uint32_t nTargetCh,n_recal_time_in_min;
#endif
#ifdef ADPD_SEM_CORRUPTION_DEBUG
  uint8_t i, tmp[256];//, dataPtr;
  /* Maximum D=4,S=4 and 2 Channels enabled for all slots
     (need to set it for maximum data==>(4+4)*2*12) */
  uint16_t slot_sz[SLOT_NUM], impulse_Slot_Size;
#else
  uint8_t i, tmp[256], dataPtr;
  /* Maximum D=4,S=4 and 2 Channels enabled for all slots
     (need to set it for maximum data==>(4+4)*2*12) */
  uint16_t slot_sz[SLOT_NUM], eachSlotSize, impulse_Slot_Size;
#endif
  uint32_t len = sizeof(tmp);
  uint16_t highest_slot, ch_num[SLOT_NUM];
  CIRC_BUFF_STATUS_t status;
  static m2m2_sensor_adpd4000_data_stream_t adpd_ch1_pkt[SLOT_NUM] = {'\0'},
                                            adpd_ch2_pkt[SLOT_NUM] = {'\0'};

  if (gsOneTimeValueWhenReadAdpdData == 0) {
    gsOneTimeValueWhenReadAdpdData = 1;
    if(gbAdpdSamplingRateChanged)
    {
      gbAdpdSamplingRateChanged = 0;
      /* reset the counter for temp only if sampling rate is changed */
#ifdef CUST4_SM
  #ifndef ENABLE_DEBUG_STREAM
        gsTempSampleCount = (gsSlot[ADPD400xDrv_SLOTD].odr - 1);
  #else
        gsTempSampleCount = 0;
  #endif
#else
      gsTempSampleCount = 0;
#endif
    }
    Adpd400xDrvRegRead(ADPD400x_REG_FIFO_STATUS_BYTES, &nStatusRegValue);
    // Handle ADPD at 500Hz by increasing ADPD task priority
    if ((gAdpdSlotMaxODR >= 500) && (gbAdpdHighPrio == 0)) {
      adi_osal_ThreadSetPrio(NULL, ADPD_500Hz_TASK_PRIO);
      gbAdpdHighPrio = 1;
    }
  }

  if (nStatusRegValue != 0) {
    /* Clear the level interrupt status register */
    Adpd400xDrvRegWrite(ADPD400x_REG_INT_STATUS_LEV0, 0xFFFF);
    Adpd400xDrvRegWrite(ADPD400x_REG_INT_STATUS_LEV1, 0xFFFF);
  }

  memset(&tmp[0], 0x00, sizeof(tmp));
  /* read data from FIFO to buffer */
  if (adpd4000_read_data_to_buffer(&slot_sz[0], &highest_slot, &ch_num[0]) !=
      0) {
    /* Get all slot samples */

    status = adpd4000_buff_get(&tmp[0], &timestamp, &len);
  } else {
    /* Something went wrong with data interrupt */
    return 1;
  }

  while (status == CIRC_BUFF_STATUS_OK) {
    dataPtr = 0;
    /* Get the FIFO pattern for the given sequecne number  */
    get_current_datapattern(
        &nReadSequence, gsSlot, gnLcmValue, highest_slot, &sReadBufferPattern);

    for (i = 0; i <= highest_slot; i++) {
#ifdef ENABLE_PPG_APP
      nModifyChannelData = 0;
      nShiftFactor = 0;
      nSkipCh2Packetization = 0;
#endif
      /* not subscriber or inactive slot */
      if ((slot_sz[i] & 0x0100) == 0) { /* not Impulse mode */
        /* eachSlotSize = (slot_sz[i] & 0xF) + ((slot_sz[i] & 0xF0) >> 4); */
        /* Get the Lit, Dart & Signal size from 16-bit array element
         *---------*---------*--------*--------*
         * Lit     * Impulse * Dark   * Signal *
         *---------*---------*--------*--------*
         * [15:12] * [11:8]  * [7:4]  * [3:0]  *
         *---------*---------*--------*--------*
         */
        eachSlotSize = ((slot_sz[i] >> 12) & 0xF) + ((slot_sz[i] >> 4) & 0xF) +
                       (slot_sz[i] & 0xF);
      } else {
        eachSlotSize = 2;
        impulse_Slot_Size = slot_sz[i] & 0xff;
      }

      if (sReadBufferPattern.slot_info[i] == '0') {
        /* if the slot is skipped, increment the dataPtr
           with skipped slot size and iterate through next slot */
        dataPtr += eachSlotSize;
        continue;
      }

      if (gsSlot[i].activeSlot == 0) { /* Inactive slot */
        continue;
      }

#ifdef ENABLE_TEMPERATURE_APP
#ifdef EVTBOARD
#ifdef USER0_CONFIG_APP
      if(temp_app_timings.app_mode_on)
      {
#endif
      /* Check the slot is in Temperature mode */
      if(g_therm_slot_en_bit_mask != 0)
      {
        if(g_therm_slot_en_bit_mask & (1 << i))
        {
          temperatureAppData((uint32_t *)&tmp[dataPtr],i);
        }
      }
#ifdef USER0_CONFIG_APP
      } //if(temp_app_timings.app_mode_on)
#endif
#endif//EVTBOARD
#endif//ENABLE_TEMPERATURE_APP

#ifdef ENABLE_SQI_APP
#ifdef USER0_CONFIG_APP
      if(adpd_app_timings.app_mode_on)
      {
#endif
      /* Check if sqi event is started */
      if (gnSQI_Slot != 0) {
        if (gnSQI_Slot == (0x01 << i)) {
          if (sqi_event == 1 && ((g_state.num_subs[i] > 0)
#ifdef ENABLE_PPG_APP
          || (gnSQI_Slot == Ppg_Slot)) )
#else
          ))
#endif
          {
            /* Send only Ch1 data */
            memcpy(&tmpPPGData[CH1], &tmp[dataPtr], eachSlotSize);
            ppgData[CH1] = *(uint32_t *)tmpPPGData;
            send_sqi_app_data(&ppgData[CH1], timestamp);
          }
        }
      }
#ifdef USER0_CONFIG_APP
      } // if(adpd_app_timings.app_mode_on)
#endif
#endif//ENABLE_SQI_APP

#ifdef ENABLE_PPG_APP
      /* Check the slot is in PPG mode and agc calibration is done */
      if (Ppg_Slot != 0 && !gRun_agc) {
        if (Ppg_Slot == (0x01 << i)) {
          if(!gPpg_agc_en && !gPpg_agc_done){
             update_ppg_default_current_gain(i);// update lcfg current and gain if static agc not enabled for ppg
             gPpg_agc_done = 1;
           }
#ifdef SLOT_SELECT
          if (++gsSampleCount1 == gsSlot[i].odr / gAdpd400x_lcfg->hrmInputRate) {
            gsSampleCount1 = 0;
#endif
            memcpy(&tmpPPGData[CH1], &tmp[dataPtr], eachSlotSize);
            ppgData[CH1] = *(uint32_t *)tmpPPGData;
            ppgData[CH2] = 0;
            /* check if CH2 enabled */
            if ((ch_num[i] == 3) && ((gAdpd400xLibCfg.targetChs & BITM_TARGET_CH) != TARGET_CH3)) {
              /* Get CH2 data */
              memcpy(&tmpPPGData[CH2], &tmp[dataPtr + eachSlotSize], eachSlotSize);
              ppgData[CH2] = *(uint32_t *)(tmpPPGData + 1);
            }
            SyncAppDataSend(ppgData, timestamp, 0, 0);
#ifdef SLOT_SELECT
           }
#endif
         }//if (Ppg_Slot == (0x01 << i))
       }//if (Ppg_Slot != 0 && !gRun_agc)

#ifdef USER0_CONFIG_APP
    if(adpd_app_timings.app_mode_on)
    {
#endif
    /* Check if ppg app is not running & if its slot F data
       to calculate HR */
    if (gn_uc_hr_enable && Ppg_Slot == 0 && i==(gn_uc_hr_slot-1) && !gRun_agc) {
      MwPPG_ReadLCFG(2, &nTargetCh);
      if(((nTargetCh & BITM_TARGET_CH) == TARGET_CH4) || ((nTargetCh & BITM_TARGET_CH) == TARGET_CH3)
       || ((nTargetCh & BITM_TARGET_CH) == TARGET_CH1)) {

        if((nTargetCh & BITM_TARGET_CH) != TARGET_CH1) {// sum and shift factor option not available for TARGET_CH1
          nModifyChannelData = 1;
          nShiftFactor = (nTargetCh & BITM_CHANNEL_SHIFT_FACTOR) >> 4;
        }
        if((nTargetCh & BITM_DISABLE_CH2_PACKETIZATION) != 0) {// skip ch2 packet option available for TARGET_CH1/CH3/CH4
          nSkipCh2Packetization = 1;
        }
      }
      MwPPG_ReadLCFG(12, &n_recal_time_in_min);/*! To read staticAgcRecalTime */
      if((n_recal_time_in_min != 0) && (++gsResetStaticAgcSampleCnt >= (gsSlot[i].odr * n_recal_time_in_min * SECONDS_PER_MIN)))
      {
        gsResetStaticAgcSampleCnt = 0;
        /*Put the device into IDLE mode*/
        Adpd400xDrvSetOperationMode(ADPD400xDrv_MODE_IDLE);

        /*Reset flags related green LED*/
        gPpg_agc_done = 0;

        /*Set flags condition to trigger agc recalibration */
        gPpg_agc_en = 1;
        gn_led_slot_g = G_LED_AGC_SLOTS;
        gn_agc_active_slots = gn_led_slot_g;
        gRun_agc = true;

        /*Put the device back to sample mode*/
        Adpd400xDrvSetOperationMode(ADPD400xDrv_MODE_SAMPLE);
      }

      if(!gPpg_agc_en && !gPpg_agc_done){
        update_ppg_default_current_gain(i);// update lcfg current and gain if static agc not enabled for ppg
        gPpg_agc_done = 1;
      }
      if (++gnHRAdpdSampleCount == gsSlot[i].odr / gAdpd400xLibCfg.hrmInputRate) {
        gnHRAdpdSampleCount = 0;
        /* send CH1 data */
        memcpy(&tmpPPGData[CH1], &tmp[dataPtr], eachSlotSize);
        ppgData[CH1] = *(uint32_t *)tmpPPGData;
        ppgData[CH2] = 0;
        /* check if CH2 enabled */
        if ((ch_num[i] == 3) && ((gAdpd400xLibCfg.targetChs & BITM_TARGET_CH) != TARGET_CH3)) {
        /* Get CH2 data */
        memcpy(&tmpPPGData[CH2], &tmp[dataPtr + eachSlotSize], eachSlotSize);
        ppgData[CH2] = *(uint32_t *)(tmpPPGData + 1);
        }
        /* send data to ppg task */
        SyncAppDataSend(ppgData, timestamp, 0, 0);
       }
     }
#ifdef USER0_CONFIG_APP
    } //if(adpd_app_timings.app_mode_on)
#endif
#endif //ENABLE_PPG_APP

     if(gRun_agc){
#ifdef USER0_CONFIG_APP
      if(adpd_app_timings.app_mode_on)
      {
#endif
       if (gn_agc_active_slots != 0) {
         if (gn_agc_active_slots & (0x01 << i)) {
            int8_t slotnum = i;
            if (gsAgcFlag == 0) {
              agc_init();
              gsAgcFlag = 1;
            }
            if (gsAgcSampleCnt[slotnum] < SKIP_SAMPLES) {
              gsAgcSampleCnt[slotnum]++;
              /* waiting for agc buffer to fill */
            } else if (gsAgcSampleCnt[slotnum] < SKIP_SAMPLES + SAMPLE_AVG_NUM) {
              /* Get CH1 data */
              memcpy(&tmpPPGData[CH1], &tmp[dataPtr], eachSlotSize);
              ppgData[CH1] = *(uint32_t *)tmpPPGData;
              agc_data[slotnum].ch1[gsAgcSampleCnt[slotnum] - SKIP_SAMPLES] = ppgData[CH1];
              /* check if CH2 enabled */
#ifdef ENABLE_PPG_APP
              /* if this slot is for green LED and UC HR enabled or if it is a PPG slot */
              if(((slotnum == (gn_uc_hr_slot - 1)) && gn_uc_hr_enable) || (Ppg_Slot == (1 << slotnum))) {
                ppgData[CH2] = 0;
                if (ch_num[i] == 3 && ((gAdpd400xLibCfg.targetChs & BITM_TARGET_CH) != TARGET_CH3)){
                  /* Get CH2 data */
                  memcpy(&tmpPPGData[CH2], &tmp[dataPtr + eachSlotSize], eachSlotSize);
                  ppgData[CH2] = *(uint32_t *)(tmpPPGData + 1);
                }
                agc_data[slotnum].ch2[gsAgcSampleCnt[slotnum] - SKIP_SAMPLES] = ppgData[CH2];
              }else{
#endif
                  ppgData[CH2] = 0;
                  if (ch_num[i] == 3) {
                  /* Get CH2 data */
                  memcpy(&tmpPPGData[CH2], &tmp[dataPtr + eachSlotSize], eachSlotSize);
                  ppgData[CH2] = *(uint32_t *)(tmpPPGData + 1);
                }
                agc_data[slotnum].ch2[gsAgcSampleCnt[slotnum] - SKIP_SAMPLES] = ppgData[CH2];
#ifdef ENABLE_PPG_APP
              }
#endif
              gsAgcSampleCnt[slotnum]++;
            } else if (gsAgcSampleCnt[slotnum] >=  SKIP_SAMPLES + SAMPLE_AVG_NUM) {
                agc_data_process();// do agc process for all the active slots
                for(uint8_t slot = 0; slot <= highest_slot ; slot++){
                  if (gn_agc_active_slots & (0x01 << slot)){
                  gsAgcSampleCnt[slot] = 0;
                  }
                }
#ifdef ENABLE_PPG_APP
                if(gPpg_agc_en)//if(gn_agc_active_slots & Ppg_slot)
                {
                  gPpg_agc_done = 1;//Notify HRM library that static agc process done
                }
#endif
                /* Capture AGC settings for AGC stream*/
                g_state_static_agc.active_slots = gn_agc_active_slots;
                /* If UCHR and PPG is not enabled , update AGC state for static AGC stream*/
                if(g_state_static_agc.agc_log_state == ADPD400x_AGCLOG_STATIC_AGC_INVALID) {
                  g_state_static_agc.agc_log_state = ADPD400xLIB_AGCLOG_STATIC_AGC_FIRST_CAL;
                }else {
                  g_state_static_agc.agc_log_state = ADPD400xLIB_AGCLOG_STATIC_AGC_RECAL;
                }
                agc_deinit();
                gsAgcFlag = 0;
                gRun_agc = false;
            }
         }//if (gn_agc_active_slots & (0x01 << i))
      }//if (gn_agc_active_slots != 0)
#ifdef USER0_CONFIG_APP
    } //if(adpd_app_timings.app_mode_on)
#endif
    }//gRun_agc

      if (eachSlotSize == 0) {
        /* update dataptr in tmp buffer, ptr to next slot */
        dataPtr += eachSlotSize;
        /* Check the channel 2 status,
           if channel 2 enabled then do update dataptr */
        if (ch_num[i] == 3) {
          /* update dataptr in tmp buffer, ptr to next slot */
          dataPtr += eachSlotSize;
        }
        continue;
      }

      /*
        considering no decimation,
        if odr < 25Hz, no. of samples = 1
        if odr < 75Hz, no. of samples = 2
        else, no. of samples = 6
      */
      g_state.sl_pktizer1[i].packet_nsamples++;
      if (g_state.sl_pktizer1[i].packet_nsamples == 1) { /* first sample */
        g_state.sl_pktizer1[i].packet_max_nsamples = 24 / eachSlotSize;
        if (g_state.sl_pktizer1[i].packet_max_nsamples > 6) {
          g_state.sl_pktizer1[i].packet_max_nsamples = 6;
        }

        if (gsSlot[i].odr < 25) {
          g_state.sl_pktizer1[i].packet_max_nsamples = 1;
        } else if (gsSlot[i].odr < 75) {
          if (g_state.sl_pktizer1[i].packet_max_nsamples > 2)
            g_state.sl_pktizer1[i].packet_max_nsamples = 2;
        }

        if ((slot_sz[i] & 0x0100) == 0) {
          g_state.sl_pktizer1[i].payload_ptr = &(adpd_ch1_pkt[i].adpddata[0]);
        } else {
          g_state.sl_pktizer1[i].packet_max_nsamples =
              (256) / (eachSlotSize * impulse_Slot_Size);
          g_state.sl_pktizer1[i].payload_ptr = &(adpd_ch1_pkt[i].adpddata[0]);
        }
      }
      /* in future, data in circular buffer should have a byte to indicate the
         size, slot, channel etc. */
      if ((slot_sz[i] & 0x0100) == 0) {
#ifdef ENABLE_PPG_APP
        if (nModifyChannelData) {
            ch1ch2data[CH1] = *(uint32_t*)&tmp[dataPtr];
            ch1ch2data[CH2] = 0;
            if (ch_num[i] == 3) {
             ch1ch2data[CH2] = *(uint32_t*)&tmp[dataPtr + eachSlotSize];
            }
            if((nTargetCh & BITM_TARGET_CH) == TARGET_CH4){
              ch1ch2data[CH1] = (ch1ch2data[CH1] >> nShiftFactor) + (ch1ch2data[CH2] >> nShiftFactor); //shift,sum ch1-ch2 and store to ch1 packetizer
            }else{
              ch1ch2data[CH1] = (ch1ch2data[CH1] >> nShiftFactor);// shift only the ch1 as PD1+PD2 connected to ch1 and store to ch1 packetizer
            }
            memcpy(g_state.sl_pktizer1[i].payload_ptr, &ch1ch2data[CH1], eachSlotSize);
         }else{
#endif
            memcpy(g_state.sl_pktizer1[i].payload_ptr,  &tmp[dataPtr], eachSlotSize);
#ifdef ENABLE_PPG_APP
         }
#endif
        /* update adpddata ptr */
        g_state.sl_pktizer1[i].payload_ptr += eachSlotSize;
        /* update dataptr in tmp buffer, ptr to next channel */
        dataPtr += eachSlotSize;
      } else {
        memcpy(g_state.sl_pktizer1[i].payload_ptr, &tmp[dataPtr],
            eachSlotSize * impulse_Slot_Size);
        /* update adpddata ptr */
        g_state.sl_pktizer1[i].payload_ptr +=
            (eachSlotSize * impulse_Slot_Size);
        /* update dataptr in tmp buffer, ptr to next channel */
        dataPtr += (eachSlotSize * impulse_Slot_Size);
      }

      g_state.sl_pktizer1[i].decimation_nsamples++;
      if ((g_state.sl_pktizer1[i].packet_nsamples >=
          g_state.sl_pktizer1[i].packet_max_nsamples) && (!gAdpdPause)) {
        /* PO pkt creation and sending is put under critical region, to avoid
           task switching during this phase, hence avoid pkt corruption */
        if (g_state.sl_pktizer1[i].decimation_nsamples >= (g_state.decimation_factor*g_state.sl_pktizer1[i].packet_max_nsamples)) {
            g_state.sl_pktizer1[i].decimation_nsamples = 0;
            g_state.sl_pktizer1[i].packet_nsamples = 0;
          if(g_state.num_subs[i] > 0) {
            adi_osal_EnterCriticalRegion();
            if ((slot_sz[i] & 0x0100) == 0) {
              g_state.sl_pktizer1[i].p_pkt = post_office_create_msg(sizeof(m2m2_sensor_adpd4000_data_stream_t) + M2M2_HEADER_SZ);
            } else {
              g_state.sl_pktizer1[i].p_pkt = post_office_create_msg(sizeof(m2m2_sensor_adpd4000_impulse_stream_t) + M2M2_HEADER_SZ);
            }
            if (g_state.sl_pktizer1[i].p_pkt != NULL) {
    #ifdef ADPD_SEM_CORRUPTION_DEBUG
              sempahore_add_channel1 = g_state.sl_pktizer1[i].p_pkt;
    #endif
              PYLD_CST(g_state.sl_pktizer1[i].p_pkt,
                  m2m2_sensor_adpd4000_data_stream_t, ptr_payload1);
              g_state.sl_pktizer1[i].p_pkt->src = M2M2_ADDR_SENSOR_ADPD4000;
              g_state.sl_pktizer1[i].p_pkt->dest = gsStream[i];
              memcpy(&ptr_payload1->command, &adpd_ch1_pkt[i],
                  sizeof(m2m2_sensor_adpd4000_data_stream_t));
              ptr_payload1->command =
                  (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_SENSOR_COMMON_CMD_STREAM_DATA;
              ptr_payload1->status =
                  (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_APP_COMMON_STATUS_OK;
              ptr_payload1->sequence_num = g_state.data_pkt_seq_num[i][CH1]++;
              ptr_payload1->data_format = slot_sz[i];
              ptr_payload1->timestamp = timestamp;
              ptr_payload1->sample_num = g_state.sl_pktizer1[i].packet_max_nsamples;
              ptr_payload1->channel_num = 1;
    #ifdef DEBUG_PKT
              post_office_msg_cnt(g_state.sl_pktizer1[i].p_pkt);
    #endif
#ifdef USER0_CONFIG_APP
            if(adpd_app_timings.app_mode_on)
            {
#endif //USER0_CONFIG_APP
              post_office_send(g_state.sl_pktizer1[i].p_pkt, &err);
#ifdef USER0_CONFIG_APP
            }//if(adpd_app_timings.app_mode_on)
            else
              post_office_consume_msg(g_state.sl_pktizer1[i].p_pkt);
#endif //USER0_CONFIG_APP
            }
            adi_osal_ExitCriticalRegion();
          }//if (g_state.num_subs[i] > 0)
        g_state.sl_pktizer1[i].p_pkt = NULL;
        }//if (g_state.decimation_nsamples_ch1 >= (g_state.decimation_factor*g_state.sl_pktizer1[i].packet_max_nsamples))
      }
      // NRF_LOG_INFO("ADPD Time stamp=%d",timestamp);

      if (ch_num[i] != 3)
        continue;
//nSkipCh2Packetization will be 1 ,if ch2 packet has to be skipped for targetch 4 and 3 option in LCFG
#ifdef ENABLE_PPG_APP
        if(!nSkipCh2Packetization) {
#endif
        g_state.sl_pktizer2[i].packet_nsamples++;
        if (g_state.sl_pktizer2[i].packet_nsamples == 1) { /* first sample */
          g_state.sl_pktizer2[i].packet_max_nsamples =
              g_state.sl_pktizer1[i].packet_max_nsamples;

          g_state.sl_pktizer2[i].payload_ptr = &(adpd_ch2_pkt[i].adpddata[0]);
        }

        memcpy(g_state.sl_pktizer2[i].payload_ptr, &tmp[dataPtr], eachSlotSize);
        g_state.sl_pktizer2[i].payload_ptr += eachSlotSize;
        /* update dataptr in tmp buffer. Ptr to next slot */
        dataPtr += eachSlotSize;
        g_state.sl_pktizer2[i].decimation_nsamples++;
        if ((g_state.sl_pktizer2[i].packet_nsamples >=
            g_state.sl_pktizer2[i].packet_max_nsamples)&&(!gAdpdPause)) {
          if (g_state.sl_pktizer2[i].decimation_nsamples >= (g_state.decimation_factor*g_state.sl_pktizer2[i].packet_max_nsamples)) {
            g_state.sl_pktizer2[i].decimation_nsamples = 0;
            g_state.sl_pktizer2[i].packet_nsamples = 0;
            if(g_state.num_subs[i] > 0) {
              adi_osal_EnterCriticalRegion();
              g_state.sl_pktizer2[i].p_pkt = post_office_create_msg(sizeof(m2m2_sensor_adpd4000_data_stream_t) +M2M2_HEADER_SZ); // TODO: review the changes to :check if p_pkt is
                                   // not NULL before writing
                                 //  g_state.sl_pktizer2[i].p_pkt=NULL;
              if (g_state.sl_pktizer2[i].p_pkt != NULL) {
      #ifdef ADPD_SEM_CORRUPTION_DEBUG
                sempahore_add_channel2 = g_state.sl_pktizer2[i].p_pkt;
      #endif
                PYLD_CST(g_state.sl_pktizer2[i].p_pkt,m2m2_sensor_adpd4000_data_stream_t, ptr_payload2);
                g_state.sl_pktizer2[i].p_pkt->src = M2M2_ADDR_SENSOR_ADPD4000;
                g_state.sl_pktizer2[i].p_pkt->dest = gsStream[i];
                memcpy(&ptr_payload2->command, &adpd_ch2_pkt[i],
                    sizeof(m2m2_sensor_adpd4000_data_stream_t));
                ptr_payload2->command =(M2M2_APP_COMMON_CMD_ENUM_t)M2M2_SENSOR_COMMON_CMD_STREAM_DATA;
                ptr_payload2->status =(M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_APP_COMMON_STATUS_OK;
                ptr_payload2->sequence_num = g_state.data_pkt_seq_num[i][CH2]++;
                ptr_payload2->data_format = slot_sz[i];
                ptr_payload2->timestamp = timestamp;
                ptr_payload2->sample_num = g_state.sl_pktizer2[i].packet_max_nsamples;
                ptr_payload2->channel_num = 2;
      #ifdef DEBUG_PKT
                post_office_msg_cnt(g_state.sl_pktizer2[i].p_pkt);
      #endif
#ifdef USER0_CONFIG_APP
                if(adpd_app_timings.app_mode_on)
                {
#endif //USER0_CONFIG_APP
                  post_office_send(g_state.sl_pktizer2[i].p_pkt, &err);
#ifdef USER0_CONFIG_APP
                }//if(adpd_app_timings.app_mode_on)
                else
                  post_office_consume_msg(g_state.sl_pktizer2[i].p_pkt);
#endif //USER0_CONFIG_APP
              }
              adi_osal_ExitCriticalRegion();
            }//g_state.num_subs[i] > 0
          g_state.sl_pktizer2[i].p_pkt = NULL;
          }//if (g_state.decimation_nsamples_ch2 >= (g_state.decimation_factor*g_state.sl_pktizer2[i].packet_max_nsamples))
        }
//Enable this code, if ch2 packet has to be skipped for targetch 4 option in LCFG
#ifdef ENABLE_PPG_APP
        }//if(nSkipCh2Packetization)
#endif
    }

    if (sReadBufferPattern.slot_info[1] != '0') {
      /* stream the Optional bytes data */
      if (bIsStatusStreamEnabled && (nStatusRegValue != 0)) {
        pOptionalPkt = post_office_create_msg(
            sizeof(m2m2_sensor_fifo_status_bytes_t) + M2M2_HEADER_SZ);
        if (pOptionalPkt != NULL) {
          pOptionalPkt->src = M2M2_ADDR_SENSOR_ADPD4000;
          pOptionalPkt->dest = M2M2_ADDR_SENSOR_ADPD_OPTIONAL_BYTES_STREAM;
          if (_OptionalByteArrange(
                  &tmp[dataPtr], nStatusRegValue, timestamp, pOptionalPkt)) {
            post_office_send(pOptionalPkt, &err);
          } else {
            post_office_consume_msg(pOptionalPkt);
          }
          pOptionalPkt = NULL;
        }
      }
    }
    status = adpd4000_buff_get(&tmp[0], &timestamp, &len);
  }
  nInterruptSequence = nReadSequence;
#ifdef ENABLE_DEBUG_STREAM
  g_adpdOffset = 0;
  packetize_adpd_debug_data();
#endif
  /* set FIFO threshold value for 4 sets */
  nSampleSize = get_samples_size(
      gnAdpdFifoWaterMark, gsSlot, &nInterruptSequence, gnLcmValue, highest_slot);
  if (nSampleSize != 0) {
    nSampleSize += nFifoStatusByte;
    nMaximumFifoTh = (!dvt2) ? 0xFF: 0x1FF;
    Adpd400xDrvRegWrite(ADPD400x_REG_FIFO_CTL, (nSampleSize - 1) & nMaximumFifoTh);
  }
  return 0;
}

/*!
 ****************************************************************************
 * @brief Get ADPD4k DCFG
 *
 * @param[in]           p_pkt: input m2m2 packet
 *
 * @return              pointer to reponse m2m2 packet
 *****************************************************************************/
static m2m2_hdr_t *adpd_app_get_dcfg(m2m2_hdr_t *p_pkt) {

    uint16_t dcfg_size;
    uint16_t i = 0, num_pkts = 0, dcfg_array_index = 0;
    uint32_t  dcfgdata[MAXADPD4000DCBSIZE*MAX_ADPD4000_DCB_PKTS];
    ADI_OSAL_STATUS  err;
    M2M2_APP_COMMON_STATUS_ENUM_t status = M2M2_APP_COMMON_STATUS_ERROR;

  /* Declare and malloc a response packet */
  PKT_MALLOC(p_resp_pkt, m2m2_sensor_dcfg_data_t, 0);

  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_sensor_dcfg_data_t, p_resp_payload);
    memset(dcfgdata, 0,sizeof(dcfgdata));
    memset(p_resp_payload->dcfgdata, 0, sizeof(p_resp_payload->dcfgdata));
    if (ADPD4000_DCFG_STATUS_OK == read_adpd4000_dcfg(&dcfgdata[0], &dcfg_size)) { // dcfg size will give number of registers
      status = M2M2_APP_COMMON_STATUS_OK;
      num_pkts = (dcfg_size / MAXADPD4000DCBSIZE) + ((dcfg_size % MAXADPD4000DCBSIZE) ? 1 : 0 );
      dcfg_array_index = 0;
      for(uint16_t p = 0; p < num_pkts; p++) {
         p_resp_payload->size = \
              (p != num_pkts-1) ? MAXADPD4000DCBSIZE : (dcfg_size % MAXADPD4000DCBSIZE);
         p_resp_payload->num_tx_pkts = num_pkts;
         for (i = 0; i < p_resp_payload->size; i++) {
            p_resp_payload->dcfgdata[i] = dcfgdata[dcfg_array_index++];
         }
         if(p != num_pkts-1) {
           p_resp_pkt->src = p_pkt->dest;
           p_resp_pkt->dest = p_pkt->src;
           p_resp_payload->status = status;
           p_resp_payload->command = M2M2_SENSOR_COMMON_CMD_GET_DCFG_RESP;
           post_office_send(p_resp_pkt, &err);

           /* Delay is kept same as what is there in low touch task */
           MCU_HAL_Delay(60);

           PKT_MALLOC(p_resp_pkt, m2m2_sensor_dcfg_data_t, 0);
           if(NULL != p_resp_pkt) {
             // Declare a pointer to the response packet payload
             PYLD_CST(p_resp_pkt, m2m2_sensor_dcfg_data_t, p_resp_payload);
             memset(p_resp_payload->dcfgdata, 0, sizeof(p_resp_payload->dcfgdata));

           }//if(NULL != p_resp_pkt)
           else {
             return NULL;
           }
         }
       }
      }else {
        p_resp_payload->size = 0;
        p_resp_payload->num_tx_pkts = 0;
        status = M2M2_APP_COMMON_STATUS_ERROR;
      }//if(read_adpd4000_dcfg())
    p_resp_payload->status = status;
    p_resp_payload->command = M2M2_SENSOR_COMMON_CMD_GET_DCFG_RESP;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  }//if(NULL != p_resp_pkt)
  return p_resp_pkt;
}

/*!
 ****************************************************************************
 * @brief Get ADPD4k app version
 *
 * @param[in]           p_pkt: input m2m2 packet
 *
 * @return              pointer to reponse m2m2 packet
 *****************************************************************************/
static m2m2_hdr_t *adpd_app_get_version(m2m2_hdr_t *p_pkt) {
  PKT_MALLOC(p_resp_pkt, m2m2_app_common_version_t, 0);

  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_app_common_version_t, p_resp_payload);

    p_resp_payload->command = M2M2_APP_COMMON_CMD_GET_VERSION_RESP;
    p_resp_payload->status = M2M2_APP_COMMON_STATUS_OK;
    p_resp_payload->major = 0x00;
    p_resp_payload->minor = 0x03;
    p_resp_payload->patch = 0x01;
    memcpy(&p_resp_payload->verstr[0], "ADPD_App", 9);
    memcpy(&p_resp_payload->str[0], &GIT_ADPD4000_VERSION,
        GIT_ADPD4000_VERSION_LEN);
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
    p_resp_pkt->checksum = 0x0000;
  }
  return p_resp_pkt;
}

/*!
 ****************************************************************************
 * @brief Set/Get ADPD4k Slot parameters
 *
 * @param[in]           p_pkt: input m2m2 packet
 *
 * @return              pointer to reponse m2m2 packet
 *****************************************************************************/
static m2m2_hdr_t *adpd_app_slotmode(m2m2_hdr_t *p_pkt) {
  M2M2_APP_COMMON_STATUS_ENUM_t status = M2M2_APP_COMMON_STATUS_ERROR;
  uint16_t slotFormat, channelNum, highestSelectedSlot;

  /* Declare a pointer to access the input packet payload */
  PYLD_CST(p_pkt, m2m2_sensor_adpd4000_slot_resp_t, p_in_payload);
  /* Declare and malloc a response packet */
  PKT_MALLOC(p_resp_pkt, m2m2_sensor_adpd4000_slot_resp_t, 0);
  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_sensor_adpd4000_slot_resp_t, p_resp_payload);

    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;

    switch (p_in_payload->command) {
    case M2M2_SENSOR_ADPD_COMMAND_SET_SLOT_REQ:

      if (p_in_payload->slot_enable == 0) {
        slotFormat = 0;
      } else {
        slotFormat = p_in_payload->slot_format;
      }
      p_resp_payload->command =
          (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_SENSOR_ADPD_COMMAND_SET_SLOT_RESP;
      if (p_in_payload->slot_num == 0 || p_in_payload->slot_num > SLOT_NUM) {
        p_resp_payload->status = status;
        return p_resp_pkt;
      }

      if (p_in_payload->channel_num == 1)
        channelNum = 1;
      else
        channelNum = 3; /* anything else sets to 3 */
      if (Adpd400xDrvSlotSetup(p_in_payload->slot_num - 1,
              p_in_payload->slot_enable, p_in_payload->slot_format,
              channelNum) != ADPD400xDrv_SUCCESS)
        break;
      p_resp_payload->slot_num = p_in_payload->slot_num;
      p_resp_payload->slot_enable = p_in_payload->slot_enable;
      p_resp_payload->slot_format = p_in_payload->slot_format;
      p_resp_payload->channel_num = p_in_payload->channel_num;
      status = M2M2_APP_COMMON_STATUS_OK;
      reset_adpd_packetization();
      break;

    case M2M2_SENSOR_ADPD_COMMAND_GET_SLOT_REQ:
      p_resp_payload->command =
          (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_SENSOR_ADPD_COMMAND_GET_SLOT_RESP;

      if (p_in_payload->slot_num == 0 || p_in_payload->slot_num > SLOT_NUM) {
        p_resp_payload->status = status;
        return p_resp_pkt;
      }
      if (Adpd400xDrvGetParameter(ADPD400x_LATEST_SLOT_DATASIZE,
              p_in_payload->slot_num - 1, &slotFormat) != ADPD400xDrv_SUCCESS)
        break;
      if (Adpd400xDrvGetParameter(ADPD400x_THIS_SLOT_CHANNEL_NUM,
              p_in_payload->slot_num - 1, &channelNum) != ADPD400xDrv_SUCCESS)
        break;
      if (Adpd400xDrvGetParameter(ADPD400x_HIGHEST_SLOT_NUM,
              p_in_payload->slot_num - 1,
              &highestSelectedSlot) != ADPD400xDrv_SUCCESS)
        break;
      if ((p_in_payload->slot_num - 1) <= highestSelectedSlot)
        p_resp_payload->slot_enable = 1;
      else
        p_resp_payload->slot_enable = 0;
      status = M2M2_APP_COMMON_STATUS_OK;
      p_resp_payload->slot_num = p_in_payload->slot_num;
      p_resp_payload->slot_format = slotFormat;
      p_resp_payload->channel_num = (uint8_t)channelNum;

      break;
    }
    p_resp_payload->status = status;
  }
  return p_resp_pkt;
}

/*!
 ****************************************************************************
 * @brief Set/Get ADPD4k Active Slot
 *
 * @param[in]           p_pkt: input m2m2 packet
 *
 * @return              pointer to reponse m2m2 packet
 *****************************************************************************/
static m2m2_hdr_t *adpd_app_slot_active_mode(m2m2_hdr_t *p_pkt) {
  M2M2_APP_COMMON_STATUS_ENUM_t status = M2M2_APP_COMMON_STATUS_ERROR;
  uint16_t slotActive;

  /* Declare a pointer to access the input packet payload */
  PYLD_CST(p_pkt, m2m2_sensor_adpd4000_slot_active_resp_t, p_in_payload);
  /* Declare and malloc a response packet */
  PKT_MALLOC(p_resp_pkt, m2m2_sensor_adpd4000_slot_active_resp_t, 0);
  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(
        p_resp_pkt, m2m2_sensor_adpd4000_slot_active_resp_t, p_resp_payload);

    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;

    switch (p_in_payload->command) {
    case M2M2_SENSOR_ADPD_COMMAND_SET_SLOT_ACTIVE_REQ:
      p_resp_payload->command = (M2M2_APP_COMMON_CMD_ENUM_t)
          M2M2_SENSOR_ADPD_COMMAND_SET_SLOT_ACTIVE_RESP;
      if (p_in_payload->slot_num == 0 || p_in_payload->slot_num > SLOT_NUM) {
        p_resp_payload->status = status;
        return p_resp_pkt;
      }
      if (Adpd400xDrvSlotSetActive(p_in_payload->slot_num - 1,
              p_in_payload->slot_active) != ADPD400xDrv_SUCCESS)
        break;
      p_resp_payload->slot_num = p_in_payload->slot_num;
      p_resp_payload->slot_active = p_in_payload->slot_active;
      status = M2M2_APP_COMMON_STATUS_OK;
      reset_adpd_packetization();
      break;
    case M2M2_SENSOR_ADPD_COMMAND_GET_SLOT_ACTIVE_REQ:
      p_resp_payload->command = (M2M2_APP_COMMON_CMD_ENUM_t)
          M2M2_SENSOR_ADPD_COMMAND_GET_SLOT_ACTIVE_RESP;
      if (p_in_payload->slot_num == 0 || p_in_payload->slot_num > SLOT_NUM) {
        p_resp_payload->status = status;
        return p_resp_pkt;
      }
      if (Adpd400xDrvGetParameter(ADPD400x_IS_SLOT_ACTIVE,
              p_in_payload->slot_num - 1, &slotActive) != ADPD400xDrv_SUCCESS)
        break;
      status = M2M2_APP_COMMON_STATUS_OK;
      p_resp_payload->slot_num = p_in_payload->slot_num;
      p_resp_payload->slot_active = slotActive;

      break;
    }

    p_resp_payload->status = status;
  }
  return p_resp_pkt;
}

/*!
 ****************************************************************************
 * @brief Do ADPD4k Clock Calibration
 *
 * @param[in]           p_pkt: input m2m2 packet
 *
 * @return              pointer to reponse m2m2 packet
 *****************************************************************************/
#ifdef ADPD_SEM_CORRUPTION_DEBUG
uint32_t clock_cal=0;
#endif
static m2m2_hdr_t *adpd_app_do_clock_cal(m2m2_hdr_t *p_pkt) {
  m2m2_sensor_clockcal_resp_t *clockcal =
      (m2m2_sensor_clockcal_resp_t *)&p_pkt->data[0];
  /* Declare and malloc a response packet */
  PKT_MALLOC(p_resp_pkt, m2m2_sensor_clockcal_resp_t, 0);
  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_sensor_clockcal_resp_t, p_resp_payload);
    p_resp_payload->status = M2M2_APP_COMMON_STATUS_ERROR;

    if (g_state.num_starts == 0) {
      if (Adpd4000DoClockCalibration(clockcal->clockcalid) == 0) {
#ifdef ADPD_SEM_CORRUPTION_DEBUG
        clock_cal += 1;
#endif
        p_resp_payload->status = M2M2_APP_COMMON_STATUS_OK;
      } else {
        p_resp_payload->status = M2M2_APP_COMMON_STATUS_ERROR;
      }
    }
    Adpd400xDrvDataReadyCallback(adpd_data_ready_cb);
    p_resp_payload->clockcalid = clockcal->clockcalid;
    p_resp_payload->command = M2M2_SENSOR_ADPD_COMMAND_CLOCK_CAL_RESP;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  }
  return p_resp_pkt;
}

/*!
 ****************************************************************************
 * @brief Get ADPD4k communication mode
 *
 * @param[in]           p_pkt: input m2m2 packet
 *
 * @return              pointer to reponse m2m2 packet
 *****************************************************************************/
static m2m2_hdr_t *adpd_app_get_com_mode(m2m2_hdr_t *p_pkt) {
  uint16_t nDevId;
  /* Declare and malloc a response packet */
  PKT_MALLOC(p_resp_pkt, m2m2_sensor_com_mode_resp_t, 0);
  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_sensor_com_mode_resp_t, p_resp_payload);
    Adpd400xDrvRegRead(ADPD400x_REG_CHIP_ID, &nDevId);
    if ((nDevId & 0xC0) == 0xC0) {
      p_resp_payload->com_mode = Adpd400xDrvGetComMode() + 1;
    } else {
      p_resp_payload->com_mode = 0x0;
    }
    p_resp_payload->status = M2M2_APP_COMMON_STATUS_OK;
    p_resp_payload->command = M2M2_SENSOR_ADPD_COMMUNICATION_MODE_RESP;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  }
  return p_resp_pkt;
}

/*!
 ****************************************************************************
 * @brief Load ADPD4k DCFG
 *
 * @param[in]           p_pkt: input m2m2 packet
 *
 * @return              pointer to reponse m2m2 packet
 *****************************************************************************/
#ifdef ADPD_SEM_CORRUPTION_DEBUG
 uint32_t load_adpd_cfg=0;
#endif
static m2m2_hdr_t *adpd_app_load_cfg(m2m2_hdr_t *p_pkt) {
  /* Declare a pointer to access the input packet payload */
  PYLD_CST(p_pkt, m2m2_sensor_adpd_resp_t, p_in_payload);
  /* Declare and malloc a response packet */
  PKT_MALLOC(p_resp_pkt, m2m2_sensor_adpd_resp_t, 0);

  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_sensor_adpd_resp_t, p_resp_payload);
    p_resp_payload->status = M2M2_APP_COMMON_STATUS_ERROR;
    uint16_t device_id = p_in_payload->deviceid;
    if (g_state.num_starts == 0) {
      if (!load_adpd4000_cfg(device_id)) { /* Loads the device configuration */
#ifdef ADPD_SEM_CORRUPTION_DEBUG
        load_adpd_cfg += 1;
#endif
        p_resp_payload->status = M2M2_APP_COMMON_STATUS_OK;
        external_load_dcfg = true;
      } else {
        p_resp_payload->status = M2M2_APP_COMMON_STATUS_ERROR;
      }
    }
    p_resp_payload->deviceid = device_id;
    p_resp_payload->command = M2M2_SENSOR_ADPD_COMMAND_LOAD_CFG_RESP;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  }
  return p_resp_pkt;
}

/*!
 ****************************************************************************
 * @brief Get ADPD4k running/subscription status
 *
 * @param[in]           p_pkt: input m2m2 packet
 *
 * @return              pointer to reponse m2m2 packet
 *****************************************************************************/
static m2m2_hdr_t *adpd_app_status(m2m2_hdr_t *p_pkt) {
  /* Declare a pointer to the response packet payload */
  PYLD_CST(p_pkt, m2m2_app_common_status_t, p_in_payload);
  /* Declare and malloc a response packet */
  PKT_MALLOC(p_resp_pkt, m2m2_app_common_status_t, 0);
  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_app_common_status_t, p_resp_payload);
    uint8_t i=0;

    p_resp_payload->command = M2M2_APP_COMMON_CMD_SENSOR_STATUS_QUERY_RESP;
    p_resp_payload->status = M2M2_APP_COMMON_STATUS_ERROR;

    if (g_state.num_starts == 0) {
      p_resp_payload->status = M2M2_APP_COMMON_STATUS_STREAM_STOPPED;
    } else {
      p_resp_payload->status = M2M2_APP_COMMON_STATUS_STREAM_STARTED;
    }

    //Get the index of the slot
    i = p_in_payload->stream - M2M2_ADDR_SENSOR_ADPD_STREAM1;

    p_resp_payload->stream = p_in_payload->stream;

    p_resp_payload->num_subscribers = g_state.num_subs[i];
    p_resp_payload->num_start_reqs = g_state.num_starts;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  }
  return p_resp_pkt;
}

/*!
  ****************************************************************************
* @brief Interface for m2m2 cmd request and response to start/stop ADPD4k stream
         and add/remove Subscription to it
*
* @param[in]           p_pkt: input m2m2 packet
*
* @return              pointer to reponse m2m2 packet
*****************************************************************************/
static m2m2_hdr_t *adpd_app_stream_config(m2m2_hdr_t *p_pkt) {
  M2M2_APP_COMMON_STATUS_ENUM_t status = M2M2_APP_COMMON_STATUS_ERROR;
  M2M2_APP_COMMON_CMD_ENUM_t command;
  uint8_t sl_num;
  uint8_t clockcalid;
  /*Keeping nRetCode as success by default, to handle else part
    of if(!adpd_app_timings.delayed_start) */
  int16_t nRetCode = ADPD400xDrv_SUCCESS;

  /* Declare a pointer to access the input packet payload */
  PYLD_CST(p_pkt, m2m2_app_common_sub_op_t, p_in_payload);
  /* Declare and malloc a response packet */
  PKT_MALLOC(p_resp_pkt, m2m2_app_common_sub_op_t, 0);
  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_app_common_sub_op_t, p_resp_payload);

    switch (p_in_payload->command) {
    case M2M2_APP_COMMON_CMD_STREAM_START_REQ:
      gsOneTimeValueWhenReadAdpdData = 0;
      if (g_state.num_starts == 0) {
#ifdef ENABLE_PPG_APP

        /*! Initialize the PPG LCFG so that ADPD application works with PPG
            DCB contents, if PPG DCB is present */
        if(ppg_get_dcb_present_flag() == true)
        {
          MwPpg_LoadppgLCFG(M2M2_SENSOR_PPG_LCFG_ID_ADPD4000);
        }

        if(gn_uc_hr_enable)
        {
          uint32_t temp_val;
          MwPPG_ReadLCFG(4, &temp_val);/*! To read featureSelect */
          gPpg_agc_en = ((uint16_t)temp_val & (uint16_t)(STATIC_AGC_EN)) >> STATIC_AGC_BIT_P;
          gn_led_slot_g = (gPpg_agc_en == 1) ? (1 << (gn_uc_hr_slot - 1)) : 0;
          if(gPpg_agc_en)
          {
            gn_agc_active_slots |= (1 << (gn_uc_hr_slot - 1));
          }
          gRun_agc = (gn_agc_active_slots != 0) ? true: false;
          gPpg_agc_done = 0;
          MwPPG_HeartRateInit();
          Adpd400xDrvSetOperationMode(ADPD400xDrv_MODE_IDLE);
          SyncInit();
        }
#endif
        /*
           Before ADPD sensor is started, we need to check & give loadAdpdDcfg
           & clockCalibration cmd, if required. This is becasue WT doesnt give
           this cmd explicitly and hence needs to be handled in f/w
           Following flags are added for the same:
             *adpd4000_get_dcb_present_flag()  -> returns true if DCB is present
             *check_dcb_erase -> this flag is true following DCB erase m2m2 cmd
             *external_load_dcfg -> if this flag is true, which happens if
               loadAdpdDcfg cmd is issued explicityly, no need to do code block
        */
#ifdef DCB
        if ((adpd4000_get_dcb_present_flag() || check_dcb_erase == true) &&
            (external_load_dcfg == false)) {
          if (load_adpd4000_cfg(M2M2_SENSOR_ADPD4000_DEVICE_4000_G) !=
              ADPD4000_DCFG_STATUS_OK) {
            Debug_Handler();
          }

          /*On DVT2 with ADPD4100, clock calibration with clk id = 2 works*/
          if(dvt2)
            clockcalid = 2;
          /*On DVT1 & other boards with ADPD4000, clock calibration with clk id = 6 works*/
          else
            clockcalid = 6;

          if (Adpd4000DoClockCalibration(clockcalid) != 0) {
            Debug_Handler();
          }

          if (check_dcb_erase == true) {
            check_dcb_erase = false;
          }
        }
        external_load_dcfg = false;
        Adpd400xDrvSetParameter(ADPD400x_WATERMARKING, 0, 4);
        Adpd400xDrvDataReadyCallback(adpd_data_ready_cb);
        reset_adpd_packetization();
        /* Reset the Interrupt Status variables */
        _PreviousStatus.level0_int = 1;
        _PreviousStatus.level1_int = 1;
        _PreviousStatus.tia_ch1_int = 1;
        _PreviousStatus.tia_ch2_int = 1;
        for (int i = 0; i < TOTAL_SLOT; i++) {
          if (g_state.sl_pktizer1[i].p_pkt != NULL) {
            post_office_consume_msg(g_state.sl_pktizer1[i].p_pkt);
            g_state.sl_pktizer1[i].p_pkt = NULL;
          }
          if (g_state.sl_pktizer2[i].p_pkt != NULL) {
            post_office_consume_msg(g_state.sl_pktizer2[i].p_pkt);
            g_state.sl_pktizer2[i].p_pkt = NULL;
          }
        }
#endif
        g_adpd_odr = get_adpd_odr();
//        Adpd400xDrvRegWrite(ADPD400x_REG_GPIO01,0x0302);
        enable_ext_syncmode();
        enable_adpd_ext_trigger(g_adpd_odr);
#ifdef USER0_CONFIG_APP
        get_adpd_app_timings_from_user0_config_app_lcfg(&adpd_app_timings.start_time,\
                                      &adpd_app_timings.on_time, &adpd_app_timings.off_time);
        if(adpd_app_timings.start_time > 0)
        {
          adpd_app_timings.delayed_start = true;
        }
#ifdef LOW_TOUCH_FEATURE
        //ADPD app not in continuous mode & its interval operation mode
        if(!is_adpd_app_mode_continuous() && !(get_low_touch_trigger_mode3_status()))
#else
        //ADPD app not in continuous mode
        if(!is_adpd_app_mode_continuous())
#endif

        {
          start_adpd_app_timer();
        }

        if(!adpd_app_timings.delayed_start)
        {
          nRetCode = Adpd400xDrvSetOperationMode(ADPD400xDrv_MODE_SAMPLE);
          adpd_app_timings.app_mode_on = true;
        }
        if(ADPD400xDrv_SUCCESS == nRetCode) {
#else
        if(ADPD400xDrv_SUCCESS == Adpd400xDrvSetOperationMode(ADPD400xDrv_MODE_SAMPLE)) {
#endif// USER0_CONFIG_APP
          g_state.num_starts = 1;
          status = M2M2_APP_COMMON_STATUS_STREAM_STARTED;
        } else {
          status = M2M2_APP_COMMON_STATUS_STREAM_NOT_STARTED;
        }
      } else {
#ifdef ENABLE_TEMPERATURE_APP
        /* if Temperarture stream is already running,
           then set the correct LED current because it was set to 0
           during temperature stream start */
        if ((gsTemperatureStarts > 0) &&
            (gsTemperatureStarts == g_state.num_starts)) {
#ifndef SLOT_SELECT
          for (uint8_t i = 0; i < SLOT_NUM; i++) {
            g_adpd_reg_base = i * ADPD400x_SLOT_BASE_ADDR_DIFF;
            Adpd400xDrvRegWrite(ADPD400x_REG_LED_POW12_A + g_adpd_reg_base,
                led_reg.reg_val[i].reg_pow12);
            Adpd400xDrvRegWrite(ADPD400x_REG_LED_POW34_A + g_adpd_reg_base,
                led_reg.reg_val[i].reg_pow34);
          }
#endif
         }
#endif
        g_state.num_starts++;
        status = M2M2_APP_COMMON_STATUS_STREAM_IN_PROGRESS;
#ifdef USER0_CONFIG_APP
        get_adpd_app_timings_from_user0_config_app_lcfg(&adpd_app_timings.start_time,\
                                      &adpd_app_timings.on_time, &adpd_app_timings.off_time);
        if(adpd_app_timings.start_time > 0)
        {
          adpd_app_timings.delayed_start = true;
        }

#ifdef LOW_TOUCH_FEATURE
        //ADPD app not in continuous mode & its interval operation mode
        if(!is_adpd_app_mode_continuous() && !(get_low_touch_trigger_mode3_status()))
#else
        //ADPD app not in continuous mode
        if(!is_adpd_app_mode_continuous())
#endif
        {
          start_adpd_app_timer();
        }

        if(!adpd_app_timings.delayed_start)
        {
          adpd_app_timings.app_mode_on = true;
          ;//TODO: Check and handle ADPD sample mode/idle mode switching
        }
#endif// USER0_CONFIG_APP
      }
      command = M2M2_APP_COMMON_CMD_STREAM_START_RESP;
      break;

    case M2M2_APP_COMMON_CMD_STREAM_STOP_REQ:
      gsOneTimeValueWhenReadAdpdData = 0;

      if (1 == gbAdpdHighPrio) {
        adi_osal_ThreadSetPrio(NULL, APP_OS_CFG_SENSOR_ADPD4000_TASK_PRIO);
        gbAdpdHighPrio = 0;
      }
      /* reset the sample count after the stream has stopped*/
      gsResetStaticAgcSampleCnt = 0;

      if (g_state.num_starts == 0) {
        status = M2M2_APP_COMMON_STATUS_STREAM_STOPPED;
      } else if (g_state.num_starts == 1) {
        disable_ext_syncmode(); /*! Disable ext sync mode*/
        g_adpd_odr = get_adpd_odr();
        disable_adpd_ext_trigger(g_adpd_odr);
        if (ADPD400xDrv_SUCCESS ==
            Adpd400xDrvSetOperationMode(ADPD400xDrv_MODE_IDLE)) {
          g_state.num_starts = 0;
          status = M2M2_APP_COMMON_STATUS_STREAM_STOPPED;
          external_load_dcfg = false;
          reset_agc_flags();/**reset AGC flags **/

          reset_adpd_packetization();
          for (int i = 0; i < TOTAL_SLOT; i++) {
            if (g_state.sl_pktizer1[i].p_pkt != NULL) {
              post_office_consume_msg(g_state.sl_pktizer1[i].p_pkt);
              g_state.sl_pktizer1[i].p_pkt = NULL;
            }
            if (g_state.sl_pktizer2[i].p_pkt != NULL) {
              post_office_consume_msg(g_state.sl_pktizer2[i].p_pkt);
              g_state.sl_pktizer2[i].p_pkt = NULL;
            }
          }
#ifdef USER0_CONFIG_APP
          if(adpd_app_timings.check_timer_started)
          {
            adpd_timer_stop();
            adpd_app_timings.on_time_count = 0;
            adpd_app_timings.off_time_count = 0;
            adpd_app_timings.start_time_count = 0;
            adpd_app_timings.delayed_start =  false;
          }
#endif//USER0_CONFIG_APP
        } else {
          g_state.num_starts = 1;
          status = M2M2_APP_COMMON_STATUS_ERROR;
        }
#ifdef ENABLE_PPG_APP
        if(gn_uc_hr_enable)
        {
          gPpg_agc_en = 0;
          gPpg_agc_done = 0;
          MwPPG_HeartRateDeInit();
          gn_uc_hr_slot = ADPD4K_SLOT_F; //Slot F
        }
#endif
      } else {
        g_state.num_starts--;
#ifdef ENABLE_TEMPERATURE_APP
        if ((g_state.num_starts > 0) &&
            (g_state.num_starts == gsTemperatureStarts)) {
#ifndef SLOT_SELECT
          for (uint8_t i = 0; i < SLOT_NUM; i++) {
            g_adpd_reg_base = i * ADPD400x_SLOT_BASE_ADDR_DIFF;
            if (Adpd400xDrvRegRead(ADPD400x_REG_LED_POW12_A + g_adpd_reg_base,
                    &led_reg.reg_val[i].reg_pow12) == ADPD400xDrv_SUCCESS) {
              Adpd400xDrvRegWrite(ADPD400x_REG_LED_POW12_A + g_adpd_reg_base, 0x0);
            } /* disable led for slot-A - I */

            if (Adpd400xDrvRegRead(ADPD400x_REG_LED_POW34_A + g_adpd_reg_base,
                    &led_reg.reg_val[i].reg_pow34) == ADPD400xDrv_SUCCESS) {
              Adpd400xDrvRegWrite(ADPD400x_REG_LED_POW34_A + g_adpd_reg_base, 0x0);
            } /* disable led for slot-> A - I */
          }
#endif
        }
#endif//ENABLE_TEMPERATURE_APP
        status = M2M2_APP_COMMON_STATUS_STREAM_COUNT_DECREMENT;
      }
      command = M2M2_APP_COMMON_CMD_STREAM_STOP_RESP;
      break;

    case M2M2_APP_COMMON_CMD_STREAM_SUBSCRIBE_REQ:
      /* Get the slot number for every subscription. */
      if(p_in_payload->stream == M2M2_ADDR_SYS_STATIC_AGC_STREAM) {
        g_state_static_agc.num_subs++;
        if(g_state_static_agc.num_subs == 1) {
        /* reset pkt sequence no. only during 1st sub request */
          static_agc_stream_state_reset();
        }
        post_office_setup_subscriber(M2M2_ADDR_SENSOR_ADPD4000, M2M2_ADDR_SYS_STATIC_AGC_STREAM, p_pkt->src, true);
      }else {
        if (p_in_payload->stream == M2M2_ADDR_SENSOR_ADPD_OPTIONAL_BYTES_STREAM) {
          /* Enable Stream Flag */
          bIsStatusStreamEnabled = true;
          /* Reset the Interrupt Status variables */
          _PreviousStatus.level0_int = 1;
          _PreviousStatus.level1_int = 1;
          _PreviousStatus.tia_ch1_int = 1;
          _PreviousStatus.tia_ch2_int = 1;
        } else {
          sl_num = p_in_payload->stream - M2M2_ADDR_SENSOR_ADPD_STREAM1;
          g_state.num_subs[sl_num]++;
          if(g_state.num_subs[sl_num] == 1)
          {
            /* reset pkt sequence no. only during 1st sub request */
            g_state.data_pkt_seq_num[sl_num][CH1] = 0;
            g_state.data_pkt_seq_num[sl_num][CH2] = 0;
          }
#ifdef ENABLE_DEBUG_STREAM
          g_adpd_debug_data.num_subs++;
          if(g_adpd_debug_data.num_subs == 1){
            g_adpd_debug_data.data_pkt_seq_num = 0;
            g_adpdOffset = 0;
            memset(g_adpd_debugInfo,0,sizeof(g_adpd_debugInfo));
            post_office_setup_subscriber(M2M2_ADDR_SENSOR_ADPD4000,M2M2_ADDR_SYS_DBG_STREAM, p_pkt->src, true);
          }
#endif
          //Check if no: of subscribers are more than 1, for all slots
          uint8_t i;
          // Go through total adpd slots
          gb_adpd_multi_sub_start = 0;
          for (i = 0; i < SLOT_NUM; i++) {
            if (g_state.num_subs[i] >= 1) {
              gb_adpd_multi_sub_start = 1;
              break;
            }
          }
        }
        post_office_setup_subscriber(
            M2M2_ADDR_SENSOR_ADPD4000, p_in_payload->stream, p_pkt->src, true);
      }
      status = M2M2_APP_COMMON_STATUS_SUBSCRIBER_ADDED;
      command = M2M2_APP_COMMON_CMD_STREAM_SUBSCRIBE_RESP;
      break;

    case M2M2_APP_COMMON_CMD_STREAM_UNSUBSCRIBE_REQ:
      if(p_in_payload->stream == M2M2_ADDR_SYS_STATIC_AGC_STREAM) {
        if(g_state_static_agc.num_subs <= 1) {
          g_state_static_agc.num_subs = 0;
          static_agc_stream_state_reset();
          status = M2M2_APP_COMMON_STATUS_SUBSCRIBER_REMOVED;
        }else {
          g_state_static_agc.num_subs--;
          status = M2M2_APP_COMMON_STATUS_SUBSCRIBER_COUNT_DECREMENT;
        }
        post_office_setup_subscriber(M2M2_ADDR_SENSOR_ADPD4000, M2M2_ADDR_SYS_STATIC_AGC_STREAM, p_pkt->src, false);
      }else {
        if (p_in_payload->stream == M2M2_ADDR_SENSOR_ADPD_OPTIONAL_BYTES_STREAM) {
            bIsStatusStreamEnabled = false;
            status = M2M2_APP_COMMON_STATUS_SUBSCRIBER_REMOVED;
        }else {
          sl_num = p_in_payload->stream - M2M2_ADDR_SENSOR_ADPD_STREAM1;
          if (g_state.num_subs[sl_num] <= 1) {
            g_state.num_subs[sl_num] = 0;
            status = M2M2_APP_COMMON_STATUS_SUBSCRIBER_REMOVED;
          } else {
            g_state.num_subs[sl_num]--;
            status = M2M2_APP_COMMON_STATUS_SUBSCRIBER_COUNT_DECREMENT;
          }
#ifdef ENABLE_DEBUG_STREAM
          if(g_adpd_debug_data.num_subs <= 1){
            g_adpd_debug_data.num_subs = 0;
            g_adpd_debug_data.data_pkt_seq_num = 0;
            g_adpdOffset = 0;
            memset(g_adpd_debugInfo,0,sizeof(g_adpd_debugInfo));
            post_office_setup_subscriber(M2M2_ADDR_SENSOR_ADPD4000,M2M2_ADDR_SYS_DBG_STREAM,p_pkt->src, false);
          }else{
            g_adpd_debug_data.num_subs--;
          }
#endif
        }
        post_office_setup_subscriber(
            M2M2_ADDR_SENSOR_ADPD4000, p_in_payload->stream, p_pkt->src, false);
        //Check if no: of subscribers are more than 1, for all slots
        uint8_t i;
        // Go through total adpd slots
        gb_adpd_multi_sub_start = 0;
        for (i = 0; i < SLOT_NUM; i++) {
          if (g_state.num_subs[i] >= 1) {
            gb_adpd_multi_sub_start = 1;
            break;
          }
        }
      }
      command = M2M2_APP_COMMON_CMD_STREAM_UNSUBSCRIBE_RESP;
      break;

    default:
      post_office_consume_msg(p_resp_pkt);
      return NULL;
      break;
    }
    p_resp_payload->command = command;
    p_resp_payload->status = status;
    p_resp_payload->stream = p_in_payload->stream;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  } // if(NULL != p_resp_pkt)
  return p_resp_pkt;
}

/*!
 ****************************************************************************
 * @brief Set ADPD4k operation mode to pause/ unpause
 *
 * @param[in]           p_pkt: input m2m2 packet
 *
 * @return              pointer to reponse m2m2 packet
 *****************************************************************************/
static m2m2_hdr_t *adpd_app_set_pause(m2m2_hdr_t *p_pkt) {
  /* Declare a pointer to access the input packet payload */
  PYLD_CST(p_pkt, m2m2_sensor_adpd4000_resp_t, p_in_payload);

  /* Declare and malloc a response packet */
  PKT_MALLOC(p_resp_pkt, m2m2_sensor_adpd4000_resp_t, 0);
  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_sensor_adpd4000_resp_t, p_resp_payload);

    memset(p_resp_payload,0,sizeof(m2m2_sensor_adpd4000_resp_t));
    p_resp_payload->deviceid = M2M2_SENSOR_ADPD4000_DEVICE_4000_G_R_IR_B;
    p_resp_payload->status = M2M2_APP_COMMON_STATUS_ERROR;

    /*Pause bit is expected to be either 1 or 0;
      1 for Pause/Idle mode ;
      0 for Unpause/Sample mode*/

    if((p_in_payload->retdata[0]) == 0x1)
    {
      if ((g_state.num_starts !=0) && (gAdpdPause == false))
      {
        /*Put the device into Idle mode*/
        if (Adpd400xDrvSetOperationPause(0) == ADPD400xDrv_SUCCESS)
        {
          p_resp_payload->status = M2M2_APP_COMMON_STATUS_OK;
          gAdpdPause = true;
        }
      }
    } else if((p_in_payload->retdata[0]) == 0x0)
    {
      if ((g_state.num_starts !=0) && (gAdpdPause == true))
      {
        /*Put the device into Sample mode*/
        if (Adpd400xDrvSetOperationPause(1) == ADPD400xDrv_SUCCESS)
        {
          p_resp_payload->status = M2M2_APP_COMMON_STATUS_OK;
          gAdpdPause = false;
        }
      }
    }
    else
    {
      p_resp_payload->status = M2M2_APP_COMMON_STATUS_ERROR;
    }

    p_resp_payload->retdata[0] = p_in_payload->retdata[0];
    p_resp_payload->command = M2M2_SENSOR_ADPD_COMMAND_SET_PAUSE_RESP;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
    gsOneTimeValueWhenReadAdpdData = 0;
  }
  return p_resp_pkt;
}

/*!
 ****************************************************************************
 * @brief Read/Write ADPD4k registers
 *
 * @param[in]           p_pkt: input m2m2 packet
 *
 * @return              pointer to reponse m2m2 packet
 *****************************************************************************/
#ifdef ADPD_SEM_CORRUPTION_DEBUG
uint32_t reg_write_cnt=0;
#endif
static m2m2_hdr_t *adpd_app_reg_access(m2m2_hdr_t *p_pkt) {
  M2M2_APP_COMMON_STATUS_ENUM_t status = M2M2_APP_COMMON_STATUS_ERROR;
  /* Declare a pointer to access the input packet payload */
  PYLD_CST(p_pkt, m2m2_sensor_common_reg_op_16_hdr_t, p_in_payload);
  /* Allocate a response packet
     with space for the correct number of operations */
  PKT_MALLOC(p_resp_pkt, m2m2_sensor_common_reg_op_16_hdr_t,
      p_in_payload->num_ops * sizeof(p_in_payload->ops[0]));
  if (NULL != p_resp_pkt) {
    PYLD_CST(p_resp_pkt, m2m2_sensor_common_reg_op_16_hdr_t, p_resp_payload);
    uint16_t reg_data = 0;

    switch (p_in_payload->command) {
    case M2M2_SENSOR_COMMON_CMD_READ_REG_16_REQ:
      for (int i = 0; i < p_in_payload->num_ops; i++) {
        if (Adpd400xDrvRegRead(p_in_payload->ops[i].address, &reg_data) ==
            ADPD400xDrv_SUCCESS) {
          status = M2M2_APP_COMMON_STATUS_OK;
        } else {
          status = M2M2_APP_COMMON_STATUS_ERROR;
        }
        p_resp_payload->ops[i].address = p_in_payload->ops[i].address;
        p_resp_payload->ops[i].value = reg_data;
      }
      p_resp_payload->command =
          (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_SENSOR_COMMON_CMD_READ_REG_16_RESP;
      break;
    case M2M2_SENSOR_COMMON_CMD_WRITE_REG_16_REQ:
#ifdef ADPD_SEM_CORRUPTION_DEBUG
      reg_write_cnt += 1;
#endif
      for (int i = 0; i < p_in_payload->num_ops; i++) {
        if (Adpd400xDrvRegWrite(p_in_payload->ops[i].address,
                p_in_payload->ops[i].value) == ADPD400xDrv_SUCCESS) {
          status = M2M2_APP_COMMON_STATUS_OK;
        } else {
          status = M2M2_APP_COMMON_STATUS_ERROR;
        }
        p_resp_payload->ops[i].address = p_in_payload->ops[i].address;
        p_resp_payload->ops[i].value = p_in_payload->ops[i].value;
      }
      p_resp_payload->command =(M2M2_APP_COMMON_CMD_ENUM_t)M2M2_SENSOR_COMMON_CMD_WRITE_REG_16_RESP;
      break;
    default:
      post_office_consume_msg(p_resp_pkt);
      return NULL;
    }
    p_resp_pkt->dest = p_pkt->src;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_payload->num_ops = p_in_payload->num_ops;
    p_resp_payload->status = status;
  }
  return p_resp_pkt;
}

/*!
 ****************************************************************************
 * @brief Get/Set ADPD4k stream decimation factor
 *
 * @param[in]           p_pkt: input m2m2 packet
 *
 * @return              pointer to reponse m2m2 packet
 *****************************************************************************/
static m2m2_hdr_t *adpd_app_decimation(m2m2_hdr_t *p_pkt) {
  M2M2_APP_COMMON_STATUS_ENUM_t status = M2M2_APP_COMMON_STATUS_OK;

  /* Declare a pointer to access the input packet payload */
  PYLD_CST(p_pkt, m2m2_sensor_common_decimate_stream_t, p_in_payload);
  /* Allocate a response packet
     with space for the correct number of operations */
  PKT_MALLOC(p_resp_pkt, m2m2_sensor_common_decimate_stream_t, 0);
  if (NULL != p_resp_pkt) {
    PYLD_CST(p_resp_pkt, m2m2_sensor_common_decimate_stream_t, p_resp_payload);

    switch (p_in_payload->command) {
    case M2M2_SENSOR_COMMON_CMD_GET_STREAM_DEC_FACTOR_REQ:
      p_resp_payload->command =
          M2M2_SENSOR_COMMON_CMD_GET_STREAM_DEC_FACTOR_RESP;
      status = M2M2_APP_COMMON_STATUS_OK;
      break;
    case M2M2_SENSOR_COMMON_CMD_SET_STREAM_DEC_FACTOR_REQ:
      g_state.decimation_factor = p_in_payload->dec_factor;
      if (g_state.decimation_factor == 0) {
        g_state.decimation_factor = 1;
      }
      status = M2M2_APP_COMMON_STATUS_OK;
      p_resp_payload->command =
          M2M2_SENSOR_COMMON_CMD_SET_STREAM_DEC_FACTOR_RESP;
      break;
    }
    p_resp_payload->status = status;
    p_resp_payload->dec_factor = g_state.decimation_factor;
    p_resp_payload->stream = p_in_payload->stream;
    p_resp_pkt->dest = p_pkt->src;
    p_resp_pkt->src = p_pkt->dest;
  }
  return p_resp_pkt;
}

/*!
 ****************************************************************************
 * @brief Create ADPD4k Dcfg based on selected Slot and Application
 *
 * @param[in]           p_pkt: input m2m2 packet
 *
 * @return              pointer to reponse m2m2 packet
 *****************************************************************************/
static m2m2_hdr_t *adpd_app_create_dcfg(m2m2_hdr_t *p_pkt) {
  M2M2_APP_COMMON_STATUS_ENUM_t status = M2M2_APP_COMMON_STATUS_ERROR;
  /* Declare a pointer to access the input packet payload */
  PYLD_CST(p_pkt, m2m2_adpd_dcfg_op_hdr_t, p_in_payload);
  PKT_MALLOC(p_resp_pkt, m2m2_adpd_dcfg_op_hdr_t,
      p_in_payload->num_ops * sizeof(p_in_payload->ops[0]));
  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_adpd_dcfg_op_hdr_t, p_resp_payload);

    for (uint8_t i = 0; i < p_in_payload->num_ops; i++) {
      if (get_adpd4k_dcfg(p_in_payload->ops[i].slotid,
              p_in_payload->ops[i].appid, i,
              p_in_payload->num_ops) == ADPD4000_DCFG_STATUS_OK) {
        status = M2M2_APP_COMMON_STATUS_OK;
      } else {
        status = M2M2_APP_COMMON_STATUS_ERROR;
        break;
      }
    }

    p_resp_payload->num_ops = p_in_payload->num_ops;
    for (uint8_t i = 0; i < p_in_payload->num_ops; i++) {
      p_resp_payload->ops[i].slotid = p_in_payload->ops[i].slotid;
      p_resp_payload->ops[i].appid = p_in_payload->ops[i].appid;
    }
    p_resp_payload->status = status;
    p_resp_payload->command =
        (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_SENSOR_ADPD_COMMAND_CREATE_DCFG_RESP;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  }
  return p_resp_pkt;
}

#ifdef SLOT_SELECT
/*!
 ****************************************************************************
 * @brief Disable ADPD4k Slots
 *
 * @param[in]           p_pkt: input m2m2 packet
 *
 * @return              pointer to reponse m2m2 packet
 *****************************************************************************/
static m2m2_hdr_t *adpd_app_disable_slot(m2m2_hdr_t *p_pkt) {
  M2M2_APP_COMMON_STATUS_ENUM_t status = M2M2_APP_COMMON_STATUS_ERROR;
  /* Declare a pointer to access the input packet payload */
  PYLD_CST(p_pkt, m2m2_adpd4k_slot_info_t, p_in_payload);
  PKT_MALLOC(p_resp_pkt, m2m2_adpd4k_slot_info_t, 0);
  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_adpd4k_slot_info_t, p_resp_payload);

    uint16_t reg_addr = ADPD400x_REG_OPMODE;
    uint16_t reg_value = 0x0000;
    if (g_state.num_starts == 0) {
      if (Adpd400xDrvSlotSetup(1, 0, 0x04, 1) == ADPD400xDrv_SUCCESS) {
        status = M2M2_APP_COMMON_STATUS_OK;
      } else {
        status = M2M2_APP_COMMON_STATUS_ERROR;
      }
    }
    p_resp_payload->status = status;
    p_resp_payload->command =
        (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_SENSOR_ADPD_COMMAND_DISABLE_SLOTS_RESP;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
    return p_resp_pkt;
  }
}
#endif

/*!
 ****************************************************************************
 * @brief test command
 *
 * @param[in]           p_pkt: input m2m2 packet
 *
 * @return              pointer to reponse m2m2 packet
 *****************************************************************************/
static m2m2_hdr_t *adpd_app_do_general3_cmd(m2m2_hdr_t *p_pkt) {
  /* Declare and malloc a response packet */
  PKT_MALLOC(p_resp_pkt, m2m2_sensor_adpd_testcommand_resp_t, 0);
  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_sensor_adpd_testcommand_resp_t, p_resp_payload);
    /* p_resp_payload->status = M2M2_APP_COMMON_STATUS_ERROR; */
    p_resp_payload->status = M2M2_APP_COMMON_STATUS_OK;
    p_resp_payload->retdata[0] = 0x99007788;
    p_resp_payload->retdata[1] = 0x123456;
    p_resp_payload->retdata[2] = 12345678;
    p_resp_payload->command = M2M2_SENSOR_ADPD_COMMAND_DO_TEST3_RESP;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  }
  return p_resp_pkt;
}

/*!
 ****************************************************************************
 * @brief test command
 *
 * @param[in]           p_pkt: input m2m2 packet
 *
 * @return              pointer to reponse m2m2 packet
 *****************************************************************************/
static m2m2_hdr_t *adpd_app_do_general2_cmd(m2m2_hdr_t *p_pkt) {
  /* Declare and malloc a response packet */
  PKT_MALLOC(p_resp_pkt, m2m2_sensor_adpd_testcommand_resp_t, 0);
  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_sensor_adpd_testcommand_resp_t, p_resp_payload);
    /* p_resp_payload->status = M2M2_APP_COMMON_STATUS_ERROR; */

    p_resp_payload->status = M2M2_APP_COMMON_STATUS_OK;
    p_resp_payload->retdata[0] = 0x55667788;
    p_resp_payload->retdata[1] = 0xabcdef12;
    p_resp_payload->retdata[2] = 12345678;
    p_resp_payload->command = M2M2_SENSOR_ADPD_COMMAND_DO_TEST2_RESP;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  }
  return p_resp_pkt;
}

/*!
 ****************************************************************************
 * @brief test command
 *
 * @param[in]           p_pkt: input m2m2 packet
 *
 * @return              pointer to reponse m2m2 packet
 *****************************************************************************/
static m2m2_hdr_t *adpd_app_do_general1_cmd(m2m2_hdr_t *p_pkt) {

  PYLD_CST(p_pkt, m2m2_sensor_adpd_testcommand_resp_t, p_in_payload);

  switch (p_in_payload->retdata[0]) {
  case 0:
    break;
  case 1:
    break;
  case 2:
    fetch_adpd_data();
    break;
  case 3:
    Adpd400xDrvRegWrite(0, 0x8000); /* Clear FIFO */
    break;
  case 4:
    Adpd400xDrvRegWrite(1, 0x8000); /* Clear FIFO status */
    break;
  default:
    break;
  }

  /* Declare and malloc a response packet */
  PKT_MALLOC(p_resp_pkt, m2m2_sensor_adpd_testcommand_resp_t, 0);
  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_sensor_adpd_testcommand_resp_t, p_resp_payload);
    /* p_resp_payload->status = M2M2_APP_COMMON_STATUS_ERROR; */

    p_resp_payload->status = M2M2_APP_COMMON_STATUS_OK;
    p_resp_payload->retdata[0] = 0x11223344;
    p_resp_payload->retdata[1] = 0xaabbccdd;
    p_resp_payload->retdata[2] = 12345678;
    p_resp_payload->command = M2M2_SENSOR_ADPD_COMMAND_DO_TEST1_RESP;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  }
  return p_resp_pkt;
}

#ifdef ENABLE_PPG_APP
/*!
 ****************************************************************************
 * @brief Set UC1,2,3,5 HR calculation to enable/dsisable
 *
 * @param[in]           p_pkt: input m2m2 packet
 *
 * @return              pointer to reponse m2m2 packet
 *****************************************************************************/
static m2m2_hdr_t *adpd_app_uc_hr_enable(m2m2_hdr_t *p_pkt) {
  /* Declare a pointer to the response packet payload */
  PYLD_CST(p_pkt, m2m2_adpd_set_uc_hr_enab_t, p_in_payload);
  /* Declare and malloc a response packet */
  PKT_MALLOC(p_resp_pkt, m2m2_adpd_set_uc_hr_enab_t, 0);
  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_adpd_set_uc_hr_enab_t, p_resp_payload);

    gn_uc_hr_enable = p_in_payload->control;
    gn_uc_hr_slot =  p_in_payload->slotNum;

    //Cleat all flags used in UC HR
    gnHRAdpdSampleCount = 0;
    gnHRAdxlSampleCount = 0;

    gnAppSyncTimerStarted = 0;

    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
    p_resp_payload->command = M2M2_SENSOR_ADPD_COMMAND_UC_HR_ENAB_RESP;
    p_resp_payload->status = M2M2_APP_COMMON_STATUS_OK;
    p_resp_payload->control = p_in_payload->control;
    p_resp_payload->slotNum = p_in_payload->slotNum;
  }
  return p_resp_pkt;
}
#endif

/*!
 ****************************************************************************
 * @brief Set AGC ON/OFF for each of the four LEDs in ADPD4k
 *
 * @param[in]           p_pkt: input m2m2 packet
 *
 * @return              pointer to reponse m2m2 packet
 *****************************************************************************/
static m2m2_hdr_t *adpd_app_agc_on_off_cntrl(m2m2_hdr_t *p_pkt) {
  /* Declare a pointer to the response packet payload */
  PYLD_CST(p_pkt, m2m2_adpd_agc_cntrl_t, p_in_payload);
  /* Declare and malloc a response packet */
  PKT_MALLOC(p_resp_pkt, m2m2_adpd_agc_cntrl_t,
      p_in_payload->num_ops * sizeof(p_in_payload->ops[0]));
  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_adpd_agc_cntrl_t, p_resp_payload);

    p_resp_payload->command = M2M2_SENSOR_ADPD_COMMAND_AGC_ON_OFF_RESP;
    p_resp_payload->num_ops = p_in_payload->num_ops;
    for (uint8_t i = 0; i < p_in_payload->num_ops; i++) {
      p_resp_payload->ops[i].agc_cntrl = p_in_payload->ops[i].agc_cntrl;
      p_resp_payload->ops[i].agc_type = p_in_payload->ops[i].agc_type;
    }

    /* Turn ON STATIC_AGC */
    for (uint8_t i = 0; i < p_in_payload->num_ops; i++) {
      switch (p_in_payload->ops[i].agc_type) {
      case 0: /* MWL view */
      {
        if (p_in_payload->ops[i].agc_cntrl) /* AGC_ON */
        {
#ifndef SLOT_SELECT
          gn_led_slot_g = G_LED_AGC_SLOTS;
          gn_led_slot_r = R_LED_AGC_SLOTS;
          gn_led_slot_ir = IR_LED_AGC_SLOTS;
          gn_led_slot_b = B_LED_AGC_SLOTS;
#endif
        }
        else
        {
          gn_led_slot_g = 0;
          gn_led_slot_r = 0;
          gn_led_slot_ir = 0;
          gn_led_slot_b = 0;
        }
          p_resp_payload->status = M2M2_APP_COMMON_STATUS_OK;
          break;
      }
      case 1: /* Green LED */
      {
        if (p_in_payload->ops[i].agc_cntrl)
         {
#ifndef SLOT_SELECT
           gn_led_slot_g = G_LED_AGC_SLOTS;
#endif
         }
         else
         {
           gn_led_slot_g = 0;
         }
          p_resp_payload->status = M2M2_APP_COMMON_STATUS_OK;
          break;
      }
      case 2: /* Red LED */
      {
        if (p_in_payload->ops[i].agc_cntrl)
         {
#ifndef SLOT_SELECT
           gn_led_slot_r = R_LED_AGC_SLOTS;
#endif
         }
         else
         {
           gn_led_slot_r = 0;
         }
          p_resp_payload->status = M2M2_APP_COMMON_STATUS_OK;
          break;
      }
      case 3: /* IR LED */
      {
        if (p_in_payload->ops[i].agc_cntrl)
         {
#ifndef SLOT_SELECT
           gn_led_slot_ir = IR_LED_AGC_SLOTS;
#endif
         }
         else
         {
           gn_led_slot_ir = 0;
         }
          p_resp_payload->status = M2M2_APP_COMMON_STATUS_OK;
          break;
      }
      case 4: /* Blue LED */
      {
        if (p_in_payload->ops[i].agc_cntrl)
         {
#ifndef SLOT_SELECT
           gn_led_slot_b = B_LED_AGC_SLOTS;
#endif
         }
         else
         {
           gn_led_slot_b = 0;
         }
          p_resp_payload->status = M2M2_APP_COMMON_STATUS_OK;
          break;
      }
      default: {
        p_resp_payload->status = M2M2_APP_COMMON_STATUS_ERROR;
        break;
      }
      } // switch case
    }   // for loop
    gn_agc_active_slots = gn_led_slot_g |  gn_led_slot_r | gn_led_slot_ir | gn_led_slot_b;
    gRun_agc = (gn_agc_active_slots != 0) ? true: false;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  } // if(NULL != p_resp_pkt)
  return p_resp_pkt;
}

/*!
 ****************************************************************************
 * @brief Get AGC Settings done in ADPD4k
 *
 * @param[in]           p_pkt: input m2m2 packet
 *
 * @return              pointer to reponse m2m2 packet
 *****************************************************************************/
static m2m2_hdr_t *adpd_app_agc_info(m2m2_hdr_t *p_pkt) {
  /* Declare a pointer to the response packet payload */
  PYLD_CST(p_pkt, m2m2_adpd_agc_info_t, p_in_payload);
  /* Declare and malloc a response packet */
  PKT_MALLOC(p_resp_pkt, m2m2_adpd_agc_info_t, 0);
  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_adpd_agc_info_t, p_resp_payload);

    p_resp_payload->led_index = p_in_payload->led_index;
    get_agc_info(p_resp_payload);
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
    p_resp_payload->command = M2M2_SENSOR_ADPD_COMMAND_AGC_INFO_RESP;
    p_resp_payload->status = M2M2_APP_COMMON_STATUS_OK;
  }
  return p_resp_pkt;
}

#ifdef DCB
/*!
 ****************************************************************************
 * @brief Read ADPD4K DCB Configuration
 *
 * @param[in]           p_pkt: input m2m2 packet
 *
 * @return              pointer to reponse m2m2 packet
 *****************************************************************************/
static m2m2_hdr_t *adpd_dcb_command_read_config(m2m2_hdr_t *p_pkt)
{
    uint16_t r_size;
    uint16_t i = 0, num_pkts = 0, dcbdata_index = 0;

    /* dcbdata - storage for DCB content during READ DCB block
     * for Gen block DCB; Reusing the RAM buffer from system task,
     * instead of declaring it in the function,
     * saves space on Stack requirement from LT Task
     */
    uint32_t *dcbdata = (uint32_t *)&gsConfigTmpFile[0];
    ADI_OSAL_STATUS  err;
    M2M2_DCB_STATUS_ENUM_t status = M2M2_DCB_STATUS_ERR_NOT_CHKD;

    PKT_MALLOC(p_resp_pkt, m2m2_dcb_adpd4000_data_t, 0);
    if(p_resp_pkt != NULL )
    {
       // Declare a pointer to the response packet payload
       PYLD_CST(p_resp_pkt, m2m2_dcb_adpd4000_data_t, p_resp_payload);
       memset(p_resp_payload->dcbdata, 0, sizeof(p_resp_payload->dcbdata));
       memset(dcbdata, 0, sizeof(dcbdata));
       r_size = (uint16_t)(MAXADPD4000DCBSIZE*MAX_ADPD4000_DCB_PKTS); //Max words that can be read from FDS
       if(read_adpd4000_dcb(&dcbdata[0], &r_size) == ADPD4000_DCB_STATUS_OK)
       {
         status = M2M2_DCB_STATUS_OK;
         num_pkts = (r_size / MAXADPD4000DCBSIZE) + ((r_size % MAXADPD4000DCBSIZE) ? 1 : 0 );
         dcbdata_index = 0;
         for(uint16_t p = 0; p < num_pkts; p++)
         {
      	   p_resp_payload->size = \
                (p != num_pkts-1) ? MAXADPD4000DCBSIZE : (r_size % MAXADPD4000DCBSIZE);
      	   p_resp_payload->num_of_pkts = num_pkts;
           for (i = 0; i < p_resp_payload->size; i++)
           {
      	      p_resp_payload->dcbdata[i] = dcbdata[dcbdata_index++];
           }
           if(p != num_pkts-1)
      	   {
             p_resp_pkt->src = p_pkt->dest;
      	     p_resp_pkt->dest = p_pkt->src;
             p_resp_payload->status = status;
             p_resp_payload->command = M2M2_DCB_COMMAND_READ_CONFIG_RESP;
      	     post_office_send(p_resp_pkt, &err);

      	     /* Delay is kept same as what is there in low touch task */
      	     MCU_HAL_Delay(60);

      	     PKT_MALLOC(p_resp_pkt, m2m2_dcb_adpd4000_data_t, 0);
      	     if(NULL != p_resp_pkt)
      	     {
               // Declare a pointer to the response packet payload
               PYLD_CST(p_resp_pkt, m2m2_dcb_adpd4000_data_t, p_resp_payload);
               memset(p_resp_payload->dcbdata, 0, sizeof(p_resp_payload->dcbdata));
      	     }//if(NULL != p_resp_pkt)
      	     else
      	     {
               return NULL;
      	     }
      	   }
         }
        }
        else
        {
            p_resp_payload->size = 0;
            p_resp_payload->num_of_pkts = 0;
            status = M2M2_DCB_STATUS_ERR_ARGS;
        }//if(read_adpd4000_dcb())
        p_resp_payload->status = status;
        p_resp_payload->command = M2M2_DCB_COMMAND_READ_CONFIG_RESP;
        p_resp_pkt->src = p_pkt->dest;
        p_resp_pkt->dest = p_pkt->src;
    }//if(NULL != p_resp_pkt)
  return p_resp_pkt;
}

/*!
 ****************************************************************************
 * @brief Write ADPD4K DCB Configuration
 *
 * @param[in]           p_pkt: input m2m2 packet
 *
 * @return              pointer to reponse m2m2 packet
 *****************************************************************************/
static m2m2_hdr_t *adpd_dcb_command_write_config(m2m2_hdr_t *p_pkt)
{
    M2M2_DCB_STATUS_ENUM_t status = M2M2_DCB_STATUS_ERR_NOT_CHKD;
    static uint16_t i = 0;
    static uint16_t num_of_pkts = 0;
    uint16_t j;
    /* dcbdata - storage for DCB content during write DCB block
     * for Gen block DCB. Reusing the RAM buffer from system task,
     * instead of declaring a static buffer in the function,
     * saves space on total RAM usage from the watch application
     */
    uint32_t *dcbdata = (uint32_t *)&gsConfigTmpFile[0];

    // Declare a pointer to access the input packet payload
    PYLD_CST(p_pkt, m2m2_dcb_adpd4000_data_t, p_in_payload);
    PKT_MALLOC(p_resp_pkt, m2m2_dcb_adpd4000_data_t, 0);
    if(p_resp_pkt != NULL )
    {
        // Declare a pointer to the response packet payload
        PYLD_CST(p_resp_pkt, m2m2_dcb_adpd4000_data_t, p_resp_payload);

        //Maximum two packets can be written to ADPD4000_DCB
        if(p_in_payload->num_of_pkts >= 1 && p_in_payload->num_of_pkts <= MAX_ADPD4000_DCB_PKTS)
        {
            num_of_pkts += 1;
            for( j=0; j<p_in_payload->size; j++ )
              dcbdata[i++] = p_in_payload->dcbdata[j];
            NRF_LOG_INFO("Wr DCB:pkt sz->%d, arr index=%d",p_in_payload->size,i);
            if(num_of_pkts == p_in_payload->num_of_pkts)
            {
                if(write_adpd4000_dcb(&dcbdata[0], i) == ADPD4000_DCB_STATUS_OK)
                {
                    adpd4000_set_dcb_present_flag(true);
                    status = M2M2_DCB_STATUS_OK;
                }
                else
                {
                    status = M2M2_DCB_STATUS_ERR_ARGS;
                }
                num_of_pkts = 0;
                i=0;
            }
            else
              status = M2M2_DCB_STATUS_OK;
        }
        else
        {
            status = M2M2_DCB_STATUS_ERR_ARGS;
        }

        p_resp_payload->status = status;
        p_resp_payload->command = M2M2_DCB_COMMAND_WRITE_CONFIG_RESP;
        p_resp_payload->size = 0;
        p_resp_payload->num_of_pkts = p_in_payload->num_of_pkts;
        for(uint16_t k=0; k < MAXADPD4000DCBSIZE; k++)
           p_resp_payload->dcbdata[k] = 0;
        p_resp_pkt->src = p_pkt->dest;
        p_resp_pkt->dest = p_pkt->src;
    }
  return p_resp_pkt;
}

/*!
 ****************************************************************************
 * @brief Delete ADPD4K DCB Configuration
 *
 * @param[in]           p_pkt: input m2m2 packet
 *
 * @return              pointer to reponse m2m2 packet
 *****************************************************************************/
static m2m2_hdr_t *adpd_dcb_command_delete_config(m2m2_hdr_t *p_pkt) {
  M2M2_DCB_STATUS_ENUM_t status = M2M2_DCB_STATUS_ERR_NOT_CHKD;

  PKT_MALLOC(p_resp_pkt, m2m2_dcb_adpd4000_data_t, 0);
  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_dcb_adpd4000_data_t, p_resp_payload);

    if (delete_adpd4000_dcb() == ADPD4000_DCB_STATUS_OK) {
      adpd4000_set_dcb_present_flag(false);
      check_dcb_erase = true;
      status = M2M2_DCB_STATUS_OK;
    } else {
      status = M2M2_DCB_STATUS_ERR_ARGS;
    }

    p_resp_payload->status = status;
    p_resp_payload->command = M2M2_DCB_COMMAND_ERASE_CONFIG_RESP;
    p_resp_payload->size = 0;
    for(uint16_t i=0; i< MAXADPD4000DCBSIZE; i++)
      p_resp_payload->dcbdata[i] = 0;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  }
  return p_resp_pkt;
}
#endif

/*!
 ****************************************************************************
 * @brief Callback function for ADPD4k FIFO Interrupt
 *
 * @param[in]    None
 *
 * @return       None
 *****************************************************************************/
#ifdef ADPD_SEM_CORRUPTION_DEBUG
uint32_t g_adpd_isr_list[40] = {0};
uint8_t adpd_list_index = 0;
#endif
static void adpd_data_ready_cb(void) {
  /* Get the RTC timestamp at the time the ADPD data ready interrupt happened */
#ifdef ENABLE_DEBUG_STREAM
  g_adpd_ts_flag_set = 1;
#endif  
  gADPD4000_dready_ts = get_sensor_time_stamp();
#ifdef ADPD_SEM_CORRUPTION_DEBUG
 g_adpd_isr_list[adpd_list_index++] = gADPD4000_dready_ts;
  if (adpd_list_index >=40)
    adpd_list_index =0;
#endif
  g_time_diff = gADPD4000_dready_ts - gADPD4000_dready_ts_prev;
  gAdpdIntCnt++;
  adi_osal_SemPost(adpd4000_task_evt_sem);
  gADPD4000_dready_ts_prev = gADPD4000_dready_ts;
}

/*!
 ****************************************************************************
 * @brief ADPD400x _OptionalByteArrange
 *
 * @param[in]         pnData: data pointer reference
 * @param[in]         nRegValue: Optional byte register value
 * @param[in]         timestamp: input m2m2 packet
 * @param[out]        pData: input m2m2 packet
 *
 * @return            bool
 *****************************************************************************/
static bool _OptionalByteArrange(uint8_t *pnData, uint16_t nRegValue,
    uint32_t timestamp, m2m2_hdr_t *pData) {
  /* Flag for returns */
  bool _bFlag = false;
  /* Declare a pointer to the p_payload_ptr */
  PYLD_CST(pData, m2m2_sensor_fifo_status_bytes_t, p_payload_ptr);
  /* Clear the specified memory block */
  memset(p_payload_ptr, 0x00, sizeof(m2m2_sensor_fifo_status_bytes_t));
  p_payload_ptr->command =
      (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_SENSOR_COMMON_CMD_STREAM_DATA;
  p_payload_ptr->status =
      (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_APP_COMMON_STATUS_OK;
  /* Assign timestamp to payload pointer */
  p_payload_ptr->timestamp = timestamp;
  /* Run the loop to get all optinal byte status from FIFO */
  for (uint8_t nIndex = 0; nIndex <= 8; nIndex++) {
    switch (nRegValue & 0x01 << nIndex) {
    case 0x01:
      /* Sequence Number Enabled */
      p_payload_ptr->sequence_num = *pnData & 0x0f;
      pnData++;
      break;
    case 0x02:
      /* Data INT Enabled Slot A-H */
      p_payload_ptr->data_int = *pnData;
      pnData++;
      break;
    case 0x04:
      /* Data INT Enabled Slot I-L */
      p_payload_ptr->data_int = *pnData;
      pnData++;
      break;
    case 0x08:
      /* Level 0 INT Enabled Slot A-H */
      p_payload_ptr->level0_int = *pnData;
      pnData++;
      break;
    case 0x10:
      /* Level 1 INT Enabled Slot A-H */
      p_payload_ptr->level1_int = *pnData;
      pnData++;
      break;
    case 0x20:
      /* Level 0 & 1 Enabled Slot I-L */
      p_payload_ptr->level0_int |= ((*pnData & 0x0f) << 8);
      *pnData = (*pnData & 0xf0) >> 4;
      p_payload_ptr->level1_int |= (*pnData << 8);
      pnData++;
      break;
    case 0x40:
      /* Tia saturation Ch1 INT for Slot A-H */
      p_payload_ptr->tia_ch1_int = *pnData;
      pnData++;
      break;
    case 0x80:
      /* Tia saturation Ch2 INT for Slot A-H */
      p_payload_ptr->tia_ch2_int = *pnData;
      pnData++;
      break;
    case 0x100:
      /* Tia saturation Ch1 & Ch2 INT for Slot I-L */
      p_payload_ptr->tia_ch1_int |= ((uint16_t)(*pnData & 0x0f) << 8);
      p_payload_ptr->tia_ch2_int |= ((uint16_t)(*pnData & 0xf0) << 8);
    default:
      break;
    }
  }
  if (_PreviousStatus.level0_int != p_payload_ptr->level0_int ||
      _PreviousStatus.level1_int != p_payload_ptr->level1_int ||
      _PreviousStatus.tia_ch1_int != p_payload_ptr->tia_ch1_int ||
      _PreviousStatus.tia_ch2_int != p_payload_ptr->tia_ch2_int) {
    /* Copy the current status into previous status array */
    _PreviousStatus.level0_int = p_payload_ptr->level0_int;
    _PreviousStatus.level1_int = p_payload_ptr->level1_int;
    _PreviousStatus.tia_ch1_int = p_payload_ptr->tia_ch1_int;
    _PreviousStatus.tia_ch2_int = p_payload_ptr->tia_ch2_int;
    _bFlag = true;
  } else {
    _bFlag = false;
  }

  return _bFlag;
}

/*!
 ****************************************************************************
 * @brief Read/Write ECG4k lcfg values
 *
 * @param[in]           p_pkt: input m2m2 packet
 *
 * @return              pointer to reponse m2m2 packet
 *****************************************************************************/
static m2m2_hdr_t *ecg4k_lcfg_access(m2m2_hdr_t *p_pkt) {
  M2M2_APP_COMMON_STATUS_ENUM_t status = M2M2_APP_COMMON_STATUS_ERROR;
  /* Declare a pointer to access the input packet payload */
  PYLD_CST(p_pkt, ecg_app_lcfg_op_hdr_t, p_in_payload);

  /* Allocate a response packet
     with space for the correct number of operations */
  PKT_MALLOC(p_resp_pkt, ecg_app_lcfg_op_hdr_t,
      p_in_payload->num_ops * sizeof(p_in_payload->ops[0]));
  if (NULL != p_resp_pkt) {
    PYLD_CST(p_resp_pkt, ecg_app_lcfg_op_hdr_t, p_resp_payload);
    uint16_t reg_data = 0;

    switch (p_in_payload->command) {
    case M2M2_APP_COMMON_CMD_READ_LCFG_REQ:
      for (int i = 0; i < p_in_payload->num_ops; i++) {
        if (Ecg4kgGetLCFG(p_in_payload->ops[i].field, &reg_data) ==
            ADPD4000_DCFG_STATUS_OK) {
          status = M2M2_APP_COMMON_STATUS_OK;
        } else {
          status = M2M2_APP_COMMON_STATUS_ERROR;
        }
        p_resp_payload->ops[i].field = p_in_payload->ops[i].field;
        p_resp_payload->ops[i].value = reg_data;
        reg_data = 0;
      }
      p_resp_payload->command =
          (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_APP_COMMON_CMD_READ_LCFG_RESP;
      break;

    case M2M2_APP_COMMON_CMD_WRITE_LCFG_REQ:
      for (int i = 0; i < p_in_payload->num_ops; i++) {
        if (Ecg4kSetLCFG(p_in_payload->ops[i].field,
                p_in_payload->ops[i].value) == ADPD4000_DCFG_STATUS_OK) {
          status = M2M2_APP_COMMON_STATUS_OK;
        } else {
          status = M2M2_APP_COMMON_STATUS_ERROR;
        }
        p_resp_payload->ops[i].field = p_in_payload->ops[i].field;
        p_resp_payload->ops[i].value = p_in_payload->ops[i].value;
      }
      p_resp_payload->command =
          (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_APP_COMMON_CMD_WRITE_LCFG_RESP;
      break;

    default:
      break;
    }
    p_resp_pkt->dest = p_pkt->src;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_payload->num_ops = p_in_payload->num_ops;
    p_resp_payload->status = status;
  } // if(NULL != p_resp_pkt)

  return p_resp_pkt;
}

/*!
 ****************************************************************************
 * @brief Set the Sampling Frequency for the ADPD4k
 *
 * @param[in]           p_pkt: input m2m2 packet
 *
 * @return              pointer to reponse m2m2 packet
 *****************************************************************************/
static m2m2_hdr_t *set_adpd4000_fs(m2m2_hdr_t *p_pkt) {
  M2M2_APP_COMMON_STATUS_ENUM_t status = M2M2_APP_COMMON_STATUS_ERROR;
  /* Declare a pointer to access the input packet payload */
  PYLD_CST(p_pkt, m2m2_sensor_adpd4000_set_fs_t, p_in_payload);
  /* Allocate a response packet
     with space for the correct number of operations */
  PKT_MALLOC(p_resp_pkt, m2m2_sensor_adpd4000_set_fs_t, 0);
  if (NULL != p_resp_pkt) {
    PYLD_CST(p_resp_pkt, m2m2_sensor_adpd4000_set_fs_t, p_resp_payload);

    if (Set_adpd4000_SamplingFreq(p_in_payload->odr) ==
        ADPD4000_DCFG_STATUS_OK) {
      status = M2M2_APP_COMMON_STATUS_OK;
    } else {
      status = M2M2_APP_COMMON_STATUS_ERROR;
    }

    p_resp_payload->command =
        (M2M2_SENSOR_ADPD_COMMAND_ENUM_t)M2M2_SENSOR_ADPD_COMMAND_SET_FS_RESP;

    p_resp_payload->odr = p_in_payload->odr;
    p_resp_pkt->dest = p_pkt->src;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_payload->status = status;
  }
  return p_resp_pkt;
}

/*!
 ****************************************************************************
 * @brief Get the ADPD4k Chip ID
 *
 * @param[in]   - None
 *
 * @return      - chipID
 *****************************************************************************/
uint16_t GetChipIdAdpd() {
  uint16_t nDevId = 0;
  /* 0xC0 - ADPD4000, 0x1C2 - ADPD4100 MLW*/
  Adpd400xDrvRegRead(ADPD400x_REG_CHIP_ID, &nDevId);
  return nDevId;
}

#ifdef ENABLE_SQI_APP
/*!
 ****************************************************************************
 * @brief Feed ADPD Data externally from logs
 *
 * @param[in]           p_pkt: input m2m2 packet
 *
 * @return              pointer to reponse m2m2 packet
 *****************************************************************************/
static m2m2_hdr_t *adpd_app_ext_adpd_datastream(m2m2_hdr_t *p_pkt) {

  // Declare a pointer to access the input packet payload
  PYLD_CST(p_pkt, adpd_ext_data_stream_t, p_in_payload);
  send_sqi_app_data(&p_in_payload->data,p_in_payload->timestamp);
  return NULL;

}
#endif

#ifdef ENABLE_SQI_APP
/*!
 ****************************************************************************
 * @brief Set ODR of externally fed ADPD data
 *
 * @param[in]           p_pkt: input m2m2 packet
 *
 * @return              pointer to reponse m2m2 packet
 *****************************************************************************/
static m2m2_hdr_t *adpd_app_set_ext_adpd_datastream_odr(m2m2_hdr_t *p_pkt) {

  /* Declare a pointer to access the input packet payload */
  PYLD_CST(p_pkt, adpd_ext_data_stream_odr_t, p_in_payload);
  PKT_MALLOC(p_resp_pkt, adpd_ext_data_stream_odr_t, 0);

  if (NULL != p_resp_pkt) {
    PYLD_CST(p_resp_pkt, adpd_ext_data_stream_odr_t, p_resp_payload);
    /* set adpd external data streaming flag to true */
    gAdpd_ext_data_stream_active = true;
    /* set adpd external data stream odr */
    gAdpd_ext_data_stream_odr = p_in_payload->sampling_freq;
    p_resp_payload->sampling_freq = gAdpd_ext_data_stream_odr;

    p_resp_payload->status =  M2M2_APP_COMMON_STATUS_OK;
    p_resp_payload->command = \
      (M2M2_SENSOR_ADPD_COMMAND_ENUM_t)M2M2_SENSOR_ADPD_COMMAND_SET_EXT_DATA_STREAM_ODR_RESP;
    p_resp_pkt->dest = p_pkt->src;
    p_resp_pkt->src = p_pkt->dest;
  }

  return p_resp_pkt;
}
#endif
/*!
 ****************************************************************************
 * @brief Reset AGC related flags
 *
 * @param[in]           None
 *
 * @return              None
 *****************************************************************************/
void reset_agc_flags(void)
{
    gn_led_slot_g = 0;
    gn_led_slot_r = 0;
    gn_led_slot_ir = 0;
    gn_led_slot_b = 0;
    gn_agc_active_slots = 0;

    gRun_agc = false;
   /****************************/
}

#ifdef ENABLE_PPG_APP
/*!
 ****************************************************************************
 * @brief Update PPG settings for current and gain from lcfg if static agc disabled
 *
 * @param[in]    slot_id: specify slot used for PPG
                          (Default Slot-F, Slot_id = 5)
 *
 * @return       None
 *****************************************************************************/
static void update_ppg_default_current_gain(uint8_t slot_id){
uint32_t ReadLCFGvalue;
uint16_t nTIAgain = 0x0000;
uint16_t nTsCtrl = 0x0000;
uint8_t nCh2Enable = 0x00;
uint16_t CurrRegVal,nRegValue;

g_adpd_reg_base = log2(gAdpd400x_lcfg->targetSlots) * ADPD400x_SLOT_BASE_ADDR_DIFF;
MwPPG_ReadLCFG(32, &ReadLCFGvalue);/*! To read InitialCurrentTiaGain */
uint8_t nCurrVal = ReadLCFGvalue >> 8;// getting MSB 8bit
uint8_t nGainValue = ReadLCFGvalue & 0x003F; // previous Gain value will be maintained if the lcfg value not valid
Adpd400xDrvSetOperationMode(ADPD400xDrv_MODE_IDLE);

if(nCurrVal != 0){
  CurrRegVal = Adpd400xUtilGetRegFromMonotonicCurrent(nCurrVal);
  Adpd400xDrvRegRead(ADPD400x_REG_LED_POW12_A + g_adpd_reg_base, &nRegValue);
  nRegValue = nRegValue & (~BITM_LED_POW12_X_LED_CURRENT1_X);
  nRegValue |= CurrRegVal << BITP_LED_POW12_X_LED_CURRENT1_X;
  Adpd400xDrvRegWrite(ADPD400x_REG_LED_POW12_A + g_adpd_reg_base, nRegValue);
}
  /* check if ch2 is enabled */
Adpd400xDrvRegRead(ADPD400x_REG_TS_CTRL_A + g_adpd_reg_base, &nTsCtrl);
nCh2Enable = (nTsCtrl & BITM_TS_CTRL_A_CH2_EN_A) >> BITP_TS_CTRL_A_CH2_EN_A;
Adpd400xDrvRegRead(ADPD400x_REG_AFE_TRIM_A + g_adpd_reg_base, &nTIAgain);
if((nGainValue & BITM_AFE_TRIM_X_TIA_GAIN_CH1_X) <= CH1_GAIN_MAX_REG_VAL){//the maximum value of ch1 gain settings will be 4
nTIAgain = nTIAgain & (~BITM_AFE_TRIM_X_TIA_GAIN_CH1_X);
nTIAgain |= (nGainValue & BITM_AFE_TRIM_X_TIA_GAIN_CH1_X) << BITP_AFE_TRIM_X_TIA_GAIN_CH1_X;
}
if(nCh2Enable)
{
  if((nGainValue & BITM_AFE_TRIM_X_TIA_GAIN_CH2_X) <= CH2_GAIN_MAX_REG_VAL){ //the maximum value of ch2 gain settings will be 32
  nTIAgain = nTIAgain & (~BITM_AFE_TRIM_X_TIA_GAIN_CH2_X);
  nTIAgain |= (nGainValue & BITM_AFE_TRIM_X_TIA_GAIN_CH2_X);
  }
}
Adpd400xDrvRegWrite(ADPD400x_REG_AFE_TRIM_A + g_adpd_reg_base, nTIAgain);
Adpd400xDrvSetOperationMode(ADPD400xDrv_MODE_SAMPLE);
}
#endif
/*!
 ****************************************************************************
 * @brief update AGC structure with ADPD device info
 *
 * @param[in]           AGCState_t - AGC structure
 * @param[in]           slotnum - ADPD slot number
 *
 * @return              None
 *****************************************************************************/
static void Adpd400xUpdateStaticAgcInfo(AGCState_t *pnagcInfo,uint8_t slotnum){
  uint16_t nRegValue = 0;
  pnagcInfo->setting[0] = g_state_static_agc.agc_log_state;
  g_adpd_reg_base = slotnum * ADPD400x_SLOT_BASE_ADDR_DIFF;
  AdpdDrvRegRead(g_adpd_reg_base + ADPD400x_REG_LED_POW12_A, &nRegValue);
  pnagcInfo->setting[1] = nRegValue;
  AdpdDrvRegRead(g_adpd_reg_base + ADPD400x_REG_LED_POW34_A, &nRegValue);
  pnagcInfo->setting[2] = nRegValue;
  AdpdDrvRegRead(g_adpd_reg_base + ADPD400x_REG_COUNTS_A, &nRegValue);
  pnagcInfo->setting[3] = nRegValue;
  AdpdDrvRegRead(g_adpd_reg_base + ADPD400x_REG_AFE_TRIM_A, &nRegValue);
  pnagcInfo->setting[4] = nRegValue;
  pnagcInfo->setting[5] = get_adpd_odr();
  pnagcInfo->setting[6] = slotnum;
  pnagcInfo->mts[0] = 0xFFFF;// No signal quality check for static AGC calibration and saturation case
  pnagcInfo->setting[9] = 0x0000;
}

/**
* @brief Packetize and send static AGC data
*
* @return None
*
*/
static void packetize_adpd_static_agc_settings(void) {
  ADI_OSAL_STATUS err;
  AGCState_t agcInfo;
  uint16_t nHighestSlotActive = 0;
  if((g_state_static_agc.num_subs > 0) && (g_state_static_agc.skip_samples >= SKIP_STATIC_AGC_SAMPLES)) {
   /* Highest active slot */
    if(g_state_static_agc.active_slots != 0){
      Adpd400xDrvGetParameter(ADPD400x_HIGHEST_SLOT_NUM, 0, &nHighestSlotActive);
      for(uint8_t slot = 0; slot <= nHighestSlotActive; slot++){
        if (g_state_static_agc.active_slots & (0x01 << slot)){
          memset(&agcInfo,0,sizeof(agcInfo));
          Adpd400xUpdateStaticAgcInfo(&agcInfo,slot);
          adi_osal_EnterCriticalRegion();
          g_state_static_agc.agc_pktizer.p_pkt = post_office_create_msg(sizeof(m2m2_sensor_adpd_static_agc_stream_t) + M2M2_HEADER_SZ);
          if(g_state_static_agc.agc_pktizer.p_pkt != NULL){
            PYLD_CST(g_state_static_agc.agc_pktizer.p_pkt, m2m2_sensor_adpd_static_agc_stream_t, p_payload_ptr);
            p_payload_ptr->command = M2M2_SENSOR_COMMON_CMD_STREAM_DATA;
            p_payload_ptr->status = (M2M2_SENSOR_INTERNAL_STATUS_ENUM_t)M2M2_SENSOR_INTERNAL_STATUS_PKT_READY;
            p_payload_ptr->timestamp = get_sensor_time_stamp(); // Since nand flash log start command given at last , capturing TS before that will create negative delta in epoch calculation
            for(int i = 0; i < 6; i++)
            {
             p_payload_ptr->mts[i] = agcInfo.mts[i];
            }
            for(int i = 0; i < 10; i++)
            {
             p_payload_ptr->setting[i] = agcInfo.setting[i];
            }
            g_state_static_agc.agc_pktizer.p_pkt->src = M2M2_ADDR_SENSOR_ADPD4000;
            g_state_static_agc.agc_pktizer.p_pkt->dest = M2M2_ADDR_SYS_STATIC_AGC_STREAM;
            p_payload_ptr->sequence_num = g_state_static_agc.data_pkt_seq_num++;
            post_office_send(g_state_static_agc.agc_pktizer.p_pkt, &err);
            g_state_static_agc.agc_pktizer.p_pkt = NULL;
          }
          adi_osal_ExitCriticalRegion(); /* exiting critical region even if mem_alloc fails*/
        }
      }
    }
    g_state_static_agc.active_slots = 0;
  }else{
    g_state_static_agc.skip_samples++;
  }
}

/**
* @brief Clear the static AGC stream state variables
*
* @return None
*
*/
static void static_agc_stream_state_reset(void){
  g_state_static_agc.skip_samples = 0;
  g_state_static_agc.data_pkt_seq_num = 0;
  g_state_static_agc.agc_log_state = ADPD400x_AGCLOG_STATIC_AGC_INVALID; /* clear AGC state*/
  g_state_static_agc.active_slots = 0;
}

#ifdef ENABLE_DEBUG_STREAM

/**
* @brief Constructs the packet to send the ADPD debug data
*
* @return None
*
*/
void packetize_adpd_debug_data(void) {
  ADI_OSAL_STATUS         err;
  if(g_adpd_debug_data.num_subs > 0){
    adi_osal_EnterCriticalRegion();
    g_adpd_debug_data.p_pkt = post_office_create_msg(sizeof(m2m2_app_debug_stream_t) + M2M2_HEADER_SZ);
    if(g_adpd_debug_data.p_pkt != NULL){
      PYLD_CST(g_adpd_debug_data.p_pkt, m2m2_app_debug_stream_t,p_payload_ptr);
      p_payload_ptr->command = M2M2_SENSOR_COMMON_CMD_STREAM_DATA;
      p_payload_ptr->status = (M2M2_SENSOR_INTERNAL_STATUS_ENUM_t)M2M2_SENSOR_INTERNAL_STATUS_PKT_READY;
      p_payload_ptr->timestamp = get_sensor_time_stamp();
      memset(p_payload_ptr->debuginfo, 0, M2M2_DEBUG_INFO_SIZE*4);
      for (uint8_t i = 0; i < M2M2_DEBUG_INFO_SIZE; i++){
        p_payload_ptr->debuginfo[i] = g_adpd_debugInfo[i];
      }
      for (uint8_t j = 0; j < M2M2_DEBUG_INFO_SIZE; j++){
        p_payload_ptr->debuginfo_64[j] = g_adpd_rtc_info[j];
      }
      memset(g_adpd_debugInfo, 0, sizeof(g_adpd_debugInfo));
      g_adpd_debug_data.p_pkt->src = M2M2_ADDR_SENSOR_ADPD4000;
      g_adpd_debug_data.p_pkt->dest = M2M2_ADDR_SYS_DBG_STREAM;
      p_payload_ptr->sequence_num = g_adpd_debug_data.data_pkt_seq_num++;

      post_office_send(g_adpd_debug_data.p_pkt, &err);
      g_adpd_debug_data.p_pkt = NULL;
    }
    adi_osal_ExitCriticalRegion(); /* exiting critical region even if mem_alloc fails*/
  }
}
#endif