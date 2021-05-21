/**
***************************************************************************
* @addtogroup Tasks
* @{
* @file         sync_data_buffering.c
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
#ifdef ENABLE_PPG_APP
#include <stdint.h>
#include "sync_data_buffering.h"
#include "adpd4000_buffering.h"

typedef struct _g_adpd_buffer {
  circular_buffer_t data_buff;
  circular_buffer_t ts_buff;
} g_data_buffer_t;

/* ------------------------- Private variables ----------------------------- */
static g_data_buffer_t g_sync_adpd_buffer, g_sync_adxl_buffer;
//To hold 64 elements in adpd, adxl & ts circular buffers
static uint8_t g_sync_adpd_data_byte_buff[256];
static uint8_t g_sync_adpd_ts_byte_buff[256];
static uint8_t g_sync_adxl_data_byte_buff[384];
static uint8_t g_sync_adxl_ts_byte_buff[256];

/* ------------------------- Private Definition ----------------------------- */
//extern uint8_t gnSynxDataSetSize;

void SyncDataClear(uint32_t data_sample_sz) {
    // reset the buffer
  circular_buffer_reset(&g_sync_adpd_buffer.data_buff, data_sample_sz);
  circular_buffer_reset(&g_sync_adpd_buffer.ts_buff, sizeof(ADPD_TS_DATA_TYPE));
  circular_buffer_reset(&g_sync_adxl_buffer.data_buff, 6);
  circular_buffer_reset(&g_sync_adxl_buffer.ts_buff, sizeof(ADXL_TS_DATA_TYPE));
}

void SyncDataBufferInit(uint32_t data_sample_sz) {
  // Init adpd, adxl data & timestamp buffer
  circular_buffer_init(&g_sync_adpd_buffer.data_buff, &g_sync_adpd_data_byte_buff[0], sizeof(g_sync_adpd_data_byte_buff), data_sample_sz);
  circular_buffer_init(&g_sync_adpd_buffer.ts_buff, &g_sync_adpd_ts_byte_buff[0], sizeof(g_sync_adpd_ts_byte_buff), sizeof(ADPD_TS_DATA_TYPE));
  circular_buffer_init(&g_sync_adxl_buffer.data_buff, &g_sync_adxl_data_byte_buff[0], sizeof(g_sync_adxl_data_byte_buff), 6);
  circular_buffer_init(&g_sync_adxl_buffer.ts_buff, &g_sync_adxl_ts_byte_buff[0], sizeof(g_sync_adxl_ts_byte_buff), sizeof(ADXL_TS_DATA_TYPE));
}

CIRC_BUFF_STATUS_t sync_adpd_buff_get(uint32_t *p_data,
                                ADPD_TS_DATA_TYPE *p_timestamp) {
  CIRC_BUFF_STATUS_t  status = CIRC_BUFF_STATUS_ERROR;

  status = circular_buffer_get(&g_sync_adpd_buffer.ts_buff, (uint8_t*)p_timestamp);
  if (status != CIRC_BUFF_STATUS_OK) {
    return status;
  }
  return circular_buffer_get(&g_sync_adpd_buffer.data_buff, p_data);
}

CIRC_BUFF_STATUS_t sync_adxl_buff_get(uint16_t *p_data,
                                ADXL_TS_DATA_TYPE *p_timestamp) {
  CIRC_BUFF_STATUS_t  status = CIRC_BUFF_STATUS_ERROR;

  status = circular_buffer_get(&g_sync_adxl_buffer.ts_buff, (uint8_t*)p_timestamp);
    if (status != CIRC_BUFF_STATUS_OK) {
    return status;
  }
  return circular_buffer_get(&g_sync_adxl_buffer.data_buff, p_data);


}

CIRC_BUFF_STATUS_t sync_adxl_buff_put(uint16_t *p_data, ADXL_TS_DATA_TYPE *timestamp) {
  CIRC_BUFF_STATUS_t  status = CIRC_BUFF_STATUS_ERROR;

  status = circular_buffer_put(&g_sync_adxl_buffer.ts_buff, (uint8_t*)timestamp);
  if (status != CIRC_BUFF_STATUS_OK) {
    return status;
  }
  status = circular_buffer_put(&g_sync_adxl_buffer.data_buff, (uint32_t *)p_data);
  return status;

}

CIRC_BUFF_STATUS_t sync_adpd_buff_put(uint32_t *p_data, ADPD_TS_DATA_TYPE *timestamp) {
  CIRC_BUFF_STATUS_t  status = CIRC_BUFF_STATUS_ERROR;

//  if(gnSynxDataSetSize != g_sync_adpd_buffer.data_buff.element_sz) {
//    SyncDataClear(gnSynxDataSetSize);
//  }
  status = circular_buffer_put(&g_sync_adpd_buffer.ts_buff, (uint8_t*)timestamp);
  if (status != CIRC_BUFF_STATUS_OK) {
    return status;
  }
  return circular_buffer_put(&g_sync_adpd_buffer.data_buff, (uint32_t *)p_data);
}
#endif