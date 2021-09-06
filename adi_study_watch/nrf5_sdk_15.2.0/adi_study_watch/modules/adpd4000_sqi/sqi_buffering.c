/**
***************************************************************************
* @addtogroup Tasks
* @{
* @file         sqi_buffering.c
* @author       ADI
* @version      V1.0.0
* @date         09-June-2021
* @brief        Source file contains data buffering functions for SQI
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
* Copyright (c) 2021 Analog Devices Inc.
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
#ifdef ENABLE_SQI_APP

#include <stdint.h>
#include <circular_buffer.h>
#include <sqi_buffering.h>

circular_buffer_t g_sqi_adpd_data_buff;
/* ------------------------- Private variables ----------------------------- */
/* to store max 128 adpd samples in floating point format */
static uint8_t g_sqi_adpd_data_byte_buff[512];
/* ------------------------- Private Definition ----------------------------- */

void SqiAdpdDataBuffClear(uint32_t data_sample_sz) {
  /* reset the buffer */
  circular_buffer_reset(&g_sqi_adpd_data_buff, data_sample_sz);
}

void SqiAdpdDataBuffInit(uint32_t data_sample_sz) {
  /* Init adpd buffer for SQI */
  circular_buffer_init(&g_sqi_adpd_data_buff, &g_sqi_adpd_data_byte_buff[0], sizeof(g_sqi_adpd_data_byte_buff), data_sample_sz);
}

/* num_elements -> no. of elements in the buffer after a adding new sample in it */
CIRC_BUFF_STATUS_t sqi_adpd_buff_put(float *p_data, uint32_t* num_elements) {
  CIRC_BUFF_STATUS_t  status = CIRC_BUFF_STATUS_ERROR;

  /* put the adpd sample into the circular buffer */
  status = circular_buffer_put(&g_sqi_adpd_data_buff, p_data);
  if (status != CIRC_BUFF_STATUS_OK) {
    return status;
  }
  /* get the number of adpd samples currently in the circular buffer */
  *num_elements = g_sqi_adpd_data_buff.num_elements;
  return status;
}

/* num_elements -> no. of elements to get from the circular buffer */
CIRC_BUFF_STATUS_t sqi_adpd_buff_get(float **p_data, uint32_t num_elements) {
  return circular_buffer_get_chunk(&g_sqi_adpd_data_buff, (void**)p_data, num_elements);
}
#endif
