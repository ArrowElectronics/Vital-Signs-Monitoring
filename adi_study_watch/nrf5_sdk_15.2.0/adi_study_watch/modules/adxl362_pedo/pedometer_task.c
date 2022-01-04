/**
*******************************************************************************
* @file         pedometer_task.c
* @author       ADI
* @version      V1.0.0
* @date         17-April-2020
* @brief        Source file contains Pedometer processing wrapper.
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
* This software is intended for use with the pedometer                        *
*                                                                             *
*                                                                             *
******************************************************************************/
#ifdef ENABLE_PEDO_APP
#include <pedometer_application_interface.h>
#include <pedometer_task.h>
#include <task_includes.h>
#include <string.h>
#include <stdint.h>
#include "post_office_interface.h"
#include "m2m2_core.h"
#include "Common.h"
#include "pedometer_lib.h"
#include "Adxl362.h"
#include "adxl_task.h"
#include "adxl_buffering.h"
#include "post_office.h"

#define PED_MAJOR_VERSION     3
#define PED_MINOR_VERSION     5
#define PED_PATCH_VERSION     0

#define PED_APP_MAX_LCFG_OPS (10)

/* -------------------------Public variables -------------------------------*/
uint32_t pedometer_event;
int16_t pnRData[ADXL_MAXDATASETSIZE] = {0,0,0};
uint32_t ntimestamp = 0;
extern g_state_adxl_t  g_state_adxl;

/* ------------------------- Private variables ----------------------------- */
/* Create the stack for task */
uint8_t pedometer_app_task_stack[APP_OS_CFG_PEDOMETER_APP_TASK_STK_SIZE];

/* Create handler for task */
ADI_OSAL_THREAD_HANDLE pedometer_app_task_handler;

/* Create task attributes variable */
ADI_OSAL_STATIC_THREAD_ATTR pedometer_app_task_attributes;

/* Create TCB for task */
StaticTask_t pedometer_app_task_tcb;

/* Create semaphores */
ADI_OSAL_SEM_HANDLE   pedometer_app_task_evt_sem;

/* Create Queue Handler for task */
ADI_OSAL_QUEUE_HANDLE  pedometer_app_task_msg_queue = NULL;

const char GIT_PEDOMETER_VERSION[] = "TEST PEDOMETER_VERSION STRING";
const uint8_t GIT_PEDOMETER_VERSION_LEN = sizeof(GIT_PEDOMETER_VERSION);

typedef  m2m2_hdr_t*(app_cb_function_t)(m2m2_hdr_t*);

/*!
* @brief:  Struture holding pedometer application state info
*/
static struct _g_state_pedometer {
  uint16_t nSubscriberCount;
  uint16_t nSequenceCount;
  uint16_t nStreamStartCount;
  uint16_t  nPktCounter;
  uint16_t nPedometerODR;
} g_state_pedometer = {0, 0, 0, 0, 0};
ped_cfg_t gPedConfiguration = {PED_ADI_ALGO};

typedef struct _app_routing_table_entry_t {
  uint8_t                    command;
  app_cb_function_t          *cb_handler;
}app_routing_table_entry_t;

/* ------------------------- Private Function Prototypes ------------------- */

