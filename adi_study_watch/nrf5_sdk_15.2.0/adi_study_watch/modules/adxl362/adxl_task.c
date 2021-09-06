/**
***************************************************************************
* @addtogroup Tasks
* @{
* @file         adxl_task.c
* @author       ADI
* @version      V1.0.0
* @date         26-Nov-2020
* @brief        Source file contains adxl processing wrapper.
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
/* -------------------------------- Includes --------------------------------*/
#include "adxl362.h"
#include "sensor_adxl_application_interface.h"
#include "us_tick.h"
#include <adxl_buffering.h>
#include <adxl_dcfg.h>
#include <adxl_task.h>
#include <clock_calibration.h>
#include <common_application_interface.h>
#include <includes.h>
#ifdef ENABLE_PEDO_APP
#include <pedometer_task.h>
#endif
#include <rtc.h>
#include "adpd400x_lib.h"
#include <app_common.h>

#ifdef DCB
#include <dcb_interface.h>
#endif

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
/*--------------------------- Defines ---------------------------------------*/
#define ADXL_APP_ROUTING_TBL_SZ                                                \
  (sizeof(adxl_app_routing_table) / sizeof(adxl_app_routing_table[0]))

/*--------------------------- Typedefs --------------------------------------*/

typedef m2m2_hdr_t *(app_cb_function_t)(m2m2_hdr_t *);

/*!
 * @brief:  The dictionary type for mapping addresses to callback handlers.
 */
typedef struct _app_routing_table_entry_t {
  uint8_t command;
  app_cb_function_t *cb_handler;
} app_routing_table_entry_t;

/*-------------------------- Public Variables -------------------------------*/
extern ADXL_TS_DATA_TYPE gADXL_dready_ts;
int16_t g_accelExists = -2;
#ifdef ENABLE_PPG_APP
extern uint32_t Ppg_Slot;
#endif
#ifdef ENABLE_PEDO_APP
extern uint32_t pedometer_event;
#endif
/*! This variable used for indicating the frequency of adxl samples*/
extern uint8_t gAdxlFreqSelected;
/*-------------------------- Public Function Prototype ----------------------*/

int32_t AdxlInit(void);
extern void AdxlFifoCallBack(void);
extern void SyncAppDataSend(uint32_t *p_data, uint32_t adpdtimestamp,
    int16_t *pAdxlData, uint32_t adxltimestamp);
/*-------------------------- Private Variables ------------------------------*/
/* Create the stack for task */
static uint8_t sensor_adxl_task_stack[APP_OS_CFG_SENSOR_ADXL_TASK_STK_SIZE];

/* Create handler for task */
static ADI_OSAL_THREAD_HANDLE sensor_adxl_task_handler;

/* Create task attributes variable */
static ADI_OSAL_STATIC_THREAD_ATTR sensor_adxl_task_attributes;

/* Create TCB for task */
static StaticTask_t adxlTaskTcb;

/* Create semaphores */
ADI_OSAL_SEM_HANDLE adxl_task_evt_sem;

/* Create Queue Handler for task */
static ADI_OSAL_QUEUE_HANDLE adxl_task_msg_queue = NULL;

/* ADXL VERSION STRING variable*/
const char GIT_ADXL_VERSION[] = "TEST ADXL_VERSION STRING";
/* ADXL VERIOSN STRING size variable*/
const uint8_t GIT_ADXL_VERSION_LEN = sizeof(GIT_ADXL_VERSION);

/* structure varible for storing the adxl app states*/
g_state_adxl_t g_state_adxl;

volatile uint8_t gsOneTimeValueWhenReadAdxlData = 0;
uint16_t gnAdxlODR;
#ifdef ENABLE_PPG_APP
/* counter used to send every kth sample to HR algo
   to have ppg odr = 50Hz, where k = odr of adxl*/
uint32_t gnHRAdxlSampleCount = 0;
extern Adpd400xLibConfig_t gAdpd400xLibCfg;
extern volatile uint8_t gn_uc_hr_enable;
extern volatile uint8_t gnAppSyncTimerStarted;
#endif
extern bool gRun_agc;
/*-------------------------- Private Function Prototype ---------------------*/

static void packetize_adxl_raw_data_no_compress(int16_t *p_data, uint32_t len,
    m2m2_sensor_adxl_data_no_compress_stream_t *pPkt, uint32_t timestamp);
