/**
****************************************************************************
* @file     temperature_task.c
* @author   ADI
* @version  V0.1
* @date     04-Dec-2019
* @brief    This is the source file used to measure temperature from adpd4100
****************************************************************************
* @attention
******************************************************************************
* Copyright (c) 2021 Analog Devices, Inc.  All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
* - Redistributions of source code must retain the above copyright notice, this
*   list of conditions and the following disclaimer.
* - Redistributions in binary form must reproduce the above copyright notice,
*   this list of conditions and the following disclaimer in the documentation
*   and/or other materials provided with the distribution.
* - Modified versions of the software must be conspicuously marked as such.
* - This software is licensed solely and exclusively for use with
*   processors/products manufactured by or for Analog Devices, Inc.
* - This software may not be combined or merged with other code in any manner
*   that would cause the software to become subject to terms and conditions
*   which differ from those listed here.
* - Neither the name of Analog Devices, Inc. nor the names of its contributors
*   may be used to endorse or promote products derived from this software
*   without specific prior written permission.
* - The use of this software may or may not infringe the patent rights of one
**   or more patent holders.  This license does not release you from the
*   requirement that you obtain separate licenses from these patent holders to
*   use this software.
*
* THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES, INC. AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
* NONINFRINGEMENT, TITLE, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL ANALOG DEVICES, INC. OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, PUNITIVE OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, DAMAGES ARISING OUT OF
* CLAIMS OF INTELLECTUAL PROPERTY RIGHTS INFRINGEMENT; PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
******************************************************************************/
#ifdef ENABLE_TEMPERATURE_APP
/* -------------------------------- Includes -------------------------------- */
#ifdef EVTBOARD
#include <temperature_task.h>
#include <adpd4000_task.h>
#include "adpd400x_reg.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

/* ---------------------------- Defines -------------------------------------- */
#define   CAL_RESISTANCE            100000.0f   /*!< Calibration resistance value */
#define   ADC_DARK_OFFSET           0x2010u     /*!< output of ADC of ADPD4k when no pulse vtg is applied */
#define   THERM_SENSITIVITY         4400u       /*!< 4.4k for every degree Celsius change in temperature */
#define   THERM_RESISTANCE_AT_25    100000u     /*!< 100k @ 25 degree Celsius */
#define   REFERENCE_TEMPERATURE     25          /*!< Reference Temperature value */
#define   ADC_RESOLUTION            1.02f       /*!< ADC resolution in terms of nA/LSB for the given configuration */
#define   TEMPERATURE_APP_ROUTING_TBL_SZ (sizeof(temperature_app_routing_table) / sizeof(temperature_app_routing_table[0]))


#define LUT_METHOD      /*!< macro for enabling the LUT based temperature calculation */
#define LUT_STEP_CNT    21u /*!< number of entries in LUT */
#define LUT_STEP_SIZE    5u /*!< LUT step size */

/*--------------------------- Typedef ----------------------------------------*/
typedef  m2m2_hdr_t*(app_cb_function_t)(m2m2_hdr_t*);

/*! The dictionary type for mapping addresses to callback handlers */
typedef struct _app_routing_table_entry_t {
  uint8_t                    command;      /*!< command for the application */
  app_cb_function_t          *cb_handler;  /*!< function for the above command */
}app_routing_table_entry_t;

/*------------------------------ Public Variable -----------------------------*/
uint32_t  gnTemperature_Slot;
#ifdef DEBUG_PKT
uint32_t g_temp_pkt_cnt = 0;
#endif
/*! Look up table for thermistor's resistance based on beta value for
  temperature ranging from 0 to 100 degree celcius in steps of 5 degree celcius*/

uint32_t aTemp_LUT[LUT_STEP_CNT] = {  355600, 271800, 209400, 162500, 127000,
                                      100000, 79230,  63180,  50680,  40900,
                                      33190,  27090,  22220,  18320,  15180,
                                      12640,  10580,  8887,   7500,   6357,
                                      5410
                                   };