static m2m2_hdr_t *pedometer_app_stream_config(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *pedometer_app_lcfg_access(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *pedometer_app_status(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *pedometer_app_get_version(m2m2_hdr_t *p_pkt);
static void pedometer_app_task(void *pArgument);
static void packetize_pedometer_app_data(int16_t *pPedometerData, \
                                           uint32_t nTimeStamp);

app_routing_table_entry_t pedometer_app_routing_table[] = {
  {M2M2_APP_COMMON_CMD_STREAM_START_REQ, pedometer_app_stream_config},
  {M2M2_APP_COMMON_CMD_STREAM_STOP_REQ, pedometer_app_stream_config},
  {M2M2_APP_COMMON_CMD_STREAM_SUBSCRIBE_REQ, pedometer_app_stream_config},
  {M2M2_APP_COMMON_CMD_STREAM_UNSUBSCRIBE_REQ, pedometer_app_stream_config},
  {M2M2_APP_COMMON_CMD_SENSOR_STATUS_QUERY_REQ, pedometer_app_status},
  {M2M2_APP_COMMON_CMD_GET_VERSION_REQ, pedometer_app_get_version},
  {M2M2_PED_APP_CMD_GET_ALGO_VENDOR_VERSION_REQ, pedometer_app_get_version},
  {M2M2_APP_COMMON_CMD_WRITE_LCFG_REQ, pedometer_app_lcfg_access},
  {M2M2_APP_COMMON_CMD_READ_LCFG_REQ, pedometer_app_lcfg_access},
};

#define PEDOMETER_APP_ROUTING_TBL_SZ (sizeof(pedometer_app_routing_table) \
                                      / sizeof(pedometer_app_routing_table[0]))

/*!
  ****************************************************************************
* @brief Send messages to Pedometer application, called by the post office
*
* @param[in]           p_pkt: input m2m2 packet
*
* @return              None
*****************************************************************************/
void send_message_pedometer_app_task(m2m2_hdr_t *p_pkt) {
  ADI_OSAL_STATUS osal_result;
  osal_result = adi_osal_MsgQueuePost(pedometer_app_task_msg_queue,p_pkt);
  if (osal_result != ADI_OSAL_SUCCESS)
    post_office_consume_msg(p_pkt);
  adi_osal_SemPost(pedometer_app_task_evt_sem);
}

/*!
  ****************************************************************************
* @brief Pedometer application task initialization
*
* @param[in]           None
*
* @return              None
*****************************************************************************/
void pedometer_app_task_init(void) {
  ADI_OSAL_STATUS eOsStatus = ADI_OSAL_SUCCESS;
  pedometer_app_task_attributes.pThreadFunc = pedometer_app_task;
  pedometer_app_task_attributes.nPriority = APP_OS_CFG_PEDOMETER_APP_TASK_PRIO;
  pedometer_app_task_attributes.pStackBase = &pedometer_app_task_stack[0];
  pedometer_app_task_attributes.nStackSize = \
                                   APP_OS_CFG_PEDOMETER_APP_TASK_STK_SIZE;
  pedometer_app_task_attributes.pTaskAttrParam = NULL;
  /* Thread Name should be of max 10 Characters */
  pedometer_app_task_attributes.szThreadName = "Pedo_Task";
  pedometer_app_task_attributes.pThreadTcb = &pedometer_app_task_tcb;

  eOsStatus = adi_osal_MsgQueueCreate(&pedometer_app_task_msg_queue,NULL,
                                    5);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
      Debug_Handler();
  }
  else
  {
    update_task_queue_list(APP_OS_CFG_PEDOMETER_TASK_INDEX, \
                                      pedometer_app_task_msg_queue);
  }

  eOsStatus = adi_osal_ThreadCreateStatic(&pedometer_app_task_handler,
                                    &pedometer_app_task_attributes);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
      Debug_Handler();
  }

}

