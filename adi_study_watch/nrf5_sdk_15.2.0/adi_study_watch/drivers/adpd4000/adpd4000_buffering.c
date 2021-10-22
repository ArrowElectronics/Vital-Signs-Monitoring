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

extern uint32_t nWriteSequence;               //!< Stores the slot number
extern uint8_t  nFifoStatusByte;              //!< Number of FIFO status bytes stored after data
extern uint16_t decimation_info[SLOT_NUM];    //!< Buffer storing decimation of slots
extern uint32_t gnLcmValue;                   //!< Hold LCM value
extern adpd400xDrv_slot_t gsSlot[SLOT_NUM];   //!< Slot information
extern uint8_t gnAdpdFifoWaterMark;
#ifdef ENABLE_DEBUG_STREAM
extern uint8_t g_adpdOffset;
extern uint32_t g_adpd_debugInfo[20];
#endif
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
static void adpd_rearrange_data(uint16_t slot_data_sz, uint8_t *tmp_data_buff);

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

  uint8_t   tmp_data_buff[256+SPI_DUMMY_BYTES];  //!< 256 data bytes + 2 dummy bytes for SPI transfer
#ifdef ADPD_SEM_CORRUPTION_DEBUG
  extern uint8_t *p_slot_bkup;
  extern uint8_t *p_slot_data_sz_0;
  extern uint32_t gHighest_slot_num;
  extern uint8_t *pAdpdSemPtr;
  extern ADI_OSAL_SEM_HANDLE adpd4000_task_evt_sem;
  extern uint32_t gnSamples_in_fifo;
  extern uint32_t gnBytes_in_fifo;
#endif
/**
  * @brief  Read data out from hw FIFO to circular buffer, rearrange byte order
            See data sheet for explanation.
  * @param  p_slot_sz pointer to array of Output data size of each slot
  * @param  p_max_slot highest slot number used
  * @param  p_ch_num pointer to array of number of channels used from each slot
  * @retval uint8_t 1: SUCCESS, 0: ERROR (when number of samples in fifo is zero)
  */
