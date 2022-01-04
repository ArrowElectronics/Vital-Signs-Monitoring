/**
***************************************************************************
* @addtogroup Tasks
* @{
* @file         adi_vsm_bcm.h
* @author       ADI
* @version      V1.0.0
* @date         30-Aug-2021
* @brief        Header file of BCM algorithm parameters
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

#ifndef ADI_VSM_BCM_H
#define ADI_VSM_BCM_H

/******************************************************************************
**          Macro definitions
******************************************************************************/

/**
* Maximum polynomial coefficients
*/
#define ALLOWED_POLY_COEFFICIENTS 8


/******************************************************************************
**      Enumerations
******************************************************************************/

/**
* @brief Enum defined for possible BCM return codes
*/
typedef enum {
    ADI_VSM_BCM_SUCCESS = 0,
    ADI_VSM_BCM_IN_PROGRESS,
    ADI_VSM_BCM_FFM_ESTIMATED_ERROR,
    ADI_VSM_BCM_NULL_PTR_ERROR,
    ADI_VSM_BCM_ERROR,
} adi_vsm_bcm_return_code_t;

/******************************************************************************
**      Structure Declarations
******************************************************************************/

/**
* @brief struct define the state memory block
*/
typedef struct {
    struct {
        void* block;
        uint16_t length_numchars;
    } state;
} adi_vsm_bcm_mem_t;


/**
* @brief  Config Struct Algorithm version Information
*/ 
typedef struct {
  uint32_t  bcm_algo_version_num;  //!< BCM Algorithm version in integer format
  char      *version_text;         //!< BCM Algorithm version in string format
} adi_vsm_bcm_module_info_t;


/**
* @brief  Structure contains fat free mass estimated,
* fat free mass sum, passed as an argument to 
* adi_vsm_bcm_output_t API
*/  
typedef struct {
    float ffm_estimated;  //!< Fat Free Mass ( FFM ) estimated
    float bmi;  //!< body mass Index estimated
    float fatpercent;  //!< fat percent estimated
} adi_vsm_bcm_output_t;


/**
* @brief  Config Structure contains body parameters / coefficients for ffm prediction
   passed as an argument to adi_vsm_bcm_process API
*/ 
typedef struct {
   float height;  //!< Height of person in Cms
   float age;     //!< Age of person in Years
   float weight;  //!< Weight of person in Kgs 
   float b[ALLOWED_POLY_COEFFICIENTS];  //!< Polynomial Coefficients required for FFM estimation 
} adi_vsm_bcm_algo_config_t ;

/**
* @brief  Config Struct contains sampling freq, output data rate 
   bcm coeff config, body parameters config, to be configured during 
   instance creation, passed as an argument to adi_vsm_bcm_create API
*/
typedef struct {
    uint16_t sampling_freq;    //!< Sampling frequency
    uint16_t output_data_rate; //!< Output data rate
    adi_vsm_bcm_algo_config_t adi_vsm_bcm_algo_config;
}adi_vsm_bcm_config_t;


/* The BCM instance structure is opaque - its fields are not accessible
   by the user. */
typedef struct adi_vsm_bcm_instance adi_vsm_bcm_instance_t;

/* The BCM module info structure holds algo version information */
extern const adi_vsm_bcm_module_info_t adi_vsm_bcm_module_info;

/******************************************************************************
**          Function Prototypes
******************************************************************************/

/**
* @brief		Creates an BCM Instance
*
* @param[in]            per_instance_mem_blocks:    Specifies memory pools
*                                                   for creating an instance
* @param[in]            config_params:              pointer to struct containing cfg
*
* @return               Pointer to BCM instance, or NULL if unsuccessful.
*/
adi_vsm_bcm_instance_t* adi_vsm_bcm_create(
                        adi_vsm_bcm_mem_t* const per_instance_mem_blocks,
                        const adi_vsm_bcm_config_t* config_params);


/**
* @brief  		 Reset of algorithm parameters
*
* @param[in]             handle:  Pointer to BCM Instance adi_vsm_bcm_instance_t
* @return 		 adi_vsm_bcm_return_code_t
*/
adi_vsm_bcm_return_code_t adi_vsm_bcm_algo_reset(adi_vsm_bcm_instance_t* handle);

/**
* @brief  		BCM Update Config
*
* @param[in]            handle:         Pointer to BCM Instance adi_vsm_bcm_instance_t
* @param[in]            config_params:  pointer to struct containing Cfg
* 											  
* @return 		adi_vsm_bcm_return_code_t
*/
adi_vsm_bcm_return_code_t adi_vsm_bcm_update_config(adi_vsm_bcm_instance_t* handle,\
						   const adi_vsm_bcm_config_t* config_params);

/**
* @brief		  Process the sample through different algorithm blocks
*        		  and get the FFM
*
* @param[in]              instance_handle: Pointer to BCM instance
* @param[in]              body_impedance:  Pointer to the body impedance values
* @param[out]             algo_output:   Pointer to struct containing algorithm output
*
* @return                 adi_vsm_bcm_return_code_t
*/
adi_vsm_bcm_return_code_t adi_vsm_bcm_process(adi_vsm_bcm_instance_t* instance_handle,
                                 float *body_impedance,
                                 adi_vsm_bcm_output_t* algo_output);

#endif