/*!
  ****************************************************************************
* @brief - Creates pedometer task semaphore during bootup
*        - Interface for packetizing pedometer data
*
* @param[in]           pArgument not used
*
* @return              None
*****************************************************************************/
static void pedometer_app_task(void *pArgument) {
  ADI_OSAL_STATUS         err;
  adi_osal_SemCreate(&pedometer_app_task_evt_sem, 0);
  post_office_add_mailbox(M2M2_ADDR_MED_PED, M2M2_ADDR_MED_PED_STREAM);
  while (1)
  {
      m2m2_hdr_t *p_in_pkt = NULL;
      m2m2_hdr_t *p_out_pkt = NULL;
      adi_osal_SemPend(pedometer_app_task_evt_sem, ADI_OSAL_TIMEOUT_FOREVER);
      p_in_pkt = post_office_get(ADI_OSAL_TIMEOUT_NONE, \
                                            APP_OS_CFG_PEDOMETER_TASK_INDEX);
      if (p_in_pkt == NULL) {
        /* No m2m2 messages to process, so fetch some data from the device. */
          packetize_pedometer_app_data(pnRData, ntimestamp);
      } else {
        /* We got an m2m2 message from the queue, process it. */
       PYLD_CST(p_in_pkt, _m2m2_app_common_cmd_t, p_in_cmd);
       /* Look up the appropriate function to call in the function table */
        for (int i = 0; i < PEDOMETER_APP_ROUTING_TBL_SZ; i++) {
          if (pedometer_app_routing_table[i].command == p_in_cmd->command) {
            p_out_pkt = pedometer_app_routing_table[i].cb_handler(p_in_pkt);
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
* @brief Packetize pedometer app packet
*
* @param[in]           pPedometerData: Input pedometer Data
* @param[in]           nTimeStamp: timestamp
*
* @return              None
*****************************************************************************/
static void packetize_pedometer_app_data(int16_t *pPedometerData, \
                                                     uint32_t nTimeStamp) {
  m2m2_hdr_t *resp_pkt;
  ADI_OSAL_STATUS err;
  PedometerResult_t pedometer_result;
  if (pPedometerData == NULL) {
    return;
  }
  if (g_state_pedometer.nPedometerODR < 25) {
    pedometer_result.nPedometerSteps = 0;
    pedometer_result.nPedometerStatus = 0xFFFF;
    pedometer_result.nTimeStamp = 0; /* default value */
    return;
  }
  memset(&pedometer_result, 0x00, sizeof(pedometer_result));
  if (g_state_pedometer.nStreamStartCount > 0) {
      PedometerGetStep(&pedometer_result, pPedometerData);
      pedometer_result.nTimeStamp = nTimeStamp;
    if (g_state_pedometer.nSubscriberCount > 0) {
      /* the packets are send one a second, hence the
        counter is set to the sampling frequency */
      if (++g_state_pedometer.nPktCounter >= g_state_pedometer.nPedometerODR) {
        resp_pkt = post_office_create_msg(M2M2_HEADER_SZ + \
                                               sizeof(pedometer_app_stream_t));
        pedometer_app_stream_t resp_pedometer;
        if(resp_pkt != NULL)
        {
          resp_pkt->src = M2M2_ADDR_MED_PED;
          resp_pkt->dest = M2M2_ADDR_MED_PED_STREAM;
          resp_pkt->length = M2M2_HEADER_SZ + sizeof(pedometer_app_stream_t);
          resp_pedometer.command = \
                  (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_SENSOR_COMMON_CMD_STREAM_DATA;
          resp_pedometer.sequence_num = g_state_pedometer.nSequenceCount++;
          resp_pedometer.status = M2M2_APP_COMMON_STATUS_OK;
          resp_pedometer.nNumSteps = pedometer_result.nPedometerSteps;
          resp_pedometer.nAlgoStatus = pedometer_result.nPedometerStatus;
          resp_pedometer.nTimeStamp = pedometer_result.nTimeStamp;
          resp_pedometer.nReserved = 0;  /* not used now, hence set to zero */
          memcpy(&resp_pkt->data[0], &resp_pedometer, \
                                               sizeof(pedometer_app_stream_t));
          post_office_send(resp_pkt,&err);
        }
        g_state_pedometer.nPktCounter = 0;
      }
    }
  }
}

/*!
  ****************************************************************************
* @brief Get Pedometer app version
*
* @param[in]           p_pkt: input m2m2 packet
*
* @return              pointer to reponse m2m2 packet
*****************************************************************************/
static m2m2_hdr_t *pedometer_app_get_version(m2m2_hdr_t *p_pkt) {
  /* Declare a pointer to access the input packet payload */
  PYLD_CST(p_pkt, m2m2_app_common_sub_op_t, p_in_payload);

  PKT_MALLOC(p_resp_pkt, m2m2_app_common_version_t, 0);
  if(NULL != p_resp_pkt)
  {
    // Declare a pointer to the response packet payload
    PYLD_CST(p_resp_pkt, m2m2_app_common_version_t, p_resp_payload);
    p_resp_payload->status = M2M2_APP_COMMON_STATUS_ERROR;
    switch(p_in_payload->command){
    case M2M2_APP_COMMON_CMD_GET_VERSION_REQ:
      p_resp_payload->command = M2M2_APP_COMMON_CMD_GET_VERSION_RESP;
      p_resp_payload->major = PED_MAJOR_VERSION;
      p_resp_payload->minor = PED_MINOR_VERSION;
      p_resp_payload->patch = PED_PATCH_VERSION;
      memcpy(&p_resp_payload->verstr[0], "Pedometer",  10);
      memcpy(&p_resp_payload->str[0], &GIT_PEDOMETER_VERSION, GIT_PEDOMETER_VERSION_LEN);
      p_resp_payload->status = M2M2_APP_COMMON_STATUS_OK;
      break;
    case M2M2_PED_APP_CMD_GET_ALGO_VENDOR_VERSION_REQ: {
      char aNameStr[40];
      p_resp_payload->command = M2M2_PED_APP_CMD_GET_ALGO_VENDOR_VERSION_RESP;

      p_resp_payload->major = PED_ALGO_MAJOR_VERSION;
      p_resp_payload->minor = PED_ALGO_MINOR_VERSION;
      p_resp_payload->patch = PED_ALGO_PATCH_VERSION;
      memcpy(&p_resp_payload->verstr[0], "-HC",  4);
      strcpy((char *)aNameStr, "ADI PedometerH");
      memcpy(p_resp_payload->str, aNameStr, sizeof(aNameStr));
      p_resp_payload->status = M2M2_APP_COMMON_STATUS_OK;
      break;
      }
    }
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
    p_resp_pkt->checksum = 0x0000;
  }
  return p_resp_pkt;
}

/*!
  ****************************************************************************
* @brief Get pedometer app status
*
* @param[in]           p_pkt: input m2m2 packet
*
* @return              pointer to reponse m2m2 packet
*****************************************************************************/
static m2m2_hdr_t *pedometer_app_status(m2m2_hdr_t *p_pkt) {
  /* Declare and malloc a response packet */
  PKT_MALLOC(p_resp_pkt, m2m2_app_common_status_t, 0);
  if(NULL != p_resp_pkt)
  {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_app_common_status_t, p_resp_payload);

    p_resp_payload->command = M2M2_APP_COMMON_CMD_SENSOR_STATUS_QUERY_RESP;
    p_resp_payload->status = M2M2_APP_COMMON_STATUS_ERROR;

    if (g_state_pedometer.nStreamStartCount == 0) {
      p_resp_payload->status = M2M2_APP_COMMON_STATUS_STREAM_STOPPED;
    } else {
      p_resp_payload->status = M2M2_APP_COMMON_STATUS_STREAM_STARTED;
    }
    p_resp_payload->stream = M2M2_ADDR_MED_PED_STREAM;
    p_resp_payload->num_subscribers = g_state_pedometer.nSubscriberCount;
    p_resp_payload->num_start_reqs = g_state_pedometer.nStreamStartCount;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  }
  return p_resp_pkt;
}

/*!
  ****************************************************************************
* @brief Interface for m2m2 cmd request and response to Start/Stop pedomter
         stream and Add/Remove Subscription to it
*
* @param[in]           p_pkt: input m2m2 packet
*
* @return              pointer to reponse m2m2 packet
*****************************************************************************/
static m2m2_hdr_t *pedometer_app_stream_config(m2m2_hdr_t *p_pkt) {
  M2M2_APP_COMMON_STATUS_ENUM_t status = M2M2_APP_COMMON_STATUS_ERROR;
  M2M2_APP_COMMON_CMD_ENUM_t    command;

  /* Declare a pointer to access the input packet payload */
  PYLD_CST(p_pkt, m2m2_app_common_sub_op_t, p_in_payload);
  /* Declare and malloc a response packet */
  PKT_MALLOC(p_resp_pkt, m2m2_app_common_sub_op_t, 0);
  if(NULL != p_resp_pkt)
  {
  /* Declare a pointer to the response packet payload */
  PYLD_CST(p_resp_pkt, m2m2_app_common_sub_op_t, p_resp_payload);

  switch (p_in_payload->command) {
  case M2M2_APP_COMMON_CMD_STREAM_START_REQ:
    g_state_pedometer.nPedometerODR = PedometerGetDataRate();
    if (g_state_pedometer.nStreamStartCount == 0) {
      if (PedometerOpenStep() == PEDOMETER_SUCCESS)
      {
        g_state_pedometer.nStreamStartCount = 1;
        g_state_pedometer.nPktCounter = 0;
        status = M2M2_APP_COMMON_STATUS_STREAM_STARTED;
      }
      else
      {
        status = M2M2_APP_COMMON_STATUS_STREAM_NOT_STARTED;
      }
    }
    else
    {
      g_state_pedometer.nStreamStartCount++;
      status = M2M2_APP_COMMON_STATUS_STREAM_IN_PROGRESS;
    }
    command = M2M2_APP_COMMON_CMD_STREAM_START_RESP;
    break;
  case M2M2_APP_COMMON_CMD_STREAM_STOP_REQ:
    if (g_state_pedometer.nStreamStartCount == 0)
    {
      status = M2M2_APP_COMMON_STATUS_STREAM_STOPPED;
    }
    else if (g_state_pedometer.nStreamStartCount == 1)
    {
        g_state_pedometer.nStreamStartCount = 0;
        status = M2M2_APP_COMMON_STATUS_STREAM_STOPPED;
    }
    else
    {
      g_state_pedometer.nStreamStartCount--;
      status = M2M2_APP_COMMON_STATUS_STREAM_COUNT_DECREMENT;
    }
    command = M2M2_APP_COMMON_CMD_STREAM_STOP_RESP;
    break;
  case M2M2_APP_COMMON_CMD_STREAM_SUBSCRIBE_REQ:
    pedometer_event = 1;
    g_state_pedometer.nSubscriberCount++;
    if(g_state_pedometer.nSubscriberCount == 1)
    {
       /* reset pkt sequence no. only during 1st sub request */
       g_state_pedometer.nSequenceCount = 0;
    }
    post_office_setup_subscriber(M2M2_ADDR_MED_PED, M2M2_ADDR_MED_PED_STREAM, \
                                                              p_pkt->src, true);
    status = M2M2_APP_COMMON_STATUS_SUBSCRIBER_ADDED;
    command = M2M2_APP_COMMON_CMD_STREAM_SUBSCRIBE_RESP;
    break;
  case M2M2_APP_COMMON_CMD_STREAM_UNSUBSCRIBE_REQ:
    if (g_state_pedometer.nSubscriberCount <= 1)
    {
      pedometer_event = 0;
      g_state_pedometer.nSubscriberCount = 0;
      status = M2M2_APP_COMMON_STATUS_SUBSCRIBER_REMOVED;
    }
    else
    {
      g_state_pedometer.nSubscriberCount--;
      status = M2M2_APP_COMMON_STATUS_SUBSCRIBER_COUNT_DECREMENT;
    }
    post_office_setup_subscriber(M2M2_ADDR_MED_PED, M2M2_ADDR_MED_PED_STREAM, \
                                                             p_pkt->src, false);
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
  }
  return p_resp_pkt;
}

/*!
  ****************************************************************************
* @brief Write Pedometer LCFG parameter
*
* @param[in]           index: Specifying LCFG field to be written
* @param[in]           value: LCFG value to be written
*
* @return              return status
*****************************************************************************/
PED_ERROR_CODE_t PedWriteLCFG(uint8_t field, uint16_t value) {
  if(field < PED_LCFG_MAX){
    switch(field){
    }
    return PED_SUCCESS;
  }
  return PED_ERROR;
}

/*!
  ****************************************************************************
* @brief Read Pedometer LCFG parameter
*
* @param[in]           index: Specifying LCFG field to be read
* @param[in]           value: LCFG value to be read
*
* @return              return status
*****************************************************************************/
PED_ERROR_CODE_t PedReadLCFG(uint8_t index, uint16_t *value) {
if(index < PED_LCFG_MAX){
    switch(index){
    }
    return PED_SUCCESS;
  }
  return PED_ERROR;
}

/*!
  ****************************************************************************
* @brief Read/Write pedometer LCFG values based on received m2m2 pkt
*        and send the response m2m2 pkt
*
* @param[in]           p_pkt: input m2m2 packet
*
* @return              pointer to reponse m2m2 packet
*****************************************************************************/
static m2m2_hdr_t *pedometer_app_lcfg_access(m2m2_hdr_t *p_pkt) {
  M2M2_APP_COMMON_STATUS_ENUM_t status = M2M2_APP_COMMON_STATUS_ERROR;
  /* Declare a pointer to access the input packet payload */
  PYLD_CST(p_pkt, ped_app_lcfg_op_hdr_t, p_in_payload);
  /* Allocate a response packet with space for the correct number of operations */
  PKT_MALLOC(p_resp_pkt, ped_app_lcfg_op_hdr_t, \
		            p_in_payload->num_ops * sizeof(p_in_payload->ops[0]));
  if(NULL != p_resp_pkt)
  {
  PYLD_CST(p_resp_pkt, ped_app_lcfg_op_hdr_t, p_resp_payload);
  uint16_t  reg_data = 0;

  switch (p_in_payload->command) {
  case M2M2_APP_COMMON_CMD_READ_LCFG_REQ:
    for (int i = 0; i < p_in_payload->num_ops; i++) {
      if (PedReadLCFG(p_in_payload->ops[i].field, &reg_data) == PED_SUCCESS)
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
    p_resp_payload->command = \
              (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_APP_COMMON_CMD_READ_LCFG_RESP;
    break;
  case M2M2_APP_COMMON_CMD_WRITE_LCFG_REQ:
    for (int i = 0; i < p_in_payload->num_ops; i++) {
      if (PedWriteLCFG(p_in_payload->ops[i].field, \
                                 p_in_payload->ops[i].value) == PED_SUCCESS)
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
    p_resp_payload->command = \
             (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_APP_COMMON_CMD_WRITE_LCFG_RESP;
    break;
  default:
    /* Invalid cmd */
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
* @brief Send adxl data from adxl task to pedometer task
*
* @param[in]           adxlData: accel values
* @param[in]           ts: timestamp
*
* @return              None
*****************************************************************************/
void send_pedometer_app_data(int16_t *adxlData, uint32_t ts)
{
   for(uint8_t i = 0; i < ADXL_MAXDATASETSIZE; i++)
   {
      pnRData[i] = *adxlData;
      adxlData++;
   }
   ntimestamp = ts;
   adi_osal_SemPost(pedometer_app_task_evt_sem);
}
#endif