uint8_t adpd4000_read_data_to_buffer(uint16_t *p_slot_sz, uint16_t *p_max_slot, uint16_t *p_ch_num) {
  ADPD_TS_DATA_TYPE  current_ts = 0;
  uint16_t  sample_interval = 0;
  uint16_t  nbytes_in_fifo = 0;
  uint16_t  highest_slot_num = 0;
  uint16_t  slot_data_sz[SLOT_NUM] = {0}, slot_channel_num[SLOT_NUM] = {1};
  uint16_t  is_active_slots = 0;
  uint16_t  nsamples_in_fifo = 0;
  uint8_t   i, each_slot_size, dataOffset, nWriteBufferOffset;
  uint8_t j, loop_size = 0;
  uint16_t  ODR = 0;
  uint16_t  drv_status;
  //uint8_t   tmp_data_buff[256] = {0};
  uint8_t   naDataCopyBuffer[256] = {0};
  uint16_t total_slot_size = 0;

  Adpd400xDrvGetParameter(ADPD400x_TIMEGAP, 0, &ODR);
  Adpd400xDrvGetParameter(ADPD400x_FIFOLEVEL, 0, &nbytes_in_fifo);
  Adpd400xDrvGetParameter(ADPD400x_HIGHEST_SLOT_NUM, 0, &highest_slot_num);
  Adpd400xDrvGetParameter(ADPD400x_SUM_SLOT_DATASIZE, 0, &total_slot_size);

  if (total_slot_size == 0 || nbytes_in_fifo == 0)
    return 0;

  for (i=0; i<=highest_slot_num; i++)  {
    Adpd400xDrvGetParameter(ADPD400x_THIS_SLOT_DATASIZE, i, &slot_data_sz[i]);
    Adpd400xDrvGetParameter(ADPD400x_THIS_SLOT_CHANNEL_NUM, i, &slot_channel_num[i]);
    Adpd400xDrvGetParameter(ADPD400x_IS_SLOT_ACTIVE, i, &is_active_slots);
    //slot_data_sz[i] |= (is_active_slots<<15);
    *p_ch_num = slot_channel_num[i] & 0xFF;
    p_ch_num++;
    // warning: check i>highest_slot_num, slot_data_sz[i] are 0
  }

  //total_slot_size += nFifoStatusByte;
  get_current_datapattern(&nWriteSequence, gsSlot,
                                gnLcmValue, highest_slot_num,
                                  &sWriteBufferPattern);
  nPreviousSeqNo = nWriteSequence;
  // Update total sample size with Optional status bytes

  *p_max_slot = highest_slot_num;
#ifdef ADPD_SEM_CORRUPTION_DEBUG
  p_slot_bkup = (uint8_t *) p_slot_sz;
  p_slot_data_sz_0 = (uint8_t *) &slot_data_sz[0];
  gHighest_slot_num = highest_slot_num;
#endif
  memcpy(p_slot_sz, &slot_data_sz[0], (highest_slot_num+1)*2);
#ifdef ADPD_SEM_CORRUPTION_DEBUG
  gnBytes_in_fifo = nbytes_in_fifo;
#endif
  if(gnAdpdFifoWaterMark == 0)
    return 0;

  if (prev_ADPD_ts != 0) {
    if(gADPD4000_dready_ts > prev_ADPD_ts) {
      // Calculate the number of timestamp units between each sample
#ifdef ADPD_SEM_CORRUPTION_DEBUG
      if(nsamples_in_fifo)
        gnSamples_in_fifo = nsamples_in_fifo;
      else
        gnSamples_in_fifo = nbytes_in_fifo;
#endif
      sample_interval = (uint16_t)((gADPD4000_dready_ts - prev_ADPD_ts) / gnAdpdFifoWaterMark);
    }
    else // handle day roll-over after 24hrs
    {
      /* MAX_RTC_TICKS_FOR_24_HOUR:- Max RTC Count value returned by get_sensor_timestamp() after 24hrs.
        Adding that value to have sample interval value during day roll-over */
      sample_interval = (uint16_t)((MAX_RTC_TICKS_FOR_24_HOUR + gADPD4000_dready_ts- prev_ADPD_ts) / gnAdpdFifoWaterMark);
    }
  } else {
    sample_interval = (uint16_t) (ODR * RTC_TICKS_PER_MILLI_SEC);
  }
#ifdef ENABLE_DEBUG_STREAM
  g_adpdOffset = 0;
  g_adpd_debugInfo[g_adpdOffset++] = gnAdpdFifoWaterMark;
  g_adpd_debugInfo[g_adpdOffset++] = sample_interval;
  g_adpd_debugInfo[g_adpdOffset++] = prev_ADPD_ts;
  g_adpd_debugInfo[g_adpdOffset++] = gADPD4000_dready_ts;
#endif
  //NRF_LOG_INFO("*********gADPD4000_dready_ts=%d,prev_ADPD_ts=%d, nsamples_in_fifo=%d\
  // Sample interval inside firmware=%d*******",gADPD4000_dready_ts,
  // prev_ADPD_ts,nsamples_in_fifo,sample_interval);
  prev_ADPD_ts = gADPD4000_dready_ts;
  current_ts = gADPD4000_dready_ts;

//  if (total_slot_size != g_adpdcl_buffer.data_buff.element_sz) {
//    // If the data sample size has been changed, reset the buffer with the new data size
//    adpdCl_buff_reset(total_slot_size);
//  }

  // Fetch one sample at a time from the device
  while (nbytes_in_fifo >= (sWriteBufferPattern.sample_size + nFifoStatusByte)) {
    memset(&naDataCopyBuffer[0], 0x00, sizeof(naDataCopyBuffer));
    nPreviousSeqNo = nWriteSequence;
    drv_status = Adpd400xDrvReadFifoData(&tmp_data_buff[0], (sWriteBufferPattern.sample_size + nFifoStatusByte));
    nbytes_in_fifo -= (sWriteBufferPattern.sample_size + nFifoStatusByte);
    if (drv_status == ADPD400xDrv_SUCCESS) {
      // Unpack the data packet based on the sample size (0-12 bytes)
      dataOffset = 0 + SPI_DUMMY_BYTES;
      nWriteBufferOffset = 0;
      for (i=0; i<=highest_slot_num; i++)  {

        if((slot_data_sz[i] & 0x0100) == 0) {
         /* Get the Lit, Dart & Signal size from 16-bit array element
          *---------*---------*--------*--------*
          * Lit     * Impulse * Dark   * Signal *
          *---------*---------*--------*--------*
          * [15:12] * [11:8]  * [7:4]  * [3:0]  *
          *---------*---------*--------*--------*
          */
          each_slot_size = ((slot_data_sz[i] >> 12) & 0xF) +
                              ((slot_data_sz[i] >> 4) & 0xF) +
                                (slot_data_sz[i] & 0xF);
        } else {
          each_slot_size = 2;
        }

        if ((each_slot_size == 0) || (gsSlot[i].activeSlot == 0))
          continue;                 // not an active slot

        if(sWriteBufferPattern.slot_info[i] == '0'){
          //if the slot is skipped, increment the dataoffset of put buffer and iterate to nextslot
          nWriteBufferOffset += each_slot_size;
          continue;
        }

        // if Impulse Mode, do the below in different way
        if(slot_data_sz[i] & 0x100)
        {
          loop_size = slot_data_sz[i] & 0xff;
          // Impulse mode do it in the different way
          for(j = 0; j < loop_size; j++) {
            adpd_rearrange_data(each_slot_size, &tmp_data_buff[dataOffset]);
          }
        } else {
          adpd_rearrange_data(slot_data_sz[i], &tmp_data_buff[dataOffset]);
        }

        // copy the non-skipped slot data to put data buffer
        memcpy(&naDataCopyBuffer[nWriteBufferOffset], &tmp_data_buff[dataOffset], each_slot_size);
        nWriteBufferOffset += each_slot_size;

        if (slot_channel_num[i] == 3)   {   // for 2nd channel
          dataOffset += each_slot_size;
          adpd_rearrange_data(slot_data_sz[i], &tmp_data_buff[dataOffset]);
          // copy the non-skipped slot data to put data buffer
          memcpy(&naDataCopyBuffer[nWriteBufferOffset], &tmp_data_buff[dataOffset], each_slot_size);
          nWriteBufferOffset += each_slot_size;
        }
        dataOffset += each_slot_size;
      }   // end num of slots
      memcpy(&naDataCopyBuffer[nWriteBufferOffset], &tmp_data_buff[dataOffset], nFifoStatusByte);
      // in future, should change to packetize data before putting into buffer
      adpd_buff_put((uint8_t*)&naDataCopyBuffer[0], &current_ts);
      current_ts += sample_interval;
      current_ts = (uint32_t)(current_ts % MAX_RTC_TICKS_FOR_24_HOUR);
      // Get next sequence pattern
      get_current_datapattern(&nWriteSequence,gsSlot,
                                gnLcmValue, highest_slot_num,
                                  &sWriteBufferPattern);
    }else{
       return 0; //error in FIFO data read
	}
  }   // end while
  // Need to decrement the nWriteSequence to previous Seq no if nbytesinFIFO not sufficient
  nWriteSequence = nPreviousSeqNo;
  return 1;
}

