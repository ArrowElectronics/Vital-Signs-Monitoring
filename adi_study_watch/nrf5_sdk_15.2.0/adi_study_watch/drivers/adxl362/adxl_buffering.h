/**
***************************************************************************
* @file         adxl_buffering.h
* @author       ADI
* @version
* @date         26-Jun-2019
* @brief        Header file of buffering scheme for ADI ADXL362 chip
*
***************************************************************************
* @attention
***************************************************************************
*/
/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2020 Analog Devices Inc.                                      *
* All rights reserved.                                                        *
*                                                                             *
* This source code is intended for the recipient only under the guidelines of *
* the non-disclosure agreement with Analog Devices Inc.                       *
*                                                                             *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,    *
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE *
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      *
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING     *
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER         *
* DEALINGS IN THE SOFTWARE.                                                   *
*                                                                             *
*                                                                             *
* This software is intended for use with the ADXL362 part                     *
* only                                                                        *
*                                                                             *
******************************************************************************/

#ifndef _ADXL_BUFFERING_H_
#define _ADXL_BUFFERING_H_
#include <string.h>
#include <adxl362.h>
#include <circular_buffer.h>
#define ADXL_TS_DATA_TYPE uint32_t
#define ADXL_FIFO_LEN                   MAX_WATER_MARK_SAMPLES
#define ADXL_DATA_BUFFER_SIZE           (ADXL_FIFO_LEN)
#define ADXL_TS_BUFFER_SIZE             ((ADXL_FIFO_LEN) * sizeof(ADXL_TS_DATA_TYPE))
#define ADXL_BYTES_PER_SAMPLE           6
#define ADXL_FIFOREAD_BUFSIZE           MAX_WATER_MARK_SAMPLES

void adxl_buff_init( void );
CIRC_BUFF_STATUS_t adxl_buff_get(uint8_t *p_data,
                                ADXL_TS_DATA_TYPE *p_timestamp,
                                uint32_t *p_data_len);
void adxl_buff_init(void);
void adxl_buff_reset(void);
int16_t adxl_read_data_to_buffer(void);
int16_t sync_adxl_read_data_to_buffer(uint16_t *pAdxlData, \
                                          uint32_t *pAdxlTimeStamp);
#endif
