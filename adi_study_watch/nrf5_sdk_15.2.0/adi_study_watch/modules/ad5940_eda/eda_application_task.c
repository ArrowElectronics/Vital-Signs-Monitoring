/**
***************************************************************************
* @addtogroup Tasks
* @{
* @file         eda_application_task.c
* @author       ADI
* @version      V1.0.0
* @date         26-June-2019
* @brief        Source file contains EDA processing wrapper.
***************************************************************************
* @attention
***************************************************************************
*/
/*!
*  copyright Analog Devices
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
#ifdef ENABLE_EDA_APP
#include "eda_application_task.h"
#include <adpd4000_dcfg.h>
#include <ecg_task.h>
#include <includes.h>
#include <limits.h>
#include <power_manager.h>
#include <stdint.h>
#include <string.h>
#include "us_tick.h"

#ifdef VSM_MBOARD
#include "watch_vsm_motherboard_pin_config.h"
#elif PCBA
#include "watch_board_pcba_pin_config.h"
#else
#include "watch_board_evt_pin_config.h"
#endif
#include "nrf_drv_gpiote.h"

#include "nrf_log_ctrl.h"

/////////////////////////////////////////
#ifdef DCB
#include <adi_dcb_config.h>
#include <dcb_interface.h>
static volatile bool g_eda_dcb_Present = false;
bool eda_get_dcb_present_flag(void);
void eda_set_dcb_present_flag(bool set_flag);
EDA_ERROR_CODE_t read_eda_dcb(uint32_t *eda_dcb_data, uint16_t *read_size);
EDA_ERROR_CODE_t write_eda_dcb(uint32_t *eda_dcb_data, uint16_t write_Size);
EDA_ERROR_CODE_t delete_eda_dcb(void);
#endif
/////////////////////////////////////////

#define ADMITANCE_CARTESIAN_FORM

/* EDA app Task Module Log settings */
#define NRF_LOG_MODULE_NAME EDA_App_Task

#if EDA_APP_CONFIG_LOG_ENABLED
#ifdef NRF_LOG_DEFAULT_LEVEL
#undef NRF_LOG_DEFAULT_LEVEL
#define NRF_LOG_DEFAULT_LEVEL EDA_APP_CONFIG_LOG_LEVEL
#endif
#define NRF_LOG_INFO_COLOR EDA_APP_CONFIG_INFO_COLOR
#define NRF_LOG_DEBUG_COLOR EDA_APP_CONFIG_DEBUG_COLOR
#else /* EDA_TASK_CONFIG_LOG_ENABLED */
#define NRF_LOG_LEVEL 0
#endif /* EDA_TASK_CONFIG_LOG_ENABLED */
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

uint32_t temp;
#ifdef DEBUG_EDA
static uint32_t pkt_count = 0;
#endif

#ifdef PROFILE_TIME_ENABLED
static float eda_odr = 0, time_elapsed_for_eda = 0, eda_start_time = 0;
#endif

static _gEdaAd5940Data_t gEdaData = {{0}, 0, 0, 0};
extern uint32_t FifoCount;
extern uint64_t nTcv; /* timestamp */
uint32_t ResistorForBaseline = 0;
uint32_t UserConfig_BaselineResistor = 0;
float magnitude, phase;
static int32_t SignalData[MAX_NUM_OF_FFT_POINTS] = {0}, eda_counter = 0;
uint8_t measurement_cycle_completed = 0;
uint8_t init_flag;
extern AD5940_APP_ENUM_t gnAd5940App;
extern volatile uint8_t gnAd5940DataReady;
extern uint8_t gnAd5940TimeIrqSet;
extern uint8_t gnAd5940TimeIrqSet1;

g_state_eda_t g_state_eda;
volatile bool gEdaAppInitFlag = false;
#ifdef EXTERNAL_TRIGGER_EDA
uint8_t eda_start_req=0;
#endif
AppEDACfg_Type AppEDACfg;
volatile int16_t eda_user_applied_odr = 0;
volatile int16_t eda_user_applied_dftnum = 0;
volatile int16_t eda_user_applied_rtia_cal = 0;
volatile int16_t eda_user_applied_baseline_imp = 0;

uint8_t rtia_cal_completed = 0;
extern uint32_t AppBuff[APPBUFF_SIZE];

/*Base resistor used for measurements is 20kOhm*/

fImpCar_Type EDABase_Dft16 = {
      .Real = 29079.223,
      .Image = 29079.223,
  };

fImpCar_Type EDABase_Dft8 = {
      .Real = 26012.926,
      .Image = 26000.182,
  };

fImpCar_Type EDABase_Userconfig_Dft16 = {
      .Real = 0,
      .Image =0,
  };

fImpCar_Type EDABase_Userconfig_Dft8 = {
      .Real = 0,
      .Image =0,
  };

#define USER_CONFIG_BASELINE_IMPEDANCE_RESET  0x00
#define USER_CONFIG_BASELINE_IMPEDANCE_SET    0x01
#define DEFAULT_BASELINE_RESISTOR             19940

#ifdef PROFILE_TIME_ENABLED
uint64_t delay_in_first_measurement;
uint64_t voltage_measurement_diff_time;
uint64_t current_measurement_diff_time;
uint64_t voltage_cycles_diff_time;
uint64_t current_cycles_diff_time;
extern uint64_t gnAd5940TimeCurVal;
extern uint64_t gnAd5940TimeCurValInMicroSec;
uint32_t eda_init_diff_time;
uint32_t eda_rtia_cal_diff_time;
uint32_t eda_init_start_time;
#endif

typedef m2m2_hdr_t *(app_cb_function_t)(m2m2_hdr_t *);

const char GIT_EDA_VERSION[] = "TEST EDA_VERSION STRING";
const uint8_t GIT_EDA_VERSION_LEN = sizeof(GIT_EDA_VERSION);
static void packetize_eda_raw_data(
    int32_t *SignalData,  uint8_t index, eda_app_stream_t *pPkt, uint32_t timestamp);
