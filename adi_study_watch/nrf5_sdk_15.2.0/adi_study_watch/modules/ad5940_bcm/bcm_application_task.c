/**
***************************************************************************
* @addtogroup Tasks
* @{
* @file         bcm_application_task.c
* @author       ADI
* @version      V1.0.0
* @date         10-October-2019
* @brief        Source file contains BCM processing wrapper.
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
#ifdef ENABLE_BCM_APP
#include "bcm_application_task.h"
#include "app_bcm.h"
#include "dcb_interface.h"
#include <includes.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <ecg_task.h>
#include <adpd4000_dcfg.h>
#include <adi_dcb_config.h>
#include "fds.h"
#include "fds_drv.h"
#include <power_manager.h>
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
static volatile bool g_bcm_dcb_Present = false;
bool bcm_get_dcb_present_flag(void);
void bcm_set_dcb_present_flag(bool set_flag);
BCM_ERROR_CODE_t read_bcm_dcb(uint32_t *bcm_dcb_data, uint16_t *read_size);
BCM_ERROR_CODE_t write_bcm_dcb(uint32_t *bcm_dcb_data, uint16_t write_Size);
BCM_ERROR_CODE_t delete_bcm_dcb(void);
#endif
/////////////////////////////////////////

/* BCM app Task Module Log settings */
#define NRF_LOG_MODULE_NAME BCM_App_Task

#if BCM_APP_CONFIG_LOG_ENABLED
#ifdef NRF_LOG_DEFAULT_LEVEL
#undef NRF_LOG_DEFAULT_LEVEL
#define NRF_LOG_DEFAULT_LEVEL BCM_APP_CONFIG_LOG_LEVEL
#endif
#define NRF_LOG_INFO_COLOR  BCM_APP_CONFIG_INFO_COLOR
#define NRF_LOG_DEBUG_COLOR BCM_APP_CONFIG_DEBUG_COLOR
#else /* BCM_TASK_CONFIG_LOG_ENABLED */
#define NRF_LOG_LEVEL       0
#endif /* BCM_TASK_CONFIG_LOG_ENABLED */
#include "nrf_log.h"
/* Enable logger module */
NRF_LOG_MODULE_REGISTER();

#define APPBUFF_SIZE 1024
static uint32_t AppBuff[APPBUFF_SIZE];

