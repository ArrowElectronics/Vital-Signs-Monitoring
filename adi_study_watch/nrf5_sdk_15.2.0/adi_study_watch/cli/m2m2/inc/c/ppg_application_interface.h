// #############################################################################
// THIS IS AN AUTOMATICALLY GENERATED FILE, DO NOT MODIFY IT!
// #############################################################################
#pragma once

#include "common_application_interface.h"
#include "common_sensor_interface.h"
#include "m2m2_core.h"
#include "dcb_interface.h"
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
typedef enum M2M2_PPG_APP_CMD_ENUM_t {
  _M2M2_PPG_APP_CMD_LOWEST = 64,
  M2M2_PPG_APP_CMD_GET_LAST_STATES_REQ = 66,
  M2M2_PPG_APP_CMD_GET_LAST_STATES_RESP = 67,
  M2M2_PPG_APP_CMD_GET_STATES_INFO_REQ = 68,
  M2M2_PPG_APP_CMD_GET_STATES_INFO_RESP = 69,
  M2M2_PPG_APP_CMD_GET_ALGO_VENDOR_VERSION_REQ = 70,
  M2M2_PPG_APP_CMD_GET_ALGO_VENDOR_VERSION_RESP = 71,
  M2M2_PPG_APP_CMD_SYNC_DATA_REQ = 72,
  M2M2_PPG_APP_CMD_SYNC_DATA_RESP = 73,
  M2M2_PPG_APP_CMD_DEBUG_DATA_REQ = 74,
  M2M2_PPG_APP_CMD_DEBUG_DATA_RESP = 75,
  M2M2_PPG_APP_CMD_GET_CTRVALUE_REQ = 76,
  M2M2_PPG_APP_CMD_GET_CTRVALUE_RESP = 77,
  M2M2_PPG_APP_CMD_GET_SMETRICS_REQ = 78,
  M2M2_PPG_APP_CMD_GET_SMETRICS_RESP = 79,
} M2M2_PPG_APP_CMD_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_PPG_APP_CMD_ENUM_t) == 1, INCORRECT_SIZE_M2M2_PPG_APP_CMD_ENUM_t);

typedef enum M2M2_PPG_APP_SYNC_ENUM_t {
  M2M2_PPG_APP_SOFTWARE_SYNC = 0,
  M2M2_PPG_APP_HARDWARE_SYNC = 3,
} M2M2_PPG_APP_SYNC_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_PPG_APP_SYNC_ENUM_t) == 1, INCORRECT_SIZE_M2M2_PPG_APP_SYNC_ENUM_t);

typedef enum M2M2_SENSOR_PPG_SYNC_NSAMPLES_ENUM_t {
  M2M2_SENSOR_PPG_SYNC_NSAMPLES = 3,
} M2M2_SENSOR_PPG_SYNC_NSAMPLES_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_SENSOR_PPG_SYNC_NSAMPLES_ENUM_t) == 1, INCORRECT_SIZE_M2M2_SENSOR_PPG_SYNC_NSAMPLES_ENUM_t);

typedef enum M2M2_SENSOR_HRV_NSAMPLES_ENUM_t {
  M2M2_SENSOR_HRV_NSAMPLES = 4,
} M2M2_SENSOR_HRV_NSAMPLES_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_SENSOR_HRV_NSAMPLES_ENUM_t) == 1, INCORRECT_SIZE_M2M2_SENSOR_HRV_NSAMPLES_ENUM_t);

typedef enum M2M2_SENSOR_PPG_SYNC_DATA_TYPES_ENUM_t {
  M2M2_SENSOR_PPG_SYNC_DATA_TYPES_NO_SYNC = 0,
  M2M2_SENSOR_PPG_SYNC_DATA_TYPES_HW_SYNC1 = 1,
  M2M2_SENSOR_PPG_SYNC_DATA_TYPES_HW_SYNC2 = 2,
  M2M2_SENSOR_PPG_SYNC_DATA_TYPES_HW_SYNC3 = 3,
  M2M2_SENSOR_PPG_SYNC_DATA_TYPES_SW_SYNC = 4,
} M2M2_SENSOR_PPG_SYNC_DATA_TYPES_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_SENSOR_PPG_SYNC_DATA_TYPES_ENUM_t) == 1, INCORRECT_SIZE_M2M2_SENSOR_PPG_SYNC_DATA_TYPES_ENUM_t);