static m2m2_hdr_t *eda_app_reg_access(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *eda_app_stream_config(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *eda_app_status(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *eda_app_get_version(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *eda_app_decimation(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *eda_app_dynamic_scaling(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *eda_app_dft_num_set(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *eda_app_baseline_imp_set(m2m2_hdr_t *p_pkt);
#ifdef DEBUG_EDA
static m2m2_hdr_t *eda_app_debug_info(m2m2_hdr_t *p_pkt);
#endif
static m2m2_hdr_t *eda_app_perform_rtia_calibration(m2m2_hdr_t *p_pkt);
#ifdef DCB
static m2m2_hdr_t *eda_app_set_dcb_lcfg(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *eda_dcb_command_read_config(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *eda_dcb_command_write_config(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *eda_dcb_command_delete_config(m2m2_hdr_t *p_pkt);
#endif
#ifdef AD5940_REG_ACCESS
static m2m2_hdr_t *ad5940_app_reg_access(m2m2_hdr_t *p_pkt);
#endif
static void fetch_eda_data(void);
static void sensor_eda_task(void *pArgument);
void Enable_ephyz_power(void);
uint32_t ad5940_port_deInit(void);

uint8_t sensor_eda_task_stack[APP_OS_CFG_EDA_TASK_STK_SIZE];
ADI_OSAL_THREAD_HANDLE sensor_eda_task_handler;
ADI_OSAL_STATIC_THREAD_ATTR sensor_eda_task_attributes;
StaticTask_t edaTaskTcb;
ADI_OSAL_SEM_HANDLE eda_task_evt_sem;

/*!
 * @brief:  The dictionary type for mapping addresses to callback handlers.
 */
typedef struct _app_routing_table_entry_t {
  uint8_t command;
  app_cb_function_t *cb_handler;
} app_routing_table_entry_t;

app_routing_table_entry_t eda_app_routing_table[] = {
    {M2M2_APP_COMMON_CMD_STREAM_START_REQ, eda_app_stream_config},
    {M2M2_APP_COMMON_CMD_STREAM_STOP_REQ, eda_app_stream_config},
    {M2M2_APP_COMMON_CMD_STREAM_SUBSCRIBE_REQ, eda_app_stream_config},
    {M2M2_APP_COMMON_CMD_STREAM_UNSUBSCRIBE_REQ, eda_app_stream_config},
    {M2M2_APP_COMMON_CMD_SENSOR_STATUS_QUERY_REQ, eda_app_status},
    {M2M2_APP_COMMON_CMD_READ_LCFG_REQ, eda_app_reg_access},
    {M2M2_APP_COMMON_CMD_WRITE_LCFG_REQ, eda_app_reg_access},
    {M2M2_SENSOR_COMMON_CMD_GET_STREAM_DEC_FACTOR_REQ, eda_app_decimation},
    {M2M2_SENSOR_COMMON_CMD_SET_STREAM_DEC_FACTOR_REQ, eda_app_decimation},
    {M2M2_APP_COMMON_CMD_GET_VERSION_REQ, eda_app_get_version},
    {M2M2_EDA_APP_CMD_DYNAMIC_SCALE_REQ, eda_app_dynamic_scaling},
    {M2M2_EDA_APP_CMD_RTIA_CAL_REQ, eda_app_perform_rtia_calibration},
#ifdef DEBUG_EDA
    {M2M2_EDA_APP_CMD_REQ_DEBUG_INFO_REQ, eda_app_debug_info},
#endif
    {M2M2_EDA_APP_CMD_SET_DFT_NUM_REQ, eda_app_dft_num_set},
    {M2M2_EDA_APP_CMD_BASELINE_IMP_SET_REQ,eda_app_baseline_imp_set},
    {M2M2_EDA_APP_CMD_BASELINE_IMP_RESET_REQ,eda_app_baseline_imp_set},
#ifdef DCB
    {M2M2_APP_COMMON_CMD_SET_LCFG_REQ, eda_app_set_dcb_lcfg},
    {M2M2_DCB_COMMAND_READ_CONFIG_REQ, eda_dcb_command_read_config},
    {M2M2_DCB_COMMAND_WRITE_CONFIG_REQ, eda_dcb_command_write_config},
    {M2M2_DCB_COMMAND_ERASE_CONFIG_REQ, eda_dcb_command_delete_config},
#endif
#ifdef AD5940_REG_ACCESS
    {M2M2_SENSOR_COMMON_CMD_READ_REG_16_REQ, ad5940_app_reg_access},
    {M2M2_SENSOR_COMMON_CMD_WRITE_REG_16_REQ, ad5940_app_reg_access},
#endif
};

#define EDA_APP_ROUTING_TBL_SZ                                                 \
  (sizeof(eda_app_routing_table) / sizeof(eda_app_routing_table[0]))

ADI_OSAL_QUEUE_HANDLE eda_task_msg_queue = NULL;

#ifdef USER0_CONFIG_APP
#include "user0_config_app_task.h"
#include "app_timer.h"
#include "low_touch_task.h"
APP_TIMER_DEF(m_eda_timer_id);     /**< Handler for repeated timer for EDA. */
static void eda_timer_start(void);
static void eda_timer_stop(void);
static void eda_timeout_handler(void * p_context);
void start_eda_app_timer();
static user0_config_app_timing_params_t eda_app_timings = {0};
static volatile uint8_t gnDoEdaStart = 0; //Flag to do EDAInit() from task context
static volatile uint8_t gnDoEdaStop = 0;  //Flag to do EDADeInit() from task context

/**@brief Function for the Timer initialization.
*
* @details Initializes the timer module. This creates and starts application timers.
*
* @param[in]  None
*
* @return     None
*/
static void eda_timer_init(void)
{
    ret_code_t err_code;

    /*! Create timers */
    err_code =  app_timer_create(&m_eda_timer_id, APP_TIMER_MODE_REPEATED, eda_timeout_handler);

    APP_ERROR_CHECK(err_code);
}

/**@brief   Function for starting application timers.
* @details Timers are run after the scheduler has started.
*
* @param[in]  None
*
* @return     None
*/
static void eda_timer_start(void)
{
    /*! Start repeated timer */
    ret_code_t err_code = app_timer_start(m_eda_timer_id, APP_TIMER_TICKS(TIMER_ONE_SEC_INTERVAL), NULL);
    APP_ERROR_CHECK(err_code);

    eda_app_timings.check_timer_started = true;
}

/**@brief   Function for stopping the application timers.
*
* @param[in]  None
*
* @return     None
*/
static void eda_timer_stop(void)
{
    /*! Stop the repeated timer */
    ret_code_t err_code = app_timer_stop(m_eda_timer_id);
    APP_ERROR_CHECK(err_code);

    eda_app_timings.check_timer_started = false;
}

/**@brief   Callback Function for application timer events
*
* @param[in]  p_context: pointer to the callback function arguments
*
* @return     None
*/
static void eda_timeout_handler(void * p_context)
{
    if(eda_app_timings.delayed_start && eda_app_timings.start_time>0)
    {
      eda_app_timings.start_time_count++; /*! Increment counter every sec., till it is equal to start_time Value in seconds. */
      if(eda_app_timings.start_time_count == eda_app_timings.start_time)
      {
        //delayed start time expired-turn ON EDA
        gnDoEdaStart = 1; //Do EDAInit() from task context
        adi_osal_SemPost(eda_task_evt_sem);//Intimate EDA task
        eda_app_timings.delayed_start = false;
        eda_app_timings.start_time_count = 0;
        eda_app_timings.app_mode_on = true;
      }
      return;
    }

    if(eda_app_timings.app_mode_on && eda_app_timings.on_time>0)
    {
        eda_app_timings.on_time_count++; /*! Increment counter every sec. incase of ADPD ON, till it is equal to Ton Value in seconds. */
        if(eda_app_timings.on_time_count == eda_app_timings.on_time)
        {
          //on timer expired - turn off EDA
          if (g_state_eda.num_starts == 1) {
              gnDoEdaStop = 1;//Do EDADeInit() from task context
              adi_osal_SemPost(eda_task_evt_sem);//Intimate EDA task
          }
          eda_app_timings.app_mode_on = false;
          eda_app_timings.on_time_count = 0;
        }
    }
    else if(!eda_app_timings.app_mode_on && eda_app_timings.off_time>0)
    {
      eda_app_timings.off_time_count++; /*! Increment counter every sec. incase of ADPD OFF, till it is equal to Toff Value in seconds.*/
      if(eda_app_timings.off_time_count == eda_app_timings.off_time)
        {
           //off timer expired - turn on EDA
           if (g_state_eda.num_starts == 1) {
               gnDoEdaStart = 1;
               adi_osal_SemPost(eda_task_evt_sem);//Intimate EDA task
           }
           eda_app_timings.app_mode_on = true;
           eda_app_timings.off_time_count = 0;
        }
    }
}

/**@brief   Function to start EDA app timer
* @details  This is used in either interval based/intermittent LT mode3
*
* @param[in]  None
*
* @return     None
*/
void start_eda_app_timer()
{
  if(eda_app_timings.on_time > 0)
  {
    eda_timer_start();
  }

}
#endif//USER0_CONFIG_APP

/*!
 ****************************************************************************
 *@brief      Initialize configurations for eda application
 *@param      None
 *@return     None
 ******************************************************************************/
void InitCfg() {
  AppEDACfg.bParaChanged = bTRUE;
  AppEDACfg.SeqStartAddr = 0;
  AppEDACfg.MaxSeqLen = 0;
  AppEDACfg.SeqStartAddrCal = 0;
  AppEDACfg.MaxSeqLenCal = 0;
  /* Application related parameters */
  AppEDACfg.bBioElecBoard = bTRUE;
  AppEDACfg.ReDoRtiaCal = bFALSE;
  AppEDACfg.SysClkFreq = 16000000.0;
  AppEDACfg.LfoscClkFreq = 32000.0;
  AppEDACfg.AdcClkFreq = 16000000.0;
  AppEDACfg.FifoThresh = 4;
  AppEDACfg.NumOfData = -1;
  AppEDACfg.VoltCalPoints = 8;
  AppEDACfg.RcalVal = 10000.0;          /* 10kOhm */
  AppEDACfg.SinFreq = 100.0;            /* 100Hz */
  AppEDACfg.SampleFreq = 400.0;         /* 400Hz */
  AppEDACfg.SinAmplitude = 1100.0f / 2; /* 1100mV peak */
  AppEDACfg.DacUpdateRate = 7;
  AppEDACfg.HanWinEn = bTRUE;
  AppEDACfg.RtiaIndexCurr = 0;
  AppEDACfg.RtiaIndexNext = 0;
  AppEDACfg.bChangeRtia = bFALSE;
  /* private variables */
  AppEDACfg.SeqPatchInfo.BuffLen = 32;
  AppEDACfg.SeqPatchInfo.pSeqCmd = NULL;
  AppEDACfg.ImpEDABase.Real = 0;
  AppEDACfg.ImpEDABase.Image = 0;
  AppEDACfg.ImpSum.Real = 0;
  AppEDACfg.ImpSum.Real = 0;
  AppEDACfg.EDAInited = bFALSE;
  AppEDACfg.StopRequired = bFALSE;
  AppEDACfg.bRunning = bFALSE;
  AppEDACfg.bMeasVoltReq = bFALSE;
  AppEDACfg.EDAStateCurr = EDASTATE_INIT;
  AppEDACfg.EDAStateNext = EDASTATE_INIT;
};

/* magnitude and phase */
/*!
 *************************************************************************************
 *@brief       Calculate EDA Impedance
 *@param       pData: Pointer to data buffer holding information about Impedance
               DataCount: data count/ fifo count
 *@return      AD5940Err
 ************************************************************************************/
fImpCar_Type *pImp;
AD5940Err EDACalculateImpedance(void *pData) {
  /* Process data */
  pImp = (fImpCar_Type *)pData;
  /* Process data */
  uint8_t i =0;
  fImpCar_Type res;
  res = pImp[i];
  /* Show the real result of impedance under test(between F+/S+) */
  res.Real += ResistorForBaseline;
#if defined(IMPEDANCE_POLAR_FORM)
  gEdaData.edaImpedenc.real = AD5940_ComplexMag(&res);
  gEdaData.edaImpedence.img = AD5940_ComplexPhase(&res) * 180 / MATH_PI;
#elif defined(IMPEDANCE_CARTESIAN_FORM)
  gEdaData.edaImpedence.real = res.Real;
  gEdaData.edaImpedence.img = res.Image;
#elif defined(ADMITANCE_POLAR_FORM)
  res.Real = res.Real / (res.Real * res.Real + res.Image * res.Image);
  res.Image = -res.Image / (res.Real * res.Real + res.Image * res.Image);
  gEdaData.edaImpedence.real = AD5940_ComplexMag(&res);
  gEdaData.edaImpedence.img = AD5940_ComplexPhase(&res) * 180 / MATH_PI;
#else
  gEdaData.edaImpedence.real =
      (res.Real / (float)(res.Real * res.Real + res.Image * res.Image)) *
      1000;
  gEdaData.edaImpedence.img =
      (-res.Image / (float)(res.Real * res.Real + res.Image * res.Image)) *
      1000;
#endif

  return 0;
}
/*!
 ****************************************************************************
 *@brief       Eda Task initialization
 *@param       None
 *@return      None
 ******************************************************************************/
void ad5940_eda_task_init(void) {
  /* Initialize app state */
  g_state_eda.num_subs = 0;
  g_state_eda.num_starts = 0;
  /* Default behaviour is to send every packet */
  g_state_eda.decimation_factor = 1;
  g_state_eda.decimation_nsamples = 0;
  g_state_eda.data_pkt_seq_num = 0;

  ADI_OSAL_STATUS eOsStatus = ADI_OSAL_SUCCESS;
  sensor_eda_task_attributes.pThreadFunc = sensor_eda_task;
  sensor_eda_task_attributes.nPriority = APP_OS_CFG_EDA_TASK_PRIO;
  sensor_eda_task_attributes.pStackBase = &sensor_eda_task_stack[0];
  sensor_eda_task_attributes.nStackSize = APP_OS_CFG_EDA_TASK_STK_SIZE;
  sensor_eda_task_attributes.pTaskAttrParam = NULL;
  sensor_eda_task_attributes.szThreadName = "EDA Sensor";
  sensor_eda_task_attributes.pThreadTcb = &edaTaskTcb;

  eOsStatus = adi_osal_MsgQueueCreate(&eda_task_msg_queue, NULL, 5);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
    Debug_Handler();
  } else {
    update_task_queue_list(APP_OS_CFG_EDA_TASK_INDEX, eda_task_msg_queue);
  }
  eOsStatus = adi_osal_ThreadCreateStatic(
      &sensor_eda_task_handler, &sensor_eda_task_attributes);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
    Debug_Handler();
  }
  adi_osal_SemCreate(&eda_task_evt_sem, 0);
}

/*!
 ****************************************************************************
 *@brief      Called by the post office to send messages to this application
 *@param      p_pkt: packet of type m2m2_hdr_t
 *@return     None
 ******************************************************************************/
void send_message_ad5940_eda_task(m2m2_hdr_t *p_pkt) {
  ADI_OSAL_STATUS osal_result;
  osal_result = adi_osal_MsgQueuePost(eda_task_msg_queue, p_pkt);
  if (osal_result != ADI_OSAL_SUCCESS)
    post_office_consume_msg(p_pkt);
  adi_osal_SemPost(eda_task_evt_sem);
}

/*!
 ****************************************************************************
 *@brief      Eda Task
 *@param      None
 *@return     None
 ******************************************************************************/
static void sensor_eda_task(void *pArgument) {
  ADI_OSAL_STATUS err;
  post_office_add_mailbox(M2M2_ADDR_MED_EDA, M2M2_ADDR_MED_EDA_STREAM);

  /*Wait for FDS init to complete*/
  adi_osal_SemPend(eda_task_evt_sem, ADI_OSAL_TIMEOUT_FOREVER);

#ifdef USER0_CONFIG_APP
  eda_timer_init();
#endif

  while (1) {
    m2m2_hdr_t *p_in_pkt = NULL;
    m2m2_hdr_t *p_out_pkt = NULL;
    adi_osal_SemPend(eda_task_evt_sem, ADI_OSAL_TIMEOUT_FOREVER);
    p_in_pkt =
        post_office_get(ADI_OSAL_TIMEOUT_NONE, APP_OS_CFG_EDA_TASK_INDEX);
    if (p_in_pkt == NULL) {
#ifndef EXTERNAL_TRIGGER_EDA
       if(g_state_eda.num_starts != 0) /* Fetch data from device only if the device is in active state(initialized properly)*/
          fetch_eda_data();
#else
      /* No m2m2 messages to process, so fetch some data from the device */
      if(g_state_eda.num_starts != 0){ /* Fetch data from device only if the device is in active state(initialized properly)*/
           /* Now there should be fifo data ; number of samples to read is being set in
        eds app which FIFO threshold variable*/

          /* set ISR flags high */
          gnAd5940DataReady = 1;
          gnAd5940TimeIrqSet = 1;
          gnAd5940TimeIrqSet1 = 1;
          fetch_eda_data();
        }
#endif
        else{
          /* false trigger */
        }
#ifdef USER0_CONFIG_APP
    } if(gnDoEdaStart && (g_state_eda.num_starts == 1)) {
       gnDoEdaStart = 0;
       if(EDA_SUCCESS == EDAAppInit())
       {
       }
    } if(gnDoEdaStop && (g_state_eda.num_starts == 1)) {
       gnDoEdaStop = 0;
       if(EDA_SUCCESS == EDAAppDeInit())
       {
       }
    } if (p_in_pkt != NULL) {
#else
    } else {
#endif //USER0_CONFIG_APP
      /* Got an m2m2 message from the queue, process it */
      PYLD_CST(p_in_pkt, _m2m2_app_common_cmd_t, p_in_cmd);
      /* Look up the appropriate function to call in the function table */
      for (int i = 0; i < EDA_APP_ROUTING_TBL_SZ; i++) {
        if (eda_app_routing_table[i].command == p_in_cmd->command) {
          p_out_pkt = eda_app_routing_table[i].cb_handler(p_in_pkt);
          break;
        }
      }
      post_office_consume_msg(p_in_pkt);
      if (p_out_pkt != NULL) {
        post_office_send(p_out_pkt, &err);
        NRF_LOG_INFO("EDA status packets sent");
      }
    }
  }
}

/*!
 ****************************************************************************
 *@brief      Packetize eda data
 *@param      pSignalData: pointer to the eda data
 *@param      pPkt: pointer to the packet structure
 *@param      timestamp: timestamp of the packet
 *@return     None
 ******************************************************************************/
static void packetize_eda_raw_data(
    int32_t *SignalData, uint8_t index, eda_app_stream_t *pPkt, uint32_t timestamp) {
  /*   Calculate Impedance in form of Cartesian form */
  EDACalculateImpedance(&SignalData[index]);
  if (g_state_eda.eda_pktizer.packet_nsamples == 0) {
    g_state_eda.eda_pktizer.packet_max_nsamples =
        (sizeof(pPkt->eda_data) / sizeof(pPkt->eda_data[0]));
    pPkt->eda_data[0].timestamp = timestamp;
    pPkt->eda_data[0].realdata = (int16_t)(gEdaData.edaImpedence.real / 1000);
    pPkt->eda_data[0].imgdata = (int16_t)(gEdaData.edaImpedence.img / 1000);
    g_state_eda.eda_pktizer.packet_nsamples++;
  } else if (g_state_eda.eda_pktizer.packet_nsamples <
             g_state_eda.eda_pktizer.packet_max_nsamples) {
    /* one packet =  6 samples
     * first sample is added in above check and
     *  remaining 5 samples is added here */
    uint16_t i = g_state_eda.eda_pktizer.packet_nsamples;
    pPkt->eda_data[i].timestamp = timestamp;
    pPkt->eda_data[i].realdata = (int16_t)(gEdaData.edaImpedence.real / 1000);
    pPkt->eda_data[i].imgdata = (int16_t)(gEdaData.edaImpedence.img / 1000);
    g_state_eda.eda_pktizer.packet_nsamples++;
  }
}

/*!
 ****************************************************************************
 *@brief      Reset eda packetization
 *@param      None
 *@return     None
 ******************************************************************************/
static void reset_eda_packetization(void) {
  g_state_eda.eda_pktizer.packet_nsamples = 0;
}

/*!
 ****************************************************************************
 *@brief      Fetch eda data
 *@param      None
 *@return     None
 ******************************************************************************/
static void fetch_eda_data(void) {
  ADI_OSAL_STATUS err;
  uint32_t edaTS = 0;
  int8_t status = 0;
  //To hold real, imaginary data read from AD5940 fifo
  static uint32_t edaData[2] = {0};
  static eda_app_stream_t pkt;
  g_state_eda.eda_pktizer.p_pkt = NULL;
  temp = APPBUFF_SIZE;
  AppEDAISR(AppBuff, &temp);
  status = ad5940_buff_get((uint32_t *)&edaData, &edaTS);

  /* reading a pair of samples for impedance calculation */
  //SignalData[eda_counter++] = edaData;
  //if (eda_counter > FifoCount / 2) {
  //  eda_counter = 0;
  //}
  eda_counter = 0;

  //while ((status == AD5940Drv_SUCCESS) && (eda_counter == FifoCount / 2)) {
  while ((status == AD5940Drv_SUCCESS)) {
    SignalData[eda_counter++] = edaData[REAL_INDEX];//Real part
    SignalData[eda_counter++] = edaData[IMAG_INDEX];//Imaginary part
    g_state_eda.decimation_nsamples++;

    if (g_state_eda.decimation_nsamples >= g_state_eda.decimation_factor) {
      g_state_eda.decimation_nsamples = 0;
      packetize_eda_raw_data(SignalData, (eda_counter-RL_IM_PAIR), &pkt, edaTS);
      if (g_state_eda.eda_pktizer.packet_nsamples >=
          g_state_eda.eda_pktizer.packet_max_nsamples) {
        adi_osal_EnterCriticalRegion();
        g_state_eda.eda_pktizer.p_pkt =
            post_office_create_msg(sizeof(eda_app_stream_t) + M2M2_HEADER_SZ);
        if (g_state_eda.eda_pktizer.p_pkt != NULL) {
          PYLD_CST(
              g_state_eda.eda_pktizer.p_pkt, eda_app_stream_t, p_payload_ptr);
          g_state_eda.eda_pktizer.p_pkt->src = M2M2_ADDR_MED_EDA;
          g_state_eda.eda_pktizer.p_pkt->dest = M2M2_ADDR_MED_EDA_STREAM;
          memcpy(&p_payload_ptr->command, &pkt, sizeof(eda_app_stream_t));
          p_payload_ptr->command = M2M2_SENSOR_COMMON_CMD_STREAM_DATA;
          p_payload_ptr->status = 0x00;
          p_payload_ptr->datatype = M2M2_SENSOR_EDA_DATA;
          g_state_eda.eda_pktizer.p_pkt->src = M2M2_ADDR_MED_EDA;
          g_state_eda.eda_pktizer.p_pkt->dest = M2M2_ADDR_MED_EDA_STREAM;
          p_payload_ptr->sequence_num = g_state_eda.data_pkt_seq_num++;
#ifdef DEBUG_PKT
          post_office_msg_cnt(g_state_eda.eda_pktizer.p_pkt);
#endif
#ifdef USER0_CONFIG_APP
          if(eda_app_timings.app_mode_on)
          {
#endif //USER0_CONFIG_APP
          post_office_send(g_state_eda.eda_pktizer.p_pkt, &err);
#ifdef USER0_CONFIG_APP
          }//if(eda_app_timings.app_mode_on)
          else
            post_office_consume_msg(g_state_eda.eda_pktizer.p_pkt);
#endif //USER0_CONFIG_APP
#ifdef PROFILE_TIME_ENABLED
          time_elapsed_for_eda = MCU_HAL_GetTick() - eda_start_time;
#endif
#ifdef DEBUG_EDA
          pkt_count++;
          eda_odr = ((pkt_count * 6 * 1000) / time_elapsed_for_eda);
          NRF_LOG_INFO("TIME ELAPSED = " NRF_LOG_FLOAT_MARKER,
              NRF_LOG_FLOAT(time_elapsed_for_eda));
          NRF_LOG_INFO("START TIME = " NRF_LOG_FLOAT_MARKER,
              NRF_LOG_FLOAT(eda_start_time));
          NRF_LOG_INFO("EDA MEASURED ODR = " NRF_LOG_FLOAT_MARKER,
              NRF_LOG_FLOAT(eda_odr));
#endif
          g_state_eda.eda_pktizer.packet_nsamples = 0;
          g_state_eda.eda_pktizer.packet_max_nsamples = 0;
          g_state_eda.eda_pktizer.p_pkt = NULL;
        }
        adi_osal_ExitCriticalRegion(); /* exiting critical region even if
                                          mem_alloc fails*/
      }
    } /* Copy data from circular buffer data to soft buffer*/
    status = ad5940_buff_get((uint32_t *)&edaData, &edaTS);
    //SignalData[eda_counter++] = edaData;
  }
  measurement_cycle_completed = 0;
}

/*!
 ****************************************************************************
 *@brief      Get eda application version
 *@param      pPkt: pointer to the packet structure
 *@return     m2m2_hdr_t
 ******************************************************************************/
static m2m2_hdr_t *eda_app_get_version(m2m2_hdr_t *p_pkt) {
  /*  Allocate memory to response packet */
  PKT_MALLOC(p_resp_pkt, m2m2_app_common_version_t, 0);
  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_app_common_version_t, p_resp_payload);

    p_resp_payload->command = M2M2_APP_COMMON_CMD_GET_VERSION_RESP;
    p_resp_payload->major = 0x03;
    p_resp_payload->minor = 0x04;
    p_resp_payload->patch = 0x03;
    memcpy(&p_resp_payload->verstr[0], "EDA_App", 8);
    memcpy(&p_resp_payload->str[0], &GIT_EDA_VERSION, GIT_EDA_VERSION_LEN);
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  }
  return p_resp_pkt;
}

uint32_t const RtiaTable[] = {0, 110, 1000, 2000, 3000, 4000, 6000, 8000, 10000,
    12000, 16000, 20000, 24000, 30000, 32000, 40000, 48000, 64000, 85000, 96000,
    100000, 120000, 128000, 160000, 196000, 256000, 512000};

/*!
 ****************************************************************************
 *@brief      Perform RTIA calibration
 *@param      pPkt: pointer to the packet structure
 *@return     m2m2_hdr_t
 ******************************************************************************/
static m2m2_hdr_t *eda_app_perform_rtia_calibration(m2m2_hdr_t *p_pkt) {
  uint16_t num_cal_points = 0;
  uint8_t index = 0;

  /* Declare a pointer to the input packet payload */
  PYLD_CST(p_pkt, eda_app_perform_rtia_cal_t, p_in_payload);
  num_cal_points = p_in_payload->maxscale - p_in_payload->minscale + 1;

  /* Allocate a response packet with space for the correct number of operations
   */
  PKT_MALLOC(p_resp_pkt, eda_app_perform_rtia_cal_t,num_cal_points * sizeof(p_in_payload->rtia_cal_table_val[0]));
  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, eda_app_perform_rtia_cal_t, p_resp_payload);

    p_resp_payload->command = M2M2_EDA_APP_CMD_RTIA_CAL_RESP;
    p_resp_payload->status = M2M2_APP_COMMON_STATUS_OK;
    p_resp_payload->minscale = p_in_payload->minscale;
    p_resp_payload->maxscale = p_in_payload->maxscale;
    p_resp_payload->lowpowerrtia = p_in_payload->lowpowerrtia;

    /* LDO power on */
    adp5360_enable_ldo(ECG_LDO, true);


    /* Initilaize eda app configuration */
    InitCfg();

    AppEDACfg.RtiaAutoScaleMin = p_in_payload->minscale;
    AppEDACfg.RtiaAutoScaleMax = p_in_payload->maxscale;
    AppEDACfg.LptiaRtiaSel = p_in_payload->lowpowerrtia;
    rtia_cal_completed = 0;

    /* avoid re-init of eda configuration */
    init_flag=0;

    /* start eda sensor */
    ad5940_eda_start();

    /* Waiting for rtia calibration to get over */
    while (!rtia_cal_completed) {
      MCU_HAL_Delay(1000);
    }
    NRF_LOG_INFO("Calibration started");

    int i = AppEDACfg.RtiaAutoScaleMin;
    for (; i <= AppEDACfg.RtiaAutoScaleMax; i++) {
      p_resp_payload->rtia_cal_table_val[index].calibrated_res = (uint32_t)AD5940_ComplexMag(&AppEDACfg.RtiaCalTable[i]);
      p_resp_payload->rtia_cal_table_val[index].actual_res = RtiaTable[i];
      NRF_LOG_INFO("%d:%d",
          p_resp_payload->rtia_cal_table_val[index].actual_res,
          p_resp_payload->rtia_cal_table_val[index].calibrated_res);
      index++;
    }

    p_resp_payload->num_calibrated_values = num_cal_points;

    /* LDO power off */
    adp5360_enable_ldo(ECG_LDO, false);
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  } // if(NULL != p_resp_pkt)
  return p_resp_pkt;
}

/*!
 ******  **********************************************************************
 *@brief      Get eda application status (subscribers and start requests)
 *@param      pPkt: pointer to the packet structure
 *@return     m2m2_hdr_t
 ******************************************************************************/
static m2m2_hdr_t *eda_app_status(m2m2_hdr_t *p_pkt) {
  /* Declare and malloc a response packet */
  PKT_MALLOC(p_resp_pkt, m2m2_app_common_status_t, 0);
  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_app_common_status_t, p_resp_payload);

    p_resp_payload->command = M2M2_APP_COMMON_CMD_SENSOR_STATUS_QUERY_RESP;
    p_resp_payload->status = M2M2_APP_COMMON_STATUS_ERROR;
    if (g_state_eda.num_starts == 0) {
      p_resp_payload->status = M2M2_APP_COMMON_STATUS_STREAM_STOPPED;
    } else {
      p_resp_payload->status = M2M2_APP_COMMON_STATUS_STREAM_STARTED;
    }
  p_resp_payload->stream = M2M2_ADDR_MED_EDA_STREAM;
  p_resp_payload->num_subscribers = g_state_eda.num_subs;
  p_resp_payload->num_start_reqs = g_state_eda.num_starts;
  p_resp_pkt->src = p_pkt->dest;
  p_resp_pkt->dest = p_pkt->src;
  }
  return p_resp_pkt;
}

/*!
 ****************************************************************************
 *@brief      Set DFT number for eda application
 *@param      pPkt: pointer to the packet structure
 *@return     m2m2_hdr_t
 ******************************************************************************/
static m2m2_hdr_t *eda_app_dft_num_set(m2m2_hdr_t *p_pkt) {
  /* Declare a pointer to access the input packet payload */
  PYLD_CST(p_pkt, eda_app_set_dft_num_t, p_in_payload);

  /* Allocate memory to the response packet payload */
  PKT_MALLOC(p_resp_pkt, eda_app_set_dft_num_t, 0);
  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, eda_app_set_dft_num_t, p_resp_payload);

    /* Copy dft number from payload if valid setting  */
     /* supported dft numbers are 4,8,16,32 */
    if (p_in_payload->dftnum <= DFTNUM_64) {
      NRF_LOG_INFO("Valid dft num sel");
      eda_user_applied_dftnum = 1;
      AppEDACfg.DftNum = p_in_payload->dftnum;
    } else {
      /* default DFT number value */
      AppEDACfg.DftNum = DFTNUM_16;
    }

    p_resp_payload->command = M2M2_EDA_APP_CMD_SET_DFT_NUM_RESP;
    p_resp_payload->status = M2M2_APP_COMMON_STATUS_OK;
    p_resp_payload->dftnum = p_in_payload->dftnum;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  }
  return p_resp_pkt;
}

