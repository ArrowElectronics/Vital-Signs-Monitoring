/**
*******************************************************************************
* @file         sqi_task.c
* @author       ADI
* @version      V1.0.0
* @date         17-April-2020
* @brief        Source file contains sqi processing wrapper.
*
*******************************************************************************
* @attention
*******************************************************************************
*/
/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2020 Analog Devices Inc.                                      *
* All rights reserved.                                                        *
*                                                                             *
* This source code is intended for the recipient only under the guidelines of *
* the non-disclosure agreement with Analog Devices Inc.                       *
*                                                                             *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,    *
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE *
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      *
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING     *
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER         *
* DEALINGS IN THE SOFTWARE.                                                   *
*                                                                             *
*                                                                             *
* This software is intended for use with the sqi                     *
*                                                                             *
*                                                                             *
******************************************************************************/
#ifdef ENABLE_SQI_APP
#include <sqi_application_interface.h>
#include <sqi_task.h>
#include <task_includes.h>
#include <string.h>
#include <stdint.h>
#include "post_office_interface.h"
#include "m2m2_core.h"
#include "Common.h"
#include "adi_sqi_algo.h"
#include "adi_vsm_sqi.h"
#include "adpd4000_task.h"
#include "post_office.h"
#include "nrf_log.h"
#ifdef PROFILE_TIME_ENABLED
#include <us_tick.h>
#endif

#define SQI_ALGO_MAJOR_VERSION     ((ADI_SQI_VERSION_NUM >> 16) & 0xFF)
#define SQI_ALGO_MINOR_VERSION     ((ADI_SQI_VERSION_NUM >> 8) & 0xFF)
#define SQI_ALGO_PATCH_VERSION     (ADI_SQI_VERSION_NUM & 0xFF)

#define SQI_APP_MAX_LCFG_OPS (10)
/* -------------------------Public variables -------------------------------*/
uint32_t  gnSQI_Slot = 0x20; //slot-F;
uint32_t  sqi_event;
#ifdef DEBUG_PKT
uint32_t g_sqi_pkt_cnt = 0;
#endif
extern g_state_t  g_state;

/* flag to check if adpd data is externally fed for sqi stream */
extern bool gAdpd_ext_data_stream_active;
/* odr for externally fed adpd data */
extern uint16_t gAdpd_ext_data_stream_odr;

#undef PROFILE_TIME

/* ------------------------- Private variables ----------------------------- */
// Create the stack for task
uint8_t sqi_task_stack[APP_OS_CFG_SQI_APP_TASK_STK_SIZE];

// Create handler for task
ADI_OSAL_THREAD_HANDLE sqi_task_handler;

// Create task attributes variable
ADI_OSAL_STATIC_THREAD_ATTR sqi_task_attributes;

// Create TCB for task
StaticTask_t sqi_task_tcb;

// Create semaphores
ADI_OSAL_SEM_HANDLE   sqi_task_evt_sem;

// Create Queue Handler for task
ADI_OSAL_QUEUE_HANDLE  sqi_task_msg_queue = NULL;
ADI_OSAL_MUTEX_HANDLE g_sqi_data_lock;

const char GIT_SQI_VERSION[] = "TEST SQI_VERSION STRING";
const uint8_t GIT_SQI_VERSION_LEN = sizeof(GIT_SQI_VERSION);

typedef  m2m2_hdr_t*(app_cb_function_t)(m2m2_hdr_t*);

static struct _g_state_sqi {
  uint16_t nSubscriberCount;
  uint16_t nSequenceCount;
  uint16_t nStreamStartCount;
  uint16_t nSQIODR;
} g_state_sqi = {0, 0, 0, 0};

typedef struct _app_routing_table_entry_t {
  uint8_t                    command;
  app_cb_function_t          *cb_handler;
}app_routing_table_entry_t;

/* ------------------------- Private Function Prototypes ------------------- */

