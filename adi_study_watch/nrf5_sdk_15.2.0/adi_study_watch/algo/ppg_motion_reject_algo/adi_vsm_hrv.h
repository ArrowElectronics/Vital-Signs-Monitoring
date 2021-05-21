/*******************************************************************************
$Id: 1a51311981ead3ece05eca5c359f3e73bf3ad007 $

Project:    VSM Heart Rate Variability
Title:      Public API header file adi_vsm_hrv.h

Description:
            This module provides functions for a heart rate variability
            system using wrist-based photoplethysmography (PPG) measurement.
            It is intended to be used with ADI components that may consist of
            an integrated circuit module containing LEDs / photosensor /
            accelerometer / AFE, or discrete ADI components for all or some
            of these functions.

Assumptions:
            Compiler supports specified width integer types
            (defined in stdint.h as part of C99 and C++11)

References:
            KT-2634 VSM Heart Rate Variablity Reference Guide

*****************************************************************************//*! *  \copyright Analog Devices * **************************************************************************** * * License Agreement * * Copyright (c) 2019 Analog Devices Inc. * All rights reserved. * * This source code is intended for the recipient only under the guidelines of * the non-disclosure agreement with Analog Devices Inc. * * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, * EXPRESS OR  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY * CLAIM, DAMAGES OR OTHER  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, * TORT OR OTHERWISE, ARISING  FROM, OUT OF OR IN CONNECTION WITH THE * SOFTWARE OR THE USE OR OTHER  DEALINGS IN THE SOFTWARE. * **************************************************************************** */#ifndef ADI_VSM_HRV_H
#define ADI_VSM_HRV_H

#include <stddef.h>
#if __STDC_VERSION__ >= 199900L || __cplusplus >= 201100L
#include <stdint.h>
#else
/* need typedefs for uint32_t, int32_t, int16_t, etc */
#error Your compiler does not appear to support C99 or C++11 - please modify this line to include a header file defining specified width integer types
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/******************************************************************************
**          #defines
******************************************************************************/
#define adi_hrv_ppg_b2b_create         (adi_hrv_ppg_b2b_create_0_1)
#define adi_hrv_ppg_b2b_load             (adi_hrv_ppg_b2b_load_0_1)

/* See adi_hrv_ppg_b2b_load.
#define ADI_HRV_PPG_B2B_PARAM_BLOCK_NUMCHARS 360*/

/******************************************************************************
**      Structure Declarations
******************************************************************************/

typedef struct {
    uint32_t version_num;
    char *version_text;
} adi_hrv_module_info_t;

typedef enum {
    ADI_HRV_PPG_B2B_VALID_RR = 1,
    ADI_HRV_PPG_B2B_GAP = 8,
    ADI_HRV_PPG_B2B_NEED_MORE_INPUT = 9,
    ADI_HRV_PPG_B2B_INVALID_POINTER = -255
} adi_hrv_ppg_b2b_return_code_t;

/* Outputs from the HRV B2B algorithm.
    interval_ms is either an identified RR interval
        (when ADI_HRV_PPG_B2B_VALID_RR returned by adi_hrv_ppg_b2b_process),
    or the length of a gap between identified RR intervals
        (when ADI_HRV_PPG_B2B_GAP returned by adi_hrv_ppg_b2b_process)
*/
typedef struct {
    int16_t interval_ms;
} adi_hrv_ppg_b2b_output_status_t;


/* These parameters can only be configured at instance creation time */
typedef struct {
    int16_t sample_rate; /* in Hz */
    int16_t max_reported_gap_ms;
} adi_hrv_ppg_b2b_config_t;

typedef struct {
    struct {
        void *block;
        uint16_t length_numchars;
    } state;
} adi_hrv_ppg_b2b_mem_t;

/* The HRV B2B instance structure is opaque - its fields are not accessible
   by the user. */
typedef struct adi_hrv_ppg_b2b_instance adi_hrv_ppg_b2b_instance_t;

// Placeholder - to be removed if not needed
/* These parameters can be updated at run-time
typedef struct adi_hrv_ppg_b2b_parameters {
    int16_t max_heart_rate_bpm;
} adi_hrv_ppg_b2b_parameters_t;
*/


