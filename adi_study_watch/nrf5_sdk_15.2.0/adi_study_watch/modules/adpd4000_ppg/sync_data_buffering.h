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
#ifndef _ADPD_ADXL_BUFFERING_H_
#define _ADPD_ADXL_BUFFERING_H_

#include <circular_buffer.h>
#include "adpd400x_drv.h"
#include "adxl362.h"
#include "app_sync.h"
#include "hw_if_config.h"
#include "app_common.h"
#include "adpd400x_lib_common.h"

#define ADPD_TS_DATA_TYPE uint32_t
#define ADXL_TS_DATA_TYPE uint32_t

CIRC_BUFF_STATUS_t sync_adpd_buff_get(uint32_t *p_data,
                                ADPD_TS_DATA_TYPE *p_timestamp);
CIRC_BUFF_STATUS_t sync_adxl_buff_get(uint16_t *p_data,
                                ADXL_TS_DATA_TYPE *p_timestamp);
CIRC_BUFF_STATUS_t sync_adxl_buff_put(uint16_t *p_data, ADXL_TS_DATA_TYPE *timestamp);
CIRC_BUFF_STATUS_t sync_adpd_buff_put(uint32_t *p_data, ADPD_TS_DATA_TYPE *timestamp);

void SyncDataClear(uint32_t data_sample_sz);
void SyncDataBufferInit(uint32_t data_sample_sz);
#endif // _ADPD_ADXL_BUFFERING_H_