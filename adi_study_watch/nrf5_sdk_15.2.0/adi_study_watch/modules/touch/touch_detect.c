#include <post_office.h>
#include "touch_detect.h"
#include "ad7156_dcfg.h"
#include "sensor_ad7156_application_interface.h"
#ifdef VSM_MBOARD
#include "watch_vsm_motherboard_pin_config.h"
#elif PCBA
#include "watch_board_pcba_pin_config.h"
#else
#include "watch_board_evt_pin_config.h"
#endif
//#include "app_timer.h"
#include "nrf_drv_gpiote.h"
#include "app_error.h"
#include "sdk_common.h"
#include "ad7156.h"
#ifdef LOW_TOUCH_FEATURE
#include "low_touch_task.h"
#endif

#include "nrf_log.h"
#if NRF_LOG_ENABLED
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#endif

#include <adi_osal.h>
#include <app_cfg.h>
#include <task_includes.h>

#ifdef DCB
#include <dcb_interface.h>
#endif

ADI_OSAL_STATIC_THREAD_ATTR touch_task_attributes;
uint8_t touch_task_stack[APP_OS_CFG_TOUCH_TASK_STK_SIZE];
StaticTask_t touch_task_tcb;
ADI_OSAL_THREAD_HANDLE touch_task_handler;
ADI_OSAL_QUEUE_HANDLE  touch_task_msg_queue = NULL;

#define TOP_TOUCH_APP_ROUTING_TBL_SZ (sizeof(top_touch_app_routing_table) / sizeof(top_touch_app_routing_table[0]))

typedef  m2m2_hdr_t *(app_cb_function_t)(m2m2_hdr_t*);

typedef struct _app_routing_table_entry_t {
  uint8_t                    command;
  app_cb_function_t          *cb_handler;
}app_routing_table_entry_t;