/******************************************************************************
**          Externs
******************************************************************************/

/* Module version information */
extern const adi_hrv_module_info_t adi_hrv_module_info;

/******************************************************************************
**      Function Declarations
******************************************************************************/

/* Returns the size in chars of a memory block required to store state. The
   required size depends on the configuration parameters. */
uint16_t adi_hrv_ppg_b2b_numchars_state_memory(
    const adi_hrv_ppg_b2b_config_t *config_params
);

/* Creates a HRV B2B instance. per_instance_mem_blocks describes the state
   memory areas for the instance to use. config_params describes the
   configuration parameters. If the memory blocks are not large enough for the
   given parameter values (as exposed by adi_hrv_ppg_b2b_numchars_state_memory),
   NULL is returned. */
adi_hrv_ppg_b2b_instance_t *adi_hrv_ppg_b2b_create(
    const adi_hrv_ppg_b2b_mem_t *per_instance_mem_blocks,
    const adi_hrv_ppg_b2b_config_t *config_params
);

// Placeholder - to be removed if not needed
#if 0
/* Returns a copy of the user-configurable tuning parameters that the
   instance is using. */
void adi_hrv_ppg_b2b_get_params(
    adi_hrv_ppg_b2b_parameters_t *user_copy_of_params,
    const adi_hrv_ppg_b2b_instance_t *instance_handle
);

/* Updates the instance with a new set of user-configurable tuning parameters. */
void adi_hrv_ppg_b2b_set_params(
    adi_hrv_ppg_b2b_instance_t *instance_handle,
    const adi_hrv_ppg_b2b_parameters_t *user_supplied_params
);

/* Creates a HRV B2B instance. per_instance_mem_blocks describes the state
   and scratch memory areas for the instance to use.  Initializes the
   instance with algorithm parameters supplied in load_parameter_block.
   If the memory blocks described by per_instance_mem_blocks are not
   large enough, NULL is returned. If the version information stored in the
   persisted data does not match the major and minor (but not release) fields
   of the library's version, NULL is returned. */
adi_hrv_ppg_b2b_instance_t *adi_hrv_ppg_b2b_load(
    adi_hrv_ppg_b2b_mem_t *per_instance_mem_blocks,
    char *load_parameter_block
);

/* Saves the complete set of algorithm parameters necessary to recreate
   this instance in a production application through the use of
   the _load function rather than the _create function.
   Parameter data is saved to the supplied memory buffer which must be
   of length ADI_HRV_PPG_B2B_PARAM_BLOCK_NUMCHARS bytes.
   The user must separately record the required size of the state memory block
   used to create this instance. */
void adi_hrv_ppg_b2b_save(
  adi_hrv_ppg_b2b_instance_t *instance_handle,
  char *save_parameter_block
);
#endif  /* if 0 */

/* Informs the instance that the system has applied changes to the input gain.*/
void adi_hrv_ppg_b2b_frontend_reset(
    adi_hrv_ppg_b2b_instance_t *instance_handle
);

/* Resets the instance state to initial conditions. Does not affect the
   instance's parameters. */
void adi_hrv_ppg_b2b_reset(
    adi_hrv_ppg_b2b_instance_t *instance_handle
);

/* Processes a sample of PPG data, produces an updated output status.
   Input provides the PPG sample, and should be scaled from 0.0 to 400.0.
   Fixed-point format in this version is 12.20 for all input data, which
   means the signal value 1.0 is represented by the integral value 1048576.
   If ADI_HRV_PPG_B2B_VALID is returned, outputs a valid RR interval;
   if ADI_HRV_PPG_B2B_GAP is returned, outputs the gap time between
        valid RR intervals;
   otherwise, output interval_ms is undefined.
*/
adi_hrv_ppg_b2b_return_code_t adi_hrv_ppg_b2b_process(
    adi_hrv_ppg_b2b_instance_t *instance_handle,
    int32_t ppg_input_sample,
    adi_hrv_ppg_b2b_output_status_t *module_status
);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ADI_VSM_HRV_H */

/*
**
** EOF:
**
*/