/**
  * @brief  Rearrange byte order of the data read from FIFO
  * @param  slot_data_sz Slot size
  * @param  tmp_data_buff Data buffer pointer
  * @retval None.
  * @note   Data format notation formed like this LSB_L MSB_L LSB_H MSB_H[0:31]
  */
static void adpd_rearrange_data(uint16_t slot_data_sz, uint8_t *tmp_data_buff)  {
  uint8_t j, bytePtr, tmp_byte[2];
  bytePtr = 0;
  for (j=4; j==0||j==4; j-=4)  {    // Dark, Signal
    switch((slot_data_sz>>j)&0xF) {
    case 1:     // do nothing
      bytePtr += 1;
      break;
    case 2:     // do byte swap
      tmp_byte[0] = tmp_data_buff[bytePtr];
      // Swap the data byte in the order [7:0] [15:8]
      tmp_data_buff[bytePtr] = tmp_data_buff[bytePtr+1];    // 0=>1
      tmp_data_buff[bytePtr+1] = tmp_byte[0];               // 1=>0
      bytePtr += 2;
      break;
    case 3:     // do byte swap
      tmp_byte[0] = tmp_data_buff[bytePtr];
      // Swap the data byte in the order [7:0] [15:8] [23:16]
      tmp_data_buff[bytePtr] = tmp_data_buff[bytePtr+1];    // 1=>0
      tmp_data_buff[bytePtr+1] = tmp_byte[0];               // 0=>1
      bytePtr += 3;
      break;
    case 4:     // do byte swap
      tmp_byte[0] = tmp_data_buff[bytePtr];
      tmp_byte[1] = tmp_data_buff[bytePtr+2];
      // Swap the data byte in the order [7:0] [15:8] [23:16] [31:24]
      tmp_data_buff[bytePtr] = tmp_data_buff[bytePtr+1];    // 1=>0
      tmp_data_buff[bytePtr+1] = tmp_byte[0];               // 0=>1
      tmp_data_buff[bytePtr+2] = tmp_data_buff[bytePtr+3];  // 3=>2
      tmp_data_buff[bytePtr+3] = tmp_byte[1];               // 2=>3
      bytePtr += 4;
    default:
      break;
    }
  }   // end for j

  // Process Lit data from soft-fifo buffer
  if((slot_data_sz & 0x100) != 0x100) {
    for (j=12; j==12; j-=12)  {    // Lit
      switch((slot_data_sz>>j)&0xF) {
      case 1:     // do nothing
        bytePtr += 1;
        break;
      case 2:     // do byte swap
        tmp_byte[0] = tmp_data_buff[bytePtr];
        // Swap the data byte in the order [7:0] [15:8]
        tmp_data_buff[bytePtr] = tmp_data_buff[bytePtr+1];    // 0=>1
        tmp_data_buff[bytePtr+1] = tmp_byte[0];               // 1=>0
        bytePtr += 2;
        break;
      case 3:     // do byte swap
        tmp_byte[0] = tmp_data_buff[bytePtr];
        // Swap the data byte in the order [7:0] [15:8] [23:16]
        tmp_data_buff[bytePtr] = tmp_data_buff[bytePtr+1];    // 1=>0
        tmp_data_buff[bytePtr+1] = tmp_byte[0];               // 0=>1
        bytePtr += 3;
        break;
      case 4:     // do byte swap
        tmp_byte[0] = tmp_data_buff[bytePtr];
        tmp_byte[1] = tmp_data_buff[bytePtr+2];
        // Swap the data byte in the order [7:0] [15:8] [23:16] [31:24]
        tmp_data_buff[bytePtr] = tmp_data_buff[bytePtr+1];    // 1=>0
        tmp_data_buff[bytePtr+1] = tmp_byte[0];               // 0=>1
        tmp_data_buff[bytePtr+2] = tmp_data_buff[bytePtr+3];  // 3=>2
        tmp_data_buff[bytePtr+3] = tmp_byte[1];               // 2=>3
        bytePtr += 4;
      default:
        break;
      }
    }   // end for j
  }
}