/*!
 ****************************************************************************
 *@brief      Set Baseline resistor used and measured impedance  for eda application
 *@param      pPkt: pointer to the packet structure
 *@return     m2m2_hdr_t
 ******************************************************************************/
static m2m2_hdr_t *eda_app_baseline_imp_set(m2m2_hdr_t *p_pkt) {
  /* Declare a pointer to access the input packet payload */
  PYLD_CST(p_pkt, eda_app_set_baseline_imp_t, p_in_payload);

  /* Allocate memory to the response packet payload */
  PKT_MALLOC(p_resp_pkt, eda_app_set_baseline_imp_t, 0);
  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, eda_app_set_baseline_imp_t, p_resp_payload);
    switch (p_in_payload->command) {
    case M2M2_EDA_APP_CMD_BASELINE_IMP_SET_REQ:
      EDABase_Userconfig_Dft16.Real  =  p_in_payload->imp_real_dft16;
      EDABase_Userconfig_Dft16.Image =  p_in_payload->imp_img_dft16;
      EDABase_Userconfig_Dft8.Real  =  p_in_payload->imp_real_dft8;
      EDABase_Userconfig_Dft8.Image =  p_in_payload->imp_img_dft8;
      UserConfig_BaselineResistor =  p_in_payload->resistor_baseline;
      eda_user_applied_baseline_imp = USER_CONFIG_BASELINE_IMPEDANCE_SET;

      p_resp_payload->command = M2M2_EDA_APP_CMD_BASELINE_IMP_SET_RESP;
      p_resp_payload->status = M2M2_APP_COMMON_STATUS_OK;
      p_resp_payload->imp_real_dft16 = EDABase_Userconfig_Dft16.Real;
      p_resp_payload->imp_img_dft16  = EDABase_Userconfig_Dft16.Image;
      p_resp_payload->imp_real_dft8 = EDABase_Userconfig_Dft8.Real;
      p_resp_payload->imp_img_dft8  = EDABase_Userconfig_Dft8.Image;

      p_resp_payload->resistor_baseline = UserConfig_BaselineResistor;

      break;
    case M2M2_EDA_APP_CMD_BASELINE_IMP_RESET_REQ:
      eda_user_applied_baseline_imp = USER_CONFIG_BASELINE_IMPEDANCE_RESET;
      EDABase_Userconfig_Dft16.Real  =  0x00;
      EDABase_Userconfig_Dft16.Image =  0x00;
      EDABase_Userconfig_Dft8.Real  =  0x00;
      EDABase_Userconfig_Dft8.Image =  0x00;
      UserConfig_BaselineResistor   = 0x00;

      p_resp_payload->command = M2M2_EDA_APP_CMD_BASELINE_IMP_RESET_RESP;
      p_resp_payload->status = M2M2_APP_COMMON_STATUS_OK;
      p_resp_payload->imp_real_dft16 = EDABase_Userconfig_Dft16.Real;
      p_resp_payload->imp_img_dft16  = EDABase_Userconfig_Dft16.Image;
      p_resp_payload->imp_real_dft8 = EDABase_Userconfig_Dft8.Real;
      p_resp_payload->imp_img_dft8  = EDABase_Userconfig_Dft8.Image;

      p_resp_payload->resistor_baseline = UserConfig_BaselineResistor;

      break;
    }

    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  }
  return p_resp_pkt;
}

