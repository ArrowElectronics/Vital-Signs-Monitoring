/**
    ***************************************************************************
    * @addtogroup Tasks
    * @{
    * @file         led_task.c
    * @author       ADI
    * @version      V1.0.0
    * @date         20-May-2015
    * @brief        Source file contains led task for wearable device framework.
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
 * Copyright (c) 2016 Analog Devices Inc.
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

/* Includes -----------------------------------------------------------------*/
#include <task_includes.h>
#include <includes.h>
#include <post_office.h>
#include <led_interface.h>
#include <led_task.h>

#define LED_PORT  ADI_GPIO_PORT2
#define LED_PIN_R ADI_GPIO_PIN_5
#define LED_PIN_G ADI_GPIO_PIN_4

#pragma location = "bank1_retained_ram"
uint8_t LedTaskStack[APP_OS_CFG_LED_TASK_STK_SIZE];
#pragma location = "bank1_retained_ram"
ADI_OSAL_THREAD_HANDLE LedTaskHandler;

#pragma location = "bank1_retained_ram"
#ifdef FreeRTOS_PM_ONLY
ADI_OSAL_STATIC_THREAD_ATTR LedTaskAttributes;
StaticTask_t LedTaskTcb;
#else
ADI_OSAL_THREAD_ATTR LedTaskAttributes;
#endif //FreeRTOS_PM_ONLY

ADI_OSAL_QUEUE_HANDLE  led_task_msg_queue = NULL;

static void set_led(M2M2_LED_PATTERN_ENUM_t r_pattern,
                    M2M2_LED_PATTERN_ENUM_t g_pattern,
                    M2M2_LED_PATTERN_ENUM_t b_pattern);
static void  LedTask(void *pArgument);

ADI_OSAL_MUTEX_HANDLE LEDControlLock;

void led_task_init(void) {

  ADI_OSAL_STATUS eOsStatus;
  /* Create led thread */
  LedTaskAttributes.pThreadFunc = LedTask;
  LedTaskAttributes.nPriority = APP_OS_CFG_LED_TASK_PRIO;
  LedTaskAttributes.pStackBase = &LedTaskStack[0];
  LedTaskAttributes.nStackSize = APP_OS_CFG_LED_TASK_STK_SIZE;
  LedTaskAttributes.pTaskAttrParam = NULL;
  LedTaskAttributes.szThreadName = "LedTask";
#ifndef FreeRTOS_PM_ONLY  
  LedTaskAttributes.nThreadQueueSize = 1;
  eOsStatus = adi_osal_ThreadCreate(&LedTaskHandler,
                                    &LedTaskAttributes);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
      Debug_Handler();
  }  
#else
  LedTaskAttributes.pThreadTcb = &LedTaskTcb;
  eOsStatus = adi_osal_MsgQueueCreate(&led_task_msg_queue,NULL,
                                    1);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
      Debug_Handler();
  }
  else
  {
    update_task_queue_list(APP_OS_CFG_LED_TASK_INDEX,led_task_msg_queue);
  }  
  
  eOsStatus = adi_osal_ThreadCreateStatic(&LedTaskHandler,
                                    &LedTaskAttributes);
  if (eOsStatus != ADI_OSAL_SUCCESS) {
      Debug_Handler();
  }  
#endif //FreeRTOS_PM_ONLY   

  /* Mutex to hold LED control */
  adi_osal_MutexCreate(&LEDControlLock);

}


void send_message_led_task(m2m2_hdr_t *p_pkt) {
#ifndef FreeRTOS_PM_ONLY
  adi_osal_ThreadQueuePost(LedTaskHandler, ADI_OSAL_OPT_POST_FIFO,
                         (void *)p_pkt,
                         p_pkt->length);
#else
ADI_OSAL_STATUS osal_result;  
  osal_result = adi_osal_MsgQueuePost(led_task_msg_queue,p_pkt);
  if (osal_result != ADI_OSAL_SUCCESS)
    Debug_Handler(); 
#endif //FreeRTOS_PM_ONLY
}

M2M2_LED_PATTERN_ENUM_t   r_led_pattern_set = M2M2_LED_PATTERN_OFF;
M2M2_LED_PATTERN_ENUM_t   g_led_pattern_set = M2M2_LED_PATTERN_SLOW_BLINK_DC_12;
M2M2_LED_PATTERN_ENUM_t   b_led_pattern_set = M2M2_LED_PATTERN_OFF;
  
/**
  * @brief  led task - control different board leds
  * @param  pArgument not used
  * @return None
  */
