/**
***************************************************************************
* @addtogroup Tasks
* @{
* @file         adi_vsm_sqi.h
* @author       ADI
* @version      V1.1.0
* @date         03-May-2021
* @brief        Header file of sqi processing APIs.
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

#ifndef ADI_VSM_SQI_H
#define ADI_VSM_SQI_H

#include <stdint.h>

/******************************************************************************
**          Macro Definitions
******************************************************************************/

#define ADI_SQI_VERSION_NUM 0x00010100 /* Current Version = 01.01.00 */
/*
* SQI_Version_No. = 0x00XXYYZZ
* where, XX-> Major, YY-> Minor, ZZ-> Patch
*/

/******************************************************************************
**      Structure Declarations
******************************************************************************/

/* struct define the state memory block */
typedef struct {
    struct {
        void* block;
        uint16_t length_numchars;
    } state;
} adi_vsm_sqi_mem_t;


/* Return code for the public APIs */
typedef enum {
    ADI_VSM_SQI_SUCCESS = 0,
    ADI_VSM_SQI_BUFFERING,
    ADI_VSM_SQI_NULL_PTR_ERROR,
    ADI_VSM_SQI_DIV_BY_ZERO_ERROR,
    ADI_VSM_SQI_ERROR
} adi_vsm_sqi_return_code_t;


/* Config Struct contains sampling freq. 
   to be configured during instance creation,
   passed as an argument to adi_vsm_sqi_create API*/
typedef struct {
    uint8_t sampling_freq;
} adi_vsm_sqi_config_t;


/* Structure contains SQI output,
   passed as an argument to adi_vsm_sqi_process API*/
typedef struct {
    float sqi;
} adi_vsm_sqi_output_t;


/* The SQI instance structure is opaque - its fields are not accessible
   by the user. */
typedef struct adi_vsm_sqi_instance adi_vsm_sqi_instance_t;


/******************************************************************************
**      Function Declarations
******************************************************************************/

/* Get the Version of SQI Algorithm */
adi_vsm_sqi_return_code_t adi_vsm_sqi_get_version(
    adi_vsm_sqi_instance_t*, char*);

/* Creates a single instance of the SQI algorithm*/
adi_vsm_sqi_instance_t* adi_vsm_sqi_create(
    adi_vsm_sqi_mem_t* const,
    const adi_vsm_sqi_config_t*);


/*Resets the filter states, Input buffer index is reset to accept 
  new set of samples to freshly start the SQI algorithm*/
adi_vsm_sqi_return_code_t adi_vsm_sqi_frontend_reset(adi_vsm_sqi_instance_t*);


/* In addition to frontend reset, 
   In this case all the Internal buffers are cleared */
adi_vsm_sqi_return_code_t adi_vsm_sqi_reset(adi_vsm_sqi_instance_t*);


/*Returns size of state memory block required for creating one SQI Instance*/
uint16_t adi_vsm_sqi_numchars_state_memory(void);

/*Process the input sample. SQI output is calculated when sufficient amount of
  samples has been passed through this process API*/
adi_vsm_sqi_return_code_t adi_vsm_sqi_process(adi_vsm_sqi_instance_t*, float,
                                                        adi_vsm_sqi_output_t*);
#endif /* ADI_VSM_SQI_H */