static m2m2_hdr_t *ad7156_app_reg_access(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *ad7156_app_load_cfg(m2m2_hdr_t *p_pkt);
#ifdef DCB
static m2m2_hdr_t *ad7156_dcb_command_read_config(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *ad7156_dcb_command_write_config(m2m2_hdr_t *p_pkt);
static m2m2_hdr_t *ad7156_dcb_command_delete_config(m2m2_hdr_t *p_pkt);
#endif

app_routing_table_entry_t top_touch_app_routing_table[] = {
  {M2M2_SENSOR_AD7156_COMMAND_LOAD_CFG_REQ, ad7156_app_load_cfg},
  {M2M2_SENSOR_COMMON_CMD_READ_REG_16_REQ, ad7156_app_reg_access},
  {M2M2_SENSOR_COMMON_CMD_WRITE_REG_16_REQ, ad7156_app_reg_access},
#ifdef DCB
  {M2M2_DCB_COMMAND_READ_CONFIG_REQ,ad7156_dcb_command_read_config },
  {M2M2_DCB_COMMAND_WRITE_CONFIG_REQ, ad7156_dcb_command_write_config},
  {M2M2_DCB_COMMAND_ERASE_CONFIG_REQ, ad7156_dcb_command_delete_config},
#endif
};

/* Variable which holds the status of ad7156 CH1 specific init/deinitialisation */
static uint8_t gsTopTouchInitFlag = 0;

#define TOUCH_USER_MAX (2)
static Send_touch_func touch_user_handle[TOUCH_USER_MAX] = {NULL};

void Register_touch_send_func(Send_touch_func hander)
{
    for(int i = 0;i<TOUCH_USER_MAX;i++)
    {
        if(NULL == touch_user_handle[i])
        {
            touch_user_handle[i] = hander;
            break;
        }
    }
}

void Unregister_touch_send_func(Send_touch_func hander)
{
    for(int i = 0;i<TOUCH_USER_MAX;i++)
    {
        if(hander == touch_user_handle[i])
        {
            touch_user_handle[i] = NULL;
            break;
        }
    }
}

void Clear_register_touch_func(void)
{
    for(int i = 0;i<TOUCH_USER_MAX;i++)
    {
        touch_user_handle[i] = NULL;
    }
}

void top_touch_func_set(uint8_t en)
{
    Ad7156_detect_set(AD7156_CHANNEL1,en);
}

static void out_pin_detect(uint8_t value)
{
    if(AD7156_TOUCH == value)
    {
        adi_osal_ThreadResumeFromISR(touch_task_handler);
    }
}

/** @brief   Top Touch Initialization
 * @details  Register handle for top touch application from AD7156 for backlight
 * Control
 * @param    None
 * @retval   0  --> success 1 --> fail(returned when it has been initialised already)
 */
int top_touch_init() {
  if (!gsTopTouchInitFlag) {
    Register_out1_pin_detect_func(out_pin_detect);
    top_touch_func_set(1);
    gsTopTouchInitFlag = 1;
    return 0;
  }
  return 1;
}

/** @brief    Top Touch Deinitialization
 * @details  Unregister handle for top touch application from AD7156 for backlight
 * enable
 * @param    None
 * @retval   0  --> success 1 --> fail(returned when it has been de-initialised already)
 */
int top_touch_deinit() {
  if (gsTopTouchInitFlag) {
    top_touch_func_set(0);
    Unregister_out1_pin_detect_func(out_pin_detect);
    gsTopTouchInitFlag = 0;

    return 0;
  }
  return 1;
}

/*
* @note: combination short press can't be detect, because it very hard to keep release at the same time.
         but we don't need short press, so ignore it for the moment.
*/
void touch_detect_thread(void * arg)
{
    ADI_OSAL_STATUS         err;

    static uint8_t touch_up_value = AD7156_RELEASE;
    static uint8_t last_touch_up_value = AD7156_RELEASE;
    static uint16_t touch_up_cnt = 0;

    /*static uint8_t touch_down_value = 0;
    static uint8_t last_touch_down_value = 0;
    static uint16_t touch_down_cnt = 0;*/
    UNUSED_PARAMETER(arg);

#ifdef ENABLE_TOP_TOUCH
    top_touch_init();
#else
    adi_osal_ThreadSuspend(NULL);
#endif
    while(true)
    {
        m2m2_hdr_t *p_in_pkt = NULL;
        m2m2_hdr_t *p_out_pkt = NULL;
        p_in_pkt = post_office_get(ADI_OSAL_TIMEOUT_NONE, APP_OS_CFG_TOUCH_TASK_INDEX);

        // We got an m2m2 message from the queue, process it.
        if(p_in_pkt != NULL)
        {
          // We got an m2m2 message from the queue, process it.
          PYLD_CST(p_in_pkt, _m2m2_app_common_cmd_t, p_in_cmd);
          // Look up the appropriate function to call in the function table
          for (int i = 0; i < TOP_TOUCH_APP_ROUTING_TBL_SZ; i++) {
            if (top_touch_app_routing_table[i].command == p_in_cmd->command) {
              p_out_pkt = top_touch_app_routing_table[i].cb_handler(p_in_pkt);
              break;
            }
          }
          post_office_consume_msg(p_in_pkt);
          if (p_out_pkt != NULL) {
            post_office_send(p_out_pkt, &err);
          }
          adi_osal_ThreadSuspend(NULL);
        }
#ifdef ENABLE_TOP_TOUCH
        else
        {
            /*up touch detect handle start*/
            touch_up_value = ad7156_out1_pin_status_get();//up touch
            if(last_touch_up_value == touch_up_value)
            {
                if(AD7156_RELEASE != touch_up_value)
                {
                    touch_up_cnt++;
                    if(touch_up_cnt > TOUCH_LONG_PRESS_TIMEOUT_MS)
                    {
                        touch_up_cnt = 0;
                        /*for(int i = 0;i<TOUCH_USER_MAX;i++)
                        {
                            if(NULL != touch_user_handle[i])*/
                            if(NULL != touch_user_handle[0])
                            {
                                //touch_user_handle[i](TOUCH_TOP_VALUE|(APP_TOUCH_LONG_PUSH << 4));
                                touch_user_handle[0](TOUCH_TOP_VALUE|(APP_TOUCH_LONG_PUSH << 4));
                            }
                        //}
                        touch_up_value = AD7156_RELEASE;//if detect the long press, then stop detect.
                    }
                }
            }
            else
            {
                if((touch_up_cnt > TOUCH_SHORT_PRESS_TIME_MS)&&(touch_up_value == AD7156_RELEASE))
                {
                    /*for(int i = 0;i<TOUCH_USER_MAX;i++)
                    {
                        if(NULL != touch_user_handle[i])*/
                        if(NULL != touch_user_handle[0])
                        {
                            //touch_user_handle[i](TOUCH_TOP_VALUE|(APP_TOUCH_RELEASE << 4));
                            touch_user_handle[0](TOUCH_TOP_VALUE|(APP_TOUCH_RELEASE << 4));
                        }

                    //}
                }
                touch_up_cnt = 0;
            }
            last_touch_up_value = touch_up_value;
            /*up touch detect handle end*/
#if 0
            /*down touch detect handle start*/
            touch_down_value = ad7156_out2_pin_status_get();//down touch
            if(last_touch_down_value == touch_down_value)
            {
                    touch_down_cnt++;
                    if(NULL != touch_user_handle[1])
                    {
                        //touch_user_handle[i](TOUCH_BUTTOM_VALUE|(APP_TOUCH_LONG_PUSH << 4));
                        touch_user_handle[1](touch_down_value);
                    }
            }
            else
            {
              touch_down_cnt=0;
            }
            last_touch_down_value = touch_down_value;
            /*down touch detect handle end*/
#endif
            if((AD7156_RELEASE == touch_up_value))//supend the task when no touch press
            {
                adi_osal_ThreadSuspend(NULL);
            }
            adi_osal_ThreadSleep(10);
        }//else of if(p_in_pkt != NULL)
#endif//ENABLE_TOP_TOUCH
    }
}

void touch_detect_init(void) {
  ADI_OSAL_STATUS eOsStatus;

  AD7156_Init();
  /* Create touch thread */
  touch_task_attributes.pThreadFunc = touch_detect_thread;
  touch_task_attributes.nPriority = APP_OS_CFG_TOUCH_TASK_PRIO;
  touch_task_attributes.pStackBase = &touch_task_stack[0];
  touch_task_attributes.nStackSize = APP_OS_CFG_TOUCH_TASK_STK_SIZE;
  touch_task_attributes.pTaskAttrParam = NULL;
  touch_task_attributes.szThreadName = "touch";
  touch_task_attributes.pThreadTcb = &touch_task_tcb;

  eOsStatus = adi_osal_MsgQueueCreate(&touch_task_msg_queue,NULL,
                                    9);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
      Debug_Handler();
  }
  else
  {
    update_task_queue_list(APP_OS_CFG_TOUCH_TASK_INDEX,touch_task_msg_queue);
  }

  eOsStatus = adi_osal_ThreadCreateStatic(&touch_task_handler,
                                    &touch_task_attributes);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
      Debug_Handler();
  }
}

