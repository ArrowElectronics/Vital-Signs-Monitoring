// #############################################################################
// THIS IS AN AUTOMATICALLY GENERATED FILE, DO NOT MODIFY IT!
// #############################################################################
#pragma once

#include "common_application_interface.hpp"
#include "common_sensor_interface.hpp"
#include "dcb_interface.hpp"
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


enum M2M2_PPG_APP_CMD_ENUM_t:uint8_t {
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
};
static_assert(sizeof(M2M2_PPG_APP_CMD_ENUM_t) == 1, "Enum 'M2M2_PPG_APP_CMD_ENUM_t' has an incorrect size!");

enum M2M2_PPG_APP_SYNC_ENUM_t:uint8_t {
  M2M2_PPG_APP_SOFTWARE_SYNC = 0,
  M2M2_PPG_APP_HARDWARE_SYNC = 3,
};
static_assert(sizeof(M2M2_PPG_APP_SYNC_ENUM_t) == 1, "Enum 'M2M2_PPG_APP_SYNC_ENUM_t' has an incorrect size!");

enum M2M2_SENSOR_PPG_SYNC_NSAMPLES_ENUM_t:uint8_t {
  M2M2_SENSOR_PPG_SYNC_NSAMPLES = 3,
};
static_assert(sizeof(M2M2_SENSOR_PPG_SYNC_NSAMPLES_ENUM_t) == 1, "Enum 'M2M2_SENSOR_PPG_SYNC_NSAMPLES_ENUM_t' has an incorrect size!");

enum M2M2_SENSOR_HRV_NSAMPLES_ENUM_t:uint8_t {
  M2M2_SENSOR_HRV_NSAMPLES = 4,
};
static_assert(sizeof(M2M2_SENSOR_HRV_NSAMPLES_ENUM_t) == 1, "Enum 'M2M2_SENSOR_HRV_NSAMPLES_ENUM_t' has an incorrect size!");

enum M2M2_SENSOR_PPG_SYNC_DATA_TYPES_ENUM_t:uint8_t {
  M2M2_SENSOR_PPG_SYNC_DATA_TYPES_NO_SYNC = 0,
  M2M2_SENSOR_PPG_SYNC_DATA_TYPES_HW_SYNC1 = 1,
  M2M2_SENSOR_PPG_SYNC_DATA_TYPES_HW_SYNC2 = 2,
  M2M2_SENSOR_PPG_SYNC_DATA_TYPES_HW_SYNC3 = 3,
  M2M2_SENSOR_PPG_SYNC_DATA_TYPES_SW_SYNC = 4,
};
static_assert(sizeof(M2M2_SENSOR_PPG_SYNC_DATA_TYPES_ENUM_t) == 1, "Enum 'M2M2_SENSOR_PPG_SYNC_DATA_TYPES_ENUM_t' has an incorrect size!");

enum M2M2_SENSOR_PPG_LCFG_ID_ENUM_t:uint8_t {
  M2M2_SENSOR_PPG_LCFG_ID_ADPD4000 = 40,
  M2M2_SENSOR_PPG_LCFG_ID_ADPD107 = 107,
  M2M2_SENSOR_PPG_LCFG_ID_ADPD108 = 108,
  M2M2_SENSOR_PPG_LCFG_ID_ADPD185 = 185,
  M2M2_SENSOR_PPG_LCFG_ID_ADPD188 = 188,
};
static_assert(sizeof(M2M2_SENSOR_PPG_LCFG_ID_ENUM_t) == 1, "Enum 'M2M2_SENSOR_PPG_LCFG_ID_ENUM_t' has an incorrect size!");

struct ppg_app_lib_state_t {
  uint8_t  command; 
  uint8_t  status; 
  uint8_t  states[10]; 
};

struct ppg_app_ctrValue_t {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  ctrValue; 
};

struct ppg_app_signal_metrics_t {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  metrics[3]; 
};

struct ppg_app_lcfg_op_t {
  uint8_t  field; 
  uint32_t  value; 
};

struct ppg_app_lcfg_op_hdr_t {
  uint8_t  command; 
  uint8_t  status; 
  uint8_t  num_ops; 
  ppg_app_lcfg_op_t  ops[1]; // NOTE: THIS FIELD IS INTENDED TO BE OF VARIABLE LENGTH! 
        // NOTE: Use offsetof(ppg_app_lcfg_op_hdr_t, ops) instead of sizeof(ppg_app_lcfg_op_hdr_t)
};

struct ppg_app_hr_debug_stream_t {
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
};

struct ppg_app_state_info_t {
  uint8_t  command; 
  uint8_t  status; 
  uint8_t  state; 
  uint8_t  info[20]; 
};

struct ppg_app_cmd_t {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  commandVal; 
};

struct ppg_app_set_lcfg_req_t {
  uint8_t  command; 
  uint8_t  status; 
  M2M2_SENSOR_PPG_LCFG_ID_ENUM_t  lcfgid; 
};

struct ppg_app_set_lcfg_resp_t {
  uint8_t  command; 
  uint8_t  status; 
};

struct ppg_app_dynamic_agc_stream_t {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  sequence_num; 
  uint32_t  timestamp; 
  uint16_t  mts[6]; 
  uint16_t  setting[10]; 
};

struct hrv_data_set_t {
  uint16_t  timestamp; 
  int16_t  rr_interval; 
  uint16_t  is_gap; 
  uint16_t  rmssd; 
};

struct ppg_app_hrv_info_t {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  sequence_num; 
  uint32_t  timestamp; 
  int16_t  first_rr_interval; 
  uint16_t  first_is_gap; 
  uint16_t  first_rmssd; 
  hrv_data_set_t  hrv_data[3]; 
};

struct m2m2_ppg_lcfg_data_t {
  uint8_t  command; 
  uint8_t  status; 
  uint8_t  size; 
  int32_t  lcfgdata[56]; 
};

// Reset struct packing outside of this file
#pragma pack()
