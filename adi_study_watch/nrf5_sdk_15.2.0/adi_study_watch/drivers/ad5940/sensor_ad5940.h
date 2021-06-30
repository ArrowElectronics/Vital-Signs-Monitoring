/**
******************************************************************************
* @file     sensor_ad5940.h
* @author   ADI
* @version  V1.0.0
* @date     12-Nov-2017
* @brief    Include file for the AD5940 application internal functions
******************************************************************************
* @attention
******************************************************************************
*/
/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2016 Analog Devices Inc.                                      *
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
* This software is intended for use with the ADPD and derivative parts        *
* only                                                                        *
*                                                                             *
******************************************************************************/


#ifndef _SENSOR_AD5940_H_
#define _SENSOR_AD5940_H_
#define AD5940Drv_SUCCESS (0)
#define AD5940Drv_ERROR  (-1)

typedef enum AD5950_APP_ENUM_t {
  AD5940_APP_NONE = 0,
  AD5940_APP_ECG,
  AD5940_APP_EDA,
  AD5940_APP_BCM
} AD5950_APP_ENUM_t;

void Ad5940Init();
void Ad5940FifoCallBack(void);
void Ad5940RxBufferInit(void);
void ClearDataBufferAd5940(void);
uint8_t ReadAd5940DrvBcmData(uint32_t *pData, uint32_t *pTimeStamp);
uint8_t ReadAd5940DrvEdaData(uint32_t *pData, uint32_t *pTimeStamp);
int8_t ad5940_read_ecg_data_to_buffer();
int8_t ad5940_read_eda_data_to_buffer();
uint16_t ad5950_buff_get(uint32_t *rxData, uint32_t *time);
uint8_t getAd5940DataReady();
void ResetTimeGapAd5940();
int8_t ad5940_read_bcm_data_to_buffer();
void Ad5940DrvDataReadyCallback(void (*pfAD5940DataReady)());
int32_t AppECGDataProcess(int32_t * const pData, uint32_t *pDataCount);
int32_t AppECGRegModify(int32_t * const pData, uint32_t *pDataCount);
uint32_t Ad5940DrvGetDebugInfo();
uint8_t get_fifostat(uint32_t *fifo_cnt);
#endif  // _SENSOR_AD5940_H_
