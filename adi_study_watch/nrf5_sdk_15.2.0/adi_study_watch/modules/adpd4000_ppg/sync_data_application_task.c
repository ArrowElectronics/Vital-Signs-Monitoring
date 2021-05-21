/**
***************************************************************************
* @addtogroup Tasks
* @{
* @file         sync_data_application_task.c
* @author       ADI
* @version      V1.0.0
* @date         15-Mar-2019
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
* Copyright (c) 2019 Analog Devices Inc.
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
#include <limits.h>
#include <includes.h>
#include "sync_data_application_task.h"
#include "sync_data_application_interface.h"
#include "ppg_application_interface.h"
#include "hw_if_config.h"
#include "AdpdLib.h"
#include "AppSync.h"
#include "MwPPG.h"
#include "Adpd400xDrv.h"
#include "Adxl362.h"
#include "sensor_internal.h"
#include "AppCommon.h"
//#include <adpd_buffering.h>
#include <adxl_buffering.h>
#include "adpd4000_buffering.h"
/* ------------------------- Defines  -------------------------------------- */
#define SKIP_ADPD_ADXL_SAMPLES     16

/* ------------------------- Type Definition --------------------------------*/

/* -------------------------Public variables -------------------------------*/
const char GIT_SYNC_DATA_APP_VERSION[] = "TEST SYNC_DATA_APP_VERSION STRING";
const uint8_t GIT_SYNC_DATA_APP_VERSION_LEN = sizeof(GIT_SYNC_DATA_APP_VERSION);
/* ------------------------- Public Function Prototypes -------------------- */
//extern void PpgGetEvent(uint32_t *pPpgData, int16_t *pAdxlData, TimeStamps_t timeStamp);
extern void event_from_sync(void);
void RegisterSyncPpgCB(int16_t (*fnp)(uint8_t));
void RegisterSyncPpgDataCB(int16_t (*fnp)(uint32_t *, uint32_t *, uint32_t *));
/* ------------------------- Private variables ----------------------------- */
typedef struct _packetizer_t {
  m2m2_hdr_t                *p_pkt;
  uint8_t                    packet_nsamples;
  uint8_t                    skip_samples;
  uint32_t                   nPrevPpgTS;
  uint32_t                   nPrevAdxlTS;
} packetizer_t;

static struct _g_state_syncapp {
  uint16_t  num_subs;
  uint16_t  num_starts;
  uint8_t   decimation_factor;
  uint16_t  decimation_nsamples;
  uint16_t  data_pkt_seq_num;
  packetizer_t  syncapp_pktizer;
} g_state_syncapp;
static int16_t (*gfpGetDeviceData)(uint32_t *, uint32_t *, uint32_t *);
// Create the queue
ADI_OSAL_QUEUE_HANDLE  sync_data_task_msg_queue = NULL;
StaticTask_t syncdataTaskTcb;
ADI_OSAL_STATIC_THREAD_ATTR sync_data_application_task_attributes;
ADI_OSAL_THREAD_HANDLE sync_data_task_handler;
ADI_OSAL_SEM_HANDLE   sync_data_application_task_evt_sem;
uint8_t sync_data_application_task_stack[APP_OS_CFG_SYNC_DATA_APPLICATION_TASK_STK_SIZE];


  uint16_t                  sample_szA, sample_szB;
//  ADPDDrv_Operation_Slot_t  slot_modeA, slot_modeB;