/*!
 ****************************************************************************
 *@brief      Perform dynamic scaling
 *@param      pPkt: pointer to the packet structure
 *@return     m2m2_hdr_t
 ******************************************************************************/
static m2m2_hdr_t *eda_app_dynamic_scaling(m2m2_hdr_t *p_pkt) {
  PYLD_CST(p_pkt, eda_app_dynamic_scale_t, p_in_payload);

  /* allocate memory for response payload */
  PKT_MALLOC(p_resp_pkt, eda_app_dynamic_scale_t, 0);
  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, eda_app_dynamic_scale_t, p_resp_payload);
    p_resp_payload->command = M2M2_EDA_APP_CMD_DYNAMIC_SCALE_RESP;
    p_resp_payload->status = M2M2_APP_COMMON_STATUS_OK;


    /* rtia autoscale */
    AppEDACfg.RtiaAutoScaleEnable = p_in_payload->dscale ? bTRUE : bFALSE;
    if (AppEDACfg.RtiaAutoScaleEnable) {
      AppEDACfg.RtiaAutoScaleMin = p_in_payload->minscale;
      AppEDACfg.RtiaAutoScaleMax = p_in_payload->maxscale;
      AppEDACfg.LptiaRtiaSel = p_in_payload->lprtia;
      eda_user_applied_rtia_cal = 1;
    } else {
      AppEDACfg.LptiaRtiaSel = LPTIARTIA_100K;
      eda_user_applied_rtia_cal = 0;
    }
    NRF_LOG_INFO("autoscale=%d,autoscalemin=%d,autoscalemax=%d,lprtiasel=%d",
        AppEDACfg.RtiaAutoScaleEnable, AppEDACfg.RtiaAutoScaleMin,
        AppEDACfg.RtiaAutoScaleMax, AppEDACfg.LptiaRtiaSel);

    p_resp_payload->dscale = p_in_payload->dscale;
    p_resp_payload->minscale = p_in_payload->minscale;
    p_resp_payload->maxscale = p_in_payload->maxscale;
    p_resp_payload->lprtia = p_in_payload->lprtia;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  }
  return p_resp_pkt;
}

