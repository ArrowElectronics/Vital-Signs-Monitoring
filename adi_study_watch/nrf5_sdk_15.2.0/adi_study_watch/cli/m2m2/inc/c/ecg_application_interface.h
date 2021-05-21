// #############################################################################
// THIS IS AN AUTOMATICALLY GENERATED FILE, DO NOT MODIFY IT!
// #############################################################################
#pragma once

#include "common_application_interface.h"
#include "common_sensor_interface.h"
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
typedef enum M2M2_ECG_APP_CMD_ENUM_t {
  _M2M2_ECG_APP_CMD_LOWEST = 93,
  M2M2_ECG_APP_CMD_GET_ALGO_VENDOR_VERSION_REQ = 94,
  M2M2_ECG_APP_CMD_GET_ALGO_VENDOR_VERSION_RESP = 95,
} M2M2_ECG_APP_CMD_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_ECG_APP_CMD_ENUM_t) == 1, INCORRECT_SIZE_M2M2_ECG_APP_CMD_ENUM_t);

typedef enum M2M2_SENSOR_ECG_NSAMPLES_ENUM_t {
  M2M2_SENSOR_ECG_NSAMPLES = 11,
} M2M2_SENSOR_ECG_NSAMPLES_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_SENSOR_ECG_NSAMPLES_ENUM_t) == 1, INCORRECT_SIZE_M2M2_SENSOR_ECG_NSAMPLES_ENUM_t);

typedef enum M2M2_SENSOR_ECG_RAW_DATA_TYPES_ENUM_t {
  M2M2_SENSOR_ECG_MONITOR = 0,
  M2M2_SENSOR_ECG_SPORT = 1,
} M2M2_SENSOR_ECG_RAW_DATA_TYPES_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_SENSOR_ECG_RAW_DATA_TYPES_ENUM_t) == 1, INCORRECT_SIZE_M2M2_SENSOR_ECG_RAW_DATA_TYPES_ENUM_t);

typedef enum M2M2_SENSOR_ECG_APP_INFO_BITSET_ENUM_t {
  M2M2_SENSOR_ECG_APP_INFO_BITSET_LEADSOFF = 0,
  M2M2_SENSOR_ECG_APP_INFO_BITSET_LEADSON = 1,
} M2M2_SENSOR_ECG_APP_INFO_BITSET_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_SENSOR_ECG_APP_INFO_BITSET_ENUM_t) == 1, INCORRECT_SIZE_M2M2_SENSOR_ECG_APP_INFO_BITSET_ENUM_t);

typedef struct _ecg_app_lib_state_t {
  uint8_t  command; 
  uint8_t  status; 
  uint8_t  states[10]; 
} ecg_app_lib_state_t;

typedef struct _ecg_app_lcfg_op_t {
  uint8_t  field; 
  uint16_t  value; 
} ecg_app_lcfg_op_t;

typedef struct _ecg_app_lcfg_op_hdr_t {
  uint8_t  command; 
  uint8_t  status; 
  uint8_t  num_ops; 
  ecg_app_lcfg_op_t  ops[0]; 
} ecg_app_lcfg_op_hdr_t;

typedef struct _ecg_data_set_t {
  uint16_t  timestamp; 
  uint16_t  ecgdata; 
} ecg_data_set_t;

typedef struct _ecg_app_stream_t {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  sequence_num; 
  M2M2_SENSOR_ECG_RAW_DATA_TYPES_ENUM_t  datatype; 
  uint32_t  timestamp; 
  M2M2_SENSOR_ECG_APP_INFO_BITSET_ENUM_t  ecg_info; 
  uint8_t  HR; 
  uint16_t  firstecgdata; 
  ecg_data_set_t  ecg_data[10]; 
} ecg_app_stream_t;

typedef struct _ecg_app_sync_t {
  uint8_t  command; 
  uint8_t  status; 
  uint8_t  sync; 
} ecg_app_sync_t;

typedef struct _ecg_app_dcb_lcfg_t {
  uint8_t  command; 
  uint8_t  status; 
} ecg_app_dcb_lcfg_t;

// Reset struct packing outside of this file
#pragma pack()