/**
*******************************************************************************
* @file         adpd4000_buffering.c
* @author       ADI
* @version
* @date         26-Jun-2019
* @brief        Buffering scheme to get data from ADI ADPD400x chip
*
*******************************************************************************
* @attention
*******************************************************************************
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
* This software is intended for use with the ADPD400x part only               *
*                                                                             *
******************************************************************************/

#include <adpd4000_buffering.h>
#include <hw_if_config.h>
#include "mw_ppg.h"

#include "nrf_log_ctrl.h"

/* System Task Module Log settings */
#include "nrf_log.h"
//NRF_LOG_MODULE_REGISTER();
#include "lcm.h"
#include "rtc.h"

// Size of the ADPD device FIFO
#define ADPD_FIFO_LEN 512              //!< HW FIFO Size in bytes

/* Make the timestamp buffer large enough to hold the same number of timestamps
 as the largest number of samples that can be in the ADPD FIFO
 ADPD_TS_BUFFER_SIZE = ((ADPD_DATA_BUFFER_SIZE/SMALLEST_ADPD_SAMPLE_SIZE_NBYTES) * sizeof(TIMESTAMP TYPE))
*/
#define ADPD_DATA_BUFFER_SIZE (ADPD_FIFO_LEN)   //!< Software data buffer size
#define ADPD_TS_BUFFER_SIZE ((ADPD_FIFO_LEN/2) * sizeof(ADPD_TS_DATA_TYPE)) //!< Timestamp buffer size

ADPD_TS_DATA_TYPE  gADPD4000_dready_ts = 0;   //!< Timestamp of data at interrupt
/******************Public variables**************************************/
uint32_t nWriteSequence;               //!< Stores the slot number
uint8_t  nFifoStatusByte;              //!< Number of FIFO status bytes stored after data
uint16_t decimation_info[SLOT_NUM];    //!< Buffer storing decimation of slots
extern uint32_t gnLcmValue;                   //!< Hold LCM value
uint8_t gnAdpdFifoWaterMark;
/*------------------------- Public Function Prototypes -----------------------*/

/*------------------------- Private Variables --------------------------------*/
static uint64_t prev_ADPD_ts = 0;             //!< Timestamp of data at previous interrupt
static uint8_t g_adpdcl_data_byte_buff[ADPD_DATA_BUFFER_SIZE]; //!< Buffer holding data
static uint8_t g_adpdcl_ts_byte_buff[ADPD_TS_BUFFER_SIZE];     //!< Buffer holding timestamp
/*!
 * @brief:  Buffer type to store ADPD data and its timestamps
 */
static struct _g_adpdcl_buffer {
  circular_buffer_t data_buff;        //!< Circular data buffer
  circular_buffer_t ts_buff;          //!< Circular timestamp buffer
} g_adpdcl_buffer;                    //!<  Buffer type to store ADPD data and its timestamps
static fifo_data_pattern sWriteBufferPattern; //!< Write pattern for slot information and sample size
static uint32_t nPreviousSeqNo = 1;   //!< Holds previous slot sequence number
/*------------------------- Private Function Prototypes ----------------------*/

/** @brief  ADPD Buffer initialization
*
* @param  data_sample_sz sample size of ADPD data
* @return None
*/
void adpd4000_buff_init(uint32_t data_sample_sz) {
  circular_buffer_init(&g_adpdcl_buffer.data_buff, &g_adpdcl_data_byte_buff[0], sizeof(g_adpdcl_data_byte_buff), data_sample_sz);
  circular_buffer_init(&g_adpdcl_buffer.ts_buff, &g_adpdcl_ts_byte_buff[0], sizeof(g_adpdcl_ts_byte_buff), sizeof(ADPD_TS_DATA_TYPE));
  prev_ADPD_ts = 0;
}

/** @brief  ADPD Buffer reset
*
* @param  data_sample_sz sample size of ADPD data
* @return None
*/
void adpd4000_buff_reset(uint32_t data_sample_sz) {
  circular_buffer_reset(&g_adpdcl_buffer.data_buff, data_sample_sz);
  circular_buffer_reset(&g_adpdcl_buffer.ts_buff, sizeof(ADPD_TS_DATA_TYPE));
  prev_ADPD_ts = 0;
}

/** @brief  Get data data and timestamp from circular buffer
*
* @param  p_data pointer to the data
* @param  p_timestamp pointer to the timestamp
* @param  p_data_len data size of ADPD sample
* @return CIRC_BUFF_STATUS_t status of reading from circular buffer
*/
CIRC_BUFF_STATUS_t adpd4000_buff_get(uint8_t *p_data,
                                ADPD_TS_DATA_TYPE *p_timestamp,
                                uint32_t *p_data_len) {
  CIRC_BUFF_STATUS_t  status = CIRC_BUFF_STATUS_ERROR;

  if (*p_data_len < g_adpdcl_buffer.data_buff.element_sz) {
    return CIRC_BUFF_STATUS_OVERFLOW;
  }
  *p_data_len = g_adpdcl_buffer.data_buff.element_sz;
  status = circular_buffer_get(&g_adpdcl_buffer.ts_buff, (uint8_t*)p_timestamp);
  if (status != CIRC_BUFF_STATUS_OK) {
    return status;
  }
  return circular_buffer_get(&g_adpdcl_buffer.data_buff, p_data);
}

/** @brief  Put data and timestamp into circular buffer
*
* @param  p_data pointer to the data
* @param  p_timestamp pointer to the timestamp
* @return CIRC_BUFF_STATUS_t status of putting data and timestamp into circular buffer
*/
static CIRC_BUFF_STATUS_t adpd_buff_put(uint8_t *p_data, ADPD_TS_DATA_TYPE *p_timestamp) {
  CIRC_BUFF_STATUS_t  status = CIRC_BUFF_STATUS_ERROR;

  status = circular_buffer_put(&g_adpdcl_buffer.data_buff, p_data);
  if (status != CIRC_BUFF_STATUS_OK) {
    return status;
  }
  return circular_buffer_put(&g_adpdcl_buffer.ts_buff, (uint8_t*)p_timestamp);
}

#ifdef ADPD_SEM_CORRUPTION_DEBUG
  extern uint8_t *p_slot_bkup;
  extern uint8_t *p_slot_data_sz_0;
  extern uint32_t gHighest_slot_num;
  extern uint8_t *pAdpdSemPtr;
  extern ADI_OSAL_SEM_HANDLE adpd4000_task_evt_sem;
  extern uint32_t gnSamples_in_fifo;
  extern uint32_t gnBytes_in_fifo;
#endif
