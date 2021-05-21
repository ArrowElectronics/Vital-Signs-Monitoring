/******************************************************************************
$Id: eb91877fc5459d6c32553149e06d04c7168ceff6 $

Project:    VSM Wrist Device Heart Rate Measurement
Title:      Public API header file adi_vsm_hrm.h

Description:
            This module provides an algorithm for a heart rate measurement
            system using wrist-based photoplethysmography (PPG) measurement
            with accelerometer-based interference removal. It is intended to
            be used with ADI components that may consist of an integrated
            circuit module containing LEDs / photosensor / accelerometer /
            AFE, or discrete ADI components for all or some of these
            functions. The algorithm is designed to be highly robust to many
            types of motion affecting the wrist device containing this
            module, providing a reasonably accurate estimate of the wearer's
            heart rate in real-time.

Assumptions:
            Compiler supports specified width integer types
            (defined in stdint.h as part of C99 and C++11)

References:
            KT-2521 VSM Wrist Device Heart Rate Measurement Reference Guide

*****************************************************************************/
/*!
 *  \copyright Analog Devices
 * ****************************************************************************
 *
 * License Agreement
 *
 * Copyright (c) 2019 Analog Devices Inc.
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
#ifndef ADI_VSM_HRM_H
#define ADI_VSM_HRM_H

#include <stddef.h>
#if __STDC_VERSION__ >= 199900L || __cplusplus >= 201100L
#include <stdint.h>
#else
/* need typedefs for uint32_t, int32_t, int16_t */
#error Your compiler does not appear to support C99 or C++11 - please modify this line to include a header file defining specified width integer types
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/******************************************************************************
**          #defines
******************************************************************************/
#define adi_vsm_hrm_create           (adi_vsm_hrm_create_1_2)
#define adi_vsm_hrm_load               (adi_vsm_hrm_load_1_2)
#define adi_vsm_spothr_create     (adi_vsm_spothr_create_1_2)
#define adi_vsm_spothr_load         (adi_vsm_spothr_load_1_2)
#define adi_vsm_hrm_plus_create (adi_vsm_hrm_plus_create_1_2)
#define adi_vsm_hrm_plus_load     (adi_vsm_hrm_plus_load_1_2)

/* See adi_vsm_hrm_load. */
#define ADI_VSM_HRM_PARAM_BLOCK_NUMCHARS 420

/* See adi_vsm_spothr_load. */
#define ADI_VSM_SPOTHR_PARAM_BLOCK_NUMCHARS 212

/* See adi_vsm_hrm_plus_load. */
#define ADI_VSM_HRM_PLUS_PARAM_BLOCK_NUMCHARS 648

/* Default configuration values for creating an instance */
#define ADI_VSM_HRM_DEFAULT_MIN_HEART_RATE 30

/******************************************************************************
**      Structure Declarations
******************************************************************************/

typedef struct {
    uint32_t version_num;
    char *version_text;
} adi_vsm_hrm_module_info_t;

typedef enum {
    ADI_VSM_HRM_VALID = 1,
    ADI_VSM_HRM_INVALID_POINTER = -255
} adi_vsm_hrm_return_code_t;

/* Outputs from the tracking HR algorithm. */
typedef struct {
    int16_t heart_rate_bpm;     /* 12.4  fixed pt format */
    int16_t confidence;         /* 6.10  fixed pt format */
} adi_vsm_hrm_output_status_t;


/* These parameters can only be configured at instance creation time */
typedef struct {
    int16_t sample_rate;
    int16_t min_heart_rate_bpm;
} adi_vsm_hrm_config_t;

typedef struct {
    struct {
        void *block;
        uint16_t length_numchars;
    } state;
} adi_vsm_hrm_mem_t;

/* The HRM instance structure is opaque - its fields are not accessible
   by the user. */
typedef struct adi_vsm_hrm_instance adi_vsm_hrm_instance_t;

/* These parameters can be updated at run-time */
typedef struct adi_vsm_hrm_parameters {
    int16_t max_heart_rate_bpm;
} adi_vsm_hrm_parameters_t;

/* SpotHR structures */

/* Output from the spot HR algorithm */
typedef struct {
    int16_t heart_rate_bpm;
    uint8_t poor_signal_quality;
    uint8_t high_motion;
} adi_vsm_spothr_output_status_t;