/*!
 ****************************************************************************
 *@brief      Eda stream START/STOP/SUBSCRIBE/UNSUBSCRIBE options
 *@param      pPkt: pointer to the packet structure
 *@return     m2m2_hdr_t
 ******************************************************************************/
static m2m2_hdr_t *eda_app_stream_config(m2m2_hdr_t *p_pkt) {
  M2M2_APP_COMMON_STATUS_ENUM_t status = M2M2_APP_COMMON_STATUS_ERROR;
  M2M2_APP_COMMON_CMD_ENUM_t command;
  int32_t nRetCode = EDA_SUCCESS;
  PYLD_CST(p_pkt, m2m2_app_common_sub_op_t, p_in_payload);
  PKT_MALLOC(p_resp_pkt, m2m2_app_common_sub_op_t, 0);
  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_app_common_sub_op_t, p_resp_payload);
    switch (p_in_payload->command) {
    case M2M2_APP_COMMON_CMD_STREAM_START_REQ:
      gnAd5940App = AD5940_APP_EDA;
      reset_eda_packetization();
      if (g_state_eda.num_starts == 0) {
#ifdef EXTERNAL_TRIGGER_EDA
        eda_start_req = 1; /* eda app to start */
#endif
#ifdef USER0_CONFIG_APP
      get_eda_app_timings_from_user0_config_app_lcfg(&eda_app_timings.start_time,\
                                        &eda_app_timings.on_time, &eda_app_timings.off_time);
      if(eda_app_timings.start_time > 0)
      {
        eda_app_timings.delayed_start = true;
      }
#ifdef LOW_TOUCH_FEATURE
      //EDA app not in continuous mode & its interval operation mode
      if(!is_eda_app_mode_continuous() && !(get_low_touch_trigger_mode3_status()))
#else
      //EDA app not in continuous mode
      if(!is_eda_app_mode_continuous())
#endif
      {
        start_eda_app_timer();
      }

      if(!eda_app_timings.delayed_start)
      {
        nRetCode = EDAAppInit();
        eda_app_timings.app_mode_on = true;
      }
      if(EDA_SUCCESS == nRetCode) {
#else
      if(EDA_SUCCESS == EDAAppInit()) {
#endif// USER0_CONFIG_APP
        //if (EDAAppInit() == EDA_SUCCESS) {
          g_state_eda.num_starts = 1;
#ifdef DEBUG_EDA
          eda_start_time = MCU_HAL_GetTick();
          pkt_count = 0;
#endif
          status = M2M2_APP_COMMON_STATUS_STREAM_STARTED;
        } else {
          status = M2M2_APP_COMMON_STATUS_STREAM_NOT_STARTED;
        }
      } else {
        g_state_eda.num_starts++;
        status = M2M2_APP_COMMON_STATUS_STREAM_IN_PROGRESS;
      }
      command = M2M2_APP_COMMON_CMD_STREAM_START_RESP;
      break;

    case M2M2_APP_COMMON_CMD_STREAM_STOP_REQ:
      if (g_state_eda.num_starts == 0) {
        status = M2M2_APP_COMMON_STATUS_STREAM_STOPPED;
      } else if (g_state_eda.num_starts == 1) {
#ifdef EXTERNAL_TRIGGER_EDA
        eda_start_req = 0;/* eda app to stop */
#endif
        if (EDAAppDeInit()) {
          g_state_eda.num_starts = 0;
          status = M2M2_APP_COMMON_STATUS_STREAM_STOPPED;
#ifdef USER0_CONFIG_APP
          if(eda_app_timings.check_timer_started)
          {
            eda_timer_stop();
            eda_app_timings.on_time_count = 0;
            eda_app_timings.off_time_count = 0;
            eda_app_timings.start_time_count = 0;
            eda_app_timings.delayed_start =  false;
          }
#endif//USER0_CONFIG_APP
        } else {
          g_state_eda.num_starts = 1;
          status = M2M2_APP_COMMON_STATUS_ERROR;
        }
      } else {
        g_state_eda.num_starts--;
        status = M2M2_APP_COMMON_STATUS_STREAM_COUNT_DECREMENT;
      }
      command = M2M2_APP_COMMON_CMD_STREAM_STOP_RESP;
      break;

    case M2M2_APP_COMMON_CMD_STREAM_SUBSCRIBE_REQ:
      g_state_eda.num_subs++;
      if(g_state_eda.num_subs == 1)
      {
         /* reset pkt sequence no. only during 1st sub request */
         g_state_eda.data_pkt_seq_num = 0;
      }
      post_office_setup_subscriber(
          M2M2_ADDR_MED_EDA, M2M2_ADDR_MED_EDA_STREAM, p_pkt->src, true);
      status = M2M2_APP_COMMON_STATUS_SUBSCRIBER_ADDED;
      command = M2M2_APP_COMMON_CMD_STREAM_SUBSCRIBE_RESP;
      break;
    case M2M2_APP_COMMON_CMD_STREAM_UNSUBSCRIBE_REQ:
      if (g_state_eda.num_subs <= 1) {
        g_state_eda.num_subs = 0;
        status = M2M2_APP_COMMON_STATUS_SUBSCRIBER_REMOVED;
      } else {
        g_state_eda.num_subs--;
        status = M2M2_APP_COMMON_STATUS_SUBSCRIBER_COUNT_DECREMENT;
      }
      post_office_setup_subscriber(
          M2M2_ADDR_MED_EDA, M2M2_ADDR_MED_EDA_STREAM, p_pkt->src, false);
      command = M2M2_APP_COMMON_CMD_STREAM_UNSUBSCRIBE_RESP;
      break;
    default:
      /* Something has gone horribly wrong.*/
      post_office_consume_msg(p_resp_pkt);
      return NULL;
    }
    p_resp_payload->command = command;
    p_resp_payload->status = status;
    p_resp_payload->stream = p_in_payload->stream;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  } // if(NULL != p_resp_pkt)
  return p_resp_pkt;
}