typedef enum M2M2_SENSOR_PPG_LCFG_ID_ENUM_t {
  M2M2_SENSOR_PPG_LCFG_ID_ADPD107 = 107,
  M2M2_SENSOR_PPG_LCFG_ID_ADPD185 = 185,
  M2M2_SENSOR_PPG_LCFG_ID_ADPD108 = 108,
  M2M2_SENSOR_PPG_LCFG_ID_ADPD188 = 188,
  M2M2_SENSOR_PPG_LCFG_ID_ADPD4000 = 40,
} M2M2_SENSOR_PPG_LCFG_ID_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_SENSOR_PPG_LCFG_ID_ENUM_t) == 1, INCORRECT_SIZE_M2M2_SENSOR_PPG_LCFG_ID_ENUM_t);

typedef struct _ppg_app_lib_state_t {
  uint8_t  command;
  uint8_t  status;
  uint8_t  states[10];
} ppg_app_lib_state_t;

typedef struct _ppg_app_ctrValue_t {
  uint8_t  command;
  uint8_t  status;
  uint16_t  ctrValue;
} ppg_app_ctrValue_t;

typedef struct _ppg_app_signal_metrics_t {
  uint8_t  command;
  uint8_t  status;
  uint16_t  metrics[3];
} ppg_app_signal_metrics_t;

typedef struct _ppg_app_lcfg_op_t {
  uint8_t  field;
  uint32_t  value;
} ppg_app_lcfg_op_t;

typedef struct _ppg_app_lcfg_op_hdr_t {
  uint8_t  command;
  uint8_t  status;
  uint8_t  num_ops;
  ppg_app_lcfg_op_t  ops[0];
} ppg_app_lcfg_op_hdr_t;

typedef struct _ppg_app_hr_debug_stream_t {
  uint8_t  command;
  uint8_t  status;
  uint16_t  sequence_num;
  uint32_t  timestamp;
  uint16_t  adpdlibstate;
  uint16_t  hr;
  uint16_t  confidence;
  uint16_t  hr_type;
  uint16_t  rr_interval;
  uint16_t  debugInfo[10];
} ppg_app_hr_debug_stream_t;

typedef struct _ppg_app_state_info_t {
  uint8_t  command;
  uint8_t  status;
  uint8_t  state;
  uint8_t  info[20];
} ppg_app_state_info_t;

typedef struct _ppg_app_cmd_t {
  uint8_t  command;
  uint8_t  status;
  uint16_t  commandVal;
} ppg_app_cmd_t;

typedef struct _ppg_app_set_lcfg_req_t {
  uint8_t  command;
  uint8_t  status;
  M2M2_SENSOR_PPG_LCFG_ID_ENUM_t  lcfgid;
} ppg_app_set_lcfg_req_t;

typedef struct _ppg_app_set_lcfg_resp_t {
  uint8_t  command;
  uint8_t  status;
} ppg_app_set_lcfg_resp_t;

typedef struct _ppg_app_agc_info_t {
  uint8_t  command;
  uint8_t  status;
  uint16_t  sequence_num;
  uint32_t  timestamp;
  uint16_t  mts[6];
  uint16_t  setting[10];
} ppg_app_agc_info_t;

typedef struct _hrv_data_set_t {
  uint16_t  timestamp;
  int16_t  rr_interval;
  uint16_t  is_gap;
} hrv_data_set_t;

typedef struct _ppg_app_hrv_info_t {
  uint8_t  command;
  uint8_t  status;
  uint16_t  sequence_num;
  uint32_t  timestamp;
  int16_t  first_rr_interval;
  uint16_t  first_is_gap;
  hrv_data_set_t  hrv_data[3];
} ppg_app_hrv_info_t;

typedef struct _m2m2_ppg_lcfg_data_t {
  uint8_t  command; 
  uint8_t  status; 
  uint8_t  size; 
  int32_t  lcfgdata[MAXPPGDCBSIZE]; 
} m2m2_ppg_lcfg_data_t;


// Reset struct packing outside of this file
#pragma pack()
