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


typedef enum M2M2_USER0_CONFIG_APP_COMMAND_ENUM_t {
  _M2M2_USER0_CONFIG_APP_COMMAND_LOWEST = 64,
  M2M2_USER0_CONFIG_APP_COMMAND_SET_STATE_REQ = 66,
  M2M2_USER0_CONFIG_APP_COMMAND_SET_STATE_RESP = 67,
  M2M2_USER0_CONFIG_APP_COMMAND_GET_STATE_REQ = 68,
  M2M2_USER0_CONFIG_APP_COMMAND_GET_STATE_RESP = 69,
  M2M2_USER0_CONFIG_APP_COMMAND_ID_OP_REQ = 70,
  M2M2_USER0_CONFIG_APP_COMMAND_ID_OP_RESP = 71,
} M2M2_USER0_CONFIG_APP_COMMAND_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_USER0_CONFIG_APP_COMMAND_ENUM_t) == 1, INCORRECT_SIZE_M2M2_USER0_CONFIG_APP_COMMAND_ENUM_t);

typedef enum M2M2_USER0_CONFIG_APP_STATUS_ENUM_t {
  _M2M2_USER0_CONFIG_APP_STATUS_LOWEST = 64,
  M2M2_USER0_CONFIG_APP_STATUS_OK = 65,
  M2M2_USER0_CONFIG_APP_STATUS_ERR_ARGS = 66,
  M2M2_USER0_CONFIG_APP_STATUS_ERR_NOT_CHKD = 255,
} M2M2_USER0_CONFIG_APP_STATUS_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_USER0_CONFIG_APP_STATUS_ENUM_t) == 1, INCORRECT_SIZE_M2M2_USER0_CONFIG_APP_STATUS_ENUM_t);

typedef enum USER0_CONFIG_APP_STATE_t {
  STATE_ADMIT_STANDBY = 0,
  STATE_START_MONITORING = 1,
  STATE_SLEEP = 2,
  STATE_INTERMITTENT_MONITORING = 3,
  STATE_INTERMITTENT_MONITORING_START_LOG = 4,
  STATE_INTERMITTENT_MONITORING_STOP_LOG = 5,
  STATE_END_MONITORING = 6,
  STATE_CHARGING_BATTERY = 7,
  STATE_OUT_OF_BATTERY_STATE_BEFORE_START_MONITORING = 8,
  STATE_OUT_OF_BATTERY_STATE_DURING_INTERMITTENT_MONITORING = 9,
} USER0_CONFIG_APP_STATE_t;
STATIC_ASSERT_PROJ(sizeof(USER0_CONFIG_APP_STATE_t) == 1, INCORRECT_SIZE_USER0_CONFIG_APP_STATE_t);

typedef enum ID_SELECTION_ENUM_t {
  ID_HW_ID = 0,
  ID_EXP_ID = 1,
} ID_SELECTION_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(ID_SELECTION_ENUM_t) == 1, INCORRECT_SIZE_ID_SELECTION_ENUM_t);

typedef enum ID_OPERATION_MODE_ENUM_t {
  ID_OPERATION_MODE_READ = 0,
  ID_OPERATION_MODE_WRITE = 1,
  ID_OPERATION_MODE_DELETE = 2,
} ID_OPERATION_MODE_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(ID_OPERATION_MODE_ENUM_t) == 1, INCORRECT_SIZE_ID_OPERATION_MODE_ENUM_t);

typedef struct _m2m2_user0_config_app_cmd_t {
  uint8_t  command; 
  uint8_t  status; 
} m2m2_user0_config_app_cmd_t;

typedef struct _user0_config_app_lcfg_op_t {
  uint8_t  field; 
  uint32_t  value; 
} user0_config_app_lcfg_op_t;

typedef struct _user0_config_app_lcfg_op_hdr_t {
  uint8_t  command; 
  uint8_t  status; 
  uint8_t  num_ops; 
  user0_config_app_lcfg_op_t  ops[0]; 
} user0_config_app_lcfg_op_hdr_t;

typedef struct _user0_config_app_set_state_t {
  uint8_t  command; 
  uint8_t  status; 
  USER0_CONFIG_APP_STATE_t  state; 
} user0_config_app_set_state_t;

typedef struct _user0_config_app_id_t {
  uint8_t  command; 
  uint8_t  status; 
  ID_SELECTION_ENUM_t  id_sel; 
  ID_OPERATION_MODE_ENUM_t  id_op; 
  uint16_t  id_num; 
} user0_config_app_id_t;

// Reset struct packing outside of this file
#pragma pack()
