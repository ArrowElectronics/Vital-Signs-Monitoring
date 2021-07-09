/**
***************************************************************************
* @file         adxl_buffering.c
* @author       ADI
* @version
* @date         26-Jun-2019
* @brief        Buffering scheme to get data from ADI ADXL362 chip
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

#include <adxl_buffering.h>
#include <app_common.h>
#include <hw_if_config.h>
#include "nrf_log_ctrl.h"
#include <rtc.h>

/* System Task Module Log settings */
#include "nrf_log.h"
ADXL_TS_DATA_TYPE  gADXL_dready_ts = 0;
static uint8_t g_adXL_ts_byte_buff[ADXL_TS_BUFFER_SIZE];
//static int16_t gnXDataArray[ADXL_FIFOREAD_BUFSIZE], gnYDataArray[ADXL_FIFOREAD_BUFSIZE],gnZDataArray[ADXL_FIFOREAD_BUFSIZE];
static int8_t g_adxl_data_byte_buff[ADXL_DATA_BUFFER_SIZE*ADXL_BYTES_PER_SAMPLE];
static struct _g_adxl_buffer {
  circular_buffer_t data_buff;
  circular_buffer_t ts_buff;
} g_adxl_buffer;

static uint64_t prev_ADXL_ts = 0;
extern ADXL_TS_DATA_TYPE gsyncADXL_dready_ts;
void adxl_buff_init(void)
{
   uint32_t element_sz=6;//minimum one samples to be store(LSB+MSB)
   circular_buffer_init(&g_adxl_buffer.data_buff, (uint8_t *)&g_adxl_data_byte_buff[0], sizeof(g_adxl_data_byte_buff),element_sz);
   circular_buffer_init(&g_adxl_buffer.ts_buff, &g_adXL_ts_byte_buff[0], sizeof(g_adXL_ts_byte_buff), sizeof(ADXL_TS_DATA_TYPE));
 // return circular_buffer_init( capacity , data_sample_sz);
   prev_ADXL_ts = 0;
}

void adxl_buff_reset(void) {
  circular_buffer_reset(&g_adxl_buffer.data_buff, 6);//(3axis*(LSB+MSB=2))
  circular_buffer_reset(&g_adxl_buffer.ts_buff, sizeof(ADXL_TS_DATA_TYPE));
  prev_ADXL_ts = 0;
}

CIRC_BUFF_STATUS_t adxl_buff_get(uint8_t *p_data,
                                ADXL_TS_DATA_TYPE *p_timestamp,
                                uint32_t *p_data_len) {
  CIRC_BUFF_STATUS_t  status = CIRC_BUFF_STATUS_ERROR;

  if (*p_data_len < g_adxl_buffer.data_buff.element_sz) {
    return CIRC_BUFF_STATUS_OVERFLOW;
  }
  *p_data_len = g_adxl_buffer.data_buff.element_sz;
  status = circular_buffer_get(&g_adxl_buffer.ts_buff, (uint8_t*)p_timestamp);
    if (status != CIRC_BUFF_STATUS_OK) {
    return status;
  }
  // status = circular_buffer_get(&g_adxl_buffer.ts_buff, (uint8_t*)p_timestamp);
  return circular_buffer_get(&g_adxl_buffer.data_buff, p_data);


}

static CIRC_BUFF_STATUS_t adxl_buff_put(uint8_t *p_data, ADXL_TS_DATA_TYPE *timestamp) {
  CIRC_BUFF_STATUS_t  status = CIRC_BUFF_STATUS_ERROR;
  status = circular_buffer_put(&g_adxl_buffer.ts_buff, (uint8_t*)timestamp);
  if (status != CIRC_BUFF_STATUS_OK) {
    return status;
  }
  status = circular_buffer_put(&g_adxl_buffer.data_buff, p_data);
  return status;

}

