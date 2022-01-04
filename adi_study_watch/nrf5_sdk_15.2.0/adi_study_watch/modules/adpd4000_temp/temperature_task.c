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
#include <adpd4000_task.h>
volatile uint8_t gb_adpd_raw_start_temp = 1; /* Flag to handle whether ADPD sensor was 'start'ed to get raw data(CLI_USB/CLI_BLE) or if it was started by internal applications like Temp */
#ifdef ENABLE_TEMPERATURE_APP
/* -------------------------------- Includes -------------------------------- */
#ifdef EVTBOARD
#include <temperature_task.h>
#include "adpd400x_reg.h"
#include <tempr_lcfg.h>
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "adi_adpd_m2m2.h"
#include "adi_adpd_ssm.h"

#ifdef ENABLE_TEMP_DEBUG_STREAM
#include <debug_interface.h>
#endif
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
extern tAdiAdpdAppState oAppState;
uint32_t  gnTemperature_Slot = 0;
/*! structure to hold the lcfg which is currently being used
    It could be the default lcfg or from the temperature lcfg dcb */
temperature_lcfg_t active_temperature_lcfg = {0};

/*! structure to store temperature slots info*/
tempr_slot_info_t tempr_slot_info[SLOT_NUM];

#ifdef DEBUG_PKT
uint32_t g_temp_pkt_cnt = 0;
#endif
#ifdef ENABLE_TEMP_DEBUG_STREAM
#define M2M2_DEBUG_INFO_SIZE   12
#define DEBUG_INFO_SIZE    M2M2_DEBUG_INFO_SIZE + 6 // 6 here to avoid overflow
uint8_t g_tempOffset = 0;
uint32_t g_pkt_fail_count = 0;
uint32_t g_temp_debugInfo[DEBUG_INFO_SIZE];
uint64_t g_temp_debugInfo_64[DEBUG_INFO_SIZE];
uint32_t temp_app_packet_count = 0;
uint32_t disp_app_packet_count = 0;
uint32_t batt_app_packet_count = 0;
uint32_t usb_pend_start_time = 0;
uint32_t usb_delay_time = 0;
void packetize_temp_debug_data(void);
static struct _g_temp_debug_data{
  m2m2_hdr_t   *p_pkt;
  uint8_t num_subs;
  uint16_t  data_pkt_seq_num;
} g_temp_debug_data;
#endif
/*! Look up table for thermistor's resistance based on beta value for
  temperature ranging from 0 to 100 degree celcius in steps of 5 degree celcius*/

uint32_t aTemp_LUT[LUT_STEP_CNT] = {  355600, 271800, 209400, 162500, 127000,
                                      100000, 79230,  63180,  50680,  40900,
                                      33190,  27090,  22220,  18320,  15180,
                                      12640,  10580,  8887,   7500,   6357,
                                      5410
                                   };

extern slot_led_reg_t led_reg;
extern uint8_t gsOneTimeValueWhenReadAdpdData;
extern uint16_t g_adpd_odr;
extern const temperature_lcfg_t tempr_default_lcfg;
extern tAdiAdpdSSmInst goAdiAdpdSSmInst;
/*------------------------------ Public Function Prototype ------------------ */
extern void temperatureAppData (uint32_t *pData, uint8_t slot_index);
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
static uint16_t gsTemperatureSubscribers = 0, gsTemperatureSeqNum;
/* Variable for counting the number of stream start requests*/
uint16_t gsTemperatureStarts;
/* Variables for storing the adpd4k value for thermistor and cal-resistor slots */
static uint32_t gsThermValue = 0, gsCalResValue = 0;
/*
  Each bit of this variable inidcates the presence/absence of thermistor in a given slot
  0th bit - slot A; 1st bit - slot B and so on till slot L
  1 - Thermistor is connected to the slot
  0 - No thermistor is connected the slot
*/
uint32_t g_therm_slot_en_bit_mask = 0;

/* Variable to indicate the last thermistor slot selected */
uint8_t g_last_thermistor_slot_index = 0;

/* Array to store the thermistor's ADC values read from ADPD4K */
static uint32_t thermistor_adc_val[SLOT_NUM];

/* Variable to count the number of samples from ADPD4K */
uint32_t num_thermistor_samples = 0;

extern tAdiAdpdSSmInst goAdiAdpdSSmInst;

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
  {M2M2_SENSOR_COMMON_CMD_GET_DCFG_REQ, adpd_app_get_dcfg},
  {M2M2_APP_COMMON_CMD_READ_LCFG_REQ, temperature_app_lcfg_access},
  {M2M2_APP_COMMON_CMD_WRITE_LCFG_REQ, temperature_app_lcfg_access},
#ifdef DCB
    {M2M2_APP_COMMON_CMD_SET_LCFG_REQ, tempr_app_set_dcb_lcfg},
    {M2M2_DCB_COMMAND_READ_CONFIG_REQ, tempr_dcb_command_read_config},
    {M2M2_DCB_COMMAND_WRITE_CONFIG_REQ, tempr_dcb_command_write_config},
    {M2M2_DCB_COMMAND_ERASE_CONFIG_REQ, tempr_dcb_command_delete_config},
#endif
};

