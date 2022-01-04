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
extern bool g_disable_ad5940_port_init;
extern bool g_disable_ad5940_port_deinit;
#ifdef BIA_DCFG_ENABLE

uint8_t bia_load_dcfg=1;
volatile uint8_t bia_user_applied_dcfg = 0;

uint64_t g_current_dcfg_bia[MAX_DEFAULT_DCFG_REGISTER_NUM_BIA];
uint32_t g_user_bia_config_dcfg[MAX_USER_CONFIG_REGISTER_NUM_BIA];
uint8_t g_user_bia_indexes[MAX_USER_CONFIG_REGISTER_NUM_BIA];

uint8_t g_bia_num_ind_settings=0;

#ifdef BCM_ALGO
float prev_ffm_estimated=0;
float prev_bmi=0;
float prev_fatpercent=0;
#endif

// Register numbering used here starts from 0
typedef enum BIA_DCFG_REGISTERS_ENUM_t{

  BIA_SEQUENCER_CONFIG = 4,
  BIA_COMMAND_FIFO_CONFIG = 9,// Command FIFO Mode and Size allowed 
  BIA_FIFO_THRESHOLD_CONFIG = 10,
  BIA_DATA_FIFO_SOURCE_CONFIG = 11,// This Index is saved after reconfiguring FIFO,source of FIFO
  BIA_HIGH_SPEED_RTIA_CONFIG  = 19,
  BIA_WAVEGEN_FREQUENCY_CONFIG = 26, 
  BIA_WAVEGEN_AMPLITUDE_CONFIG = 27,
  BIA_WAVEGEN_OFFSET_CONFIG    = 28, 
  BIA_WAVEGEN_PHASE_CONFIG     = 29,
  BIA_WAVEGEN_CONFIG           = 30,
  BIA_ADC_PGA_GAIN             = 36,
  BIA_ADC_OUTPUT_FILTER_CONFIG = 43,
  BIA_DFT_CONFIG               = 44,
  BIA_DCFG_MAX_CONFIG          = 76,
}BIA_DCFG_REGISTERS_ENUM;

typedef enum BIA_USER_DCFG_REGISTERS_ENUM_t{
  /* list of indexes to be given to user */
  BIA_USER_SEQCFG_REG_COMMAND_SEQUENCER_CONFIG = 0,
  BIA_USER_SEQCFG_REG_COMMAND_FIFO_CONFIG = 1,
  BIA_USER_SEQCFG_REG_FIFO_THRESHOLD_CONFIG = 2,
  BIA_USER_SEQCFG_REG_FIFO_SOURCE_CONFIG = 3,
  BIA_USER_SEQCFG_REG_WAVEGEN_FREQUENCY_CONFIG = 4,
  BIA_USER_SEQCFG_REG_WAVEGEN_AMPLITUDE_CONFIG = 5,
  BIA_USER_SEQCFG_REG_WAVEGEN_OFFSET_CONFIG = 6,
  BIA_USER_SEQCFG_REG_WAVEGEN_PHASE_CONFIG = 7,
  BIA_USER_SEQCFG_REG_WAVEGEN_CONFIG = 8,
  BIA_USER_SEQCFG_REG_ADC_OUTPUT_FILTER_CONFIG = 9,
  BIA_USER_SEQCFG_REG_DFT_CONFIG = 10,
  BIA_MAX_USER_SEQCFG = 11,
}BIA_USER_DCFG_REGISTERS_ENUM;

