/**
***************************************************************************
* @addtogroup Tasks
* @{
* @file         bia_application_task.c
* @author       ADI
* @version      V1.0.0
* @date         10-October-2019
* @brief        Source file contains BIA processing wrapper.
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
#ifdef ENABLE_BIA_APP
#include "bia_application_task.h"
#include "sensor_ad5940.h"
#include "app_bia.h"
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
static volatile bool g_bia_dcb_Present = false;
bool bia_get_dcb_present_flag(void);
void bcm_set_dcb_present_flag(bool set_flag);
BIA_ERROR_CODE_t read_bia_dcb(uint32_t *bia_dcb_data, uint16_t *read_size);
BIA_ERROR_CODE_t write_bia_dcb(uint32_t *bia_dcb_data, uint16_t write_Size);
BIA_ERROR_CODE_t delete_bia_dcb(void);
#endif
/////////////////////////////////////////

/* BCM app Task Module Log settings */
#define NRF_LOG_MODULE_NAME BIA_App_Task

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

const char GIT_BIA_VERSION[] = "TEST BIA_VERSION STRING";
const uint8_t GIT_BIA_VERSION_LEN = sizeof(GIT_BIA_VERSION);
static void packetize_bia_raw_data(bia_packetizer_t *p_pktizer);
static void packetize_bcm_algo_data();
static m2m2_hdr_t *bia_app_reg_access(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *bia_app_stream_config(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *bia_app_status(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *bia_app_get_version(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *bia_app_decimation(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *bia_app_dft_num_set(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *bia_app_set_cal(m2m2_hdr_t *p_pkt);
static M2M2_SENSOR_BIA_SWEEP_FREQ_INDEX_ENUM_t GetBIASweepFrequencyIndex(float frequency);
#ifdef DEBUG_DCB
static m2m2_hdr_t *adi_dcb_fds_stat(m2m2_hdr_t *p_pkt);
#endif
static void fetch_bia_data(void);
static void sensor_bia_task(void *pArgument);
void Enable_ephyz_power(void);
static void InitCfg();
uint32_t ad5940_port_Init(void);
uint32_t ad5940_port_deInit(void);
void ad5940_bia_start(void);
#ifdef DCB
static m2m2_hdr_t *bia_app_set_dcb_lcfg(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *bia_dcb_command_read_config(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *bia_dcb_command_write_config(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *bia_dcb_command_delete_config(m2m2_hdr_t *p_pkt);
#endif
#ifdef PROFILE_TIME_ENABLED
static m2m2_hdr_t *bcm_app_dcb_timing_info(m2m2_hdr_t *p_pkt) ;
#endif
g_state_bia_t g_state_bia;
volatile bool gBiaAppInitFlag = false;
typedef  m2m2_hdr_t*(app_cb_function_t)(m2m2_hdr_t*);
static _gBiaData_t gBiaData = {{0},0,0,0};
static int32_t BiaSignalData[BIA_BUFFER_SIZE];

volatile uint8_t bia_user_applied_odr = 0;
volatile uint8_t bia_user_applied_dft_num = 0;
static volatile uint8_t user_applied_pga_gain = 0;
static volatile uint8_t user_applied_bia_hsrtia_sel = 0;
static volatile uint8_t user_applied_bia_pwr_mod = 0;
static volatile uint8_t user_applied_sin_freq = 0;
static volatile uint8_t user_applied_algo_decimation_factor = 0;

/*stability bia data*/
#define BIA_DATA_BUFFER_SIZE 20
#define BIA_STABILITY_SAMPLES_COUNT 2
#define BIA_STABILITY_OHMS_DIFF  6
float gbia_data_buffer[BIA_DATA_BUFFER_SIZE];
uint8_t gbia_data_count = 0,gbia_stability_samplecount = 0;
uint8_t gnBiaDataStable = 0;
uint8_t check_bia_data_stability(float body_impedance_real,float body_impedance_imaginary);
void Init_bia_data_stability();

uint8_t sensor_bia_task_stack[APP_OS_CFG_BIA_TASK_STK_SIZE];
ADI_OSAL_THREAD_HANDLE sensor_bia_task_handler;
ADI_OSAL_STATIC_THREAD_ATTR sensor_bia_task_attributes;
StaticTask_t bcmTaskTcb;
ADI_OSAL_SEM_HANDLE   bia_task_evt_sem;
extern uint32_t BiaFifoCount;
extern uint16_t BiaRealImgDataSetCount;
static int32_t bia_counter=0;
extern AD5940_APP_ENUM_t gnAd5940App;

/* first 10 elements are decimal entires in lcfg which does not require IEEE conversion */
#define MAX_DECIMAL_ENTRIES 10

#ifdef DEBUG_BCM
static uint32_t pkt_count = 0;
static float bcm_odr = 0, time_elapsed_for_bcm = 0, bcm_start_time = 0;
#endif

#ifdef BCM_ALGO
#include <adi_vsm_bcm.h>
#include <adi_bcm_algo.h>

adi_vsm_bcm_instance_t* bcm_instance = NULL;

adi_vsm_bcm_config_t bcm_config_handle={
  /***ADI_algo parameters**/
  .sampling_freq = 20,     /*sampling data rate in Hz*/
  .output_data_rate = 20,     /*Output data rate in Hz*/
  .adi_vsm_bcm_algo_config.height = 160,      /*Height in Cm*/
  .adi_vsm_bcm_algo_config.weight = 60,      /*Weight in Kg*/
  .adi_vsm_bcm_algo_config.age = 30,      /*Age in years*/
  .adi_vsm_bcm_algo_config.b[0] = 0.377285,      /*Zeroth Coeff*/
  .adi_vsm_bcm_algo_config.b[1] = -0.1124,      /*First Coeff*/
  .adi_vsm_bcm_algo_config.b[2] = 0.51977,      /*Second Coeff*/
  .adi_vsm_bcm_algo_config.b[3] = -0.04686,      /*Third Coeff*/
  .adi_vsm_bcm_algo_config.b[4] = 0.000319,      /*Fourth Coeff*/
  .adi_vsm_bcm_algo_config.b[5] = -0.02164,      /*Fifth Coeff*/
  .adi_vsm_bcm_algo_config.b[6] = -0.0001,      /*Sixth Coeff*/
  .adi_vsm_bcm_algo_config.b[7] = -42.9923,      /*Seventh Coeff*/
  };

adi_vsm_bcm_output_t adi_vsm_bcm_output;
uint16_t lcfg_bcm_algo_en=0;
static uint16_t bcm_algo_data_pkt_seq_num;
static uint8_t algo_start_count;
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

  if(!user_applied_bia_pwr_mod) {
    AppBIACfg.PwrMod = AFEPWR_LP;
  } else {
    user_applied_bia_pwr_mod = 0;
  }
  if(!user_applied_bia_hsrtia_sel) {
    AppBIACfg.HstiaRtiaSel = HSTIARTIA_1K;
  } else {
    user_applied_bia_hsrtia_sel = 0;
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

app_routing_table_entry_t bia_app_routing_table[] = {
  {M2M2_APP_COMMON_CMD_STREAM_START_REQ, bia_app_stream_config},
  {M2M2_APP_COMMON_CMD_STREAM_STOP_REQ, bia_app_stream_config},
  {M2M2_APP_COMMON_CMD_STREAM_SUBSCRIBE_REQ, bia_app_stream_config},
  {M2M2_APP_COMMON_CMD_STREAM_UNSUBSCRIBE_REQ, bia_app_stream_config},
  {M2M2_APP_COMMON_CMD_SENSOR_STATUS_QUERY_REQ, bia_app_status},
  {M2M2_APP_COMMON_CMD_READ_LCFG_REQ, bia_app_reg_access},
  {M2M2_APP_COMMON_CMD_WRITE_LCFG_REQ, bia_app_reg_access},
  {M2M2_SENSOR_COMMON_CMD_GET_STREAM_DEC_FACTOR_REQ, bia_app_decimation},
  {M2M2_SENSOR_COMMON_CMD_SET_STREAM_DEC_FACTOR_REQ, bia_app_decimation},
  {M2M2_APP_COMMON_CMD_GET_VERSION_REQ, bia_app_get_version},
  {M2M2_BIA_APP_CMD_SET_DFT_NUM_REQ, bia_app_dft_num_set},
  {M2M2_BIA_APP_CMD_SET_HS_RTIA_CAL_REQ, bia_app_set_cal},
#ifdef DEBUG_DCB
  {M2M2_DCB_COMMAND_FDS_STATUS_REQ, adi_dcb_fds_stat},
#endif
#ifdef PROFILE_TIME_ENABLED
  {M2M2_APP_COMMON_CMD_DCB_TIMING_INFO_REQ, bcm_app_dcb_timing_info},
#endif
#ifdef DCB
    {M2M2_APP_COMMON_CMD_SET_LCFG_REQ, bia_app_set_dcb_lcfg},
    {M2M2_DCB_COMMAND_READ_CONFIG_REQ, bia_dcb_command_read_config},
    {M2M2_DCB_COMMAND_WRITE_CONFIG_REQ, bia_dcb_command_write_config},
    {M2M2_DCB_COMMAND_ERASE_CONFIG_REQ, bia_dcb_command_delete_config},
#endif
};

#define BIA_APP_ROUTING_TBL_SZ (sizeof(bia_app_routing_table) / sizeof(bia_app_routing_table[0]))

ADI_OSAL_QUEUE_HANDLE  bia_task_msg_queue = NULL;

/*!
  ****************************************************************************
  *@brief       BCM Task initialization
  *@param       None
  *@return      None
******************************************************************************/
void ad5940_bia_task_init(void) {
  /* Initialize app state */
  g_state_bia.num_subs = 0;
  g_state_bia.num_starts = 0;
  /* Default behaviour is to send every packet */
  g_state_bia.decimation_factor = 1;
  g_state_bia.decimation_nsamples = 0;
  g_state_bia.data_pkt_seq_num = 0;
#ifdef BCM_ALGO
  bcm_algo_data_pkt_seq_num = 0;
  algo_start_count = 0;
#endif
  ADI_OSAL_STATUS eOsStatus = ADI_OSAL_SUCCESS;
  sensor_bia_task_attributes.pThreadFunc = sensor_bia_task;
  sensor_bia_task_attributes.nPriority = APP_OS_CFG_BIA_TASK_PRIO;
  sensor_bia_task_attributes.pStackBase = &sensor_bia_task_stack[0];
  sensor_bia_task_attributes.nStackSize = APP_OS_CFG_BIA_TASK_STK_SIZE;
  sensor_bia_task_attributes.pTaskAttrParam = NULL;
  sensor_bia_task_attributes.szThreadName = "BIA Sensor";
  sensor_bia_task_attributes.pThreadTcb = &bcmTaskTcb;

  eOsStatus = adi_osal_MsgQueueCreate(&bia_task_msg_queue,NULL,5);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
    Debug_Handler();
  } else {
    update_task_queue_list(APP_OS_CFG_BCM_TASK_INDEX,bia_task_msg_queue);
  }
  eOsStatus = adi_osal_ThreadCreateStatic(&sensor_bia_task_handler,
                                          &sensor_bia_task_attributes);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
    Debug_Handler();
  }

  adi_osal_SemCreate(&bia_task_evt_sem, 0);
}

/*!
  ****************************************************************************
  *@brief      Called by the post office to send messages to this application
  *@param      p_pkt: packet of type m2m2_hdr_t
  *@return     None
******************************************************************************/
void send_message_ad5940_bia_task(m2m2_hdr_t *p_pkt) {
  ADI_OSAL_STATUS osal_result;
  osal_result = adi_osal_MsgQueuePost(bia_task_msg_queue,p_pkt);
  if (osal_result != ADI_OSAL_SUCCESS)
    post_office_consume_msg(p_pkt);
  adi_osal_SemPost(bia_task_evt_sem);
}

/*!
  ****************************************************************************
  *@brief      BCM Task
  *@param      None
  *@return     None
******************************************************************************/
static void sensor_bia_task(void *pArgument) {
  ADI_OSAL_STATUS         err;

  /*Wait for FDS init to complete*/
  adi_osal_SemPend(bia_task_evt_sem, ADI_OSAL_TIMEOUT_FOREVER);

  post_office_add_mailbox(M2M2_ADDR_MED_BIA, M2M2_ADDR_MED_BIA_STREAM);
  post_office_add_mailbox(M2M2_ADDR_MED_BIA, M2M2_ADDR_BCM_ALGO_STREAM);
  while (1) {
    m2m2_hdr_t *p_in_pkt = NULL;
    m2m2_hdr_t *p_out_pkt = NULL;
    adi_osal_SemPend(bia_task_evt_sem, ADI_OSAL_TIMEOUT_FOREVER);
    p_in_pkt = post_office_get(ADI_OSAL_TIMEOUT_NONE, APP_OS_CFG_BCM_TASK_INDEX);
    if (p_in_pkt == NULL) {
      /* No m2m2 messages to process, so fetch some data from the device. */
      if(g_state_bia.num_starts != 0) /* Fetch data from device only if the device is in active state(initialized properly)*/
        fetch_bia_data();
    } else {
      /* Got an m2m2 message from the queue, process it. */
     PYLD_CST(p_in_pkt, _m2m2_app_common_cmd_t, p_in_cmd);
     /* Look up the appropriate function to call in the function table */
      for (int i = 0; i < BIA_APP_ROUTING_TBL_SZ; i++) {
        if (bia_app_routing_table[i].command == p_in_cmd->command) {
          p_out_pkt = bia_app_routing_table[i].cb_handler(p_in_pkt);
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
static void reset_bia_packetization(void) {
   g_state_bia.bia_pktizer.packet_nsamples = 0;
}


/*!
  ****************************************************************************
  *@brief      Fetch BCM data
  *@param      None
  *@return     None
******************************************************************************/
static void fetch_bia_data(void){
  ADI_OSAL_STATUS         err;
  uint32_t bcmData[2] = {0};
  uint32_t bcmTS = 0;
  int8_t status = 0;
  M2M2_SENSOR_BIA_APP_INFO_BITSET_ENUM_t leadsOn = M2M2_SENSOR_BIA_APP_INFO_BITSET_LEADSOFF;
  fImpCar_Type *pImp;
  ad5940_read_bia_data_to_buffer();
  status = ad5940_buff_get(&bcmData[0], &bcmTS);
  bia_counter = 0;
  while(status == AD5940Drv_SUCCESS) {
   /* reading a pair of samples for impedance calculation */
    BiaSignalData[bia_counter++] = bcmData[0];
    BiaSignalData[bia_counter++] = bcmData[1];
    pImp = (fImpCar_Type*)&BiaSignalData; // convert two 32 bits value pairs to real and imaginary
    gBiaData.biaImpedence.real = pImp->Real;
    gBiaData.biaImpedence.img = pImp->Image;
    gBiaData.biaImpedence.timestamp = bcmTS;
    g_state_bia.decimation_nsamples++;
    if (g_state_bia.decimation_nsamples >= g_state_bia.decimation_factor) {
      g_state_bia.decimation_nsamples = 0;
      packetize_bia_raw_data(&g_state_bia.bia_pktizer);
      leadsOn = check_bia_data_stability(gBiaData.biaImpedence.real, gBiaData.biaImpedence.img);
      if (g_state_bia.bia_pktizer.packet_nsamples >= g_state_bia.bia_pktizer.packet_max_nsamples) {
        adi_osal_EnterCriticalRegion();
        PYLD_CST(g_state_bia.bia_pktizer.p_pkt,bia_app_stream_t, p_payload_ptr);
        p_payload_ptr->command = M2M2_SENSOR_COMMON_CMD_STREAM_DATA;
        p_payload_ptr->status = 0x00;
        p_payload_ptr->datatype = M2M2_SENSOR_BIA_DATA;
        p_payload_ptr->bia_info = leadsOn;
        g_state_bia.bia_pktizer.p_pkt->src = M2M2_ADDR_MED_BIA;
        g_state_bia.bia_pktizer.p_pkt->dest = M2M2_ADDR_MED_BIA_STREAM;
        p_payload_ptr->sequence_num = g_state_bia.data_pkt_seq_num++;
#ifdef DEBUG_PKT
        post_office_msg_cnt(g_state_bia.bia_pktizer.p_pkt);
#endif
#ifdef DEBUG_BCM
        pkt_count++;
        time_elapsed_for_bcm =  MCU_HAL_GetTick() - bcm_start_time;
        bcm_odr = ((pkt_count*4*1000)/time_elapsed_for_bcm);
        NRF_LOG_INFO("TIME ELAPSED = "NRF_LOG_FLOAT_MARKER, NRF_LOG_FLOAT(time_elapsed_for_bcm));
        NRF_LOG_INFO("START TIME = "NRF_LOG_FLOAT_MARKER, NRF_LOG_FLOAT(bcm_start_time));
        NRF_LOG_INFO("BCM MEASURED ODR = "NRF_LOG_FLOAT_MARKER, NRF_LOG_FLOAT(bcm_odr));
#endif
        post_office_send(g_state_bia.bia_pktizer.p_pkt, &err);
        adi_osal_ExitCriticalRegion();
        g_state_bia.bia_pktizer.packet_nsamples = 0;
        g_state_bia.bia_pktizer.packet_max_nsamples = 0;
        g_state_bia.bia_pktizer.p_pkt = NULL;
      }
    }
#ifdef BCM_ALGO
   /* packetize algo data */
    if(leadsOn == 1)  {
      // if (g_state_bia.decimation_nsamples >= g_state_bia.algo_decimation_factor) {
      //  g_state_bia.decimation_nsamples = 0;
        packetize_bcm_algo_data();
     // }
    }
#endif
   /* Copy data from circular buffer data to soft buffer*/
    status = ad5940_buff_get(&bcmData[0], &bcmTS);
    bia_counter = 0;
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
static void packetize_bia_raw_data(bia_packetizer_t *p_pktizer) {
     M2M2_SENSOR_BIA_SWEEP_FREQ_INDEX_ENUM_t nfrequency_index = GetBIASweepFrequencyIndex(AppBIACfg.SinFreq);
    /*Calculate Impedance in form of Cartesian form */
    if (p_pktizer->packet_nsamples == 0) {
      p_pktizer->p_pkt = post_office_create_msg(sizeof(bia_app_stream_t) + M2M2_HEADER_SZ);
      PYLD_CST(p_pktizer->p_pkt, bia_app_stream_t, p_payload_ptr);
      g_state_bia.bia_pktizer.packet_max_nsamples=(sizeof(p_payload_ptr->bia_data)/sizeof(p_payload_ptr->bia_data[0]));
      p_payload_ptr->bia_data[0].timestamp = gBiaData.biaImpedence.timestamp;
      /* multiply data by 1000 to save the float precision*/
      p_payload_ptr->bia_data[0].real = (int32_t)(gBiaData.biaImpedence.real * 1000);
      p_payload_ptr->bia_data[0].img = (int32_t)(gBiaData.biaImpedence.img * 1000);
      p_payload_ptr->bia_data[0].freq_index = nfrequency_index;
      p_pktizer->packet_nsamples++;
    } else if (p_pktizer->packet_nsamples < p_pktizer->packet_max_nsamples) {
      PYLD_CST(p_pktizer->p_pkt, bia_app_stream_t, p_payload_ptr);
      /*  one packet =  four samples
      first sample was added with above check
      and remaining 3 samples are added here */
      uint16_t i = p_pktizer->packet_nsamples;
      p_payload_ptr->bia_data[i].timestamp =  gBiaData.biaImpedence.timestamp;
       /* multiply data by 1000 to save the float precision*/
      p_payload_ptr->bia_data[i].real = (int32_t)(gBiaData.biaImpedence.real * 1000);
      p_payload_ptr->bia_data[i].img = (int32_t)(gBiaData.biaImpedence.img * 1000);
      p_payload_ptr->bia_data[i].freq_index = nfrequency_index;
      p_pktizer->packet_nsamples++;
    }
}

#ifdef BCM_ALGO
/*!
  ****************************************************************************
  *@brief      Packetize bcm algo data
  *@param      pBiaData: Pointer to BIA
  *@return     None
******************************************************************************/
static void packetize_bcm_algo_data() {
  ADI_OSAL_STATUS         err;
  BCM_ALG_RETURN_CODE_t ret = BCM_ALG_ERROR;
    /* Estimate Fat Free Mass ( FFM ) */
    /* if bia is running */
  if(algo_start_count == 1) {
    /* if algorithm is enabled */
    if(lcfg_bcm_algo_en) {
      float bodyImpedance[2]={0};
      bodyImpedance[0] = gBiaData.biaImpedence.real;
      bodyImpedance[1] = gBiaData.biaImpedence.img;
      ret = BcmAlgProcess(bcm_instance,bodyImpedance,
                      &adi_vsm_bcm_output);
      /* bcm algo process */
      if(ret != ADI_VSM_BCM_IN_PROGRESS) {
        adi_osal_EnterCriticalRegion();
        /* packetize stream */
        m2m2_hdr_t *p_pkt = post_office_create_msg(sizeof(bcm_app_algo_out_stream_t) + M2M2_HEADER_SZ);
        if(p_pkt != NULL){
          PYLD_CST(p_pkt, bcm_app_algo_out_stream_t, p_payload_ptr);
          p_payload_ptr->sequence_num = bcm_algo_data_pkt_seq_num++;
          p_payload_ptr->status = M2M2_APP_COMMON_STATUS_OK;
          if(ret == BCM_ALG_SUCCESS) {
            p_payload_ptr->ffm_estimated = adi_vsm_bcm_output.ffm_estimated;
            p_payload_ptr->bmi = adi_vsm_bcm_output.bmi;
            p_payload_ptr->fat_percent = adi_vsm_bcm_output.fatpercent;
          }else if(ret == BCM_ALG_NULL_PTR_ERROR || ret == BCM_ALG_ERROR) {
            p_payload_ptr->ffm_estimated = 0xFF;
            p_payload_ptr->bmi = 0xFF;
            p_payload_ptr->fat_percent = 0xFF;
          }
          p_payload_ptr->time_stamp = get_sensor_time_stamp();
          p_payload_ptr->command = M2M2_SENSOR_COMMON_CMD_STREAM_DATA;
          p_pkt->src = M2M2_ADDR_MED_BIA;
          p_pkt->dest = M2M2_ADDR_BCM_ALGO_STREAM;
          post_office_send(p_pkt, &err);
        }
        adi_osal_ExitCriticalRegion();
      }else {
        // skip packetization during decimation 
        adi_vsm_bcm_output.ffm_estimated = 0;
        adi_vsm_bcm_output.bmi = 0;
        adi_vsm_bcm_output.fatpercent = 0;
      }
    }
  }
}

#endif
/*!
  ****************************************************************************
  *@brief      Set BCM DFT number
  *@param      pPkt: pointer to the packet structure
  *@return     m2m2_hdr_t
******************************************************************************/
static m2m2_hdr_t *bia_app_dft_num_set(m2m2_hdr_t *p_pkt) {
   /* Declare a pointer to access the input packet payload */
  PYLD_CST(p_pkt, bia_app_set_dft_num_t, p_in_payload);

  /* Allocate memory to the response packet payload */
  PKT_MALLOC(p_resp_pkt, bia_app_set_dft_num_t, 0);
  if (NULL != p_resp_pkt) {
  	/* Declare a pointer to the response packet payload */
  	PYLD_CST(p_resp_pkt, bia_app_set_dft_num_t, p_resp_payload);

  	/* Copy dft number from payload if valid setting  */
  	if(p_in_payload->dftnum <= 12) {
   	NRF_LOG_INFO("Valid dft num sel");
    	AppBIACfg.DftNum = p_in_payload->dftnum;
    	bia_user_applied_dft_num = 1;
  } else {
    /* default DFT number value */
    AppBIACfg.DftNum = DFTNUM_8192;
  }

  	p_resp_payload->command = M2M2_BIA_APP_CMD_SET_DFT_NUM_RESP;
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
static m2m2_hdr_t *bia_app_get_version(m2m2_hdr_t *p_pkt) {
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
  memcpy(&p_resp_payload->str[0], &GIT_BIA_VERSION, GIT_BIA_VERSION_LEN);
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
static m2m2_hdr_t *bia_app_status(m2m2_hdr_t *p_pkt) {
  /* Declare and malloc a response packet */
  PKT_MALLOC(p_resp_pkt, m2m2_app_common_status_t, 0);
  if (NULL != p_resp_pkt) {
  /* Declare a pointer to the response packet payload */
  PYLD_CST(p_resp_pkt, m2m2_app_common_status_t, p_resp_payload);

  p_resp_payload->command = M2M2_APP_COMMON_CMD_SENSOR_STATUS_QUERY_RESP;
  p_resp_payload->status = M2M2_APP_COMMON_STATUS_ERROR;

  if (g_state_bia.num_starts == 0) {
    p_resp_payload->status = M2M2_APP_COMMON_STATUS_STREAM_STOPPED;
  } else {
    p_resp_payload->status = M2M2_APP_COMMON_STATUS_STREAM_STARTED;
  }
  p_resp_payload->stream = M2M2_ADDR_MED_BIA_STREAM;
  p_resp_payload->num_subscribers = g_state_bia.num_subs;
  p_resp_payload->num_start_reqs = g_state_bia.num_starts;
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
static m2m2_hdr_t *bia_app_set_cal(m2m2_hdr_t *p_pkt) {
  /* Declare a pointer to access the input packet payload */
  PYLD_CST(p_pkt, bia_app_hs_rtia_sel_t, p_in_payload);

  /* Allocate memory for response payload */
  PKT_MALLOC(p_resp_pkt, bia_app_hs_rtia_sel_t, 0);
  if (NULL != p_resp_pkt) {
  /* Declare a pointer to the response packet payload */
  PYLD_CST(p_resp_pkt, bia_app_hs_rtia_sel_t, p_resp_payload);
  p_resp_payload->command = M2M2_BIA_APP_CMD_SET_HS_RTIA_CAL_RESP;
  p_resp_payload->status = M2M2_APP_COMMON_STATUS_OK;

  /* HS rtia selection  */
  AppBIACfg.HstiaRtiaSel = p_in_payload->hsritasel;
  NRF_LOG_INFO("HsRtiaSel=%d",AppBIACfg.HstiaRtiaSel);
  p_resp_payload->hsritasel = AppBIACfg.HstiaRtiaSel;
  user_applied_bia_hsrtia_sel = 1;

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
static m2m2_hdr_t *bia_app_stream_config(m2m2_hdr_t *p_pkt) {
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
      gnAd5940App = AD5940_APP_BIA;
      reset_bia_packetization();
     if (g_state_bia.num_starts == 0) {
      if (BiaAppInit() == BIA_SUCCESS) {
        g_state_bia.num_starts = 1;
        /*bia data stability initialize*/
        Init_bia_data_stability();
#ifdef BCM_ALGO
        if(lcfg_bcm_algo_en && (algo_start_count == 0))  {
          bcm_config_handle.sampling_freq = AppBIACfg.BiaODR;
          if(bcm_algo_init() ==  BCM_ALG_SUCCESS) {
            algo_start_count = 1;
            status = M2M2_APP_COMMON_STATUS_STREAM_STARTED;
          }else {            
            algo_start_count = 0;
            status = M2M2_APP_COMMON_STATUS_ERROR;
          }
        }
#endif
          status = M2M2_APP_COMMON_STATUS_STREAM_STARTED;
#ifdef DEBUG_BCM
        bcm_start_time = MCU_HAL_GetTick();
        pkt_count = 0;
#endif
      } else {
        status = M2M2_APP_COMMON_STATUS_STREAM_NOT_STARTED;
      }
    } else {
      g_state_bia.num_starts++;
      status = M2M2_APP_COMMON_STATUS_STREAM_IN_PROGRESS;
    }
    command = M2M2_APP_COMMON_CMD_STREAM_START_RESP;
    break;
  case M2M2_APP_COMMON_CMD_STREAM_STOP_REQ:
    if (g_state_bia.num_starts == 0) {
      status = M2M2_APP_COMMON_STATUS_STREAM_STOPPED;
    } else if (g_state_bia.num_starts == 1) {
      if (BiaAppDeInit()) {
        g_state_bia.num_starts = 0;
#ifdef BCM_ALGO
        if(lcfg_bcm_algo_en && (algo_start_count == 1)){
         if(BcmAlgReset(bcm_instance) ==  BCM_ALG_SUCCESS) {
           status = M2M2_APP_COMMON_STATUS_STREAM_STOPPED;
           algo_start_count = 0;
         }else{
           status = M2M2_APP_COMMON_STATUS_ERROR;
         }
        }else{
          status = M2M2_APP_COMMON_STATUS_STREAM_STOPPED;
        }
#else
        status = M2M2_APP_COMMON_STATUS_STREAM_STOPPED;
#endif
      } else {
        g_state_bia.num_starts = 1;
        status = M2M2_APP_COMMON_STATUS_ERROR;
      }
    } else {
      g_state_bia.num_starts--;
      status = M2M2_APP_COMMON_STATUS_STREAM_COUNT_DECREMENT;
    }
    command = M2M2_APP_COMMON_CMD_STREAM_STOP_RESP;
    break;
  case M2M2_APP_COMMON_CMD_STREAM_SUBSCRIBE_REQ:
    g_state_bia.num_subs++;
    if(g_state_bia.num_subs == 1)
    {
       /* reset pkt sequence no. only during 1st sub request */
       g_state_bia.data_pkt_seq_num = 0;
#ifdef BCM_ALGO
       bcm_algo_data_pkt_seq_num = 0;
#endif
    }
    post_office_setup_subscriber(M2M2_ADDR_MED_BIA, M2M2_ADDR_MED_BIA_STREAM, p_pkt->src, true);
    post_office_setup_subscriber(M2M2_ADDR_MED_BIA, M2M2_ADDR_BCM_ALGO_STREAM, p_pkt->src, true);
    status = M2M2_APP_COMMON_STATUS_SUBSCRIBER_ADDED;
    command = M2M2_APP_COMMON_CMD_STREAM_SUBSCRIBE_RESP;
    break;
  case M2M2_APP_COMMON_CMD_STREAM_UNSUBSCRIBE_REQ:
    if (g_state_bia.num_subs <= 1) {
      g_state_bia.num_subs = 0;
      status = M2M2_APP_COMMON_STATUS_SUBSCRIBER_REMOVED;
    } else {
      g_state_bia.num_subs--;
      status = M2M2_APP_COMMON_STATUS_SUBSCRIBER_COUNT_DECREMENT;
    }
    post_office_setup_subscriber(M2M2_ADDR_MED_BIA, M2M2_ADDR_MED_BIA_STREAM, p_pkt->src, false);
    post_office_setup_subscriber(M2M2_ADDR_MED_BIA, M2M2_ADDR_BCM_ALGO_STREAM, p_pkt->src, false);
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
static m2m2_hdr_t *bia_app_reg_access(m2m2_hdr_t *p_pkt) {
  M2M2_APP_COMMON_STATUS_ENUM_t status = M2M2_APP_COMMON_STATUS_ERROR;
  /* Declare a pointer to access the input packet payload */
  PYLD_CST(p_pkt, bia_app_lcfg_op_hdr_t, p_in_payload);
  /* Allocate a response packet with space for the correct number of operations */
  PKT_MALLOC(p_resp_pkt, bia_app_lcfg_op_hdr_t, p_in_payload->num_ops * sizeof(p_in_payload->ops[0]));
  if (NULL != p_resp_pkt) {
  PYLD_CST(p_resp_pkt, bia_app_lcfg_op_hdr_t, p_resp_payload);
  float  reg_data = 0;

  switch (p_in_payload->command) {
  case M2M2_APP_COMMON_CMD_READ_LCFG_REQ:
    for (int i = 0; i < p_in_payload->num_ops; i++) {
      if (BiaReadLCFG(p_in_payload->ops[i].field, &reg_data) == BIA_SUCCESS) {
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
      if (BiaWriteLCFG(p_in_payload->ops[i].field, p_in_payload->ops[i].value) == BIA_SUCCESS) {
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
static m2m2_hdr_t *bia_app_decimation(m2m2_hdr_t *p_pkt) {
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
    g_state_bia.decimation_factor = p_in_payload->dec_factor;
    if (g_state_bia.decimation_factor == 0) {
      g_state_bia.decimation_factor = 1;
    }
    p_resp_payload->command = M2M2_SENSOR_COMMON_CMD_SET_STREAM_DEC_FACTOR_RESP;
    break;
  }
    p_resp_payload->status = status;
    p_resp_payload->dec_factor = g_state_bia.decimation_factor;
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
BIA_ERROR_CODE_t BiaAppInit() {
  BIA_ERROR_CODE_t retVal = BIA_ERROR;
  /* Ldo power on */
  adp5360_enable_ldo(ECG_LDO,true);


  /* switch off other switches */

/*
#ifdef ENABLE_ECG_APP
  DG2502_SW_control_AD8233(false);
#endif
  DG2502_SW_control_ADPD4000(false);
*/
   /* Initialization of application parameters */
  InitCfg();

   /* start bia initialization */
   ad5940_bia_start();
   gBiaAppInitFlag = true;
  /* Control BIA measurment to start. Second parameter has no meaning with this command. */
  AppBIACtrl(APPCTRL_START, 0);
  NRF_LOG_INFO("Bia ODR=%d",AppBIACfg.BiaODR);
  retVal = BIA_SUCCESS;

  return retVal;

}

/*!
  ****************************************************************************
 * @brief  Bia App de Initialization
 * @param  None
 * @return BIA_ERROR_CODE_t: Success/Error
 *****************************************************************************/
BIA_ERROR_CODE_t BiaAppDeInit() {
  /* BIA stop sync */
  if(AppBIACtrl(APPCTRL_STOPSYNC, 0)!=0){
    return BIA_ERROR;
  }
  /* Interrupts de init */
  ad5940_port_deInit();
  gBiaAppInitFlag = false;
  /* de init ldo power */
  adp5360_enable_ldo(ECG_LDO,false);
  return BIA_SUCCESS;
}

/*!
  ****************************************************************************
 * @brief   convert_uint32_to_float
 * @param   Hex value
 * @return  float value
 *****************************************************************************/
float convert_uint32_to_float (uint32_t num)
{
    //// num comes in as 32 bits or 8 byte  
    float original_num = num;
    uint32_t  sign = num >>31;
    int32_t  exponent = ((num >> 23) & 0xff) - 0x7F;
    uint32_t  mantissa = num << 9;
    float  value = 0;
    float mantissa2;

    NRF_LOG_INFO("input is : %d", num);

    value = 0.5f;
    mantissa2 = 0.0f;
    while (mantissa)
    {
        if (mantissa & 0x80000000)
            mantissa2 += value;
        mantissa <<= 1;
        value *= 0.5f;
    }

      NRF_LOG_INFO("Sign:%d,exponent:%d, mantissa2:%f", sign,exponent,mantissa2);

      /* value = sign * (1.0f + mantissa2) * (pow (2, exponent)); */
      value = (1.0f + mantissa2) * (pow (2, exponent));
     if (sign) value = -value;
    NRF_LOG_INFO("Float value:%f", value);
    return value;
}

/*!
  ****************************************************************************
  * @brief    Example of how to write an LCFG parameter
  * @param    field: LCFG field that has to be written
  * @param    value: Value to be written
  * @retval   BIA_ERROR_CODE_t
 *****************************************************************************/
BIA_ERROR_CODE_t BiaWriteLCFG(uint8_t field, float value) {
  AppBIACfg_Type *pCfg;
  AppBIAGetCfg(&pCfg);
  if(field < BIA_LCFG_MAX){
    switch(field){
    case BIA_LCFG_FS: /* field 0 */
      pCfg->BiaODR = value;
      bia_user_applied_odr = 1;
      break;
    case BIA_LCFG_ADC_PGA_GAIN: /* field 1 */
      pCfg->ADCPgaGain = value;
      user_applied_pga_gain = 1;
      break;
    case BIA_LCFG_PWR_MOD:/* field 2 */
      pCfg->PwrMod = value;
      user_applied_bia_pwr_mod = 1;
      break;
   case BIA_LCFG_SIN_FREQ: /* field 3 */
      pCfg->SinFreq= value;
      user_applied_sin_freq = 1;
      break;
   case BIA_DFT_NUM: /* field 4 */
      pCfg->DftNum= value;
      bia_user_applied_dft_num = 1;
      break;
#ifdef BCM_ALGO
   case BCM_FFM_DATA_RATE: /* field 5 */
     // g_state_bia.algo_decimation_factor = value;
      bcm_config_handle.output_data_rate = value;
      user_applied_algo_decimation_factor = 1;
      break;
   case BCM_ALGO_ENABLE:/* field 6 */
      lcfg_bcm_algo_en = value;
     break;
    case BCM_ALGO_HEIGHT:/* field 7 */
      bcm_config_handle.adi_vsm_bcm_algo_config.height = value;
      break;
    case BCM_ALGO_WEIGHT:/* field 8 */
      bcm_config_handle.adi_vsm_bcm_algo_config.weight = value;
     break;
    case BCM_ALGO_AGE: /* field 9 */
      bcm_config_handle.adi_vsm_bcm_algo_config.age = value;
     break;
    case BCM_ALGO_COEFF_CONFIG_ZERO: /* field 10 */
      bcm_config_handle.adi_vsm_bcm_algo_config.b[0] = value;
      break;
    case BCM_ALGO_COEFF_CONFIG_ONE: /* field 11 */
      bcm_config_handle.adi_vsm_bcm_algo_config.b[1] = value;
      break;
    case BCM_ALGO_COEFF_CONFIG_TWO: /* field 12 */
      bcm_config_handle.adi_vsm_bcm_algo_config.b[2] = value;
      break;
    case BCM_ALGO_COEFF_CONFIG_THREE: /* field 13 */
      bcm_config_handle.adi_vsm_bcm_algo_config.b[3] = value;
      break;
     case BCM_ALGO_COEFF_CONFIG_FOUR: /* field 14 */
      bcm_config_handle.adi_vsm_bcm_algo_config.b[4] = value;
      break;
     case BCM_ALGO_COEFF_CONFIG_FIVE: /* field 15 */
      bcm_config_handle.adi_vsm_bcm_algo_config.b[5] = value;
      break;
     case BCM_ALGO_COEFF_CONFIG_SIX: /* field 16 */
      bcm_config_handle.adi_vsm_bcm_algo_config.b[6] = value;
      break;
     case BCM_ALGO_COEFF_CONFIG_SEVEN: /* field 17 */
      bcm_config_handle.adi_vsm_bcm_algo_config.b[7] = value;
      break;
#endif
  }
#ifdef BCM_ALGO
    /* if bcm algo en, bia running, update config */
     if(g_state_bia.num_starts == 1){
      if(lcfg_bcm_algo_en){
        /* update configuration */
        adi_vsm_bcm_update_config(bcm_instance,&bcm_config_handle);
      }
     }
#endif
    return BIA_SUCCESS;
  }
  return BIA_ERROR;
}

/*!
  ****************************************************************************
  * @brief    Read LCFG parameter
  * @param    index: LCFG field
  * @param    value: Returned corresponding LCFG value
  * @retval   BIA_ERROR_CODE_t
 *****************************************************************************/
BIA_ERROR_CODE_t BiaReadLCFG(uint8_t index, float *value) {
AppBIACfg_Type *pCfg;
AppBIAGetCfg(&pCfg);
if(index < BIA_LCFG_MAX){
    switch(index){
    case BIA_LCFG_FS: /* field 0 */
      *value = (uint64_t)(pCfg->BiaODR);
      break;
    case BIA_LCFG_ADC_PGA_GAIN: /* field 1 */
      *value = pCfg->ADCPgaGain;
      break;
    case BIA_LCFG_PWR_MOD: /* field 2 */
      *value = pCfg->PwrMod;
      break;
    case BIA_LCFG_SIN_FREQ: /* field 3 */
      *value =  (uint64_t)(pCfg->SinFreq);
      break;
    case BIA_DFT_NUM: /* field 4 */
      *value = pCfg->DftNum;
      break;
#ifdef BCM_ALGO
    case BCM_FFM_DATA_RATE: /* field 5 */
      //*value = g_state_bia.algo_decimation_factor;
      *value = bcm_config_handle.output_data_rate;
      break;
   case BCM_ALGO_ENABLE:/* field 6 */
      *value = lcfg_bcm_algo_en;
      break;
    case BCM_ALGO_HEIGHT:/* field 7 */
     *value = bcm_config_handle.adi_vsm_bcm_algo_config.height;
     break;
    case BCM_ALGO_WEIGHT:/* field 8 */
      *value = bcm_config_handle.adi_vsm_bcm_algo_config.weight;
      break;
    case BCM_ALGO_AGE: /* field 9 */
      *value = bcm_config_handle.adi_vsm_bcm_algo_config.age;
      break;
    case BCM_ALGO_COEFF_CONFIG_ZERO: /* field 10 */
      *value = bcm_config_handle.adi_vsm_bcm_algo_config.b[0];
      break;
    case BCM_ALGO_COEFF_CONFIG_ONE: /* field 11 */
      *value = bcm_config_handle.adi_vsm_bcm_algo_config.b[1];
      break;
    case BCM_ALGO_COEFF_CONFIG_TWO: /* field 12 */
     *value = bcm_config_handle.adi_vsm_bcm_algo_config.b[2];
      break;
    case BCM_ALGO_COEFF_CONFIG_THREE: /* field 13 */
     *value = bcm_config_handle.adi_vsm_bcm_algo_config.b[3];
      break;
     case BCM_ALGO_COEFF_CONFIG_FOUR: /* field 14 */
      *value = bcm_config_handle.adi_vsm_bcm_algo_config.b[4];
      break;
     case BCM_ALGO_COEFF_CONFIG_FIVE: /* field 15 */
     *value = bcm_config_handle.adi_vsm_bcm_algo_config.b[5];
      break;
     case BCM_ALGO_COEFF_CONFIG_SIX: /* field 16 */
     *value = bcm_config_handle.adi_vsm_bcm_algo_config.b[6];
      break;
     case BCM_ALGO_COEFF_CONFIG_SEVEN: /* field 17 */
     *value = bcm_config_handle.adi_vsm_bcm_algo_config.b[7];
      break;
#endif
   }
    return BIA_SUCCESS;
  }
  return BIA_ERROR;
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
  *@brief      Convert into Cartesian form
  *@param      Pointer to float real/imaginary
  *@return     SUCCESS
******************************************************************************* */
void ConvertToCartesianForm(fImpCar_Type *pData){
  fImpCar_Type pRes;
  pRes.Real = (pData->Real)*cos(pData->Image);
  pRes.Image = (pData->Real)*sin(pData->Image);
  NRF_LOG_INFO("%d,%d",pRes.Real,pRes.Image);
  pData->Real = pRes.Real;
  pData->Image = pRes.Image;
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
 * @param    bia_dcb_data: pointer to dcb struct variable,
 *            in size of data in Double Word (32-bits)
 * @param    read_size: The size of the record to be returned to the user
 * @retval   BIA_ERROR_CODE_t: Status
 *****************************************************************************/
BIA_ERROR_CODE_t read_bia_dcb(uint32_t *bia_dcb_data, uint16_t *read_size) {
  BIA_ERROR_CODE_t dcb_status = BIA_ERROR;

  if (adi_dcb_read_from_fds(ADI_DCB_BIA_BLOCK_IDX, bia_dcb_data, read_size) == DEF_OK) {
    dcb_status = BIA_SUCCESS;
  }
  return dcb_status;
}

/*!
 ****************************************************************************
 * @brief    Sets the entire bcm DCB configuration in flash
 * @param    bia_dcb_data: pointer to dcb struct variable,
 *            in size of data in Double Word (32-bits)
 * @param    write_Size: The size of the record to be written
 * @retval   BIA_ERROR_CODE_t: Status
 *****************************************************************************/
BIA_ERROR_CODE_t write_bia_dcb(uint32_t *bia_dcb_data, uint16_t write_Size) {
  BIA_ERROR_CODE_t dcb_status = BIA_ERROR;

  if (adi_dcb_write_to_fds(ADI_DCB_BIA_BLOCK_IDX, bia_dcb_data, write_Size) == DEF_OK) {
    dcb_status = BIA_SUCCESS;
  }
  return dcb_status;
}

/*!
 ****************************************************************************
 * @brief    Delete the entire bcm DCB configuration in flash
 * @param    None
 * @retval   BIA_ERROR_CODE_t: Status
 *****************************************************************************/
BIA_ERROR_CODE_t delete_bia_dcb(void) {
  BIA_ERROR_CODE_t dcb_status = BIA_ERROR;

  if (adi_dcb_delete_fds_settings(ADI_DCB_BIA_BLOCK_IDX) == DEF_OK) {
    dcb_status = BIA_SUCCESS;
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
  g_bia_dcb_Present = set_flag;
  NRF_LOG_INFO("BCM DCB present set: %s",
      (g_bia_dcb_Present == true ? "TRUE" : "FALSE"));
}

/*!
 ****************************************************************************
 * @brief    Get whether the dcb is present in flash
 * @param    None
 * @retval   bool: TRUE: dcb present, FALSE: dcb absent
 *****************************************************************************/
bool bia_get_dcb_present_flag(void) {
  NRF_LOG_INFO(
      "BCM DCB present: %s", (g_bia_dcb_Present == true ? "TRUE" : "FALSE"));
  return g_bia_dcb_Present;
}

/*!
 ****************************************************************************
 * @brief    Update the global dcb presence flag in flash
 * @param    None
 * @retval   None
 *****************************************************************************/
void bia_update_dcb_present_flag(void) {
  g_bia_dcb_Present = adi_dcb_check_fds_entry(ADI_DCB_BIA_BLOCK_IDX);
  NRF_LOG_INFO("Updated. BCM DCB present: %s",
      (g_bia_dcb_Present == true ? "TRUE" : "FALSE"));
}


/*!
 ****************************************************************************
 * @brief    Update the lcfg from dcb
 * @param    p_pkt: pointer to packet
 * @retval   m2m2_hdr_t
 *****************************************************************************/
static m2m2_hdr_t *bia_app_set_dcb_lcfg(m2m2_hdr_t *p_pkt) {
  uint32_t bia_dcb[MAXBIADCBSIZE] = {'\0'};
  float f_value;
  M2M2_APP_COMMON_STATUS_ENUM_t status = M2M2_APP_COMMON_STATUS_ERROR;
  PKT_MALLOC(p_resp_pkt, bia_app_dcb_lcfg_t, 0);
  if (NULL != p_resp_pkt) {
    PYLD_CST(p_resp_pkt, bia_app_dcb_lcfg_t, p_resp_payload);
    if (bia_get_dcb_present_flag() == true) {
      uint16_t size = MAXBIADCBSIZE;
      if (read_bia_dcb(&bia_dcb[0], &size) == BIA_SUCCESS) {
        for (uint16_t i = 0; i < size; i++) {
          /*
          each bia_dcb array element stores value in total 4
          bytes. for ex- ifvbia_dcb[i] = 0x00ABCDEF 
          Value = 0xCDEF 
          */
          /* field is iteration count */
          uint16_t field = i;

          /* copy value and convert into hex */
          if( i < MAX_DECIMAL_ENTRIES ){
            f_value = (float)bia_dcb[i];
          }
          else  {
            f_value = convert_uint32_to_float(bia_dcb[i]);
          }
          if (BiaWriteLCFG(field, f_value) == BIA_SUCCESS) {
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
static m2m2_hdr_t *bia_dcb_command_read_config(m2m2_hdr_t *p_pkt) {
  uint32_t dcbdata[MAXBIADCBSIZE];
  static uint16_t r_size = 0;
  M2M2_DCB_STATUS_ENUM_t status = M2M2_DCB_STATUS_ERR_NOT_CHKD;

  // Declare a pointer to access the input packet payload
  // PYLD_CST(p_pkt, m2m2_dcb_bcm_data_t, p_in_payload);
  PKT_MALLOC(p_resp_pkt, m2m2_dcb_bia_data_t, 0);
  if (NULL != p_resp_pkt) {
    // Declare a pointer to the response packet payload
    PYLD_CST(p_resp_pkt, m2m2_dcb_bia_data_t, p_resp_payload);

    r_size = MAXBIADCBSIZE;
    if (read_bia_dcb(&dcbdata[0], &r_size) == BIA_SUCCESS) {
      for (uint16_t i = 0; i < r_size; i++)  {
        p_resp_payload->dcbdata[i] = dcbdata[i];
        p_resp_payload->size = (r_size);
        status = M2M2_DCB_STATUS_OK;
      }
   }else {
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

static m2m2_hdr_t *bia_dcb_command_write_config(m2m2_hdr_t *p_pkt) {
  M2M2_DCB_STATUS_ENUM_t status = M2M2_DCB_STATUS_ERR_NOT_CHKD;
  uint32_t dcbdata[MAXBIADCBSIZE];
  
  /* Declare a pointer to access the input packet payload */
  PYLD_CST(p_pkt, m2m2_dcb_bia_data_t, p_in_payload);
  PKT_MALLOC(p_resp_pkt, m2m2_dcb_bia_data_t, 0);
  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_dcb_bia_data_t, p_resp_payload);
    for (int i = 0; i < p_in_payload->size; i++)  {
      dcbdata[i] = p_in_payload->dcbdata[i];
     }
      if (write_bia_dcb(&dcbdata[0], p_in_payload->size) == BIA_SUCCESS) {
        bcm_set_dcb_present_flag(true);
        status = M2M2_DCB_STATUS_OK;
      } else {
        status = M2M2_DCB_STATUS_ERR_ARGS;
      }

    p_resp_payload->status = status;
    p_resp_payload->command = M2M2_DCB_COMMAND_WRITE_CONFIG_RESP;
    p_resp_payload->size = 0;
    for(uint16_t i=0; i< MAXBIADCBSIZE; i++)
      p_resp_payload->dcbdata[i] = 0;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  }
  return p_resp_pkt;
}

static m2m2_hdr_t *bia_dcb_command_delete_config(m2m2_hdr_t *p_pkt) {
  M2M2_DCB_STATUS_ENUM_t status = M2M2_DCB_STATUS_ERR_NOT_CHKD;
  PKT_MALLOC(p_resp_pkt, m2m2_dcb_bia_data_t, 0);
  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_dcb_bia_data_t, p_resp_payload);

    if (delete_bia_dcb() == BIA_SUCCESS) {
      bcm_set_dcb_present_flag(false);
      status = M2M2_DCB_STATUS_OK;
    } else {
      status = M2M2_DCB_STATUS_ERR_ARGS;
    }

    p_resp_payload->status = status;
    p_resp_payload->command = M2M2_DCB_COMMAND_ERASE_CONFIG_RESP;
    p_resp_payload->size = 0;
    for(uint16_t i=0; i< MAXBIADCBSIZE; i++)
      p_resp_payload->dcbdata[i] = 0;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  }
  return p_resp_pkt;
}

/**
* @brief            Get Frequency index of excitation signal frequecny
*                   during the BIA packetization
*
* @param[in]        float frequency: excitation frequecny using for BIA measurement
*
* @return           Enum M2M2_SENSOR_BIA_SWEEP_FREQ_INDEX_ENUM_t   
*                    M2M2_SENSOR_BIA_FREQ_1000HZ = 0,
*                    M2M2_SENSOR_BIA_FREQ_3760HZ = 1,
*                    M2M2_SENSOR_BIA_FREQ_14140HZ = 2,
*                    M2M2_SENSOR_BIA_FREQ_53180HZ = 3,
*                    M2M2_SENSOR_BIA_FREQ_200KHZ = 4,
*                    M2M2_SENSOR_BIA_FREQ_50KHZ = 255,
*/
static M2M2_SENSOR_BIA_SWEEP_FREQ_INDEX_ENUM_t GetBIASweepFrequencyIndex(float frequency)  {
  /* TODO compare sweep frquency and rerurn index based on that
     currently assuming default frequecny is 50KHz always*/
  //if(frequecny == 50000){
   return M2M2_SENSOR_BIA_FREQ_50KHZ;
}

#ifdef BCM_ALGO
/*
---------------------------------------BCM Configurations -------------------------------------------------------
*/
#define STATE_MEM_PER_INSTANCE          100 //state memory size=71

#define NUM_INSTANCE                    1
#define STATE_BCM_MEM_NUM_CHARS         (NUM_INSTANCE * STATE_MEM_PER_INSTANCE)

/* Allocate a max amount of memory for the SQI Algo state memory block */
uint8_t STATE_memory_BCM[STATE_BCM_MEM_NUM_CHARS];

adi_vsm_bcm_mem_t bcm_memory_setup;

/**
* @brief       Configure BCM Algorithm Generic parameters
*
* @param[in]   input_sample_freq    sampling frequency of BCM signal
*              ODR                  Output data rate of BCM signal
* @param[in]   bcm_config_handle        pointer to struct. holding the config
*
* @return      BCM_ALG_RETURN_CODE_t
*/
BCM_ALG_RETURN_CODE_t BcmAlgGenericConfig(uint8_t input_sample_freq,
                                          uint8_t odr,
                                          adi_vsm_bcm_config_t* bcm_config_handle)  {
    NULL_POINTER_CHECK(bcm_config_handle);
    if (input_sample_freq < 1 || input_sample_freq > 100)
    {
        return BCM_ALG_ERROR;
    }
    /* Decimation not possible */
    if(odr > input_sample_freq)
    {
      return BCM_ALG_ERROR;
    }
    bcm_config_handle->sampling_freq = input_sample_freq;
    return BCM_ALG_SUCCESS;
}

/**
* @brief       Configure BCM Algorithm Body parameters
*
* @param[in]   height               Height of person
*              weight               Weight of person
*              age                  Age of person
* @param[in]   bcm_config_handle        pointer to struct. holding the config
*
* @return      BCM_ALG_RETURN_CODE_t
*/
BCM_ALG_RETURN_CODE_t BcmAlgBodyParametersConfig(uint8_t height,
                                                uint8_t weight,
                                                uint8_t age,
                                                adi_vsm_bcm_config_t* bcm_config_handle)  {
    NULL_POINTER_CHECK(bcm_config_handle);
    bcm_config_handle->adi_vsm_bcm_algo_config.age = age;
    bcm_config_handle->adi_vsm_bcm_algo_config.height = age;
    bcm_config_handle->adi_vsm_bcm_algo_config.weight = age;

    return BCM_ALG_SUCCESS;
}

/**
* @brief       Configure BCM Algorithm Coefficients
*
* @param[in]   coeff                Pointer to an array of coefficients
* @param[in]   bcm_config_handle        pointer to struct. holding the config
*
* @return       BCM_ALG_RETURN_CODE_t
*/
BCM_ALG_RETURN_CODE_t BcmAlgCoeffConfig(float coeff[],
                                       adi_vsm_bcm_config_t* bcm_config_handle)  {
    NULL_POINTER_CHECK(bcm_config_handle);
    memcpy(bcm_config_handle->adi_vsm_bcm_algo_config.b,coeff,(sizeof(float)*ALLOWED_POLY_COEFFICIENTS));

    return BCM_ALG_SUCCESS;
}

/**
* @brief        BCM Algorithm Init, Creates BCM Instance
*
* @param[in]    bcm_config_handle        pointer to struct. holding the config
* @param[out]   instance             double pointer to BCM instance to be created
*
* @return       BCM_ALG_RETURN_CODE_t
*/
BCM_ALG_RETURN_CODE_t BcmAlgInit(const adi_vsm_bcm_config_t* bcm_config_handle,
                                  adi_vsm_bcm_instance_t** instance)  {
    NULL_POINTER_CHECK(bcm_config_handle);
    NULL_POINTER_CHECK(instance);

    /* initialize the memory object for the BCM instance */
    bcm_memory_setup.state.block = STATE_memory_BCM;
    bcm_memory_setup.state.length_numchars = STATE_MEM_PER_INSTANCE;

    /* Create the BCM Measurement instance */
    *instance = adi_vsm_bcm_create(&bcm_memory_setup, bcm_config_handle);
    if (*instance == NULL) {
        return BCM_ALG_ERROR;
    }
    return BCM_ALG_SUCCESS;
}


/**
* @brief        Do BCM Instance reset
*
* @param[in]    instance            pointer to BCM instance
*
* @return       BCM_ALG_RETURN_CODE_t
*/
BCM_ALG_RETURN_CODE_t BcmAlgReset(adi_vsm_bcm_instance_t* instance) {
    NULL_POINTER_CHECK(instance);
    adi_vsm_bcm_return_code_t ret_code = ADI_VSM_BCM_ERROR;

    ret_code = adi_vsm_bcm_algo_reset(instance);
    RETURN_CODE(ret_code);
}


/**
* @brief        Do BCM Algorithm process
*
* @param[in]    instance                pointer to BCM instance
* @param[in]    resistive_sample        resistive sample to be processed
* @param[in]    reactive_sample         reactive sample to be processed
* @param[out]   adi_vsm_bcm_output      pointer to struct containing BCM output
*
* @return       BCM_ALG_RETURN_CODE_t
*/
BCM_ALG_RETURN_CODE_t BcmAlgProcess(adi_vsm_bcm_instance_t * instance,
                                    float *bodyImpedance,
                                    adi_vsm_bcm_output_t * adi_vsm_bcm_output)  {
    NULL_POINTER_CHECK(instance);
    NULL_POINTER_CHECK(adi_vsm_bcm_output);
    adi_vsm_bcm_return_code_t ret_code = ADI_VSM_BCM_ERROR;
    ret_code = adi_vsm_bcm_process(instance,bodyImpedance,
                                    adi_vsm_bcm_output);
    RETURN_CODE(ret_code);
}

/**
* @brief        Do BCM Algo init
*
* @param[in]    NONE
*
* @return       BCM_ALG_RETURN_CODE_t
*/
BCM_ALG_RETURN_CODE_t bcm_algo_init() {
  BCM_ALG_RETURN_CODE_t ret_code;
  ret_code = BcmAlgInit(&bcm_config_handle,&bcm_instance);
  return ret_code;
}

/**
* @brief            Check for BIA data stability
*
* @param[in]        float body_impedance_real     : impedance Real value
* @param[in]        float body_impedance_imaginary: impedance Imaginary value
*
* @return           0-bia data not stable, 1-bia data stable
*/
uint8_t check_bia_data_stability(float body_impedance_real,float body_impedance_imaginary)  {
  uint8_t nElectrodeStatus = 0;
  float Ohmsdiff = 0;

  if (((body_impedance_real > 100) && (body_impedance_real < 5000)) && ((body_impedance_imaginary > -500) && (body_impedance_imaginary < 100)))
  {
    nElectrodeStatus = 1; /* set the electrode status = 1 // touched */
  }
  else
  {
    nElectrodeStatus = 0;
  }

  if (nElectrodeStatus == 1)
  {
    gbia_data_buffer[gbia_data_count++] = body_impedance_real;
    if(gbia_data_count == BIA_DATA_BUFFER_SIZE) { 
      gbia_data_count = 0;
      Ohmsdiff = gbia_data_buffer[0] - gbia_data_buffer[BIA_DATA_BUFFER_SIZE -1];
      memset(gbia_data_buffer,0,sizeof(gbia_data_buffer));
      if (Ohmsdiff < BIA_STABILITY_OHMS_DIFF) //Checking ohms difference
      {
        if (++gbia_stability_samplecount >= BIA_STABILITY_SAMPLES_COUNT) //counting samples
        {
            gnBiaDataStable = 1; // set signal stability
            gbia_stability_samplecount = 0;
        }
      }else {
        gbia_stability_samplecount = 0;
        gnBiaDataStable = 0;
      }
    }
  }else{
    Init_bia_data_stability();
  }

  return gnBiaDataStable;
}

/**
* @brief            Init for BIA data stability check
*
* @param[in]        None
*
* @return           None
*/
void Init_bia_data_stability()  {
  memset(gbia_data_buffer,0,sizeof(gbia_data_buffer));
  gbia_data_count = 0;
  gbia_stability_samplecount = 0;
  gnBiaDataStable = 0;
}


#endif
#endif
#endif

