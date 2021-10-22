// #############################################################################
// THIS IS AN AUTOMATICALLY GENERATED FILE, DO NOT MODIFY IT!
// #############################################################################
#pragma once

#include "common_application_interface.h"
#include "m2m2_core.h"
#include <stdint.h>


/* Explicitly enforce struct packing so that the nested structs and unions are laid out
    as expected. */
#if defined __CC_ARM || defined __IAR_SYSTEMS_ICC__ || __clang__ || defined _MSC_VER || defined __GNUC__
// DELIBERATELY BLANK
#else
#error "WARNING! Your compiler might not support '#pragma pack(1)'! \
  You must add an equivalent compiler directive to the file generator!"
#endif  // defined __CC_ARM || defined __IAR_SYSTEMS_ICC__ || __clang__ || defined _MSC_VER || defined __GNUC__
#pragma pack(1)

#ifndef STATIC_ASSERT_PROJ
#define STATIC_ASSERT_PROJ(COND, MSG) typedef char static_assertion_##MSG[(COND)?1:-1]
#endif // STATIC_ASSERT_PROJ


typedef struct _m2m2_debug_data_t {
  uint8_t  str[127]; 
} m2m2_debug_data_t;

typedef struct _m2m2_app_debug_stream_t {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  sequence_num; 
  uint32_t  timestamp; 
  uint64_t  debuginfo_64[4]; 
  uint32_t  debuginfo[4]; 
} m2m2_app_debug_stream_t;

// Reset struct packing outside of this file
#pragma pack()