extern g_state_t g_state;
volatile uint8_t gb_adpd_raw_start_temp = 1; /* Flag to handle whether ADPD sensor was 'start'ed to get raw data(CLI_USB/CLI_BLE) or if it was started by internal applications like Temp */
extern slot_led_reg_t led_reg;
extern uint8_t gsOneTimeValueWhenReadAdpdData;
extern uint32_t gsTempSampleCount;
extern uint16_t g_adpd_odr;
/*------------------------------ Public Function Prototype ------------------ */
extern void temperatureAppData (uint32_t *pData);
extern uint16_t get_adpd_odr();
extern void enable_ext_syncmode();
extern void disable_ext_syncmode();
extern void enable_adpd_ext_trigger();
extern void disable_adpd_ext_trigger();
/*---------------------- Private variables -----------------------------------*/

/* Create the stack for task */
uint8_t temperature_app_task_stack[APP_OS_CFG_TEMPERATURE_APP_TASK_STK_SIZE];

/* Create handler for task */
ADI_OSAL_THREAD_HANDLE temperature_app_task_handler;

/* Create task attributes variable */
ADI_OSAL_STATIC_THREAD_ATTR temperature_app_task_attributes;

/* Create TCB for task */
StaticTask_t temperature_app_task_tcb;

/* Create semaphores */
ADI_OSAL_SEM_HANDLE   temperature_app_task_evt_sem;

/* Create Queue Handler for task */
ADI_OSAL_QUEUE_HANDLE  temperature_app_task_msg_queue = NULL;

/* TEMPERATURE VERSION STRING variable*/
const char GIT_TEMPERATURE_VERSION[] = "TEST TEMPERATURE_VERSION STRING";

/* TEMPERATURE VERSION STRING size variable*/
const uint8_t GIT_TEMPERATURE_VERSION_LEN = sizeof(GIT_TEMPERATURE_VERSION);

/* Variable used for storing the pulse voltage applied*/
static volatile float gsPulseVtg = 0;

/* Variable used for storing the resistance measured */
static volatile float gsResistance= 0;

#ifdef LUT_METHOD
/* Variable used for storing the resistance value change per degree celcius*/
static volatile uint32_t gsDeltaResistance;
#endif
static uint16_t g_reg_base;

#ifdef SLOT_SELECT
bool check_temp_slot_set = false;
#endif
/* Variable used for storing the temperature stream subscriber and sequence count */
static uint16_t gsTemperatureSubscribers, gsTemperatureSeqNum;
/* Variable for counting the number of stream start requests*/
uint16_t gsTemperatureStarts;

/* Variables for storing the adpd4k value for thermistor and cal-resistor slots */
static uint32_t gsThermValue = 0, gsCalResValue = 0;