static m2m2_hdr_t *sqi_stream_config(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *sqi_lcfg_access(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *sqi_status(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *sqi_get_version(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *sqi_set_adpd_slot(m2m2_hdr_t *p_pkt);
static void sqi_task(void *pArgument);
static void packetize_sqi_data(uint32_t nSQIData, uint32_t nTimeStamp);

app_routing_table_entry_t sqi_routing_table[] = {
  {M2M2_APP_COMMON_CMD_STREAM_START_REQ, sqi_stream_config},
  {M2M2_APP_COMMON_CMD_STREAM_STOP_REQ, sqi_stream_config},
  {M2M2_APP_COMMON_CMD_STREAM_SUBSCRIBE_REQ, sqi_stream_config},
  {M2M2_APP_COMMON_CMD_STREAM_UNSUBSCRIBE_REQ, sqi_stream_config},
  {M2M2_APP_COMMON_CMD_SENSOR_STATUS_QUERY_REQ, sqi_status},
  {M2M2_APP_COMMON_CMD_GET_VERSION_REQ, sqi_get_version},
  {M2M2_SQI_APP_CMD_GET_ALGO_VENDOR_VERSION_REQ, sqi_get_version},
  {M2M2_APP_COMMON_CMD_WRITE_LCFG_REQ, sqi_lcfg_access},
  {M2M2_APP_COMMON_CMD_READ_LCFG_REQ, sqi_lcfg_access},
  {M2M2_SQI_APP_CMD_SET_SLOT_REQ, sqi_set_adpd_slot},
};

#define SQI_APP_ROUTING_TBL_SZ (sizeof(sqi_routing_table) / sizeof(sqi_routing_table[0]))

SQI_ALG_RETURN_CODE_t sqi_algo_init(uint8_t input_sample_freq);

void send_message_sqi_app_task(m2m2_hdr_t *p_pkt) {
  ADI_OSAL_STATUS osal_result;
  osal_result = adi_osal_MsgQueuePost(sqi_task_msg_queue,p_pkt);
  if (osal_result != ADI_OSAL_SUCCESS)
    post_office_consume_msg(p_pkt);
  adi_osal_SemPost(sqi_task_evt_sem);
}

void sqi_app_task_init(void) {
  ADI_OSAL_STATUS eOsStatus = ADI_OSAL_SUCCESS;
  sqi_task_attributes.pThreadFunc = sqi_task;
  sqi_task_attributes.nPriority = APP_OS_CFG_SQI_APP_TASK_PRIO;
  sqi_task_attributes.pStackBase = &sqi_task_stack[0];
  sqi_task_attributes.nStackSize = APP_OS_CFG_SQI_APP_TASK_STK_SIZE;
  sqi_task_attributes.pTaskAttrParam = NULL;
  sqi_task_attributes.szThreadName = "SQI";
  sqi_task_attributes.pThreadTcb = &sqi_task_tcb;

  eOsStatus = adi_osal_MsgQueueCreate(&sqi_task_msg_queue,NULL,
                                    5);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
      Debug_Handler();
  }
  else
  {
    update_task_queue_list(APP_OS_CFG_SQI_TASK_INDEX,sqi_task_msg_queue);
  }

  eOsStatus = adi_osal_ThreadCreateStatic(&sqi_task_handler,
                                    &sqi_task_attributes);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
      Debug_Handler();
  }

  eOsStatus = adi_osal_MutexCreate(&g_sqi_data_lock);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
      Debug_Handler();
  }

  eOsStatus = adi_osal_SemCreate(&sqi_task_evt_sem, 0U);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
      Debug_Handler();
  }

}

  uint32_t pn_RData=0;
  uint32_t  nTimestamp = 0;
