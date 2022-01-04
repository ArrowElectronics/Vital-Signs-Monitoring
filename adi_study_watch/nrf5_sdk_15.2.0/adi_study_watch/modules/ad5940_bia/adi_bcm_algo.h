/**
***************************************************************************
* @addtogroup Tasks
* @{
* @file         adi_bcm_algo.h
* @author       ADI
* @version      V1.1.0
* @date         01-Sept-2021
* @brief        Wrapper header file for bcm algorithm integration 
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


#ifndef ADI_BCM_ALGO_H
#define ADI_BCM_ALGO_H

/******************************************************************************
**          includes
******************************************************************************/

#include <stdint.h>
#include <adi_vsm_bcm.h>
//#include <adi_vsm_bcm_internal.h>

/******************************************************************************
**          Macro Definitions
******************************************************************************/

/*
* Algorithm version number is in format "XX.YY.ZZ"
* So MAX 9 characters including '\0' to store the version number in a string
* XX->Major, yy->Minor, ZZ-> Patch
*/
#define ALGORITHM_VERSION_NUM_LENGTH         9

/* macro to check Null Pointer */
#define NULL_POINTER_CHECK(handle)            \
   if(handle == NULL)                         \
   {                                          \
       return BCM_ALG_NULL_PTR_ERROR;         \
   }    

/* macro for return code errors */
#define RETURN_CODE(ret_code)                   \
    switch (ret_code)                           \
    {                                           \
      case ADI_VSM_BCM_SUCCESS:                 \
      {                                         \
        return BCM_ALG_SUCCESS;                 \
      }                                         \
      case ADI_VSM_BCM_IN_PROGRESS:             \
      {                                         \
        return BCM_ALG_IN_PROGRESS;             \
      }                                         \
      case ADI_VSM_BCM_NULL_PTR_ERROR:          \
      {                                         \
        return BCM_ALG_NULL_PTR_ERROR;          \
      }                                         \
      default:                                  \
      {                                         \
        return BCM_ALG_ERROR;                   \
      }                                         \
    } 

/******************************************************************************
**      Enum Declarations
******************************************************************************/

typedef enum {
    BCM_ALG_SUCCESS = 0,//!< BCM Algorithm Succes
    BCM_ALG_IN_PROGRESS,
    BCM_ALG_IN_FFM_ESTIMATED_ERROR,//!< BCM Algorithm Null Pointer error
    BCM_ALG_NULL_PTR_ERROR,//!< BCM Algorithm Null Pointer error
    BCM_ALG_ERROR//!< BCM Algorithm Error
} BCM_ALG_RETURN_CODE_t;

/******************************************************************************
**      Structure declarations
******************************************************************************/

typedef struct {
  float height;//!< Height of person
  float weight;//!< Weight of person
  float age;//!< Age of person
  uint16_t ffmodr;//!< Output FFM ODR
  uint16_t sampling_freq;//!< Input BIA Sampling frequency
  float b[ALLOWED_POLY_COEFFICIENTS];//!< coefficients
}config_t;

/******************************************************************************
**      Function Declarations
******************************************************************************/

/**
* @brief        Get BCM Algorithm version
*
* @param[in]    instance                pointer to BCM instance
* @param[out]   bcm_algo_version_num    pointer to BCM algorithm version integer number
*               bcm_algo_version        pointer to char array to store 
*                                       BCM algorithm version
*
* @return       BCM_ALG_RETURN_CODE_t
*/
BCM_ALG_RETURN_CODE_t GetBcmAlgVersion(adi_vsm_bcm_instance_t* instance,
                                      uint32_t *bcm_algo_version_num,
                                       char* bcm_algo_version);

/**
* @brief        Configure BCM Algorithm Generic parameters
*
* @param[in]    bcmconfig            pointer to structure which holds config paramters
* @param[in]    config_handle        pointer to structure holding the config
* 
* @return       BCM_ALG_RETURN_CODE_t
*/
BCM_ALG_RETURN_CODE_t BcmAlgLoadConfig(config_t *bcmconfig,
                                          adi_vsm_bcm_config_t* config_handle);

/**
* @brief        BCM Algorithm Init, Creates BCM Instance
*
* @param[in]    config_handle        pointer to struct. holding the config
* @param[out]   instance             double pointer to BCM instance to be created
* 
* @return       BCM_ALG_RETURN_CODE_t
*/
BCM_ALG_RETURN_CODE_t BcmAlgInit(const adi_vsm_bcm_config_t* config_handle, 
                                  adi_vsm_bcm_instance_t** instance);

/**
* @brief        Do BCM Instance reset
*
* @param[in]    instance            pointer to BCM instance
*
* @return       BCM_ALG_RETURN_CODE_t
*/
BCM_ALG_RETURN_CODE_t BcmAlgReset(adi_vsm_bcm_instance_t* instance);

/**
* @brief        Do BCM Algorithm process
*
* @param[in]    instance                pointer to BCM instance
* @param[in]    resistive_sample        resistive sample to be processed
* @param[in]    reactive_sample         reactive sample to be processed
* @param[out]   adi_vsm_bcm_output      pointer to struct containing BCM output
*
* @return       BCM_ALG_RETURN_CODE_t
*/
BCM_ALG_RETURN_CODE_t BcmAlgProcess(adi_vsm_bcm_instance_t * instance,
                                    float *bodyImpedance, 
                                    adi_vsm_bcm_output_t * adi_vsm_bcm_output);

#endif