#ifdef PROFILE_TIME_ENABLED
uint32_t eda_de_init_diff_time;
#endif
/*!
 ****************************************************************************
 * @brief  Eda App Deinitialization
 * @param  None
 * @return ECG_ERROR_CODE_t: Success/Error
 *****************************************************************************/
EDA_ERROR_CODE_t EDAAppDeInit() {
  if(init_flag)
  {
#ifdef PROFILE_TIME_ENABLED
  uint32_t eda_de_init_start_time = get_micro_sec();
#endif
  NRF_LOG_DEBUG("Stop EDA measurement right now!!\n");
  if (measurement_cycle_completed) {
    AD5940_LPModeClkS(
        LPMODECLK_HFOSC); /* Trigger switching system clock to 32kHz */
    AppEDACtrl(APPCTRL_STOPSYNC, 0);
    AppEDACtrl(APPCTRL_STOPSYNC, 0); /* to make sure it is completely stopped */
  }

#ifdef EXTERNAL_TRIGGER_EDA
  disable_ad5940_ext_trigger(AppEDACfg.EDAODR);
#endif

  ad5940_port_deInit();
  gEdaAppInitFlag = false;
  /* de initing for next use */
  init_flag=0;

  /* de init power */
  adp5360_enable_ldo(ECG_LDO, false);
  NRF_LOG_DEBUG("EDA Sensor stopped!!");
#ifdef PROFILE_TIME_ENABLED
  eda_de_init_diff_time = get_micro_sec() - eda_de_init_start_time;
#endif
  }
  return EDA_SUCCESS;
}

/*!
 ****************************************************************************
 * @brief    Example of how to write an LCFG parameter
 * @param    field: LCFG field that has to be written
 * @param    value: Value to be written
 * @retval   EDA_ERROR_CODE_t
 *****************************************************************************/
EDA_ERROR_CODE_t EDAWriteLCFG(uint8_t field, uint16_t value) {
  AppEDACfg_Type *pCfg;
  AppEDAGetCfg(&pCfg);
  if (field < EDA_LCFG_MAX) {
    switch (field) {
    case EDA_LCFG_FS: /* field = 0 */
      pCfg->EDAODR = value;
      eda_user_applied_odr = 1;
      break;
    case EDA_LCFG_DFT_NUM: /* field = 2 */
      pCfg->DftNum = value;
      eda_user_applied_dftnum = 1;
      break;
    }
    return EDA_SUCCESS;
  }
  return EDA_ERROR;
}

/*!
 ****************************************************************************
 * @brief    Read LCFG parameter
 * @param    index: LCFG field
 * @param    value: Returned corresponding LCFG value
 * @retval   EDA_ERROR_CODE_t
 *****************************************************************************/
EDA_ERROR_CODE_t EDAReadLCFG(uint8_t index, uint16_t *value) {
  AppEDACfg_Type *pCfg;
  AppEDAGetCfg(&pCfg);
  if (index < EDA_LCFG_MAX) {
    switch (index) {
    case EDA_LCFG_FS: /* index = 0 */
      *value = (uint16_t)(pCfg->EDAODR);
      break;
    case EDA_LCFG_DFT_NUM: /* index = 2 */
      *value = pCfg->DftNum;
      break;
    }
    return EDA_SUCCESS;
  }
  return EDA_ERROR;
}

/*!
 ****************************************************************************
 * @brief  Eda App initialization
 * @param  None
 * @return EDA_ERROR_CODE_t
 *****************************************************************************/
EDA_ERROR_CODE_t EDAAppInit() {
#ifdef PROFILE_TIME_ENABLED
  eda_init_start_time = get_micro_sec();
#endif
   EDA_ERROR_CODE_t retVal = EDA_ERROR;
/*
  fImpCar_Type EDABase = {
      .Real = 24368.6,
      .Image = -16.696,
  };
*/
  /* power on */
  adp5360_enable_ldo(ECG_LDO, true);


  /* Init required */
  init_flag=1;

  /* start eda initialization */
  ad5940_eda_start();
  gEdaAppInitFlag = true;
  /* Control BIA measurment to start. Second parameter has no
   * meaning with this command. */
  AppEDACtrl(APPCTRL_START, 0);/* to disable running on wake up timer */

  /* Base line impedance value can be controlled by user or
     default measurements will be used */
  if( eda_user_applied_baseline_imp == USER_CONFIG_BASELINE_IMPEDANCE_SET){
    ResistorForBaseline = UserConfig_BaselineResistor;
    /*Setting baseline impedance with corresponding DFT measured values*/
    if( AppEDACfg.DftNum == DFTNUM_16){
      AppEDACtrl(EDACTRL_SETBASE,&EDABase_Userconfig_Dft16);
    }
    else {
      AppEDACtrl(EDACTRL_SETBASE,&EDABase_Userconfig_Dft8);
    }
  }
  else {
    //Default value will be used
    ResistorForBaseline = DEFAULT_BASELINE_RESISTOR;
    /*Setting baseline impedance with corresponding DFT measured values*/
    if( AppEDACfg.DftNum == DFTNUM_16){
      AppEDACtrl(EDACTRL_SETBASE,&EDABase_Dft16);
    }
    else {
      AppEDACtrl(EDACTRL_SETBASE,&EDABase_Dft8);
    }
  }

  retVal = EDA_SUCCESS;
#ifdef PROFILE_TIME_ENABLED
  eda_init_diff_time = get_micro_sec() - eda_init_start_time;
#endif
  return retVal;
}


/*!
 ****************************************************************************
 *@brief      Eda stream Read/write configuration options
 *@param      pPkt: pointer to the packet structure
 *@return     m2m2_hdr_t
 ******************************************************************************/
