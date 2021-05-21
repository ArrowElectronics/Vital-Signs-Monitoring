/**
***************************************************************************
* @addtogroup Tasks
* @{
* @file         ppg_application_task.c
* @author       ADI
* @version      V1.0.0
* @date         15-Nov-2020
* @brief        Source file contains ppg processing wrapper.
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
#include <includes.h>
volatile uint8_t gb_adxl_raw_start =1;  /* Flag to handle whether ADXL sensor was 'start'ed to get raw data(CLI_USB/CLI_BLE) or if it was started by internal applications like PPG */
volatile uint8_t gb_adpd_raw_start = 1; /* Flag to handle whether ADPD sensor was 'start'ed to get raw data(CLI_USB/CLI_BLE) or if it was started by internal applications like PPG/Temp */
#ifdef ENABLE_PPG_APP
/* -------------------------------- Includes --------------------------------*/
#include "ppg_application_task.h"
#include "adpd4000_task.h"
#include "adxl_task.h"
#include "sync_data_application_task.h"
#include "sync_data_application_interface.h"
#include <rtc.h>
#include <adxl_buffering.h>
#include "adpd4000_buffering.h"
#include "ppg_application_interface.h"
#include "hw_if_config.h"
#include "adpd400x_lib.h"
#include "app_sync.h"
#include "Adxl362.h"
#include "adpd400x_drv.h"
#include "sensor_internal.h"
#include "app_common.h"
#include "mw_ppg.h"
#include "app_timer.h"

#ifdef DCB
#include <dcb_interface.h>
#endif

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
/* ------------------------- Defines  -------------------------------------- */
#define PPG_SYNC_DATA_STREAM_MAX_SIZE 80
#define PPG_APP_MAX_LCFG_OPS (100)
#define DEBUG_INFO_SIZE       10
#define PPG_MAJOR_VERSION     3
#define PPG_MINOR_VERSION     5
#define PPG_PATCH_VERSION     0
#define SKIP_ADPD_ADXL_SAMPLES     16
/* ------------------------- Type Definition --------------------------------*/
typedef struct _ppgpacketizer_t {
  m2m2_hdr_t                *p_pkt;
} ppgpacketizer_t;

typedef struct _syncpacketizer_t {
  m2m2_hdr_t                *p_pkt;
  uint8_t                    packet_nsamples;
  uint8_t                    skip_samples;
  uint32_t                   nPrevPpgTS;
  uint32_t                   nPrevAdxlTS;
} syncpacketizer_t;

typedef struct _SyncAppBuff {
  uint32_t  gFifoDataB[2];
  uint8_t   gFifoDataBcount;
  uint32_t  gTsAdpd;
  uint32_t  gTsAdxl;
  uint16_t   gAccelData[3];
  uint8_t   gAccelDatacount;
}SyncAppBuff_t;

/* -------------------------Public variables -------------------------------*/
uint32_t  Ppg_Slot = 0;
#ifdef DEBUG_PKT
uint32_t g_ppg_hr_pkt_cnt =0, g_sync_ppg_pkt_cnt = 0;
#endif
extern uint8_t g_mwl_view;
extern uint8_t agc_count;
extern uint32_t gn_led_slot_g;
extern uint8_t gb_static_agc_green_en;
extern uint8_t gb_static_agc_green_done;
extern g_state_t       g_state;
extern g_state_adxl_t  g_state_adxl;
#ifdef ENABLE_TEMPERATURE_APP
extern uint16_t gsTemperatureStarts;
#endif
extern slot_led_reg_t led_reg;
extern uint8_t gb_ppg_static_agc_green_en;
extern uint32_t gsSampleCount1;
extern uint8_t gsOneTimeValueWhenReadAdpdData;
extern volatile uint8_t gn_uc_hr_enable;
#ifdef SLOT_SELECT
extern bool check_ppg_slot_set;
#endif
/* ------------------------- Public Function Prototypes -------------------- */
void packetize_ppg_data(uint32_t *pPpgData, int16_t *pAdxlData, TimeStamps_t timeStamp);
void packetize_ppg_HR_debug_data(LibResultX_t lib_result, ppgpacketizer_t *p_pktizer);
int16_t PpgSetOperationMode(uint8_t eOpState);
void event_from_sync(void);
void RegisterPpgCB(int16_t (*pfppg_set_opmode)(uint8_t));
extern ADPDLIB_ERROR_CODE_t MwPpgGetLibStateInfo(uint8_t, uint16_t*);
/* ------------------------- Private variables ----------------------------- */
/* Create the stack for task */
static uint8_t ppg_application_task_stack[APP_OS_CFG_PPG_APPLICATION_TASK_STK_SIZE];

/* Create handler for task */
static ADI_OSAL_THREAD_HANDLE ppg_task_handler;

/* Create task attributes variable */
static ADI_OSAL_STATIC_THREAD_ATTR ppg_application_task_attributes;

/* Create TCB for task */
static StaticTask_t ppgTaskTcb;

/* Create semaphores */
static ADI_OSAL_SEM_HANDLE   ppg_application_task_evt_sem;

/* Create Queue Handler for task */
static ADI_OSAL_QUEUE_HANDLE  ppg_task_msg_queue = NULL;

const char GIT_PPG_APP_VERSION[] = "TEST PPG_APP_VERSION STRING";
const uint8_t GIT_PPG_APP_VERSION_LEN = sizeof(GIT_PPG_APP_VERSION);
static SyncAppBuff_t g_SyncAppBuff;

ADXL_TS_DATA_TYPE gsyncADXL_dready_ts;
ADPD_TS_DATA_TYPE gsyncADPD_dready_ts;
/*! variable to indicate ADPD data ready and ADXL data ready*/
#if 0
volatile uint8_t gnSyncAdpdDataReady, gnSyncAdxlDataReady;
#endif
uint32_t gIntCnt = 0;
static uint16_t g_reg_base;

static struct _g_state_syncapp {
  uint16_t  num_subs;
  uint16_t  num_starts;
  uint8_t   decimation_factor;
  uint16_t  decimation_nsamples;
  uint16_t  data_pkt_seq_num;
  syncpacketizer_t  syncapp_pktizer;
} g_state_syncapp;

static struct _g_state_ppgapp {
  uint16_t  num_subs;
  uint16_t  num_starts;
  uint16_t  data_pkt_seq_num;
  uint8_t   PpgPaused;
  uint8_t   CurLibState;
  uint8_t   PreLibState;
  uint8_t   PpgHrCounter;
  ppgpacketizer_t  ppgapp_pktizer;
} g_state_ppgapp;

/*! structure for stroing library result*/
LibResultX_t lib_result;

