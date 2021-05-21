/*******************************************************************************
$Id: 1f01004a65302952f9faedd3735a1c538bd8f628 $

Project:    VSM Pre-Processing Module
Title:      Public API header file adi_vsm_toolbox.h

Description:
            This module provides a toolbox library for use in vital
            signs monitoring (VSM) systems, such as those
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
            KT-2614 VSM Toolbox Reference Guide

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
#ifndef ADI_VSM_TOOLBOX_H
#define ADI_VSM_TOOLBOX_H

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

/* These renaming macros prevent users from accidentally linking against
    incompatible versions of the API functions
*/
#define adi_vsmtb_ac_metrics_create (adi_vsmtb_ac_metrics_create_1_0)
#define adi_vsmtb_accel_metrics_create (adi_vsmtb_accel_metrics_create_2_0)
#define adi_vsmtb_dc_measurement_set_time_const (adi_vsmtb_dc_measurement_set_time_const_1_0)


#define ADI_VSMTB_AC_METRICS_UNIDENTIFIED_FREQUENCY (-1)


/******************************************************************************
**      Structure Declarations
******************************************************************************/

typedef struct {
    uint32_t version_num;
    char *version_text;
} adi_vsmtb_module_info_t;

typedef enum {
    ADI_VSMTB_AC_METRICS_READY = 1,
    ADI_VSMTB_AC_METRICS_NOT_READY = -254,
    ADI_VSMTB_AC_METRICS_INVALID_POINTER = -255
} adi_vsmtb_ac_metrics_return_code_t;

/* Outputs from the pre-processing module. */
typedef struct {
    int32_t modulation_index;      /* 12.20 fixed-pt format */
    int32_t clarity_index;         /* 12.20 fixed-pt format */
    int32_t randomness_index;      /* 12.20 fixed-pt format */
    int32_t detected_frequency_hz; /* 12.20 fixed-pt format */
} adi_vsmtb_ac_metrics_output_status_t;

/* The following parameters can only be configured at instance
 * creation time */
typedef struct {
    int16_t sample_rate;        /* Hz (Q16.0) */
    uint16_t data_window_length_ms; /* ms (Q16.0) */
    int16_t min_freq;           /* Hz (Q10.6) */
    int16_t max_freq;           /* Hz (Q10.6) */
    /* Note: 'min_freq' cannot be smaller than a lower limit
     * determined by the value of 'data_window_length_ms'. The
     * following inequality must be true:
     * (ceil(sample_rate/min_freq) + 1) <=
     *     ((data_window_length_ms / 1000 * sample_rate) - 1)   */
    int16_t peak_acceptance_factor; /* 0..1 (Q2.14) */
} adi_vsmtb_ac_metrics_config_t;

typedef struct {
    struct {
        void *block;
        uint16_t length_numchars;
    } state;
    struct {
        void *block;
        uint16_t length_numchars;
    } scratch;
} adi_vsmtb_ac_metrics_mem_t;


/*
 * Motion Index
 */

typedef enum {
    ADI_VSMTB_ACCEL_METRICS_READY = 1,
    ADI_VSMTB_ACCEL_METRICS_INVALID_POINTER = -255
} adi_vsmtb_accel_metrics_return_code_t;

typedef struct {
    struct {
        void *block;
        uint16_t length_numchars;
    } state;
} adi_vsmtb_accel_metrics_mem_t;

/* The following parameters can only be configured at instance
 * creation time */
typedef struct {
    int16_t sample_rate;        /* Hz (Q16.0) */
} adi_vsmtb_accel_metrics_config_t;

/* The following parameters can be modified at run time calling
 * 'adi_vsmtb_accel_metrics_set_params()' */
typedef struct {
    uint16_t data_window_length_ms; /* [ms]  Q16.0 */
    int32_t thresh_a;               /* [g^2] Q12.20 */
    int32_t thresh_b;               /* [g^2] Q12.20 */
} adi_vsmtb_accel_metrics_rt_parameters_t;

typedef struct {
    int32_t motion_index;       /* [g^2] Q12.20 fixed-point format */
    int16_t flag_a;             /* 1 if threshold was crossed during
                                 * last 'data_window_length_ms'
                                 * milliseconds, 0 otherwise. */
    int16_t flag_b;             /* same description as 'flag_a' */
} adi_vsmtb_accel_metrics_output_status_t;

/* The instance structures are opaque - their fields are not
   accessible to the user. */
typedef struct adi_vsmtb_ac_metrics_instance adi_vsmtb_ac_metrics_instance_t;
typedef struct adi_vsmtb_accel_metrics_instance adi_vsmtb_accel_metrics_instance_t;


/******************************************************************************
**          Externs
******************************************************************************/