static m2m2_hdr_t *eda_app_reg_access(m2m2_hdr_t *p_pkt) {
  M2M2_APP_COMMON_STATUS_ENUM_t status = M2M2_APP_COMMON_STATUS_ERROR;
  PYLD_CST(p_pkt, eda_app_lcfg_op_hdr_t, p_in_payload);
  /* Allocate a response packet with space for the correct number of operations
   */
  PKT_MALLOC(p_resp_pkt, eda_app_lcfg_op_hdr_t,
      p_in_payload->num_ops * sizeof(p_in_payload->ops[0]));
  if (NULL != p_resp_pkt) {
    PYLD_CST(p_resp_pkt, eda_app_lcfg_op_hdr_t, p_resp_payload);

    switch (p_in_payload->command) {
    case M2M2_APP_COMMON_CMD_READ_LCFG_REQ:
      for (int i = 0; i < p_in_payload->num_ops; i++) {
        uint16_t reg_data = 0;
        if (EDAReadLCFG(p_in_payload->ops[i].field, &reg_data) == EDA_SUCCESS) {
          status = M2M2_APP_COMMON_STATUS_OK;
        } else {
          status = M2M2_APP_COMMON_STATUS_ERROR;
        }
        p_resp_payload->ops[i].field = p_in_payload->ops[i].field;
        p_resp_payload->ops[i].value = reg_data;
      }
      p_resp_payload->command =
          (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_APP_COMMON_CMD_READ_LCFG_RESP;
      break;
    case M2M2_APP_COMMON_CMD_WRITE_LCFG_REQ:
      for (int i = 0; i < p_in_payload->num_ops; i++) {
        if (EDAWriteLCFG(p_in_payload->ops[i].field,
                p_in_payload->ops[i].value) == EDA_SUCCESS) {
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
      /* Something has gone horribly wrong. */
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

#ifdef DEBUG_EDA
extern uint32_t eda_load_time_diff;
static m2m2_hdr_t *eda_app_debug_info(m2m2_hdr_t *p_pkt) {
  M2M2_APP_COMMON_STATUS_ENUM_t status = M2M2_APP_COMMON_STATUS_OK;
  uint32_t fifo_level = 0;
  PYLD_CST(p_pkt, m2m2_get_eda_debug_info_req_cmd_t, p_in_payload);
  PKT_MALLOC(p_resp_pkt, m2m2_get_eda_debug_info_resp_cmd_t, 0);
  PYLD_CST(p_resp_pkt, m2m2_get_eda_debug_info_resp_cmd_t, p_resp_payload);

  p_resp_payload->Interrupts_time_gap = eda_load_time_diff;
  p_resp_payload->rtia_calibration_time = eda_rtia_cal_diff_time / 1000000;
  p_resp_payload->delay_in_first_measurements = delay_in_first_measurement;
  p_resp_payload->packets_time_gap = time_elapsed_for_eda;
  p_resp_payload->first_voltage_measure_time =
      voltage_measurement_diff_time + delay_in_first_measurement;
  p_resp_payload->first_current_measure_time =
      current_measurement_diff_time + delay_in_first_measurement;
  p_resp_payload->voltage_measure_time_gap = voltage_cycles_diff_time;
  p_resp_payload->current_measure_time_gap = current_cycles_diff_time;
  p_resp_payload->EDA_Init_Time = eda_init_diff_time / 1000000;
  p_resp_payload->EDA_DeInit_Time = eda_de_init_diff_time;
  p_resp_payload->ad5940_fifo_overflow_status = get_fifostat(&fifo_level);
  p_resp_payload->ad5940_fifo_level = fifo_level;

  p_resp_payload->command = M2M2_EDA_APP_CMD_REQ_DEBUG_INFO_RESP;

  p_resp_payload->status = status;
  p_resp_pkt->dest = p_pkt->src;
  p_resp_pkt->src = p_pkt->dest;
  return p_resp_pkt;
}
#endif

/*!
 ****************************************************************************
 *@brief      Eda stream set decimation factor
 *@param      pPkt: pointer to the packet structure
 *@return     m2m2_hdr_t
 ******************************************************************************/
static m2m2_hdr_t *eda_app_decimation(m2m2_hdr_t *p_pkt) {
  M2M2_APP_COMMON_STATUS_ENUM_t status = M2M2_APP_COMMON_STATUS_OK;
  PYLD_CST(p_pkt, m2m2_sensor_common_decimate_stream_t, p_in_payload);
  PKT_MALLOC(p_resp_pkt, m2m2_sensor_common_decimate_stream_t, 0);
  if (NULL != p_resp_pkt) {
    PYLD_CST(p_resp_pkt, m2m2_sensor_common_decimate_stream_t, p_resp_payload);

    switch (p_in_payload->command) {
    case M2M2_SENSOR_COMMON_CMD_GET_STREAM_DEC_FACTOR_REQ:
      p_resp_payload->command =
          M2M2_SENSOR_COMMON_CMD_GET_STREAM_DEC_FACTOR_RESP;
      break;
    case M2M2_SENSOR_COMMON_CMD_SET_STREAM_DEC_FACTOR_REQ:
      g_state_eda.decimation_factor = p_in_payload->dec_factor;
      if (g_state_eda.decimation_factor == 0) {
        g_state_eda.decimation_factor = 1;
      }
      p_resp_payload->command =
          M2M2_SENSOR_COMMON_CMD_SET_STREAM_DEC_FACTOR_RESP;
      break;
    }
    p_resp_payload->status = status;
    p_resp_payload->dec_factor = g_state_eda.decimation_factor;
    p_resp_payload->stream = p_in_payload->stream;
    p_resp_pkt->dest = p_pkt->src;
    p_resp_pkt->src = p_pkt->dest;
  }
  return p_resp_pkt;
}




#ifdef AD5940_REG_ACCESS
/*!
 ****************************************************************************
 *@brief      AD5940 stream Read/write configuration options
 *@param      pPkt: pointer to the packet structure
 *@return     m2m2_hdr_t
 ******************************************************************************/
m2m2_hdr_t *ad5940_app_reg_access(m2m2_hdr_t *p_pkt) {
  M2M2_APP_COMMON_STATUS_ENUM_t status = M2M2_APP_COMMON_STATUS_ERROR;
  PYLD_CST(p_pkt, m2m2_sensor_common_reg_op_16_hdr_t, p_in_payload);
  PKT_MALLOC(p_resp_pkt, m2m2_sensor_common_reg_op_16_hdr_t,
      p_in_payload->num_ops * sizeof(p_in_payload->ops[0]));
  if (NULL != p_resp_pkt) {
    PYLD_CST(p_resp_pkt, m2m2_sensor_common_reg_op_16_hdr_t, p_resp_payload);
    uint32_t reg_data = 0;
    /*TODO: ad5940_port_deInit();
    // de init power
    adp5360_enable_ldo(ECG_LDO,false);*/
    switch (p_in_payload->command) {
    case M2M2_SENSOR_COMMON_CMD_READ_REG_16_REQ:
      for (int i = 0; i < p_in_payload->num_ops; i++) {
        reg_data = AD5940_ReadReg(p_in_payload->ops[i].address);
        status = M2M2_APP_COMMON_STATUS_OK;
        p_resp_payload->ops[i].address = p_in_payload->ops[i].address;
        p_resp_payload->ops[i].value = reg_data;
      }
      p_resp_payload->command =
          (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_SENSOR_COMMON_CMD_READ_REG_16_RESP;
      break;
    case M2M2_SENSOR_COMMON_CMD_WRITE_REG_16_REQ:
      for (int i = 0; i < p_in_payload->num_ops; i++) {
        AD5940_WriteReg((uint16_t)p_in_payload->ops[i].address,
            (uint32_t)p_in_payload->ops[i].value);
        status = M2M2_APP_COMMON_STATUS_OK;
        p_resp_payload->ops[i].address = p_in_payload->ops[i].address;
        p_resp_payload->ops[i].value = p_in_payload->ops[i].value;
      }
      p_resp_payload->command =
          (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_SENSOR_COMMON_CMD_WRITE_REG_16_RESP;
      break;
    default:
      // Something has gone horribly wrong.
      return NULL;
    }
    /*TODO: ad5940_port_deInit();
    // de init power
    adp5360_enable_ldo(ECG_LDO,false);*/
    p_resp_pkt->dest = p_pkt->src;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_payload->num_ops = p_in_payload->num_ops;
    p_resp_payload->status = status;
  }
  return p_resp_pkt;
}
#endif

///////////////////////////////////////////////////////////////////
#ifdef DCB
/*!
 ****************************************************************************
 * @brief    Gets the entire eda DCB configuration written in flash
 * @param    eda_dcb_data: pointer to dcb struct variable,
 *            in size of data in Double Word (32-bits)
 * @param    read_size: The size of the record to be returned to the user
 * @retval   EDA_ERROR_CODE_t: Status
 *****************************************************************************/
EDA_ERROR_CODE_t read_eda_dcb(uint32_t *eda_dcb_data, uint16_t *read_size) {
  EDA_ERROR_CODE_t dcb_status = EDA_ERROR;

  if (adi_dcb_read_from_fds(ADI_DCB_EDA_BLOCK_IDX, eda_dcb_data, read_size) ==
      DEF_OK) {
    dcb_status = EDA_SUCCESS;
  }
  return dcb_status;
}

/*!
 ****************************************************************************
 * @brief    Sets the entire eda DCB configuration in flash
 * @param    eda_dcb_data: pointer to dcb struct variable,
 *            in size of data in Double Word (32-bits)
 * @param    write_Size: The size of the record to be written
 * @retval   EDA_ERROR_CODE_t: Status
 *****************************************************************************/
EDA_ERROR_CODE_t write_eda_dcb(uint32_t *eda_dcb_data, uint16_t write_Size) {
  EDA_ERROR_CODE_t dcb_status = EDA_ERROR;

  if (adi_dcb_write_to_fds(ADI_DCB_EDA_BLOCK_IDX, eda_dcb_data, write_Size) ==
      DEF_OK) {
    dcb_status = EDA_SUCCESS;
  }
  return dcb_status;
}

/*!
 ****************************************************************************
 * @brief    Delete the entire eda DCB configuration in flash
 * @param    None
 * @retval   EDA_ERROR_CODE_t: Status
 *****************************************************************************/
EDA_ERROR_CODE_t delete_eda_dcb(void) {
  EDA_ERROR_CODE_t dcb_status = EDA_ERROR;

  if (adi_dcb_delete_fds_settings(ADI_DCB_EDA_BLOCK_IDX) == DEF_OK) {
    dcb_status = EDA_SUCCESS;
  }
  return dcb_status;
}

/*!
 ****************************************************************************
 * @brief    Set the dcb present flag
 * @param    set_flag: flag to set presence of dcb in flash
 * @retval   None
 *****************************************************************************/
void eda_set_dcb_present_flag(bool set_flag) {
  g_eda_dcb_Present = set_flag;
  NRF_LOG_INFO("EDA DCB present set: %s",
      (g_eda_dcb_Present == true ? "TRUE" : "FALSE"));
}

/*!
 ****************************************************************************
 * @brief    Get whether the dcb is present in flash
 * @param    None
 * @retval   bool: TRUE: dcb present, FALSE: dcb absent
 *****************************************************************************/
bool eda_get_dcb_present_flag(void) {
  NRF_LOG_INFO(
      "EDA DCB present: %s", (g_eda_dcb_Present == true ? "TRUE" : "FALSE"));
  return g_eda_dcb_Present;
}

/*!
 ****************************************************************************
 * @brief    Update the global dcb presence flag in flash
 * @param    None
 * @retval   None
 *****************************************************************************/
void eda_update_dcb_present_flag(void) {
  g_eda_dcb_Present = adi_dcb_check_fds_entry(ADI_DCB_EDA_BLOCK_IDX);
  NRF_LOG_INFO("Updated. EDA DCB present: %s",
      (g_eda_dcb_Present == true ? "TRUE" : "FALSE"));
}

/*!
 ****************************************************************************
 * @brief    Update the lcfg from dcb
 * @param    p_pkt: pointer to packet
 * @retval   m2m2_hdr_t
 *****************************************************************************/
static m2m2_hdr_t *eda_app_set_dcb_lcfg(m2m2_hdr_t *p_pkt) {
  M2M2_APP_COMMON_STATUS_ENUM_t status = M2M2_APP_COMMON_STATUS_ERROR;
  PKT_MALLOC(p_resp_pkt, eda_app_dcb_lcfg_t, 0);
  if (NULL != p_resp_pkt) {
    PYLD_CST(p_resp_pkt, eda_app_dcb_lcfg_t, p_resp_payload);
    if (eda_get_dcb_present_flag() == true) {
      uint32_t eda_dcb[MAXEDADCBSIZE] = {'\0'};
      uint16_t size = MAXEDADCBSIZE;
      if (read_eda_dcb(eda_dcb, &size) == EDA_SUCCESS) {
        for (uint8_t i = 0; i < size; i++) {
          /*
          each eda_dcb array element stores field and value together in total 4
          bytes. field -> 1 Byte value -> 2 Bytes undefined -> 1 Byte for ex- if
          eda_dcb[i] = 0x00ABCDEF field = 0xAB Value = 0xCDEF not_used/undefined
          -> 0x00
          */
          /* doing right shift and then & operation to get field */
          uint8_t field = (eda_dcb[i] >> 16) & 0xFF;
          /* doing & operation to get value */
          uint16_t value = eda_dcb[i] & 0xFFFF;
          if (EDAWriteLCFG(field, value) == EDA_SUCCESS) {
            status = M2M2_APP_COMMON_STATUS_OK;
          } else {
            status = M2M2_APP_COMMON_STATUS_ERROR;
          }
        }
      } else {
        status = M2M2_APP_COMMON_STATUS_ERROR;
      }
    } else {
      /* Since this function is for setting dcb lcfg for eda,
        and because this is the case when dcb lcfg is not present,
        therefore error status given */
      status = M2M2_APP_COMMON_STATUS_ERROR;
    }
    p_resp_payload->status = status;
    p_resp_payload->command = M2M2_APP_COMMON_CMD_SET_LCFG_RESP;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  }
  return p_resp_pkt;
}

/*!
 ****************************************************************************
 * @brief    Read the dcb configuration
 * @param    p_pkt: pointer to packet
 * @retval   m2m2_hdr_t
 *****************************************************************************/
static m2m2_hdr_t *eda_dcb_command_read_config(m2m2_hdr_t *p_pkt) {
  static uint16_t r_size = 0;
  uint32_t dcbdata[MAXEDADCBSIZE];
  M2M2_DCB_STATUS_ENUM_t status = M2M2_DCB_STATUS_ERR_NOT_CHKD;

  // Declare a pointer to access the input packet payload
  // PYLD_CST(p_pkt, m2m2_dcb_eda_data_t, p_in_payload);
  PKT_MALLOC(p_resp_pkt, m2m2_dcb_eda_data_t, 0);
  if (NULL != p_resp_pkt) {
    // Declare a pointer to the response packet payload
    PYLD_CST(p_resp_pkt, m2m2_dcb_eda_data_t, p_resp_payload);

    r_size = MAXEDADCBSIZE;
    if (read_eda_dcb(&dcbdata[0], &r_size) == EDA_SUCCESS) {
      for (int i = 0; i < r_size; i++)
        p_resp_payload->dcbdata[i] = dcbdata[i];
      p_resp_payload->size = (r_size);
      status = M2M2_DCB_STATUS_OK;
    } else {
      p_resp_payload->size = 0;
      status = M2M2_DCB_STATUS_ERR_ARGS;
    }

    p_resp_payload->status = status;
    p_resp_payload->command = M2M2_DCB_COMMAND_READ_CONFIG_RESP;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  }
  return p_resp_pkt;
}

static m2m2_hdr_t *eda_dcb_command_write_config(m2m2_hdr_t *p_pkt) {
  M2M2_DCB_STATUS_ENUM_t status = M2M2_DCB_STATUS_ERR_NOT_CHKD;
  uint32_t dcbdata[MAXEDADCBSIZE];

  /* Declare a pointer to access the input packet payload */
  PYLD_CST(p_pkt, m2m2_dcb_eda_data_t, p_in_payload);
  PKT_MALLOC(p_resp_pkt, m2m2_dcb_eda_data_t, 0);
  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_dcb_eda_data_t, p_resp_payload);

    for (int i = 0; i < p_in_payload->size; i++)
      dcbdata[i] = p_in_payload->dcbdata[i];
    if (write_eda_dcb(&dcbdata[0], p_in_payload->size) == EDA_SUCCESS) {
      eda_set_dcb_present_flag(true);
      status = M2M2_DCB_STATUS_OK;
    } else {
      status = M2M2_DCB_STATUS_ERR_ARGS;
    }

    p_resp_payload->status = status;
    p_resp_payload->command = M2M2_DCB_COMMAND_WRITE_CONFIG_RESP;
    p_resp_payload->size = 0;
    for(uint16_t i=0; i< MAXEDADCBSIZE; i++)
      p_resp_payload->dcbdata[i] = 0;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  }
  return p_resp_pkt;
}

static m2m2_hdr_t *eda_dcb_command_delete_config(m2m2_hdr_t *p_pkt) {
  M2M2_DCB_STATUS_ENUM_t status = M2M2_DCB_STATUS_ERR_NOT_CHKD;
  PKT_MALLOC(p_resp_pkt, m2m2_dcb_eda_data_t, 0);
  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_dcb_eda_data_t, p_resp_payload);

    if (delete_eda_dcb() == EDA_SUCCESS) {
      eda_set_dcb_present_flag(false);
      status = M2M2_DCB_STATUS_OK;
    } else {
      status = M2M2_DCB_STATUS_ERR_ARGS;
    }

    p_resp_payload->status = status;
    p_resp_payload->command = M2M2_DCB_COMMAND_ERASE_CONFIG_RESP;
    p_resp_payload->size = 0;
    for(uint16_t i=0; i< MAXEDADCBSIZE; i++)
      p_resp_payload->dcbdata[i] = 0;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  }
  return p_resp_pkt;
}

#endif
#endif//ENABLE_EDA_APP