const char GIT_BCM_VERSION[] = "TEST BCM_VERSION STRING";
const uint8_t GIT_BCM_VERSION_LEN = sizeof(GIT_BCM_VERSION);
static void packetize_bcm_raw_data(uint32_t *p_data,bcm_packetizer_t *p_pktizer, uint32_t timestamp);
static m2m2_hdr_t *bcm_app_reg_access(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *bcm_app_stream_config(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *bcm_app_status(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *bcm_app_get_version(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *bcm_app_decimation(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *bcm_app_dft_num_set(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *bcm_app_set_cal(m2m2_hdr_t *p_pkt);
#ifdef DEBUG_DCB
static m2m2_hdr_t *adi_dcb_fds_stat(m2m2_hdr_t *p_pkt);
#endif
static void fetch_bcm_data(void);
static void sensor_bcm_task(void *pArgument);
static uint16_t gnBcmSequenceCount = 0;
void Enable_ephyz_power(void);
static void InitCfg();
uint32_t ad5940_port_Init(void);
uint32_t ad5940_port_deInit(void);
#ifdef DCB
static m2m2_hdr_t *bcm_app_set_dcb_lcfg(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *bcm_dcb_command_read_config(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *bcm_dcb_command_write_config(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *bcm_dcb_command_delete_config(m2m2_hdr_t *p_pkt);
#endif
#ifdef PROFILE_TIME_ENABLED
static m2m2_hdr_t *bcm_app_dcb_timing_info(m2m2_hdr_t *p_pkt) ;
#endif

g_state_bcm_t g_state_bcm;

typedef  m2m2_hdr_t*(app_cb_function_t)(m2m2_hdr_t*);
static _gBcmData_t gBcmData = {{0},0,0,0};

static volatile int16_t user_applied_odr = 0;
static volatile int16_t user_applied_pga_gain = 0;
static volatile int16_t user_applied_bcm_dft_num = 0;
static volatile int16_t user_applied_bcm_hsrtia_sel = 0;
static volatile int16_t user_applied_bcm_pwr_mod = 0;
static volatile int16_t user_applied_sin_freq = 0;

uint8_t sensor_bcm_task_stack[APP_OS_CFG_BCM_TASK_STK_SIZE];
ADI_OSAL_THREAD_HANDLE sensor_bcm_task_handler;
ADI_OSAL_STATIC_THREAD_ATTR sensor_bcm_task_attributes;
StaticTask_t bcmTaskTcb;
ADI_OSAL_SEM_HANDLE   bcm_task_evt_sem;
extern uint32_t BcmFifoCount;
static uint32_t BcmSignalData[BCM_ARRAY_SIZE]={0},bcm_counter=0;
extern int var;

#ifdef DEBUG_BCM
static uint32_t pkt_count = 0;
static float bcm_odr = 0, time_elapsed_for_bcm = 0, bcm_start_time = 0;
#endif

AppBIACfg_Type AppBIACfg;

/*!
  ****************************************************************************
  *@brief      Initialize configurations for bcm application
  *@param      None
  *@return     None
******************************************************************************/
static void InitCfg()
{
  AppBIACfg.bParaChanged = bTRUE;
  AppBIACfg.SeqStartAddr = 0;
  AppBIACfg.MaxSeqLen = 0;

  AppBIACfg.SeqStartAddrCal = 0;
  AppBIACfg.MaxSeqLenCal = 0;

  /* Application related parameters */
  AppBIACfg.ReDoRtiaCal = bFALSE;
  AppBIACfg.SysClkFreq = 16000000.0;
  AppBIACfg.WuptClkFreq = 32000.0;
  AppBIACfg.AdcClkFreq = 16000000.0;
  AppBIACfg.NumOfData = -1;
  AppBIACfg.RcalVal = 10000.0; /* 10kOhm */

  if(!user_applied_bcm_pwr_mod) {
    AppBIACfg.PwrMod = AFEPWR_LP;
  } else {
    user_applied_bcm_pwr_mod = 0;
  }
  if(!user_applied_bcm_hsrtia_sel) {
    AppBIACfg.HstiaRtiaSel = HSTIARTIA_1K;
  } else {
    user_applied_bcm_hsrtia_sel = 0;
  }
  AppBIACfg.CtiaSel = 16;
  AppBIACfg.ExcitBufGain = EXCITBUFGAIN_2;
  AppBIACfg.HsDacGain = HSDACGAIN_1;
  AppBIACfg.HsDacUpdateRate = 7;
  AppBIACfg.DacVoltPP = 800.0;

  if(!user_applied_sin_freq) {
   AppBIACfg.SinFreq = 50000.0; /* 50kHz */
  } else {
    user_applied_sin_freq = 0;
  }
  if(!user_applied_pga_gain) {
    AppBIACfg.ADCPgaGain = ADCPGA_1;
  } else {
    user_applied_pga_gain = 0;
  }
  AppBIACfg.ADCSinc3Osr = ADCSINC3OSR_2;
  AppBIACfg.ADCSinc2Osr = ADCSINC2OSR_22;

  AppBIACfg.DftNum = DFTNUM_8192;
  AppBIACfg.DftSrc = DFTSRC_SINC3;
  AppBIACfg.HanWinEn = bTRUE;

  /* private variables */
  AppBIACfg.SweepCfg.SweepEn = bFALSE;
  AppBIACfg.SweepCfg.SweepStart = 10000;
  AppBIACfg.SweepCfg.SweepStop = 150000.0;
  AppBIACfg.SweepCfg.SweepPoints = 100;
  AppBIACfg.SweepCfg.SweepLog = bTRUE;
  AppBIACfg.SweepCfg.SweepIndex = 0;

  AppBIACfg.FifoThresh = 4;
  AppBIACfg.BIAInited = bFALSE;
  AppBIACfg.StopRequired = bFALSE;
  AppBIACfg.bRunning = bFALSE;
  AppBIACfg.MeasSeqCycleCount = 0;
}


/*!
 * @brief:  The dictionary type for mapping addresses to callback handlers.
 */
typedef struct _app_routing_table_entry_t {
  uint8_t                    command;
  app_cb_function_t          *cb_handler;
}app_routing_table_entry_t;

app_routing_table_entry_t bcm_app_routing_table[] = {
  {M2M2_APP_COMMON_CMD_STREAM_START_REQ, bcm_app_stream_config},
  {M2M2_APP_COMMON_CMD_STREAM_STOP_REQ, bcm_app_stream_config},
  {M2M2_APP_COMMON_CMD_STREAM_SUBSCRIBE_REQ, bcm_app_stream_config},
  {M2M2_APP_COMMON_CMD_STREAM_UNSUBSCRIBE_REQ, bcm_app_stream_config},
  {M2M2_APP_COMMON_CMD_SENSOR_STATUS_QUERY_REQ, bcm_app_status},
  {M2M2_APP_COMMON_CMD_READ_LCFG_REQ, bcm_app_reg_access},
  {M2M2_APP_COMMON_CMD_WRITE_LCFG_REQ, bcm_app_reg_access},
  {M2M2_SENSOR_COMMON_CMD_GET_STREAM_DEC_FACTOR_REQ, bcm_app_decimation},
  {M2M2_SENSOR_COMMON_CMD_SET_STREAM_DEC_FACTOR_REQ, bcm_app_decimation},
  {M2M2_APP_COMMON_CMD_GET_VERSION_REQ, bcm_app_get_version},
  {M2M2_BCM_APP_CMD_SET_DFT_NUM_REQ, bcm_app_dft_num_set},
  {M2M2_BCM_APP_CMD_SET_HS_RTIA_CAL_REQ, bcm_app_set_cal},
#ifdef DEBUG_DCB
  {M2M2_DCB_COMMAND_FDS_STATUS_REQ, adi_dcb_fds_stat},
#endif
#ifdef PROFILE_TIME_ENABLED
  {M2M2_APP_COMMON_CMD_DCB_TIMING_INFO_REQ, bcm_app_dcb_timing_info},
#endif
#ifdef DCB
    {M2M2_APP_COMMON_CMD_SET_LCFG_REQ, bcm_app_set_dcb_lcfg},
    {M2M2_DCB_COMMAND_READ_CONFIG_REQ, bcm_dcb_command_read_config},
    {M2M2_DCB_COMMAND_WRITE_CONFIG_REQ, bcm_dcb_command_write_config},
    {M2M2_DCB_COMMAND_ERASE_CONFIG_REQ, bcm_dcb_command_delete_config},
#endif
};

#define BCM_APP_ROUTING_TBL_SZ (sizeof(bcm_app_routing_table) / sizeof(bcm_app_routing_table[0]))

ADI_OSAL_QUEUE_HANDLE  bcm_task_msg_queue = NULL;

/*!
  ****************************************************************************
  *@brief       BCM Task initialization
  *@param       None
  *@return      None
******************************************************************************/
void ad5940_bcm_task_init(void) {
  /* Initialize app state */
  g_state_bcm.num_subs = 0;
  g_state_bcm.num_starts = 0;
  /* Default behaviour is to send every packet */
  g_state_bcm.decimation_factor = 1;
  g_state_bcm.decimation_nsamples = 0;
  g_state_bcm.data_pkt_seq_num = 0;

  ADI_OSAL_STATUS eOsStatus = ADI_OSAL_SUCCESS;
  sensor_bcm_task_attributes.pThreadFunc = sensor_bcm_task;
  sensor_bcm_task_attributes.nPriority = APP_OS_CFG_BCM_TASK_PRIO;
  sensor_bcm_task_attributes.pStackBase = &sensor_bcm_task_stack[0];
  sensor_bcm_task_attributes.nStackSize = APP_OS_CFG_BCM_TASK_STK_SIZE;
  sensor_bcm_task_attributes.pTaskAttrParam = NULL;
  sensor_bcm_task_attributes.szThreadName = "BCM Sensor";
  sensor_bcm_task_attributes.pThreadTcb = &bcmTaskTcb;

  eOsStatus = adi_osal_MsgQueueCreate(&bcm_task_msg_queue,NULL,5);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
    Debug_Handler();
  } else {
    update_task_queue_list(APP_OS_CFG_BCM_TASK_INDEX,bcm_task_msg_queue);
  }
  eOsStatus = adi_osal_ThreadCreateStatic(&sensor_bcm_task_handler,
                                          &sensor_bcm_task_attributes);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
    Debug_Handler();
  }

  adi_osal_SemCreate(&bcm_task_evt_sem, 0);
}

/*!
  ****************************************************************************
  *@brief      Called by the post office to send messages to this application
  *@param      p_pkt: packet of type m2m2_hdr_t
  *@return     None
******************************************************************************/
void send_message_ad5940_bcm_task(m2m2_hdr_t *p_pkt) {
  ADI_OSAL_STATUS osal_result;
  osal_result = adi_osal_MsgQueuePost(bcm_task_msg_queue,p_pkt);
  if (osal_result != ADI_OSAL_SUCCESS)
    post_office_consume_msg(p_pkt);
  adi_osal_SemPost(bcm_task_evt_sem);
}

/*!
  ****************************************************************************
  *@brief      BCM Task
  *@param      None
  *@return     None
******************************************************************************/
static void sensor_bcm_task(void *pArgument) {
  m2m2_hdr_t *p_in_pkt = NULL;
  m2m2_hdr_t *p_out_pkt = NULL;
  ADI_OSAL_STATUS         err;

  post_office_add_mailbox(M2M2_ADDR_MED_BCM, M2M2_ADDR_MED_BCM_STREAM);
  while (1) {
    adi_osal_SemPend(bcm_task_evt_sem, ADI_OSAL_TIMEOUT_FOREVER);
    p_in_pkt = post_office_get(ADI_OSAL_TIMEOUT_NONE, APP_OS_CFG_BCM_TASK_INDEX);
    if (p_in_pkt == NULL) {
      /* No m2m2 messages to process, so fetch some data from the device. */
      if(g_state_bcm.num_starts != 0) /* Fetch data from device only if the device is in active state(initialized properly)*/
        fetch_bcm_data();
    } else {
      /* Got an m2m2 message from the queue, process it. */
     PYLD_CST(p_in_pkt, _m2m2_app_common_cmd_t, p_in_cmd);
     /* Look up the appropriate function to call in the function table */
      for (int i = 0; i < BCM_APP_ROUTING_TBL_SZ; i++) {
        if (bcm_app_routing_table[i].command == p_in_cmd->command) {
          p_out_pkt = bcm_app_routing_table[i].cb_handler(p_in_pkt);
          break;
        }
      }
      post_office_consume_msg(p_in_pkt);
      if (p_out_pkt != NULL) {
        post_office_send(p_out_pkt, &err);
      }
    }
  }
}

/*!
  ****************************************************************************
  *@brief      Reset BCM packet counter
  *@param      None
  *@return     None
******************************************************************************/
static void reset_bcm_packetization(void) {
   g_state_bcm.bcm_pktizer.packet_nsamples = 0;
}


/*!
  ****************************************************************************
  *@brief      Fetch BCM data
  *@param      None
  *@return     None
******************************************************************************/
static void fetch_bcm_data(void){
  ADI_OSAL_STATUS         err;
  static uint32_t bcmData = 0;
  uint32_t bcmTS = 0;
  int8_t status = 0;
  ad5940_read_bcm_data_to_buffer();
  status = ad5950_buff_get(&bcmData, &bcmTS);

  /* reading a pair of samples for impedance calculation */
  BcmSignalData[bcm_counter++] = bcmData;

  while((status == AD5940Drv_SUCCESS) && (bcm_counter == BcmFifoCount)) {
    g_state_bcm.decimation_nsamples++;

    if (g_state_bcm.decimation_nsamples >= g_state_bcm.decimation_factor) {
      g_state_bcm.decimation_nsamples = 0;
      packetize_bcm_raw_data(BcmSignalData,&g_state_bcm.bcm_pktizer,bcmTS);

      if (g_state_bcm.bcm_pktizer.packet_nsamples >= g_state_bcm.bcm_pktizer.packet_max_nsamples) {
        adi_osal_EnterCriticalRegion();
        PYLD_CST(g_state_bcm.bcm_pktizer.p_pkt,bcm_app_stream_t, p_payload_ptr);
        p_payload_ptr->command = M2M2_SENSOR_COMMON_CMD_STREAM_DATA;
        p_payload_ptr->status = 0x00;
        p_payload_ptr->datatype = M2M2_SENSOR_BCM_DATA;
        g_state_bcm.bcm_pktizer.p_pkt->src = M2M2_ADDR_MED_BCM;
        g_state_bcm.bcm_pktizer.p_pkt->dest = M2M2_ADDR_MED_BCM_STREAM;
        p_payload_ptr->sequence_num = gnBcmSequenceCount++;
#ifdef DEBUG_PKT
        post_office_msg_cnt(g_state_bcm.bcm_pktizer.p_pkt);
#endif
#ifdef DEBUG_BCM
        pkt_count++;
        time_elapsed_for_bcm =  MCU_HAL_GetTick() - bcm_start_time;
        bcm_odr = ((pkt_count*4*1000)/time_elapsed_for_bcm);
        NRF_LOG_INFO("TIME ELAPSED = "NRF_LOG_FLOAT_MARKER, NRF_LOG_FLOAT(time_elapsed_for_bcm));
        NRF_LOG_INFO("START TIME = "NRF_LOG_FLOAT_MARKER, NRF_LOG_FLOAT(bcm_start_time));
        NRF_LOG_INFO("BCM MEASURED ODR = "NRF_LOG_FLOAT_MARKER, NRF_LOG_FLOAT(bcm_odr));
#endif
        post_office_send(g_state_bcm.bcm_pktizer.p_pkt, &err);
        adi_osal_ExitCriticalRegion();
        g_state_bcm.bcm_pktizer.packet_nsamples = 0;
        g_state_bcm.bcm_pktizer.packet_max_nsamples = 0;
        g_state_bcm.bcm_pktizer.p_pkt = NULL;
      }
    }
   /* Copy data from circular buffer data to soft buffer*/
    status = ad5950_buff_get(&bcmData, &bcmTS);
    BcmSignalData[bcm_counter++] = bcmData;
  }

  if(bcm_counter > BcmFifoCount){
      bcm_counter = 0;
  }
}

/*!
  ****************************************************************************
  *@brief      Packetize bcm data
  *@param      pSignalData: pointer to the bcm data
  *@param      pPkt: pointer to the packet structure
  *@param      timestamp: timestamp of the packet
  *@return     None
******************************************************************************/
static void packetize_bcm_raw_data(uint32_t *pBcmData,
                                  bcm_packetizer_t *p_pktizer,
                                  uint32_t timestamp) {
    /*   Calculate Impedance in form of Cartesian form */
    CalculateBioImpedance(pBcmData,BcmFifoCount);
    if (p_pktizer->packet_nsamples == 0) {
      p_pktizer->p_pkt = post_office_create_msg(sizeof(bcm_app_stream_t) + M2M2_HEADER_SZ);
      PYLD_CST(p_pktizer->p_pkt, bcm_app_stream_t, p_payload_ptr);
      g_state_bcm.bcm_pktizer.packet_max_nsamples=(sizeof(p_payload_ptr->bcm_data)/sizeof(p_payload_ptr->bcm_data[0]));
      p_payload_ptr->bcm_data[0].timestamp = timestamp;
      p_payload_ptr->bcm_data[0].real = gBcmData.bcmImpedence.real;
      p_payload_ptr->bcm_data[0].img = gBcmData.bcmImpedence.img;
      p_pktizer->packet_nsamples++;
    } else if (p_pktizer->packet_nsamples < p_pktizer->packet_max_nsamples) {
      PYLD_CST(p_pktizer->p_pkt, bcm_app_stream_t, p_payload_ptr);
      /*  one packet =  four samples
      first sample was added with above check
      and remaining 3 samples are added here */
      uint16_t i = p_pktizer->packet_nsamples;
      p_payload_ptr->bcm_data[i].timestamp = timestamp;
      p_payload_ptr->bcm_data[i].real = gBcmData.bcmImpedence.real;
      p_payload_ptr->bcm_data[i].img = gBcmData.bcmImpedence.img;
      p_pktizer->packet_nsamples++;
    }
}

/*!
  ****************************************************************************
  *@brief      Set BCM DFT number
  *@param      pPkt: pointer to the packet structure
  *@return     m2m2_hdr_t
******************************************************************************/
static m2m2_hdr_t *bcm_app_dft_num_set(m2m2_hdr_t *p_pkt) {
   /* Declare a pointer to access the input packet payload */
  PYLD_CST(p_pkt, bcm_app_set_dft_num_t, p_in_payload);

  /* Allocate memory to the response packet payload */
  PKT_MALLOC(p_resp_pkt, bcm_app_set_dft_num_t, 0);
  if (NULL != p_resp_pkt) {
  	/* Declare a pointer to the response packet payload */
  	PYLD_CST(p_resp_pkt, bcm_app_set_dft_num_t, p_resp_payload);

  	/* Copy dft number from payload if valid setting  */
  	if(p_in_payload->dftnum <= 12) {
   	NRF_LOG_INFO("Valid dft num sel");
    	AppBIACfg.DftNum = p_in_payload->dftnum;
    	user_applied_bcm_dft_num = 1;
  } else {
    /* default DFT number value */
    AppBIACfg.DftNum = DFTNUM_8192;
  }

  	p_resp_payload->command = M2M2_BCM_APP_CMD_SET_DFT_NUM_RESP;
 	p_resp_payload->status = M2M2_APP_COMMON_STATUS_OK;
  	p_resp_payload->dftnum = p_in_payload->dftnum;
  	p_resp_pkt->src = p_pkt->dest;
  	p_resp_pkt->dest = p_pkt->src;
  }
  return p_resp_pkt;
}

/*!
  ****************************************************************************
  *@brief      Get BCM application version
  *@param      pPkt: pointer to the packet structure
  *@return     m2m2_hdr_t
******************************************************************************/
static m2m2_hdr_t *bcm_app_get_version(m2m2_hdr_t *p_pkt) {
  /*  Allocate memory to response packet */
  PKT_MALLOC(p_resp_pkt, m2m2_app_common_version_t, 0);
  if (NULL != p_resp_pkt) {
  /* Declare a pointer to the response packet payload */
  PYLD_CST(p_resp_pkt, m2m2_app_common_version_t, p_resp_payload);

  p_resp_payload->command = M2M2_APP_COMMON_CMD_GET_VERSION_RESP;
  p_resp_payload->major = 0x03;
  p_resp_payload->minor = 0x04;
  p_resp_payload->patch = 0x03;
  memcpy(&p_resp_payload->verstr[0], "BCM_App",  8);
  memcpy(&p_resp_payload->str[0], &GIT_BCM_VERSION, GIT_BCM_VERSION_LEN);
  p_resp_pkt->src = p_pkt->dest;
  p_resp_pkt->dest = p_pkt->src;
  }
  return p_resp_pkt;
}

/*!
  ****************************************************************************
  *@brief      Get BCM application status
  *@param      pPkt: pointer to the packet structure
  *@return     m2m2_hdr_t
******************************************************************************/
static m2m2_hdr_t *bcm_app_status(m2m2_hdr_t *p_pkt) {
  /* Declare and malloc a response packet */
  PKT_MALLOC(p_resp_pkt, m2m2_app_common_status_t, 0);
  if (NULL != p_resp_pkt) {
  /* Declare a pointer to the response packet payload */
  PYLD_CST(p_resp_pkt, m2m2_app_common_status_t, p_resp_payload);

  p_resp_payload->command = M2M2_APP_COMMON_CMD_SENSOR_STATUS_QUERY_RESP;
  p_resp_payload->status = M2M2_APP_COMMON_STATUS_ERROR;

  if (g_state_bcm.num_starts == 0) {
    p_resp_payload->status = M2M2_APP_COMMON_STATUS_STREAM_STOPPED;
  } else {
    p_resp_payload->status = M2M2_APP_COMMON_STATUS_STREAM_STARTED;
  }
  p_resp_payload->stream = M2M2_ADDR_MED_BCM_STREAM;
  p_resp_payload->num_subscribers = g_state_bcm.num_subs;
  p_resp_payload->num_start_reqs = g_state_bcm.num_starts;
  p_resp_pkt->src = p_pkt->dest;
  p_resp_pkt->dest = p_pkt->src;
  }
  return p_resp_pkt;
}

#ifdef PROFILE_TIME_ENABLED
extern uint16_t update_entry;
extern uint16_t read_entry;
extern uint16_t del_rec;
extern uint16_t check_rec;
extern uint16_t clear_entries;

/*!
  ****************************************************************************
  *@brief      Get BCM application timing information
  *@param      pPkt: pointer to the packet structure
  *@return     m2m2_hdr_t
******************************************************************************/
static m2m2_hdr_t *bcm_app_dcb_timing_info(m2m2_hdr_t *p_pkt) {
   /* Declare a pointer to access the input packet payload */
  PYLD_CST(p_pkt, m2m2_dcb_fds_timing_info_req_t, p_in_payload);

  /* Allocate memory for response packet */
  PKT_MALLOC(p_resp_pkt, m2m2_dcb_fds_timing_info_resp_t, 0);
  /* Declare a pointer to the response packet payload */
  PYLD_CST(p_resp_pkt, m2m2_dcb_fds_timing_info_resp_t, p_resp_payload);

  p_resp_payload->command = M2M2_APP_COMMON_CMD_DCB_TIMING_INFO_RESP;
  p_resp_payload->status = M2M2_APP_COMMON_STATUS_OK;
  p_resp_payload->adi_dcb_clear_entries_time = clear_entries;
  p_resp_payload->adi_dcb_check_entries_time = check_rec;
  p_resp_payload->adi_dcb_delete_record_time = del_rec;
  p_resp_payload->adi_dcb_read_entry_time = read_entry;
  p_resp_payload->adi_dcb_update_entry_time =  update_entry;
  p_resp_pkt->src = p_pkt->dest;
  p_resp_pkt->dest = p_pkt->src;
  return p_resp_pkt;
}
#endif

/*!
  ****************************************************************************
  *@brief      Set high power loop calibration resitance
  *@param      pPkt: pointer to the packet structure
  *@return     m2m2_hdr_t
******************************************************************************/
static m2m2_hdr_t *bcm_app_set_cal(m2m2_hdr_t *p_pkt) {
  /* Declare a pointer to access the input packet payload */
  PYLD_CST(p_pkt, bcm_app_hs_rtia_sel_t, p_in_payload);

  /* Allocate memory for response payload */
  PKT_MALLOC(p_resp_pkt, bcm_app_hs_rtia_sel_t, 0);
  if (NULL != p_resp_pkt) {
  /* Declare a pointer to the response packet payload */
  PYLD_CST(p_resp_pkt, bcm_app_hs_rtia_sel_t, p_resp_payload);
  p_resp_payload->command = M2M2_BCM_APP_CMD_SET_HS_RTIA_CAL_RESP;
  p_resp_payload->status = M2M2_APP_COMMON_STATUS_OK;

  /* HS rtia selection  */
  AppBIACfg.HstiaRtiaSel = p_in_payload->hsritasel;
  NRF_LOG_INFO("HsRtiaSel=%d",AppBIACfg.HstiaRtiaSel);
  p_resp_payload->hsritasel = AppBIACfg.HstiaRtiaSel;
  user_applied_bcm_hsrtia_sel = 1;

  p_resp_pkt->src = p_pkt->dest;
  p_resp_pkt->dest = p_pkt->src;
  }
  return p_resp_pkt;
}

/*!
  ****************************************************************************
  *@brief      Set bcm stream configuration
  *@param      pPkt: pointer to the packet structure
  *@return     m2m2_hdr_t
******************************************************************************/
static m2m2_hdr_t *bcm_app_stream_config(m2m2_hdr_t *p_pkt) {
  M2M2_APP_COMMON_STATUS_ENUM_t status = M2M2_APP_COMMON_STATUS_ERROR;
  M2M2_APP_COMMON_CMD_ENUM_t    command;
  /* Declare a pointer to access the input packet payload */
  PYLD_CST(p_pkt, m2m2_app_common_sub_op_t, p_in_payload);
  /* Declare and malloc a response packet */
  PKT_MALLOC(p_resp_pkt, m2m2_app_common_sub_op_t, 0);
  /* Declare a pointer to the response packet payload */
  PYLD_CST(p_resp_pkt, m2m2_app_common_sub_op_t, p_resp_payload);

  switch (p_in_payload->command) {
  case M2M2_APP_COMMON_CMD_STREAM_START_REQ:
      /*  BCM variable app */
      var = 3;
      reset_bcm_packetization();
     if (g_state_bcm.num_starts == 0) {
      if (BcmAppInit() == BCM_SUCCESS) {
        g_state_bcm.num_starts = 1;
#ifdef DEBUG_BCM
        bcm_start_time = MCU_HAL_GetTick();
        pkt_count = 0;
#endif
        status = M2M2_APP_COMMON_STATUS_STREAM_STARTED;
      } else {
        status = M2M2_APP_COMMON_STATUS_STREAM_NOT_STARTED;
      }
    } else {
      g_state_bcm.num_starts++;
      status = M2M2_APP_COMMON_STATUS_STREAM_IN_PROGRESS;
    }
    command = M2M2_APP_COMMON_CMD_STREAM_START_RESP;
    break;
  case M2M2_APP_COMMON_CMD_STREAM_STOP_REQ:
    if (g_state_bcm.num_starts == 0) {
      status = M2M2_APP_COMMON_STATUS_STREAM_STOPPED;
    } else if (g_state_bcm.num_starts == 1) {
      if (BcmAppDeInit()) {
      g_state_bcm.num_starts = 0;
      status = M2M2_APP_COMMON_STATUS_STREAM_STOPPED;
      } else {
        g_state_bcm.num_starts = 1;
        status = M2M2_APP_COMMON_STATUS_ERROR;
      }
    } else {
      g_state_bcm.num_starts--;
      status = M2M2_APP_COMMON_STATUS_STREAM_COUNT_DECREMENT;
    }
    command = M2M2_APP_COMMON_CMD_STREAM_STOP_RESP;
    break;
  case M2M2_APP_COMMON_CMD_STREAM_SUBSCRIBE_REQ:
    g_state_bcm.num_subs++;
    post_office_setup_subscriber(M2M2_ADDR_MED_BCM, M2M2_ADDR_MED_BCM_STREAM, p_pkt->src, true);
    status = M2M2_APP_COMMON_STATUS_SUBSCRIBER_ADDED;
    command = M2M2_APP_COMMON_CMD_STREAM_SUBSCRIBE_RESP;
    break;
  case M2M2_APP_COMMON_CMD_STREAM_UNSUBSCRIBE_REQ:
    if (g_state_bcm.num_subs <= 1) {
      g_state_bcm.num_subs = 0;
      status = M2M2_APP_COMMON_STATUS_SUBSCRIBER_REMOVED;
    } else {
      g_state_bcm.num_subs--;
      status = M2M2_APP_COMMON_STATUS_SUBSCRIBER_COUNT_DECREMENT;
    }
    post_office_setup_subscriber(M2M2_ADDR_MED_BCM, M2M2_ADDR_MED_BCM_STREAM, p_pkt->src, false);
    command = M2M2_APP_COMMON_CMD_STREAM_UNSUBSCRIBE_RESP;
    break;
  default:
    /* Something has gone horribly wrong. */
    post_office_consume_msg(p_resp_pkt);
    return NULL;
  }
  p_resp_payload->command = command;
  p_resp_payload->status = status;
  p_resp_payload->stream = p_in_payload->stream;
  p_resp_pkt->src = p_pkt->dest;
  p_resp_pkt->dest = p_pkt->src;
  return p_resp_pkt;
}

/*!
  ****************************************************************************
  *@brief      Bcm stream register access
  *@param      pPkt: pointer to the packet structure
  *@return     m2m2_hdr_t
******************************************************************************/
static m2m2_hdr_t *bcm_app_reg_access(m2m2_hdr_t *p_pkt) {
  M2M2_APP_COMMON_STATUS_ENUM_t status = M2M2_APP_COMMON_STATUS_ERROR;
  /* Declare a pointer to access the input packet payload */
  PYLD_CST(p_pkt, bcm_app_lcfg_op_hdr_t, p_in_payload);
  /* Allocate a response packet with space for the correct number of operations */
  PKT_MALLOC(p_resp_pkt, bcm_app_lcfg_op_hdr_t, p_in_payload->num_ops * sizeof(p_in_payload->ops[0]));
  if (NULL != p_resp_pkt) {
  PYLD_CST(p_resp_pkt, bcm_app_lcfg_op_hdr_t, p_resp_payload);
  uint64_t  reg_data = 0;

  switch (p_in_payload->command) {
  case M2M2_APP_COMMON_CMD_READ_LCFG_REQ:
    for (int i = 0; i < p_in_payload->num_ops; i++) {
      if (BcmReadLCFG(p_in_payload->ops[i].field, &reg_data) == BCM_SUCCESS) {
        status = M2M2_APP_COMMON_STATUS_OK;
      } else {
        status = M2M2_APP_COMMON_STATUS_ERROR;
      }
      p_resp_payload->ops[i].field = p_in_payload->ops[i].field;
      p_resp_payload->ops[i].value = reg_data;
    }
    p_resp_payload->command = (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_APP_COMMON_CMD_READ_LCFG_RESP;
    break;
  case M2M2_APP_COMMON_CMD_WRITE_LCFG_REQ:
    for (int i = 0; i < p_in_payload->num_ops; i++) {
      if (BcmWriteLCFG(p_in_payload->ops[i].field, p_in_payload->ops[i].value) == BCM_SUCCESS) {
        status = M2M2_APP_COMMON_STATUS_OK;
      } else {
        status = M2M2_APP_COMMON_STATUS_ERROR;
      }
      p_resp_payload->ops[i].field = p_in_payload->ops[i].field;
      p_resp_payload->ops[i].value = p_in_payload->ops[i].value;
    }
    p_resp_payload->command = (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_APP_COMMON_CMD_WRITE_LCFG_RESP;
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


/*!
  ****************************************************************************
 * @brief  Bcm App decimation
 * @param  None
 * @return ECG_ERROR_CODE_t: Success/Error
 *****************************************************************************/
static m2m2_hdr_t *bcm_app_decimation(m2m2_hdr_t *p_pkt) {
  M2M2_APP_COMMON_STATUS_ENUM_t status = M2M2_APP_COMMON_STATUS_OK;

  /* Declare a pointer to access the input packet payload */
  PYLD_CST(p_pkt, m2m2_sensor_common_decimate_stream_t, p_in_payload);

  /* Allocate a response packet with space for the correct number of operations */
  PKT_MALLOC(p_resp_pkt, m2m2_sensor_common_decimate_stream_t, 0);
  if (NULL != p_resp_pkt) {
  PYLD_CST(p_resp_pkt, m2m2_sensor_common_decimate_stream_t, p_resp_payload);

  switch (p_in_payload->command) {
  case M2M2_SENSOR_COMMON_CMD_GET_STREAM_DEC_FACTOR_REQ:
    p_resp_payload->command = M2M2_SENSOR_COMMON_CMD_GET_STREAM_DEC_FACTOR_RESP;
    break;
  case M2M2_SENSOR_COMMON_CMD_SET_STREAM_DEC_FACTOR_REQ:
    g_state_bcm.decimation_factor = p_in_payload->dec_factor;
    if (g_state_bcm.decimation_factor == 0) {
      g_state_bcm.decimation_factor = 1;
    }
    p_resp_payload->command = M2M2_SENSOR_COMMON_CMD_SET_STREAM_DEC_FACTOR_RESP;
    break;
  }
  p_resp_payload->status = status;
  p_resp_payload->dec_factor = g_state_bcm.decimation_factor;
  p_resp_pkt->dest = p_pkt->src;
  p_resp_pkt->src = p_pkt->dest;
  }
  return p_resp_pkt;
}

/*!
  ****************************************************************************
 * @brief  Bcm App Initialization
 * @param  None
 * @return ECG_ERROR_CODE_t: Success/Error
 *****************************************************************************/
BCM_ERROR_CODE_t BcmAppInit() {
  BCM_ERROR_CODE_t retVal = BCM_ERROR;
  /* Ldo power on */
  adp5360_enable_ldo(ECG_LDO,true);

  /* configure call back */
  Ad5940DrvDataReadyCallback(Ad5940FifoCallBack);

  /* Interrupts setting */
  ad5940_port_Init();

  /* switch off other switches */
#ifdef ENABLE_ECG_APP
  DG2502_SW_control_AD8233(false);
#endif
  DG2502_SW_control_ADPD4000(false);

  /* Initialization of application parameters */
  InitCfg();
  /* clear AD5940 soft buffer */
  ClearDataBufferAd5940();
  /* default parameters setting */
  AD5940PlatformCfg();
  /* BIA Initialization */
  /* Configure your parameters in this function */
  AD5940BIAStructInit();
  /* Initialize BIA application. Provide a buffer, which is used to store sequencer commands */
  AppBIAInit(AppBuff, APPBUFF_SIZE);
  /* Control BIA measurment to start. Second parameter has no meaning with this command. */
  AppBIACtrl(APPCTRL_START, 0);
  NRF_LOG_INFO("Bia ODR=%d",AppBIACfg.BiaODR);
  retVal = BCM_SUCCESS;

  return retVal;

}

/*!
  ****************************************************************************
 * @brief  Bcm App de Initialization
 * @param  None
 * @return BCM_ERROR_CODE_t: Success/Error
 *****************************************************************************/
BCM_ERROR_CODE_t BcmAppDeInit() {
  /* BIA stop sync */
  if(AppBIACtrl(APPCTRL_STOPSYNC, 0)!=0){
    return BCM_ERROR;
  }
  /* Interrupts de init */
  ad5940_port_deInit();

  /* de init ldo power */
  adp5360_enable_ldo(ECG_LDO,false);
  return BCM_SUCCESS;
}

/*!
  ****************************************************************************
  * @brief    Example of how to write an LCFG parameter
  * @param    field: LCFG field that has to be written
  * @param    value: Value to be written
  * @retval   BCM_ERROR_CODE_t
 *****************************************************************************/
BCM_ERROR_CODE_t BcmWriteLCFG(uint8_t field, uint64_t value) {
  AppBIACfg_Type *pCfg;
  AppBIAGetCfg(&pCfg);
  if(field < BCM_LCFG_MAX){
    switch(field){
    case BCM_LCFG_FS: /* field 0 */
      pCfg->BiaODR = value;
      user_applied_odr = 1;
      break;
    case BCM_LCFG_ADC_PGA_GAIN: /* field 1 */
      pCfg->ADCPgaGain = value;
      user_applied_pga_gain = 1;
      break;
    case BCM_LCFG_PWR_MOD:/* field 2 */
      pCfg->PwrMod = value;
      user_applied_bcm_pwr_mod = 1;
      break;
   case BCM_LCFG_SIN_FREQ: /* field 3 */
      pCfg->SinFreq= value;
      user_applied_sin_freq = 1;
      break;
    }
    return BCM_SUCCESS;
  }
  return BCM_ERROR;
}

/*!
  ****************************************************************************
  * @brief    Read LCFG parameter
  * @param    index: LCFG field
  * @param    value: Returned corresponding LCFG value
  * @retval   BCM_ERROR_CODE_t
 *****************************************************************************/
BCM_ERROR_CODE_t BcmReadLCFG(uint8_t index, uint64_t *value) {
AppBIACfg_Type *pCfg;
AppBIAGetCfg(&pCfg);
if(index < BCM_LCFG_MAX){
    switch(index){
    case BCM_LCFG_FS: /* field 0 */
      *value = (uint64_t)(pCfg->BiaODR);
      break;
    case BCM_LCFG_ADC_PGA_GAIN: /* field 1 */
      *value = pCfg->ADCPgaGain;
      break;
    case BCM_LCFG_PWR_MOD: /* field 2 */
      *value = pCfg->PwrMod;
      break;
    case BCM_LCFG_SIN_FREQ: /* field 3 */
      *value =  (uint64_t)(pCfg->SinFreq);
      break;
   }
    return BCM_SUCCESS;
  }
  return BCM_ERROR;
}

/*!
  ****************************************************************************
  *@brief      Get Bcm application structure
  *@param      pCfg: void pointer
  *@return     AD5940ERR_OK/AD5940ERR_PARA
******************************************************************************/
AD5940Err AppBIAGetCfg(void *pCfg) {
  if(pCfg) {
    *(AppBIACfg_Type**)pCfg = &AppBIACfg;
    return AD5940ERR_OK;
  }
  return AD5940ERR_PARA;
}

/*!
  ****************************************************************************
 *  @brief Control application like start, stop

 *  @param  BIACtrl The command for this applicaiton, select from below paramters
 *            - APPCTRL_START: start the measurment. Note: the ramp test need
 *              to call function AppRAMPInit() every time before start it.
 *            - APPCTRL_STOPNOW: Stop the measurment immediately.
 *            - APPCTRL_STOPSYNC: Stop the measuremnt when current measured data
 *                     is read back.
 *            - BIACTRL_GETFREQ: Get Sine frequency.
 *            - APPCTRL_SHUTDOWN: Stop the measurment immediately and put AFE to
 *                    shut down mode(turn off LP loop and enter hibernate).
 *            - APPCTRL_RUNNING: Read state if running.
  *@return            AD5940ERR_OK/AD5940ERR_PARA
******************************************************************************/
AD5940Err AppBIACtrl(int32_t BcmCtrl, void *pPara) {
  switch (BcmCtrl) {
    case APPCTRL_START:
    {
      WUPTCfg_Type wupt_cfg;
      if(AD5940_WakeUp(10) > 10)  /* Wakeup AFE by read register, read 10 times at most */
        return AD5940ERR_WAKEUP;  /* Wakeup Failed */
      if(AppBIACfg.BIAInited == bFALSE)
        return AD5940ERR_APPERROR;
      /* Start it */
      wupt_cfg.WuptEn = bTRUE;
      wupt_cfg.WuptEndSeq = WUPTENDSEQ_A;
      wupt_cfg.WuptOrder[0] = SEQID_0;
      wupt_cfg.SeqxSleepTime[SEQID_0] = (uint32_t)(AppBIACfg.WuptClkFreq/AppBIACfg.BiaODR)-2-1;
      /* The minimum value is 1. Do not set it to zero. Set it to 1 will spend 2 32kHz clock. */
      wupt_cfg.SeqxWakeupTime[SEQID_0] = 1;
      AD5940_WUPTCfg(&wupt_cfg);

      AppBIACfg.FifoDataCount = 0;  /* restart */
	  AppBIACfg.bRunning = bTRUE;
      break;
    }
    case APPCTRL_STOPNOW:
    {
      if(AD5940_WakeUp(10) > 10)  /* Wakeup AFE by read register, read 10 times at most */
        return AD5940ERR_WAKEUP;  /* Wakeup Failed */
      /* Start Wupt right now */
      AD5940_WUPTCtrl(bFALSE);
      AD5940_WUPTCtrl(bFALSE);  /* @todo is it sure this will stop Wupt? */
	  AppBIACfg.bRunning = bFALSE;
      break;
    }
    case APPCTRL_STOPSYNC:
    {
      AppBIACfg.StopRequired = bTRUE;
      break;
    }
    case BIACTRL_GETFREQ:
    if(pPara)
    {
      if(AppBIACfg.SweepCfg.SweepEn == bTRUE)
        *(float*)pPara = AppBIACfg.FreqofData;
      else
        *(float*)pPara = AppBIACfg.SinFreq;
    }
    break;
    case APPCTRL_SHUTDOWN:
    {
      /* Stop the measurment if it's running. */
      AppBIACtrl(APPCTRL_STOPNOW, 0);
      /* Turn off LPloop related blocks which are not controlled
       * automatically by sleep operation */
      AFERefCfg_Type aferef_cfg;
      LPLoopCfg_Type lp_loop;
      memset(&aferef_cfg, 0, sizeof(aferef_cfg));
      AD5940_REFCfgS(&aferef_cfg);
      memset(&lp_loop, 0, sizeof(lp_loop));
      AD5940_LPLoopCfgS(&lp_loop);
      AD5940_EnterSleepS();  /* Enter Hibernate */
    }
    break;
    case APPCTRL_RUNNING:
      if(pPara == NULL)
        return AD5940ERR_NULLP; /* Null pointer */
    *(BoolFlag*)pPara = AppBIACfg.bRunning;
    break;
   default:
    break;
  }
  return AD5940ERR_OK;
}

/*!
  ****************************************************************************
  *@brief      BIA sequence generation
  *@param      None
  *@return     AD5940Err/AD5940ERR_OK
******************************************************************************/
static AD5940Err AppBIASeqCfgGen(void)
{
  AD5940Err error = AD5940ERR_OK;
  uint32_t const *pSeqCmd;
  uint32_t SeqLen;

  AFERefCfg_Type aferef_cfg;
  HSLoopCfg_Type hs_loop;
  LPLoopCfg_Type lp_loop;
  DSPCfg_Type dsp_cfg;
  float sin_freq;

  /* Start sequence generator here */
  AD5940_SEQGenCtrl(bTRUE);

  /* analog front end configuration */
  aferef_cfg.HpBandgapEn = bTRUE;
  aferef_cfg.Hp1V1BuffEn = bTRUE;
  aferef_cfg.Hp1V8BuffEn = bTRUE;
  aferef_cfg.Disc1V1Cap = bFALSE;
  aferef_cfg.Disc1V8Cap = bFALSE;
  aferef_cfg.Hp1V8ThemBuff = bFALSE;
  aferef_cfg.Hp1V8Ilimit = bFALSE;
  aferef_cfg.Lp1V1BuffEn = bFALSE;
  aferef_cfg.Lp1V8BuffEn = bFALSE;

  /* LP reference control - turn off them to save powr*/
  aferef_cfg.LpBandgapEn = bTRUE;
  aferef_cfg.LpRefBufEn = bTRUE;
  aferef_cfg.LpRefBoostEn = bFALSE;
  AD5940_REFCfgS(&aferef_cfg);

  /* high power loop configuration */
  hs_loop.HsDacCfg.ExcitBufGain = AppBIACfg.ExcitBufGain;
  hs_loop.HsDacCfg.HsDacGain = AppBIACfg.HsDacGain;
  hs_loop.HsDacCfg.HsDacUpdateRate = AppBIACfg.HsDacUpdateRate;
  hs_loop.HsTiaCfg.DiodeClose = bFALSE;
  hs_loop.HsTiaCfg.HstiaBias = HSTIABIAS_1P1;
  hs_loop.HsTiaCfg.HstiaCtia = AppBIACfg.CtiaSel; /* 31pF + 2pF */
  hs_loop.HsTiaCfg.HstiaDeRload = HSTIADERLOAD_OPEN;
  hs_loop.HsTiaCfg.HstiaDeRtia = HSTIADERTIA_OPEN;
  hs_loop.HsTiaCfg.HstiaRtiaSel = AppBIACfg.HstiaRtiaSel;

  /* Configure switches */
  hs_loop.SWMatCfg.Dswitch = SWD_OPEN;
  hs_loop.SWMatCfg.Pswitch = SWP_PL|SWP_PL2;
  hs_loop.SWMatCfg.Nswitch = SWN_NL|SWN_NL2;
  hs_loop.SWMatCfg.Tswitch = SWT_TRTIA;

  /* high power loop wave form generator configuration */
  hs_loop.WgCfg.WgType = WGTYPE_SIN;
  hs_loop.WgCfg.GainCalEn = bFALSE;
  hs_loop.WgCfg.OffsetCalEn = bFALSE;

   /* If sweep enabled, perform sweep operation */
  if(AppBIACfg.SweepCfg.SweepEn == bTRUE) {
    AppBIACfg.SweepCfg.SweepIndex = 0;
    AppBIACfg.FreqofData = AppBIACfg.SweepCfg.SweepStart;
    AppBIACfg.SweepCurrFreq = AppBIACfg.SweepCfg.SweepStart;
    AD5940_SweepNext(&AppBIACfg.SweepCfg, &AppBIACfg.SweepNextFreq);
    sin_freq = AppBIACfg.SweepCurrFreq;
  } else {
    sin_freq = AppBIACfg.SinFreq;
    AppBIACfg.FreqofData = sin_freq;
  }

  /* high power loop sine signal settings */
  hs_loop.WgCfg.SinCfg.SinFreqWord = AD5940_WGFreqWordCal(sin_freq, AppBIACfg.SysClkFreq);
  hs_loop.WgCfg.SinCfg.SinAmplitudeWord = (uint32_t)(AppBIACfg.DacVoltPP/800.0f*2047 + 0.5f);
  hs_loop.WgCfg.SinCfg.SinOffsetWord = 0;
  hs_loop.WgCfg.SinCfg.SinPhaseWord = 0;
  AD5940_HSLoopCfgS(&hs_loop);

  /* low power loop dac configuration settings */
  lp_loop.LpDacCfg.LpdacSel = LPDAC0;
  lp_loop.LpDacCfg.LpDacSrc = LPDACSRC_MMR;
  lp_loop.LpDacCfg.LpDacSW = LPDACSW_VBIAS2LPPA|LPDACSW_VBIAS2PIN|LPDACSW_VZERO2LPTIA|LPDACSW_VZERO2PIN;
  lp_loop.LpDacCfg.LpDacVzeroMux = LPDACVZERO_6BIT;
  lp_loop.LpDacCfg.LpDacVbiasMux = LPDACVBIAS_12BIT;
  lp_loop.LpDacCfg.LpDacRef = LPDACREF_2P5;
  lp_loop.LpDacCfg.DataRst = bFALSE;
  lp_loop.LpDacCfg.PowerEn = bTRUE;
  lp_loop.LpDacCfg.DacData12Bit = (uint32_t)((1100-200)/2200.0*4095);
  lp_loop.LpDacCfg.DacData6Bit = 31;

  /* low power loop amplifier settings */
  lp_loop.LpAmpCfg.LpAmpSel = LPAMP0;
  lp_loop.LpAmpCfg.LpAmpPwrMod = LPAMPPWR_NORM;
  lp_loop.LpAmpCfg.LpPaPwrEn = bTRUE;
  lp_loop.LpAmpCfg.LpTiaPwrEn = bTRUE;
  lp_loop.LpAmpCfg.LpTiaRf = LPTIARF_20K;
  lp_loop.LpAmpCfg.LpTiaRload = LPTIARLOAD_SHORT;
  lp_loop.LpAmpCfg.LpTiaRtia = LPTIARTIA_OPEN;
  lp_loop.LpAmpCfg.LpTiaSW = LPTIASW(5)|LPTIASW(6)|LPTIASW(7)|LPTIASW(8)|\
                             LPTIASW(9)|LPTIASW(12)|LPTIASW(13);
  AD5940_LPLoopCfgS(&lp_loop);

  /* dsp configuration ADC settings */
  dsp_cfg.ADCBaseCfg.ADCMuxN = ADCMUXN_HSTIA_N;
  dsp_cfg.ADCBaseCfg.ADCMuxP = ADCMUXP_HSTIA_P;
  dsp_cfg.ADCBaseCfg.ADCPga = AppBIACfg.ADCPgaGain;

  memset(&dsp_cfg.ADCDigCompCfg, 0, sizeof(dsp_cfg.ADCDigCompCfg));
  /* Don't care because it's disabled */
  dsp_cfg.ADCFilterCfg.ADCAvgNum = ADCAVGNUM_16;
  /* Tell filter block clock rate of ADC*/
  dsp_cfg.ADCFilterCfg.ADCRate = ADCRATE_800KHZ;
  dsp_cfg.ADCFilterCfg.ADCSinc2Osr = AppBIACfg.ADCSinc2Osr;
  dsp_cfg.ADCFilterCfg.ADCSinc3Osr = AppBIACfg.ADCSinc3Osr;
  dsp_cfg.ADCFilterCfg.BpSinc3 = bFALSE;
  dsp_cfg.ADCFilterCfg.BpNotch = bTRUE;
  dsp_cfg.ADCFilterCfg.Sinc2NotchEnable = bTRUE;
  dsp_cfg.DftCfg.DftNum = AppBIACfg.DftNum;
  dsp_cfg.DftCfg.DftSrc = AppBIACfg.DftSrc;
  dsp_cfg.DftCfg.HanWinEn = AppBIACfg.HanWinEn;

  memset(&dsp_cfg.StatCfg, 0, sizeof(dsp_cfg.StatCfg));
  AD5940_DSPCfgS(&dsp_cfg);

  /* Enable all of them. They are automatically turned off during hibernate mode to save power */
  AD5940_AFECtrlS(AFECTRL_HPREFPWR|AFECTRL_HSTIAPWR|AFECTRL_INAMPPWR|AFECTRL_EXTBUFPWR|\
                AFECTRL_WG|AFECTRL_DACREFPWR|AFECTRL_HSDACPWR|\
                AFECTRL_SINC2NOTCH, bTRUE);
  /* GP6->endSeq, GP5 -> AD8233=OFF, GP1->RLD=OFF */
  AD5940_SEQGpioCtrlS(0);

  /* Sequence end. */
  /* Add one external command to disable sequencer for
   * initialization sequence because we only want it to run one time. */
  AD5940_SEQGenInsert(SEQ_STOP());

  /* Stop here */
  error = AD5940_SEQGenFetchSeq(&pSeqCmd, &SeqLen);
  AD5940_SEQGenCtrl(bFALSE); /* Stop sequencer generator */
  if(error == AD5940ERR_OK) {
    AppBIACfg.InitSeqInfo.SeqId = SEQID_1;
    AppBIACfg.InitSeqInfo.SeqRamAddr = AppBIACfg.SeqStartAddr;
    AppBIACfg.InitSeqInfo.pSeqCmd = pSeqCmd;
    AppBIACfg.InitSeqInfo.SeqLen = SeqLen;
    /* Write command to SRAM */
    AD5940_SEQCmdWrite(AppBIACfg.InitSeqInfo.SeqRamAddr, pSeqCmd, SeqLen);
  } else {
    return error; /* Error */
  }
  return AD5940ERR_OK;
}

/*!
  ****************************************************************************
  *@brief      BIA sequence measurement generation
  *@param      None
  *@return     AD5940Err/AD5940ERR_OK
******************************************************************************/
static AD5940Err AppBIASeqMeasureGen(void) {
  AD5940Err error = AD5940ERR_OK;
  uint32_t const *pSeqCmd;
  uint32_t SeqLen;

  uint32_t WaitClks;
  SWMatrixCfg_Type sw_cfg;
  ClksCalInfo_Type clks_cal;

  clks_cal.DataType = DATATYPE_DFT;
  clks_cal.DftSrc = AppBIACfg.DftSrc;
  /* Enable DFT data count 2^(DFTNUMBER+2) */
  clks_cal.DataCount = 1L<<(AppBIACfg.DftNum+2);
  clks_cal.ADCSinc2Osr = AppBIACfg.ADCSinc2Osr;
  clks_cal.ADCSinc3Osr = AppBIACfg.ADCSinc3Osr;
  clks_cal.ADCAvgNum = 0;
  clks_cal.BpNotch = bFALSE;

  clks_cal.RatioSys2AdcClk = AppBIACfg.SysClkFreq/AppBIACfg.AdcClkFreq;
  AD5940_ClksCalculate(&clks_cal, &WaitClks);

  /* Start sequence generator here */
  AD5940_SEQGenCtrl(bTRUE);
  /* GP6->endSeq, GP5 -> AD8233=OFF, GP1->RLD=OFF */
  AD5940_SEQGpioCtrlS(AGPIO_Pin6);

  /* wait 250*16 micro sec*/
  AD5940_SEQGenInsert(SEQ_WAIT(16*250));
  sw_cfg.Dswitch = SWD_CE0;
  sw_cfg.Pswitch = SWP_CE0;
  sw_cfg.Nswitch = SWN_AIN1;
  sw_cfg.Tswitch = SWT_AIN1|SWT_TRTIA;
  AD5940_SWMatrixCfgS(&sw_cfg);
  AD5940_ADCMuxCfgS(ADCMUXP_HSTIA_P, ADCMUXN_HSTIA_N);
  /* Enable Waveform generator, ADC power */
  AD5940_AFECtrlS(AFECTRL_WG|AFECTRL_ADCPWR, bTRUE);
  /* Enable sequencer wait */
  AD5940_SEQGenInsert(SEQ_WAIT(16*50));
  /* Start ADC convert and DFT */
  AD5940_AFECtrlS(AFECTRL_ADCCNV|AFECTRL_DFT, bTRUE);
  /* wait for first data ready */
  AD5940_SEQGenInsert(SEQ_WAIT(WaitClks));
  AD5940_AFECtrlS(AFECTRL_ADCCNV|AFECTRL_DFT|AFECTRL_WG|\
                  AFECTRL_ADCPWR, bFALSE);  /* Stop ADC convert and DFT */

  AD5940_ADCMuxCfgS(ADCMUXP_AIN3, ADCMUXN_AIN2);
  /* Enable Waveform generator, ADC power */
  AD5940_AFECtrlS(AFECTRL_WG|AFECTRL_ADCPWR, bTRUE);
  /* delay for signal settling DFT_WAIT */
  AD5940_SEQGenInsert(SEQ_WAIT(16*50));
  /* Start ADC convert and DFT */
  AD5940_AFECtrlS(AFECTRL_ADCCNV|AFECTRL_DFT, bTRUE);
  /* wait for first data ready */
  AD5940_SEQGenInsert(SEQ_WAIT(WaitClks));
  /* Stop ADC convert and DFT */
  AD5940_AFECtrlS(AFECTRL_ADCCNV|AFECTRL_DFT|AFECTRL_WG|AFECTRL_ADCPWR, bFALSE);

  /* Configure float switches */
  sw_cfg.Dswitch = SWD_OPEN;
  sw_cfg.Pswitch = SWP_PL|SWP_PL2;
  sw_cfg.Nswitch = SWN_NL|SWN_NL2;
  sw_cfg.Tswitch = SWT_TRTIA;
  AD5940_SWMatrixCfgS(&sw_cfg);
  /* GP6->endSeq, GP5 -> AD8233=OFF, GP1->RLD=OFF .*/
  AD5940_SEQGpioCtrlS(0);
  /* Goto hibernate */
  AD5940_EnterSleepS();
  /* Sequence end. */
  error = AD5940_SEQGenFetchSeq(&pSeqCmd, &SeqLen);
  /* Stop sequencer generator */
  AD5940_SEQGenCtrl(bFALSE);
  AppBIACfg.MeasSeqCycleCount = AD5940_SEQCycleTime();
  AppBIACfg.MaxODR = 1/(((AppBIACfg.MeasSeqCycleCount + 10) / 16.0)* 1E-6)  ;
  if(AppBIACfg.BiaODR > AppBIACfg.MaxODR) {
    /* We have requested a sampling rate that cannot be achieved with the time it
       takes to acquire a sample.
    */
    AppBIACfg.BiaODR = AppBIACfg.MaxODR;
  }

  if(error == AD5940ERR_OK) {
    /* Sequencer configuration */
    AppBIACfg.MeasureSeqInfo.SeqId = SEQID_0;
    AppBIACfg.MeasureSeqInfo.SeqRamAddr = AppBIACfg.InitSeqInfo.SeqRamAddr + AppBIACfg.InitSeqInfo.SeqLen ;
    AppBIACfg.MeasureSeqInfo.pSeqCmd = pSeqCmd;
    AppBIACfg.MeasureSeqInfo.SeqLen = SeqLen;
    /* Write command to SRAM */
    AD5940_SEQCmdWrite(AppBIACfg.MeasureSeqInfo.SeqRamAddr, pSeqCmd, SeqLen);
  } else {
    return error; /* Error */
  }
  return AD5940ERR_OK;
}

/*!
  ****************************************************************************
  *@brief      BIA rtia calibration
  *@param      None
  *@return     AD5940Err/AD5940ERR_OK
******************************************************************************/
static AD5940Err AppBIARtiaCal(void) {
  HSRTIACal_Type hsrtia_cal;
  hsrtia_cal.AdcClkFreq = AppBIACfg.AdcClkFreq;
  hsrtia_cal.ADCSinc2Osr = AppBIACfg.ADCSinc2Osr;
  hsrtia_cal.ADCSinc3Osr = AppBIACfg.ADCSinc3Osr;
  /* Enable magnitude and phase */
  hsrtia_cal.bPolarResult = bTRUE;
  hsrtia_cal.DftCfg.DftNum = AppBIACfg.DftNum;
  hsrtia_cal.DftCfg.DftSrc = AppBIACfg.DftSrc;
  hsrtia_cal.DftCfg.HanWinEn = AppBIACfg.HanWinEn;
  hsrtia_cal.fRcal= AppBIACfg.RcalVal;
  hsrtia_cal.HsTiaCfg.DiodeClose = bFALSE;
  hsrtia_cal.HsTiaCfg.HstiaBias = HSTIABIAS_1P1;
  hsrtia_cal.HsTiaCfg.HstiaCtia = AppBIACfg.CtiaSel;
  hsrtia_cal.HsTiaCfg.HstiaDeRload = HSTIADERLOAD_OPEN;
  hsrtia_cal.HsTiaCfg.HstiaDeRtia = HSTIADERTIA_TODE;
  hsrtia_cal.HsTiaCfg.HstiaRtiaSel = AppBIACfg.HstiaRtiaSel;
  hsrtia_cal.SysClkFreq = AppBIACfg.SysClkFreq;
  hsrtia_cal.fFreq = AppBIACfg.SweepCfg.SweepStart;

  /* If sweep is enabled, consider update of Rtia cal table based on sweep points */
  if(AppBIACfg.SweepCfg.SweepEn == bTRUE) {
    uint32_t i;
    AppBIACfg.SweepCfg.SweepIndex = 0;  /* Reset index */
    for(i = 0; i < AppBIACfg.SweepCfg.SweepPoints;i++) {
      AD5940_SweepNext(&AppBIACfg.SweepCfg, &hsrtia_cal.fFreq);
    }
    AppBIACfg.SweepCfg.SweepIndex = 0;  /* Reset index */
    AppBIACfg.RtiaCurrValue[0] = AppBIACfg.RtiaCalTable[AppBIACfg.SweepCfg.SweepIndex][0];
    AppBIACfg.RtiaCurrValue[1] = AppBIACfg.RtiaCalTable[AppBIACfg.SweepCfg.SweepIndex][1];
  } else {
    hsrtia_cal.fFreq = AppBIACfg.SinFreq;
    AD5940_HSRtiaCal(&hsrtia_cal, AppBIACfg.RtiaCurrValue);
  }
  return AD5940ERR_OK;
}

/*!
  ****************************************************************************
  *@brief      BIA Initialization
  *@param      None
  *@return     AD5940Err/AD5940ERR_OK
******************************************************************************/
AD5940Err AppBIAInit(uint32_t *pBuffer, uint32_t BufferSize) {
  AD5940Err error = AD5940ERR_OK;
  SEQCfg_Type seq_cfg;
  FIFOCfg_Type fifo_cfg;

  if(AD5940_WakeUp(10) > 10)  /* Wakeup AFE by read register, read 10 times at most */
    return AD5940ERR_WAKEUP;  /* Wakeup Failed */

  /* Add mux enable for BCM measurement */
  /* output enable */
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
  tempreg =  AD5940_ReadReg(REG_AGPIO_GP0SET);
  tempreg &= 0xFFE7;
  tempreg |= 0x0008;
  AD5940_AGPIOSet(tempreg);
  /* Configure sequencer and stop it */
  /* 2kB SRAM is used for sequencer, others for data FIFO */
  seq_cfg.SeqMemSize = SEQMEMSIZE_2KB;
  seq_cfg.SeqBreakEn = bFALSE;
  seq_cfg.SeqIgnoreEn = bFALSE;
  seq_cfg.SeqCntCRCClr = bTRUE;
  seq_cfg.SeqEnable = bFALSE;
  seq_cfg.SeqWrTimer = 0;
  AD5940_SEQCfg(&seq_cfg);

  /* Do RTIA calibration */
  if((AppBIACfg.ReDoRtiaCal == bTRUE) || \
      AppBIACfg.BIAInited == bFALSE)  /* Do calibration on the first initialization */
  {
    AppBIARtiaCal();
    AppBIACfg.ReDoRtiaCal = bFALSE;
  }
  /* Reconfigure FIFO */
  /* Disable FIFO firstly */
  AD5940_FIFOCtrlS(FIFOSRC_DFT, bFALSE);
  fifo_cfg.FIFOEn = bTRUE;
  /* fifo mode set */
  fifo_cfg.FIFOMode = FIFOMODE_FIFO;
  /* 4kB for FIFO, The reset 2kB for sequencer */
  fifo_cfg.FIFOSize = FIFOSIZE_4KB;
  fifo_cfg.FIFOSrc = FIFOSRC_DFT;
  /* DFT result. One pair for RCAL, another for Rz.
   * One DFT result have real part and imaginary part */
  fifo_cfg.FIFOThresh = AppBIACfg.FifoThresh;
  AD5940_FIFOCfg(&fifo_cfg);
  AD5940_INTCClrFlag(AFEINTSRC_ALLINT);
  /* Start sequence generator */
  /* Initialize sequencer generator */
  if((AppBIACfg.BIAInited == bFALSE)||\
       (AppBIACfg.bParaChanged == bTRUE)){
    if(pBuffer == 0)  return AD5940ERR_PARA;
    if(BufferSize == 0) return AD5940ERR_PARA;
    AD5940_SEQGenInit(pBuffer, BufferSize);
    /* Generate initialize sequence */
    /* Application initialization sequence using either MCU or sequencer */
    error = AppBIASeqCfgGen();
    if(error != AD5940ERR_OK) return error;
    /* Generate measurement sequence */
    error = AppBIASeqMeasureGen();
    if(error != AD5940ERR_OK) return error;
    /* Clear this flag as we already implemented the new configuration */
    AppBIACfg.bParaChanged = bFALSE;
  }
  /* Initialization sequencer  */
  AppBIACfg.InitSeqInfo.WriteSRAM = bFALSE;
  AD5940_SEQInfoCfg(&AppBIACfg.InitSeqInfo);
  seq_cfg.SeqEnable = bTRUE;
  /* Enable sequencer */
  AD5940_SEQCfg(&seq_cfg);
  AD5940_SEQMmrTrig(AppBIACfg.InitSeqInfo.SeqId);
  while(AD5940_INTCTestFlag(AFEINTC_1, AFEINTSRC_ENDSEQ) == bFALSE);
  /* Measurment sequence  */
  AppBIACfg.MeasureSeqInfo.WriteSRAM = bFALSE;
  AD5940_SEQInfoCfg(&AppBIACfg.MeasureSeqInfo);
  seq_cfg.SeqEnable = bTRUE;
  /* Enable sequencer, and wait for trigger */
  AD5940_SEQCfg(&seq_cfg);
  /* Clear interrupt flag generated before */
  AD5940_ClrMCUIntFlag();
  AD5940_AFEPwrBW(AppBIACfg.PwrMod, AFEBW_250KHZ);
  AD5940_WriteReg(REG_AFE_SWMUX, 1<<3);
  /* BIA application has been initialized. */
  AppBIACfg.BIAInited = bTRUE;
  return AD5940ERR_OK;
}

/*!
  ****************************************************************************
  *@brief      BIA register modification
  *@param      None
  *@return     AD5940Err/AD5940ERR_OK
******************************************************************************/
AD5940Err AppBIARegModify(int32_t * const pData, uint32_t *pDataCount) {
  if(AppBIACfg.NumOfData > 0) {
    AppBIACfg.FifoDataCount += *pDataCount/4;
    if(AppBIACfg.FifoDataCount >= AppBIACfg.NumOfData) {
      AD5940_WUPTCtrl(bFALSE);
      return AD5940ERR_OK;
    }
  }
  if(AppBIACfg.StopRequired == bTRUE) {
    AD5940_WUPTCtrl(bFALSE);
    return AD5940ERR_OK;
  }
  /* Need to set new frequency and set power mode */
  if(AppBIACfg.SweepCfg.SweepEn) {
    AD5940_WGFreqCtrlS(AppBIACfg.SweepNextFreq, AppBIACfg.SysClkFreq);
  }
  return AD5940ERR_OK;
}


iImpCar_Type *pDftVolt, *pDftCurr;

/*!
  ****************************************************************************
  *@brief      Depending on the data type, do appropriate data pre-process
               before return back to controller
  *@param      pointer to data buffer, pointer to data count
  *@return     AD5940Err/AD5940ERR_OK
*******************************************************************************/
AD5940Err AppBIADataProcess(int32_t * const pData, uint32_t *pDataCount)  {
  uint32_t DataCount = *pDataCount;
  uint32_t ImpResCount = DataCount/4;
  fImpPol_Type * const pOut = (fImpPol_Type*)pData;
  iImpCar_Type * pSrcData = (iImpCar_Type*)pData;
  *pDataCount = 0;
  /* We expect RCAL data together with Rz data.
   * One DFT result has two data in FIFO, real part and imaginary part.  */
  DataCount = (DataCount/4)*4;

  /* Convert DFT result to int32_t type */
  for(uint32_t i = 0; i<DataCount; i++) {
    /* rounding off data buffer */
    pData[i] &= 0x3ffff;
    if(pData[i]&(1<<17)) {/* Bit17 is sign bit */
      /* Data is 18bit in two's complement, bit17 is the sign bit */
      pData[i] |= 0xfffc0000;
    }
  }
  /* calculate voltage magnitude/ phase, current magnitude and phase, \
      and hence impedance magnitude and phase */
  for(uint32_t i = 0; i<ImpResCount; i++){
    pDftCurr = pSrcData++;
    pDftVolt = pSrcData++;
    float VoltMag,VoltPhase;
    float CurrMag, CurrPhase;

    VoltMag = sqrt((float)pDftVolt->Real*pDftVolt->Real+(float)pDftVolt->Image*pDftVolt->Image);
    VoltPhase = atan2(-pDftVolt->Image,pDftVolt->Real);
    CurrMag = sqrt((float)pDftCurr->Real*pDftCurr->Real+(float)pDftCurr->Image*pDftCurr->Image);
    CurrPhase = atan2(-pDftCurr->Image,pDftCurr->Real);

    VoltMag = VoltMag/CurrMag*AppBIACfg.RtiaCurrValue[0];
    VoltPhase = VoltPhase - CurrPhase + AppBIACfg.RtiaCurrValue[1];

    pOut[i].Magnitude = VoltMag;
    pOut[i].Phase = VoltPhase;
  }

  *pDataCount = ImpResCount;

  /* Calculate next frequency point */
  if(AppBIACfg.SweepCfg.SweepEn == bTRUE){
    AppBIACfg.FreqofData = AppBIACfg.SweepCurrFreq;
    AppBIACfg.SweepCurrFreq = AppBIACfg.SweepNextFreq;
    AppBIACfg.RtiaCurrValue[0] = AppBIACfg.RtiaCalTable[AppBIACfg.SweepCfg.SweepIndex][0];
    AppBIACfg.RtiaCurrValue[1] = AppBIACfg.RtiaCalTable[AppBIACfg.SweepCfg.SweepIndex][1];
    AD5940_SweepNext(&AppBIACfg.SweepCfg, &AppBIACfg.SweepNextFreq);
  }
  return AD5940ERR_OK;
}

/*!
  ****************************************************************************
  *@brief      Initialize AD5940 basic blocks like clock
  *@param      void
  *@return     AD5940Err/AD5940ERR_OK
******************************************************************************* */
static int32_t AD5940PlatformCfg(void)
{
  CLKCfg_Type clk_cfg;
  FIFOCfg_Type fifo_cfg;
  AGPIOCfg_Type gpio_cfg;
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
  /* DFT result. One pair for RCAL, another for Rz.
   * One DFT result have real part and imaginary part */
  fifo_cfg.FIFOThresh = 4;
  AD5940_FIFOCfg(&fifo_cfg);
  /* Enable FIFO here */
  fifo_cfg.FIFOEn = bTRUE;
  AD5940_FIFOCfg(&fifo_cfg);

  /* Step3. Interrupt controller */
  /* Enable all interrupt in Interrupt Controller 1, so we can check INTC flags */
  AD5940_INTCCfg(AFEINTC_1, AFEINTSRC_ALLINT, bTRUE);
  /* Interrupt Controller 0 will control GP0 to generate interrupt to MCU */
  AD5940_INTCCfg(AFEINTC_0, AFEINTSRC_DATAFIFOTHRESH, bTRUE);
  AD5940_INTCClrFlag(AFEINTSRC_ALLINT);

  /* Step4: Reconfigure GPIO */
  gpio_cfg.FuncSet = GP6_SYNC|GP5_SYNC|GP4_SYNC|GP2_TRIG|GP1_SYNC|GP0_INT;
  gpio_cfg.InputEnSet = AGPIO_Pin2;
  gpio_cfg.OutputEnSet = AGPIO_Pin0|AGPIO_Pin1|AGPIO_Pin4|AGPIO_Pin5|AGPIO_Pin6;
  gpio_cfg.OutVal = 0;
  gpio_cfg.PullEnSet = 0;

  AD5940_AGPIOCfg(&gpio_cfg);
  /* Allow AFE to enter sleep mode. */
  AD5940_SleepKeyCtrlS(SLPKEY_UNLOCK);
  return 0;
}

/*!
  ****************************************************************************
  *@brief      !!Change the application parameters here if you want to change it
  *@param      void
  *@return     void
******************************************************************************* */
void AD5940BIAStructInit(void) {
  AppBIACfg_Type *pBIACfg;
  AppBIAGetCfg(&pBIACfg);
  pBIACfg->SeqStartAddr = 0;
  pBIACfg->MaxSeqLen = 512; /** @todo add checker in function */
  pBIACfg->RcalVal = 10000.0;
  if(!user_applied_bcm_dft_num) {
    pBIACfg->DftNum = DFTNUM_8192;
  } else {
    user_applied_bcm_dft_num = 0;
  }
  /* Never stop until you stop it mannually by AppBIACtrl() function */
  pBIACfg->NumOfData = -1;

 if(!user_applied_odr) {
    /* ODR(Sample Rate) 20Hz */
   /* ODR decides how freuquently to start the engine to measure impedance. */
    pBIACfg->BiaODR = 20;
  } else {
    user_applied_odr = 0;
  }
  pBIACfg->FifoThresh = 4;      /* 4 */
  pBIACfg->ADCSinc3Osr = ADCSINC3OSR_2;
}

float freq;

/*!
  ****************************************************************************
  *@brief      Calculate BIA Impedance
  *@param      Pointer to soft buffer, fifo count
  *@return     SUCCESS
******************************************************************************* */
int32_t CalculateBioImpedance(uint32_t *pData, uint32_t DataCount) {
  fImpCar_Type *pImp;
  pImp = (fImpCar_Type*)pData;
  AppBIACtrl(BIACTRL_GETFREQ, &freq);
  for(int i=0;i<DataCount;i++) {
    gBcmData.bcmImpedence.real = (int32_t)(pImp[i].Real);
    gBcmData.bcmImpedence.img = (int32_t)(pImp[i].Image);
  }
  return 0;
}


#ifdef DEBUG_DCB
extern uint16_t MemNumBlks;
extern uint16_t MemFreeBlks;

/*!
  ****************************************************************************
  *@brief      Function for getting status of fds records
  *@param      Pointer to m2m2_hdr_t
  *@return     structure m2m2_hdr_t
******************************************************************************* */
static m2m2_hdr_t *adi_dcb_fds_stat(m2m2_hdr_t *p_pkt) {
    fds_stat_t stat = {0};
    /* Allocate memory for response packet payload */
    PKT_MALLOC(p_resp_pkt, m2m2_dcb_fds_status_info_resp_t, 0);
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_dcb_fds_status_info_resp_t, p_resp_payload);
    /* Read FDS status */
    get_fds_status(&stat);
    p_resp_payload->command = M2M2_DCB_COMMAND_FDS_STATUS_RESP;
    p_resp_payload->status = M2M2_APP_COMMON_STATUS_OK;
    p_resp_payload->dirty_records = stat.dirty_records;
    p_resp_payload->open_records = stat.open_records;
    p_resp_payload->valid_records = stat.valid_records;
    p_resp_payload->pages_available = stat.pages_available;
    p_resp_payload->num_blocks = MemNumBlks;
    p_resp_payload->blocks_free = MemFreeBlks;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
    return p_resp_pkt;
}
#endif

///////////////////////////////////////////////////////////////////
#ifdef DCB
/*!
 ****************************************************************************
 * @brief    Gets the entire bcm DCB configuration written in flash
 * @param    bcm_dcb_data: pointer to dcb struct variable,
 *            in size of data in Double Word (32-bits)
 * @param    read_size: The size of the record to be returned to the user
 * @retval   BCM_ERROR_CODE_t: Status
 *****************************************************************************/
BCM_ERROR_CODE_t read_bcm_dcb(uint32_t *bcm_dcb_data, uint16_t *read_size) {
  BCM_ERROR_CODE_t dcb_status = BCM_ERROR;

  if (adi_dcb_read_from_fds(ADI_DCB_BCM_BLOCK_IDX, bcm_dcb_data, read_size) == DEF_OK) {
    dcb_status = BCM_SUCCESS;
  }
  return dcb_status;
}

/*!
 ****************************************************************************
 * @brief    Sets the entire bcm DCB configuration in flash
 * @param    bcm_dcb_data: pointer to dcb struct variable,
 *            in size of data in Double Word (32-bits)
 * @param    write_Size: The size of the record to be written
 * @retval   BCM_ERROR_CODE_t: Status
 *****************************************************************************/
BCM_ERROR_CODE_t write_bcm_dcb(uint32_t *bcm_dcb_data, uint16_t write_Size) {
  BCM_ERROR_CODE_t dcb_status = BCM_ERROR;

  if (adi_dcb_write_to_fds(ADI_DCB_BCM_BLOCK_IDX, bcm_dcb_data, write_Size) == DEF_OK) {
    dcb_status = BCM_SUCCESS;
  }
  return dcb_status;
}

/*!
 ****************************************************************************
 * @brief    Delete the entire bcm DCB configuration in flash
 * @param    None
 * @retval   BCM_ERROR_CODE_t: Status
 *****************************************************************************/
BCM_ERROR_CODE_t delete_bcm_dcb(void) {
  BCM_ERROR_CODE_t dcb_status = BCM_ERROR;

  if (adi_dcb_delete_fds_settings(ADI_DCB_BCM_BLOCK_IDX) == DEF_OK) {
    dcb_status = BCM_SUCCESS;
  }
  return dcb_status;
}

/*!
 ****************************************************************************
 * @brief    Set the dcb present flag
 * @param    set_flag: flag to set presence of dcb in flash
 * @retval   None
 *****************************************************************************/
void bcm_set_dcb_present_flag(bool set_flag) {
  g_bcm_dcb_Present = set_flag;
  NRF_LOG_INFO("BCM DCB present set: %s",
      (g_bcm_dcb_Present == true ? "TRUE" : "FALSE"));
}

/*!
 ****************************************************************************
 * @brief    Get whether the dcb is present in flash
 * @param    None
 * @retval   bool: TRUE: dcb present, FALSE: dcb absent
 *****************************************************************************/
bool bcm_get_dcb_present_flag(void) {
  NRF_LOG_INFO(
      "BCM DCB present: %s", (g_bcm_dcb_Present == true ? "TRUE" : "FALSE"));
  return g_bcm_dcb_Present;
}

/*!
 ****************************************************************************
 * @brief    Update the global dcb presence flag in flash
 * @param    None
 * @retval   None
 *****************************************************************************/
void bcm_update_dcb_present_flag(void) {
  g_bcm_dcb_Present = adi_dcb_check_fds_entry(ADI_DCB_BCM_BLOCK_IDX);
  NRF_LOG_INFO("Updated. BCM DCB present: %s",
      (g_bcm_dcb_Present == true ? "TRUE" : "FALSE"));
}

/*!
 ****************************************************************************
 * @brief    Update the lcfg from dcb
 * @param    p_pkt: pointer to packet
 * @retval   m2m2_hdr_t
 *****************************************************************************/
static m2m2_hdr_t *bcm_app_set_dcb_lcfg(m2m2_hdr_t *p_pkt) {
  M2M2_APP_COMMON_STATUS_ENUM_t status = M2M2_APP_COMMON_STATUS_ERROR;
  PKT_MALLOC(p_resp_pkt, bcm_app_dcb_lcfg_t, 0);
  if (NULL != p_resp_pkt) {
    PYLD_CST(p_resp_pkt, bcm_app_dcb_lcfg_t, p_resp_payload);
    if (bcm_get_dcb_present_flag() == true) {
      uint32_t bcm_dcb[MAXBCMDCBSIZE] = {'\0'};
      uint16_t size = MAXBCMDCBSIZE;
      if (read_bcm_dcb(bcm_dcb, &size) == BCM_SUCCESS) {
        for (uint8_t i = 0; i < size; i++) {
          /*
          each bcm_dcb array element stores field and value together in total 4
          bytes. field -> 1 Byte value -> 2 Bytes undefined -> 1 Byte for ex- if
          bcm_dcb[i] = 0x00ABCDEF field = 0xAB Value = 0xCDEF not_used/undefined
          -> 0x00
          */
          /* doing right shift and then & operation to get field */
          uint8_t field = (bcm_dcb[i] >> 16) & 0xFF;
          /* doing & operation to get value */
          uint16_t value = bcm_dcb[i] & 0xFFFF;
          if (BcmWriteLCFG(field, value) == BCM_SUCCESS) {
            status = M2M2_APP_COMMON_STATUS_OK;
          } else {
            status = M2M2_APP_COMMON_STATUS_ERROR;
          }
        }
      } else {
        status = M2M2_APP_COMMON_STATUS_ERROR;
      }
    } else {
      /* Since this function is for setting dcb lcfg for bcm,
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
static m2m2_hdr_t *bcm_dcb_command_read_config(m2m2_hdr_t *p_pkt) {
  static uint16_t r_size = 0;
  uint32_t dcbdata[MAXBCMDCBSIZE];
  M2M2_DCB_STATUS_ENUM_t status = M2M2_DCB_STATUS_ERR_NOT_CHKD;

  // Declare a pointer to access the input packet payload
  // PYLD_CST(p_pkt, m2m2_dcb_bcm_data_t, p_in_payload);
  PKT_MALLOC(p_resp_pkt, m2m2_dcb_bcm_data_t, 0);
  if (NULL != p_resp_pkt) {
    // Declare a pointer to the response packet payload
    PYLD_CST(p_resp_pkt, m2m2_dcb_bcm_data_t, p_resp_payload);

    r_size = MAXBCMDCBSIZE;
    if (read_bcm_dcb(&dcbdata[0], &r_size) == BCM_SUCCESS) {
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

static m2m2_hdr_t *bcm_dcb_command_write_config(m2m2_hdr_t *p_pkt) {
  M2M2_DCB_STATUS_ENUM_t status = M2M2_DCB_STATUS_ERR_NOT_CHKD;
  uint32_t dcbdata[MAXBCMDCBSIZE];

  /* Declare a pointer to access the input packet payload */
  PYLD_CST(p_pkt, m2m2_dcb_bcm_data_t, p_in_payload);
  PKT_MALLOC(p_resp_pkt, m2m2_dcb_bcm_data_t, 0);
  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_dcb_bcm_data_t, p_resp_payload);

    for (int i = 0; i < p_in_payload->size; i++)
      dcbdata[i] = p_in_payload->dcbdata[i];
    if (write_bcm_dcb(&dcbdata[0], p_in_payload->size) == BCM_SUCCESS) {
      bcm_set_dcb_present_flag(true);
      status = M2M2_DCB_STATUS_OK;
    } else {
      status = M2M2_DCB_STATUS_ERR_ARGS;
    }

    p_resp_payload->status = status;
    p_resp_payload->command = M2M2_DCB_COMMAND_WRITE_CONFIG_RESP;
    p_resp_payload->size = 0;
    for(uint16_t i=0; i< MAXBCMDCBSIZE; i++)
      p_resp_payload->dcbdata[i] = 0;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  }
  return p_resp_pkt;
}

static m2m2_hdr_t *bcm_dcb_command_delete_config(m2m2_hdr_t *p_pkt) {
  M2M2_DCB_STATUS_ENUM_t status = M2M2_DCB_STATUS_ERR_NOT_CHKD;
  PKT_MALLOC(p_resp_pkt, m2m2_dcb_bcm_data_t, 0);
  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_dcb_bcm_data_t, p_resp_payload);

    if (delete_bcm_dcb() == BCM_SUCCESS) {
      bcm_set_dcb_present_flag(false);
      status = M2M2_DCB_STATUS_OK;
    } else {
      status = M2M2_DCB_STATUS_ERR_ARGS;
    }

    p_resp_payload->status = status;
    p_resp_payload->command = M2M2_DCB_COMMAND_ERASE_CONFIG_RESP;
    p_resp_payload->size = 0;
    for(uint16_t i=0; i< MAXBCMDCBSIZE; i++)
      p_resp_payload->dcbdata[i] = 0;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  }
  return p_resp_pkt;
}

#endif
#endif