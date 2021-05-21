/******************************************************************************
$Id: dbeffde4293cbaba96ff5598ddb0ddba5fba19f5 $

Project:    VSM Filter Module
Title:      Public API header file adi_vsm_filter.h

Description:
            This module provides a filtering module for use in signal
            processing systems. It implements an IIR filter
            as a cascade of biquad sections.

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
#ifndef ADI_VSM_FILTER_H
#define ADI_VSM_FILTER_H

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
#define adi_vsm_filter_create (adi_vsm_filter_create_0_9)


/******************************************************************************
**      Structure Declarations
******************************************************************************/

typedef float adi_vsm_filter_biquad_coeffs_t[5];

/* These parameters can only be configured at instance creation time */
typedef struct {
    /* number of biquad sections for this filter */
    int16_t num_biquad_sections;

    /* section_gains is a pointer to an array of gain values for each section */
    float *section_gains;

    /* section_coeffs is a pointer to a 2D array of filter coefficients,
       where each row is a 5-element-array of coeffs for each biquad section.
       First two elements of each row are the denominators
       and the last three are the numerators. */
    adi_vsm_filter_biquad_coeffs_t *section_coeffs;
} adi_vsm_filter_config_t;

typedef struct {
    struct {
        void *block;
        uint16_t length_numchars;
    } state;
} adi_vsm_filter_mem_t;

/* The Pre-process module instance structure is opaque - its fields are not
   accessible by the user. */
typedef struct adi_vsm_filter_instance adi_vsm_filter_instance_t;


/******************************************************************************
**          Externs
******************************************************************************/

/* Module version information */
//extern const adi_vsm_filter_module_info_t adi_vsm_filter_module_info;

/******************************************************************************
**      Function Declarations
******************************************************************************/


/* Returns the size in chars of a memory block required to store state. The
   required size depends on the configuration parameters. */
uint16_t adi_vsm_filter_numchars_state_memory(
    const adi_vsm_filter_config_t *config_params
);

/* Creates a library instance. per_instance_mem_blocks describes the state
   and scratch memory areas for the instance to use. config_params describes
   the instance parameters. If the memory blocks are not large enough for the
   given parameter values (as exposed by adi_vsm_filter_numchars_state_memory
   and adi_vsm_filter_numchars_scratch_memory), NULL is returned. */
adi_vsm_filter_instance_t *adi_vsm_filter_create(
    const adi_vsm_filter_mem_t *per_instance_mem_blocks,
    const adi_vsm_filter_config_t *config_params
);

/* Resets the instance state to initial conditions. Does not affect the
   instance's parameters. */
void adi_vsm_filter_reset(
    adi_vsm_filter_instance_t *instance_handle
);

/* Performs filtering on a sample from a signal.
   Fixed-point format in this version is 12.20 for all input data, which
   means the signal value 1.0 is represented by the integral value 1048576. */
int32_t adi_vsm_filter(
    adi_vsm_filter_instance_t *instance_handle,
    int32_t input_sample
);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ADI_VSM_FILTER_H */

/*
**
** EOF:
**
*/