int16_t adxl_read_data_to_buffer(void)
{
  uint8_t pnData[6]={0,0,0,0,0,0};
  int16_t nRetValue;
  uint32_t nIndexCnt;
  uint16_t nAdxlFifoLevelSize = 0;
  uint16_t nODR = 0;
  ADXL_TS_DATA_TYPE  current_ts = 0;
  uint16_t  sample_interval = 0;
  uint16_t pnXData[ADXL_FIFOREAD_BUFSIZE] = {0}; // Temporary X Data array
  uint16_t pnYData[ADXL_FIFOREAD_BUFSIZE] = {0}; // Temporary Y Data array
  uint16_t pnZData[ADXL_FIFOREAD_BUFSIZE] = {0}; // Temporary Z Data array
  AdxlDrvGetParameter(ADXL_FIFOLEVEL, &nAdxlFifoLevelSize);
  GetAdxlOutputRate(&nODR);

  if (prev_ADXL_ts != 0) {
    // Calculate the number of timestamp units between each sample
   if(gADXL_dready_ts < prev_ADXL_ts) // handle day roll-over after 24hrs
    {
       /* MAX_RTC_TICKS_FOR_24_HOUR:- Max RTC Count value returned by get_sensor_timestamp() after 24hrs.
        Adding that value to have correct sample_interval during day roll-over */
      sample_interval = (uint16_t)((MAX_RTC_TICKS_FOR_24_HOUR + gADXL_dready_ts - prev_ADXL_ts) / nAdxlFifoLevelSize);
    }
    else
    {
       sample_interval = (uint16_t)((gADXL_dready_ts - prev_ADXL_ts) / nAdxlFifoLevelSize);
    }
   }
  else
  {
    if(nODR == 12)  /*fractional part of 12.5 Hz is required for calculating the sample interval*/
      sample_interval = (uint16_t)((1000.0/12.5) * RTC_TICKS_PER_MILLI_SEC);//32KHz ticks resolution
    else
      sample_interval = (uint16_t)((1000.0/nODR) * RTC_TICKS_PER_MILLI_SEC);//32KHz ticks resolution
  }
  //NRF_LOG_INFO("FIFO_LEVEL:%d",nAdxlFifoLevelSize);
  //NRF_LOG_INFO("gADXL_dready_ts=%d,prev_ADXL_ts=%d,\
  //sample_interval=%d",gADXL_dready_ts,prev_ADXL_ts,sample_interval);

  prev_ADXL_ts = gADXL_dready_ts;
  current_ts = gADXL_dready_ts;

  while(nAdxlFifoLevelSize >= ADXL_FIFOREAD_BUFSIZE)
  {
    //nRetValue = AdxlDrvReadFifoData(&gnXDataArray[0],&gnYDataArray[0], &gnZDataArray[0], ADXL_FIFOREAD_BUFSIZE);
    nRetValue = AdxlDrvReadFifoData(&pnXData[0],&pnYData[0], &pnZData[0], ADXL_FIFOREAD_BUFSIZE);
    if (nRetValue == ADXLDrv_SUCCESS)
    {
      for (nIndexCnt = 0; nIndexCnt < ADXL_FIFOREAD_BUFSIZE; nIndexCnt++)
      {
        *(uint16_t *)&pnData[0] = pnXData[nIndexCnt];
        *(uint16_t *)&pnData[2] = pnYData[nIndexCnt];
        *(uint16_t *)&pnData[4] = pnZData[nIndexCnt];
        adxl_buff_put((uint8_t*)pnData,&current_ts);
        current_ts += sample_interval;
        current_ts = (uint32_t)(current_ts % MAX_RTC_TICKS_FOR_24_HOUR);
      }
    }
    nAdxlFifoLevelSize -= ADXL_FIFOREAD_BUFSIZE;
  }
  if(nAdxlFifoLevelSize)
  {
    //nRetValue = AdxlDrvReadFifoData(&gnXDataArray[0],&gnYDataArray[0], &gnZDataArray[0], nAdxlFifoLevelSize);
    nRetValue = AdxlDrvReadFifoData(&pnXData[0],&pnYData[0], &pnZData[0], nAdxlFifoLevelSize);
    if (nRetValue == ADXLDrv_SUCCESS)
    {
      for (nIndexCnt = 0; nIndexCnt < nAdxlFifoLevelSize; nIndexCnt++)
      {
        *(uint16_t *)&pnData[0] = pnXData[nIndexCnt];
        *(uint16_t *)&pnData[2] = pnYData[nIndexCnt];
        *(uint16_t *)&pnData[4] = pnZData[nIndexCnt];
        adxl_buff_put((uint8_t*)pnData,&current_ts);
        current_ts += sample_interval;
        current_ts = (uint32_t)(current_ts % MAX_RTC_TICKS_FOR_24_HOUR);
      }
    }
  }
  return 0;
}