#ifdef USER0_CONFIG_APP
#include "user0_config_app_task.h"
#include "app_timer.h"
#include "low_touch_task.h"
APP_TIMER_DEF(m_temp_timer_id);     /**< Handler for repeated timer for temp. */
static void temp_timer_start(void);
static void temp_timer_stop(void);
static void temp_timeout_handler(void * p_context);
void start_temp_app_timer();
user0_config_app_timing_params_t temp_app_timings = {0};
#endif


/*!***********************************************************************************
* \brief      Get the slot index of the last thermistor/cal resistor slot enabled
*             This will be used to identify one complete cycle of ADC measurement for
*             different thermistors connected to ADPD4K
*
* \param[in]  None
*
* \return     returns slot index of the last thermistor/cal resistor slot enabled
**************************************************************************************/
uint8_t get_last_thermistor_slot_index_enabled(void)
{
  int8_t i = 0;
  for( i=(SLOT_NUM-1); i>=0; i--)
  {
    if(active_temperature_lcfg.slots_selected & ( 1 << i))
    {
        return i;
    }

  }
}

/*!****************************************************************************
 * \brief Get ADPD4k DCFG
 *
 * \param[in]           p_pkt: input m2m2 packet
 *
 * \return              pointer to reponse m2m2 packet
 *****************************************************************************/
