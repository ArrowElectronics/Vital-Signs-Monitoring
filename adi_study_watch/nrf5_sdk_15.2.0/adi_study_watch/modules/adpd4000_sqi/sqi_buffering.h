 /**
 ***************************************************************************
 * @addtogroup Tasks
 * @{
 * @file         sqi_buffering.h
 * @author       ADI
 * @version      V1.0.0
 * @date         09-June-2021
 * @brief        header file for data buffering functions of SQI
 ***************************************************************************
 * @attention
 ***************************************************************************
 *
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
#ifndef _SQI_BUFFERING_H_
#define _SQI_BUFFERING_H_

#include <circular_buffer.h>

void SqiAdpdDataBuffClear(uint32_t data_sample_sz);
void SqiAdpdDataBuffInit(uint32_t data_sample_sz);
/* num_elements -> no. of elements in the buffer after a adding new sample in it */
CIRC_BUFF_STATUS_t sqi_adpd_buff_put(float *p_data, uint32_t* num_elements);

/* num_elements -> no. of elements to get from the circular buffer */
CIRC_BUFF_STATUS_t sqi_adpd_buff_get(float **p_data, uint32_t num_elements);
#endif // _SQI_BUFFERING_H_