typedef enum {
    ADI_VSM_SPOTHR_READY = 1,
    ADI_VSM_SPOTHR_NOT_READY = -254,
    ADI_VSM_SPOTHR_INVALID_POINTER = -255
} adi_vsm_spothr_return_code_t;

typedef struct {
    int16_t sample_rate;
    int16_t min_heart_rate_bpm;
    int16_t max_heart_rate_bpm;
    uint16_t data_window_length_ms;
} adi_vsm_spothr_config_t;

typedef struct {
    struct {
        void *block;
        uint16_t length_numchars;
    } state;
    struct {
        void *block;
        uint16_t length_numchars;
    } scratch;
} adi_vsm_spothr_mem_t;

/* These parameters can be updated at run-time */
typedef struct adi_vsm_spothr_parameters {
    int32_t high_activity_threshold;
} adi_vsm_spothr_parameters_t;

typedef struct adi_vsm_spothr_instance adi_vsm_spothr_instance_t;

/******************************************************************************
**          Externs
******************************************************************************/

/* Module version information */
extern const adi_vsm_hrm_module_info_t adi_vsm_hrm_module_info;

/******************************************************************************
**      Function Declarations
******************************************************************************/

/* Returns the size in chars of a memory block required to store state. The
   required size depends on the configuration parameters. */
uint16_t adi_vsm_hrm_numchars_state_memory(
    const adi_vsm_hrm_config_t *config_params
);

/* Creates a Tracking HR instance. per_instance_mem_blocks describes the state
   memory areas for the instance to use. config_params describes the
   configuration parameters. If the memory blocks are not large enough for the
   given parameter values (as exposed by adi_vsm_hrm_numchars_state_memory),
   NULL is returned. */
adi_vsm_hrm_instance_t *adi_vsm_hrm_create(
    const adi_vsm_hrm_mem_t *per_instance_mem_blocks,
    const adi_vsm_hrm_config_t *config_params
);

/* Returns a copy of the user-configurable tuning parameters that the
   instance is using. */
void adi_vsm_hrm_get_params(
    adi_vsm_hrm_parameters_t *user_copy_of_params,
    const adi_vsm_hrm_instance_t *instance_handle
);

/* Updates the instance with a new set of user-configurable tuning parameters. */
void adi_vsm_hrm_set_params(
    adi_vsm_hrm_instance_t *instance_handle,
    const adi_vsm_hrm_parameters_t *user_supplied_params
);


/* Creates a Tracking HR instance. per_instance_mem_blocks describes the state
   and scratch memory areas for the instance to use.  Initializes the
   instance with algorithm parameters supplied in load_parameter_block.
   If the memory blocks described by per_instance_mem_blocks are not
   large enough, NULL is returned. If the version information stored in the
   persisted data does not match the major and minor (but not release) fields
   of the library's version, NULL is returned. */
adi_vsm_hrm_instance_t *adi_vsm_hrm_load(
    adi_vsm_hrm_mem_t *per_instance_mem_blocks,
    char *load_parameter_block
);

/* Saves the complete set of algorithm parameters necessary to recreate
   this instance in a production application through the use of
   the _load function rather than the _create function.
   Parameter data is saved to the supplied memory buffer which must be
   of length ADI_VSM_HRM_PARAM_BLOCK_NUMCHARS bytes.
   The user must separately record the required size of the state memory block
   used to create this instance. */
void adi_vsm_hrm_save(
  adi_vsm_hrm_instance_t *instance_handle,
  char *save_parameter_block
);

/* Informs the instance that the system has applied changes to the input gain.*/
void adi_vsm_hrm_frontend_reset(
    adi_vsm_hrm_instance_t *instance_handle
);

/* Resets the instance state to initial conditions. Does not affect the
   instance's parameters. */
void adi_vsm_hrm_reset(
    adi_vsm_hrm_instance_t *instance_handle
);

/* Processes a sample of PPG and accelerometer data and produces an updated
   output status, including heart rate and confidence. Input provides the
   PPG sample, and should be scaled from 0.0 to 400.0. accelx, accely, and accelz
   provide the accelerometer channel samples, and should be scaled in units of
   standard gravity ("g").
   Fixed-point format in this version is 12.20 for all input data, which
   means the signal value 1.0 is represented by the integral value 1048576. */