/*------------------------ Private Function Prototype -----------------------*/
static m2m2_hdr_t *temperature_app_stream_config(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *temperature_app_status(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *adpd_app_get_dcfg(m2m2_hdr_t *p_pkt);
static void temperature_app_task(void *pArgument);
static void fetch_temperature_data();

/* Table which maps commands to the callback functions */
app_routing_table_entry_t temperature_app_routing_table[] = {
  {M2M2_APP_COMMON_CMD_STREAM_START_REQ, temperature_app_stream_config},
  {M2M2_APP_COMMON_CMD_STREAM_STOP_REQ, temperature_app_stream_config},
  {M2M2_APP_COMMON_CMD_STREAM_SUBSCRIBE_REQ, temperature_app_stream_config},
  {M2M2_APP_COMMON_CMD_STREAM_UNSUBSCRIBE_REQ, temperature_app_stream_config},
  {M2M2_APP_COMMON_CMD_SENSOR_STATUS_QUERY_REQ, temperature_app_status},
  {M2M2_SENSOR_COMMON_CMD_GET_DCFG_REQ, adpd_app_get_dcfg}
};

/*!****************************************************************************
 * \brief Get ADPD4k DCFG
 *
 * \param[in]           p_pkt: input m2m2 packet
 *
 * \return              pointer to reponse m2m2 packet
 *****************************************************************************/
static m2m2_hdr_t *adpd_app_get_dcfg(m2m2_hdr_t *p_pkt) {
    uint8_t r_size;
    uint8_t  dcfgdata[MAXTXRXDCFGSIZE*8];
    ADI_OSAL_STATUS  err;
    M2M2_APP_COMMON_STATUS_ENUM_t status = M2M2_APP_COMMON_STATUS_ERROR;
  /* Declare and malloc a response packet */
  PKT_MALLOC(p_resp_pkt, m2m2_sensor_dcfg_data_t, 0);

  if (NULL != p_resp_pkt)
  {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_sensor_dcfg_data_t, p_resp_payload);
    memset(dcfgdata, 0, (MAXTXRXDCFGSIZE*8));
    r_size = (uint8_t)(MAXTXRXDCFGSIZE*4); /* Max words that can be read from FDS */
    if (ADPD4000_DCFG_STATUS_OK == read_adpd4000_dcfg((uint32_t *)&dcfgdata[0], &r_size))
    {
      if(r_size > MAXTXRXDCFGSIZE)
      {
        p_resp_payload->command = M2M2_SENSOR_COMMON_CMD_GET_DCFG_RESP;
        p_resp_payload->status = M2M2_APP_COMMON_STATUS_OK;
        p_resp_payload->size = MAXTXRXDCFGSIZE;
        p_resp_payload->num_tx_pkts = 1;

        memcpy(&p_resp_payload->dcfgdata[0] , &dcfgdata[0], sizeof(p_resp_payload->dcfgdata));
        p_resp_pkt->src = p_pkt->dest;
        p_resp_pkt->dest = p_pkt->src;
        NRF_LOG_INFO("1st pkt sz->%d",p_resp_payload->size);
        post_office_send(p_resp_pkt, &err);
        MCU_HAL_Delay(20);

        PKT_MALLOC(p_resp_pkt, m2m2_sensor_dcfg_data_t, 0);
        /* Declare a pointer to the response packet payload */
        PYLD_CST(p_resp_pkt, m2m2_sensor_dcfg_data_t, p_resp_payload);
        memset(p_resp_payload->dcfgdata, 0, sizeof(p_resp_payload->dcfgdata));

        p_resp_payload->command =
         (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_SENSOR_COMMON_CMD_GET_DCFG_RESP;
        p_resp_payload->status = M2M2_APP_COMMON_STATUS_OK;
        p_resp_payload->size = r_size - MAXTXRXDCFGSIZE;
        p_resp_payload->num_tx_pkts = 0;

        memcpy(&p_resp_payload->dcfgdata[0] , &dcfgdata[MAXTXRXDCFGSIZE*4], (4*r_size - sizeof(p_resp_payload->dcfgdata)));


        NRF_LOG_INFO("2nd pkt sz->%d",p_resp_payload->size);
        status = M2M2_APP_COMMON_STATUS_OK;
      }
      else
      {
          p_resp_payload->size = (r_size); /* Update payload with actual words read from FDS */
          p_resp_payload->num_tx_pkts = 0;
          memset(p_resp_payload->dcfgdata, 0, sizeof(p_resp_payload->dcfgdata));
          memcpy(&p_resp_payload->dcfgdata[0] , &dcfgdata[0], sizeof(p_resp_payload->dcfgdata));

          status = M2M2_APP_COMMON_STATUS_OK;
          NRF_LOG_INFO("1/1 pkt sz->%d",p_resp_payload->size);
      }
    }
    else
    {
        p_resp_payload->size = 0;
        p_resp_payload->num_tx_pkts = 0;
        status = M2M2_APP_COMMON_STATUS_ERROR;
    }

        p_resp_payload->status = status;
        p_resp_payload->command =
         (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_SENSOR_COMMON_CMD_GET_DCFG_RESP;
        p_resp_pkt->src = p_pkt->dest;
        p_resp_pkt->dest = p_pkt->src;
  }

  return p_resp_pkt;
}

/*!****************************************************************************
* \brief  Returns the Temperature stream status
*
* \param[in]  p_pkt: Pointer to the input m2m2 packet structure
*
* \return     p_resp_pkt: Pointer to the output response m2m2 packet structure
*/
static m2m2_hdr_t *temperature_app_status(m2m2_hdr_t *p_pkt) {
  /* Declare and malloc a response packet */
  PKT_MALLOC(p_resp_pkt, m2m2_app_common_status_t, 0);
  if(NULL != p_resp_pkt)
  {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_app_common_status_t, p_resp_payload);

    p_resp_payload->command = M2M2_APP_COMMON_CMD_SENSOR_STATUS_QUERY_RESP;
    p_resp_payload->status = M2M2_APP_COMMON_STATUS_ERROR;

    if (g_state.num_starts == 0) {
      p_resp_payload->status = M2M2_APP_COMMON_STATUS_STREAM_STOPPED;
    } else {
      p_resp_payload->status = M2M2_APP_COMMON_STATUS_STREAM_STARTED;
    }
    p_resp_payload->stream = M2M2_ADDR_MED_TEMPERATURE_STREAM;
    p_resp_payload->num_subscribers = gsTemperatureSubscribers;
    p_resp_payload->num_start_reqs = gsTemperatureStarts;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  }
  return p_resp_pkt;
}

/*!****************************************************************************
* \brief  Handles start/stop/sub/unsub of temperature data streams commands
*
* \param[in]  p_pkt: Pointer to the input m2m2 packet structure with command
*                    for stream start/stop/sub/unsub operations
*
* \return     p_resp_pkt: Pointer to the output response m2m2 packet structure
*/
static m2m2_hdr_t *temperature_app_stream_config(m2m2_hdr_t *p_pkt) {
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
    if (g_state.num_starts == 0) {
/*        //load_temperature_dcfg();
        //Adpd400xDrvSlotSetup(4,1,0x0004,1);
      //reset_adpd_packetization(); */
#ifndef SLOT_SELECT
  for(uint8_t i=0 ; i<SLOT_NUM ; i++)
  {
    g_reg_base = i * 0x20;
    if(Adpd400xDrvRegRead(ADPD400x_REG_LED_POW12_A + g_reg_base, &led_reg.reg_val[i].reg_pow12) == ADPD400xDrv_SUCCESS)
      {
      Adpd400xDrvRegWrite(ADPD400x_REG_LED_POW12_A + g_reg_base, 0x0);
      }/* disable led for slot-A - I */

     if(Adpd400xDrvRegRead(ADPD400x_REG_LED_POW34_A + g_reg_base, &led_reg.reg_val[i].reg_pow34) == ADPD400xDrv_SUCCESS)
      {
      Adpd400xDrvRegWrite(ADPD400x_REG_LED_POW34_A + g_reg_base, 0x0);
      }/* disable led for slot-> A - I */
   }
#endif
      g_adpd_odr = get_adpd_odr();
      enable_ext_syncmode();
      enable_adpd_ext_trigger(g_adpd_odr);

      gsOneTimeValueWhenReadAdpdData = 0;
      if (ADPD400xDrv_SUCCESS == Adpd400xDrvSetOperationMode(ADPD400xDrv_MODE_SAMPLE))
      {
        g_state.num_starts = 1;
        gb_adpd_raw_start_temp = 0;
        gsTemperatureStarts = 1;
        gsTempSampleCount = 0;
        status = M2M2_APP_COMMON_STATUS_STREAM_STARTED;
      }
      else
      {
        status = M2M2_APP_COMMON_STATUS_STREAM_NOT_STARTED;
      }
    } else {
      g_state.num_starts++;
      gb_adpd_raw_start_temp = 0;
      gsTemperatureStarts++;
      if(gsTemperatureStarts == 1)
      {
      gsTempSampleCount = 0;
      status = M2M2_APP_COMMON_STATUS_STREAM_STARTED;
      }
      else
      {
      status = M2M2_APP_COMMON_STATUS_STREAM_IN_PROGRESS;
      }
    }
    command = M2M2_APP_COMMON_CMD_STREAM_START_RESP;
    break;
  case M2M2_APP_COMMON_CMD_STREAM_STOP_REQ:
    if (g_state.num_starts == 0) {
      status = M2M2_APP_COMMON_STATUS_STREAM_STOPPED;
    } else if (g_state.num_starts == 1) {

      gsOneTimeValueWhenReadAdpdData = 0;
      disable_ext_syncmode();  /*! Disable ext sync mode*/
      g_adpd_odr = get_adpd_odr();
      disable_adpd_ext_trigger(g_adpd_odr);
      if (ADPD400xDrv_SUCCESS == Adpd400xDrvSetOperationMode(ADPD400xDrv_MODE_IDLE)) {
        g_state.num_starts = 0;
        gb_adpd_raw_start_temp = 1;
        gsTemperatureStarts = 0;
        gsTempSampleCount = 0;
        status = M2M2_APP_COMMON_STATUS_STREAM_STOPPED;
        /* reset_adpd_packetization();*/
      } else {
        g_state.num_starts = 1;
        gb_adpd_raw_start_temp = 1;
        gsTemperatureStarts = 1;
        status = M2M2_APP_COMMON_STATUS_ERROR;
      }
#ifndef SLOT_SELECT
  for(uint8_t i=0 ; i<SLOT_NUM ; i++)
  {
    g_reg_base = i * 0x20;
    Adpd400xDrvRegWrite(ADPD400x_REG_LED_POW12_A + g_reg_base, led_reg.reg_val[i].reg_pow12); /* enable led */
    Adpd400xDrvRegWrite(ADPD400x_REG_LED_POW34_A + g_reg_base, led_reg.reg_val[i].reg_pow34);
  }
#endif
    } else {
      g_state.num_starts--;
      gsTemperatureStarts--;
      if(gsTemperatureStarts == 0)
      {
      gb_adpd_raw_start_temp = 1;
      gsTempSampleCount = 0;
      status = M2M2_APP_COMMON_STATUS_STREAM_STOPPED;
      }
      else
      {
      status = M2M2_APP_COMMON_STATUS_STREAM_COUNT_DECREMENT;
      }
    }
    command = M2M2_APP_COMMON_CMD_STREAM_STOP_RESP;
    break;
  case M2M2_APP_COMMON_CMD_STREAM_SUBSCRIBE_REQ:
    gsTemperatureSubscribers++;
#ifdef SLOT_SELECT
    if(check_temp_slot_set == false)
    {
      gnTemperature_Slot = 0x08; /* slot-D */
    }
    else
    {
    check_temp_slot_set = false;
    }
#else
      gnTemperature_Slot = 0x08; /* slot-D */
#endif
    /* post_office_setup_subscriber(M2M2_ADDR_SENSOR_ADPD, M2M2_ADDR_SENSOR_ADPD_STREAM, p_pkt->src, true); */
    post_office_setup_subscriber(M2M2_ADDR_MED_TEMPERATURE,M2M2_ADDR_MED_TEMPERATURE_STREAM, p_pkt->src, true);
    status = M2M2_APP_COMMON_STATUS_SUBSCRIBER_ADDED;
    command = M2M2_APP_COMMON_CMD_STREAM_SUBSCRIBE_RESP;
    break;
  case M2M2_APP_COMMON_CMD_STREAM_UNSUBSCRIBE_REQ:
    if (gsTemperatureSubscribers <= 1) {
      gsTemperatureSubscribers = 0;
      gnTemperature_Slot = 0;
      status = M2M2_APP_COMMON_STATUS_SUBSCRIBER_REMOVED;
    } else {
      gsTemperatureSubscribers--;
      status = M2M2_APP_COMMON_STATUS_SUBSCRIBER_COUNT_DECREMENT;
    }
    post_office_setup_subscriber(M2M2_ADDR_MED_TEMPERATURE, M2M2_ADDR_MED_TEMPERATURE_STREAM, p_pkt->src, false);
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
  }//if(NULL != p_resp_pkt)
  return p_resp_pkt;
}

/*!****************************************************************************
* \brief  Posts the message packet received into the Temperature task queue
*
* \param[in]  p_pkt: pointer to the message to be posted
*
* \return None
*/
void send_message_temperature_app_task(m2m2_hdr_t *p_pkt) {
  ADI_OSAL_STATUS osal_result;
  osal_result = adi_osal_MsgQueuePost(temperature_app_task_msg_queue,p_pkt);
  if (osal_result != ADI_OSAL_SUCCESS)
    post_office_consume_msg(p_pkt);
  adi_osal_SemPost(temperature_app_task_evt_sem);
}

/*!****************************************************************************
* \brief  Initializes the temperature task
*
* \param[in]  None
*
* \return     None
*
*/
void temperature_app_task_init(void) {
  ADI_OSAL_STATUS eOsStatus = ADI_OSAL_SUCCESS;
  temperature_app_task_attributes.pThreadFunc = temperature_app_task;
  temperature_app_task_attributes.nPriority = APP_OS_CFG_TEMPERATURE_APP_TASK_PRIO;
  temperature_app_task_attributes.pStackBase = &temperature_app_task_stack[0];
  temperature_app_task_attributes.nStackSize = APP_OS_CFG_TEMPERATURE_APP_TASK_STK_SIZE;
  temperature_app_task_attributes.pTaskAttrParam = NULL;
  temperature_app_task_attributes.szThreadName = "Temperature Sensor";
  temperature_app_task_attributes.pThreadTcb = &temperature_app_task_tcb;

  eOsStatus = adi_osal_MsgQueueCreate(&temperature_app_task_msg_queue,NULL,
                                    25);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
      Debug_Handler();
  }
  else
  {
    update_task_queue_list(APP_OS_CFG_TEMPERATURE_TASK_INDEX,temperature_app_task_msg_queue);
  }

  eOsStatus = adi_osal_ThreadCreateStatic(&temperature_app_task_handler,
                                    &temperature_app_task_attributes);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
      Debug_Handler();
  }

  adi_osal_SemCreate(&temperature_app_task_evt_sem, 0);
  post_office_add_mailbox(M2M2_ADDR_MED_TEMPERATURE, M2M2_ADDR_MED_TEMPERATURE_STREAM);
}

 /* Temperature sensor sampling period in terms of os ticks;
    free RTOS tick period is set to 1000ms or 1 Hz*/
