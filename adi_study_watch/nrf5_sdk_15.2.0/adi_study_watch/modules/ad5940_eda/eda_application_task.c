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
#ifdef PROFILE_TIME_ENABLED
#include "us_tick.h"
#endif
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

#define APPBUFF_SIZE 1024
static uint32_t AppBuff[APPBUFF_SIZE];
static float LFOSCFreq; /* Measured LFOSC frequency */

static uint32_t AppBuff[APPBUFF_SIZE];
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
float magnitude, phase;
static int32_t SignalData[APPBUFF_SIZE] = {0}, eda_counter = 0;
uint8_t measurement_cycle_completed = 0;
extern int var;
uint32_t ad5940_port_Init(void);
uint32_t ad5940_port_deInit(void);

g_state_eda_t g_state_eda;
AppEDACfg_Type AppEDACfg;
static volatile int16_t user_applied_odr = 0;
static volatile int16_t user_applied_dftnum = 0;
static volatile int16_t user_applied_rtia_cal = 0;
uint8_t rtia_cal_completed = 0;

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
    int32_t *SignalData, eda_app_stream_t *pPkt, uint32_t timestamp);
static m2m2_hdr_t *eda_app_reg_access(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *eda_app_stream_config(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *eda_app_status(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *eda_app_get_version(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *eda_app_decimation(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *eda_app_dynamic_scaling(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *eda_app_dft_num_set(m2m2_hdr_t *p_pkt);
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
static uint16_t gnEDASequenceCount = 0;
void Enable_ephyz_power(void);
static int32_t AD5940PlatformCfg(void);
static void InitCfg();

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

/*!
 ****************************************************************************
 *@brief      Initialize configurations for eda application
 *@param      None
 *@return     None
 ******************************************************************************/
static void InitCfg() {
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
  AppEDACfg.FifoThresh = 2;
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
  m2m2_hdr_t *p_in_pkt = NULL;
  m2m2_hdr_t *p_out_pkt = NULL;
  ADI_OSAL_STATUS err;
  post_office_add_mailbox(M2M2_ADDR_MED_EDA, M2M2_ADDR_MED_EDA_STREAM);
  while (1) {
    adi_osal_SemPend(eda_task_evt_sem, ADI_OSAL_TIMEOUT_FOREVER);
    p_in_pkt =
        post_office_get(ADI_OSAL_TIMEOUT_NONE, APP_OS_CFG_EDA_TASK_INDEX);
    if (p_in_pkt == NULL) {
      /* No m2m2 messages to process, so fetch some data from the device */
      if(g_state_eda.num_starts != 0) /* Fetch data from device only if the device is in active state(initialized properly)*/
          fetch_eda_data();
    } else {
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
    int32_t *SignalData, eda_app_stream_t *pPkt, uint32_t timestamp) {
  /*   Calculate Impedance in form of Cartesian form */
  EDACalculateImpedance(&SignalData[0], FifoCount);
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
  static uint32_t edaData = 0;
  static eda_app_stream_t pkt;
  g_state_eda.eda_pktizer.p_pkt = NULL;
  temp = APPBUFF_SIZE;
  AppEDAISR(AppBuff, &temp);
  status = ad5950_buff_get((uint32_t *)&edaData, &edaTS);

  /* reading a pair of samples for impedance calculation */
  SignalData[eda_counter++] = edaData;
  if (eda_counter > FifoCount / 2) {
    eda_counter = 0;
  }
  while ((status == AD5940Drv_SUCCESS) && (eda_counter == FifoCount / 2)) {
    g_state_eda.decimation_nsamples++;

    if (g_state_eda.decimation_nsamples >= g_state_eda.decimation_factor) {
      g_state_eda.decimation_nsamples = 0;
      packetize_eda_raw_data(SignalData, &pkt, edaTS);
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
          p_payload_ptr->sequence_num = gnEDASequenceCount++;
#ifdef DEBUG_PKT
          post_office_msg_cnt(g_state_eda.eda_pktizer.p_pkt);
#endif
          post_office_send(g_state_eda.eda_pktizer.p_pkt, &err);
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
    status = ad5950_buff_get((uint32_t *)&edaData, &edaTS);
    SignalData[eda_counter++] = edaData;
  }
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

        /* configure call back */
    Ad5940DrvDataReadyCallback(Ad5940FifoCallBack);

    /* Init Interrupts */
    ad5940_port_Init();

    /* Initilaize eda app configuration */
    InitCfg();

    AppEDACfg.RtiaAutoScaleMin = p_in_payload->minscale;
    AppEDACfg.RtiaAutoScaleMax = p_in_payload->maxscale;
    AppEDACfg.LptiaRtiaSel = p_in_payload->lowpowerrtia;
    rtia_cal_completed = 0;

    ClearDataBufferAd5940();
    AD5940PlatformCfg();
    AD5940EDAStructInit();
    AppEDAInit(AppBuff, APPBUFF_SIZE);

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

    /* deinitalize  interrupts */
    ad5940_port_deInit();

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
      user_applied_dftnum = 1;
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
      user_applied_rtia_cal = 1;
    } else {
      AppEDACfg.LptiaRtiaSel = LPTIARTIA_100K;
      user_applied_rtia_cal = 0;
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
  PYLD_CST(p_pkt, m2m2_app_common_sub_op_t, p_in_payload);
  PKT_MALLOC(p_resp_pkt, m2m2_app_common_sub_op_t, 0);
  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_app_common_sub_op_t, p_resp_payload);
    switch (p_in_payload->command) {
    case M2M2_APP_COMMON_CMD_STREAM_START_REQ:
      var = 2;
      reset_eda_packetization();
      if (g_state_eda.num_starts == 0) {
        if (EDAAppInit() == EDA_SUCCESS) {
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
        if (EDAAppDeInit()) {
          g_state_eda.num_starts = 0;
          status = M2M2_APP_COMMON_STATUS_STREAM_STOPPED;
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
  ad5940_port_deInit();
  /* de init power */
  adp5360_enable_ldo(ECG_LDO, false);
  NRF_LOG_DEBUG("EDA Sensor stopped!!");
#ifdef PROFILE_TIME_ENABLED
  eda_de_init_diff_time = get_micro_sec() - eda_de_init_start_time;
#endif
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
      user_applied_odr = 1;
      break;
    case EDA_LCFG_DFT_NUM: /* field = 2 */
      pCfg->DftNum = value;
      user_applied_dftnum = 1;
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

/*!
 ****************************************************************************
 *@brief      This function is provided for upper controllers that want to
 *            change application parameters specially for user defined
 *            parameters
 *@param      pCfg: pointer to configuration structure
 *@return     AD5940Err
 ******************************************************************************/
AD5940Err AppEDAGetCfg(void *pCfg) {
  if (pCfg) {
    *(AppEDACfg_Type **)pCfg = &AppEDACfg;
    return AD5940ERR_OK;
  }
  return AD5940ERR_PARA;
}

/*!
 ****************************************************************************
 * @brief Control application like start, stop.
 * @param EDACtrl The command for this applicaiton, select from below paramters
 *        - APPCTRL_START: start the measurment. Note: the ramp test need
 *            to call function AppRAMPInit() every time before start it.
 *        - APPCTRL_STOPNOW: Stop the measurment immediately.
 *        - APPCTRL_STOPSYNC: Stop the measuremnt when current measured data
 *                             is read back.
 *        - APPCTRL_SHUTDOWN: Stop the measurment immediately and put AFE to
 *           shut down mode(turn off LP loop and enter hibernate).
 *        - EDACTRL_MEASVOLT: Measure voltage once current measurment
 *            is done(Interrupt occured).
 *        - EDACTRL_GETRTIAMAG: Get current RTIA value.
 * @param pPara: pointer to return state
 * @return AD5940Err.
 *******************************************************************************/
AD5940Err AppEDACtrl(int32_t EDACtrl, void *pPara) {
  switch (EDACtrl) {
  case APPCTRL_START: {
    WUPTCfg_Type wupt_cfg;
    /* Wakeup AFE by read register, read 10 times at most */
    if (AD5940_WakeUp(10) > 10)
      return AD5940ERR_WAKEUP; /* Wakeup Failed */
    if (AppEDACfg.EDAInited == bFALSE)
      return AD5940ERR_APPERROR;
    /* Start it */
    wupt_cfg.WuptEn = bTRUE;
    wupt_cfg.WuptEndSeq = WUPTENDSEQ_A;
    wupt_cfg.WuptOrder[0] = SEQID_0;
    wupt_cfg.SeqxSleepTime[SEQID_0] =
        (uint32_t)(AppEDACfg.LfoscClkFreq / AppEDACfg.EDAODR) - 2 - 4;
    /* The minimum value is 1. Do not set it to zero. Set it to 1 will spend 2
     * 32kHz clock */
    wupt_cfg.SeqxWakeupTime[SEQID_0] = 4;
    AD5940_WUPTCfg(&wupt_cfg);
    AppEDACfg.FifoDataCount = 0; /* restart */
    AppEDACfg.bRunning = bTRUE;
    break;
  }
  case APPCTRL_STOPNOW: {
    /* Wakeup AFE by read register, read 10 times at most */
    if (AD5940_WakeUp(10) > 10)
      return AD5940ERR_WAKEUP; /* Wakeup Failed */
    /* Start Wupt right now */
    AD5940_WUPTCtrl(bFALSE);
    /* There is chance this operation will fail because sequencer could put AFE
      back to hibernate mode just after waking up. Use STOPSYNC is better. */
    AD5940_WUPTCtrl(bFALSE);
    AppEDACfg.bRunning = bFALSE;
    break;
  }
  case APPCTRL_STOPSYNC: {
    AppEDACfg.StopRequired = bTRUE;
    break;
  }
  case APPCTRL_SHUTDOWN: {
    /* Stop the measurement if it's running. */
    AppEDACtrl(APPCTRL_STOPNOW, 0);
    /* Turn off LPLoop related blocks which are not controlled
     * automatically by hibernate operation */
    AFERefCfg_Type aferef_cfg;
    LPLoopCfg_Type lp_loop;
    memset(&aferef_cfg, 0, sizeof(aferef_cfg));
    AD5940_REFCfgS(&aferef_cfg);
    memset(&lp_loop, 0, sizeof(lp_loop));
    AD5940_LPLoopCfgS(&lp_loop);
    AD5940_EnterSleepS(); /* Enter Hibernate */
  } break;
  case EDACTRL_MEASVOLT:
    AppEDACfg.bMeasVoltReq = bTRUE;
    break;
  case EDACTRL_GETRTIAMAG:
    if (pPara == NULL)
      return AD5940ERR_NULLP; /* Null pointer */
    *(float *)pPara = AD5940_ComplexMag(&AppEDACfg.RtiaCurrValue);
    break;
  case EDACTRL_RSTBASE:
    AppEDACfg.ImpEDABase.Real = 0;
    AppEDACfg.ImpEDABase.Image = 0;
    AppEDACfg.ImpSum.Real = 0;
    AppEDACfg.ImpSum.Image = 0;
    AppEDACfg.ImpSumCount = 0;
    break;
  case EDACTRL_SETBASE: {
    /* The impedance used to set base line */
    fImpCar_Type *pImpBase = (fImpCar_Type *)pPara;
    AppEDACfg.ImpEDABase = *pImpBase;
  } break;
  case EDACTRL_GETAVR:
    if (pPara == NULL)
      return AD5940ERR_NULLP;
    {
      fImpCar_Type *pImpAVR = (fImpCar_Type *)pPara;
      pImpAVR->Real = AppEDACfg.ImpSum.Real / AppEDACfg.ImpSumCount;
      pImpAVR->Image = AppEDACfg.ImpSum.Image / AppEDACfg.ImpSumCount;
      break;
    }
  case APPCTRL_RUNNING:
  case EDACTRL_STATUS:
    if (pPara == NULL)
      return AD5940ERR_NULLP; /* Null pointer */
    *(BoolFlag *)pPara = AppEDACfg.bRunning;
    break;
  default:
    break;
  }
  return AD5940ERR_OK;
}

/*!
 ****************************************************************************
 *@brief      Eda application sequencer configuration generate
 *@param      None
 *@return     AD5940Err
 ******************************************************************************/
static AD5940Err AppEDASeqCfgGen(void) {
  AD5940Err error = AD5940ERR_OK;
  const uint32_t *pSeqCmd;
  uint32_t SeqLen;
  AFERefCfg_Type aferef_cfg;
  /* Waveform Generator uses some parameter(DAC update rate)
   * from HSDAC config registers */
  HSDACCfg_Type hsdac_cfg;
  LPLoopCfg_Type lp_loop;
  WGCfg_Type wg_cfg;
  DSPCfg_Type dsp_cfg;
  SWMatrixCfg_Type sw_cfg;

  AD5940_SEQGenCtrl(bTRUE);
  /* Sequence starts here */
  AD5940_SEQGpioCtrlS(AGPIO_Pin6 | AGPIO_Pin1);
  AD5940_StructInit(&aferef_cfg, sizeof(aferef_cfg));
  /* Turn off all references, only enable it when needed */
  AD5940_REFCfgS(&aferef_cfg);
  /* Disable everything, configure them during measurement */
  AD5940_StructInit(&lp_loop, sizeof(lp_loop));
  AD5940_LPLoopCfgS(&lp_loop);
  AD5940_StructInit(&wg_cfg, sizeof(wg_cfg));
  /* Maximum amplitude is 1100mV */
  wg_cfg.SinCfg.SinAmplitudeWord =
      (uint32_t)(AppEDACfg.SinAmplitude / 1100.0f * 2047);
  wg_cfg.SinCfg.SinFreqWord = AD5940_WGFreqWordCal(AppEDACfg.SinFreq, AppEDACfg.LfoscClkFreq);
  wg_cfg.SinCfg.SinPhaseWord = 0;
  wg_cfg.WgType = WGTYPE_SIN;
  AD5940_WGCfgS(&wg_cfg);
  AD5940_AFECtrlS(AFECTRL_ALL, bFALSE);
  /* Switch configuration for BioElec board */
  sw_cfg.Dswitch = SWD_OPEN; /* Open all switch D */
  sw_cfg.Pswitch = SWP_AIN2 | SWP_SE0;
  sw_cfg.Nswitch = SWN_OPEN;
  sw_cfg.Tswitch = SWT_AIN0 | SWT_AFE3LOAD;
  AD5940_SWMatrixCfgS(&sw_cfg);
  AD5940_StructInit(&dsp_cfg, sizeof(dsp_cfg));
  dsp_cfg.ADCBaseCfg.ADCMuxN = ADCMUXN_VSET1P1;
  dsp_cfg.ADCBaseCfg.ADCMuxP = ADCMUXP_AIN4;
  dsp_cfg.ADCBaseCfg.ADCPga = ADCPGA_1;
  dsp_cfg.ADCFilterCfg.ADCRate = ADCRATE_800KHZ;
  /* Use averaged SINC3 output as DFT input source */
  dsp_cfg.ADCFilterCfg.ADCAvgNum = ADCAVGNUM_4;
  dsp_cfg.ADCFilterCfg.ADCSinc2Osr = ADCSINC2OSR_22;
  dsp_cfg.ADCFilterCfg.ADCSinc3Osr = ADCSINC3OSR_5;
  dsp_cfg.ADCFilterCfg.BpNotch = bTRUE;
  dsp_cfg.ADCFilterCfg.BpSinc3 = bFALSE;
  dsp_cfg.ADCFilterCfg.Sinc2NotchEnable = bFALSE;
  dsp_cfg.DftCfg.DftNum = AppEDACfg.DftNum;
  /* Use averaged SINC3 data */
  dsp_cfg.DftCfg.DftSrc = DFTSRC_AVG;
  dsp_cfg.DftCfg.HanWinEn = AppEDACfg.HanWinEn;
  AD5940_DSPCfgS(&dsp_cfg);
  /* (n+2)*osr + 1, n=4,osr=5 */
  AD5940_ADCRepeatCfgS(5 * (4 + 2) + 1);
  hsdac_cfg.ExcitBufGain = EXCITBUFGAIN_2;
  hsdac_cfg.HsDacGain = HSDACGAIN_1;
  /* Note: the DAC update rate is decided by register DACON.RATE */
  hsdac_cfg.HsDacUpdateRate = AppEDACfg.DacUpdateRate;
  AD5940_HSDacCfgS(&hsdac_cfg);
  /* GP6->endSeq, GP5 -> AD8233=OFF, GP1->RLD=OFF */
  AD5940_SEQGpioCtrlS(0);
  /* Sequence end. */
  /* Add one extra command to disable sequencer for initialization
   * sequence because it is needed only at run one time. */
  AD5940_SEQGenInsert(SEQ_STOP());
  /* Stop here */
  /* Stop sequencer generator */
  AD5940_SEQGenCtrl(bFALSE);
  error = AD5940_SEQGenFetchSeq(&pSeqCmd, &SeqLen);
  if (error == AD5940ERR_OK) {
    AppEDACfg.InitSeqInfo.SeqId = SEQID_1;
    AppEDACfg.InitSeqInfo.SeqRamAddr = AppEDACfg.SeqStartAddr;
    AppEDACfg.InitSeqInfo.pSeqCmd = pSeqCmd;
    AppEDACfg.InitSeqInfo.SeqLen = SeqLen;
    AppEDACfg.InitSeqInfo.WriteSRAM = bTRUE;
    /* Write command to SRAM */
    AD5940_SEQInfoCfg(&AppEDACfg.InitSeqInfo);
  } else {
    return error; /* Error */
  }
  return AD5940ERR_OK;
}

/*!
 ****************************************************************************
 * @brief Generate patch sequence according to current measurement
 *            type(Voltage or Current).
 * @details The patch is used to adjust sequencer commands already
 *   stored in SRAM of AD5940 in order to perform different measurments.
 *   The reason is that the sequences need to be adjusted. Using the patch
 *   method will make things easiy and we won't need to modify
 *   sequences in register level.
 * @param   pPatch: pointer to patch information include the measurement
 *                   type, Rtia selection and buffers.
 * @return  AD5940Err: error code.
 ******************************************************************************/
static AD5940Err ApPEDASeqPatchGen(SeqPatchInfo_Type *pPatch) {
  AD5940Err err;
  const uint32_t *pSeqCmd;
  uint32_t SeqLen;
  LPAmpCfg_Type lpamp_cfg;
  AD5940_SEQGenInit(pPatch->Buffer, pPatch->BuffLen);
  AD5940_SEQGenCtrl(bTRUE);
  lpamp_cfg.LpAmpSel = LPAMP0;
  /* Use normal power mode is enough */
  lpamp_cfg.LpAmpPwrMod = LPAMPPWR_NORM;
  /* Enable Potential amplifier */
  lpamp_cfg.LpPaPwrEn = bTRUE;
  /* Enable TIA amplifier */
  lpamp_cfg.LpTiaPwrEn = bTRUE;
  /* Rf resistor controls cut off frequency. */
  lpamp_cfg.LpTiaRf = LPF_RF;
  /** @note Use 100Ohm Rload. */
  lpamp_cfg.LpTiaRload = LPTIARLOAD_100R;
  /* If autoscaling is enabled, use selected value. */
  lpamp_cfg.LpTiaRtia = pPatch->RtiaSel;
  /* Swtich settings for voltage measurement */
  if (pPatch->Type == PATCHTYPE_VOLT)
    lpamp_cfg.LpTiaSW = LPTIASW_VOLT;
  /* Switch settings for current measurement */
  else if (pPatch->Type == PATCHTYPE_CURR)
    lpamp_cfg.LpTiaSW = LPTIASW_CURR;
  AD5940_LPAMPCfgS(&lpamp_cfg);
  AD5940_SEQGenCtrl(bFALSE);
  err = AD5940_SEQGenFetchSeq(&pSeqCmd, &SeqLen);
  if (err != AD5940ERR_OK)
    return err;
  pPatch->pSeqCmd = pSeqCmd;
  pPatch->SeqLen = SeqLen;
  return AD5940ERR_OK;
}

/*!
 ****************************************************************************
 *@brief      Eda application sequencer Measurement mode command generate
 *             and write to SRAM
 *@param      None
 *@return     AD5940Err
 ******************************************************************************/
static AD5940Err AppEDASeqMeasureGen(void) {
  AD5940Err error = AD5940ERR_OK;
  const uint32_t *pSeqCmd;
  uint32_t SeqLen;
  uint32_t i;
  uint32_t DFTNumber;
  LPDACCfg_Type lpdac_cfg;
  LPAmpCfg_Type lpamp_cfg;
  /* Start sequence generator here */
  AD5940_SEQGenCtrl(bTRUE);

  /* Stage I: Initialization */
  /* GP6->endSeq, GP5 -> AD8233=OFF, GP1->RLD=OFF */
  AD5940_SEQGpioCtrlS(AGPIO_Pin6);
  /* LP loop configure: LPDAC and LPAMP */
  lpdac_cfg.LpdacSel = LPDAC0;
  lpdac_cfg.DataRst = bFALSE;
  lpdac_cfg.LpDacSW =
      LPDACSW_VBIAS2LPPA | LPDACSW_VZERO2LPTIA | LPDACSW_VZERO2PIN;
  /* Use internal 2.5V reference */
  lpdac_cfg.LpDacRef = LPDACREF_2P5;
  /* Use data from waveform generator */
  lpdac_cfg.LpDacSrc = LPDACSRC_WG;
  lpdac_cfg.LpDacVbiasMux = LPDACVBIAS_12BIT;
  /* Use 6bit LPDAC for Vzero */
  lpdac_cfg.LpDacVzeroMux = LPDACVZERO_6BIT;
  /* Enable LPDAC */
  lpdac_cfg.PowerEn = bTRUE;
  /* Don't care, 12bit DAC data is from WG */
  lpdac_cfg.DacData12Bit = 0;
  /* Set it to middle scale of LPDAC. Vzero is the
   * bias voltage of LPTIA amplifier */
  lpdac_cfg.DacData6Bit = 32;
  AD5940_LPDACCfgS(&lpdac_cfg);
  /* Voltage and current measurment need different switch settings,
   *  record the difference and only modify this part
   *  for different purpose */
  /* Record the start address of below commands */
  error = AD5940_SEQGenFetchSeq(NULL, &AppEDACfg.SeqPatchInfo.SRAMAddr);
  if (error != AD5940ERR_OK)
    return error;
  lpamp_cfg.LpAmpSel = LPAMP0;
  /* Use normal power mode is enough */
  lpamp_cfg.LpAmpPwrMod = LPAMPPWR_NORM;
  /* Enable Potential amplifier */
  lpamp_cfg.LpPaPwrEn = bTRUE;
  /* Enable TIA amplifier */
  lpamp_cfg.LpTiaPwrEn = bTRUE;
  /* Rf resistor controls cut off frequency. */
  lpamp_cfg.LpTiaRf = LPF_RF;
  /** @note Use 100Ohm Rload. */
  lpamp_cfg.LpTiaRload = LPTIARLOAD_100R;
  /* If autoscaling is enabled, then use selected value. */
  lpamp_cfg.LpTiaRtia = AppEDACfg.LptiaRtiaSel;
  /* Switch settings for voltage measurement */
  lpamp_cfg.LpTiaSW = LPTIASW_VOLT;
  AD5940_LPAMPCfgS(&lpamp_cfg);
  /* Enable low power bandgap and 2.5V reference buffer */
  AD5940_WriteReg(REG_AFE_LPREFBUFCON, 0);
  /* Switch Off everything */
  AD5940_AFECtrlS(AFECTRL_ALL, bFALSE);
  /* Enter LP control mode. The registers are summarized to
   * LPMODECON, so we can control some blocks conveniently */
  AD5940_LPModeEnS(bTRUE);
  /* Trigger switching system clock to 32kHz */
  AD5940_LPModeClkS(LPMODECLK_LFOSC);
  /* Disable all */
  AD5940_LPModeCtrlS(LPMODECTRL_NONE);
  /* Start waveform generator */
  AD5940_AFECtrlS(AFECTRL_WG, bTRUE);
  /* wait for stable state */
  AD5940_SEQGenInsert(SEQ_WAIT(LPF_TIME * 32));
  /* Enable DFT engine */
  AD5940_AFECtrlS(AFECTRL_DFT, bTRUE);
  /* Stage II: ADC Run to sample enough data(DFT number) */
  DFTNumber = (1 << (AppEDACfg.DftNum + 2));
  for (i = 0; i < DFTNumber; i++) {
#define EDA_LPMODCTRLSETS                                                      \
  LPMODECTRL_GLBBIASZ | LPMODECTRL_GLBBIASP | LPMODECTRL_HPREFPWR |            \
      LPMODECTRL_BUFHP1P8V | LPMODECTRL_BUFHP1P1V | LPMODECTRL_HFOSCEN
    /* Turn ON HPREF */
    AD5940_LPModeCtrlS(EDA_LPMODCTRLSETS);
    AD5940_SEQGenInsert(SEQ_WAIT(4));
    /* Set RepeatEN will enable ADC power */
    AD5940_LPModeCtrlS(EDA_LPMODCTRLSETS | LPMODECTRL_REPEATEN);
    /* Wait 50us at least to allow ADC settiling. one NOP commands
     * consumes two system clock(32kHz) before ADCCNV = 61.5us. */
    AD5940_SEQGenInsert(SEQ_NOP());
    /* Start ADC conversion. !!Clear REPEATEN bit */
    AD5940_LPModeCtrlS(EDA_LPMODCTRLSETS | LPMODECTRL_ADCCNV);
    AD5940_SEQGenInsert(SEQ_NOP());
    /* One command need 31.25us because the system clock is 32kHz now. */
    /* There is no need to wait such long time for last point, only
     * enough clock for DFT calculation before disable it. */
    if (i != DFTNumber - 1) {
      /* Disable all */
      AD5940_LPModeCtrlS(LPMODECTRL_NONE);
      AD5940_SEQGenInsert(
          SEQ_WAIT(AppEDACfg.LfoscClkFreq / AppEDACfg.SampleFreq - 12));
    } else {
      /* Disable all except 16MHz HFOSC */
      AD5940_LPModeCtrlS(LPMODECTRL_HFOSCEN);
      /* wait another 21 clocks. DFT need it to calculte last input data */
      AD5940_SEQGenInsert(SEQ_WAIT(21));
    }
  }
  /* Stage III: Turn off all we can */
  /* Switching back to 16MHz */
  AD5940_LPModeClkS(LPMODECLK_HFOSC);
  /* Disable waveform generator */
  AD5940_AFECtrlS(AFECTRL_ALL, bFALSE);
  lpamp_cfg.LpAmpSel = LPAMP0;
  lpamp_cfg.LpPaPwrEn = bFALSE;
  lpamp_cfg.LpTiaPwrEn = bFALSE;
  lpamp_cfg.LpTiaRf = LPTIARF_OPEN;
  lpamp_cfg.LpTiaRtia = LPTIARTIA_OPEN;
  lpamp_cfg.LpTiaSW = 0;
  AD5940_LPAMPCfgS(&lpamp_cfg);
  AD5940_WriteReg(REG_AFE_LPREFBUFCON,
      BITM_AFE_LPREFBUFCON_LPBUF2P5DIS | BITM_AFE_LPREFBUFCON_LPREFDIS);
  /* Disconnect all switch */
  lpdac_cfg.LpDacSW = 0;
  lpdac_cfg.PowerEn = bFALSE;
  AD5940_LPDACCfgS(&lpdac_cfg);
  /* GP6->endSeq, GP5 -> AD8233=OFF, GP1->RLD=OFF */
  AD5940_SEQGpioCtrlS(0);
  /* Go to hibernate */
  AD5940_EnterSleepS();
  /* Sequence end. */
  /* Stop sequencer generator */
  AD5940_SEQGenCtrl(bFALSE);
  error = AD5940_SEQGenFetchSeq(&pSeqCmd, &SeqLen);
  if (error == AD5940ERR_OK) {
    /* Buffer limited */
    if (AppEDACfg.MaxSeqLen < (SeqLen + AppEDACfg.InitSeqInfo.SeqLen))
      return AD5940ERR_BUFF;
    AppEDACfg.MeasureSeqInfo.SeqId = SEQID_0;
    AppEDACfg.MeasureSeqInfo.SeqRamAddr =
        AppEDACfg.InitSeqInfo.SeqRamAddr + AppEDACfg.InitSeqInfo.SeqLen;
    AppEDACfg.MeasureSeqInfo.pSeqCmd = pSeqCmd;
    AppEDACfg.MeasureSeqInfo.SeqLen = SeqLen;
    AppEDACfg.MeasureSeqInfo.WriteSRAM = bTRUE;
    /* Write command to SRAM */
    AD5940_SEQInfoCfg(&AppEDACfg.MeasureSeqInfo);
    /* Record where the patch should be applied. */
    /* The start address in AD5940 SRAM */
    AppEDACfg.SeqPatchInfo.SRAMAddr += AppEDACfg.MeasureSeqInfo.SeqRamAddr;
  } else {
    return error; /* Error */
  }
  return AD5940ERR_OK;
}

fImpCar_Type RtiaCalValue; /* Calibration result - imaginary and real */
/*!
 ****************************************************************************
 * @brief Calibrate LPTIA internal RTIA resistor(s).
 * @details This function will do calibration using
 * parameters stored in @ref AppEDACfg structure.
 * @param None
 * @return AD5940Err: return error code.
 ******************************************************************************/
AD5940Err AppEDARtiaCal(void) {
  LPRTIACal_Type lprtia_cal;
  AD5940_StructInit(&lprtia_cal, sizeof(lprtia_cal));
  /* low power amplifier (LPTIA + potentiostat amplifier) */
  lprtia_cal.LpAmpSel = LPAMP0;
  /* Real + Image */
  lprtia_cal.bPolarResult = bFALSE;
  lprtia_cal.AdcClkFreq = AppEDACfg.AdcClkFreq;
  lprtia_cal.SysClkFreq = AppEDACfg.SysClkFreq;
  lprtia_cal.ADCSinc3Osr = ADCSINC3OSR_4;
  lprtia_cal.ADCSinc2Osr = ADCSINC2OSR_22;
  /* Maximum DFT number */
  lprtia_cal.DftCfg.DftNum = DFTNUM_2048;
  lprtia_cal.DftCfg.DftSrc = DFTSRC_SINC2NOTCH;
  lprtia_cal.DftCfg.HanWinEn = bTRUE;
  lprtia_cal.fFreq = AppEDACfg.SinFreq;
  lprtia_cal.fRcal = AppEDACfg.RcalVal;
  lprtia_cal.bWithCtia = bTRUE;
  lprtia_cal.LpAmpPwrMod = LPAMPPWR_NORM;
  lprtia_cal.bWithCtia = bTRUE;
  lprtia_cal.LpTiaRtia = AppEDACfg.LptiaRtiaSel;

  /* if autoscale enable */
  if (AppEDACfg.RtiaAutoScaleEnable == bTRUE) {
    int i = AppEDACfg.RtiaAutoScaleMin;
    for (; i <= AppEDACfg.RtiaAutoScaleMax; i++) {
      lprtia_cal.LpTiaRtia = i;
      AD5940_LPRtiaCal(&lprtia_cal, &RtiaCalValue);
      AppEDACfg.RtiaCalTable[i] = RtiaCalValue;
    }
    AppEDACfg.RtiaCurrValue = AppEDACfg.RtiaCalTable[AppEDACfg.RtiaIndexCurr];
    rtia_cal_completed = 1;
  } else {
    AD5940_LPRtiaCal(&lprtia_cal, &RtiaCalValue);
    AppEDACfg.RtiaCurrValue = RtiaCalValue;
    rtia_cal_completed = 0;
  }
  return AD5940ERR_OK;
}

/*!
 ****************************************************************************
 * @brief Initialize the EDA measurement.
 * @details This function must be called before start measurement.
 *  It will initialize all needed hardwares and put AD5940 to ready state.
 *  The application parameters stored in @ref AppEDACfg can be changed.
 *  Call this function to re-initialize AD5940 with new parameters.
 * @param pBuffer: the buffer for sequencer generator. Only need to
 *                 provide it for the first time.
 * @param BufferSize: The buffer size start from pBuffer.
 * @return AD5940Err: return error code.
 ******************************************************************************/
AD5940Err AppEDAInit(uint32_t *pBuffer, uint32_t BufferSize) {
  AD5940Err error = AD5940ERR_OK;
  /* sequencer configuration */
  SEQCfg_Type seq_cfg;
  /* FIFO configuration */
  FIFOCfg_Type fifo_cfg;
  ADI_OSAL_PRIORITY default_priority;
  /* State of EDA App, init/voltage mesaurement / current / RTIA Calibration */
  AppEDACfg.EDAStateCurr = EDASTATE_INIT;
  /* Wakeup AFE by read register, read 10 times at most */
  if (AD5940_WakeUp(10) > 10)
    return AD5940ERR_WAKEUP;

  uint32_t tempreg;
  tempreg = AD5940_ReadReg(REG_AGPIO_GP0OEN);
  tempreg &= 0xFFE7;
  tempreg |= 0x0018;
  AD5940_AGPIOOen(tempreg);

  /* pull up / pull down register */
  tempreg = AD5940_ReadReg(REG_AGPIO_GP0PE);
  tempreg &= 0xFFE7;
  tempreg |= 0x0018;
  AD5940_AGPIOPen(tempreg);
  /* data set register */
  tempreg = AD5940_ReadReg(REG_AGPIO_GP0SET);
  tempreg &= 0xFFE7;
  /* set only eda gpio pin as high */
  tempreg |= 0x0008;
  AD5940_AGPIOSet(tempreg);
  /* Configure sequencer and stop it */
  seq_cfg.SeqMemSize = SEQMEMSIZE_2KB;
  seq_cfg.SeqBreakEn = bFALSE;
  seq_cfg.SeqIgnoreEn = bFALSE;
  seq_cfg.SeqCntCRCClr = bTRUE;
  seq_cfg.SeqEnable = bFALSE;
  seq_cfg.SeqWrTimer = 0;
  AD5940_SEQCfg(&seq_cfg);
#ifdef PROFILE_TIME_ENABLED
  uint32_t eda_rtia_cal_start_time = get_micro_sec();
#endif
  /* Do RTIA calibration */
  /* Do calibration on the first initialization */
  if ((AppEDACfg.ReDoRtiaCal == bTRUE) || AppEDACfg.EDAInited == bFALSE) {
    AppEDACfg.EDAStateCurr = EDASTATE_RTIACAL;
    adi_osal_ThreadGetPrio(NULL, &default_priority);
    adi_osal_ThreadSetPrio(NULL, configMAX_PRIORITIES - 1);
    AppEDARtiaCal();
    adi_osal_ThreadSetPrio(NULL, default_priority);
    AppEDACfg.ReDoRtiaCal = bFALSE;
  }
#ifdef PROFILE_TIME_ENABLED
  eda_rtia_cal_diff_time = get_micro_sec() - eda_rtia_cal_start_time;
#endif
  /* Reconfigure FIFO */
  AD5940_FIFOCtrlS(FIFOSRC_DFT, bFALSE);
  fifo_cfg.FIFOEn = bTRUE;
  fifo_cfg.FIFOMode = FIFOMODE_FIFO;
  /* 4kB for FIFO, The reset 2kB for sequencer */
  fifo_cfg.FIFOSize = FIFOSIZE_4KB;
  fifo_cfg.FIFOSrc = FIFOSRC_DFT;
  /* The first measurment is for excitation voltage. */
  fifo_cfg.FIFOThresh = AppEDACfg.VoltCalPoints * 2;
  AD5940_FIFOCfg(&fifo_cfg);
  AD5940_INTCClrFlag(AFEINTSRC_ALLINT);
  /* Start sequence generator */
  /* Initialize sequencer generator */
  if ((AppEDACfg.EDAInited == bFALSE) || (AppEDACfg.bParaChanged == bTRUE)) {
    if (pBuffer == 0)
      return AD5940ERR_PARA;
    if (BufferSize == 0)
      return AD5940ERR_PARA;
    AD5940_SEQGenInit(pBuffer, BufferSize);
    /* Generate initialize sequence */
    /* Application initialization sequence using either MCU or sequencer */
    error = AppEDASeqCfgGen();
    if (error != AD5940ERR_OK)
      return error;
    /* Generate measurement sequence */
    error = AppEDASeqMeasureGen();
    if (error != AD5940ERR_OK)
      return error;
    AppEDACfg.bParaChanged = bFALSE; /* Clear this flag as we already
                                        implemented the new configuration */
  }
  /* Initialization sequence */
  /* Enable sequencer, run initialization sequence */
  AD5940_SEQCtrlS(bTRUE);
  AD5940_SEQMmrTrig(AppEDACfg.InitSeqInfo.SeqId);
  while (AD5940_INTCTestFlag(AFEINTC_1, AFEINTSRC_ENDSEQ) == bFALSE)
    ;
  /* Apply patch for voltage measurment */
  /* After initialization, the first thing is to measure excitation voltage */
  AppEDACfg.EDAStateCurr = EDASTATE_VOLT;
  /* Init with a value */
  AppEDACfg.RtiaIndexCurr = AppEDACfg.RtiaIndexNext = AppEDACfg.LptiaRtiaSel;
  AppEDACfg.SeqPatchInfo.RtiaSel = LPTIARTIA_OPEN;
  AppEDACfg.SeqPatchInfo.Type = PATCHTYPE_VOLT;
  error = ApPEDASeqPatchGen(&AppEDACfg.SeqPatchInfo);
  if (error != AD5940ERR_OK)
    return error;
  /* Apply the patch to SRAM */
  AD5940_SEQCmdWrite(AppEDACfg.SeqPatchInfo.SRAMAddr,
      AppEDACfg.SeqPatchInfo.pSeqCmd, AppEDACfg.SeqPatchInfo.SeqLen);
  /* Enable sequencer, and wait for trigger */
  AD5940_SEQCtrlS(bTRUE);
  /* Clear interrupt flag generated before */
  AD5940_ClrMCUIntFlag();
  AD5940_AFEPwrBW(AFEPWR_LP, AFEBW_250KHZ);
  /**@todo remove it? close switch SW1  */
  AD5940_WriteReg(REG_AFE_SWMUX, 0x01);
  if (AppEDACfg.RtiaAutoScaleMin > AppEDACfg.RtiaAutoScaleMax) {
    uint32_t temp;
    temp = AppEDACfg.RtiaAutoScaleMin;
    AppEDACfg.RtiaAutoScaleMin = AppEDACfg.RtiaAutoScaleMax;
    AppEDACfg.RtiaAutoScaleMax = temp;
  }
  /* EDA application has been initialized. */
  AppEDACfg.EDAInited = bTRUE;
  return AD5940ERR_OK;
}

/*!
 ****************************************************************************
 *@brief      AD5940 structure initialization
 *@param      None
 *@return     None
 ******************************************************************************/
void AD5940EDAStructInit(void) {
  AppEDACfg_Type *pCfg;

  AppEDAGetCfg(&pCfg);
  pCfg->MaxSeqLen = 512;
  /* Don't do LFOSC calibration now. We assume the default LFOSC is trimmed. */
  pCfg->LfoscClkFreq = 32000;
  /* We manually select resistor value */
  if (!user_applied_rtia_cal) {
    pCfg->RtiaAutoScaleEnable = bTRUE;
    pCfg->RtiaAutoScaleMax = LPTIARTIA_512K;
    pCfg->RtiaAutoScaleMin = LPTIARTIA_100K;
    pCfg->LptiaRtiaSel = LPTIARTIA_100K;
  } else {
    user_applied_rtia_cal = 0;
  }
  if (!user_applied_dftnum) {
    /* DFNUM_16 = 2 */
    AppEDACfg.DftNum = DFTNUM_16;
  } else {
    user_applied_dftnum = 0;
  }
  /* if odr is > 16 use dft num = 8 */
  if (user_applied_odr) {
    if (pCfg->EDAODR > 16) {
      /* set dft num to 8 */
      /* DFTNUM_8 = 1 */
      AppEDACfg.DftNum = DFTNUM_8;
    }
  }
  /* Set excitation voltage to 0.75 times of full range */
  pCfg->SinAmplitude = 1100 * 3 / 4;
  pCfg->SinFreq = 100.0f;
  /* Do not change sample frequency unless you know how it works */
  pCfg->SampleFreq = 400.0f;
  if (!user_applied_odr) {
    /* ODR decides how freuquently to start the engine to measure impedance */
    pCfg->EDAODR = 4.0f;
  } else {
    user_applied_odr = 0;
  }
  /* The minimum threshold value is 4, and should always be 4*N, where N is
   * 1,2,3... */
  pCfg->FifoThresh = 2;
}

/*!
 ****************************************************************************
 *@brief      Initialize AD5940 basic blocks like clock. It will first reset
 *             AD5940 using reset pin
 *@param      None
 *@return     int32_t
 ******************************************************************************/
static int32_t AD5940PlatformCfg(void) {
  CLKCfg_Type clk_cfg;
  FIFOCfg_Type fifo_cfg;
  SEQCfg_Type seq_cfg;
  AGPIOCfg_Type gpio_cfg;
  LFOSCMeasure_Type LfoscMeasure;
  /* Use hardware reset */
  AD5940_HWReset();
  /* Platform configuration */
  AD5940_Initialize();
  /* Step1. Configure clock */
  clk_cfg.ADCClkDiv = ADCCLKDIV_1;
  clk_cfg.ADCCLkSrc = ADCCLKSRC_HFOSC;
  clk_cfg.SysClkDiv = SYSCLKDIV_1;
  clk_cfg.SysClkSrc = SYSCLKSRC_HFOSC;
  clk_cfg.HfOSC32MHzMode = bFALSE;
  clk_cfg.HFOSCEn = bTRUE;
  clk_cfg.HFXTALEn = bFALSE;
  clk_cfg.LFOSCEn = bTRUE;
  AD5940_CLKCfg(&clk_cfg);
  /* Step2. Configure FIFO and Sequencer*/
  fifo_cfg.FIFOEn = bFALSE;
  fifo_cfg.FIFOMode = FIFOMODE_FIFO;
  /* 4kB for FIFO, The reset 2kB for sequencer */
  fifo_cfg.FIFOSize = FIFOSIZE_4KB;
  fifo_cfg.FIFOSrc = FIFOSRC_DFT;
  /* DFT result. One pair for RCAL, another for Rz. One DFT result have
   * real part and imaginary part */
  fifo_cfg.FIFOThresh = 2;
  /* Disable to reset FIFO */
  AD5940_FIFOCfg(&fifo_cfg);
  fifo_cfg.FIFOEn = bTRUE;
  AD5940_FIFOCfg(&fifo_cfg);
  /* Enable FIFO here */
  /* Configure sequencer and stop it */
  seq_cfg.SeqMemSize = SEQMEMSIZE_2KB;
  seq_cfg.SeqBreakEn = bFALSE;
  seq_cfg.SeqIgnoreEn = bFALSE;
  seq_cfg.SeqCntCRCClr = bTRUE;
  seq_cfg.SeqEnable = bFALSE;
  seq_cfg.SeqWrTimer = 0;
  AD5940_SEQCfg(&seq_cfg);
  /* Step3. Interrupt controller */
  /* Enable all interrupt in Interrupt Controller 1, so we can check INTC flags
   */
  AD5940_INTCCfg(AFEINTC_1, AFEINTSRC_ALLINT, bTRUE);
  /* Interrupt Controller 0 will control GP0 to generate interrupt to MCU */
  AD5940_INTCCfg(AFEINTC_0, AFEINTSRC_DATAFIFOTHRESH, bTRUE);
  AD5940_INTCClrFlag(AFEINTSRC_ALLINT);

  /* Step4: Configure GPIO */
  gpio_cfg.FuncSet =  GP6_SYNC | GP5_SYNC | GP4_SYNC | GP2_EXTCLK | GP1_SYNC | GP0_INT;
  gpio_cfg.InputEnSet = AGPIO_Pin2;
  gpio_cfg.OutputEnSet =  AGPIO_Pin0 | AGPIO_Pin1 | AGPIO_Pin4 | AGPIO_Pin5 | AGPIO_Pin6;
  gpio_cfg.OutVal = 0;
  gpio_cfg.PullEnSet = 0;
  AD5940_AGPIOCfg(&gpio_cfg);

  /* Enable AFE to enter sleep mode. */
  AD5940_SleepKeyCtrlS(SLPKEY_UNLOCK);
  /* Measure LFOSC frequency */
  /**@note Calibrate LFOSC using system clock. The system clock accuracy
   * decides measurment accuracy. Use XTAL to get better result. */
  /* 1000ms used for calibration. */
  LfoscMeasure.CalDuration = 1000.0;
  /* Put sequence commands from start address of SRAM */
  LfoscMeasure.CalSeqAddr = 0;
  /* 16MHz in this firmware */
  LfoscMeasure.SystemClkFreq = 16000000.0f;
  AD5940_LFOSCMeasure(&LfoscMeasure, &LFOSCFreq);
  NRF_LOG_INFO("Freq:%d\n", LFOSCFreq);
  return 0;
}

/*!
  ****************************************************************************
  *@brief      Ecg application modify registers when AFE wakeup. It is called
               in ISR when AFE has been wakeup and we can access registers
  *@param      pData: the buffer points to data read back from FIFO
  *@param      pDataCount: The data count in pData buffer
  *@return     AD5940Err
  ******************************************************************************/
AD5940Err AppEDARegModify(int32_t *const pData, uint32_t *pDataCount) {
  AD5940Err error = AD5940ERR_OK;
  if (AppEDACfg.EDAStateCurr == EDASTATE_VOLT) {
    SWMatrixCfg_Type sw_cfg;
    /* Next step is to measure current */
    AppEDACfg.EDAStateNext = EDASTATE_CURR;
    /* Need change some registers in order to measure current */
    /* Stop it for now. */
    AD5940_SEQCtrlS(bFALSE);
    /* Disable FIFO firstly because we are going to change FIFO threshold */
    AD5940_FIFOCtrlS(FIFOSRC_DFT, bFALSE);
    AD5940_FIFOThrshSet(AppEDACfg.FifoThresh);
    /* Enable FIFO */
    AD5940_FIFOCtrlS(FIFOSRC_DFT, bTRUE);
    /* Change Switch matrix settings to connect AIN2(body) to SE0 */
    /* Open all switch D */
    sw_cfg.Dswitch = SWD_OPEN;
    sw_cfg.Pswitch = SWP_AIN2 | SWP_SE0;
    sw_cfg.Nswitch = SWN_OPEN;
    /* This switch is for ECG. */
    sw_cfg.Tswitch = SWT_AIN0 | SWT_AFE3LOAD;
    AD5940_SWMatrixCfgS(&sw_cfg);
    /* Apply patch for current measurment */
    AppEDACfg.SeqPatchInfo.Type = PATCHTYPE_CURR;
    AppEDACfg.SeqPatchInfo.RtiaSel = AppEDACfg.RtiaIndexNext;
    error = ApPEDASeqPatchGen(&AppEDACfg.SeqPatchInfo);
    if (error != AD5940ERR_OK)
      return error;
    /* Apply the patch to SRAM */
    AD5940_SEQCmdWrite(AppEDACfg.SeqPatchInfo.SRAMAddr,
        AppEDACfg.SeqPatchInfo.pSeqCmd, AppEDACfg.SeqPatchInfo.SeqLen);
    /* Enable sequencer. Sequencer will run when next valid trigger comes */
    AD5940_SEQCtrlS(bTRUE);
  } else if (AppEDACfg.EDAStateCurr == EDASTATE_CURR) {
    if (AppEDACfg.bChangeRtia == bTRUE) {
      AppEDACfg.bChangeRtia = bFALSE;
      /* Apply patch for next RTIA selection */
      AppEDACfg.SeqPatchInfo.Type = PATCHTYPE_CURR;
      AppEDACfg.SeqPatchInfo.RtiaSel = AppEDACfg.RtiaIndexNext;
      error = ApPEDASeqPatchGen(&AppEDACfg.SeqPatchInfo);
      if (error != AD5940ERR_OK)
        return error;
      /* Apply the patch to SRAM */
      AD5940_SEQCmdWrite(AppEDACfg.SeqPatchInfo.SRAMAddr,
          AppEDACfg.SeqPatchInfo.pSeqCmd, AppEDACfg.SeqPatchInfo.SeqLen);
    }
    measurement_cycle_completed = 1;
  }

  if (AppEDACfg.bMeasVoltReq == bTRUE) {
    SWMatrixCfg_Type sw_cfg;
    /* Clear this request */
    AppEDACfg.bMeasVoltReq = bFALSE;
    /* Next step is to measure voltage */
    AppEDACfg.EDAStateNext = EDASTATE_VOLT;
    /* Change Switch matrix settings to connect AIN2(body) to SE0 */
    /* Open all switch D */
    sw_cfg.Dswitch = SWD_OPEN;
    sw_cfg.Pswitch = SWP_OPEN;
    sw_cfg.Nswitch = SWN_OPEN;
    /* This switch is for ECG */
    sw_cfg.Tswitch = SWT_AIN0 | SWT_AFE3LOAD;
    AD5940_SWMatrixCfgS(&sw_cfg);
    /* Need change some registers in order to measure current */
    /* Stop it for now. */
    AD5940_SEQCtrlS(bFALSE);
    /* Disable FIFO first because we are going to change FIFO threshold */
    AD5940_FIFOCtrlS(FIFOSRC_DFT, bFALSE);
    AD5940_FIFOThrshSet(AppEDACfg.VoltCalPoints * 2);
    /* Enable FIFO*/
    AD5940_FIFOCtrlS(FIFOSRC_DFT, bTRUE);
    /* Apply patch for current measurment */
    AppEDACfg.SeqPatchInfo.Type = PATCHTYPE_VOLT;
    AppEDACfg.SeqPatchInfo.RtiaSel = LPTIARTIA_OPEN;
    error = ApPEDASeqPatchGen(&AppEDACfg.SeqPatchInfo);
    if (error != AD5940ERR_OK)
      return error;
    /* Apply the patch to SRAM */
    AD5940_SEQCmdWrite(AppEDACfg.SeqPatchInfo.SRAMAddr,
        AppEDACfg.SeqPatchInfo.pSeqCmd, AppEDACfg.SeqPatchInfo.SeqLen);
    /* Enable sequencer. Sequencer will run when next valid trigger comes */
    AD5940_SEQCtrlS(bTRUE);
  }
  if (AppEDACfg.NumOfData > 0) {
    AppEDACfg.FifoDataCount += *pDataCount / 4;
    if (AppEDACfg.FifoDataCount >= AppEDACfg.NumOfData) {
      AD5940_WUPTCtrl(bFALSE);
      return AD5940ERR_OK;
    }
  }
  if (AppEDACfg.StopRequired == bTRUE) {
    AD5940_WUPTCtrl(bFALSE);
    AppEDACfg.StopRequired = bFALSE;
    AppEDACfg.bRunning = bFALSE;
    return AD5940ERR_OK;
  }
  return AD5940ERR_OK;
}

/*!
 ****************************************************************************
 * @brief Depending on the data type, do appropriate data pre-process
 *       before return back to controller
 * @param pImpedance: the buffer points to pre-processed data.
 * The impedance magnitude value is used to decide new RTIA settings.
 * @param uiDataCount: The data count in pData buffer.
 * @return uint32_t: return the next appropriate RTIA index value.
 ******************************************************************************/
static uint32_t EDARtiaAutoScaling(
    fImpCar_Type *const pImpedance, uint32_t uiDataCount) {
  uint32_t OptRtiaIndex;
  float MagMean = 0;
  fImpCar_Type SumImp = {0, 0};
  /* Get Averaged Magnitude Result */
  for (int i = 0; i < uiDataCount; i++) {
    SumImp.Real += pImpedance[i].Real;
    SumImp.Image += pImpedance[i].Image;
  }
  SumImp.Real /= uiDataCount;
  SumImp.Image /= uiDataCount;
  /* Impedance under test is sum of changed value and baseline */
  SumImp = AD5940_ComplexAddFloat(&SumImp, &AppEDACfg.ImpEDABase);
  MagMean = AD5940_ComplexMag(&SumImp);
  OptRtiaIndex = AppEDACfg.RtiaAutoScaleMin;
  /* This is much easier because although the RTIA is not the best value,
   * the results are still reliable. We can directly choose the RTIA matched */
  for (; OptRtiaIndex < AppEDACfg.RtiaAutoScaleMax;) {
    float mag;
    mag = AD5940_ComplexMag(&AppEDACfg.RtiaCalTable[OptRtiaIndex + 1]);
    if (MagMean < mag * 0.97f)
      break;
    OptRtiaIndex++;
  }
  return OptRtiaIndex;
}

int32_t SumReal = 0, SumImage = 0;
/*!
 ****************************************************************************
 * @brief Data pre-process
 * @details Depending on the data type, do appropriate data pre-process
 *    before return back to controller
 * @param pData: the buffer points to data read back from FIFO.
 * @param pDataCount: The data count in pData buffer.
 * @return AD5940Err: return error code.
 ******************************************************************************/
AD5940Err AppEDADataProcess(int32_t *const pData, uint32_t *pDataCount) {
#ifdef PROFILE_TIME_ENABLED
  delay_in_first_measurement =
      abs(get_micro_sec() - eda_init_start_time) / 1000000;
#endif
  uint32_t DataCount = *pDataCount;
  *pDataCount = 0;
  /* EDA results are DFT results */
  for (uint32_t i = 0; i < DataCount; i++) {
    pData[i] &= 0x3ffff;
    /* Bit17 is sign bit */
    /* Data is 18bit in two's complement, bit17 is the sign bit */
    if (pData[i] & (1 << 17))
      pData[i] |= 0xfffc0000;
  }
  if (AppEDACfg.EDAStateCurr == EDASTATE_VOLT) {
    uint32_t DftResCnt;
    iImpCar_Type *pDftRes = (iImpCar_Type *)pData;
    SumReal = 0, SumImage = 0;
    /* Get average excitation voltage */
    if (DataCount != AppEDACfg.VoltCalPoints * 2)
      return EDAERR_VOLTMEASURE;
    DftResCnt = DataCount / 2;
    /* Discard the first 4 results */
    if (DftResCnt > 4) {
      DftResCnt -= 4;
      pDftRes += 4;
    }
    for (uint32_t i = 0; i < DftResCnt; i++) {
      SumReal += pDftRes[i].Real;
      SumImage += pDftRes[i].Image;
    }
    SumReal /= (int32_t)DftResCnt;
    /* Get average result */
    SumImage /= (int32_t)DftResCnt;
    /* Fix sign of imaginary part of DFT result. */
    SumImage = -SumImage;
    AppEDACfg.ExcitVolt.Real = SumReal;
    AppEDACfg.ExcitVolt.Image = SumImage;
    /* Done */
    /* Don't return voltage result */
    *pDataCount = 0;
#ifdef PROFILE_TIME_ENABLED
    voltage_measurement_diff_time =
        abs(get_sensor_time_stamp() - gnAd5940TimeCurVal) / 1000;
    voltage_cycles_diff_time =
        abs(get_micro_sec() - gnAd5940TimeCurValInMicroSec);
#endif
    /* The FIFO data is current result. We need to calculate impedance, Z=V/I */
  } else if (AppEDACfg.EDAStateCurr == EDASTATE_CURR) {
    iImpCar_Type *const pSrc = (iImpCar_Type *)pData;
    fImpCar_Type *const pOut = (fImpCar_Type *)pData;
    for (uint32_t i = 0; i < DataCount / 2; i++) {
      fImpCar_Type DftCurr;
      fImpCar_Type res;
      DftCurr.Real = (float)pSrc[i].Real;
      DftCurr.Image = (float)pSrc[i].Image;
      DftCurr.Image = -DftCurr.Image;
      DftCurr.Real = -DftCurr.Real;
      DftCurr.Image = -DftCurr.Image;
      /* I=Vrtia/Zrtia */
      res = AD5940_ComplexDivFloat(&DftCurr, &AppEDACfg.RtiaCurrValue);
      res = AD5940_ComplexDivFloat(&AppEDACfg.ExcitVolt, &res);
      AppEDACfg.ImpSum = AD5940_ComplexAddFloat(&AppEDACfg.ImpSum, &res);
      AppEDACfg.ImpSumCount++;
      res = AD5940_ComplexSubFloat(&res, &AppEDACfg.ImpEDABase);
      pOut[i] = res;
#ifdef PROFILE_TIME_ENABLED
      current_measurement_diff_time =
          abs(get_sensor_time_stamp() - gnAd5940TimeCurVal) / 1000;
      current_cycles_diff_time =
          abs(get_micro_sec() - gnAd5940TimeCurValInMicroSec);
#endif
    }
    /* Impedance result */
    *pDataCount = DataCount / 2;
    /* Process RTIA autoscaling calculation */
    if (AppEDACfg.RtiaAutoScaleEnable) {
      /* Init to invalid value */
      static uint32_t rtia_pre = (uint32_t)-1;
      uint32_t rtia_index;
      AppEDACfg.RtiaIndexCurr = AppEDACfg.RtiaIndexNext;
      AppEDACfg.RtiaCurrValue = AppEDACfg.RtiaCalTable[AppEDACfg.RtiaIndexCurr];
      rtia_index = EDARtiaAutoScaling(pOut, *pDataCount);
      if (rtia_index != rtia_pre) {
        AppEDACfg.bChangeRtia = bTRUE;
        rtia_pre = rtia_index;
        AppEDACfg.RtiaIndexNext = rtia_index;
      }
    }
  }
  AppEDACfg.EDAStateCurr = AppEDACfg.EDAStateNext;
  return AD5940ERR_OK;
}

/* magnitude and phase */
fImpCar_Type *pImp;
AD5940Err EDACalculateImpedance(void *pData, uint32_t DataCount) {
  float RtiaMag;
  /* Process data */
  pImp = (fImpCar_Type *)pData;
  AppEDACtrl(EDACTRL_GETRTIAMAG, &RtiaMag);
  /* Process data */
  for (int i = 0; i < DataCount; i++) {
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
    /* clear flag */
    measurement_cycle_completed = 0;
  }
  return 0;
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
  fImpCar_Type EDABase = {
      .Real = 24368.6,
      .Image = -16.696,
  };
  /* power on */
  adp5360_enable_ldo(ECG_LDO, true);

  /* configure call back */
  Ad5940DrvDataReadyCallback(Ad5940FifoCallBack);

  ad5940_port_Init();

  /* switch off other switches */
#ifdef ENABLE_ECG_APP
  DG2502_SW_control_AD8233(false);
#endif
  DG2502_SW_control_ADPD4000(false);
  /* clear interrupt flag */
  InitCfg();
  EDA_ERROR_CODE_t retVal = EDA_ERROR;
  ClearDataBufferAd5940();
  AD5940PlatformCfg();
  AD5940EDAStructInit();
  AppEDAInit(AppBuff, APPBUFF_SIZE);
  /* Control BIA measurment to start. Second parameter has no
   * meaning with this command. */
  AppEDACtrl(APPCTRL_START, 0);
  AppEDACtrl(EDACTRL_SETBASE, &EDABase);
  ResistorForBaseline = 19900;
  retVal = EDA_SUCCESS;
#ifdef PROFILE_TIME_ENABLED
  eda_init_diff_time = get_micro_sec() - eda_init_start_time;
#endif
  return retVal;
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