static void sqi_task(void *pArgument) {
  m2m2_hdr_t *p_in_pkt = NULL;
  m2m2_hdr_t *p_out_pkt = NULL;
  ADI_OSAL_STATUS         err;

  post_office_add_mailbox(M2M2_ADDR_MED_SQI, M2M2_ADDR_MED_SQI_STREAM);
  while (1)
  {
      adi_osal_SemPend(sqi_task_evt_sem, ADI_OSAL_TIMEOUT_FOREVER);
      p_in_pkt = post_office_get(ADI_OSAL_TIMEOUT_NONE, APP_OS_CFG_SQI_TASK_INDEX);
      if (p_in_pkt == NULL) {
        // No m2m2 messages to process, so fetch some data from the device.
          uint32_t nSQIData = pn_RData;
          packetize_sqi_data(nSQIData, nTimestamp);
      } else {
        // We got an m2m2 message from the queue, process it.
       PYLD_CST(p_in_pkt, _m2m2_app_common_cmd_t, p_in_cmd);
       // Look up the appropriate function to call in the function table
        for (int i = 0; i < SQI_APP_ROUTING_TBL_SZ; i++) {
          if (sqi_routing_table[i].command == p_in_cmd->command) {
            p_out_pkt = sqi_routing_table[i].cb_handler(p_in_pkt);
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

#ifdef PROFILE_SQI
#define SQI_TIME_WINDOW 5.12f
uint16_t gSqiSampleCnt = 0, gSqiReqSampleCnt = 0;
uint32_t gSqiStartTime = 0, gSqiEndTime = 0, gSqiTotalTime = 0;
#endif
static void packetize_sqi_data(uint32_t nSQIData, uint32_t nTimeStamp) {
  m2m2_hdr_t *resp_pkt;
  ADI_OSAL_STATUS err;
  adi_vsm_sqi_output_t sqi_result;
  SQI_ALG_RETURN_CODE_t alg_ret_code = SQI_ALG_SUCCESS;
  SQI_ERROR_CODE_t sqi_ret_code = SQI_SUCCESS;

  memset(&sqi_result, 0x00, sizeof(sqi_result));
  if (g_state_sqi.nStreamStartCount > 0) {
#ifdef PROFILE_SQI
      gSqiSampleCnt++;
      if(gSqiSampleCnt == gSqiReqSampleCnt)
      {
         adi_osal_EnterCriticalRegion();
#ifdef PROFILE_TIME_ENABLED
         gSqiStartTime = get_micro_sec();
#else
         gSqiStartTime = MCU_HAL_GetTick();
#endif
      }
#endif
      alg_ret_code = SqiAlgProcess(nSQIData, &sqi_result);
#ifdef PROFILE_SQI
     if(gSqiSampleCnt == gSqiReqSampleCnt)
      {
#ifdef PROFILE_TIME_ENABLED
         gSqiEndTime = get_micro_sec();
#else
         gSqiEndTime = MCU_HAL_GetTick();
#endif
         gSqiTotalTime = gSqiEndTime - gSqiStartTime;
         gSqiSampleCnt = 0;
         adi_osal_ExitCriticalRegion();
      }
#endif
      switch(alg_ret_code){
        case SQI_ALG_BUFFERING:
         {
           return;
         }
        case SQI_ALG_SUCCESS:
        {
          sqi_ret_code = SQI_SUCCESS;
          break;
        }
        case SQI_ALG_NULL_PTR_ERROR:
        {
         sqi_result.sqi = -1;
         sqi_ret_code = SQI_NULL_PTR_ERROR;
         break;
        }
        case SQI_ALG_DIV_BY_ZERO_ERROR:
        {
         sqi_result.sqi = -1;
         sqi_ret_code = SQI_DIV_BY_ZERO_ERROR;
         break;
        }
        default:
        {
          sqi_result.sqi = -1;
          sqi_ret_code = SQI_ERROR;
          NRF_LOG_INFO("SQI algo: Received error from Algo");
          break;
        }
      }
      if (g_state_sqi.nSubscriberCount > 0) {
       adi_osal_EnterCriticalRegion();
       resp_pkt = post_office_create_msg(M2M2_HEADER_SZ + sizeof(sqi_app_stream_t));
       if(resp_pkt != NULL)
       {
#ifdef DEBUG_PKT
         g_sqi_pkt_cnt++;
#endif
         sqi_app_stream_t resp_sqi;
         resp_pkt->src = M2M2_ADDR_MED_SQI;
         resp_pkt->dest = M2M2_ADDR_MED_SQI_STREAM;
         resp_pkt->length = M2M2_HEADER_SZ + sizeof(sqi_app_stream_t);
         resp_sqi.command = (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_SENSOR_COMMON_CMD_STREAM_DATA;
         resp_sqi.sequence_num = g_state_sqi.nSequenceCount++;
         resp_sqi.status = M2M2_APP_COMMON_STATUS_OK;
         resp_sqi.sqi = sqi_result.sqi;
         resp_sqi.nAlgoStatus = sqi_ret_code;
         resp_sqi.nTimeStamp = nTimeStamp;
         resp_sqi.nReserved = 0;  // not used now, hence set to zero
         memcpy(&resp_pkt->data[0], &resp_sqi,  sizeof(sqi_app_stream_t));
         post_office_send(resp_pkt,&err);
        }
        adi_osal_ExitCriticalRegion();
      }
  }
}

static m2m2_hdr_t *sqi_get_version(m2m2_hdr_t *p_pkt) {
    // Declare a pointer to access the input packet payload
  PYLD_CST(p_pkt, m2m2_app_common_sub_op_t, p_in_payload);

  PKT_MALLOC(p_resp_pkt, m2m2_app_common_version_t, 0);
  // Declare a pointer to the response packet payload
  PYLD_CST(p_resp_pkt, m2m2_app_common_version_t, p_resp_payload);
  p_resp_payload->status = M2M2_APP_COMMON_STATUS_ERROR;
  switch(p_in_payload->command){
  case M2M2_APP_COMMON_CMD_GET_VERSION_REQ:
    p_resp_payload->command = M2M2_APP_COMMON_CMD_GET_VERSION_RESP;
    p_resp_payload->major = SQI_ALGO_MAJOR_VERSION;
    p_resp_payload->minor = SQI_ALGO_MINOR_VERSION;
    p_resp_payload->patch = SQI_ALGO_PATCH_VERSION;
    memcpy(&p_resp_payload->str[0], &GIT_SQI_VERSION, GIT_SQI_VERSION_LEN);
    p_resp_payload->status = M2M2_APP_COMMON_STATUS_OK;
    break;
  case M2M2_SQI_APP_CMD_GET_ALGO_VENDOR_VERSION_REQ: {
    char aNameStr[40];
    p_resp_payload->command = M2M2_SQI_APP_CMD_GET_ALGO_VENDOR_VERSION_RESP;

    p_resp_payload->major = SQI_ALGO_MAJOR_VERSION;
    p_resp_payload->minor = SQI_ALGO_MINOR_VERSION;
    p_resp_payload->patch = SQI_ALGO_PATCH_VERSION;
    memcpy(&p_resp_payload->verstr[0], "-HC",  4);
    strcpy((char *)aNameStr, "ADI SQI");
    memcpy(p_resp_payload->str, aNameStr, sizeof(aNameStr));
    p_resp_payload->status = M2M2_APP_COMMON_STATUS_OK;
    break;
    }
  }
  p_resp_pkt->src = p_pkt->dest;
  p_resp_pkt->dest = p_pkt->src;
  p_resp_pkt->checksum = 0x0000;

  return p_resp_pkt;
}

static m2m2_hdr_t *sqi_status(m2m2_hdr_t *p_pkt) {
  // Declare and malloc a response packet
  PKT_MALLOC(p_resp_pkt, m2m2_app_common_status_t, 0);
  // Declare a pointer to the response packet payload
  PYLD_CST(p_resp_pkt, m2m2_app_common_status_t, p_resp_payload);

  p_resp_payload->command = M2M2_APP_COMMON_CMD_SENSOR_STATUS_QUERY_RESP;
  p_resp_payload->status = M2M2_APP_COMMON_STATUS_ERROR;

  if (g_state_sqi.nStreamStartCount == 0) {
    p_resp_payload->status = M2M2_APP_COMMON_STATUS_STREAM_STOPPED;
  } else {
    p_resp_payload->status = M2M2_APP_COMMON_STATUS_STREAM_STARTED;
  }
  p_resp_payload->stream = M2M2_ADDR_MED_SQI_STREAM;
  p_resp_payload->num_subscribers = g_state_sqi.nSubscriberCount;
  p_resp_payload->num_start_reqs = g_state_sqi.nStreamStartCount;
  p_resp_pkt->src = p_pkt->dest;
  p_resp_pkt->dest = p_pkt->src;
  return p_resp_pkt;
}

static m2m2_hdr_t *sqi_stream_config(m2m2_hdr_t *p_pkt) {
  M2M2_APP_COMMON_STATUS_ENUM_t status = M2M2_APP_COMMON_STATUS_ERROR;
  M2M2_APP_COMMON_CMD_ENUM_t    command;
  SQI_ALG_RETURN_CODE_t ret_code;

  // Declare a pointer to access the input packet payload
  PYLD_CST(p_pkt, m2m2_app_common_sub_op_t, p_in_payload);
  // Declare and malloc a response packet
  PKT_MALLOC(p_resp_pkt, m2m2_app_common_sub_op_t, 0);
  // Declare a pointer to the response packet payload
  PYLD_CST(p_resp_pkt, m2m2_app_common_sub_op_t, p_resp_payload);

  switch (p_in_payload->command) {
  case M2M2_APP_COMMON_CMD_STREAM_START_REQ:
    if(gAdpd_ext_data_stream_active)
    {
      g_state_sqi.nSQIODR = gAdpd_ext_data_stream_odr;
    }
    else
    {
      Adpd400xDrvGetParameter(ADPD400x_OUTPUTDATARATE, 0, &g_state_sqi.nSQIODR);
    }
#ifdef PROFILE_SQI
    gSqiSampleCnt = 0;
    gSqiReqSampleCnt = g_state_sqi.nSQIODR * SQI_TIME_WINDOW;
#endif
    if (g_state_sqi.nStreamStartCount == 0) {
      if( SQI_ALG_SUCCESS == sqi_algo_init(g_state_sqi.nSQIODR))
      {
        g_state_sqi.nStreamStartCount = 1;
        status = M2M2_APP_COMMON_STATUS_STREAM_STARTED;
      }
      else
      {
        status = M2M2_APP_COMMON_STATUS_STREAM_NOT_STARTED;
      }
    }
    else
    {
      g_state_sqi.nStreamStartCount++;
      status = M2M2_APP_COMMON_STATUS_STREAM_IN_PROGRESS;
    }
    command = M2M2_APP_COMMON_CMD_STREAM_START_RESP;
    break;
  case M2M2_APP_COMMON_CMD_STREAM_STOP_REQ:
    if (g_state_sqi.nStreamStartCount == 0)
    {
      status = M2M2_APP_COMMON_STATUS_STREAM_STOPPED;
    }
    else if (g_state_sqi.nStreamStartCount == 1)
    {
        ret_code = SqiAlgReset();
        g_state_sqi.nStreamStartCount = 0;
        g_state_sqi.nSQIODR = 0;
        g_state_sqi.nSequenceCount = 0;
        if(gAdpd_ext_data_stream_active)
          gAdpd_ext_data_stream_active = false;
        if(SQI_ALG_SUCCESS == ret_code)
          status = M2M2_APP_COMMON_STATUS_STREAM_STOPPED;
    }
    else
    {
      g_state_sqi.nStreamStartCount--;
      status = M2M2_APP_COMMON_STATUS_STREAM_COUNT_DECREMENT;
    }
    command = M2M2_APP_COMMON_CMD_STREAM_STOP_RESP;
    break;
  case M2M2_APP_COMMON_CMD_STREAM_SUBSCRIBE_REQ:
    sqi_event = 1;
    g_state_sqi.nSubscriberCount++;
    //gnSQI_Slot = 0x20; //slot-F
    post_office_setup_subscriber(M2M2_ADDR_MED_SQI, M2M2_ADDR_MED_SQI_STREAM, p_pkt->src, true);
    status = M2M2_APP_COMMON_STATUS_SUBSCRIBER_ADDED;
    command = M2M2_APP_COMMON_CMD_STREAM_SUBSCRIBE_RESP;
    break;
  case M2M2_APP_COMMON_CMD_STREAM_UNSUBSCRIBE_REQ:
    if (g_state_sqi.nSubscriberCount <= 1)
    {
      sqi_event = 0;
      g_state_sqi.nSubscriberCount = 0;
      gnSQI_Slot = 0;
      status = M2M2_APP_COMMON_STATUS_SUBSCRIBER_REMOVED;
    }
    else
    {
      g_state_sqi.nSubscriberCount--;
      status = M2M2_APP_COMMON_STATUS_SUBSCRIBER_COUNT_DECREMENT;
    }
    post_office_setup_subscriber(M2M2_ADDR_MED_SQI, M2M2_ADDR_MED_SQI_STREAM, p_pkt->src, false);
    command = M2M2_APP_COMMON_CMD_STREAM_UNSUBSCRIBE_RESP;
    break;
  default:
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

/**
  * @internal
  * @brief    Example of how to write an LCFG parameter
  * @param    LCFG field that has to be written
  * @param    Value to be written
  * @retval   SQI_STATUS_ENUM_t
  */

SQI_ERROR_CODE_t sqi_writeLCFG(uint8_t field, uint16_t value) {
  if(field < SQI_LCFG_MAX){
    switch(field){
    }
    return SQI_SUCCESS;
  }
  return SQI_ERROR;
}

/**
  * @internal
  * @brief    Read LCFG parameter
  * @param    LCFG field
  * @param    Returned corresponding LCFG value
  * @retval   SQI_STATUS_ENUM_t
  */
SQI_ERROR_CODE_t sqi_readLCFG(uint8_t index, uint16_t *value) {
    if(index < SQI_LCFG_MAX){
        switch(index){
        }
        return SQI_SUCCESS;
    }
    return SQI_ERROR;
}

static m2m2_hdr_t *sqi_lcfg_access(m2m2_hdr_t *p_pkt) {
    M2M2_APP_COMMON_STATUS_ENUM_t status = M2M2_APP_COMMON_STATUS_ERROR;
    // Declare a pointer to access the input packet payload
    PYLD_CST(p_pkt, sqi_app_lcfg_op_hdr_t, p_in_payload);
    // Allocate a response packet with space for the correct number of operations
    PKT_MALLOC(p_resp_pkt, sqi_app_lcfg_op_hdr_t, p_in_payload->num_ops * sizeof(p_in_payload->ops[0]));
    PYLD_CST(p_resp_pkt, sqi_app_lcfg_op_hdr_t, p_resp_payload);
    uint16_t  reg_data = 0;

    switch (p_in_payload->command) {
    case M2M2_APP_COMMON_CMD_READ_LCFG_REQ:
      for (int i = 0; i < p_in_payload->num_ops; i++) {
        if (sqi_readLCFG(p_in_payload->ops[i].field, &reg_data) == SQI_SUCCESS)
        {
          status = M2M2_APP_COMMON_STATUS_OK;
        }
        else
        {
          status = M2M2_APP_COMMON_STATUS_ERROR;
        }
        p_resp_payload->ops[i].field = p_in_payload->ops[i].field;
        p_resp_payload->ops[i].value = reg_data;
      }
      p_resp_payload->command = (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_APP_COMMON_CMD_READ_LCFG_RESP;
      break;
    case M2M2_APP_COMMON_CMD_WRITE_LCFG_REQ:
      for (int i = 0; i < p_in_payload->num_ops; i++) {
        if (sqi_writeLCFG(p_in_payload->ops[i].field, p_in_payload->ops[i].value) == SQI_SUCCESS)
        {
          status = M2M2_APP_COMMON_STATUS_OK;
        }
        else
        {
          status = M2M2_APP_COMMON_STATUS_ERROR;
        }
        p_resp_payload->ops[i].field = p_in_payload->ops[i].field;
        p_resp_payload->ops[i].value = p_in_payload->ops[i].value;
      }
      p_resp_payload->command = (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_APP_COMMON_CMD_WRITE_LCFG_RESP;
      break;
    default:
      // Something has gone horribly wrong.
      post_office_consume_msg(p_resp_pkt);
      return NULL;
     }
    p_resp_pkt->dest = p_pkt->src;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_payload->num_ops = p_in_payload->num_ops;
    p_resp_payload->status = status;
    return p_resp_pkt;
}

static m2m2_hdr_t *sqi_set_adpd_slot(m2m2_hdr_t *p_pkt) {
    M2M2_APP_COMMON_STATUS_ENUM_t status = M2M2_APP_COMMON_STATUS_ERROR;
    // Declare a pointer to access the input packet payload
    PYLD_CST(p_pkt, sqi_app_set_slot_t, p_in_payload);
    // Allocate a response packet with space for the correct number of operations
    PKT_MALLOC(p_resp_pkt, sqi_app_set_slot_t, 0);
    PYLD_CST(p_resp_pkt, sqi_app_set_slot_t, p_resp_payload);

    switch (p_in_payload->command) {
    case M2M2_SQI_APP_CMD_SET_SLOT_REQ:
      gnSQI_Slot = p_in_payload->nSQISlot;
      status = M2M2_APP_COMMON_STATUS_OK;
      p_resp_payload->command = M2M2_SQI_APP_CMD_SET_SLOT_RESP;
      break;
    default:
      // Something has gone horribly wrong.
      post_office_consume_msg(p_resp_pkt);
      return NULL;
     }
    p_resp_pkt->dest = p_pkt->src;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_payload->nSQISlot = p_in_payload->nSQISlot;
    p_resp_payload->status = status;
    return p_resp_pkt;
}

void send_sqi_app_data(uint32_t *p_adpd_data, uint32_t ts)
{
   pn_RData = *p_adpd_data;
   nTimestamp = ts;
   adi_osal_SemPost(sqi_task_evt_sem);
}

uint32_t gStateMem = 0;
/***************************SQI configurations****************************************************/
/* Allocate a max amount of memory for the SQI Algo state memory block */
#define STATE_SQI_MEM_NUM_CHARS  3816 /* actual 3815 */

unsigned char STATE_memory_SQI[STATE_SQI_MEM_NUM_CHARS];

adi_vsm_sqi_mem_t sqi_memory_setup;

adi_vsm_sqi_instance_t* sqi_instance = NULL;

adi_vsm_sqi_config_t config_handle;

adi_vsm_sqi_output_t adi_vsm_sqi_output;

#ifdef PROFILE_TIME
#include "adi_vsm_cycle_count.h"
volatile uint32_t gn_sqi_algo_cycle_cnt=0;
#endif

SQI_ALG_RETURN_CODE_t SqiAlgConfig(uint8_t input_sample_freq)
{
    if (input_sample_freq != 25 && 
            input_sample_freq != 50 && 
                   input_sample_freq != 100)
    {
        return SQI_ALG_ERROR;
    }
    config_handle.sampling_freq = input_sample_freq;
    return SQI_ALG_SUCCESS;
}

SQI_ALG_RETURN_CODE_t SqiAlgInit(const adi_vsm_sqi_config_t* config_params)
{

    /* initialize the memory object for the SQI instance */
    sqi_memory_setup.state.block = STATE_memory_SQI;
    sqi_memory_setup.state.length_numchars = STATE_SQI_MEM_NUM_CHARS;
    gStateMem = adi_vsm_sqi_numchars_state_memory();
    /* Create the SQI Measurement instance */
    sqi_instance = adi_vsm_sqi_create(&sqi_memory_setup, config_params);
    if (sqi_instance == NULL) {
        return SQI_ALG_ERROR;
    }
    return SQI_ALG_SUCCESS;
}

SQI_ALG_RETURN_CODE_t SqiAlgReset()
{
    if (adi_vsm_sqi_frontend_reset(sqi_instance) != ADI_VSM_SQI_SUCCESS)
    {
        return SQI_ALG_ERROR;
    }
    return SQI_ALG_SUCCESS;
}

SQI_ALG_RETURN_CODE_t SqiAlgProcess(float input_sample,
                                    adi_vsm_sqi_output_t* adi_vsm_sqi_output)
{
    adi_vsm_sqi_return_code_t ret_code;
#ifdef PROFILE_TIME
    uint32_t sqi_algo_start_cnt = 0, sqi_algo_stop_cnt = 0;
    static uint8_t algo_status = 0;//not_done
    if(algo_status == 0)
    {
      sqi_algo_start_cnt = adi_GetStartCycleCount();
      algo_status = 1; //in progress
    }
#endif //#ifdef PROFILE_TIME
    ret_code = adi_vsm_sqi_process(sqi_instance, input_sample, adi_vsm_sqi_output);
    switch (ret_code)
    {
    case ADI_VSM_SQI_BUFFERING:
    {
        return SQI_ALG_BUFFERING;
    }
    case ADI_VSM_SQI_NULL_PTR_ERROR:
    {
        return SQI_ALG_NULL_PTR_ERROR;
    }
    case ADI_VSM_SQI_DIV_BY_ZERO_ERROR:
    {
        return SQI_ALG_DIV_BY_ZERO_ERROR;
    }
    case ADI_VSM_SQI_SUCCESS:
    {
#ifdef PROFILE_TIME
        if(algo_status == 1)
        {
          algo_status = 0; //not done
          sqi_algo_stop_cnt = adi_GetEndCycleCount();
          gn_sqi_algo_cycle_cnt = sqi_algo_stop_cnt - sqi_algo_start_cnt;
        }
#endif //#ifdef PROFILE_TIME
        return SQI_ALG_SUCCESS;
    }
    default:
    {
        return SQI_ALG_ERROR;
    }
    }
}

SQI_ALG_RETURN_CODE_t sqi_algo_init(uint8_t input_sample_freq)
{
  SQI_ALG_RETURN_CODE_t ret_code;

  ret_code = SqiAlgConfig(input_sample_freq);
  ASSERT(ret_code == SQI_ALG_SUCCESS);

  ret_code = SqiAlgInit(&config_handle);
  ASSERT(ret_code == SQI_ALG_SUCCESS);

  return ret_code;
}
#endif
/******************************************************************************************/