uint64_t default_dcfg_bia[] = {
    /*Start of Init_1*/
    0x0000200800000000, /* Disabling fifo before changing memory configuration */ 
    0x000021d800000489, /* Command and Data control register setting */
    0x0000200400000000, /* Disable Sequencer */
    0x0000206400000000, /* Clear Sequencer Count and CRC */
    0x0000200400000000, /* Disable sequencer */ 
    0x0000200800004800, /* Restore FIFO configuration */
    0xFFFFFFFFFFFFFFFF,
    /*Start of Init_2*/
    0x0000200800004000, /* Keeping FIFO source DFT and disabling FIFO */ 
    0x0000200800000000, /* Disabling FIFO */
    0x000021d800000489, /* FIFO memory Configuartion */
    0x000021e000040000, /* Setting FIFO Threshold */
    0x0000200800004800, /* Keeping FIFO Source and enabling it */
    0x00003004ffffffff, /* Clearing all Interrupts */
    0xFFFFFFFFFFFFFFFF,
    /*Start of sequencer configuration*/                         
    0x0000200000190e40, /* Enable High Power Referenec Voltage */
    0x0000218000000037, /* Power Buffer Configurations */
    0x0000205000000000, /* Enabling Low power Reference */
    0x000020100000000e, /* High speed DAC configuration */
    0x000020fc00000000, /* High Speed TIA configuration */
    0x000020f000000201, /* High Speed Rtia configuration */
    0x000020f8000000fd, /* DE0 High Speed TIA Resistors Configuration */
    0x0000215000000000, /* D switch matrix configuration */
    0x0000215800006000, /* P switch matrix configuration */
    0x0000215400000c00, /* N switch matrix configuration */
    0x0000215c00000100, /* T switch matrix configuration */
    0x0000200c00010000, /* Switch Matrix configuration */
    0x0000203000333333, /* Waveform Generator, Sinusoid Frequency Control Word */
    0x0000203c000007ff, /* Waveform Generator, Sinusoid Amplitude configuration */
    0x0000203800000000, /* Waveform Generator, Sinusoid Offset */
    0x0000203400000000, /* Waveform Generator, Sinusoid Phase Offset */
    0x0000201400000004, /* Waveform Generator Configuration */
    0x0000212800000001, /* Low Power DAC configuration */
    0x000021200001f68b, /* Low Power DAC output data configuration */
    0x000021240000003e, /* Low Power DAC switch control register */
    0x000020ec00004000, /* Low Power TIA Configuration */
    0x000020e4000033e0, /* Low Power TIA Switch Configuration */
    0x000021a800000101, /* ADC Configuration Register */
    0x000020440000e011, /* ADC Output Filter Configuration */
    0x0000200000190e40, /* AFE Configuration */
    0x000020a800000000, /* ADC Mincheck value */
    0x000020ac00000000, /* ADC Hysterisis value */
    0x000020b000000000, /* ADC Maxvalue check */
    0x000020b400000000, /* ADC Maximum hysterisis value */
    0x000020440000e011, /* ADC Output Filter Configuration */
    0x000020d0001000b1, /* DFT Number ,Hanning Window Configuration*/
    0x000021c400000000, /* Statistics Number Configuration */ 
    0x0000200000194e40, /* AFE Configuration */
    0x0000205400000000, /* GPIO Configuration from Sequencer */
    0xFFFFFFFFFFFFFFFF, /*end of sequencer configuration*/
    /*START OF MEASURE */
    0x0000205400000008, /*  GPIO Configuration from Sequencer */
    0xeeeeeeee00000fa0, /*  static Delay */
    0x0000215000000010, /*  Switch Matrix Full Configuration D */
    0x0000215800000400, /*  P switch matrix configuration */
    0x0000215400000002, /*  N switch matrix configuration */
    0x0000215c00000102, /*  T switch matrix configuration */
    0x0000200c00010000, /*  Switch Matrix configuration */
    0x000021a800000101, /*  ADC Configuration Register */  
    0x0000200000194ec0, /*  AFE Configuration */  
    0xeeeeeeee00000320, /*  static Delay */
    0x000020000019cfc0, /*  AFE Configuration */ 
    0xeeeeeeee0005007d, /*  WaitClks Setting */
    0x0000200000190e40, /*  AFE Configuration */ 
    0x000021a800000607, /*  ADC Configuration Register */ 
    0x0000200000194ec0, /*  AFE Configuration */
    0xeeeeeeee00000320, /*  static Delay */
    0x000020000019cfc0, /*  AFE Configuration */ 
    0xeeeeeeee0005007d, /*  WaitClks Setting */
    0x0000200000190e40, /*  AFE Configuration */ 
    0x0000215000000000, /*  Switch Matrix Full Configuration D */  
    0x0000215800006000, /*  P switch matrix configuration */  
    0x0000215400000c00, /*  N switch matrix configuration */
    0x0000215c00000100, /*  T switch matrix configuration */
    0x0000200c00010000, /*  Switch Matrix configuration */
    0x0000205400000000, /*  GPIO Configuration from Sequencer */
    0x0000211c00000000, /*  Sequencer Trigger Sleep */
    0x0000211c00000001, /*  Sequencer Trigger Sleep */
    0xFFFFFFFFFFFFFFFF, /*  End of Sequencer Measurement */
};
#endif
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
static uint32_t GetBIASweepFrequencyIndex();
#ifdef DEBUG_DCB
static m2m2_hdr_t *adi_dcb_fds_stat(m2m2_hdr_t *p_pkt);
#endif
#ifdef BIA_DCFG_ENABLE
void load_bia_default_dcfg_config(uint64_t *cfg);
static m2m2_hdr_t *bia_app_dcfg_reg_update(m2m2_hdr_t *p_pkt);
#endif
static void fetch_bia_data(void);
static void sensor_bia_task(void *pArgument);
void Enable_ephyz_power(void);
static void InitCfg();
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
static volatile uint8_t user_applied_sin_freq = 0;
static volatile uint8_t user_applied_bia_pwr_mod = 0;
static volatile uint8_t user_applied_sweep_config = 0;
static volatile uint8_t user_applied_algo_decimation_factor = 0;
static volatile uint8_t user_applied_dac_voltage = 0;

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
#define MAX_FLOAT_ENTRIES 8
#define START_IEEE_FLOAT_INDEX 10
#define END_IEEE_FLOAT_INDEX 17

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
#ifndef BIA_DCFG_ENABLE
  if(!user_applied_bia_hsrtia_sel) {
    AppBIACfg.HstiaRtiaSel = HSTIARTIA_1K;
  } else {
    user_applied_bia_hsrtia_sel = 0;
  }
#endif
  AppBIACfg.CtiaSel = 16;
  AppBIACfg.ExcitBufGain = EXCITBUFGAIN_2;
  AppBIACfg.HsDacGain = HSDACGAIN_1;
  AppBIACfg.HsDacUpdateRate = 7;
  AppBIACfg.DacVoltPP = 800.0;

#ifndef BIA_DCFG_ENABLE
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
#endif

  AppBIACfg.ADCSinc3Osr = ADCSINC3OSR_2;
  AppBIACfg.ADCSinc2Osr = ADCSINC2OSR_22;