/* ------------------------- Private Function Prototypes ------------------- */
static void ppg_application_task(void *pArgument);
static void reset_ppgapp_packetization(void);
static m2m2_hdr_t *ppg_app_stream_config(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *ppg_app_status(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *ppg_app_get_version(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *ppg_app_lcfg_access(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *ppg_app_get_last_states(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *ppg_app_get_ctr(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *ppg_app_get_signalmetrics(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *ppg_app_get_states_info(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *ppg_app_set_lcfg(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *ppg_app_get_lcfg(m2m2_hdr_t *p_pkt);
#ifdef DCB
static m2m2_hdr_t *ppg_dcb_command_read_config(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *ppg_dcb_command_write_config(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *ppg_dcb_command_delete_config(m2m2_hdr_t *p_pkt);
#endif
static int16_t (*pfPpgAppSetOpMode)(uint8_t OpMode);

ppg_routing_table_entry_t ppg_app_routing_table[] = {
  {M2M2_APP_COMMON_CMD_STREAM_START_REQ, ppg_app_stream_config},
  {M2M2_APP_COMMON_CMD_STREAM_STOP_REQ, ppg_app_stream_config},
  {M2M2_APP_COMMON_CMD_STREAM_PAUSE_REQ, ppg_app_stream_config},
  {M2M2_APP_COMMON_CMD_STREAM_UNPAUSE_REQ, ppg_app_stream_config},
  {M2M2_APP_COMMON_CMD_STREAM_SUBSCRIBE_REQ, ppg_app_stream_config},
  {M2M2_APP_COMMON_CMD_STREAM_UNSUBSCRIBE_REQ, ppg_app_stream_config},
  {M2M2_APP_COMMON_CMD_SENSOR_STATUS_QUERY_REQ, ppg_app_status},
  {M2M2_APP_COMMON_CMD_GET_VERSION_REQ, ppg_app_get_version},
  {M2M2_PPG_APP_CMD_GET_ALGO_VENDOR_VERSION_REQ, ppg_app_get_version},
  {M2M2_APP_COMMON_CMD_WRITE_LCFG_REQ, ppg_app_lcfg_access},
  {M2M2_APP_COMMON_CMD_READ_LCFG_REQ, ppg_app_lcfg_access},
  {M2M2_PPG_APP_CMD_GET_LAST_STATES_REQ, ppg_app_get_last_states},
  {M2M2_PPG_APP_CMD_GET_CTRVALUE_REQ, ppg_app_get_ctr},
  {M2M2_PPG_APP_CMD_GET_SMETRICS_REQ, ppg_app_get_signalmetrics},
  {M2M2_PPG_APP_CMD_GET_STATES_INFO_REQ, ppg_app_get_states_info},
  {M2M2_APP_COMMON_CMD_SET_LCFG_REQ, ppg_app_set_lcfg},
  {M2M2_APP_COMMON_CMD_GET_LCFG_REQ, ppg_app_get_lcfg},
#ifdef DCB
  {M2M2_DCB_COMMAND_READ_CONFIG_REQ, ppg_dcb_command_read_config },
  {M2M2_DCB_COMMAND_WRITE_CONFIG_REQ, ppg_dcb_command_write_config},
  {M2M2_DCB_COMMAND_ERASE_CONFIG_REQ, ppg_dcb_command_delete_config},
#endif
};
#define PPG_APP_ROUTING_TBL_SZ (sizeof(ppg_app_routing_table) / sizeof(ppg_app_routing_table[0]))

/**
* @brief  Posts the message packet received into the ppg application task queue
*
* @param[in]  p_pkt: pointer to the message to be posted
*
* @return None
*/
void send_message_ppg_application_task(m2m2_hdr_t *p_pkt) {
  ADI_OSAL_STATUS osal_result;
  osal_result = adi_osal_MsgQueuePost(ppg_task_msg_queue,p_pkt);
  if (osal_result != ADI_OSAL_SUCCESS)
    post_office_consume_msg(p_pkt);

  adi_osal_SemPost(ppg_application_task_evt_sem);
}

/**@brief   resets sync app packetization process
*
* @param[in]  None
*
* @return     None
*/
void reset_syncapp_packetization(){
 g_state_syncapp.syncapp_pktizer.packet_nsamples = 0;
}

APP_TIMER_DEF(m_ppg_timer_id);     /**< Handler for repeated timer for ppg. */
#define PPG_INTERVAL               1000 /* 1 sec. Interval */
static bool hr_mode_on = true;
static void ppg_timeout_handler(void * p_context);
static uint32_t dutyCycle = 0;
static uint32_t on_time = 0;
static uint32_t off_time = 0;
static uint32_t on_time_count = 0;
static uint32_t off_time_count = 0;
static bool check_timer_started = false;

/**@brief Function for the Timer initialization.
*
* @details Initializes the timer module. This creates and starts application timers.
*
* @param[in]  None
*
* @return     None
*/
static void ppg_timer_init(void)
{
    ret_code_t err_code;

    /*! Create timers */
    err_code =  app_timer_create(&m_ppg_timer_id, APP_TIMER_MODE_REPEATED, ppg_timeout_handler);

    APP_ERROR_CHECK(err_code);
}

/**@brief   Function for starting application timers.
* @details Timers are run after the scheduler has started.
*
* @param[in]  None
*
* @return     None
*/
static void ppg_timer_start(void)
{
    /*! Start repeated timer */
    ret_code_t err_code = app_timer_start(m_ppg_timer_id, APP_TIMER_TICKS(PPG_INTERVAL), NULL);
    APP_ERROR_CHECK(err_code);

    check_timer_started = true;
}

/**@brief   Function for stopping the application timers.
*
* @param[in]  None
*
* @return     None
*/
static void ppg_timer_stop(void)
{
    /*! Stop the repeated timer */
    ret_code_t err_code = app_timer_stop(m_ppg_timer_id);
    APP_ERROR_CHECK(err_code);

    check_timer_started = false;
}

/**@brief   Callback Function for application timer events
*
* @param[in]  p_context: pointer to the callback function arguments
*
* @return     None
*/
static void ppg_timeout_handler(void * p_context)
{
    if(hr_mode_on)
    {
        on_time_count++; /*! Increment counter every sec. incase of ADPD ON, till it is equal to Ton Value in seconds. */
        if(on_time_count == on_time)
          {
           /*AdxlDrvExtSampleMode(PRM_EXT_SAMPLE_DISABLE);*/
            (*pfPpgAppSetOpMode)(ADPDDrv_MODE_IDLE);
         /* SyncClearDataBuffer(); */
            hr_mode_on = false;
            on_time_count = 0;
          }
    }
    else
    {
      off_time_count++; /*! Increment counter every sec. incase of ADPD OFF, till it is equal to Toff Value in seconds.*/
      if(off_time_count == off_time)
        {
          SyncClearDataBuffer();
          /*AdxlDrvExtSampleMode(PRM_EXT_SAMPLE_ENABLE);*/
          (*pfPpgAppSetOpMode)(ADPDDrv_MODE_SAMPLE);
           hr_mode_on = true;
           off_time_count = 0;
        }
    }
}

/**
* @brief  Initializes the PPG application task
*
* @param[in]  None
*
* @return     None
*
*/
void ppg_application_task_init(void) {
  /*! Initialize app state */
  g_state_ppgapp.num_subs = 0;
  g_state_ppgapp.num_starts = 0;
  g_state_ppgapp.data_pkt_seq_num = 0;
  reset_ppgapp_packetization();
  /*! Initialize app state */
  g_state_syncapp.num_subs = 0;
  g_state_syncapp.num_starts = 0;
  /*! Default behaviour is to send every packet */
  g_state_syncapp.decimation_factor = 1;
  g_state_syncapp.decimation_nsamples = 0;
  g_state_syncapp.data_pkt_seq_num = 0;
  reset_syncapp_packetization();

  ADI_OSAL_STATUS eOsStatus = ADI_OSAL_SUCCESS;
  ppg_application_task_attributes.pThreadFunc = ppg_application_task;
  ppg_application_task_attributes.nPriority = APP_OS_CFG_PPG_APPLICATION_TASK_PRIO;
  ppg_application_task_attributes.pStackBase = &ppg_application_task_stack[0];
  ppg_application_task_attributes.nStackSize = APP_OS_CFG_PPG_APPLICATION_TASK_STK_SIZE;
  ppg_application_task_attributes.pTaskAttrParam = NULL;
  ppg_application_task_attributes.szThreadName = "PPG Data";
  ppg_application_task_attributes.pThreadTcb = &ppgTaskTcb;
  eOsStatus = adi_osal_MsgQueueCreate(&ppg_task_msg_queue,NULL,
                                    5);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
      Debug_Handler();
  }
  else
  {
    update_task_queue_list(APP_OS_CFG_PPG_TASK_INDEX,ppg_task_msg_queue);
  }
  eOsStatus = adi_osal_ThreadCreateStatic(&ppg_task_handler,
                                    &ppg_application_task_attributes);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
      Debug_Handler();
  }

  adi_osal_SemCreate(&ppg_application_task_evt_sem, 0);
}

/*!
  *@brief       Constructs a packet to send the PPG sync data
  *@param[in]   pPpgData: pointer to ppg data
  *@param[in]   pAdxlData: pointer to adxl data
  *@param[in]   ppgTS: PPG timestamp
  *@param[in]   adxlTS: ADXL timestamp
  *@return
 */
static void packetize_ppg_sync_data(uint32_t *pPpgData, uint16_t *pAdxlData, uint32_t ppgTS,uint32_t adxlTS) {

  ADI_OSAL_STATUS         err;
  g_state_syncapp.syncapp_pktizer.p_pkt = NULL;
  static adpd_adxl_sync_data_stream_t syncpkt;

  if( g_state_syncapp.syncapp_pktizer.skip_samples  >= SKIP_ADPD_ADXL_SAMPLES){
    /*! first check if there is subscribtion to sync data */
    if (g_state_syncapp.num_subs > 0) {
      /*! the first sample includes the complete TS */
      if( g_state_syncapp.syncapp_pktizer.packet_nsamples == 0){
          syncpkt.syncData.ppgTS = ppgTS;
          syncpkt.syncData.adxlTS = adxlTS;
          syncpkt.syncData.ppgData[0] = *pPpgData;
          syncpkt.syncData.xData[0] = pAdxlData[0];  /*! X data */
          syncpkt.syncData.yData[0] = pAdxlData[1];  /*! Y data */
          syncpkt.syncData.zData[0] = pAdxlData[2];  /*! Z data */
          g_state_syncapp.decimation_nsamples = 1;
      }
      /*! Not the first sample of the packet */
      else{
        /*! check dec factor */
        if (!(g_state_syncapp.syncapp_pktizer.packet_nsamples% g_state_syncapp.decimation_factor)) {

          /*! Prevent rollover on ppg TS */
          if (ppgTS <  g_state_syncapp.syncapp_pktizer.nPrevPpgTS) {
          /* MAX_RTC_TICKS_FOR_24_HOUR:- Max RTC Count value returned by get_sensor_timestamp() after 24hrs.
             Adding that value to have correct incPpgTS value during day roll-over */
            syncpkt.syncData.incPpgTS[ g_state_syncapp.decimation_nsamples-1] = MAX_RTC_TICKS_FOR_24_HOUR - \
                                                                                    g_state_syncapp.syncapp_pktizer.nPrevPpgTS + ppgTS;
          } else {
            syncpkt.syncData.incPpgTS[ g_state_syncapp.decimation_nsamples-1] = ppgTS -  g_state_syncapp.syncapp_pktizer.nPrevPpgTS;
          }

          /*! Prevent rollover on adxl TS */
          if (adxlTS < g_state_syncapp.syncapp_pktizer.nPrevAdxlTS) {
          /* MAX_RTC_TICKS_FOR_24_HOUR:- Max RTC Count value returned by get_sensor_timestamp() after 24hrs.
             Adding that value to have correct incAdxlTS value during day roll-over */
            syncpkt.syncData.incAdxlTS[ g_state_syncapp.decimation_nsamples-1] = MAX_RTC_TICKS_FOR_24_HOUR - \
                                                                                    g_state_syncapp.syncapp_pktizer.nPrevAdxlTS + adxlTS;
          } else {
            syncpkt.syncData.incAdxlTS[ g_state_syncapp.decimation_nsamples-1] = adxlTS - g_state_syncapp.syncapp_pktizer.nPrevAdxlTS;
          }

          /*! ppg and adxl data */
          if(!hr_mode_on)
          {
            syncpkt.syncData.ppgData[ g_state_syncapp.decimation_nsamples] = 0;
          }
          else
          {
            syncpkt.syncData.ppgData[ g_state_syncapp.decimation_nsamples] = *pPpgData;
          }
          syncpkt.syncData.xData[ g_state_syncapp.decimation_nsamples] = pAdxlData[0];
          syncpkt.syncData.yData[ g_state_syncapp.decimation_nsamples] = pAdxlData[1];
          syncpkt.syncData.zData[ g_state_syncapp.decimation_nsamples++] = pAdxlData[2];
        }
      }

      /*! check the number of samples to send the packet */
      if(++g_state_syncapp.syncapp_pktizer.packet_nsamples >= (4 *  g_state_syncapp.decimation_factor)) {
        g_state_syncapp.syncapp_pktizer.packet_nsamples = 0;
        adi_osal_EnterCriticalRegion();
        g_state_syncapp.syncapp_pktizer.p_pkt = post_office_create_msg(sizeof(adpd_adxl_sync_data_stream_t) + M2M2_HEADER_SZ);
        if(g_state_syncapp.syncapp_pktizer.p_pkt != NULL)
        {
#ifdef DEBUG_PKT
        g_sync_ppg_pkt_cnt++;
#endif
        PYLD_CST(g_state_syncapp.syncapp_pktizer.p_pkt, adpd_adxl_sync_data_stream_t, p_payload_ptr);
        g_state_syncapp.syncapp_pktizer.p_pkt->src = M2M2_ADDR_MED_PPG;
        g_state_syncapp.syncapp_pktizer.p_pkt->dest = M2M2_ADDR_MED_SYNC_ADPD_ADXL_STREAM;
        memcpy(&p_payload_ptr->command, &syncpkt, sizeof(adpd_adxl_sync_data_stream_t));
        p_payload_ptr->command = M2M2_SENSOR_COMMON_CMD_STREAM_DATA;
        p_payload_ptr->status = 0x00;
        p_payload_ptr->sequence_num = g_state_syncapp.data_pkt_seq_num++;
#ifdef DEBUG_PKT
        post_office_msg_cnt(g_state_syncapp.syncapp_pktizer.p_pkt);
#endif
        post_office_send(g_state_syncapp.syncapp_pktizer.p_pkt, &err);

        g_state_syncapp.syncapp_pktizer.p_pkt = NULL;
        }
        adi_osal_ExitCriticalRegion(); /* exiting critical region even if mem_alloc fails*/
      }
      /*! update the previous TS */
      g_state_syncapp.syncapp_pktizer.nPrevPpgTS = ppgTS;
      g_state_syncapp.syncapp_pktizer.nPrevAdxlTS = adxlTS;
    }
  } else {
    g_state_syncapp.syncapp_pktizer.skip_samples ++;
  }
}

/**
* @brief  Task which handles processing of the PPG data and commands received
*
* @param[in]  pArgument: input argument to the task
*
* @return None
*/
static void ppg_application_task(void *pArgument) {
  m2m2_hdr_t *p_in_pkt = NULL;
  m2m2_hdr_t *p_out_pkt = NULL;
  ADI_OSAL_STATUS         err;
  TimeStamps_t timeStamp;

  post_office_add_mailbox(M2M2_ADDR_MED_PPG, M2M2_ADDR_MED_PPG_STREAM);
  post_office_add_mailbox(M2M2_ADDR_MED_PPG, M2M2_ADDR_MED_SYNC_ADPD_ADXL_STREAM);
  ppg_timer_init();
  PpgInit();
  while (1) {
    adi_osal_SemPend(ppg_application_task_evt_sem, ADI_OSAL_TIMEOUT_FOREVER);
    p_in_pkt = post_office_get(ADI_OSAL_TIMEOUT_NONE, APP_OS_CFG_PPG_TASK_INDEX);
    if (p_in_pkt == NULL) {
      /*syncppg task*/
      timeStamp.tsADPD = GetSyncAdpdTs();
      timeStamp.tsADXL = GetSyncAccelTs();
      timeStamp.tsAlgorithmCall = AdpdLibGetTick();
      if(hr_mode_on)
      {
      packetize_ppg_data(&(GetSyncAdpdData())[0], GetSyncAccelData(), timeStamp);

#define STORE_SAMPLES_USED_FOR_HR_LIB 0
#ifdef STORE_SAMPLES_USED_FOR_HR_LIB
      static volatile uint32_t storeAdpd[200], SampAdpdCnt=0;
      static volatile uint32_t storeAdxl[100], SampAdxlCnt=0;
      uint32_t gnHRAdpdData = (GetSyncAdpdData())[0];
      uint32_t gnHRAdxlData = (GetSyncAccelData())[0];
      if(SampAdpdCnt<200)
      {
        storeAdpd[SampAdpdCnt++] = gnHRAdpdData;
        //storeAdpd[SampAdpdCnt++] = timeStamp.tsADPD;
      }
      if(SampAdxlCnt<100)
      {
        storeAdxl[SampAdxlCnt++] = gnHRAdxlData;
        //storeAdxl[SampAdxlCnt++] = timeStamp.tsADXL;
      }
#endif

      if(!gn_uc_hr_enable)
        packetize_ppg_sync_data(&(GetSyncAdpdData())[0], GetSyncRawAccelData(), timeStamp.tsADPD,timeStamp.tsADXL);
      }
      else
      {
       if(g_SyncAppBuff.gAccelDatacount > 0)
       {
        g_SyncAppBuff.gFifoDataB[0] = 0;
        packetize_ppg_sync_data(&(g_SyncAppBuff.gFifoDataB[0]), g_SyncAppBuff.gAccelData, 0,timeStamp.tsADXL);
       }
      }
    } else {
      /*! We got an m2m2 message from the queue, process it.*/
      PYLD_CST(p_in_pkt, _m2m2_app_common_cmd_t, p_in_cmd);
      /*! Look up the appropriate function to call in the function table */
      for (int i = 0; i < PPG_APP_ROUTING_TBL_SZ; i++) {
        if (ppg_app_routing_table[i].command == p_in_cmd->command) {
          p_out_pkt = ppg_app_routing_table[i].cb_handler(p_in_pkt);
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

/**
* @brief  Initializes the callback function for setting the PPG operating mode
*
* @param[in]  None
*
* @return if SUCCESS :  0
*         if FAILURE : -1
*/
int32_t PpgInit() {
  /*! TODO: Do a default configuration. Should this come from the caller */
  PpgAdpd400xSetMode_RegCB(PpgSetOperationMode);
  return 0;
}

/**
* @brief  Registers the callback function for setting the PPG operating mode
*
* @param[in]  (pfppg_set_opmode)(): function pointer to be registered
*
* @return None
*/
void RegisterPpgCB(int16_t (*pfppg_set_opmode)(uint8_t)) {
  pfPpgAppSetOpMode = pfppg_set_opmode;
}


/**
* @brief  sets the operation mode of PPG sensor
*
* @param[in]  (pfppg_set_opmode)(): function pointer to be registered
*
* @return if SUCCESS :  0
*         if FAILURE : -1
*/
int16_t PpgSetOperationMode(uint8_t eOpState) {
  int16_t errCode = 0;
  /*! when change Adpd Operation Mode during HR state, do following */
  /*! ResetAdpdTimeGap(); */
  /*! ResetAdxlTimeGap(); */
  /*! SyncClearDataBuffer(); */
  /*! AdxlSetOperationMode(PRM_MEASURE_MEASURE_MODE); */

  /*! errCode = AdxlDrvSetOperationMode(PRM_MEASURE_MEASURE_MODE);*/
  errCode |= (*pfPpgAppSetOpMode)(eOpState);
  if (eOpState == ADPDDrv_MODE_SAMPLE)  {
    SyncClearDataBuffer();
  }

  return errCode;
}

/**
* @brief packetizes the ppg data received
*
* @param[in]  pPpgData: pointer to be ppg data
* @param[in]  pAdxlData: pointer to be adxl data
* @param[in]  timeStamp: time stamp of the sensors data
*
* @return None
*/
void packetize_ppg_data(uint32_t *pPpgData, int16_t *pAdxlData, TimeStamps_t timeStamp)  {
  ADPDLIB_ERROR_CODE_t retVal;
  if ((pPpgData == NULL) || (pAdxlData == NULL)) {
    return;
  }

  memset(&lib_result, 0x00, sizeof(lib_result));
  if ( g_state_ppgapp.num_subs > 0) {
    retVal = MwPPG_HeartRate(&lib_result, pPpgData, pAdxlData, timeStamp);
    g_state_ppgapp.CurLibState = MwPpgGetLibState();/*! AdpdLibGetState(); */

    /*! Todo: handle detection return here */
    if (retVal == ADPDLIB_ERR_ON_SENSOR)
      /*! detect On fail */
      retVal = ADPDLIB_ERR_ON_SENSOR;

    if (g_state_ppgapp.num_subs > 0) {
      /*! Only send out data if we aren't paused. */
      if (g_state_ppgapp.PpgPaused != 1) {

         if (g_state_ppgapp.CurLibState != g_state_ppgapp.PreLibState)  {

           packetize_ppg_HR_debug_data(lib_result,&g_state_ppgapp.ppgapp_pktizer);

        } else if (g_state_ppgapp.CurLibState == ADPDLIB_STAGE_HEART_RATE)  {
            if(g_state_ppgapp.PpgHrCounter++ >= 50)
            {
              g_state_ppgapp.PpgHrCounter = 0;
              packetize_ppg_HR_debug_data(lib_result,&g_state_ppgapp.ppgapp_pktizer);
            }
        }
      }
      g_state_ppgapp.PreLibState = g_state_ppgapp.CurLibState;
    }
  }
}

/**
* @brief Constructs the packet to send the PPG HR debug data
*
* @param[in]  lib_result: library result structure
* @param[in]  p_pktizer: pointer to the data packet
*
* @return None
*/

void packetize_ppg_HR_debug_data(LibResultX_t lib_result, ppgpacketizer_t *p_pktizer) {
  ADI_OSAL_STATUS         err;
  uint16_t debugInfo[DEBUG_INFO_SIZE];
  adi_osal_EnterCriticalRegion();
  p_pktizer->p_pkt = post_office_create_msg(sizeof(ppg_app_hr_debug_stream_t) + M2M2_HEADER_SZ);
  if(p_pktizer->p_pkt != NULL){
#ifdef DEBUG_PKT
    g_ppg_hr_pkt_cnt++;
#endif
    PYLD_CST(p_pktizer->p_pkt, ppg_app_hr_debug_stream_t, p_payload_ptr);
    p_payload_ptr->command = M2M2_SENSOR_COMMON_CMD_STREAM_DATA;
    p_payload_ptr->status = (M2M2_SENSOR_INTERNAL_STATUS_ENUM_t)M2M2_SENSOR_INTERNAL_STATUS_PKT_READY;
    p_payload_ptr->timestamp = get_sensor_time_stamp();
    p_payload_ptr->hr = lib_result.HR;
    p_payload_ptr->confidence = lib_result.confidence;
    p_payload_ptr->hr_type = lib_result.HR_Type;
    p_payload_ptr->rr_interval = lib_result.RRinterval;
    memset(debugInfo, 0, DEBUG_INFO_SIZE*2);
    MwPpgGetLibStateInfo(g_state_ppgapp.CurLibState, debugInfo);
    p_payload_ptr->adpdlibstate = (uint16_t) g_state_ppgapp.CurLibState;
    for (uint8_t i = 0; i < DEBUG_INFO_SIZE; i++){
      p_payload_ptr->debugInfo[i] = debugInfo[i];
    }
    g_state_ppgapp.ppgapp_pktizer.p_pkt->src = M2M2_ADDR_MED_PPG;
    g_state_ppgapp.ppgapp_pktizer.p_pkt->dest = M2M2_ADDR_MED_PPG_STREAM;
    p_payload_ptr->sequence_num = g_state_ppgapp.data_pkt_seq_num++;
    post_office_send(g_state_ppgapp.ppgapp_pktizer.p_pkt, &err);
    g_state_ppgapp.ppgapp_pktizer.p_pkt = NULL;
  }
  adi_osal_ExitCriticalRegion(); /* exiting critical region even if mem_alloc fails*/

}

/**
* @brief raises sync event to ppg application task
*
* @param[in]  None
*
* @return None
*/
void event_from_sync(void) {
  adi_osal_SemPost(ppg_application_task_evt_sem);
}

/**
* @brief  resets the ppg packetization variables
*
* @param[in]  None
*
* @return None
*/
static void reset_ppgapp_packetization(){

  g_state_ppgapp.PpgPaused = 0;
  g_state_ppgapp.PpgHrCounter = 0;
}

/**
* @brief  Handles getVersion command of ppg application
*
* @param[in]  p_pkt: Pointer to the input m2m2 packet structure with command
*                    for getVersion request
*
* @return     p_resp_pkt: Pointer to the output response m2m2 packet structure
*/
static m2m2_hdr_t *ppg_app_get_version(m2m2_hdr_t *p_pkt) {
  PpgAlgoVersion_t nAlgoInfo;
    /*! Declare a pointer to access the input packet payload */
  PYLD_CST(p_pkt, m2m2_app_common_sub_op_t, p_in_payload);

  PKT_MALLOC(p_resp_pkt, m2m2_app_common_version_t, 0);
  if(NULL != p_resp_pkt)
  {
    /*! Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_app_common_version_t, p_resp_payload);
    p_resp_payload->status = M2M2_APP_COMMON_STATUS_ERROR;
    switch(p_in_payload->command){
    case M2M2_APP_COMMON_CMD_GET_VERSION_REQ:
      p_resp_payload->command = M2M2_APP_COMMON_CMD_GET_VERSION_RESP;
      p_resp_payload->major = 0x03;
      p_resp_payload->minor = 0x05;
      p_resp_payload->patch = 0x00;
      memcpy(&p_resp_payload->verstr[0], "PPG_App",  8);
      memcpy(&p_resp_payload->str[0], &GIT_PPG_APP_VERSION, GIT_PPG_APP_VERSION_LEN);
      p_resp_payload->status = M2M2_APP_COMMON_STATUS_OK;
      break;
    case M2M2_PPG_APP_CMD_GET_ALGO_VENDOR_VERSION_REQ:
      PpgLibGetAlgorithmVendorAndVersion(&nAlgoInfo);
      p_resp_payload->command = M2M2_PPG_APP_CMD_GET_ALGO_VENDOR_VERSION_RESP;

      p_resp_payload->major = nAlgoInfo.nMajor;
      p_resp_payload->minor = nAlgoInfo.nMinor;
      p_resp_payload->patch = nAlgoInfo.nPatch;
      memcpy(&p_resp_payload->verstr[0], "-HC",  4);
      memcpy(p_resp_payload->str, nAlgoInfo.aNameStr, sizeof(nAlgoInfo.aNameStr));
      p_resp_payload->status = M2M2_APP_COMMON_STATUS_OK;
      break;
    }
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
    p_resp_pkt->checksum = 0x0000;
  }
  return p_resp_pkt;
}

/**
* @brief  returns the status of ppg application
*
* @param[in]  p_pkt: Pointer to the input m2m2 packet structure with command
*                    for stream status request
*
* @return     p_resp_pkt: Pointer to the output response m2m2 packet structure
*/
static m2m2_hdr_t *ppg_app_status(m2m2_hdr_t *p_pkt) {
  /*! Declare and malloc a response packet */
  PKT_MALLOC(p_resp_pkt, m2m2_app_common_status_t, 0);
  if(NULL != p_resp_pkt)
  {
    /*! Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_app_common_status_t, p_resp_payload);

    p_resp_payload->command = M2M2_APP_COMMON_CMD_SENSOR_STATUS_QUERY_RESP;
    p_resp_payload->status = M2M2_APP_COMMON_STATUS_ERROR;

    if (g_state_ppgapp.num_starts == 0) {
      p_resp_payload->status = M2M2_APP_COMMON_STATUS_STREAM_STOPPED;
    }
    else
    {
      p_resp_payload->status = M2M2_APP_COMMON_STATUS_STREAM_STARTED;
    }
    p_resp_payload->stream = M2M2_ADDR_MED_PPG_STREAM;
    p_resp_payload->num_subscribers = g_state_ppgapp.num_subs;
    p_resp_payload->num_start_reqs = g_state_ppgapp.num_starts;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  }
  return p_resp_pkt;
}

/**
* @brief  Handles start/stop/sub/unsub commands of ppg application
*
* @param[in]  p_pkt: Pointer to the input m2m2 packet structure with command
*                    for stream start/stop/sub/unsub operations
*
* @return     p_resp_pkt: Pointer to the output response m2m2 packet structure
*/
static m2m2_hdr_t *ppg_app_stream_config(m2m2_hdr_t *p_pkt) {
  M2M2_APP_COMMON_STATUS_ENUM_t status = M2M2_APP_COMMON_STATUS_ERROR;
  M2M2_APP_COMMON_CMD_ENUM_t command = 0;
  uint32_t temp_val;
  uint8_t lcfgIndex;
  /*! Declare a pointer to access the input packet payload */
  PYLD_CST(p_pkt, m2m2_app_common_sub_op_t, p_in_payload);
  /*! Declare and malloc a response packet */
  PKT_MALLOC(p_resp_pkt, m2m2_app_common_sub_op_t, 0);
  if(NULL != p_resp_pkt)
  {
  /*! Declare a pointer to the response packet payload */
  PYLD_CST(p_resp_pkt, m2m2_app_common_sub_op_t, p_resp_payload);

  switch (p_in_payload->command) {
  case M2M2_APP_COMMON_CMD_STREAM_START_REQ:
    if (g_state_ppgapp.num_starts == 0)
    {
#ifndef SLOT_SELECT
#ifdef ENABLE_TEMPERATURE_APP
      if((gsTemperatureStarts > 0) && (gsTemperatureStarts == g_state.num_starts))
      {
          for(uint8_t i=0 ; i<SLOT_NUM ; i++)
           {
              g_reg_base = i * 0x20;
              Adpd400xDrvRegWrite(ADPD400x_REG_LED_POW12_A + g_reg_base, led_reg.reg_val[i].reg_pow12); /*!enable led */
              Adpd400xDrvRegWrite(ADPD400x_REG_LED_POW34_A + g_reg_base, led_reg.reg_val[i].reg_pow34);
           }
      }
#endif //ENABLE_TEMPERATURE_APP
#endif
      lcfgIndex = 1; //To write targetSlots
#ifdef SLOT_SELECT
      gsSampleCount1 = 0;
      if(check_ppg_slot_set)
      {
      MwPPG_WriteLCFG(lcfgIndex, Ppg_Slot);
      gn_led_slot_g = Ppg_Slot;
      check_ppg_slot_set = false;
      }
      else
      {
      MwPPG_WriteLCFG(lcfgIndex, 0x20); //Set slot-F, if default dcfg is loaded
      }
#else
      MwPPG_ReadLCFG(lcfgIndex, &Ppg_Slot);
      gn_led_slot_g = Ppg_Slot;
#endif
      if(!g_mwl_view)
      {
        lcfgIndex = 4; /*! To read featureSelect */
        MwPPG_ReadLCFG(lcfgIndex, &temp_val);
        gb_static_agc_green_en = ((uint16_t)temp_val & (uint16_t)(1<<9)) >> 9;
        gb_static_agc_green_done = 0; /*! Flag to handle ADPD led and TIA reg setting for both static AGC enable/disable */
        /*! agc_count = 1; */
      }
      else
      {
        /*! do Nothing, since AGC settings would be done from MWL view */
      }
      gsOneTimeValueWhenReadAdpdData = 0;
      if (MwPPG_HeartRateInit() == ADPDLIB_ERR_SUCCESS)
      {
        (*pfPpgAppSetOpMode)(ADPDDrv_MODE_IDLE);
        /*! AdpdDrvSetOperationMode(ADPDDrv_MODE_IDLE); */
        /* Enable ADPD first then ADXL */
        /*! AdpdDrvSetOperationMode(ADPDDrv_MODE_SAMPLE); */
        SyncInit();

        (*pfPpgAppSetOpMode)(ADPDDrv_MODE_SAMPLE);

        /*!  Start adxl stream */
        AdxlDrvSetOperationMode(PRM_MEASURE_MEASURE_MODE);

        hr_mode_on = true;
        MwPPG_ReadLCFG(6, &dutyCycle);
        off_time = dutyCycle & 0x0000FFFF;
        on_time = (dutyCycle & 0xFFFF0000) >> 16;

        if(on_time > 0 && off_time > 0) /*! If Ton>0 & Toff>0 ->Periodic PPG otherwise ->Continous PPG */
        {
          ppg_timer_start();
        }

        /*!  Only update count if started successfully */
        g_state_ppgapp.CurLibState = 0;
        g_state_ppgapp.PreLibState = 0;
        g_state_ppgapp.PpgHrCounter = 0;
        g_state_ppgapp.num_starts = 1;
        g_state_syncapp.num_starts = 1;
        g_state.num_starts++;
        gb_adpd_raw_start = 0;
        g_state_adxl.num_starts++;
        gb_adxl_raw_start = 0;
        status  = M2M2_APP_COMMON_STATUS_STREAM_STARTED;
      }
      else
      {
        status  = M2M2_APP_COMMON_STATUS_ERROR;
      }
    }
    else
    {
      g_state_ppgapp.num_starts++;
      g_state_syncapp.num_starts++;
      g_state.num_starts++;
      gb_adpd_raw_start = 0;
      g_state_adxl.num_starts++;
      gb_adxl_raw_start = 0;
      status = M2M2_APP_COMMON_STATUS_STREAM_IN_PROGRESS;
    }
    command = M2M2_APP_COMMON_CMD_STREAM_START_RESP;
  break;

  case M2M2_APP_COMMON_CMD_STREAM_STOP_REQ:
    switch (g_state_ppgapp.num_starts)
    {
      case 0:
        status = M2M2_APP_COMMON_STATUS_STREAM_STOPPED;
        Ppg_Slot = 0;
      break;

      case 1:
        if (!MwPPG_HeartRateDeInit())
        {
          if (1 == g_state.num_starts) // Stop adpd stream
          {
            gsOneTimeValueWhenReadAdpdData = 0;
            (*pfPpgAppSetOpMode)(ADPDDrv_MODE_IDLE);
          }

          if (1 == g_state_adxl.num_starts) /*!  Stop adxl stream */
          {
            AdxlDrvSetOperationMode(PRM_MEASURE_STANDBY_MODE);
          }
          if(check_timer_started)
          {
          ppg_timer_stop();
          on_time_count = 0;
          off_time_count = 0;
          }
          /***********AGC Flags**********/
          gb_static_agc_green_en = 1;
          gb_static_agc_green_done = 0;
          gn_led_slot_g = 0;
          /*****************************/
          Ppg_Slot = 0;
          g_state_ppgapp.num_starts = 0;
          g_state_syncapp.num_starts = 0;
          (0 < g_state.num_starts) ? g_state.num_starts-- : (g_state.num_starts = 0);
          gb_adpd_raw_start = 1;
          (0 < g_state_adxl.num_starts) ? g_state_adxl.num_starts-- : (g_state_adxl.num_starts = 0);
          gb_adxl_raw_start = 1;
#ifndef SLOT_SELECT
#ifdef ENABLE_TEMPERATURE_APP
   if((g_state.num_starts > 0) && (g_state.num_starts == gsTemperatureStarts))
   {
      for(uint8_t i=0 ; i<SLOT_NUM ; i++)
      {
          g_reg_base = i * 0x20;
          if(Adpd400xDrvRegRead(ADPD400x_REG_LED_POW12_A + g_reg_base, &led_reg.reg_val[i].reg_pow12) == ADPD400xDrv_SUCCESS)
            {
              Adpd400xDrvRegWrite(ADPD400x_REG_LED_POW12_A + g_reg_base, 0x0);
            }/*! disable led for slot-A - I */

          if(Adpd400xDrvRegRead(ADPD400x_REG_LED_POW34_A + g_reg_base, &led_reg.reg_val[i].reg_pow34) == ADPD400xDrv_SUCCESS)
            {
              Adpd400xDrvRegWrite(ADPD400x_REG_LED_POW34_A + g_reg_base, 0x0);
            }/*! disable led for slot-> A - I */
       }
   }
#endif //#ENABLE_TEMPERATURE_APP
#else
          gsSampleCount1 = 0;
#endif
          status = M2M2_APP_COMMON_STATUS_STREAM_STOPPED;
        }
        else
        {
          status = M2M2_APP_COMMON_STATUS_ERROR;
        }

        reset_ppgapp_packetization();

        if(g_state_ppgapp.ppgapp_pktizer.p_pkt != NULL)
        {
          post_office_consume_msg(g_state_ppgapp.ppgapp_pktizer.p_pkt);
          g_state_ppgapp.ppgapp_pktizer.p_pkt = NULL;
        }
      break;

      default:
        g_state_ppgapp.num_starts--;
        g_state_syncapp.num_starts--;
        (0 < g_state.num_starts) ? g_state.num_starts-- : (g_state.num_starts = 0);
        (0 < g_state_adxl.num_starts) ? g_state_adxl.num_starts-- : (g_state_adxl.num_starts = 0);
        status = M2M2_APP_COMMON_STATUS_STREAM_COUNT_DECREMENT;
      break;
    }
    command = M2M2_APP_COMMON_CMD_STREAM_STOP_RESP;
    break;
  case M2M2_APP_COMMON_CMD_STREAM_PAUSE_REQ:
    g_state_ppgapp.PpgPaused = 1;
     break;

  case M2M2_APP_COMMON_CMD_STREAM_UNPAUSE_REQ:
    g_state_ppgapp.PpgPaused = 0;
      break;

  case M2M2_APP_COMMON_CMD_STREAM_SUBSCRIBE_REQ:
    g_state_ppgapp.num_subs++;
    g_state_syncapp.num_subs++;
    post_office_setup_subscriber(M2M2_ADDR_MED_PPG, M2M2_ADDR_MED_PPG_STREAM, p_pkt->src, true);
    post_office_setup_subscriber(M2M2_ADDR_MED_PPG, M2M2_ADDR_MED_SYNC_ADPD_ADXL_STREAM, p_pkt->src, true);
    status = M2M2_APP_COMMON_STATUS_SUBSCRIBER_ADDED;
    command = M2M2_APP_COMMON_CMD_STREAM_SUBSCRIBE_RESP;
    break;
  case M2M2_APP_COMMON_CMD_STREAM_UNSUBSCRIBE_REQ:
    if (g_state_ppgapp.num_subs <= 1) {
      g_state_ppgapp.num_subs = 0;
      g_state_syncapp.num_subs = 0;
      reset_ppgapp_packetization();
      reset_syncapp_packetization();
      status = M2M2_APP_COMMON_STATUS_SUBSCRIBER_REMOVED;
    } else {
      g_state_ppgapp.num_subs--;
      g_state_syncapp.num_subs--;
      status = M2M2_APP_COMMON_STATUS_SUBSCRIBER_COUNT_DECREMENT;
    }
    post_office_setup_subscriber(M2M2_ADDR_MED_PPG, M2M2_ADDR_MED_PPG_STREAM, p_pkt->src, false);
    post_office_setup_subscriber(M2M2_ADDR_MED_PPG, M2M2_ADDR_MED_SYNC_ADPD_ADXL_STREAM, p_pkt->src, false);
    command = M2M2_APP_COMMON_CMD_STREAM_UNSUBSCRIBE_RESP;
    break;
  default:
    /*!  Something has gone horribly wrong. */
    post_office_consume_msg(p_resp_pkt);
    return NULL;
  }
  p_resp_payload->command = command;
  p_resp_payload->status = status;
  p_resp_payload->stream = p_in_payload->stream;
  p_resp_pkt->src = p_pkt->dest;
  p_resp_pkt->dest = p_pkt->src;
  }/*! if(NULL != p_resp_pkt) */
  return p_resp_pkt;
}

/**
* @brief  Handles lcfg read/write command of ppg application
*
* @param[in]  p_pkt: Pointer to the input m2m2 packet structure with command
*                    for lcfg read/write access request
*
* @return     p_resp_pkt: Pointer to the output response m2m2 packet structure
*/
static m2m2_hdr_t *ppg_app_lcfg_access(m2m2_hdr_t *p_pkt) {
  M2M2_APP_COMMON_STATUS_ENUM_t status = M2M2_APP_COMMON_STATUS_ERROR;
  /*!  Declare a pointer to access the input packet payload */
  PYLD_CST(p_pkt, ppg_app_lcfg_op_hdr_t, p_in_payload);
  /*!  Allocate a response packet with space for the correct number of operations */
  PKT_MALLOC(p_resp_pkt, ppg_app_lcfg_op_hdr_t, p_in_payload->num_ops * sizeof(p_in_payload->ops[0]));
  if(NULL != p_resp_pkt)
  {
    PYLD_CST(p_resp_pkt, ppg_app_lcfg_op_hdr_t, p_resp_payload);
    uint32_t  value = 0;

    switch (p_in_payload->command) {
    case M2M2_APP_COMMON_CMD_READ_LCFG_REQ:
      for (int i = 0; i < p_in_payload->num_ops; i++) {
              if (MwPPG_ReadLCFG(p_in_payload->ops[i].field, &value)) {
          status = M2M2_APP_COMMON_STATUS_OK;
        } else {
          status = M2M2_APP_COMMON_STATUS_ERROR;
          break;
        }
        p_resp_payload->ops[i].field = p_in_payload->ops[i].field;
        p_resp_payload->ops[i].value = value;
      }
      p_resp_payload->command = (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_APP_COMMON_CMD_READ_LCFG_RESP;
      break;
    case M2M2_APP_COMMON_CMD_WRITE_LCFG_REQ:
      for (int i = 0; i < p_in_payload->num_ops; i++) {
        if (MwPPG_WriteLCFG(p_in_payload->ops[i].field, p_in_payload->ops[i].value)) {
          status = M2M2_APP_COMMON_STATUS_OK;
        } else {
          status = M2M2_APP_COMMON_STATUS_ERROR;
          break;
        }
        p_resp_payload->ops[i].field = p_in_payload->ops[i].field;
        p_resp_payload->ops[i].value = p_in_payload->ops[i].value;
      }
      p_resp_payload->command = (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_APP_COMMON_CMD_WRITE_LCFG_RESP;
      break;
    default:
      /*!  Something has gone horribly wrong. */
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
* @brief  Handles get last states command of ppg application
*
* @param[in]  p_pkt: Pointer to the input m2m2 packet structure with command
*                    for getting the last states of ppg app
*
* @return     p_resp_pkt: Pointer to the output response m2m2 packet structure
*/
static m2m2_hdr_t *ppg_app_get_last_states(m2m2_hdr_t *p_pkt) {
  /*!  Declare and malloc a response packet */
  PKT_MALLOC(p_resp_pkt, ppg_app_lib_state_t, 0);
  if(NULL != p_resp_pkt)
  {
    /*!  Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, ppg_app_lib_state_t, p_resp_payload);

    p_resp_payload->command = M2M2_PPG_APP_CMD_GET_LAST_STATES_RESP;
    p_resp_payload->status = M2M2_APP_COMMON_STATUS_ERROR;
    
    memset(&p_resp_payload->states[0], 0, sizeof(p_resp_payload->states));
    if (MwPPG_GetStates(&(p_resp_payload->states[0])) == PPG_SUCCESS) {
      /*!  Fill out the response packet header */
      p_resp_payload->status = M2M2_APP_COMMON_STATUS_OK;
    }
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  }
  return p_resp_pkt;
}

/**
* @brief  Handles get CTR command of ppg application
*
* @param[in]  p_pkt: Pointer to the input m2m2 packet structure with command
*                    for getting CTR value
*
* @return     p_resp_pkt: Pointer to the output response m2m2 packet structure
*/
static m2m2_hdr_t *ppg_app_get_ctr(m2m2_hdr_t *p_pkt) {
  /*!  Declare and malloc a response packet */
  PKT_MALLOC(p_resp_pkt, ppg_app_ctrValue_t, 0);
  if(NULL != p_resp_pkt)
  {
    /*!  Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, ppg_app_ctrValue_t, p_resp_payload);

    p_resp_payload->command = M2M2_PPG_APP_CMD_GET_CTRVALUE_RESP;
    p_resp_payload->status = M2M2_APP_COMMON_STATUS_OK;

    uint16_t temp16;
    /*! AdpdLibGetLibStat_CTR_Value(&temp16); */
    MwPpgGetLib_CTR_Value(&temp16);
    p_resp_payload->ctrValue = temp16;

    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  }
  return p_resp_pkt;
}

/**
* @brief  Handles get signal metrics command of ppg application
*
* @param[in]  p_pkt: Pointer to the input m2m2 packet structure with command
*                    for getting signal metrics of ppg app
*
* @return     p_resp_pkt: Pointer to the output response m2m2 packet structure
*/
static m2m2_hdr_t *ppg_app_get_signalmetrics(m2m2_hdr_t *p_pkt) {
  /*!  Declare and malloc a response packet */
  PKT_MALLOC(p_resp_pkt, ppg_app_signal_metrics_t, 0);
  if(NULL != p_resp_pkt)
  {
    /*!  Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, ppg_app_signal_metrics_t, p_resp_payload);

    p_resp_payload->command = M2M2_PPG_APP_CMD_GET_SMETRICS_RESP;
    p_resp_payload->status = M2M2_APP_COMMON_STATUS_OK;

    uint16_t temp16[6];
    /*! AdpdLibGetLibStat_AGC_SIGM(temp16); */
    MwPpgGetLib_AGC_SIGM(temp16);
    p_resp_payload->metrics[0] = temp16[0];
    p_resp_payload->metrics[1] = temp16[1];
    p_resp_payload->metrics[2] = temp16[2];

    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  }
  return p_resp_pkt;
}

/**
* @brief  Handles get states information command of ppg application
*
* @param[in]  p_pkt: Pointer to the input m2m2 packet structure with command
*                    for getting the ppg states information
*
* @return     p_resp_pkt: Pointer to the output response m2m2 packet structure
*/
static m2m2_hdr_t *ppg_app_get_states_info(m2m2_hdr_t *p_pkt) {

   /*!  Declare a pointer to access the input packet payload */
  PYLD_CST(p_pkt, ppg_app_state_info_t, p_in_payload);
  /*!  Declare and malloc a response packet */
  PKT_MALLOC(p_resp_pkt, ppg_app_state_info_t, 0);
  if(NULL != p_resp_pkt)
  {
    /*!  Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, ppg_app_state_info_t, p_resp_payload);

    p_resp_payload->command = M2M2_PPG_APP_CMD_GET_STATES_INFO_RESP;
    p_resp_payload->status = M2M2_APP_COMMON_STATUS_OK;

    MwPPG_GetStateInfo(p_in_payload->state, (uint16_t *)p_resp_payload->info);
    p_resp_payload->state = p_in_payload->state;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  }
  return p_resp_pkt;
}


/**
* @brief  sets lcfg of ppg application
*
* @param[in]  p_pkt: Pointer to the input m2m2 packet structure with command
*                    for setting the lcfg of ppg app
*
* @return     p_resp_pkt: Pointer to the output response m2m2 packet structure
*/
static m2m2_hdr_t *ppg_app_set_lcfg(m2m2_hdr_t *p_pkt) {

   /*!  Declare a pointer to access the input packet payload */
  PYLD_CST(p_pkt, ppg_app_set_lcfg_req_t, p_in_payload);
  /*!  Declare and malloc a response packet */
  PKT_MALLOC(p_resp_pkt, ppg_app_set_lcfg_resp_t, 0);
  if(NULL != p_resp_pkt)
  {
    /*!  Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, ppg_app_set_lcfg_resp_t, p_resp_payload);

    p_resp_payload->command = M2M2_APP_COMMON_CMD_SET_LCFG_RESP;
    p_resp_payload->status = M2M2_APP_COMMON_STATUS_OK;
    uint16_t lcfgid = p_in_payload->lcfgid;
    if (MwPpg_LoadppgLCFG(lcfgid) == PPG_SUCCESS) {
       p_resp_payload->status = M2M2_APP_COMMON_STATUS_OK;
    } else {
       p_resp_payload->status = M2M2_APP_COMMON_STATUS_ERROR;
    }
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  }
  return p_resp_pkt;
}

/**
* @brief  gets the lcfg of ppg application
*
* @param[in]  p_pkt: Pointer to the input m2m2 packet structure with command
*                    for getting the lcfg of ppg app
*
* @return     p_resp_pkt: Pointer to the output response m2m2 packet structure
*/
static m2m2_hdr_t *ppg_app_get_lcfg(m2m2_hdr_t *p_pkt) {
  int32_t  lcfgdata[MAXPPGDCBSIZE];
  uint8_t  size=0;
  /*!  Declare and malloc a response packet */
  PKT_MALLOC(p_resp_pkt, m2m2_ppg_lcfg_data_t, 0);
  if(NULL != p_resp_pkt)
  {
    /*!  Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_ppg_lcfg_data_t, p_resp_payload);

    p_resp_payload->command = M2M2_APP_COMMON_CMD_GET_LCFG_RESP;
    /*! if (MwPPG_ReadLCFGStruct((uint32_t *)&p_resp_payload->lcfgdata[0], &p_resp_payload->size)) {  // Gets the library configuration */
    if (MwPPG_ReadLCFGStruct((uint32_t *)&lcfgdata[0], &size)) {  /*!  Gets the library configuration */
      if(size!=0)
      {
        for(uint16_t i=0; i<size; i++)
          p_resp_payload->lcfgdata[i] = lcfgdata[i];
        p_resp_payload->size = size;
        p_resp_payload->status = M2M2_APP_COMMON_STATUS_OK;
      }
      else
      {
        p_resp_payload->status = M2M2_APP_COMMON_STATUS_ERROR;
      }
    } else {
      p_resp_payload->status = M2M2_APP_COMMON_STATUS_ERROR;
    }
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  }
  return p_resp_pkt;
}

/**
* @brief  synchronizes the adpd and adxl data with time stamps given
*
* @param[in]  p_data: Pointer to adpd data
* @param[in]  adpdtimestamp: adpd time stamp
* @param[in]  pAdxlData: Pointer to adxl data
* @param[in]  adxltimestamp: adxl time stamp
*
* @return     None
*/
void SyncAppDataSend(uint32_t *p_data,uint32_t adpdtimestamp,int16_t *pAdxlData,uint32_t adxltimestamp) {
#ifdef MUTEX
    adi_osal_MutexPend(SyncDataBuffLock, ADI_OSAL_TIMEOUT_FOREVER);
#endif /*! MUTEX */
    /*! syncdata_count++; */
  g_SyncAppBuff.gFifoDataBcount = 0;
  g_SyncAppBuff.gAccelDatacount = 0;
  SyncErrorStatus SyncRet = SYNC_ERROR;
  if (g_state_syncapp.num_subs > 0 ) {
    if (p_data != NULL) {
      g_SyncAppBuff.gFifoDataB[0] = p_data[0];
      g_SyncAppBuff.gFifoDataB[1] = p_data[1];
      g_SyncAppBuff.gTsAdpd = adpdtimestamp;
      g_SyncAppBuff.gFifoDataBcount = 1;
    }
    if (pAdxlData != NULL) {
      g_SyncAppBuff.gAccelData[0] = pAdxlData[0];
      g_SyncAppBuff.gAccelData[1] = pAdxlData[1];
      g_SyncAppBuff.gAccelData[2] = pAdxlData[2];
      g_SyncAppBuff.gTsAdxl = adxltimestamp;
      g_SyncAppBuff.gAccelDatacount = 1;
    }

    if(!hr_mode_on)
    {
      event_from_sync(); /*! In case of ADPD4000 IDLE state. release ppg_task semaphore and return.*/
      return;
    }
    else
    {
    SyncRet = DoSync((g_SyncAppBuff.gFifoDataBcount > 0)?g_SyncAppBuff.gFifoDataB:0,
                     g_SyncAppBuff.gTsAdpd,
                     (g_SyncAppBuff.gAccelDatacount > 0)?g_SyncAppBuff.gAccelData:0,
                     g_SyncAppBuff.gTsAdxl);

    if (SyncRet == SYNC_SUCCESS) {
      event_from_sync();
    }
    }
  }
#ifdef MUTEX
    adi_osal_MutexPost(SyncDataBuffLock);
#endif /*! MUTEX */
}

#ifdef DCB
/**
 * @brief  Returns the PPG DCB configuration information from FDS
 *
 * @param[in]  p_pkt: Pointer to the input m2m2 packet structure with command
 *                    for reading ppg dcb config information
 *
 * @return     p_resp_pkt: Pointer to the output response m2m2 packet structure
 *                         with ppg dcb config information
 */
static m2m2_hdr_t *ppg_dcb_command_read_config(m2m2_hdr_t *p_pkt)
{
    static uint16_t r_size = 0;
    int32_t dcbdata[MAXPPGDCBSIZE];

    M2M2_DCB_STATUS_ENUM_t status = M2M2_DCB_STATUS_ERR_NOT_CHKD;

    PKT_MALLOC(p_resp_pkt, m2m2_dcb_ppg_data_t, 0);
    if(NULL != p_resp_pkt)
    {
      /*!  Declare a pointer to the response packet payload */
      PYLD_CST(p_resp_pkt, m2m2_dcb_ppg_data_t, p_resp_payload);

      r_size = (uint16_t)MAXPPGDCBSIZE;
      if(read_ppg_dcb((uint32_t*)&dcbdata[0],&r_size) == PPG_SUCCESS)
      {
          for(int i=0; i<r_size; i++)
            p_resp_payload->dcbdata[i] = dcbdata[i];
          p_resp_payload->size = (r_size);
          status = M2M2_DCB_STATUS_OK;
      }
      else
      {
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

/**
 * @brief  Writes the PPG DCB configuration information to FDS
 *
 * @param[in]  p_pkt: Pointer to the input m2m2 packet structure with command
 *                    for writing ppg dcb config information
 *
 * @return     p_resp_pkt: Pointer to the output response m2m2 packet structure
 */
static m2m2_hdr_t *ppg_dcb_command_write_config(m2m2_hdr_t *p_pkt)
{
    M2M2_DCB_STATUS_ENUM_t status = M2M2_DCB_STATUS_ERR_NOT_CHKD;
    int32_t dcbdata[MAXPPGDCBSIZE];

    /*!  Declare a pointer to access the input packet payload */
    PYLD_CST(p_pkt, m2m2_dcb_ppg_data_t, p_in_payload);
    PKT_MALLOC(p_resp_pkt, m2m2_dcb_ppg_data_t, 0);
    if(NULL != p_resp_pkt)
    {
      /*!  Declare a pointer to the response packet payload */
      PYLD_CST(p_resp_pkt, m2m2_dcb_ppg_data_t, p_resp_payload);

      for(int i=0; i<p_in_payload->size; i++)
        dcbdata[i] = p_in_payload->dcbdata[i];
      if(write_ppg_dcb((uint32_t*)&dcbdata[0], p_in_payload->size) == PPG_SUCCESS)
        {
          ppg_set_dcb_present_flag(true);
          status = M2M2_DCB_STATUS_OK;
        }
      else
        {
          status = M2M2_DCB_STATUS_ERR_ARGS;
        }

      p_resp_payload->status = status;
      p_resp_payload->command = M2M2_DCB_COMMAND_WRITE_CONFIG_RESP;
      p_resp_payload->size = 0;
      for(uint16_t i=0; i< MAXPPGDCBSIZE; i++)
        p_resp_payload->dcbdata[i] = 0;
      p_resp_pkt->src = p_pkt->dest;
      p_resp_pkt->dest = p_pkt->src;
    }
    return p_resp_pkt;
}
/**
 * @brief  Deletes the PPG DCB configuration information from FDS
 *
 * @param[in]  p_pkt: Pointer to the input m2m2 packet structure with command
 *                    for deleting PPG dcb config information
 *
 * @return     p_resp_pkt: Pointer to the output response m2m2 packet structure
 */
static m2m2_hdr_t *ppg_dcb_command_delete_config(m2m2_hdr_t *p_pkt)
{
    M2M2_DCB_STATUS_ENUM_t status = M2M2_DCB_STATUS_ERR_NOT_CHKD;

    PKT_MALLOC(p_resp_pkt, m2m2_dcb_ppg_data_t, 0);
    if(NULL != p_resp_pkt)
    {
      /*!  Declare a pointer to the response packet payload */
      PYLD_CST(p_resp_pkt, m2m2_dcb_ppg_data_t, p_resp_payload);

      if(delete_ppg_dcb() == PPG_SUCCESS)
      {
          ppg_set_dcb_present_flag(false);
          status = M2M2_DCB_STATUS_OK;
      }
      else
      {
          status = M2M2_DCB_STATUS_ERR_ARGS;
      }

      p_resp_payload->status = status;
      p_resp_payload->command = M2M2_DCB_COMMAND_ERASE_CONFIG_RESP;
      p_resp_payload->size = 0;
      for(uint16_t i=0; i< MAXPPGDCBSIZE; i++)
        p_resp_payload->dcbdata[i] = 0;
      p_resp_pkt->src = p_pkt->dest;
      p_resp_pkt->dest = p_pkt->src;
    }
    return p_resp_pkt;

}

#endif
#endif//ENABLE_PPG_APP