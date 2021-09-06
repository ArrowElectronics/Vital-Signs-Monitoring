// #############################################################################
// THIS IS AN AUTOMATICALLY GENERATED FILE, DO NOT MODIFY IT!
// #############################################################################
#pragma once

#include "common_application_interface.hpp"
#include "common_sensor_interface.hpp"
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


enum M2M2_PED_APP_CMD_ENUM_t:uint8_t {
  _M2M2_PED_APP_CMD_LOWEST = 90,
  M2M2_PED_APP_CMD_GET_ALGO_VENDOR_VERSION_REQ = 92,
  M2M2_PED_APP_CMD_GET_ALGO_VENDOR_VERSION_RESP = 93,
};
static_assert(sizeof(M2M2_PED_APP_CMD_ENUM_t) == 1, "Enum 'M2M2_PED_APP_CMD_ENUM_t' has an incorrect size!");

struct ped_app_lcfg_op_t {
  uint8_t  field; 
  uint16_t  value; 
};

struct ped_app_lcfg_op_hdr_t {
  uint8_t  command; 
  uint8_t  status; 
  uint8_t  num_ops; 
  ped_app_lcfg_op_t  ops[1]; // NOTE: THIS FIELD IS INTENDED TO BE OF VARIABLE LENGTH! 
        // NOTE: Use offsetof(ped_app_lcfg_op_hdr_t, ops) instead of sizeof(ped_app_lcfg_op_hdr_t)
};

struct pedometer_app_stream_t {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  sequence_num; 
  int32_t  nNumSteps; 
  uint16_t  nAlgoStatus; 
  uint32_t  nTimeStamp; 
  int8_t  nReserved; 
};

// Reset struct packing outside of this file
#pragma pack()