adi_vsm_hrm_return_code_t adi_vsm_hrm_process(
    adi_vsm_hrm_instance_t *instance_handle,
    int32_t input,
    int32_t accelx,
    int32_t accely,
    int32_t accelz,
    adi_vsm_hrm_output_status_t *module_status
);

/*  Makes adjustments to the instance to continue tracking from the
    provided heart rate value, which is assumed to be accurately measured
    through some other means (e.g. the spot HR algorithm).

    The new heart rate input is in 12.4 fixed point format.

    Only call this function if the system is very confident the new
    heart rate is correct. */
void adi_vsm_hrm_adjust_hr(
    adi_vsm_hrm_instance_t *instance_handle,
    const int new_heart_rate_bpm
);

/* ------------------ SpotHR functions -------------------- */

/* Returns the size in chars of a memory block required to store state. The
   required size depends on the configuration parameters. */
uint16_t adi_vsm_spothr_numchars_state_memory(
    const adi_vsm_spothr_config_t *config_params
);

/* Returns the size in chars of a memory block required to store scratch
   memory. The required size depends on the configuration parameters. */
uint16_t adi_vsm_spothr_numchars_scratch_memory(
    const adi_vsm_spothr_config_t *config_params
);

/* Creates a Spot HR instance. per_instance_mem_blocks describes the state
   memory areas for the instance to use. config_params describes the
   configuration parameters. If the memory blocks are not large enough for the
   given parameter values (as exposed by adi_vsm_spothr_numchars_state_memory),
   NULL is returned. */
adi_vsm_spothr_instance_t *adi_vsm_spothr_create(
    const adi_vsm_spothr_mem_t *per_instance_mem_blocks,
    const adi_vsm_spothr_config_t *config_params
);

/* Returns a copy of the user-configurable tuning parameters that the
   instance is using. */
void adi_vsm_spothr_get_params(
    adi_vsm_spothr_parameters_t *user_copy_of_params,
    const adi_vsm_spothr_instance_t *instance_handle
);

/* Updates the instance with a new set of user-configurable tuning parameters. */
void adi_vsm_spothr_set_params(
    adi_vsm_spothr_instance_t *instance_handle,
    const adi_vsm_spothr_parameters_t *user_supplied_params
);

/* Creates a Spot HR instance. per_instance_mem_blocks describes the state
   and scratch memory areas for the instance to use.  Initializes the
   instance with algorithm parameters supplied in load_parameter_block.
   If the memory blocks described by per_instance_mem_blocks are not
   large enough, NULL is returned. If the version information stored in the
   persisted data does not match the major and minor (but not release) fields
   of the library's version, NULL is returned. */
adi_vsm_spothr_instance_t *adi_vsm_spothr_load(
    adi_vsm_spothr_mem_t *per_instance_mem_blocks,
    char *load_parameter_block
);

/* Saves the complete set of algorithm parameters necessary to recreate
   this instance in a production application through the use of
   the _load function rather than the _create function.
   Parameter data is saved to the supplied memory buffer which must be
   of length ADI_VSM_SPOTHR_PARAM_BLOCK_NUMCHARS bytes.
   The user must separately record the required size of the state memory block
   used to create this instance. */
void adi_vsm_spothr_save(
  adi_vsm_spothr_instance_t *instance_handle,
  char *save_parameter_block
);

/* Resets the instance state to initial conditions. Does not affect the
   instance's parameters. */
void adi_vsm_spothr_reset(
    adi_vsm_spothr_instance_t *instance_handle
);


/* Processes a sample of PPG and accelerometer data.  Input provides the
   PPG sample, and should be scaled from 0.0 to 400.0. accelx, accely, and accelz
   provide the accelerometer channel samples, and should be scaled in units of
   standard gravity ("g").
   Fixed-point format in this version is 12.20 for all input data, which
   means the signal value 1.0 is represented by the integral value 1048576. */
adi_vsm_spothr_return_code_t adi_vsm_spothr_submit(
    adi_vsm_spothr_instance_t *instance_handle,
    int32_t input,
    int32_t accelx,
    int32_t accely,
    int32_t accelz,
    adi_vsm_spothr_output_status_t *module_status
);

