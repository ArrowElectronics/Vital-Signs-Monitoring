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
typedef enum M2M2_SENSOR_COMMON_STATUS_ENUM_t {
  __M2M2_SENSOR_COMMON_STATUS_LOWEST = 32,
  M2M2_SENSOR_COMMON_STATUS_RUNNING = 33,
  __M2M2_SENSOR_COMMON_STATUS_HIGHEST = 64,
} M2M2_SENSOR_COMMON_STATUS_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_SENSOR_COMMON_STATUS_ENUM_t) == 1, INCORRECT_SIZE_M2M2_SENSOR_COMMON_STATUS_ENUM_t);

typedef enum M2M2_SENSOR_COMMON_CMD_ENUM_t {
  __M2M2_SENSOR_COMMON_CMD_LOWEST = 32,
  M2M2_SENSOR_COMMON_CMD_READ_REG_16_REQ = 33,
  M2M2_SENSOR_COMMON_CMD_READ_REG_16_RESP = 34,
  M2M2_SENSOR_COMMON_CMD_WRITE_REG_16_REQ = 35,
  M2M2_SENSOR_COMMON_CMD_WRITE_REG_16_RESP = 36,
  M2M2_SENSOR_COMMON_CMD_GET_DCFG_REQ = 37,
  M2M2_SENSOR_COMMON_CMD_GET_DCFG_RESP = 38,
  M2M2_SENSOR_COMMON_CMD_STREAM_TIMESTAMP = 39,
  M2M2_SENSOR_COMMON_CMD_STREAM_DATA = 40,
  M2M2_SENSOR_COMMON_CMD_GET_STREAM_DEC_FACTOR_REQ = 41,
  M2M2_SENSOR_COMMON_CMD_GET_STREAM_DEC_FACTOR_RESP = 42,
  M2M2_SENSOR_COMMON_CMD_SET_STREAM_DEC_FACTOR_REQ = 43,
  M2M2_SENSOR_COMMON_CMD_SET_STREAM_DEC_FACTOR_RESP = 44,
  M2M2_SENSOR_COMMON_CMD_READ_REG_32_REQ = 45,
  M2M2_SENSOR_COMMON_CMD_READ_REG_32_RESP = 46,
  M2M2_SENSOR_COMMON_CMD_WRITE_REG_32_REQ = 47,
  M2M2_SENSOR_COMMON_CMD_WRITE_REG_32_RESP = 48,
  _M2M2_SENSOR_COMMON_CMD_HIGHEST_ = 64,
} M2M2_SENSOR_COMMON_CMD_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_SENSOR_COMMON_CMD_ENUM_t) == 1, INCORRECT_SIZE_M2M2_SENSOR_COMMON_CMD_ENUM_t);

typedef struct _m2m2_sensor_common_reg_op_16_t {
  uint16_t  address; 
  uint16_t  value; 
} m2m2_sensor_common_reg_op_16_t;

typedef struct _m2m2_sensor_common_reg_op_16_hdr_t {
  uint8_t  command; 
  uint8_t  status; 
  uint8_t  num_ops; 
  m2m2_sensor_common_reg_op_16_t  ops[0]; 
} m2m2_sensor_common_reg_op_16_hdr_t;

typedef struct _m2m2_sensor_common_reg_op_32_t {
  uint16_t  address; 
  uint32_t  value; 
} m2m2_sensor_common_reg_op_32_t;

typedef struct _m2m2_sensor_common_reg_op_32_hdr_t {
  uint8_t  command; 
  uint8_t  status; 
  uint8_t  num_ops; 
  m2m2_sensor_common_reg_op_32_t  ops[0]; 
} m2m2_sensor_common_reg_op_32_hdr_t;

typedef struct _m2m2_sensor_common_timestamp_t {
  uint8_t  command; 
  uint8_t  status; 
  uint32_t  timestamp; 
} m2m2_sensor_common_timestamp_t;

typedef struct _m2m2_sensor_common_decimate_stream_t {
  uint8_t  command; 
  uint8_t  status; 
  M2M2_ADDR_ENUM_t  stream; 
  uint8_t  dec_factor; 
} m2m2_sensor_common_decimate_stream_t;

// Reset struct packing outside of this file
#pragma pack()
