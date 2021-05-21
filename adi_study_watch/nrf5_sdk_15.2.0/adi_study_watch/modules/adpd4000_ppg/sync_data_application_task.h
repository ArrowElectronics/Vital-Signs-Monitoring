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
#ifndef _ADI_SYNC_PPG_TASK_H_
#define _ADI_SYNC_PPG_TASK_H_

#include <includes.h>
#include <stdint.h>
#include <stddef.h>
#include <m2m2_core.h>
#include "sync_data_application_interface.h"


#define  APP_OS_CFG_SYNC_DATA_APPLICATION_TASK_PRIO          6
#define  APP_OS_CFG_SYNC_DATA_APPLICATION_TASK_STK_SIZE      1024

typedef  m2m2_hdr_t*(sync_ppg_cb_function_t)(m2m2_hdr_t*);
/*!
 * @brief:  The dictionary type for mapping addresses to callback handlers.
 */
typedef struct _sync_ppg_routing_table_entry_t {
  uint8_t                    command;
  sync_ppg_cb_function_t          *cb_handler;
}sync_ppg_routing_table_entry_t;

void sync_data_application_task_init(void);
void send_message_sync_data_application_task(m2m2_hdr_t *p_pkt);
#endif // _ADI_SYNC_PPG_TASK_H_