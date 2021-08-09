/**
***************************************************************************
* @addtogroup Tasks
* @{
* @file         adi_sqi_algo.h
* @author       ADI
* @version      V1.0.0
* @date         17-Sept-2020
* @brief        Wrapper header file for sqi algorithm integration 
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
* Copyright (c) 2020 Analog Devices Inc.
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


#ifndef ADI_SQI_ALGO_H
#define ADI_SQI_ALGO_H

#include <stdint.h>
#include <adi_vsm_sqi.h>

typedef enum {
    SQI_ALG_SUCCESS = 0,
    SQI_ALG_BUFFERING,
    SQI_ALG_NULL_PTR_ERROR,
    SQI_ALG_DIV_BY_ZERO_ERROR,
    SQI_ALG_ERROR
} SQI_ALG_RETURN_CODE_t;

SQI_ALG_RETURN_CODE_t SqiAlgConfig(uint8_t input_sample_freq);
SQI_ALG_RETURN_CODE_t SqiAlgInit(const adi_vsm_sqi_config_t* config_params);
SQI_ALG_RETURN_CODE_t SqiAlgReset();
SQI_ALG_RETURN_CODE_t SqiAlgProcess(float* ppg_data, adi_vsm_sqi_output_t* adi_vsm_sqi_output);

#endif
