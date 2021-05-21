// #############################################################################
// THIS IS AN AUTOMATICALLY GENERATED FILE, DO NOT MODIFY IT!
// #############################################################################
#pragma once

#include "common_application_interface.hpp"
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

enum M2M2_TEMPERATURE_APP_CMD_ENUM_t:uint8_t {
  _M2M2_TEMPERATURE_APP_CMD_LOWEST = 96,
  M2M2_TEMPERATURE_APP_CMD_SET_FS_REQ = 98,
  M2M2_TEMPERATURE_APP_CMD_SET_FS_RESP = 99,
};
static_assert(sizeof(M2M2_TEMPERATURE_APP_CMD_ENUM_t) == 1, "Enum 'M2M2_TEMPERATURE_APP_CMD_ENUM_t' has an incorrect size!");

struct temperature_app_stream_t {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  sequence_num; 
  uint32_t  nTS; 
  uint16_t  nTemperature1; 
  uint16_t  nTemperature2; 
};

// Reset struct packing outside of this file
#pragma pack()
