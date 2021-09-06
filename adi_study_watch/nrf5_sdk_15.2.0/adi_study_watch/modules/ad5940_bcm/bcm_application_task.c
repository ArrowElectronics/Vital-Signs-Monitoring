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

extern uint32_t AppBuff[APPBUFF_SIZE];

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
void Enable_ephyz_power(void);
static void InitCfg();
uint32_t ad5940_port_Init(void);
uint32_t ad5940_port_deInit(void);
void ad5940_bcm_start(void);
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

volatile int16_t bcm_user_applied_odr = 0;
volatile int16_t bcm_user_applied_bcm_dft_num = 0;
static volatile int16_t user_applied_pga_gain = 0;
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
extern AD5950_APP_ENUM_t gnAd5940App;

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
  ADI_OSAL_STATUS         err;

  /*Wait for FDS init to complete*/
  adi_osal_SemPend(bcm_task_evt_sem, ADI_OSAL_TIMEOUT_FOREVER);

  post_office_add_mailbox(M2M2_ADDR_MED_BCM, M2M2_ADDR_MED_BCM_STREAM);
  while (1) {
    m2m2_hdr_t *p_in_pkt = NULL;
    m2m2_hdr_t *p_out_pkt = NULL;
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
        p_payload_ptr->sequence_num = g_state_bcm.data_pkt_seq_num++;
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
    	bcm_user_applied_bcm_dft_num = 1;
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
      gnAd5940App = AD5940_APP_BCM;
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
    if(g_state_bcm.num_subs == 1)
    {
       /* reset pkt sequence no. only during 1st sub request */
       g_state_bcm.data_pkt_seq_num = 0;
    }
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


  /* switch off other switches */
#ifdef ENABLE_ECG_APP
  DG2502_SW_control_AD8233(false);
#endif
  DG2502_SW_control_ADPD4000(false);

   /* Initialization of application parameters */
  InitCfg();

   /* start bia initialization */
   ad5940_bcm_start();

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
      bcm_user_applied_odr = 1;
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
   case BCM_DFT_NUM: /* field 3 */
      pCfg->DftNum= value;
      bcm_user_applied_bcm_dft_num = 1;
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
    case BCM_DFT_NUM: /* field 3 */
      *value = pCfg->DftNum;
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
  *@brief      Calculate BIA Impedance
  *@param      Pointer to soft buffer, fifo count
  *@return     SUCCESS
******************************************************************************* */
int32_t CalculateBioImpedance(uint32_t *pData, uint32_t DataCount) {
  float freq;
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