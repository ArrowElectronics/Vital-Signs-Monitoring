// #############################################################################
// THIS IS AN AUTOMATICALLY GENERATED FILE, DO NOT MODIFY IT!
// #############################################################################
#pragma once

#include "common_application_interface.h"
#include "common_sensor_interface.h"
#include <stdint.h>


/* Explicitly enforce struct packing so that the nested structs and unions are laid out
    as expected. */
#if defined __CC_ARM || defined __IAR_SYSTEMS_ICC__ || __clang__ || defined _MSC_VER || defined __GNUC__ || defined __SES_ARM
// DELIBERATELY BLANK
#else
#error "WARNING! Your compiler might not support '#pragma pack(1)'! \
  You must add an equivalent compiler directive to the file generator!"
#endif  // defined __CC_ARM || defined __IAR_SYSTEMS_ICC__ || __clang__ || defined _MSC_VER || defined __GNUC__
#pragma pack(push,1)

#ifndef STATIC_ASSERT_PROJ
#define STATIC_ASSERT_PROJ(COND, MSG) typedef char static_assertion_##MSG[(COND)?1:-1]
#endif // STATIC_ASSERT_PROJ
typedef enum M2M2_SENSOR_AD7156_COMMAND_ENUM_t {
  _M2M2_SENSOR_AD7156_COMMAND_LOWEST = 64,
  M2M2_SENSOR_AD7156_COMMAND_LOAD_CFG_REQ = 66,
  M2M2_SENSOR_AD7156_COMMAND_LOAD_CFG_RESP = 67,
} M2M2_SENSOR_AD7156_COMMAND_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_SENSOR_AD7156_COMMAND_ENUM_t) == 1, INCORRECT_SIZE_M2M2_SENSOR_AD7156_COMMAND_ENUM_t);

typedef struct _m2m2_sensor_ad7156_resp_t {
  uint8_t  command; 
  uint8_t  status; 
} m2m2_sensor_ad7156_resp_t;

typedef struct _m2m2_sensor_ad7156_data_t {
  uint8_t  command;
  uint8_t  status; 
  uint16_t  sequence_num; 
  uint32_t  timestamp; 
  uint8_t  touch_position;//Top or Bottom
  uint8_t  touch_value;
} m2m2_sensor_ad7156_data_t;

// Reset struct packing outside of this file
#pragma pack()