/* ------------------------- Private Function Prototypes ------------------- */
static uint8_t MwSyncDataDeInit();
static m2m2_hdr_t *sync_data_app_stream_config(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *sync_data_app_status(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *sync_data_app_decimation(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *sync_data_app_get_version(m2m2_hdr_t *p_pkt);
static void sync_data_application_task(void *pArgument);
static void reset_syncapp_packetization(void);
static void packetize_ppg_sync_data(uint32_t *pPpgData, uint16_t *pAdxlData, uint32_t ppgTS,uint32_t adxlTS);
static int16_t (*gfpSyncPpgAppSetOpMode)(uint8_t OpMode);

void sync_data_ready_cb(void);

sync_ppg_routing_table_entry_t sync_data_app_routing_table[] = {
  {M2M2_APP_COMMON_CMD_STREAM_START_REQ, sync_data_app_stream_config},
  {M2M2_APP_COMMON_CMD_STREAM_STOP_REQ, sync_data_app_stream_config},
  {M2M2_APP_COMMON_CMD_STREAM_SUBSCRIBE_REQ, sync_data_app_stream_config},
  {M2M2_APP_COMMON_CMD_STREAM_UNSUBSCRIBE_REQ, sync_data_app_stream_config},
  {M2M2_APP_COMMON_CMD_SENSOR_STATUS_QUERY_REQ, sync_data_app_status},
  {M2M2_SENSOR_COMMON_CMD_GET_STREAM_DEC_FACTOR_REQ, sync_data_app_decimation},
  {M2M2_SENSOR_COMMON_CMD_SET_STREAM_DEC_FACTOR_REQ, sync_data_app_decimation},
  {M2M2_APP_COMMON_CMD_GET_VERSION_REQ, sync_data_app_get_version},
};
#define SYNC_DATA_APP_ROUTING_TBL_SZ (sizeof(sync_data_app_routing_table) / sizeof(sync_data_app_routing_table[0]))

// Function called by the post office to send messages to this application
void send_message_sync_data_application_task(m2m2_hdr_t *p_pkt) {
  ADI_OSAL_STATUS osal_result;
  osal_result = adi_osal_MsgQueuePost(sync_data_task_msg_queue,p_pkt);
  if (osal_result != ADI_OSAL_SUCCESS)
    Debug_Handler();

  adi_osal_SemPost(sync_data_application_task_evt_sem);
}

void sync_data_application_task_init(void) {
  // Initialize app state
  g_state_syncapp.num_subs = 0;
  g_state_syncapp.num_starts = 0;
  // Default behaviour is to send every packet
  g_state_syncapp.decimation_factor = 1;
  g_state_syncapp.decimation_nsamples = 0;
  g_state_syncapp.data_pkt_seq_num = 0;
  reset_syncapp_packetization();

  ADI_OSAL_STATUS eOsStatus = ADI_OSAL_SUCCESS;
  sync_data_application_task_attributes.pThreadFunc = sync_data_application_task;
  sync_data_application_task_attributes.nPriority = APP_OS_CFG_SYNC_DATA_APPLICATION_TASK_PRIO;
  sync_data_application_task_attributes.pStackBase = &sync_data_application_task_stack[0];
  sync_data_application_task_attributes.nStackSize = APP_OS_CFG_SYNC_DATA_APPLICATION_TASK_STK_SIZE;
  sync_data_application_task_attributes.pTaskAttrParam = NULL;
  sync_data_application_task_attributes.szThreadName = "Sync Data";
  sync_data_application_task_attributes.pThreadTcb = &syncdataTaskTcb;

  eOsStatus = adi_osal_MsgQueueCreate(&sync_data_task_msg_queue,NULL,
                                    5);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
      Debug_Handler();
  }
  else
  {
    update_task_queue_list(APP_OS_CFG_SYNC_DATA_TASK_INDEX,sync_data_task_msg_queue);
  }
  eOsStatus = adi_osal_ThreadCreateStatic(&sync_data_task_handler,
                                    &sync_data_application_task_attributes);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
      Debug_Handler();
  }
}

void reset_syncapp_packetization(){

}

void RegisterSyncPpgCB(int16_t (*pfppg_set_opmode)(uint8_t)) {
  gfpSyncPpgAppSetOpMode = pfppg_set_opmode;
}

void RegisterSyncPpgDataCB(int16_t (*GetDeviceData)(uint32_t *, uint32_t *, uint32_t *)) {
  gfpGetDeviceData = GetDeviceData;
}
static void sync_data_application_task(void *pArgument) {
  m2m2_hdr_t *p_in_pkt = NULL;
  m2m2_hdr_t *p_out_pkt = NULL;
  ADI_OSAL_STATUS         err;
  int32_t nAdpdSampleCount = 0, nAdxlSampleCount = 0;
  uint32_t aFifoDataA[4] = {0};
  uint32_t aFifoDataB[4] = {0};
  uint32_t nTsAdpd = 0, nTsAdxl = 0;
  uint16_t aAccelData[3] = {0};
  TimeStamps_t timeStamp;
  SyncErrorStatus SyncRet = (SyncErrorStatus)0;

  adi_osal_SemCreate(&sync_data_application_task_evt_sem, 0);
  post_office_add_mailbox(M2M2_ADDR_MED_SYNC_ADPD_ADXL, M2M2_ADDR_MED_SYNC_ADPD_ADXL_STREAM);
  adpd4000_buff_init(4);
  while (1) {
    adi_osal_SemPend(sync_data_application_task_evt_sem, ADI_OSAL_TIMEOUT_FOREVER);
    p_in_pkt = post_office_get(ADI_OSAL_TIMEOUT_NONE, APP_OS_CFG_SYNC_DATA_TASK_INDEX);
    if (p_in_pkt == NULL) {
      // ToDo
      for (int i = 0; i < 4; i++) {
        nAdpdSampleCount = (*gfpGetDeviceData)(aFifoDataA, aFifoDataB, &nTsAdpd);
        nAdxlSampleCount = sync_adxl_read_data_to_buffer(aAccelData, &nTsAdxl);

        if (nAdxlSampleCount > 0 || nAdpdSampleCount > 0) {
          SyncRet = DoSync((nAdpdSampleCount > 0)?aFifoDataB:0,
                           nTsAdpd,
                           (nAdxlSampleCount > 0)?aAccelData:0,
                           nTsAdxl);
          if (SyncRet == SYNC_SUCCESS) {
            timeStamp.tsADPD = GetSyncAdpdTs();
            timeStamp.tsADXL = GetSyncAccelTs();
            timeStamp.tsAlgorithmCall = AdpdLibGetTick();
            // Post the semaphore of PPG
            event_from_sync();
            packetize_ppg_sync_data(&(GetSyncAdpdData())[0], GetSyncRawAccelData(), timeStamp.tsADPD,timeStamp.tsADXL);
          }
        }
      }
    } else {
      // We got an m2m2 message from the queue, process it.
      PYLD_CST(p_in_pkt, _m2m2_app_common_cmd_t, p_in_cmd);
      // Look up the appropriate function to call in the function table
      for (int i = 0; i < SYNC_DATA_APP_ROUTING_TBL_SZ; i++) {
        if (sync_data_app_routing_table[i].command == p_in_cmd->command) {
          p_out_pkt = sync_data_app_routing_table[i].cb_handler(p_in_pkt);
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
  *@brief       Constructs a packet to send the PPG sync data
  *@param[pPkt]  UART packet to send
  *@param[PPG data] nSlotB
  *@param[pAdxlData] adxl data
  *@param[timeStamp] Timestamp data
  *@return
 */
static void packetize_ppg_sync_data(uint32_t *pPpgData, uint16_t *pAdxlData, uint32_t ppgTS,uint32_t adxlTS) {

  ADI_OSAL_STATUS         err;
  if( g_state_syncapp.syncapp_pktizer.skip_samples  >= SKIP_ADPD_ADXL_SAMPLES){
    // first check if there is subscribtion to sync data
    if (g_state_syncapp.num_subs > 0) {
      // the first sample includes the complete TS
      if( g_state_syncapp.syncapp_pktizer.packet_nsamples == 0){
        g_state_syncapp.syncapp_pktizer.p_pkt = post_office_create_msg(sizeof(adpd_adxl_sync_data_stream_t) + M2M2_HEADER_SZ);
        if(g_state_syncapp.syncapp_pktizer.p_pkt != NULL){
          PYLD_CST(g_state_syncapp.syncapp_pktizer.p_pkt, adpd_adxl_sync_data_stream_t, p_payload_ptr);
          p_payload_ptr->syncData.ppgTS = ppgTS;
          p_payload_ptr->syncData.adxlTS = adxlTS;
          p_payload_ptr->syncData.ppgData[0] = *pPpgData;
          p_payload_ptr->syncData.xData[0] = pAdxlData[0];  // X data
          p_payload_ptr->syncData.yData[0] = pAdxlData[1];  // Y data
          p_payload_ptr->syncData.zData[0] = pAdxlData[2];  // Z data
          g_state_syncapp.decimation_nsamples = 1;
        }
      }
      // Not the first sample of the packet
      else{
        PYLD_CST(g_state_syncapp.syncapp_pktizer.p_pkt, adpd_adxl_sync_data_stream_t, p_payload_ptr);
        // check dec factor
        if (!( g_state_syncapp.syncapp_pktizer.packet_nsamples% g_state_syncapp.decimation_factor)) {

          // Prevent rollover on ppg TS
          if (ppgTS <  g_state_syncapp.syncapp_pktizer.nPrevPpgTS) {
            p_payload_ptr->syncData.incPpgTS[ g_state_syncapp.decimation_nsamples-1] = UINT_MAX -  g_state_syncapp.syncapp_pktizer.nPrevPpgTS + ppgTS;
          } else {
            p_payload_ptr->syncData.incPpgTS[ g_state_syncapp.decimation_nsamples-1] = ppgTS -  g_state_syncapp.syncapp_pktizer.nPrevPpgTS;
          }

          // Prevent rollover on adxl TS
          if (adxlTS < g_state_syncapp.syncapp_pktizer.nPrevAdxlTS) {
            p_payload_ptr->syncData.incAdxlTS[ g_state_syncapp.decimation_nsamples-1] = UINT_MAX - g_state_syncapp.syncapp_pktizer.nPrevAdxlTS + adxlTS;
          } else {
            p_payload_ptr->syncData.incAdxlTS[ g_state_syncapp.decimation_nsamples-1] = adxlTS - g_state_syncapp.syncapp_pktizer.nPrevAdxlTS;
          }

          // ppg and adxl data
          p_payload_ptr->syncData.ppgData[ g_state_syncapp.decimation_nsamples] = *pPpgData;
          p_payload_ptr->syncData.xData[ g_state_syncapp.decimation_nsamples] = pAdxlData[0];
          p_payload_ptr->syncData.yData[ g_state_syncapp.decimation_nsamples] = pAdxlData[1];
          p_payload_ptr->syncData.zData[ g_state_syncapp.decimation_nsamples++] = pAdxlData[2];
        }
      }

      // check the number of samples to send the packet
      if(++g_state_syncapp.syncapp_pktizer.packet_nsamples >= (4 *  g_state_syncapp.decimation_factor)) {
        g_state_syncapp.syncapp_pktizer.packet_nsamples = 0;
        PYLD_CST(g_state_syncapp.syncapp_pktizer.p_pkt,adpd_adxl_sync_data_stream_t, p_payload_ptr);
        p_payload_ptr->command = M2M2_SENSOR_COMMON_CMD_STREAM_DATA;
        p_payload_ptr->status = 0x00;
        g_state_syncapp.syncapp_pktizer.p_pkt->src = M2M2_ADDR_MED_SYNC_ADPD_ADXL;
        g_state_syncapp.syncapp_pktizer.p_pkt->dest = M2M2_ADDR_MED_SYNC_ADPD_ADXL_STREAM;
        p_payload_ptr->sequence_num = g_state_syncapp.data_pkt_seq_num++;
        post_office_send(g_state_syncapp.syncapp_pktizer.p_pkt, &err);
        g_state_syncapp.syncapp_pktizer.p_pkt = NULL;
      }
      // update the previous TS
      g_state_syncapp.syncapp_pktizer.nPrevPpgTS = ppgTS;
      g_state_syncapp.syncapp_pktizer.nPrevAdxlTS = adxlTS;
    }
  } else {
    g_state_syncapp.syncapp_pktizer.skip_samples ++;
  }
}

static m2m2_hdr_t *sync_data_app_get_version(m2m2_hdr_t *p_pkt) {
  PKT_MALLOC(p_resp_pkt, m2m2_app_common_version_t, 0);
  // Declare a pointer to the response packet payload
  PYLD_CST(p_resp_pkt, m2m2_app_common_version_t, p_resp_payload);

  p_resp_payload->command = M2M2_APP_COMMON_CMD_GET_VERSION_RESP;
  p_resp_payload->major = 0x03;
  p_resp_payload->minor = 0x05;
  p_resp_payload->patch = 0x00;
  memcpy(&p_resp_payload->str[0], &GIT_SYNC_DATA_APP_VERSION, GIT_SYNC_DATA_APP_VERSION_LEN);
  p_resp_pkt->src = p_pkt->dest;
  p_resp_pkt->dest = p_pkt->src;
  p_resp_pkt->checksum = 0x0000;

  return p_resp_pkt;
}

static m2m2_hdr_t *sync_data_app_status(m2m2_hdr_t *p_pkt) {
  // Declare and malloc a response packet
  PKT_MALLOC(p_resp_pkt, m2m2_app_common_status_t, 0);
  // Declare a pointer to the response packet payload
  PYLD_CST(p_resp_pkt, m2m2_app_common_status_t, p_resp_payload);

  p_resp_payload->command = M2M2_APP_COMMON_CMD_SENSOR_STATUS_QUERY_RESP;
  p_resp_payload->status = M2M2_APP_COMMON_STATUS_ERROR;

  if (g_state_syncapp.num_starts == 0) {
    p_resp_payload->status = M2M2_APP_COMMON_STATUS_STREAM_STOPPED;
  } else {
    p_resp_payload->status = M2M2_APP_COMMON_STATUS_STREAM_STARTED;
  }
  p_resp_payload->num_subscribers = g_state_syncapp.num_subs;
  p_resp_payload->num_start_reqs = g_state_syncapp.num_starts;
  p_resp_pkt->src = p_pkt->dest;
  p_resp_pkt->dest = p_pkt->src;
  return p_resp_pkt;
}

static m2m2_hdr_t *sync_data_app_stream_config(m2m2_hdr_t *p_pkt) {
  M2M2_APP_COMMON_STATUS_ENUM_t status = M2M2_APP_COMMON_STATUS_ERROR;
  M2M2_APP_COMMON_CMD_ENUM_t    command;
  // Declare a pointer to access the input packet payload
  PYLD_CST(p_pkt, m2m2_app_common_sub_op_t, p_in_payload);
  // Declare and malloc a response packet
  PKT_MALLOC(p_resp_pkt, m2m2_app_common_sub_op_t, 0);
  // Declare a pointer to the response packet payload
  PYLD_CST(p_resp_pkt, m2m2_app_common_sub_op_t, p_resp_payload);

  switch (p_in_payload->command) {
  case M2M2_APP_COMMON_CMD_STREAM_START_REQ:
    if (g_state_syncapp.num_starts == 0) {
      AdxlDrvDataReadyCallback(sync_data_ready_cb);
      (*gfpSyncPpgAppSetOpMode)(ADPDDrv_MODE_IDLE);
      //AdpdDrvSetOperationMode(ADPDDrv_MODE_IDLE);
      SyncInit();
      /* Enable ADPD first then ADXL */
      (*gfpSyncPpgAppSetOpMode)(ADPDDrv_MODE_SAMPLE);
      //AdpdDrvSetOperationMode(ADPDDrv_MODE_SAMPLE);
      AdxlDrvSetOperationMode(PRM_MEASURE_MEASURE_MODE);
      g_state_syncapp.num_starts = 1;
      g_state_syncapp.syncapp_pktizer.packet_nsamples = 0;
      status  = M2M2_APP_COMMON_STATUS_STREAM_STARTED;
    } else {
      g_state_syncapp.num_starts++;
      status = M2M2_APP_COMMON_STATUS_STREAM_IN_PROGRESS;
    }
    command = M2M2_APP_COMMON_CMD_STREAM_START_RESP;
    break;
  case M2M2_APP_COMMON_CMD_STREAM_STOP_REQ:
    if (g_state_syncapp.num_starts == 0) {
      status = M2M2_APP_COMMON_STATUS_STREAM_STOPPED;
    } else if (g_state_syncapp.num_starts == 1) {
      if (!MwSyncDataDeInit()) {
        (*gfpSyncPpgAppSetOpMode)(ADPDDrv_MODE_IDLE);
        //AdpdDrvSetOperationMode(ADPDDrv_MODE_IDLE);
        AdxlDrvSetOperationMode(PRM_MEASURE_STANDBY_MODE);
        SyncDeInit();
        g_state_syncapp.num_starts = 0;
        status = M2M2_APP_COMMON_STATUS_STREAM_STOPPED;
      } else {
        g_state_syncapp.num_starts = 1;
        status = M2M2_APP_COMMON_STATUS_ERROR;
      }
      reset_syncapp_packetization();
      if(g_state_syncapp.syncapp_pktizer.p_pkt != NULL){
        post_office_consume_msg(g_state_syncapp.syncapp_pktizer.p_pkt);
        g_state_syncapp.syncapp_pktizer.p_pkt = NULL;
      }
    } else {
      g_state_syncapp.num_starts--;
      status = M2M2_APP_COMMON_STATUS_STREAM_COUNT_DECREMENT;
    }
    command = M2M2_APP_COMMON_CMD_STREAM_STOP_RESP;
    break;
  case M2M2_APP_COMMON_CMD_STREAM_SUBSCRIBE_REQ:
    g_state_syncapp.num_subs++;
    post_office_setup_subscriber(M2M2_ADDR_MED_SYNC_ADPD_ADXL, M2M2_ADDR_MED_SYNC_ADPD_ADXL_STREAM, p_pkt->src, true);
    status = M2M2_APP_COMMON_STATUS_SUBSCRIBER_ADDED;
    command = M2M2_APP_COMMON_CMD_STREAM_SUBSCRIBE_RESP;
    break;
  case M2M2_APP_COMMON_CMD_STREAM_UNSUBSCRIBE_REQ:
    if (g_state_syncapp.num_subs <= 1) {
      g_state_syncapp.num_subs = 0;
      reset_syncapp_packetization();
      status = M2M2_APP_COMMON_STATUS_SUBSCRIBER_REMOVED;
    } else {
      g_state_syncapp.num_subs--;
      status = M2M2_APP_COMMON_STATUS_SUBSCRIBER_COUNT_DECREMENT;
    }
    post_office_setup_subscriber(M2M2_ADDR_MED_SYNC_ADPD_ADXL, M2M2_ADDR_MED_SYNC_ADPD_ADXL_STREAM, p_pkt->src, false);
    command = M2M2_APP_COMMON_CMD_STREAM_UNSUBSCRIBE_RESP;
    break;
  default:
    // Something has gone horribly wrong.
    return NULL;
  }
  p_resp_payload->command = command;
  p_resp_payload->status = status;
  p_resp_payload->stream = p_in_payload->stream;
  p_resp_pkt->src = p_pkt->dest;
  p_resp_pkt->dest = p_pkt->src;
  return p_resp_pkt;
}

static m2m2_hdr_t *sync_data_app_decimation(m2m2_hdr_t *p_pkt) {
  M2M2_APP_COMMON_STATUS_ENUM_t status = M2M2_APP_COMMON_STATUS_ERROR;

  // Declare a pointer to access the input packet payload
  PYLD_CST(p_pkt, m2m2_sensor_common_decimate_stream_t, p_in_payload);
  // Allocate a response packet with space for the correct number of operations
  PKT_MALLOC(p_resp_pkt, m2m2_sensor_common_decimate_stream_t, 0);
  PYLD_CST(p_resp_pkt, m2m2_sensor_common_decimate_stream_t, p_resp_payload);

  switch (p_in_payload->command) {
  case M2M2_SENSOR_COMMON_CMD_GET_STREAM_DEC_FACTOR_REQ:
    p_resp_payload->command = M2M2_SENSOR_COMMON_CMD_GET_STREAM_DEC_FACTOR_RESP;
    status = M2M2_APP_COMMON_STATUS_OK;
    break;
  case M2M2_SENSOR_COMMON_CMD_SET_STREAM_DEC_FACTOR_REQ:
    g_state_syncapp.decimation_factor = p_in_payload->dec_factor;
    if (g_state_syncapp.decimation_factor == 0) {
      g_state_syncapp.decimation_factor = 1;
    }
    status = M2M2_APP_COMMON_STATUS_OK;
    p_resp_payload->command = M2M2_SENSOR_COMMON_CMD_SET_STREAM_DEC_FACTOR_RESP;
    break;
  }
  p_resp_payload->status = status;
  p_resp_payload->dec_factor = g_state_syncapp.decimation_factor;
  p_resp_pkt->dest = p_pkt->src;
  p_resp_pkt->src = p_pkt->dest;
  return p_resp_pkt;
}


static uint8_t MwSyncDataDeInit()  {
  SyncDeInit();
  return 0;
}

ADXL_TS_DATA_TYPE gsyncADXL_dready_ts;
ADPD_TS_DATA_TYPE gsyncADPD_dready_ts;
volatile uint8_t gnSyncAdpdDataReady, gnSyncAdxlDataReady;
uint32_t gIntCnt = 0;
void sync_data_ready_cb(void) {
  gsyncADXL_dready_ts = MCU_HAL_GetTick();//HAL_RTC_GetTick();// MCU_HAL_GetTick();
  gsyncADPD_dready_ts = gsyncADXL_dready_ts;
  adi_osal_SemPost(sync_data_application_task_evt_sem);
  gnSyncAdpdDataReady = 1;
  gnSyncAdxlDataReady = 1;
}