/* Compute the heart rate from the data already submitted to this instance,
   and should only be called when the function adi_vsm_spothr_submit returns
   ADI_VSM_SPOTHR_READY. May fail to return a heart rate if the submitted
   samples fail to pass a signal quality test.
   Produces an updated output status, including heart rate and confidence.*/
adi_vsm_spothr_return_code_t adi_vsm_spothr_get_hr(
    adi_vsm_spothr_instance_t *instance_handle,
    adi_vsm_spothr_output_status_t *module_status
);

/* -------- VSM HRM PLUS -------- */

typedef struct {
    struct {
        void *block;
        uint16_t length_numchars;
    } state;
    struct {
        void *block;
        uint16_t length_numchars;
    } scratch;
} adi_vsm_hrm_plus_mem_t;

typedef struct adi_vsm_hrm_plus_instance adi_vsm_hrm_plus_instance_t;

typedef enum {
    ADI_VSM_HRM_PLUS_VALID = 1,
    ADI_VSM_HRM_PLUS_INVALID_POINTER = -255
} adi_vsm_hrm_plus_return_code_t;


typedef struct {
    int16_t heart_rate_bpm;     /* 12.4  fixed pt format */
    int16_t confidence;         /* 6.10  fixed pt format */
    uint8_t high_motion;
} adi_vsm_hrm_plus_output_status_t;

/* Returns the size in chars of a memory block required to store state. */
uint16_t adi_vsm_hrm_plus_numchars_state_memory(void);

/* Returns the size in chars of a memory block required to store scratch. */
uint16_t adi_vsm_hrm_plus_numchars_scratch_memory(void);

/* Creates a HR Plus instance. per_instance_mem_blocks describes the state
   and scratch memory areas for the instance to use. If the memory blocks
   are not large enough (as exposed by adi_vsm_hrm_plus_numchars_state_memory),
   NULL is returned. */
adi_vsm_hrm_plus_instance_t *adi_vsm_hrm_plus_create(
    const adi_vsm_hrm_plus_mem_t *per_instance_mem_blocks
);

/* Informs the instance that the system has applied changes to the input gain.*/
void adi_vsm_hrm_plus_frontend_reset(
    adi_vsm_hrm_plus_instance_t *instance_handle
);

/* Resets the instance state to initial conditions. Does not affect the
   instance's parameters. */
void adi_vsm_hrm_plus_reset(
    adi_vsm_hrm_plus_instance_t *instance_handle
);

/* Processes a sample of PPG and accelerometer data and produces an updated
   output status, including heart rate and confidence. Input provides the
   PPG sample, and should be scaled from 0.0 to 400.0. accelx, accely, and accelz
   provide the accelerometer channel samples, and should be scaled in units of
   standard gravity ("g").
   Fixed-point format in this version is 12.20 for all input data, which
   means the signal value 1.0 is represented by the integral value 1048576. */
adi_vsm_hrm_plus_return_code_t adi_vsm_hrm_plus_process(
    adi_vsm_hrm_plus_instance_t *instance_handle,
    int32_t input,
    int32_t accelx,
    int32_t accely,
    int32_t accelz,
    adi_vsm_hrm_plus_output_status_t *module_status
);

/* Creates a HR Plus instance. per_instance_mem_blocks describes the state
   and scratch memory areas for the instance to use.  Initializes the
   instance with algorithm parameters supplied in load_parameter_block.
   If the memory blocks described by per_instance_mem_blocks are not
   large enough, NULL is returned. If the version information stored in the
   persisted data does not match the major and minor (but not release) fields
   of the library's version, NULL is returned. */
adi_vsm_hrm_plus_instance_t *adi_vsm_hrm_plus_load(
    const adi_vsm_hrm_plus_mem_t *per_instance_mem_blocks,
    char *load_parameter_block
);

/* Saves the complete set of algorithm parameters necessary to recreate
   this instance in a production application through the use of
   the _load function rather than the _create function.
   Parameter data is saved to the supplied memory buffer which must be
   of length ADI_VSM_HRM_PLUS_PARAM_BLOCK_NUMCHARS bytes.
   The user must separately record the required size of the state memory block
   used to create this instance. */
void adi_vsm_hrm_plus_save(
  adi_vsm_hrm_plus_instance_t *instance_handle,
  char *save_parameter_block
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ADI_VSM_HRM_H */

/*
**
** EOF:
**
*/