/* uint32_t gsTempSampPeriod = 1000; */

/*!****************************************************************************
* \brief  Task which handles processing of the temperature data and commands received
*
* \param[in]  pArgument: input argument to the task
*
* \return None
*/
static void temperature_app_task(void *pArgument) {
  m2m2_hdr_t *p_in_pkt = NULL;
  m2m2_hdr_t *p_out_pkt = NULL;
  ADI_OSAL_STATUS         err;

  while (1)
  {
      adi_osal_SemPend(temperature_app_task_evt_sem, ADI_OSAL_TIMEOUT_FOREVER);
      p_in_pkt = post_office_get(ADI_OSAL_TIMEOUT_NONE, APP_OS_CFG_TEMPERATURE_TASK_INDEX);
      if (p_in_pkt == NULL) {
        /* No m2m2 messages to process, so fetch some data from the device. */
        fetch_temperature_data();
      } else {
        /* We got an m2m2 message from the queue, process it. */
       PYLD_CST(p_in_pkt, _m2m2_app_common_cmd_t, p_in_cmd);
       /* Look up the appropriate function to call in the function table */
        for (int i = 0; i < TEMPERATURE_APP_ROUTING_TBL_SZ; i++) {
          if (temperature_app_routing_table[i].command == p_in_cmd->command) {
            p_out_pkt = temperature_app_routing_table[i].cb_handler(p_in_pkt);
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

/*!****************************************************************************
* \brief  Calculates the temperature value from the input data
*
* \param[in]  pTherm: pointer to the ADPD4K value obtained from thermistor slot
* \param[in]  pRefVal: pointer to the ADPD4K value obtained from refernce cal-resistor slot
*
* \return if SUCCESS : nTempResult - Temperature value computed
*         if FAILURE : -1
*/
uint32_t getTemperature(uint32_t *pTherm, uint32_t *pRefVal)
{

  *pTherm = *pTherm - ADC_DARK_OFFSET;
  *pRefVal =  *pRefVal - ADC_DARK_OFFSET;
  /* Calculate the pulse volatge applied to cal-resistor and thermistor in nV */
  gsPulseVtg = (CAL_RESISTANCE * (ADC_RESOLUTION * *pRefVal));
  gsPulseVtg = gsPulseVtg/ADC_RESOLUTION;
  /*  Resistance in ohm as vtg is in nV and current is in nA */
  gsResistance = (gsPulseVtg / *pTherm);
  uint8_t loopCnt = 0;
  uint32_t nDiffRes = 0, nDeltaResPerDegree = 0, nDeltaRes = 0,nTempResult = 0;
#ifdef LUT_METHOD

  while(loopCnt<LUT_STEP_CNT)
  {
    if(gsResistance <= aTemp_LUT[LUT_STEP_CNT-1])
    {
        /* return max_temperature_value that is 20*5 = 100 */
        return((LUT_STEP_CNT-1)*LUT_STEP_SIZE);
    }
    else if(gsResistance >= aTemp_LUT[0])
    {
         /* return the min temperature value that is zero */
        return(0);
    }
    else
    {
        /* calculate temperature value here */
        if(gsResistance < aTemp_LUT[loopCnt])
        {
          loopCnt++;
        }
        else
        {
          /* The highest Temperature range it belongs to */
          nTempResult = loopCnt * LUT_STEP_SIZE;
          /*Fine tune the Temperature value*/
          nDiffRes = aTemp_LUT[loopCnt-1] - aTemp_LUT[loopCnt];  /* Find the temperature difference in this resistance block */
          nDeltaResPerDegree = nDiffRes/LUT_STEP_SIZE;
          nDeltaRes = (uint32_t) gsResistance - aTemp_LUT[loopCnt];
          nTempResult = (nTempResult * 10) - ((nDeltaRes * 10)/nDeltaResPerDegree); /* Fine tuned the temperature value */
          return(nTempResult);
        }
    }
  }
return -1;
#else
  if (gsResistance > THERM_RESISTANCE_AT_25)
  {
    /* indicates decrease in temperateure as NTC is used*/
    gsDeltaResistance = ((uint32_t)gsResistance - THERM_RESISTANCE_AT_25);
    gsDeltaResistance = gsDeltaResistance/THERM_SENSITIVITY;
    return (REFERENCE_TEMPERATURE - gsDeltaResistance);            /*25 is the reference temperature */
  }
  else
  {
    /* indicates increase in temperateure as NTC is used*/
    gsDeltaResistance = (THERM_RESISTANCE_AT_25 - (uint32_t)gsResistance);
    gsDeltaResistance = gsDeltaResistance/THERM_SENSITIVITY;
    return (REFERENCE_TEMPERATURE + gsDeltaResistance);            /*25 is the reference temperature*/
  }
#endif /*LUT_METHOD */
}

/*!****************************************************************************
* \brief  Computes the temperature data and does the data packetization
*
* \param[in]  None
*
* \return     None
*/
void fetch_temperature_data()
{
  m2m2_hdr_t *p_pkt = NULL;
  ADI_OSAL_STATUS         err;
  temperature_app_stream_t resp_temperature;

  if(gsTemperatureSubscribers > 0)
  {
    resp_temperature.command = (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_SENSOR_COMMON_CMD_STREAM_DATA;
    resp_temperature.sequence_num = gsTemperatureSeqNum++;
    resp_temperature.status = M2M2_APP_COMMON_STATUS_OK;
    resp_temperature.nTS = get_sensor_time_stamp();
    resp_temperature.nTemperature2 = (uint16_t)(gsResistance/100);  /*Resistance measured in 100's of ohms*/
    resp_temperature.nTemperature1 = getTemperature(&gsThermValue,&gsCalResValue);
    adi_osal_EnterCriticalRegion();
    p_pkt = post_office_create_msg(sizeof(temperature_app_stream_t) + M2M2_HEADER_SZ);
    if(p_pkt != NULL)
    {
#ifdef DEBUG_PKT
    g_temp_pkt_cnt++;
#endif
    p_pkt->src = M2M2_ADDR_MED_TEMPERATURE;
    p_pkt->dest = M2M2_ADDR_MED_TEMPERATURE_STREAM;
    p_pkt->length = M2M2_HEADER_SZ + sizeof(temperature_app_stream_t);
    memcpy(&p_pkt->data[0], &resp_temperature,  sizeof(temperature_app_stream_t));
#ifdef DEBUG_PKT
    post_office_msg_cnt(p_pkt);
#endif
    post_office_send(p_pkt,&err);
    p_pkt = NULL;
    }
    adi_osal_ExitCriticalRegion(); /* exiting critical region even if mem_alloc fails*/
  }
}

/*!****************************************************************************
* \brief  updates the adpd4k data from Thermistor and cal-resistor slots
*         to the global static varaibles used for computing temperature value
*
* \param[in]  None
*
* \return     None
*/
void temperatureAppData (uint32_t *pData) {
    gsThermValue = pData[0];
    gsCalResValue = pData[1];
    adi_osal_SemPost(temperature_app_task_evt_sem);
}
#endif
#endif//ENABLE_TEMPERATURE_APP