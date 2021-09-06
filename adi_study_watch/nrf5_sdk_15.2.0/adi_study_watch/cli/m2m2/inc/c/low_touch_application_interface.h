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


typedef enum M2M2_LT_COMMAND_ENUM_t {
  _M2M2_LT_COMMAND_ENUM_t__M2M2_LT_COMMAND_LOWEST = 64,
  M2M2_LT_COMMAND_ACTIVATE_TOUCH_SENSOR_REQ = 66,
  M2M2_LT_COMMAND_ACTIVATE_TOUCH_SENSOR_RESP = 67,
  M2M2_LT_COMMAND_DEACTIVATE_TOUCH_SENSOR_REQ = 68,
  M2M2_LT_COMMAND_DEACTIVATE_TOUCH_SENSOR_RESP = 69,
  M2M2_LT_COMMAND_RD_CH2_CAP_REQ = 70,
  M2M2_LT_COMMAND_RD_CH2_CAP_RESP = 71,
} M2M2_LT_COMMAND_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_LT_COMMAND_ENUM_t) == 1, INCORRECT_SIZE_M2M2_LT_COMMAND_ENUM_t);

typedef enum M2M2_LT_STATUS_ENUM_t {
  _M2M2_LT_STATUS_ENUM_t__M2M2_LT_STATUS_LOWEST = 64,
  M2M2_LT_STATUS_OK = 65,
  M2M2_LT_STATUS_ERR_ARGS = 66,
  M2M2_LT_STATUS_ERR_NOT_CHKD = 255,
} M2M2_LT_STATUS_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_LT_STATUS_ENUM_t) == 1, INCORRECT_SIZE_M2M2_LT_STATUS_ENUM_t);

typedef struct _m2m2_lt_cmd_t {
  uint8_t  command; 
  uint8_t  status; 
} m2m2_lt_cmd_t;

typedef struct _lt_app_lcfg_op_t {
  uint8_t  field; 
  uint16_t  value; 
} lt_app_lcfg_op_t;

typedef struct _lt_app_lcfg_op_hdr_t {
  uint8_t  command; 
  uint8_t  status; 
  uint8_t  num_ops; 
  lt_app_lcfg_op_t  ops[0]; 
} lt_app_lcfg_op_hdr_t;

typedef struct _lt_app_rd_ch2_cap {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  capVal; 
} lt_app_rd_ch2_cap;

// Reset struct packing outside of this file
#pragma pack()