static m2m2_hdr_t *adpd_app_get_dcfg(m2m2_hdr_t *p_pkt) {
    uint16_t dcfg_size;
    uint16_t i = 0, num_pkts = 0, dcfg_array_index = 0;
    uint32_t  dcfgdata[MAXADPD4000DCBSIZE*MAX_ADPD4000_DCB_PKTS];
    ADI_OSAL_STATUS  err;
    M2M2_APP_COMMON_STATUS_ENUM_t status = M2M2_APP_COMMON_STATUS_ERROR;

  /* Declare and malloc a response packet */
  PKT_MALLOC(p_resp_pkt, m2m2_sensor_dcfg_data_t, 0);

  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_sensor_dcfg_data_t, p_resp_payload);
    memset(dcfgdata, 0,sizeof(dcfgdata));
    memset(p_resp_payload->dcfgdata, 0, sizeof(p_resp_payload->dcfgdata));
    if (ADPD4000_DCFG_STATUS_OK == read_adpd4000_dcfg(&dcfgdata[0], &dcfg_size)) { // dcfg size will give number of registers
      status = M2M2_APP_COMMON_STATUS_OK;
      num_pkts = (dcfg_size / MAXADPD4000DCBSIZE) + ((dcfg_size % MAXADPD4000DCBSIZE) ? 1 : 0 );
      dcfg_array_index = 0;
      for(uint16_t p = 0; p < num_pkts; p++) {
         p_resp_payload->size = \
              (p != num_pkts-1) ? MAXADPD4000DCBSIZE : (dcfg_size % MAXADPD4000DCBSIZE);
         p_resp_payload->num_tx_pkts = num_pkts;
         for (i = 0; i < p_resp_payload->size; i++) {
            p_resp_payload->dcfgdata[i] = dcfgdata[dcfg_array_index++];
         }
         if(p != num_pkts-1) {
           p_resp_pkt->src = p_pkt->dest;
           p_resp_pkt->dest = p_pkt->src;
           p_resp_payload->status = status;
           p_resp_payload->command = M2M2_SENSOR_COMMON_CMD_GET_DCFG_RESP;
           post_office_send(p_resp_pkt, &err);

           /* Delay is kept same as what is there in low touch task */
           MCU_HAL_Delay(60);

           PKT_MALLOC(p_resp_pkt, m2m2_sensor_dcfg_data_t, 0);
           if(NULL != p_resp_pkt) {
             // Declare a pointer to the response packet payload
             PYLD_CST(p_resp_pkt, m2m2_sensor_dcfg_data_t, p_resp_payload);
             memset(p_resp_payload->dcfgdata, 0, sizeof(p_resp_payload->dcfgdata));
           }//if(NULL != p_resp_pkt)
           else {
             return NULL;
           }
         }
       }
      }else {
        p_resp_payload->size = 0;
        p_resp_payload->num_tx_pkts = 0;
        status = M2M2_APP_COMMON_STATUS_ERROR;
      }//if(read_adpd4000_dcfg())
    p_resp_payload->status = status;
    p_resp_payload->command = M2M2_SENSOR_COMMON_CMD_GET_DCFG_RESP;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  }//if(NULL != p_resp_pkt)
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
  /* Declare a pointer to access the input packet payload */
  PYLD_CST(p_pkt, m2m2_app_common_status_t, p_in_payload);
  /* Declare and malloc a response packet */
  PKT_MALLOC(p_resp_pkt, m2m2_app_common_status_t, 0);
  if(NULL != p_resp_pkt)
  {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_app_common_status_t, p_resp_payload);

    p_resp_payload->command = M2M2_APP_COMMON_CMD_SENSOR_STATUS_QUERY_RESP;
    p_resp_payload->status = M2M2_APP_COMMON_STATUS_ERROR;

    if (oAppState.nNumberOfStart== 0) {
      p_resp_payload->status = M2M2_APP_COMMON_STATUS_STREAM_STOPPED;
    } else {
      p_resp_payload->status = M2M2_APP_COMMON_STATUS_STREAM_STARTED;
    }
    uint8_t slot_num;
    if(p_in_payload->stream == M2M2_ADDR_MED_TEMPERATURE_STREAM || p_in_payload->stream == M2M2_ADDR_MED_TEMPERATURE_STREAM4)
      slot_num = E_THERMISTOR_SLOT_D;
    else
      slot_num = p_in_payload->stream - M2M2_ADDR_MED_TEMPERATURE_STREAM1;
    p_resp_payload->stream = p_in_payload->stream;
    p_resp_payload->num_subscribers = tempr_slot_info[slot_num].num_subs;
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
 
  ADI_OSAL_STATUS err;
  M2M2_APP_COMMON_CMD_ENUM_t    command;
  uint16_t nSlotIdx = 0;
  /*Keeping nRetCode as success by default, to handle else part
    of if(!temp_app_timings.delayed_start) */
  int16_t nRetCode = ADPD400xDrv_SUCCESS;
  uint8_t slot_num;
  /* Declare a pointer to access the input packet payload */
  PYLD_CST(p_pkt, m2m2_app_common_sub_op_t, p_in_payload);
  /* Declare and malloc a response packet */
  PKT_MALLOC(p_resp_pkt, m2m2_app_common_sub_op_t, 0);
  
  if(NULL != p_resp_pkt)
  {
    PYLD_CST(p_resp_pkt, m2m2_app_common_sub_op_t,p_resp_payload);    
  switch (p_in_payload->command) {
  case M2M2_APP_COMMON_CMD_STREAM_START_REQ:
    if(oAppState.nNumberOfStart == 0)
    {
#ifndef SLOT_SELECT
  for(uint8_t i = 0; i < SLOT_NUM; i++)
  {
    g_reg_base = i * 0x20;
    if(adi_adpddrv_RegRead(ADPD400x_REG_LED_POW12_A + g_reg_base, &led_reg.reg_val[i].reg_pow12) == ADPD400xDrv_SUCCESS)
      {
        adi_adpddrv_RegWrite(ADPD400x_REG_LED_POW12_A + g_reg_base, 0x0);
      }/* disable led for slot-A - I */

     if(adi_adpddrv_RegRead(ADPD400x_REG_LED_POW34_A + g_reg_base, &led_reg.reg_val[i].reg_pow34) == ADPD400xDrv_SUCCESS)
      {
        adi_adpddrv_RegWrite(ADPD400x_REG_LED_POW34_A + g_reg_base, 0x0);
      }/* disable led for slot-> A - I */
   }
#endif
      g_adpd_odr = get_adpd_odr();
      enable_ext_syncmode();
      enable_adpd_ext_trigger(g_adpd_odr);
      gsOneTimeValueWhenReadAdpdData = 0;
#ifdef USER0_CONFIG_APP
      //Use user0 config app timings
      if((is_bypass_user0_timings() == false))
      {

        get_temp_app_timings_from_user0_config_app_lcfg(&temp_app_timings.start_time,\
                                        &temp_app_timings.on_time, &temp_app_timings.off_time);
        if(temp_app_timings.start_time > 0)
        {
          temp_app_timings.delayed_start = true;
        }
#ifdef LOW_TOUCH_FEATURE
        //Temp app not in continuous mode & its interval operation mode
        if(!is_temp_app_mode_continuous() && !(get_low_touch_trigger_mode3_status()))
#else
        //Temp app not in continuous mode
        if(!is_temp_app_mode_continuous())
#endif
        {
          start_temp_app_timer();
        }
      }//if((is_bypass_user0_timings() == false))

      if(!temp_app_timings.delayed_start)
      {
        nRetCode = adi_adpdssm_start();        
        temp_app_timings.app_mode_on = true;
      }
      if(ADI_ADPD_SSM_SUCCESS == nRetCode)
#else
      if(ADI_ADPD_SSM_SUCCESS == adi_adpdssm_start())
#endif// USER0_CONFIG_APP
        {
          oAppState.nNumberOfStart = 1;
          gsTemperatureStarts = 1;
          gb_adpd_raw_start_temp = 0;
          num_thermistor_samples = 0;
          g_last_thermistor_slot_index = get_last_thermistor_slot_index_enabled();
          if(get_low_touch_trigger_mode3_status())
          {
            num_thermistor_samples = ((goAdiAdpdSSmInst.oAdpdSlotInst.aSlotInfo[E_CAL_RES_SLOT_E].nOutputDataRate * active_temperature_lcfg.sample_period) - 1);
          }
          else
          {
            num_thermistor_samples = 0;
          }
          status = M2M2_APP_COMMON_STATUS_STREAM_STARTED;  
        }/* else stream not started */
        else
        {
          status = M2M2_APP_COMMON_STATUS_STREAM_NOT_STARTED;
        }
    }
  /* Stream already in progress */
  else 
  {
    oAppState.nNumberOfStart++;
    gb_adpd_raw_start_temp = 0;
    gsTemperatureStarts++;
    g_last_thermistor_slot_index = get_last_thermistor_slot_index_enabled();
    if(gsTemperatureStarts == 1)
    {            
      if(get_low_touch_trigger_mode3_status())
      {
        num_thermistor_samples = ((goAdiAdpdSSmInst.oAdpdSlotInst.aSlotInfo[E_CAL_RES_SLOT_E].nOutputDataRate * active_temperature_lcfg.sample_period) - 1);
      }
      else
      {
        num_thermistor_samples = 0;
      }
      status = M2M2_APP_COMMON_STATUS_STREAM_STARTED;
#ifdef USER0_CONFIG_APP
      get_temp_app_timings_from_user0_config_app_lcfg(&temp_app_timings.start_time,\
                                      &temp_app_timings.on_time, &temp_app_timings.off_time);
      if(temp_app_timings.start_time > 0)
      {
        temp_app_timings.delayed_start = true;
      }
#ifdef LOW_TOUCH_FEATURE
      //Temp app not in continuous mode & its interval operation mode
      if(!is_temp_app_mode_continuous() && !(get_low_touch_trigger_mode3_status()))
#else
      //Temp app not in continuous mode
      if(!is_temp_app_mode_continuous())
#endif
      {
        start_temp_app_timer();
      }

      if(!temp_app_timings.delayed_start)
      {
        temp_app_timings.app_mode_on = true;
        ;//TODO: Check and handle ADPD sample mode/idle mode switching
      }
#endif// USER0_CONFIG_APP
    }
    else
    {
      status = M2M2_APP_COMMON_STATUS_STREAM_IN_PROGRESS;
    }
  }
  command = M2M2_APP_COMMON_CMD_STREAM_START_RESP;
  break;

  case M2M2_APP_COMMON_CMD_STREAM_STOP_REQ:
  if(oAppState.nNumberOfStart == 0)
  {
    status = M2M2_APP_COMMON_STATUS_STREAM_STOPPED;
  }
  else if (oAppState.nNumberOfStart == 1)
  {    
    gsOneTimeValueWhenReadAdpdData = 0;

    disable_ext_syncmode();  /*! Disable ext sync mode*/
    g_adpd_odr = get_adpd_odr();
    disable_adpd_ext_trigger(g_adpd_odr);

    nRetCode = adi_adpdssm_stop();
    if(nRetCode == ADI_ADPD_SSM_SUCCESS)
    {    
        oAppState.nNumberOfStart = 0;
        gb_adpd_raw_start_temp = 1;
        gsTemperatureStarts = 0;
        num_thermistor_samples = 0;
        status = M2M2_APP_COMMON_STATUS_STREAM_STOPPED;
#ifdef USER0_CONFIG_APP
        if(temp_app_timings.check_timer_started)
        {
          temp_timer_stop();
          temp_app_timings.on_time_count = 0;
          temp_app_timings.off_time_count = 0;
          temp_app_timings.start_time_count = 0;
          temp_app_timings.delayed_start =  false;
        }
#endif//USER0_CONFIG_APP
    }//TODO MM: Handle M2M2_APP_COMMON_STATUS_STREAM_COUNT_DECREMENT from adpd task
    else
    {   
        oAppState.nNumberOfStart = 1;
        gb_adpd_raw_start_temp = 1;
        gsTemperatureStarts = 1;
        status = M2M2_APP_COMMON_STATUS_ERROR;

    }    
#ifndef SLOT_SELECT
if(status != M2M2_APP_COMMON_STATUS_STREAM_COUNT_DECREMENT)
{
  for(uint8_t i = 0; i < SLOT_NUM; i++)
  {
    g_reg_base = i * 0x20;
    adi_adpddrv_RegWrite(ADPD400x_REG_LED_POW12_A + g_reg_base, led_reg.reg_val[i].reg_pow12); /* enable led */
    adi_adpddrv_RegWrite(ADPD400x_REG_LED_POW34_A + g_reg_base, led_reg.reg_val[i].reg_pow34);
  }
}
#endif
  }
  else
  {
    oAppState.nNumberOfStart--;
    gsTemperatureStarts--;
    if(gsTemperatureStarts == 0)
    {
      gb_adpd_raw_start_temp = 1;
      num_thermistor_samples = 0;
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
    if(gsTemperatureSubscribers == 1)
    {
      /* reset pkt sequence no. only during 1st sub request */
      gsTemperatureSeqNum = 0;
    }

    if(p_in_payload->stream == M2M2_ADDR_MED_TEMPERATURE_STREAM || p_in_payload->stream == M2M2_ADDR_MED_TEMPERATURE_STREAM4)
    {
        slot_num = E_THERMISTOR_SLOT_D;
        post_office_setup_subscriber(
            M2M2_ADDR_MED_TEMPERATURE, M2M2_ADDR_MED_TEMPERATURE_STREAM, p_pkt->src, true);

    }
    else
    {
        slot_num = p_in_payload->stream - M2M2_ADDR_MED_TEMPERATURE_STREAM1;
        post_office_setup_subscriber(
            M2M2_ADDR_MED_TEMPERATURE, p_in_payload->stream, p_pkt->src, true);
    }

#ifdef ENABLE_TEMP_DEBUG_STREAM
          g_temp_debug_data.num_subs++;
          if(g_temp_debug_data.num_subs == 1){
            g_temp_debug_data.data_pkt_seq_num = 0;
            g_tempOffset = 0;
            g_pkt_fail_count = 0;
            g_pkt_fail_count = 0;
            memset(g_temp_debugInfo,0,sizeof(g_temp_debugInfo));
            post_office_setup_subscriber(M2M2_ADDR_SENSOR_ADPD4000,M2M2_ADDR_SYS_DBG_STREAM, p_pkt->src, true);
          }
#endif

    tempr_slot_info[slot_num].num_subs++;
    if (tempr_slot_info[slot_num].num_subs == 1)
      tempr_slot_info[slot_num].seq_num = 0;

#ifdef SLOT_SELECT
    if(check_temp_slot_set == false)
    {
      g_therm_slot_en_bit_mask = 0x18; /* slot-D & E */
    }
    else
    {
    check_temp_slot_set = false;
    }
#else
      g_therm_slot_en_bit_mask = active_temperature_lcfg.slots_selected;
#endif
    while((nSlotIdx < SLOT_NUM) && (g_therm_slot_en_bit_mask != 0))
    {
     if(g_therm_slot_en_bit_mask & (0x1 << nSlotIdx)) 
     {
       adi_adpdssm_SetpsmActive(nSlotIdx);
     }
     nSlotIdx++;
    }
    // post_office_setup_subscriber(M2M2_ADDR_MED_TEMPERATURE,M2M2_ADDR_MED_TEMPERATURE_STREAM, p_pkt->src, true);
    status = M2M2_APP_COMMON_STATUS_SUBSCRIBER_ADDED;
    command = M2M2_APP_COMMON_CMD_STREAM_SUBSCRIBE_RESP;
    break;

  case M2M2_APP_COMMON_CMD_STREAM_UNSUBSCRIBE_REQ:
    
    if (gsTemperatureSubscribers <= 1) {
      gsTemperatureSubscribers = 0;
      g_therm_slot_en_bit_mask = 0;
//      status = M2M2_APP_COMMON_STATUS_SUBSCRIBER_REMOVED;
    } else {
      gsTemperatureSubscribers--;
//      status = M2M2_APP_COMMON_STATUS_SUBSCRIBER_COUNT_DECREMENT;
    }

    if(p_in_payload->stream == M2M2_ADDR_MED_TEMPERATURE_STREAM || p_in_payload->stream == M2M2_ADDR_MED_TEMPERATURE_STREAM4)
    {
        slot_num = E_THERMISTOR_SLOT_D;
        post_office_setup_subscriber(M2M2_ADDR_MED_TEMPERATURE, M2M2_ADDR_MED_TEMPERATURE_STREAM, p_pkt->src, false);
    }
    else
    {
        slot_num = p_in_payload->stream - M2M2_ADDR_MED_TEMPERATURE_STREAM1;
        post_office_setup_subscriber(M2M2_ADDR_MED_TEMPERATURE, p_in_payload->stream, p_pkt->src, false);
    }

    if (tempr_slot_info[slot_num].num_subs <= 1)
    {
      tempr_slot_info[slot_num].num_subs = 0;
      status = M2M2_APP_COMMON_STATUS_SUBSCRIBER_REMOVED;
    }
    else
    {
      tempr_slot_info[slot_num].num_subs--;
      status = M2M2_APP_COMMON_STATUS_SUBSCRIBER_COUNT_DECREMENT;
    }
#ifdef ENABLE_TEMP_DEBUG_STREAM
          if(g_temp_debug_data.num_subs <= 1){
            g_temp_debug_data.num_subs = 0;
            g_temp_debug_data.data_pkt_seq_num = 0;
            g_tempOffset = 0;
            memset(g_temp_debugInfo,0,sizeof(g_temp_debugInfo));
            post_office_setup_subscriber(M2M2_ADDR_SENSOR_ADPD4000,M2M2_ADDR_SYS_DBG_STREAM,p_pkt->src, false);
          }else{
            g_temp_debug_data.num_subs--;
          }
#endif
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

#ifdef USER0_CONFIG_APP
/**@brief Function for the Timer initialization.
*
* @details Initializes the timer module. This creates and starts application timers.
*
* @param[in]  None
*
* @return     None
*/
static void temp_timer_init(void)
{
    ret_code_t err_code;

    /*! Create timers */
    err_code =  app_timer_create(&m_temp_timer_id, APP_TIMER_MODE_REPEATED, temp_timeout_handler);

    APP_ERROR_CHECK(err_code);
}

/**@brief   Function for starting application timers.
* @details Timers are run after the scheduler has started.
*
* @param[in]  None
*
* @return     None
*/
static void temp_timer_start(void)
{
    /*! Start repeated timer */
    ret_code_t err_code = app_timer_start(m_temp_timer_id, APP_TIMER_TICKS(TIMER_ONE_SEC_INTERVAL), NULL);
    APP_ERROR_CHECK(err_code);

    temp_app_timings.check_timer_started = true;
}

/**@brief   Function for stopping the application timers.
*
* @param[in]  None
*
* @return     None
*/
static void temp_timer_stop(void)
{
    /*! Stop the repeated timer */
    ret_code_t err_code = app_timer_stop(m_temp_timer_id);
    APP_ERROR_CHECK(err_code);

    temp_app_timings.check_timer_started = false;
}

/**@brief   Callback Function for application timer events
*
* @param[in]  p_context: pointer to the callback function arguments
*
* @return     None
*/
static void temp_timeout_handler(void * p_context)
{
    if(temp_app_timings.delayed_start && temp_app_timings.start_time>0)
    {
      temp_app_timings.start_time_count++; /*! Increment counter every sec., till it is equal to start_time Value in seconds. */
      if(temp_app_timings.start_time_count == temp_app_timings.start_time)
      {
#ifndef SLOT_SELECT
        //Check if ADPD is also started, if so turn LED off
        if (oAppState.nNumberOfStart>= 1) {
          for(uint8_t i=0 ; i<SLOT_NUM ; i++)
          {
            g_reg_base = i * 0x20;
            if(adi_adpddrv_RegRead(ADPD400x_REG_LED_POW12_A + g_reg_base, &led_reg.reg_val[i].reg_pow12) == ADPD400xDrv_SUCCESS)
            {
              adi_adpddrv_RegWrite(ADPD400x_REG_LED_POW12_A + g_reg_base, 0x0);
            }/* disable led for slot-A - I */

             if(adi_adpddrv_RegRead(ADPD400x_REG_LED_POW34_A + g_reg_base, &led_reg.reg_val[i].reg_pow34) == ADPD400xDrv_SUCCESS)
             {
              adi_adpddrv_RegWrite(ADPD400x_REG_LED_POW34_A + g_reg_base, 0x0);
             }/* disable led for slot-> A - I */
          }
        }
#endif

        //TODO: Disable other slots except Slot D, E
        //delayed start time expired-turn ON ADPD
        // if (ADPD400xDrv_SUCCESS == adi_adpdssm_setOperationMode(ADPD400xDrv_MODE_SAMPLE))
        if(adi_adpdssm_start() == ADI_ADPD_SSM_SUCCESS)
        {
        }
        temp_app_timings.delayed_start = false;
        temp_app_timings.start_time_count = 0;
        temp_app_timings.app_mode_on = true;
      }
      return;
    }

    if(temp_app_timings.app_mode_on && temp_app_timings.on_time>0)
    {
        temp_app_timings.on_time_count++; /*! Increment counter every sec. incase of ADPD ON, till it is equal to Ton Value in seconds. */
        if(temp_app_timings.on_time_count == temp_app_timings.on_time)
        {
          //on timer expired - turn off ADPD
          if (oAppState.nNumberOfStart >= 1) {
              // if (ADPD400xDrv_SUCCESS == adi_adpdssm_setOperationMode(ADPD400xDrv_MODE_IDLE)) 
              if(adi_adpdssm_stop() == ADI_ADPD_SSM_SUCCESS)
              {
              }
              //TODO: Enable other slots except Slot D, E, disable Slot D,E
          }
          temp_app_timings.app_mode_on = false;
          temp_app_timings.on_time_count = 0;
        }
    }
    else if(!temp_app_timings.app_mode_on && temp_app_timings.off_time>0)
    {
      temp_app_timings.off_time_count++; /*! Increment counter every sec. incase of ADPD OFF, till it is equal to Toff Value in seconds.*/
      if(temp_app_timings.off_time_count == temp_app_timings.off_time)
        {
           //off timer expired - turn on ADPD
           if (oAppState.nNumberOfStart>= 1) {
              //  if (ADPD400xDrv_SUCCESS == adi_adpdssm_setOperationMode(ADPD400xDrv_MODE_SAMPLE))
              if(adi_adpdssm_start() == ADI_ADPD_SSM_SUCCESS)
               {
               }
           }
           temp_app_timings.app_mode_on = true;
           temp_app_timings.off_time_count = 0;
        }
    }
}

/**@brief   Function to start Temp app timer
* @details  This is used in either interval based/intermittent LT mode3
*
* @param[in]  None
*
* @return     None
*/
void start_temp_app_timer()
{
  if(temp_app_timings.on_time > 0)
  {
    temp_timer_start();
  }
}
#endif//USER0_CONFIG_APP

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
  /* Thread Name should be of max 10 Characters */
  temperature_app_task_attributes.szThreadName = "Temp_Task";
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

  for (uint8_t slot_num = 0; slot_num < SLOT_NUM; slot_num++) {
    if( slot_num == E_THERMISTOR_SLOT_D)
        post_office_add_mailbox(M2M2_ADDR_MED_TEMPERATURE, M2M2_ADDR_MED_TEMPERATURE_STREAM);
    else
        post_office_add_mailbox(M2M2_ADDR_MED_TEMPERATURE,
            (M2M2_ADDR_ENUM_t)(M2M2_ADDR_MED_TEMPERATURE_STREAM1 + slot_num));
  }

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
static void temperature_app_task(void *pArgument) 
{
  ADI_OSAL_STATUS         err;
#ifdef USER0_CONFIG_APP
  temp_timer_init();
#endif
#ifdef ENABLE_TEMP_DEBUG_STREAM
  g_temp_debug_data.num_subs = 0;
  g_temp_debug_data.data_pkt_seq_num = 0;
  post_office_add_mailbox(M2M2_ADDR_SENSOR_ADPD4000, M2M2_ADDR_SYS_DBG_STREAM);
#endif
  /*load the lcfg structure from dcb if present else copy the default lcfg*/
  if (tempr_get_dcb_present_flag() == true) {
    /*! dcb present, so read the lcfg from dcb and load the lcfg structure */
    load_tempr_lcfg_from_dcb();
  }
  else
  {
    /*! dcb not present, so load the lcfg structure with default values */
    memcpy((uint8_t *)&active_temperature_lcfg,(uint8_t *)&tempr_default_lcfg,sizeof(active_temperature_lcfg));
  }

  while (1)
  {
    m2m2_hdr_t *p_in_pkt = NULL;
    m2m2_hdr_t *p_out_pkt = NULL;
    adi_osal_SemPend(temperature_app_task_evt_sem, ADI_OSAL_TIMEOUT_FOREVER);
    p_in_pkt = post_office_get(ADI_OSAL_TIMEOUT_NONE, APP_OS_CFG_TEMPERATURE_TASK_INDEX);
    /* if packets not present then fetch_temperature data */
    if (p_in_pkt == NULL) 
    {   
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

/*!*****************************************************************************************
* \brief  Calculates the temperature value from the input data
*
* \param[in]  pTherm: pointer to the ADPD4K value obtained from thermistor slot
* \param[in]  pRefVal: pointer to the ADPD4K value obtained from refernce cal-resistor slot
*
* \return if SUCCESS : nTempResult - Temperature value computed
*         if FAILURE : -1
*******************************************************************************************/
#if 1
uint32_t getTemperature(uint8_t slot_index, uint8_t lut_index)
{
  uint32_t thermistor_val = 0, cal_res_val = 0;
  cal_res_val = thermistor_adc_val[E_CAL_RES_SLOT_E] - ADC_DARK_OFFSET;
  thermistor_val = thermistor_adc_val[slot_index] - ADC_DARK_OFFSET;
  /* Calculate the pulse volatge applied to cal-resistor and thermistor in nV */
  gsPulseVtg = (CAL_RESISTANCE * (ADC_RESOLUTION * cal_res_val));
  gsPulseVtg = gsPulseVtg/ADC_RESOLUTION;
  /*  Resistance in ohm as vtg is in nV and current is in nA */
  gsResistance = (gsPulseVtg / thermistor_val);

  uint8_t loopCnt = 0;
  uint32_t nDiffRes = 0, nDeltaResPerDegree = 0, nDeltaRes = 0,nTempResult = 0;

  while(loopCnt<LUT_STEP_CNT)
  {
    if(gsResistance <= active_temperature_lcfg.T_I_curve_LUT[lut_index][LUT_STEP_CNT-1])
    {
        /* return max_temperature_value that is 20*5 = 100 */
        return((LUT_STEP_CNT-1) * LUT_STEP_SIZE * TEMPERATURE_SCALING_FACTOR);
    }
    else if(gsResistance >= active_temperature_lcfg.T_I_curve_LUT[lut_index][0])
    {
         /* return the min temperature value that is zero */
        return(0);
    }
    else
    {
        /* calculate temperature value here */
        if(gsResistance < active_temperature_lcfg.T_I_curve_LUT[lut_index][loopCnt])
        {
          loopCnt++;
        }
        else
        {
          /* The highest Temperature range it belongs to */
          nTempResult = loopCnt * LUT_STEP_SIZE;
          /*Fine tune the Temperature value*/
          nDiffRes = active_temperature_lcfg.T_I_curve_LUT[lut_index][loopCnt-1] - active_temperature_lcfg.T_I_curve_LUT[lut_index][loopCnt];  /* Find the temperature difference in this resistance block */
          nDeltaResPerDegree = nDiffRes/LUT_STEP_SIZE;
          nDeltaRes = (uint32_t) gsResistance - active_temperature_lcfg.T_I_curve_LUT[lut_index][loopCnt];
          nTempResult = (nTempResult * TEMPERATURE_SCALING_FACTOR) - ((nDeltaRes * TEMPERATURE_SCALING_FACTOR)/nDeltaResPerDegree); /* Fine tuned the temperature value */
          if(nTempResult >= TEMPERATURE_MAX_VALUE)
            nTempResult = TEMPERATURE_MAX_VALUE;
          return(nTempResult);
        }
    }
  }
  return 0;
}
#else
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
  return 0;
}
#endif

/*!****************************************************************************
* \brief  Computes the temperature data and does the data packetization
*
* \param[in]  None
*
* \return     None
*******************************************************************************/
void fetch_temperature_data()
{
  m2m2_hdr_t *p_pkt = NULL;
  ADI_OSAL_STATUS         err;
  temperature_app_stream_t resp_temperature;

#ifdef USER0_CONFIG_APP
  if(temp_app_timings.app_mode_on)
  {
#endif //USER0_CONFIG_APP
  if(gsTemperatureSubscribers > 0)
  {
    uint8_t lut_index = 0;
    for(uint8_t s_index = 0; s_index < SLOT_NUM; s_index++)
    {
      if((active_temperature_lcfg.slots_selected & (1 << s_index)) && (s_index != E_CAL_RES_SLOT_E))
      {
        if(tempr_slot_info[s_index].num_subs != 0) /* Check if the stream is subscribed*/
        {
          resp_temperature.command = (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_SENSOR_COMMON_CMD_STREAM_DATA;
          resp_temperature.sequence_num = tempr_slot_info[s_index].seq_num++;
          resp_temperature.status = M2M2_APP_COMMON_STATUS_OK;
          resp_temperature.nTS = get_sensor_time_stamp();
          tempr_slot_info[s_index].res = gsResistance;
          resp_temperature.nTemperature1 = getTemperature(s_index,lut_index);
          tempr_slot_info[s_index].tempr = resp_temperature.nTemperature1;
          resp_temperature.nTemperature2 = (uint16_t)(gsResistance/100);  /*Resistance measured in 100's of ohms*/
          adi_osal_EnterCriticalRegion();
          lut_index++;
#ifdef ENABLE_TEMP_DEBUG_STREAM
          temp_app_packet_count++;
#endif
          p_pkt = post_office_create_msg(sizeof(temperature_app_stream_t) + M2M2_HEADER_SZ);
          if(p_pkt != NULL)
          {
#ifdef DEBUG_PKT
            g_temp_pkt_cnt++;
#endif
            p_pkt->src = M2M2_ADDR_MED_TEMPERATURE;

            /*for slot D, keep the legacy stream ID*/
            if(s_index == E_THERMISTOR_SLOT_D)
              p_pkt->dest = M2M2_ADDR_MED_TEMPERATURE_STREAM;
            else
              p_pkt->dest = M2M2_ADDR_MED_TEMPERATURE_STREAM1 + s_index;

            p_pkt->length = M2M2_HEADER_SZ + sizeof(temperature_app_stream_t);
            memcpy(&p_pkt->data[0], &resp_temperature,  sizeof(temperature_app_stream_t));
#ifdef DEBUG_PKT
            post_office_msg_cnt(p_pkt);
#endif
            post_office_send(p_pkt,&err);
            p_pkt = NULL;
          }
#ifdef ENABLE_TEMP_DEBUG_STREAM
          else{
            g_temp_debugInfo_64[3] = get_sensor_time_stamp();
          }
#endif
          adi_osal_ExitCriticalRegion(); /* exiting critical region even if mem_alloc fails*/
        }
      }
    }
  }
#ifdef USER0_CONFIG_APP
  }
#endif//USER0_CONFIG_APP
}

/*!*******************************************************************************
* \brief      Updates the adpd4k data from Thermistor and cal-resistor slots
*             to the global static array used for computing temperature value
*
* \param[in]  pData : pointer to the 32bit ADPD4K data from a slot
*
* \param[in]  slot_index : indicates the slot to which the data belongs to
*
* \return     None
**********************************************************************************/
void temperatureAppData (uint32_t *pData, uint8_t slot_index) {

  if((active_temperature_lcfg.slots_selected & (1 << slot_index)))
    thermistor_adc_val[slot_index] = *pData;
  /* when the last thermistor's data is read from ADPD4K,trigger temperature app */
  if(slot_index == g_last_thermistor_slot_index)
  {
      if(num_thermistor_samples++ >= goAdiAdpdSSmInst.oAdpdSlotInst.aSlotInfo[E_CAL_RES_SLOT_E].nOutputDataRate * active_temperature_lcfg.sample_period)
      {
#ifdef ENABLE_TEMP_DEBUG_STREAM
  packetize_temp_debug_data();
  g_temp_debugInfo_64[0] = goAdiAdpdSSmInst.oAdpdSlotInst.aSlotInfo[E_CAL_RES_SLOT_E].nOutputDataRate;
  g_temp_debugInfo_64[1] = num_thermistor_samples;
  g_temp_debugInfo_64[2] = active_temperature_lcfg.slots_selected;
  g_temp_debugInfo_64[3] = 0;
#endif
          num_thermistor_samples = 0;
          adi_osal_SemPost(temperature_app_task_evt_sem);
      }
  }
}
#endif
#endif//ENABLE_TEMPERATURE_APP

#ifdef ENABLE_TEMP_DEBUG_STREAM

/**
* @brief Constructs the packet to send the ADPD debug data
*
* @return None
*
*/
void packetize_temp_debug_data(void) {
  ADI_OSAL_STATUS         err;
  if(g_temp_debug_data.num_subs > 0){
    adi_osal_EnterCriticalRegion();
    g_temp_debug_data.p_pkt = post_office_create_msg(sizeof(m2m2_app_debug_stream_t) + M2M2_HEADER_SZ);
    if(g_temp_debug_data.p_pkt != NULL){
      PYLD_CST(g_temp_debug_data.p_pkt, m2m2_app_debug_stream_t,p_payload_ptr);
      p_payload_ptr->command = M2M2_SENSOR_COMMON_CMD_STREAM_DATA;
      p_payload_ptr->status = M2M2_APP_COMMON_STATUS_OK;
      p_payload_ptr->timestamp = get_sensor_time_stamp();
      memset(p_payload_ptr->debuginfo, 0, M2M2_DEBUG_INFO_SIZE*4);
      for (uint8_t i = 0; i < M2M2_DEBUG_INFO_SIZE; i++){
        p_payload_ptr->debuginfo[i] = g_temp_debugInfo[i];
      }
     // memset(g_temp_debugInfo, 0, sizeof(g_temp_debugInfo));
      g_temp_debug_data.p_pkt->src = M2M2_ADDR_SENSOR_ADPD4000;
      g_temp_debug_data.p_pkt->dest = M2M2_ADDR_SYS_DBG_STREAM;
      p_payload_ptr->sequence_num = g_temp_debug_data.data_pkt_seq_num++;

      post_office_send(g_temp_debug_data.p_pkt, &err);
      g_temp_debug_data.p_pkt = NULL;
    }
    adi_osal_ExitCriticalRegion(); /* exiting critical region even if mem_alloc fails*/
  }
}
#endif