#ifndef BIA_DCFG_ENABLE
  AppBIACfg.DftNum = DFTNUM_8192;
#endif
  AppBIACfg.DftSrc = DFTSRC_SINC3;
  AppBIACfg.HanWinEn = bTRUE;


#ifdef RANGE_SWEEP_ENABLE
  //if(!user_applied_sweep_config)  {
   /* private variables */
//    AppBIACfg.SweepCfg.SweepEn = bFALSE;
 //   AppBIACfg.SweepCfg.SweepStart = 1000.0;
 //   AppBIACfg.SweepCfg.SweepStop = 150000.0;
 //   AppBIACfg.SweepCfg.SweepPoints = 100;
 //   AppBIACfg.SweepCfg.SweepLog = bTRUE;
 //   AppBIACfg.SweepCfg.SweepIndex = 0;
 //}
#endif

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
#ifdef BIA_DCFG_ENABLE    
  {M2M2_BIA_APP_CMD_LOAD_DCFG_REQ, bia_app_stream_config},
  {M2M2_BIA_APP_COMMON_CMD_WRITE_DCFG_REQ, bia_app_dcfg_reg_update},
  {M2M2_BIA_APP_COMMON_CMD_READ_DCFG_REQ, bia_app_dcfg_reg_update},
#endif  
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
  /* Thread Name should be of max 10 Characters */
  sensor_bia_task_attributes.szThreadName = "BIA_Task";
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
     uint32_t nfrequency_index = GetBIASweepFrequencyIndex();
    /*Calculate Impedance in form of Cartesian form */
    if (p_pktizer->packet_nsamples == 0) {
      p_pktizer->p_pkt = post_office_create_msg(sizeof(bia_app_stream_t) + M2M2_HEADER_SZ);
      if(p_pktizer->p_pkt != NULL) {
        PYLD_CST(p_pktizer->p_pkt, bia_app_stream_t, p_payload_ptr);
        g_state_bia.bia_pktizer.packet_max_nsamples=(sizeof(p_payload_ptr->bia_data)/sizeof(p_payload_ptr->bia_data[0]));
        p_payload_ptr->bia_data[0].timestamp = gBiaData.biaImpedence.timestamp;
        /* multiply data by 1000 to save the float precision*/
        p_payload_ptr->bia_data[0].real = (int32_t)(gBiaData.biaImpedence.real * 1000);
        p_payload_ptr->bia_data[0].img = (int32_t)(gBiaData.biaImpedence.img * 1000);
        p_payload_ptr->bia_data[0].excitation_freq = nfrequency_index;
        p_pktizer->packet_nsamples++;
      }
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
      p_payload_ptr->bia_data[i].excitation_freq = nfrequency_index;
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
            
            /* store previous value */
            prev_ffm_estimated = adi_vsm_bcm_output.ffm_estimated;
            prev_fatpercent = adi_vsm_bcm_output.fatpercent;
            prev_bmi = adi_vsm_bcm_output.bmi;
          }
          else if(ret == BCM_ALG_IN_FFM_ESTIMATED_ERROR) {
            /* send previous ffm estimated value */
            adi_vsm_bcm_output.ffm_estimated = prev_ffm_estimated;
            /* send previous bmi value */
            adi_vsm_bcm_output.bmi = prev_bmi;
            /* send previous fat percent value */
            adi_vsm_bcm_output.fatpercent = prev_fatpercent;
            
            /* Copy to the payload */
            p_payload_ptr->ffm_estimated = adi_vsm_bcm_output.ffm_estimated;
            p_payload_ptr->bmi = adi_vsm_bcm_output.bmi;
            p_payload_ptr->fat_percent = adi_vsm_bcm_output.fatpercent;
          }
          else if(ret == BCM_ALG_NULL_PTR_ERROR || ret == BCM_ALG_ERROR) {
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

  if (NULL != p_resp_pkt) {
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
        else {
#endif
          status = M2M2_APP_COMMON_STATUS_STREAM_STARTED;
#ifdef BCM_ALGO
       }
#endif
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

#ifdef BIA_DCFG_ENABLE
  case M2M2_BIA_APP_CMD_LOAD_DCFG_REQ:

    if (!g_state_bia.num_starts) {
        user_applied_bia_hsrtia_sel = 0;
        user_applied_sin_freq = 0;
        user_applied_pga_gain = 0;
        bia_user_applied_dft_num = 0;
        load_bia_default_dcfg_config(&default_dcfg_bia[0]);
        status = M2M2_APP_COMMON_STATUS_OK;
      } else {
        status = M2M2_APP_COMMON_STATUS_ERROR;
      }
    /* clear flag to not load global array again if its loaded from load req*/
    bia_load_dcfg = 0;
    command = M2M2_BIA_APP_CMD_LOAD_DCFG_RESP;
    break;
#endif
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
  }// if(NULL != p_resp_pkt)
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

#ifdef BIA_DCFG_ENABLE


/**
      @brief    Map user Index 
      @param    User Index
      @retval   DCFG Array Index
*/
uint8_t bia_mapuserind(uint8_t *userind)  {
  uint8_t array_ind=0;
    switch(*userind)  {
     case BIA_USER_SEQCFG_REG_COMMAND_SEQUENCER_CONFIG:
      array_ind = BIA_SEQUENCER_CONFIG;
      break;
     case BIA_USER_SEQCFG_REG_COMMAND_FIFO_CONFIG:
      array_ind = BIA_COMMAND_FIFO_CONFIG;
      break;
     case BIA_USER_SEQCFG_REG_FIFO_THRESHOLD_CONFIG:
      array_ind = BIA_FIFO_THRESHOLD_CONFIG;
      break;
     case BIA_USER_SEQCFG_REG_FIFO_SOURCE_CONFIG:
      array_ind = BIA_DATA_FIFO_SOURCE_CONFIG;
      break;
     case BIA_USER_SEQCFG_REG_WAVEGEN_FREQUENCY_CONFIG:
      array_ind = BIA_WAVEGEN_FREQUENCY_CONFIG;
      break;
     case BIA_USER_SEQCFG_REG_WAVEGEN_AMPLITUDE_CONFIG:
      array_ind = BIA_WAVEGEN_AMPLITUDE_CONFIG;
      break;
     case BIA_USER_SEQCFG_REG_WAVEGEN_OFFSET_CONFIG:
      array_ind = BIA_WAVEGEN_OFFSET_CONFIG;
      break;
     case BIA_USER_SEQCFG_REG_WAVEGEN_PHASE_CONFIG:
      array_ind = BIA_WAVEGEN_PHASE_CONFIG;
      break;
     case BIA_USER_SEQCFG_REG_WAVEGEN_CONFIG:
      array_ind = BIA_WAVEGEN_CONFIG;
      break; 
     case BIA_USER_SEQCFG_REG_ADC_OUTPUT_FILTER_CONFIG:
      array_ind = BIA_ADC_OUTPUT_FILTER_CONFIG;
      break;
     case BIA_USER_SEQCFG_REG_DFT_CONFIG:
      array_ind = BIA_DFT_CONFIG;
      break;
     default:
      array_ind = BIA_DCFG_MAX_CONFIG;
      break;
    }
    return array_ind;
}

/**
      @brief    Map Adress to User Index 
      @param    Register Address
      @retval   User Array Index
*/
static uint8_t bia_mapaddrind(uint32_t *address)  {
  uint8_t user_ind=0;
    switch(*address)  {
     case BIA_SEQUENCER_CONFIGURATION_REGISTER:
      user_ind = BIA_USER_SEQCFG_REG_COMMAND_SEQUENCER_CONFIG;
      break;
     case BIA_FIFO_THRESHOLD_REGISTER:
      user_ind = BIA_USER_SEQCFG_REG_FIFO_THRESHOLD_CONFIG;
      break;
     case BIA_FIFO_CONFIGURATION_REGISTER:
      user_ind = BIA_USER_SEQCFG_REG_COMMAND_FIFO_CONFIG;
      break;
     case BIA_FIFO_SRC_CONFIGURATION_REGISTER:
      user_ind = BIA_USER_SEQCFG_REG_FIFO_SOURCE_CONFIG;
      break;
     case BIA_WAVEFORM_GENERATOR_FREQUENCY_REGISTER:
      user_ind = BIA_USER_SEQCFG_REG_WAVEGEN_FREQUENCY_CONFIG;
      break;
     case BIA_WAVEFORM_GENERATOR_AMPLITUDE_REGISTER:
      user_ind = BIA_USER_SEQCFG_REG_WAVEGEN_AMPLITUDE_CONFIG;
      break;
     case BIA_WAVEFORM_GENERATOR_OFFSET_REGISTER:
      user_ind = BIA_USER_SEQCFG_REG_WAVEGEN_OFFSET_CONFIG;
      break;
     case BIA_WAVEFORM_GENERATOR_PHASE_REGISTER:
      user_ind = BIA_USER_SEQCFG_REG_WAVEGEN_PHASE_CONFIG;
      break;
     case BIA_WAVEFORM_GENERATOR_CONFIGURATION_REGISTER:
      user_ind = BIA_USER_SEQCFG_REG_WAVEGEN_CONFIG;
      break;
     case BIA_ADC_OUTPUT_FILTER_CONFIGURATION_REGISTER:
      user_ind = BIA_USER_SEQCFG_REG_ADC_OUTPUT_FILTER_CONFIG;
      break;
     case BIA_DFT_CONFIGURATION_REGISTER:
      user_ind = BIA_USER_SEQCFG_REG_DFT_CONFIG;
      break;
     default:
       user_ind = BIA_MAX_USER_SEQCFG;
      break;
    }
    return user_ind;
}

/*!
 ****************************************************************************
 *@brief      BIA Dcfg read/write
 *@param      pPkt: pointer to the packet structure
 *@return     m2m2_hdr_t
 ******************************************************************************/
static m2m2_hdr_t *bia_app_dcfg_reg_update(m2m2_hdr_t *p_pkt) {
  M2M2_APP_COMMON_STATUS_ENUM_t status = M2M2_APP_COMMON_STATUS_ERROR;
  PYLD_CST(p_pkt, bia_app_dcfg_op_hdr_t, p_in_payload);
  /* Allocate a response packet with space for the correct number of operations
   */
  PKT_MALLOC(p_resp_pkt, bia_app_dcfg_op_hdr_t,p_in_payload->num_ops * sizeof(p_in_payload->ops[0]));
  if (NULL != p_resp_pkt) {
    PYLD_CST(p_resp_pkt, bia_app_dcfg_op_hdr_t, p_resp_payload);

    switch (p_in_payload->command) {
     static uint8_t dcfg_position=0;
     static uint8_t array_ind=0;
    case M2M2_BIA_APP_COMMON_CMD_WRITE_DCFG_REQ:
      /* Copy register addresses, value pairs */
      for (int i = 0; i < p_in_payload->num_ops; i++) {
        uint32_t addr = (uint32_t)p_in_payload->ops[i].field;
        /* Map Address of register to user index */
        dcfg_position = bia_mapaddrind(&addr);
        if(dcfg_position < BIA_MAX_USER_SEQCFG){
          g_user_bia_config_dcfg[dcfg_position] = p_in_payload->ops[i].value;
          g_user_bia_indexes[i] = dcfg_position;
          status = M2M2_APP_COMMON_STATUS_OK;
        }else {
          status = M2M2_APP_COMMON_STATUS_ERROR;
        }
        /* number of user settings */
        g_bia_num_ind_settings = p_in_payload->num_ops;
        p_resp_payload->ops[i].field = p_in_payload->ops[i].field;
        p_resp_payload->ops[i].value = p_in_payload->ops[i].value;
      }
      /* Update write flag */
      bia_user_applied_dcfg = 1;
      p_resp_payload->command = (M2M2_APP_COMMON_CMD_ENUM_t) M2M2_BIA_APP_COMMON_CMD_WRITE_DCFG_RESP;
     break;
    case M2M2_BIA_APP_COMMON_CMD_READ_DCFG_REQ:
      /* Copy register addresses, value pairs */
      for (int i = 0; i < p_in_payload->num_ops; i++) {
        uint32_t addr = (uint32_t)p_in_payload->ops[i].field; 
        dcfg_position = bia_mapaddrind(&addr);
        /* find index to set */
        array_ind = bia_mapuserind(&dcfg_position);
        if(array_ind < BIA_DCFG_MAX_CONFIG) {
          status = M2M2_APP_COMMON_STATUS_OK;
        }
        else  {
          status = M2M2_APP_COMMON_STATUS_ERROR;
        }
        p_resp_payload->ops[i].field = p_in_payload->ops[i].field;
       p_resp_payload->ops[i].value = g_current_dcfg_bia[array_ind];
      }
      p_resp_payload->command = (M2M2_APP_COMMON_CMD_ENUM_t) M2M2_BIA_APP_COMMON_CMD_READ_DCFG_RESP;
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
/**
      @brief    Load bia  lcfg configuration to working dcfg array
      @param    None
      @retval   None
*/
void bia_load_lcfg_params(void){

  /* This Function is called after Initcfg so other parameters already initialized */
  if(user_applied_bia_hsrtia_sel) {
    g_current_dcfg_bia[BIA_HIGH_SPEED_RTIA_CONFIG] &= ~(BITM_AFE_HSRTIACON_RTIACON);
    g_current_dcfg_bia[BIA_HIGH_SPEED_RTIA_CONFIG] |= ((AppBIACfg.HstiaRtiaSel << BITP_AFE_HSRTIACON_RTIACON ) & (BITM_AFE_HSRTIACON_RTIACON));
    user_applied_bia_hsrtia_sel = 0;
  } else {
    AppBIACfg.HstiaRtiaSel = (uint32_t) ( g_current_dcfg_bia[BIA_HIGH_SPEED_RTIA_CONFIG] & BITM_AFE_HSRTIACON_RTIACON);
    AppBIACfg.HstiaRtiaSel >>= BITP_AFE_HSRTIACON_RTIACON; 
  }

  if(user_applied_sin_freq) {

    /*  Frequency of data need to be enabled once sweep feature is brought in */
    //AppBIACfg.FreqofData = AppBIACfg.SinFreq;
    g_current_dcfg_bia[BIA_WAVEGEN_FREQUENCY_CONFIG] &= ~(BIA_WG_SINE_FREQUENCY_WORD_MASK);
    g_current_dcfg_bia[BIA_WAVEGEN_FREQUENCY_CONFIG] |= AD5940_WGFreqWordCal(AppBIACfg.SinFreq, AppBIACfg.SysClkFreq);
    user_applied_sin_freq = 0;
  } else {
      /* Recalculate sine frequency from frequency word register and system clock */
      float temp_sinfreqword = (float)( g_current_dcfg_bia[BIA_WAVEGEN_FREQUENCY_CONFIG] & BIA_WG_SINE_FREQUENCY_WORD_MASK);
      temp_sinfreqword /= (1LL<<30); 
      AppBIACfg.SinFreq = (float)( AppBIACfg.SysClkFreq * temp_sinfreqword);
  }

   /* sweep change */
  if(user_applied_sweep_config){
    /* write sine frequency */
    /* check range sweep freq enable / fixed frequency enable, over write sine frequency with sweep frequency */
    g_current_dcfg_bia[BIA_WAVEGEN_FREQUENCY_CONFIG] &= ~(BIA_WG_SINE_FREQUENCY_WORD_MASK);
    g_current_dcfg_bia[BIA_WAVEGEN_FREQUENCY_CONFIG] |= AD5940_WGFreqWordCal(AppBIACfg.SweepCurrFreq, AppBIACfg.SysClkFreq);
    user_applied_sweep_config = 0;
  }
  
  /* sine amplitude update */
  if(user_applied_dac_voltage)  {
    uint32_t SinfreqWord =  (AppBIACfg.DacVoltPP/800.0f*2047 + 0.5f);
    g_current_dcfg_bia[BIA_WAVEFORM_GENERATOR_AMPLITUDE_REGISTER] &= ~(BIA_WG_SINE_AMPLITUDE_WORD_MASK);
    g_current_dcfg_bia[BIA_WAVEFORM_GENERATOR_AMPLITUDE_REGISTER] |= SinfreqWord;
    user_applied_dac_voltage = 0;
  }

  if(user_applied_pga_gain) {
    g_current_dcfg_bia[BIA_ADC_PGA_GAIN] &= ~(BITM_AFE_ADCCON_GNPGA);
    g_current_dcfg_bia[BIA_ADC_PGA_GAIN] |= ((AppBIACfg.ADCPgaGain << BITP_AFE_ADCCON_GNPGA ) & (BITM_AFE_ADCCON_GNPGA));
    user_applied_pga_gain = 0;    
  } else {
    AppBIACfg.ADCPgaGain = (uint32_t) ( g_current_dcfg_bia[BIA_ADC_PGA_GAIN] & BITM_AFE_ADCCON_GNPGA);
    AppBIACfg.ADCPgaGain >>= BITP_AFE_ADCCON_GNPGA;
  }

  if(bia_user_applied_dft_num){
    g_current_dcfg_bia[BIA_DFT_CONFIG] &= ~(BITM_AFE_DFTCON_DFTNUM);
    g_current_dcfg_bia[BIA_DFT_CONFIG] |= ((AppBIACfg.DftNum << BITP_AFE_DFTCON_DFTNUM ) & (BITM_AFE_DFTCON_DFTNUM) );
    bia_user_applied_dft_num = 0;
  }
  else {
    AppBIACfg.DftNum = (uint32_t) ( g_current_dcfg_bia[BIA_DFT_CONFIG] & BITM_AFE_DFTCON_DFTNUM);
    AppBIACfg.DftNum >>= BITP_AFE_DFTCON_DFTNUM; 
  }

  AppBIACfg.FifoThresh = (uint32_t)(g_current_dcfg_bia[BIA_FIFO_THRESHOLD_CONFIG] &  BITM_AFE_DATAFIFOTHRES_HIGHTHRES);
  AppBIACfg.FifoThresh >>= BITP_AFE_DATAFIFOTHRES_HIGHTHRES;
}

/**
      @brief    Load bia default dcfg configuration to working array
      @param    Pointer to configuration array
      @retval   None
*/
void load_bia_default_dcfg_config(uint64_t *cfg) {
    uint16_t i = 0, j = 0,array_ind=0;

    if (cfg == 0) {
        return;
    }
    for (i = 0; i < MAX_DCFG_COMBINED_BIA; i++) {
      while (1) {
        g_current_dcfg_bia[j] = default_dcfg_bia[j]; 
         /* Demarker between DCFG's used to differentiate different register module settings */
        if (cfg[j++] == 0xFFFFFFFFFFFFFFFF) {
          break;
        }
      }
    }
    if((bia_user_applied_dcfg == 0x01))
    {
      for(i=0;i < g_bia_num_ind_settings;i++){
        uint8_t dcfg_position = g_user_bia_indexes[i];
        if(dcfg_position < BIA_MAX_USER_SEQCFG){
          /* find index to set */
           array_ind = bia_mapuserind(&dcfg_position);
           /* clear lower 32 bits which is field */
           g_current_dcfg_bia[array_ind] &= 0xFFFFFFFF00000000;
           /* lower 32 bits to set data bits */
           g_current_dcfg_bia[array_ind] |= g_user_bia_config_dcfg[dcfg_position];
         }
       }
         /* clearing dcfg flag for next use */
        bia_user_applied_dcfg = 0;
    }
    bia_load_lcfg_params();

}

/**
* @brief    Write a bia_init_1_dcfg to ad5940
* @param    p_dcfg - pointer to Dcfg array
* @retval   Status
*/
BIA_DCFG_STATUS_t write_bia_init_1_dcfg(uint64_t *p_dcfg) {
  uint32_t reg_addr;
  uint32_t reg_data;

  if (p_dcfg == NULL) {
    return BIA_DCFG_STATUS_NULL_PTR;
  }
 
  for (int i = 0;p_dcfg[i]!= 0xFFFFFFFFFFFFFFFF; i++) {

    reg_addr = (uint32_t) (p_dcfg[i] >> 32);
    reg_data = (uint32_t)(p_dcfg[i]);

    AD5940_WriteReg(reg_addr, reg_data);
  }
  return BIA_DCFG_STATUS_OK;
}

/**
* @brief    Write Bia Default Init_1 configuration
* @param    void
* @retval   Status
*/
BIA_DCFG_STATUS_t write_bia_init_1()
{
  if (write_bia_init_1_dcfg(&g_current_dcfg_bia[0]) != BIA_DCFG_STATUS_OK) {
      return BIA_DCFG_STATUS_ERR;
  }
  return BIA_DCFG_STATUS_OK;
}

/**
* @brief    Write a bia_init_2_dcfg to ad5940
* @param    p_dcfg - pointer to Dcfg array
* @retval   Status
*/
BIA_DCFG_STATUS_t write_bia_init_2_dcfg(uint64_t *p_dcfg) {
  uint32_t reg_addr;
  uint32_t reg_data;

  if (p_dcfg == NULL) {
    return BIA_DCFG_STATUS_NULL_PTR;
  }
 
  for (int i = 0;p_dcfg[i]!= 0xFFFFFFFFFFFFFFFF; i++) {

    reg_addr = (uint32_t) (p_dcfg[i] >> 32);
    reg_data = (uint32_t)(p_dcfg[i]);

    AD5940_WriteReg(reg_addr, reg_data);
  }
  return BIA_DCFG_STATUS_OK;
}

/**
* @brief    Write Bia Default Init_2 configuration
* @param    void
* @retval   Status
*/
BIA_DCFG_STATUS_t write_bia_init_2()
{
  uint16_t i = 0;
  while (g_current_dcfg_bia[i++] != 0xFFFFFFFFFFFFFFFF);
  if (write_bia_init_2_dcfg(&g_current_dcfg_bia[i]) != BIA_DCFG_STATUS_OK) {
      return BIA_DCFG_STATUS_ERR;
  }
  return BIA_DCFG_STATUS_OK;
}


/**
* @brief    Write a sequencercfg bia_dcfg to ad5940
* @param    p_dcfg - pointer to Dcfg array
* @retval   Status
*/
BIA_DCFG_STATUS_t write_bia_seqcfg_dcfg(uint64_t *p_dcfg) {
  uint32_t reg_addr;
  uint32_t reg_data;

  if (p_dcfg == NULL) {
    return BIA_DCFG_STATUS_NULL_PTR;
  }
 
  for (int i = 0;p_dcfg[i]!= 0xFFFFFFFFFFFFFFFF; i++) {
    reg_addr = (uint32_t) (p_dcfg[i] >> 32);
    reg_data = (uint32_t)(p_dcfg[i]);

    AD5940_WriteReg(reg_addr, reg_data);
  }
  return BIA_DCFG_STATUS_OK;
}

/**
* @brief    Write bia SequencerCfg dcfg  configuration to ad5940
* @param    void
* @retval   Status
*/
BIA_DCFG_STATUS_t write_bia_seqcfg()
{
  uint16_t i = 0;
  while (g_current_dcfg_bia[i++] != 0xFFFFFFFFFFFFFFFF);
  while (g_current_dcfg_bia[i++] != 0xFFFFFFFFFFFFFFFF);
  if (write_bia_seqcfg_dcfg(&g_current_dcfg_bia[i]) != BIA_DCFG_STATUS_OK) {
      return BIA_DCFG_STATUS_ERR;
  }
  return BIA_DCFG_STATUS_OK;
}

/**
* @brief    Write a bia sequencer measurement dcfg to ad5940
* @param    p_dcfg - pointer to Dcfg array
* @retval   Status
*/
BIA_DCFG_STATUS_t write_bia_seqmeasurement_dcfg(uint64_t *p_dcfg) {
  uint32_t reg_addr;
  uint32_t reg_data;

  if (p_dcfg == NULL) {
    return BIA_DCFG_STATUS_NULL_PTR;
  }
 
  for (int i = 0;p_dcfg[i]!= 0xFFFFFFFFFFFFFFFF; i++) {
    if((uint32_t) (p_dcfg[i] >> 32) == 0xEEEEEEEE){

      AD5940_SEQGenInsert((uint32_t)(p_dcfg[i]));

    }
    else {

      reg_addr = (uint32_t) (p_dcfg[i] >> 32);
      reg_data = (uint32_t)(p_dcfg[i]);
      AD5940_WriteReg(reg_addr, reg_data);

    }
  }
  return BIA_DCFG_STATUS_OK;
}


/**
* @brief    Write bia Sequencer measurement dcfg configuration to ad5940
* @param    void
* @retval   Status
*/
BIA_DCFG_STATUS_t write_bia_seqmeasurement()
{
  uint16_t i = 0;
  while (g_current_dcfg_bia[i++] != 0xFFFFFFFFFFFFFFFF);
  while (g_current_dcfg_bia[i++] != 0xFFFFFFFFFFFFFFFF);
  while (g_current_dcfg_bia[i++] != 0xFFFFFFFFFFFFFFFF);

  if (write_bia_seqmeasurement_dcfg(&g_current_dcfg_bia[i]) != BIA_DCFG_STATUS_OK) {
      return BIA_DCFG_STATUS_ERR;
  }
  return BIA_DCFG_STATUS_OK;
}

#endif
/*!
  ****************************************************************************
 * @brief  Bcm App Initialization
 * @param  None
 * @return ECG_ERROR_CODE_t: Success/Error
 *****************************************************************************/
BIA_ERROR_CODE_t BiaAppInit() {
  BIA_ERROR_CODE_t retVal = BIA_ERROR;
  g_disable_ad5940_port_init = false;
  
#ifdef BCM_ALGO  
  prev_ffm_estimated = prev_bmi = prev_fatpercent=0;
#endif

  /* Ldo power on */
  adp5360_enable_ldo(ECG_LDO,true);

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
  gBiaAppInitFlag = false;
#ifdef BIA_DCFG_ENABLE
  /*resetting default load flag*/
  bia_load_dcfg = 1;
#endif//BIA_DCFG_ENABLE
  g_disable_ad5940_port_deinit = false;
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
      /* add sweep features here */
#ifdef RANGE_SWEEP_ENABLE
    case BIA_RANGE_SWEEP_FREQUENCY_ENABLE: /* field 18 */
        if(value == 1)  {
          /* here range sweep or fixed sweep one time will be enabled */
          user_applied_sweep_config = 1;
          AppBIACfg.SweepCfg.SweepEn = bTRUE;
          /* fixed sweep disable */
          AppBIACfg.FixedFreqSweepEn = bFALSE;
        }
       else {
          user_applied_sweep_config = 0;
          AppBIACfg.SweepCfg.SweepEn = bFALSE;
       }
       break;
    case BIA_SWEEP_START_FREQUENCY: /* field 20 */
       AppBIACfg.SweepCfg.SweepStart = value;
       break;
    case BIA_SWEEP_STOP_FREQUENCY: /* field 21 */
       AppBIACfg.SweepCfg.SweepStop = value;
       break;
    case BIA_SWEEP_SWEEP_POINTS: /* field 22 */
       AppBIACfg.SweepCfg.SweepPoints = value;
       break;
    case BIA_SWEEP_SCALE_TYPE: /* field 23 */
       if(value == 1)  {
          /* log step */
          AppBIACfg.SweepCfg.SweepLog = bTRUE;
       }
       else {
          /* linear step */
          AppBIACfg.SweepCfg.SweepLog = bFALSE;
       }
       break;
    case BIA_SWEEP_INDEX: /* field 24 */
       AppBIACfg.SweepCfg.SweepIndex = value;
       break;
#endif
#ifdef FIXED_FREQ_SWEEP_ENABLE
    case BIA_FIXED_FREQUENCY_SWEEP_ENABLE: /* field 19 */
      if(value == 1)  {
        user_applied_sweep_config =1;
       /* enable fixed freq sweep here */
        AppBIACfg.FixedFreqSweepEn = bTRUE;
        /* default range sweep is disabled */
        AppBIACfg.SweepCfg.SweepEn = bFALSE;
      }
      else  {
        user_applied_sweep_config =0;
        /* enable fixed freq sweep here */
        AppBIACfg.FixedFreqSweepEn = bFALSE;
      }
     break;
    case BIA_SWEEP_FIXED_FREQUENCY: /* field 25 */
      AppBIACfg.SweepCurrFreq = value;
     break;
    case BIA_SWEEP_VOLTAGE: /* field 26 */
      user_applied_dac_voltage = 1;
      AppBIACfg.DacVoltPP = value;
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
/* add sweep features here */
#ifdef RANGE_SWEEP_ENABLE
     case BIA_RANGE_SWEEP_FREQUENCY_ENABLE: /* field 18 */
        if(AppBIACfg.SweepCfg.SweepEn == bTRUE) {
          *value = 1;
        }
        else  {
          *value = 0;
        }
       break;
    case BIA_SWEEP_START_FREQUENCY: /* field 20 */
      *value = AppBIACfg.SweepCfg.SweepStart;
       break;
    case BIA_SWEEP_STOP_FREQUENCY: /* field 21 */
       *value = AppBIACfg.SweepCfg.SweepStop;
       break;
    case BIA_SWEEP_SWEEP_POINTS: /* field 22 */
       *value = AppBIACfg.SweepCfg.SweepPoints;
       break;
    case BIA_SWEEP_SCALE_TYPE: /* field 23 */
    if(AppBIACfg.SweepCfg.SweepLog == bTRUE)  {
       *value = 1;/* log type */
    }
    else {
      *value = 0; /* linear type */
    }
      break;
    case BIA_SWEEP_INDEX: /* field 24 */
      *value = AppBIACfg.SweepCfg.SweepIndex;
       break;
#endif
#ifdef FIXED_FREQ_SWEEP_ENABLE
    case BIA_FIXED_FREQUENCY_SWEEP_ENABLE: /* field 19 */
       *value = AppBIACfg.FixedFreqSweepEn;
       break;
    case BIA_SWEEP_FIXED_FREQUENCY: /* field 25 */
       *value = AppBIACfg.SweepCurrFreq;
        break;
    case BIA_SWEEP_VOLTAGE: /* field 26 */
       *value = AppBIACfg.DacVoltPP;
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
          if((i < START_IEEE_FLOAT_INDEX) || (i > END_IEEE_FLOAT_INDEX)){
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
* @brief            Get Frequency in Hz of excitation signal 
*                   during the BIA packetization
*
*
* @return           unsigned integer, frequency parameter
*/
static uint32_t GetBIASweepFrequencyIndex()  {
#if defined(RANGE_SWEEP_ENABLE) || defined(FIXED_FREQ_SWEEP_ENABLE)
    uint32_t nfrequencyindex = AppBIACfg.SweepCurrFreq;
#else
    uint32_t nfrequencyindex = AppBIACfg.SinFreq;
#endif
      return nfrequencyindex;
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