static m2m2_hdr_t *adxl_app_reg_access(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *adxl_app_stream_config(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *adxl_app_status(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *adxl_app_load_cfg(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *adxl_app_get_dcfg(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *adxl_app_get_version(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *adxl_app_decimation(m2m2_hdr_t *p_pkt);
#ifdef DCB
static m2m2_hdr_t *adxl_dcb_command_read_config(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *adxl_dcb_command_write_config(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *adxl_dcb_command_delete_config(m2m2_hdr_t *p_pkt);
#endif
static m2m2_hdr_t *adxl_do_self_test(m2m2_hdr_t *p_pkt);
static void adxl_data_ready_cb(void);
static void fetch_adxl_data(void);
static void sensor_adxl_task(void *pArgument);
static int32_t AdxlSampleMode(void);

/* Table which maps adxl commands to the callback functions*/
app_routing_table_entry_t adxl_app_routing_table[] = {
    {M2M2_APP_COMMON_CMD_STREAM_START_REQ, adxl_app_stream_config},
    {M2M2_APP_COMMON_CMD_STREAM_STOP_REQ, adxl_app_stream_config},
    {M2M2_APP_COMMON_CMD_STREAM_SUBSCRIBE_REQ, adxl_app_stream_config},
    {M2M2_APP_COMMON_CMD_STREAM_UNSUBSCRIBE_REQ, adxl_app_stream_config},
    {M2M2_APP_COMMON_CMD_SENSOR_STATUS_QUERY_REQ, adxl_app_status},
    {M2M2_SENSOR_COMMON_CMD_GET_DCFG_REQ, adxl_app_get_dcfg},
    {M2M2_SENSOR_ADXL_COMMAND_LOAD_CFG_REQ, adxl_app_load_cfg},
    {M2M2_SENSOR_COMMON_CMD_READ_REG_16_REQ, adxl_app_reg_access},
    {M2M2_SENSOR_COMMON_CMD_WRITE_REG_16_REQ, adxl_app_reg_access},
    {M2M2_SENSOR_COMMON_CMD_GET_STREAM_DEC_FACTOR_REQ, adxl_app_decimation},
    {M2M2_SENSOR_COMMON_CMD_SET_STREAM_DEC_FACTOR_REQ, adxl_app_decimation},
    {M2M2_APP_COMMON_CMD_GET_VERSION_REQ, adxl_app_get_version},
#ifdef DCB
    {M2M2_DCB_COMMAND_READ_CONFIG_REQ, adxl_dcb_command_read_config},
    {M2M2_DCB_COMMAND_WRITE_CONFIG_REQ, adxl_dcb_command_write_config},
    {M2M2_DCB_COMMAND_ERASE_CONFIG_REQ, adxl_dcb_command_delete_config},
#endif
    {M2M2_SENSOR_ADXL_COMMAND_SELF_TEST_REQ, adxl_do_self_test},
};

#ifdef USER0_CONFIG_APP
#include "user0_config_app_task.h"
#include "app_timer.h"
#include "low_touch_task.h"
APP_TIMER_DEF(m_adxl_timer_id);     /**< Handler for repeated timer for adxl. */
static void adxl_timer_start(void);
static void adxl_timer_stop(void);
static void adxl_timeout_handler(void * p_context);
void start_adxl_app_timer();
static user0_config_app_timing_params_t adxl_app_timings = {0};
#endif

/**
 * @brief  Initializes the adxl task
 *
 * @param[in]  None
 *
 * @return     None
 *
 */
void sensor_adxl_task_init(void) {
  /* Initialize app state */
  g_state_adxl.num_subs = 0;
  g_state_adxl.num_starts = 0;
  /* Default behaviour is to send every packet */
  g_state_adxl.decimation_factor = 1;
  g_state_adxl.decimation_nsamples = 0;
  g_state_adxl.data_pkt_seq_num = 0;

  ADI_OSAL_STATUS eOsStatus = ADI_OSAL_SUCCESS;
  sensor_adxl_task_attributes.pThreadFunc = sensor_adxl_task;
  sensor_adxl_task_attributes.nPriority = APP_OS_CFG_SENSOR_ADXL_TASK_PRIO;
  sensor_adxl_task_attributes.pStackBase = &sensor_adxl_task_stack[0];
  sensor_adxl_task_attributes.nStackSize = APP_OS_CFG_SENSOR_ADXL_TASK_STK_SIZE;
  sensor_adxl_task_attributes.pTaskAttrParam = NULL;
  sensor_adxl_task_attributes.szThreadName = "ADXL Sensor";

  sensor_adxl_task_attributes.pThreadTcb = &adxlTaskTcb;
  eOsStatus = adi_osal_MsgQueueCreate(&adxl_task_msg_queue, NULL, 25);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
    Debug_Handler();
  } else {
    update_task_queue_list(APP_OS_CFG_ADXL_TASK_INDEX, adxl_task_msg_queue);
  }
  eOsStatus = adi_osal_ThreadCreateStatic(
      &sensor_adxl_task_handler, &sensor_adxl_task_attributes);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
    Debug_Handler();
  }
}

#ifdef USER0_CONFIG_APP
/**@brief Function for the Timer initialization.
*
* @details Initializes the timer module. This creates and starts application timers.
*
* @param[in]  None
*
* @return     None
*/
static void adxl_timer_init(void)
{
    ret_code_t err_code;

    /*! Create timers */
    err_code =  app_timer_create(&m_adxl_timer_id, APP_TIMER_MODE_REPEATED, adxl_timeout_handler);

    APP_ERROR_CHECK(err_code);
}

/**@brief   Function for starting application timers.
* @details Timers are run after the scheduler has started.
*
* @param[in]  None
*
* @return     None
*/
static void adxl_timer_start(void)
{
    /*! Start repeated timer */
    ret_code_t err_code = app_timer_start(m_adxl_timer_id, APP_TIMER_TICKS(TIMER_ONE_SEC_INTERVAL), NULL);
    APP_ERROR_CHECK(err_code);

    adxl_app_timings.check_timer_started = true;
}

/**@brief   Function for stopping the application timers.
*
* @param[in]  None
*
* @return     None
*/
static void adxl_timer_stop(void)
{
    /*! Stop the repeated timer */
    ret_code_t err_code = app_timer_stop(m_adxl_timer_id);
    APP_ERROR_CHECK(err_code);

    adxl_app_timings.check_timer_started = false;
}

/**@brief   Callback Function for application timer events
*
* @param[in]  p_context: pointer to the callback function arguments
*
* @return     None
*/
static void adxl_timeout_handler(void * p_context)
{
    if(adxl_app_timings.delayed_start && adxl_app_timings.start_time>0)
    {
      adxl_app_timings.start_time_count++; /*! Increment counter every sec., till it is equal to start_time Value in seconds. */
      if(adxl_app_timings.start_time_count == adxl_app_timings.start_time)
      {
        //delayed start time expired-turn ON ADXL
        if (ADXLDrv_SUCCESS == AdxlSampleMode())
        {
        }
        adxl_app_timings.delayed_start = false;
        adxl_app_timings.start_time_count = 0;
        adxl_app_timings.app_mode_on = true;
      }
      return;
    }

    if(adxl_app_timings.app_mode_on && adxl_app_timings.on_time>0)
    {
      adxl_app_timings.on_time_count++; /*! Increment counter every sec. incase of ADPD ON, till it is equal to Ton Value in seconds. */
      if(adxl_app_timings.on_time_count == adxl_app_timings.on_time)
      {
        //on timer expired - turn off ADXL
        if (g_state_adxl.num_starts == 1) {
            if (ADXLDrv_SUCCESS == AdxlDrvSetOperationMode(PRM_MEASURE_STANDBY_MODE)) {
            }
        }
        adxl_app_timings.app_mode_on = false;
        adxl_app_timings.on_time_count = 0;
      }
    }
    else if(!adxl_app_timings.app_mode_on && adxl_app_timings.off_time>0)
    {
      adxl_app_timings.off_time_count++; /*! Increment counter every sec. incase of ADPD OFF, till it is equal to Toff Value in seconds.*/
      if(adxl_app_timings.off_time_count == adxl_app_timings.off_time)
      {
         //off timer expired - turn on ADXL
         if (g_state_adxl.num_starts == 1) {
           if (ADXLDrv_SUCCESS == AdxlSampleMode())
           {
           }
         }
         adxl_app_timings.app_mode_on = true;
         adxl_app_timings.off_time_count = 0;
      }
    }
}

/**@brief   Function to start ADXL app timer
* @details  This is used in either interval based/intermittent LT mode3
*
* @param[in]  None
*
* @return     None
*/
void start_adxl_app_timer()
{
  if(adxl_app_timings.on_time > 0)
  {
    adxl_timer_start();
  }
}
#endif//USER0_CONFIG_APP

/**
 * @brief  Posts the message packet received into the Adxl task queue
 *
 * @param[in]  p_pkt: m2m2_hdr_t pointer to the message to be posted
 *
 * @return None
 */
void send_message_adxl_task(m2m2_hdr_t *p_pkt) {
  ADI_OSAL_STATUS osal_result;
  osal_result = adi_osal_MsgQueuePost(adxl_task_msg_queue, p_pkt);
  if (osal_result != ADI_OSAL_SUCCESS)
    post_office_consume_msg(p_pkt);
  adi_osal_SemPost(adxl_task_evt_sem);
}

/**
 * @brief  Task which handles processing of the adxl data and commands received
 *
 * @param[in]  pArgument: input argument to the task
 *
 * @return None
 */
static void sensor_adxl_task(void *pArgument) {
  ADI_OSAL_STATUS err;

#ifdef USER0_CONFIG_APP
  adxl_timer_init();
#endif
  adi_osal_SemCreate(&adxl_task_evt_sem, 0);
  AdxlDrvDataReadyCallback(adxl_data_ready_cb);
  AdxlInit();

  /*Wait for FDS init to complete*/
  adi_osal_SemPend(adxl_task_evt_sem, ADI_OSAL_TIMEOUT_FOREVER);

  if (load_adxl_cfg(362) != ADXL_DCB_STATUS_OK) {
    /*device id ==362 ,bcz of ADXL362 is a sensor model.*/
    Debug_Handler(); /*exception handler */
  }
  /*  AdxlDrvExtSampleMode(PRM_EXT_SAMPLE_ENABLE); */
  /*keeping the adxl interrupt P0.31 interrupt prio <
   * configMAX_SYSCALL_INTERRUPT_PRIORITY (3)*/
  /* NVIC_SetPriority(GPIOTE_IRQn,4); */
  //ret_code_t err_code = sd_nvic_SetPriority(GPIOTE_IRQn, APP_IRQ_PRIORITY_MID);
  GPIO_IRQ_ADXL362_Enable();

  post_office_add_mailbox(M2M2_ADDR_SENSOR_ADXL, M2M2_ADDR_SENSOR_ADXL_STREAM);
  while (1) {
    m2m2_hdr_t *p_in_pkt = NULL;
    m2m2_hdr_t *p_out_pkt = NULL;
    adi_osal_SemPend(adxl_task_evt_sem, ADI_OSAL_TIMEOUT_FOREVER);
    p_in_pkt =
        post_office_get(ADI_OSAL_TIMEOUT_NONE, APP_OS_CFG_ADXL_TASK_INDEX);
    if (p_in_pkt == NULL) {
      /* No m2m2 messages to process, so fetch some data from the device. */
      fetch_adxl_data();
    } else {
      /* We got an m2m2 message from the queue, process it. */
      PYLD_CST(p_in_pkt, _m2m2_app_common_cmd_t, p_in_cmd);
      /* Look up the appropriate function to call in the function table */
      for (int i = 0; i < ADXL_APP_ROUTING_TBL_SZ; i++) {
        if (adxl_app_routing_table[i].command == p_in_cmd->command) {
          p_out_pkt = adxl_app_routing_table[i].cb_handler(p_in_pkt);
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
 * @brief  Resets the adxl packetization process
 *
 * @param[in]  None
 *
 * @return     None
 */
static void reset_adxl_packetization(void) {
  g_state_adxl.adxl_pktizer.packet_nsamples = 0;
}

volatile int gDebugVar2 = 0;
/**
 * @brief  Fetches the adxl data from device and does the data packetization
 *
 * @param[in]  None
 *
 * @return     None
 */
static void fetch_adxl_data(void) {

  ADI_OSAL_STATUS err;

  g_state_adxl.adxl_pktizer.p_pkt = NULL;
  uint8_t pnRData[6] = {0, 0, 0, 0, 0, 0};
  uint32_t nlen = 6; /* 3 axis data each of 2 bytes size */
  ADXL_TS_DATA_TYPE current_ts = 0;
  CIRC_BUFF_STATUS_t status;
  static m2m2_sensor_adxl_data_no_compress_stream_t adxlpkt;

  if (gsOneTimeValueWhenReadAdxlData == 0) {
    GetAdxlOutputRate(&gnAdxlODR);
    gsOneTimeValueWhenReadAdxlData = 1;
  }

  adxl_read_data_to_buffer();
  status = adxl_buff_get(pnRData, &current_ts, &nlen);

  while (status == CIRC_BUFF_STATUS_OK) {
#ifdef ENABLE_PPG_APP
    //Collect data for HR algo in UC 1,2,3,5
    if (gn_uc_hr_enable && !gRun_agc) {
      if(++gnHRAdxlSampleCount == (gnAdxlODR / gAdpd400xLibCfg.hrmInputRate))
      {
          gnHRAdxlSampleCount = 0;
          SyncAppDataSend(0, 0, (int16_t *)&pnRData, current_ts);
      }
    }
    /* Check the slot is in PPG mode */
    else if (Ppg_Slot != 0) {
      SyncAppDataSend(0, 0, (int16_t *)&pnRData, current_ts);
    }
#endif
#ifdef ENABLE_PEDO_APP
    if (pedometer_event == 1) {
      send_pedometer_app_data((int16_t *)&pnRData, current_ts);
    }
#endif
    if(g_state_adxl.num_subs != 0)
    {
      g_state_adxl.decimation_nsamples++;
      if (g_state_adxl.decimation_nsamples >= g_state_adxl.decimation_factor) {
        g_state_adxl.decimation_nsamples = 0;
        packetize_adxl_raw_data_no_compress(
            (int16_t *)&pnRData, 3, &adxlpkt, current_ts);
        if (g_state_adxl.adxl_pktizer.packet_nsamples >
            g_state_adxl.adxl_pktizer.packet_max_nsamples) {
          adi_osal_EnterCriticalRegion();
          g_state_adxl.adxl_pktizer.p_pkt = post_office_create_msg(
              sizeof(m2m2_sensor_adxl_data_no_compress_stream_t) +
              M2M2_HEADER_SZ);
          if (g_state_adxl.adxl_pktizer.p_pkt != NULL) {
            PYLD_CST(g_state_adxl.adxl_pktizer.p_pkt,
                m2m2_sensor_adxl_data_no_compress_stream_t, p_payload_ptr);
            g_state_adxl.adxl_pktizer.p_pkt->src = M2M2_ADDR_SENSOR_ADXL;
            g_state_adxl.adxl_pktizer.p_pkt->dest = M2M2_ADDR_SENSOR_ADXL_STREAM;
            memcpy(&p_payload_ptr->command, &adxlpkt,
                sizeof(m2m2_sensor_adxl_data_no_compress_stream_t));
            p_payload_ptr->command = M2M2_SENSOR_COMMON_CMD_STREAM_DATA;
            p_payload_ptr->status = 0x00;
            p_payload_ptr->sequence_num = g_state_adxl.data_pkt_seq_num++;
#ifdef DEBUG_PKT
            post_office_msg_cnt(g_state_adxl.adxl_pktizer.p_pkt);
#endif
#ifdef USER0_CONFIG_APP
           if(adxl_app_timings.app_mode_on)
           {
#endif //USER0_CONFIG_APP
            post_office_send(g_state_adxl.adxl_pktizer.p_pkt, &err);
#ifdef USER0_CONFIG_APP
            }//if(adxl_app_timings.app_mode_on)
            else
              post_office_consume_msg(g_state_adxl.adxl_pktizer.p_pkt);
#endif //USER0_CONFIG_APP
            adi_osal_ExitCriticalRegion();
            g_state_adxl.adxl_pktizer.packet_nsamples = 0;
            g_state_adxl.adxl_pktizer.packet_max_nsamples = 0;
            g_state_adxl.adxl_pktizer.p_pkt = NULL;
          } else {
            gDebugVar2 = 1;
            adi_osal_ExitCriticalRegion(); /* exiting critical region even if
                                              mem_alloc fails*/
          }
        }
      }
    }//if(g_state_adxl.num_subs != 0)
    status = adxl_buff_get(pnRData, &current_ts, &nlen);
  }
}

/**
 * @brief  Packetizes the adxl raw data
 *
 * @param[in]  pAdxlData: Pointer to the adxl data buffer
 * @param[in]  len: length(size) of the adxl data buffer
 * @param[out] pPkt: pointer to the output data packet structure
 *                   m2m2_sensor_adxl_data_no_compress_stream_t
 *
 * @return     None
 */
static void packetize_adxl_raw_data_no_compress(int16_t *pAdxlData,
    uint32_t len, m2m2_sensor_adxl_data_no_compress_stream_t *pPkt,
    uint32_t timestamp) {
  uint32_t nOutDataCnt = 0;
  if (g_state_adxl.adxl_pktizer.packet_nsamples == 0) {
    g_state_adxl.adxl_pktizer.packet_max_nsamples =
        (sizeof(pPkt->adxldata) / sizeof(pPkt->adxldata[0]));
    pPkt->data_type = M2M2_SENSOR_RAW_DATA_ADXL;
    pPkt->timestamp = timestamp;
    pPkt->first_xdata = pAdxlData[nOutDataCnt];
    pPkt->first_ydata = pAdxlData[nOutDataCnt + 1];
    pPkt->first_zdata = pAdxlData[nOutDataCnt + 2];
    g_state_adxl.adxl_pktizer.packet_nsamples++;
  } else if (g_state_adxl.adxl_pktizer.packet_nsamples <=
             g_state_adxl.adxl_pktizer.packet_max_nsamples) {
    /*one packet =  five samples .first sample was added with header and
     * remaining 4 samples was added next to the header. */
    uint16_t i = g_state_adxl.adxl_pktizer.packet_nsamples - 1;
    if (timestamp <
        g_state_adxl.adxl_pktizer.prev_ts) /* handle day roll-over after 24hrs*/
    {
      /* MAX_RTC_TICKS_FOR_24_HOUR:- Max RTC Count value returned by
         get_sensor_timestamp() after 24hrs. Adding that value to have correct
         incTS value during day roll-over */
      pPkt->adxldata[i].incTS = MAX_RTC_TICKS_FOR_24_HOUR +
                                (timestamp - g_state_adxl.adxl_pktizer.prev_ts);
    } else {
      pPkt->adxldata[i].incTS = timestamp - g_state_adxl.adxl_pktizer.prev_ts;
    }
    pPkt->adxldata[i].xdata = pAdxlData[nOutDataCnt];
    pPkt->adxldata[i].ydata = pAdxlData[nOutDataCnt + 1];
    pPkt->adxldata[i].zdata = pAdxlData[nOutDataCnt + 2];
    g_state_adxl.adxl_pktizer.packet_nsamples++;
  }
  g_state_adxl.adxl_pktizer.prev_ts = timestamp;
}

/**
 * @brief  returns the adxl application version info
 *
 * @param[in]  p_pkt: Pointer to the input m2m2 packet structure
 *
 * @return     p_resp_pkt: Pointer to the output m2m2 packet structure
 */
static m2m2_hdr_t *adxl_app_get_version(m2m2_hdr_t *p_pkt) {
  PKT_MALLOC(p_resp_pkt, m2m2_app_common_version_t, 0);
  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_app_common_version_t, p_resp_payload);

    p_resp_payload->command = M2M2_APP_COMMON_CMD_GET_VERSION_RESP;
    p_resp_payload->major = 0x03;
    p_resp_payload->minor = 0x04;
    p_resp_payload->patch = 0x03;
    memcpy(&p_resp_payload->verstr[0], "ADXL_App", 9);
    memcpy(&p_resp_payload->str[0], &GIT_ADXL_VERSION, GIT_ADXL_VERSION_LEN);
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  }
  return p_resp_pkt;
}

/**
 * @brief  Puts the ADXL device into self test mode
 *
 * @param[in]  p_pkt: Pointer to the input m2m2 packet structure
 *
 * @return     p_resp_pkt: Pointer to the output m2m2 packet structure
 */
static m2m2_hdr_t *adxl_do_self_test(m2m2_hdr_t *p_pkt) {
  uint8_t ret;
  uint16_t AdxlDrvSelfTest();
  ret = AdxlDrvSelfTest();
  /* Declare and malloc a response packet */
  PKT_MALLOC(p_resp_pkt, m2m2_sensor_adxl_resp_t, 0);
  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_sensor_adxl_resp_t, p_resp_payload);

    if (!ret)
      p_resp_payload->status = M2M2_APP_COMMON_STATUS_OK; /* Test passed */
    else
      p_resp_payload->status = M2M2_APP_COMMON_STATUS_ERROR; /* Test Failed */
    p_resp_payload->command = M2M2_SENSOR_ADXL_COMMAND_SELF_TEST_RESP;
    p_resp_payload->deviceid = M2M2_SENSOR_ADXL_DEVICE_362;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  }
  return p_resp_pkt;
}

/**
 * @brief  loads the configuration settings to the ADXL device
 *
 * @param[in]  p_pkt: Pointer to the input m2m2 packet structure
 *
 * @return     p_resp_pkt: Pointer to the output m2m2 packet structure
 */
static m2m2_hdr_t *adxl_app_load_cfg(m2m2_hdr_t *p_pkt) {
  /* Declare a pointer to access the input packet payload */
  PYLD_CST(p_pkt, m2m2_sensor_adxl_resp_t, p_in_payload);
  /* Declare and malloc a response packet */
  PKT_MALLOC(p_resp_pkt, m2m2_sensor_adxl_resp_t, 0);
  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_sensor_adxl_resp_t, p_resp_payload);
    p_resp_payload->status = M2M2_APP_COMMON_STATUS_ERROR;
    uint16_t device_id = p_in_payload->deviceid;

    if (g_state_adxl.num_starts == 0) {
      if (!load_adxl_cfg(device_id)) { /* Loads the device configuration */
        p_resp_payload->status = M2M2_APP_COMMON_STATUS_OK;
      } else {
        p_resp_payload->status = M2M2_APP_COMMON_STATUS_ERROR;
      }
    }
    p_resp_payload->deviceid = device_id;
    p_resp_payload->command = M2M2_SENSOR_ADXL_COMMAND_LOAD_CFG_RESP;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  }
  return p_resp_pkt;
}

/**
 * @brief  returns the ADXL stream status
 *
 * @param[in]  p_pkt: Pointer to the input m2m2 packet structure
 *
 * @return     p_resp_pkt: Pointer to the output response m2m2 packet structure
 */
static m2m2_hdr_t *adxl_app_status(m2m2_hdr_t *p_pkt) {
  /* Declare and malloc a response packet */
  PKT_MALLOC(p_resp_pkt, m2m2_app_common_status_t, 0);
  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_app_common_status_t, p_resp_payload);

    p_resp_payload->command = M2M2_APP_COMMON_CMD_SENSOR_STATUS_QUERY_RESP;
    p_resp_payload->status = M2M2_APP_COMMON_STATUS_ERROR;

    if (g_state_adxl.num_starts == 0) {
      p_resp_payload->status = M2M2_APP_COMMON_STATUS_STREAM_STOPPED;
    } else {
      p_resp_payload->status = M2M2_APP_COMMON_STATUS_STREAM_STARTED;
    }
    p_resp_payload->stream = M2M2_ADDR_SENSOR_ADXL_STREAM;
    p_resp_payload->num_subscribers = g_state_adxl.num_subs;
    p_resp_payload->num_start_reqs = g_state_adxl.num_starts;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  }
  return p_resp_pkt;
}

/**
 * @brief  Handles start/stop/sub/unsub of adxl data streams commands
 *
 * @param[in]  p_pkt: Pointer to the input m2m2 packet structure with command
 *                    for stream start/stop/sub/unsub operations
 *
 * @return     p_resp_pkt: Pointer to the output response m2m2 packet structure
 */
static m2m2_hdr_t *adxl_app_stream_config(m2m2_hdr_t *p_pkt) {
  M2M2_APP_COMMON_STATUS_ENUM_t status = M2M2_APP_COMMON_STATUS_ERROR;
  M2M2_APP_COMMON_CMD_ENUM_t command;
  /*Keeping nRetCode as success by default, to handle else part
    of if(!adxl_app_timings.delayed_start) */
  int16_t nRetCode = ADXLDrv_SUCCESS;

  /* Declare a pointer to access the input packet payload */
  PYLD_CST(p_pkt, m2m2_app_common_sub_op_t, p_in_payload);
  /* Declare and malloc a response packet */
  PKT_MALLOC(p_resp_pkt, m2m2_app_common_sub_op_t, 0);
  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_app_common_sub_op_t, p_resp_payload);

    switch (p_in_payload->command) {
    case M2M2_APP_COMMON_CMD_STREAM_START_REQ:
      AdxlDrvDataReadyCallback(adxl_data_ready_cb);
      if (g_state_adxl.num_starts == 0) {
        gsOneTimeValueWhenReadAdxlData = 0;
        /* AdxlInit(); */
        reset_adxl_packetization();
        /*device id ==362 ,bcz of ADXL362 is a sensor model. */
        if (load_adxl_cfg(362) != ADXL_DCB_STATUS_OK) {
          Debug_Handler(); /* exception handler */
        }

        uint8_t nRegVal1;
        AdxlDrvRegRead(0x2C, &nRegVal1);
        gAdxlFreqSelected =
            nRegVal1 & 0x07; /*Check 0x2C reg & update timer freq */

#ifdef USER0_CONFIG_APP
      get_adxl_app_timings_from_user0_config_app_lcfg(&adxl_app_timings.start_time,\
                                        &adxl_app_timings.on_time, &adxl_app_timings.off_time);
      if(adxl_app_timings.start_time > 0)
      {
        adxl_app_timings.delayed_start = true;
      }

      //ADXL app not in continuous mode & its interval operation mode
      if(!is_adxl_app_mode_continuous() && !(get_low_touch_trigger_mode3_status()))
      {
        start_adxl_app_timer();
      }

      if(!adxl_app_timings.delayed_start)
      {
        nRetCode = AdxlSampleMode();
        adxl_app_timings.app_mode_on = true;
      }
      if(ADXLDrv_SUCCESS == nRetCode) {
#else
      if(ADXLDrv_SUCCESS == AdxlSampleMode()) {
#endif// USER0_CONFIG_APP
          uint16_t nRegVal2;
          Adpd400xDrvRegRead(0x0022, &nRegVal2);
          nRegVal2 = nRegVal2 & 0xFFC7;
          Adpd400xDrvRegWrite(
              0x0022, nRegVal2); /* To disable GPIO1 from adpd4k */
          uint8_t adxl_odr;
          adxl_odr = (GetFilterAdxl362() & 0x07);
          enable_adxl_ext_trigger(adxl_odr);
          g_state_adxl.num_starts = 1;
          status = M2M2_APP_COMMON_STATUS_STREAM_STARTED;
        } else {
          status = M2M2_APP_COMMON_STATUS_STREAM_NOT_STARTED;
        }
      } else {
        g_state_adxl.num_starts++;
        status = M2M2_APP_COMMON_STATUS_STREAM_IN_PROGRESS;
      }
      command = M2M2_APP_COMMON_CMD_STREAM_START_RESP;
      break;

    case M2M2_APP_COMMON_CMD_STREAM_STOP_REQ:
      if (0 == g_state_adxl.num_starts) {
        status = M2M2_APP_COMMON_STATUS_STREAM_STOPPED;
      } else if (1 == g_state_adxl.num_starts) {
        gsOneTimeValueWhenReadAdxlData = 0;
        uint8_t adxl_odr;
        adxl_odr = (GetFilterAdxl362() & 0x07);
        disable_adxl_ext_trigger(adxl_odr);
        AdxlDrvExtSampleMode(PRM_EXT_SAMPLE_DISABLE);
        if (ADXLDrv_SUCCESS ==
            AdxlDrvSetOperationMode(PRM_MEASURE_STANDBY_MODE)) {
          reset_adxl_packetization();
          g_state_adxl.num_starts = 0;
          status = M2M2_APP_COMMON_STATUS_STREAM_STOPPED;
#ifdef USER0_CONFIG_APP
          if(adxl_app_timings.check_timer_started)
          {
            adxl_timer_stop();
            adxl_app_timings.on_time_count = 0;
            adxl_app_timings.off_time_count = 0;
            adxl_app_timings.start_time_count = 0;
            adxl_app_timings.delayed_start =  false;
          }
#endif//USER0_CONFIG_APP
        } else {
          g_state_adxl.num_starts = 1;
          status = M2M2_APP_COMMON_STATUS_ERROR;
        }
      } else {
        g_state_adxl.num_starts--;
        status = M2M2_APP_COMMON_STATUS_STREAM_COUNT_DECREMENT;
      }
      command = M2M2_APP_COMMON_CMD_STREAM_STOP_RESP;
      break;

    case M2M2_APP_COMMON_CMD_STREAM_SUBSCRIBE_REQ:
      g_state_adxl.num_subs++;
      if(g_state_adxl.num_subs == 1)
      {
         /* reset pkt sequence no. only during 1st sub request */
         g_state_adxl.data_pkt_seq_num = 0;
      }
      post_office_setup_subscriber(M2M2_ADDR_SENSOR_ADXL,
          M2M2_ADDR_SENSOR_ADXL_STREAM, p_pkt->src, true);
      status = M2M2_APP_COMMON_STATUS_SUBSCRIBER_ADDED;
      command = M2M2_APP_COMMON_CMD_STREAM_SUBSCRIBE_RESP;
      break;

    case M2M2_APP_COMMON_CMD_STREAM_UNSUBSCRIBE_REQ:
      if (g_state_adxl.num_subs <= 1) {
        g_state_adxl.num_subs = 0;
        status = M2M2_APP_COMMON_STATUS_SUBSCRIBER_REMOVED;
      } else {
        g_state_adxl.num_subs--;
        status = M2M2_APP_COMMON_STATUS_SUBSCRIBER_COUNT_DECREMENT;
      }
      post_office_setup_subscriber(M2M2_ADDR_SENSOR_ADXL,
          M2M2_ADDR_SENSOR_ADXL_STREAM, p_pkt->src, false);
      command = M2M2_APP_COMMON_CMD_STREAM_UNSUBSCRIBE_RESP;
      break;

    default:
      /* Something has gone horribly wrong. */
      post_office_consume_msg(p_resp_pkt);
      return NULL;
      break;
    }

    p_resp_payload->command = command;
    p_resp_payload->status = status;
    p_resp_payload->stream = p_in_payload->stream;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  } // if(NULL != p_resp_pkt)
  return p_resp_pkt;
}

/**
 * @brief  Handles ADXL register read/write commands
 *
 * @param[in]  p_pkt: Pointer to the input m2m2 packet structure with command
 *                    for register read/write operation
 *
 * @return     p_resp_pkt: Pointer to the output response m2m2 packet structure
 */
static m2m2_hdr_t *adxl_app_reg_access(m2m2_hdr_t *p_pkt) {
  M2M2_APP_COMMON_STATUS_ENUM_t status = M2M2_APP_COMMON_STATUS_ERROR;
  /* Declare a pointer to access the input packet payload */
  PYLD_CST(p_pkt, m2m2_sensor_common_reg_op_16_hdr_t, p_in_payload);
  /* Allocate a response packet with space for the correct number of operations
   */
  PKT_MALLOC(p_resp_pkt, m2m2_sensor_common_reg_op_16_hdr_t,
      p_in_payload->num_ops * sizeof(p_in_payload->ops[0]));
  if (NULL != p_resp_pkt) {
    PYLD_CST(p_resp_pkt, m2m2_sensor_common_reg_op_16_hdr_t, p_resp_payload);
    uint8_t reg_data = 0;

    switch (p_in_payload->command) {
    case M2M2_SENSOR_COMMON_CMD_READ_REG_16_REQ:
      for (int i = 0; i < p_in_payload->num_ops; i++) {
        if (ADXLDrv_SUCCESS ==
            AdxlDrvRegRead(p_in_payload->ops[i].address, &reg_data)) {
          status = M2M2_APP_COMMON_STATUS_OK;
        } else {
          status = M2M2_APP_COMMON_STATUS_ERROR;
        }
        p_resp_payload->ops[i].address = p_in_payload->ops[i].address;
        p_resp_payload->ops[i].value = reg_data;
      }
      p_resp_payload->command =
          (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_SENSOR_COMMON_CMD_READ_REG_16_RESP;
      break;
    case M2M2_SENSOR_COMMON_CMD_WRITE_REG_16_REQ:
      for (int i = 0; i < p_in_payload->num_ops; i++) {
        NRF_LOG_INFO("ADXL reg w add:val=%x:%x",p_in_payload->ops[i].address,p_in_payload->ops[i].value);
        if (ADXLDrv_SUCCESS == AdxlDrvRegWrite(p_in_payload->ops[i].address,
                                   p_in_payload->ops[i].value)) {
          /*If ODR change is requested; set the adxl trigger pulse time
           * accordingly */
          if (p_in_payload->ops[i].address ==
              REG_FILTER_CTL) { /* reset packetization can be added? */
            set_adxl_trigger_freq(
                p_in_payload->ops[i].value &
                0x07); /*Last 3 bits give the frequency range */
            gsOneTimeValueWhenReadAdxlData = 0;
#ifdef ENABLE_PPG_APP
            gnAppSyncTimerStarted = 0;
#endif
          }
          status = M2M2_APP_COMMON_STATUS_OK;
        } else {
          status = M2M2_APP_COMMON_STATUS_ERROR;
        }
        p_resp_payload->ops[i].address = p_in_payload->ops[i].address;
        p_resp_payload->ops[i].value = p_in_payload->ops[i].value;
      }
      p_resp_payload->command =
          (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_SENSOR_COMMON_CMD_WRITE_REG_16_RESP;
      break;
    default:
      /* Something has gone horribly wrong*/
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
 * @brief  Sets/returns the ADXL samples decimation factor
 *
 * @param[in]  p_pkt: Pointer to the input m2m2 packet structure with command
 *                    for setting/getting the adxl samples decimation factor
 *
 * @return     p_resp_pkt: Pointer to the output response m2m2 packet structure
 */
static m2m2_hdr_t *adxl_app_decimation(m2m2_hdr_t *p_pkt) {
  M2M2_APP_COMMON_STATUS_ENUM_t status = M2M2_APP_COMMON_STATUS_OK;

  /* Declare a pointer to access the input packet payload */
  PYLD_CST(p_pkt, m2m2_sensor_common_decimate_stream_t, p_in_payload);
  /* Allocate a response packet with space for the correct number of operations
   */
  PKT_MALLOC(p_resp_pkt, m2m2_sensor_common_decimate_stream_t, 0);
  if (NULL != p_resp_pkt) {
    PYLD_CST(p_resp_pkt, m2m2_sensor_common_decimate_stream_t, p_resp_payload);

    switch (p_in_payload->command) {
    case M2M2_SENSOR_COMMON_CMD_GET_STREAM_DEC_FACTOR_REQ:
      p_resp_payload->command =
          M2M2_SENSOR_COMMON_CMD_GET_STREAM_DEC_FACTOR_RESP;
      break;
    case M2M2_SENSOR_COMMON_CMD_SET_STREAM_DEC_FACTOR_REQ:
      g_state_adxl.decimation_factor = p_in_payload->dec_factor;
      if (g_state_adxl.decimation_factor == 0) {
        g_state_adxl.decimation_factor = 1;
      }
      p_resp_payload->command =
          M2M2_SENSOR_COMMON_CMD_SET_STREAM_DEC_FACTOR_RESP;
      break;
    }
    p_resp_payload->status = status;
    p_resp_payload->dec_factor = g_state_adxl.decimation_factor;
    p_resp_payload->stream = p_in_payload->stream;
    p_resp_pkt->dest = p_pkt->src;
    p_resp_pkt->src = p_pkt->dest;
  }
  return p_resp_pkt;
}

/**
 * @brief  Returns the ADXL Configuration settings decimation factor
 *
 * @param[in]  p_pkt: Pointer to the input m2m2 packet structure with command
 *                    for getting adxl dcfg
 *
 * @return     p_resp_pkt: Pointer to the output response m2m2 packet structure
 *                         with adxl dcfg information
 */
static m2m2_hdr_t *adxl_app_get_dcfg(m2m2_hdr_t *p_pkt) {
  /* Declare and malloc a response packet */
  PKT_MALLOC(p_resp_pkt, m2m2_sensor_dcfg_data_t, 0);
  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_sensor_dcfg_data_t, p_resp_payload);
    memset(p_resp_payload->dcfgdata, 0, sizeof(p_resp_payload->dcfgdata));
    /* Gets the device configuration */
    if (ADXL_DCFG_STATUS_OK ==
        read_adxl_dcfg(
            &p_resp_payload->dcfgdata[0], &p_resp_payload->size)) {
      p_resp_payload->status = M2M2_APP_COMMON_STATUS_OK;
    } else {
      p_resp_payload->status = M2M2_APP_COMMON_STATUS_ERROR;
    }
    p_resp_payload->command =
        (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_SENSOR_COMMON_CMD_GET_DCFG_RESP;
    p_resp_payload->num_tx_pkts = 1;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  }
  return p_resp_pkt;
}

/* Variable used for storing the current timestamp of interrupt event*/
uint32_t gadxl_dready_ts = 0;
/* Variable used for storing the previous timestamp of interrupt event*/
uint32_t gadxl_dready_ts_prev = 0;
/* Variable used for storing the timestamp difference between two consecutive
 * interrupt event*/
uint32_t g_adxltime_diff = 0;

/**
 * @brief  ADXL data ready interrupt callback function
 *
 * @param[in]  None
 *
 * @return     None
 */
#ifdef DEBUG_ADXL_TICKS
uint32_t g_adxl_isr_list[40] = {0};
uint8_t adxl_list_index = 0;
#endif
static void adxl_data_ready_cb(void) {
  gadxl_dready_ts = get_sensor_time_stamp();
#ifdef DEBUG_ADXL_TICKS
 g_adxl_isr_list[adxl_list_index++] = gadxl_dready_ts;
  if (adxl_list_index >=40)
    adxl_list_index =0;
#endif
  gADXL_dready_ts = gadxl_dready_ts;
  g_adxltime_diff = gadxl_dready_ts - gadxl_dready_ts_prev;
  gadxl_dready_ts_prev = gadxl_dready_ts;
  adi_osal_SemPost(adxl_task_evt_sem);
}

/**
 * @brief    Adxl sample mode
 * @param    None
 * @return   Success/Error
 */
int32_t AdxlSampleMode(void) {
  AdxlDrvSetOperationMode(PRM_MEASURE_MEASURE_MODE);
  AdxlDrvExtSampleMode(PRM_EXT_SAMPLE_ENABLE);
  return 0;
}

/**
 * @brief    Adxl device initialization
 * @param    None
 * @retval   Success/Error
 */
int32_t AdxlInit() {
  uint8_t nFifoWatermark = 4;
  uint16_t nAdpdOdr = 50;

  /* AdxlDrvDataReadyCallback(adxl_data_ready_cb); // AdxlFifoCallBack */
  adxl_buff_init();
  /* AdxlRxBufferInit(); */
  AdxlDrvOpenDriver(nAdpdOdr, nFifoWatermark);
  AdxlDrvSetOperationMode(PRM_MEASURE_STANDBY_MODE);
  return 0;
}

/*************************/

#ifdef DCB
/**
 * @brief  Returns the ADXL DCB configuration information from FDS
 *
 * @param[in]  p_pkt: Pointer to the input m2m2 packet structure with command
 *                    for reading adxl dcb config information
 *
 * @return     p_resp_pkt: Pointer to the output response m2m2 packet structure
 *                         with adxl dcb config information
 */
static m2m2_hdr_t *adxl_dcb_command_read_config(m2m2_hdr_t *p_pkt) {
  static uint16_t r_size = 0;
  uint32_t dcbdata[MAXADXLDCBSIZE];
  M2M2_DCB_STATUS_ENUM_t status = M2M2_DCB_STATUS_ERR_NOT_CHKD;

  PKT_MALLOC(p_resp_pkt, m2m2_dcb_adxl_data_t, 0);
  if (NULL != p_resp_pkt) {
    /*  Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_dcb_adxl_data_t, p_resp_payload);
    memset(p_resp_payload->dcbdata, 0, sizeof(p_resp_payload->dcbdata));
    r_size = (uint16_t)MAXADXLDCBSIZE;
    if (read_adxl_dcb(&dcbdata[0], &r_size) == ADXL_DCB_STATUS_OK) {
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

/**
 * @brief  Writes the ADXL DCB configuration information to FDS
 *
 * @param[in]  p_pkt: Pointer to the input m2m2 packet structure with command
 *                    for writing adxl dcb config information
 *
 * @return     p_resp_pkt: Pointer to the output response m2m2 packet structure
 */
static m2m2_hdr_t *adxl_dcb_command_write_config(m2m2_hdr_t *p_pkt) {
  M2M2_DCB_STATUS_ENUM_t status = M2M2_DCB_STATUS_ERR_NOT_CHKD;
  uint32_t dcbdata[MAXADXLDCBSIZE];

  /* Declare a pointer to access the input packet payload */
  PYLD_CST(p_pkt, m2m2_dcb_adxl_data_t, p_in_payload);
  PKT_MALLOC(p_resp_pkt, m2m2_dcb_adxl_data_t, 0);
  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_dcb_adxl_data_t, p_resp_payload);

    for (int i = 0; i < p_in_payload->size; i++)
      dcbdata[i] = p_in_payload->dcbdata[i];
    if (write_adxl_dcb(&dcbdata[0], p_in_payload->size) == ADXL_DCB_STATUS_OK) {
      adxl_set_dcb_present_flag(true);
      status = M2M2_DCB_STATUS_OK;
    } else {
      status = M2M2_DCB_STATUS_ERR_ARGS;
    }

    p_resp_payload->status = status;
    p_resp_payload->command = M2M2_DCB_COMMAND_WRITE_CONFIG_RESP;
    p_resp_payload->size = 0;
    for(uint16_t i=0; i< MAXADXLDCBSIZE; i++)
      p_resp_payload->dcbdata[i] = 0;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  }
  return p_resp_pkt;
}

/**
 * @brief  Deletes the ADXL DCB configuration information from FDS
 *
 * @param[in]  p_pkt: Pointer to the input m2m2 packet structure with command
 *                    for deleting adxl dcb config information
 *
 * @return     p_resp_pkt: Pointer to the output response m2m2 packet structure
 */
static m2m2_hdr_t *adxl_dcb_command_delete_config(m2m2_hdr_t *p_pkt) {
  M2M2_DCB_STATUS_ENUM_t status = M2M2_DCB_STATUS_ERR_NOT_CHKD;

  PKT_MALLOC(p_resp_pkt, m2m2_dcb_adxl_data_t, 0);
  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_dcb_adxl_data_t, p_resp_payload);

    if (delete_adxl_dcb() == ADXL_DCB_STATUS_OK) {
      adxl_set_dcb_present_flag(false);
      status = M2M2_DCB_STATUS_OK;
    } else {
      status = M2M2_DCB_STATUS_ERR_ARGS;
    }

    p_resp_payload->status = status;
    p_resp_payload->command = M2M2_DCB_COMMAND_ERASE_CONFIG_RESP;
    p_resp_payload->size = 0;
    for(uint16_t i=0; i< MAXADXLDCBSIZE; i++)
      p_resp_payload->dcbdata[i] = 0;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  }
  return p_resp_pkt;
}

#endif

/*@ **/