// #############################################################################
// THIS IS AN AUTOMATICALLY GENERATED FILE, DO NOT MODIFY IT!
// #############################################################################
#pragma once

#include "m2m2_core.hpp"
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


enum M2M2_LED_COMMAND_ENUM_t:uint8_t {
  M2M2_LED_COMMAND_GET = 0,
  M2M2_LED_COMMAND_SET = 1,
};
static_assert(sizeof(M2M2_LED_COMMAND_ENUM_t) == 1, "Enum 'M2M2_LED_COMMAND_ENUM_t' has an incorrect size!");

enum M2M2_LED_PATTERN_ENUM_t:uint8_t {
  M2M2_LED_PATTERN_OFF = 0,
  M2M2_LED_PATTERN_SLOW_BLINK_DC_12 = 1,
  M2M2_LED_PATTERN_FAST_BLINK_DC_50_N = 85,
  M2M2_LED_PATTERN_FAST_BLINK_DC_50 = 170,
  M2M2_LED_PATTERN_MED_BLINK_DC_50 = 204,
  M2M2_LED_PATTERN_SLOW_BLINK_DC_50 = 240,
  M2M2_LED_PATTERN_SLOW_BLINK_DC_12_N = 254,
  M2M2_LED_PATTERN_ON = 255,
};
static_assert(sizeof(M2M2_LED_PATTERN_ENUM_t) == 1, "Enum 'M2M2_LED_PATTERN_ENUM_t' has an incorrect size!");

enum M2M2_LED_PRIORITY_ENUM_t:uint8_t {
  M2M2_LED_PRIORITY_LOW = 0,
  M2M2_LED_PRIORITY_MED = 1,
  M2M2_LED_PRIORITY_HIGH = 2,
  M2M2_LED_PRIORITY_CRITICAL = 3,
};
static_assert(sizeof(M2M2_LED_PRIORITY_ENUM_t) == 1, "Enum 'M2M2_LED_PRIORITY_ENUM_t' has an incorrect size!");

struct m2m2_led_ctrl_t {
  M2M2_LED_COMMAND_ENUM_t  command; 
  M2M2_LED_PRIORITY_ENUM_t  priority; 
  M2M2_LED_PATTERN_ENUM_t  r_pattern; 
  M2M2_LED_PATTERN_ENUM_t  g_pattern; 
  M2M2_LED_PATTERN_ENUM_t  b_pattern; 
};

// Reset struct packing outside of this file
#pragma pack()
