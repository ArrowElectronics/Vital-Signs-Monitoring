/******************************************************************************
$Id: 9e120580fcc6fc0b071952fa31dfb45d280b4e19 $

Project:    VSM Pre-Processing Module
Title:      Public API header file adi_vsm_preprocess.h

Description:
            This module provides a pre-processing module for use in systems
            based on photoplethysmography (PPG) optical sensor and
            with accelerometer sensors. It is intended to
            be used with ADI components that may consist of an integrated
            circuit module containing LEDs / photosensor / accelerometer /
            AFE, or discrete ADI components for all or some of these
            functions.

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
#ifndef ADI_VSM_HRM_PREPROCESS_H
#define ADI_VSM_HRM_PREPROCESS_H

#include <stddef.h>
#if __STDC_VERSION__ >= 199900L || __cplusplus >= 201100L
#include <stdint.h>
#else
/* need typedefs for uint32_t, int32_t, int16_t */
#error Your compiler does not appear to support C99 or C++11. \
    Please include a header file defining specified-width integer types
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/******************************************************************************
**          #defines
******************************************************************************/

#define adi_vsm_preprocess_create (adi_vsm_preprocess_create_0_9)
#define ADI_VSM_UNIDENTIFIED_FREQUENCY (-1)


/******************************************************************************
**      Structure Declarations
******************************************************************************/

/* typedef struct {
    uint32_t version_num;
    char *version_text;
} adi_vsm_preprocess_module_info_t;
 */


typedef enum {
    ADI_VSM_PREPROCESS_READY = 1,
    ADI_VSM_PREPROCESS_NOT_READY = -254,
    ADI_VSM_PREPROCESS_INVALID_POINTER = -255
} adi_vsm_preprocess_return_code_t;


/* Outputs from the pre-processing module. */
typedef struct {
    int32_t modulation_index;      /* 12.20 fixed-pt format */
    int32_t clarity_index;         /* 12.20 fixed-pt format */
    int32_t randomness_index;      /* 12.20 fixed-pt format */
    int32_t detected_frequency_hz; /* 12.20 fixed-pt format */
} adi_vsm_preprocess_output_status_t;


/* These parameters can only be configured at instance creation time */
typedef struct {
    int16_t sample_rate;        /* Hz (Q16.0) */
    uint16_t data_window_length_ms;
    int16_t min_freq;           /* Hz (Q10.6) */
    int16_t max_freq;           /* Hz (Q10.6) */
    /* Note: 'min_freq' cannot be smaller than a lower limit
     * determined by the value of 'data_window_length_ms'. The
     * following inequality must be true:
     * (ceil(sample_rate/min_freq) + 1) <=
     *     ((data_window_length_ms / 1000 * sample_rate) - 1)   */
    int16_t peak_acceptance_factor; /* 0..1 (Q2.14) */
} adi_vsm_preprocess_config_t;


typedef struct {
    struct {
        void *block;
        uint16_t length_numchars;
    } state;
    struct {
        void *block;
        uint16_t length_numchars;
    } scratch;
} adi_vsm_preprocess_mem_t;


/* The Pre-process module instance structure is opaque - its fields are not
   accessible by the user. */
typedef struct adi_vsm_preprocess_instance adi_vsm_preprocess_instance_t;


/******************************************************************************
**          Externs
******************************************************************************/

/* Module version information */
//extern const adi_vsm_preprocess_module_info_t adi_vsm_preprocess_module_info;


/******************************************************************************
**      Function Declarations
******************************************************************************/

/*
 * DC level measurement
 */

typedef struct {
    /*** PUBLIC members ***/
    int32_t dc_level;           /* Q12.20 */

    /*** PRIVATE members ***/
    float dc_level_alpha;     /* Pole of low-pass filter used for
                                 * generating the DC level values. */
} adi_vsm_dc_level_instance_t;

/* Setup the DC measurement instance state to respond with the given
   time constant in seconds. */
void adi_vsm_set_dc_measurement_time_const(adi_vsm_dc_level_instance_t *s,
                                          float sample_rate,
                                          float time_const_seconds);

/* Resets the DC measurement instance state to an initial DC level (in Q12.20).
   This should be done by the application using the first input sample,
   in order to avoid a long initial transient. */
static inline void adi_vsm_reset_dc_level(adi_vsm_dc_level_instance_t *s,
                           int32_t dc_level)
{
    s->dc_level = dc_level;
}

/* Computes the DC level measurementand returns it (in Q12.20).
   Input sample is expected to be in Q12.20 fixed point format. */
int32_t adi_vsm_dc_measurement(adi_vsm_dc_level_instance_t *s,
                           int32_t raw_input_sample);


/*
 * Signal Analysis metrics
 */

/* Returns the size in chars of a memory block required to store state. The
   required size depends on the configuration parameters. */
uint16_t adi_vsm_preprocess_numchars_state_memory(
    const adi_vsm_preprocess_config_t *config_params
);

/* Returns the size in chars of a memory block required to store scratch data.
   The required size depends on the configuration parameters. */
uint16_t adi_vsm_preprocess_numchars_scratch_memory(
    const adi_vsm_preprocess_config_t *config_params
);

/* Creates a library instance. per_instance_mem_blocks describes the state
   and scratch memory areas for the instance to use. config_params describes
   the instance parameters. If the memory blocks are not large enough for the
   given parameter values (as exposed by adi_vsm_preprocess_numchars_state_memory
   and adi_vsm_preprocess_numchars_scratch_memory), NULL is returned. */
adi_vsm_preprocess_instance_t *adi_vsm_preprocess_create(
    const adi_vsm_preprocess_mem_t *per_instance_mem_blocks,
    const adi_vsm_preprocess_config_t *config_params
);

/* Resets the instance state to initial conditions. Does not affect the
   instance's parameters. */
void adi_vsm_preprocess_reset(
    adi_vsm_preprocess_instance_t *instance_handle
);

/* Performs signal pre-processing on a sample from a signal
   and produces an updated output status, including signal quality metrics.
   The signal's DC level must be measured by the application and input
   to this function.
   Fixed-point format in this version is 12.20 for all input data, which
   means the signal value 1.0 is represented by the integral value 1048576. */
adi_vsm_preprocess_return_code_t adi_vsm_preprocess_metrics(
    adi_vsm_preprocess_instance_t *instance_handle,
    int32_t input_sample,
    int32_t dc_level,
    adi_vsm_preprocess_output_status_t *module_status
);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ADI_VSM_HRM_PREPROCESS_H */

/*
**
** EOF:
**
*/