static void  LedTask(void *pArgument) {
  // Store the pattern as it was sent, so that we don't send bit-shifted patterns
  // in response to a GET command.

  M2M2_LED_PATTERN_ENUM_t   r_led_pattern = r_led_pattern_set;
  M2M2_LED_PATTERN_ENUM_t   g_led_pattern = g_led_pattern_set;
  M2M2_LED_PATTERN_ENUM_t   b_led_pattern = b_led_pattern_set;


  M2M2_LED_PRIORITY_ENUM_t  current_priority = M2M2_LED_PRIORITY_LOW;
  m2m2_hdr_t                    *pkt = NULL;
  m2m2_led_ctrl_t           *led_cmd = NULL;
  m2m2_hdr_t                    *response_mail = NULL;
  m2m2_led_ctrl_t           *response_cmd = NULL;
  ADI_OSAL_STATUS               err;

  adi_gpio_SetHigh(LED_PORT, LED_PIN_R);
  adi_gpio_SetHigh(LED_PORT, LED_PIN_G);

  while (1) {
    // Circular shift through the LED pattern
    r_led_pattern = (M2M2_LED_PATTERN_ENUM_t) ((r_led_pattern << 1) | (r_led_pattern >> (sizeof(r_led_pattern)*8 - 1)));
    g_led_pattern = (M2M2_LED_PATTERN_ENUM_t) ((g_led_pattern << 1) | (g_led_pattern >> (sizeof(g_led_pattern)*8 - 1)));
    b_led_pattern = (M2M2_LED_PATTERN_ENUM_t) ((b_led_pattern << 1) | (b_led_pattern >> (sizeof(b_led_pattern)*8 - 1)));
#ifndef FreeRTOS_PM_ONLY
    pkt = post_office_get(500);
#else
    pkt = post_office_get(500, APP_OS_CFG_LED_TASK_INDEX);
#endif //FreeRTOS_PM_ONLY    
    if (pkt != NULL) {
      led_cmd = (m2m2_led_ctrl_t*)&pkt->data[0];
      // Check if we got a request to set/get the LED state.
      if (led_cmd != NULL) {
        switch (led_cmd->command) {
          case M2M2_LED_COMMAND_SET: {
            if (led_cmd->priority >= current_priority) {
              current_priority = led_cmd->priority;
              r_led_pattern_set = led_cmd->r_pattern;
              r_led_pattern  = r_led_pattern_set;

              g_led_pattern_set = led_cmd->g_pattern;
              g_led_pattern  = g_led_pattern_set;

              b_led_pattern_set = led_cmd->b_pattern;
              b_led_pattern  = b_led_pattern_set;
            }
            break;
          } case M2M2_LED_COMMAND_GET: {
            response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_led_ctrl_t));
            if (response_mail != NULL) {
              response_cmd = (m2m2_led_ctrl_t*)&response_mail->data[0];
              response_mail->src = pkt->dest;
              response_mail->dest = pkt->src;
              response_mail->checksum = 0x0000;
              response_mail->length = sizeof(m2m2_hdr_t) + sizeof(m2m2_led_ctrl_t);
              response_cmd->command = M2M2_LED_COMMAND_GET;
              response_cmd->priority = current_priority;
              response_cmd->r_pattern = r_led_pattern_set;
              response_cmd->g_pattern = g_led_pattern_set;
              response_cmd->b_pattern = b_led_pattern_set;
              post_office_send(response_mail, &err);
            }
            break;
          } case M2M2_APP_COMMON_CMD_PING_REQ: {
              m2m2_app_common_ping_t  *ctrl = (m2m2_app_common_ping_t *)&pkt->data[0];
              ctrl->command = M2M2_APP_COMMON_CMD_PING_RESP;
              pkt->dest = pkt->src;
              pkt->src = M2M2_ADDR_SYS_LED_0;
              post_office_send(pkt, &err);
            }
            break;
        }
        post_office_consume_msg(pkt);
      }
    }
    set_led(r_led_pattern, g_led_pattern, b_led_pattern);
  }
}

static void set_led(M2M2_LED_PATTERN_ENUM_t r_pattern,
                    M2M2_LED_PATTERN_ENUM_t g_pattern,
                    M2M2_LED_PATTERN_ENUM_t b_pattern) {
  if (r_pattern & 0x01) {
    adi_gpio_SetLow(LED_PORT, LED_PIN_R);
  } else {
    adi_gpio_SetHigh(LED_PORT, LED_PIN_R);
  }

  if (g_pattern & 0x01) {
    adi_gpio_SetLow(LED_PORT, LED_PIN_G);
  } else {
    adi_gpio_SetHigh(LED_PORT, LED_PIN_G);
  }

  if (b_pattern & 0x01) {
    /* Turn OFF Blue LED is not available */
  } else {
    /* Turn ON Blue LED is not available */
  }

  return;
}

ADI_OSAL_STATUS set_led_pattern(M2M2_ADDR_ENUM_t src,
                                M2M2_LED_PATTERN_ENUM_t r_pattern,
                                M2M2_LED_PATTERN_ENUM_t g_pattern) {
  ADI_OSAL_STATUS err = ADI_OSAL_FAILED;
  m2m2_hdr_t *p_m2m2_ = NULL;
  m2m2_led_ctrl_t *p_led_ctrl;
  p_m2m2_ = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_led_ctrl_t));
  if (p_m2m2_ != NULL) {
    p_m2m2_->src  = src;
    p_m2m2_->dest = M2M2_ADDR_SYS_LED_0;
    p_led_ctrl       = (m2m2_led_ctrl_t *)&p_m2m2_->data[0];
    p_led_ctrl->command   = M2M2_LED_COMMAND_SET;
    p_led_ctrl->priority  = M2M2_LED_PRIORITY_LOW;
    p_led_ctrl->r_pattern = r_pattern;
    p_led_ctrl->g_pattern = g_pattern;
    p_led_ctrl->b_pattern = M2M2_LED_PATTERN_OFF;
    post_office_send(p_m2m2_, &err);
  }
  return err;
}

void get_led_pattern(M2M2_LED_PATTERN_ENUM_t *r_pattern, 
                     M2M2_LED_PATTERN_ENUM_t *g_pattern) {
                       
    *r_pattern = r_led_pattern_set;
    *g_pattern = g_led_pattern_set;          
}
              
/*@}*/  /* end of group Application_Task */
/**@}*/ /* end of group Tasks */