/* Module version information */
extern const adi_vsmtb_module_info_t adi_vsmtb_module_info;


/******************************************************************************
**      Function Declarations
******************************************************************************/

/*
 * DC level measurement
 */

typedef struct {
    /*** PRIVATE members ***/
    int32_t dc_level;
    float dc_level_alpha;     /* Pole of low-pass filter used for
                                 * generating the DC level values. */
} adi_vsmtb_dc_measurement_instance_t;

/* Setup the DC measurement instance state to respond with the given
   time constant in seconds, at given sample rate in Hz. */
void adi_vsmtb_dc_measurement_set_time_const(
    adi_vsmtb_dc_measurement_instance_t *s,
    int32_t sample_rate,
    int32_t time_const_seconds);

/* Resets the DC measurement instance state to an initial DC level.
   This should be done by the application using the first input sample,
   in order to avoid a long initial transient. */
static inline void adi_vsmtb_dc_measurement_reset(
    adi_vsmtb_dc_measurement_instance_t *s,
    int32_t dc_level)
{
    s->dc_level = dc_level;
}

/* Computes the input signal DC level measurement and returns it.
   Input sample is expected to be in fixed point format. */
int32_t adi_vsmtb_dc_measurement(adi_vsmtb_dc_measurement_instance_t *s,
                                 int32_t raw_input_sample);


/*
 * AC Signal Analysis metrics
 */

/* Returns the size in chars of a memory block required to store state. The
   required size depends on the configuration parameters. */
uint16_t adi_vsmtb_ac_metrics_numchars_state_memory(
    const adi_vsmtb_ac_metrics_config_t *config_params
);

/* Returns the size in chars of a memory block required to store scratch data.
   The required size depends on the configuration parameters. */
uint16_t adi_vsmtb_ac_metrics_numchars_scratch_memory(
    const adi_vsmtb_ac_metrics_config_t *config_params
);

/* Creates an AC Signal Analysis instance. per_instance_mem_blocks describes
   the state and scratch memory areas for the instance to use.
   config_params describes the instance parameters. If the memory blocks are
   not large enough for the given parameter values
   (as exposed by adi_vsmtb_ac_metrics_numchars_state_memory
   and adi_vsmtb_ac_metrics_numchars_scratch_memory), NULL is returned. */
adi_vsmtb_ac_metrics_instance_t *adi_vsmtb_ac_metrics_create(
    const adi_vsmtb_ac_metrics_mem_t *per_instance_mem_blocks,
    const adi_vsmtb_ac_metrics_config_t *config_params
);

/* Resets the instance state to initial conditions. Does not affect the
   instance's parameters. */
void adi_vsmtb_ac_metrics_reset(
    adi_vsmtb_ac_metrics_instance_t *instance_handle
);

/* Performs AC signal analysis on a sample from a signal
   and produces an updated output status, including signal quality metrics.
   The signal's DC level must be measured by the application and input
   to this function. */
adi_vsmtb_ac_metrics_return_code_t adi_vsmtb_ac_metrics(
    adi_vsmtb_ac_metrics_instance_t *instance_handle,
    int32_t input_sample,
    int32_t dc_level,
    adi_vsmtb_ac_metrics_output_status_t *module_status
);


/*
 * Motion Index
 */

uint16_t adi_vsmtb_accel_metrics_numchars_state_memory(
    const adi_vsmtb_accel_metrics_config_t *config_param);

void adi_vsmtb_accel_metrics_set_params(
    adi_vsmtb_accel_metrics_instance_t *instance,
    const adi_vsmtb_accel_metrics_rt_parameters_t *rt_params);

void adi_vsmtb_accel_metrics_get_params(
    adi_vsmtb_accel_metrics_rt_parameters_t *rt_params,
    const adi_vsmtb_accel_metrics_instance_t *instance);

void adi_vsmtb_accel_metrics_reset(
    adi_vsmtb_accel_metrics_instance_t *handle);

adi_vsmtb_accel_metrics_instance_t *adi_vsmtb_accel_metrics_create(
    const adi_vsmtb_accel_metrics_mem_t *per_instance_mem_blocks,
    const adi_vsmtb_accel_metrics_config_t *config_params);

/* Runs the Accelerometer Signal Analysis algorithm on a set of accelerometer
 * input samples and produces an indicator of the amount of motion present */
adi_vsmtb_accel_metrics_return_code_t adi_vsmtb_accel_metrics(
    adi_vsmtb_accel_metrics_instance_t *instance_handle,
    int32_t accelx,
    int32_t accely,
    int32_t accelz,
    adi_vsmtb_accel_metrics_output_status_t *module_status);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ADI_VSM_TOOLBOX_H */

/*
**
** EOF:
**
*/