void send_message_top_touch_task(m2m2_hdr_t *p_pkt) {
  ADI_OSAL_STATUS osal_result;
  osal_result = adi_osal_MsgQueuePost(touch_task_msg_queue,p_pkt);
  if (osal_result != ADI_OSAL_SUCCESS)
    post_office_consume_msg(p_pkt);
  vTaskResume((TaskHandle_t)touch_task_handler);
}

m2m2_hdr_t *ad7156_app_reg_access(m2m2_hdr_t *p_pkt) {
  M2M2_APP_COMMON_STATUS_ENUM_t status = M2M2_APP_COMMON_STATUS_ERROR;
  // Declare a pointer to access the input packet payload
  PYLD_CST(p_pkt, m2m2_sensor_common_reg_op_16_hdr_t, p_in_payload);
  // Allocate a response packet with space for the correct number of operations
  PKT_MALLOC(p_resp_pkt, m2m2_sensor_common_reg_op_16_hdr_t, p_in_payload->num_ops * sizeof(p_in_payload->ops[0]));
  if(NULL != p_resp_pkt)
  {
      PYLD_CST(p_resp_pkt, m2m2_sensor_common_reg_op_16_hdr_t, p_resp_payload);
      uint8_t  reg_data = 0;

      switch (p_in_payload->command) {
      case M2M2_SENSOR_COMMON_CMD_READ_REG_16_REQ:
        for (int i = 0; i < p_in_payload->num_ops; i++) {
          AD7156_GetRegisterValue((uint8_t *)&reg_data, (uint8_t)p_in_payload->ops[i].address, 1);
          status = M2M2_APP_COMMON_STATUS_OK;
          p_resp_payload->ops[i].address = p_in_payload->ops[i].address;
          p_resp_payload->ops[i].value = reg_data;
        }
        p_resp_payload->command = (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_SENSOR_COMMON_CMD_READ_REG_16_RESP;
        break;
      case M2M2_SENSOR_COMMON_CMD_WRITE_REG_16_REQ:
        for (int i = 0; i < p_in_payload->num_ops; i++) {
          AD7156_SetRegisterValue(p_in_payload->ops[i].value, (uint8_t)p_in_payload->ops[i].address, 1);
          status = M2M2_APP_COMMON_STATUS_OK;
          p_resp_payload->ops[i].address = p_in_payload->ops[i].address;
          p_resp_payload->ops[i].value = p_in_payload->ops[i].value;
        }
        p_resp_payload->command = (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_SENSOR_COMMON_CMD_WRITE_REG_16_RESP;
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
  }

  return p_resp_pkt;
}

static m2m2_hdr_t *ad7156_app_load_cfg(m2m2_hdr_t *p_pkt) {
  // Declare and malloc a response packet
  PKT_MALLOC(p_resp_pkt, m2m2_sensor_ad7156_resp_t, 0);
  // Declare a pointer to the response packet payload
  PYLD_CST(p_resp_pkt, m2m2_sensor_ad7156_resp_t, p_resp_payload);
  p_resp_payload->status = M2M2_APP_COMMON_STATUS_ERROR;

  if (!load_ad7156_cfg()) {  // Loads the device configuration
    p_resp_payload->status = M2M2_APP_COMMON_STATUS_OK;
  } else {
    p_resp_payload->status = M2M2_APP_COMMON_STATUS_ERROR;
  }

  p_resp_payload->command = M2M2_SENSOR_AD7156_COMMAND_LOAD_CFG_RESP;
  p_resp_pkt->src = p_pkt->dest;
  p_resp_pkt->dest = p_pkt->src;
  return p_resp_pkt;
}

/*************************/

#ifdef DCB
static m2m2_hdr_t *ad7156_dcb_command_read_config(m2m2_hdr_t *p_pkt)
{
    static uint16_t r_size = 0;
    uint32_t dcbdata[MAXAD7156DCBSIZE];
    M2M2_DCB_STATUS_ENUM_t status = M2M2_DCB_STATUS_ERR_NOT_CHKD;

    PKT_MALLOC(p_resp_pkt, m2m2_dcb_ad7156_data_t, 0);
    // Declare a pointer to the response packet payload
    PYLD_CST(p_resp_pkt, m2m2_dcb_ad7156_data_t, p_resp_payload);

    r_size = (uint16_t)MAXAD7156DCBSIZE;
    if(read_ad7156_dcb(&dcbdata[0],&r_size) == AD7156_DCB_STATUS_OK)
    {
        for(int i=0; i< r_size; i++)
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
    return p_resp_pkt;
}

static m2m2_hdr_t *ad7156_dcb_command_write_config(m2m2_hdr_t *p_pkt)
{
    M2M2_DCB_STATUS_ENUM_t status = M2M2_DCB_STATUS_ERR_NOT_CHKD;
    uint32_t dcbdata[MAXAD7156DCBSIZE];

    // Declare a pointer to access the input packet payload
    PYLD_CST(p_pkt, m2m2_dcb_ad7156_data_t, p_in_payload);
    PKT_MALLOC(p_resp_pkt, m2m2_dcb_ad7156_data_t, 0);
    // Declare a pointer to the response packet payload
    PYLD_CST(p_resp_pkt, m2m2_dcb_ad7156_data_t, p_resp_payload);

    for(int i=0; i<p_in_payload->size; i++)
      dcbdata[i] = p_in_payload->dcbdata[i];
    if(write_ad7156_dcb(&dcbdata[0],p_in_payload->size) == AD7156_DCB_STATUS_OK)
    {
        ad7156_set_dcb_present_flag(true);
        status = M2M2_DCB_STATUS_OK;
    }
    else
    {
        status = M2M2_DCB_STATUS_ERR_ARGS;
    }

    p_resp_payload->status = status;
    p_resp_payload->command = M2M2_DCB_COMMAND_WRITE_CONFIG_RESP;
    p_resp_payload->size = 0;
    for(int i=0; i<MAXAD7156DCBSIZE; i++)
      p_resp_payload->dcbdata[i] = 0;

    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
    return p_resp_pkt;
}

static m2m2_hdr_t *ad7156_dcb_command_delete_config(m2m2_hdr_t *p_pkt)
{
    M2M2_DCB_STATUS_ENUM_t status = M2M2_DCB_STATUS_ERR_NOT_CHKD;

    PKT_MALLOC(p_resp_pkt, m2m2_dcb_ad7156_data_t, 0);
    // Declare a pointer to the response packet payload
    PYLD_CST(p_resp_pkt, m2m2_dcb_ad7156_data_t, p_resp_payload);

    if(delete_ad7156_dcb() == AD7156_DCB_STATUS_OK)
    {
        ad7156_set_dcb_present_flag(false);
        status = M2M2_DCB_STATUS_OK;
    }
    else
    {
        status = M2M2_DCB_STATUS_ERR_ARGS;
    }

    p_resp_payload->status = status;
    p_resp_payload->command = M2M2_DCB_COMMAND_ERASE_CONFIG_RESP;
    p_resp_payload->size = 0;
    for(int i=0; i<MAXAD7156DCBSIZE; i++)
      p_resp_payload->dcbdata[i] = 0;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
    return p_resp_pkt;

}
#endif
