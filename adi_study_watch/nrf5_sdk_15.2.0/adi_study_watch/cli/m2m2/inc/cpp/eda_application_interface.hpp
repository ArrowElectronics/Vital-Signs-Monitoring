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


enum M2M2_EDA_APP_CMD_ENUM_t:uint8_t {
  _M2M2_EDA_APP_CMD_LOWEST = 64,
  M2M2_EDA_APP_CMD_DYNAMIC_SCALE_REQ = 66,
  M2M2_EDA_APP_CMD_DYNAMIC_SCALE_RESP = 67,
  M2M2_EDA_APP_CMD_SET_DATA_RATE_REQ = 68,
  M2M2_EDA_APP_CMD_SET_DATA_RATE_RESP = 69,
  M2M2_EDA_APP_CMD_SET_DFT_NUM_REQ = 70,
  M2M2_EDA_APP_CMD_SET_DFT_NUM_RESP = 71,
  M2M2_EDA_APP_CMD_REQ_DEBUG_INFO_REQ = 72,
  M2M2_EDA_APP_CMD_REQ_DEBUG_INFO_RESP = 73,
  M2M2_EDA_APP_CMD_RTIA_CAL_REQ = 74,
  M2M2_EDA_APP_CMD_RTIA_CAL_RESP = 75,
  M2M2_EDA_APP_CMD_BASELINE_IMP_SET_REQ = 76,
  M2M2_EDA_APP_CMD_BASELINE_IMP_SET_RESP = 77,
  M2M2_EDA_APP_CMD_BASELINE_IMP_RESET_REQ = 78,
  M2M2_EDA_APP_CMD_BASELINE_IMP_RESET_RESP = 79,
  M2M2_EDA_APP_CMD_LOAD_DCFG_REQ = 80,
  M2M2_EDA_APP_CMD_LOAD_DCFG_RESP = 81,
  M2M2_APP_COMMON_CMD_WRITE_DCFG_REQ = 82,
  M2M2_APP_COMMON_CMD_WRITE_DCFG_RESP = 83,
  M2M2_APP_COMMON_CMD_READ_DCFG_REQ = 84,
  M2M2_APP_COMMON_CMD_READ_DCFG_RESP = 85,
};
static_assert(sizeof(M2M2_EDA_APP_CMD_ENUM_t) == 1, "Enum 'M2M2_EDA_APP_CMD_ENUM_t' has an incorrect size!");

enum M2M2_SENSOR_EDA_NSAMPLES_ENUM_t:uint8_t {
  M2M2_SENSOR_EDA_NSAMPLES = 6,
};
static_assert(sizeof(M2M2_SENSOR_EDA_NSAMPLES_ENUM_t) == 1, "Enum 'M2M2_SENSOR_EDA_NSAMPLES_ENUM_t' has an incorrect size!");

enum M2M2_SENSOR_EDA_RAW_DATA_TYPES_ENUM_t:uint8_t {
  M2M2_SENSOR_EDA_DATA = 0,
};
static_assert(sizeof(M2M2_SENSOR_EDA_RAW_DATA_TYPES_ENUM_t) == 1, "Enum 'M2M2_SENSOR_EDA_RAW_DATA_TYPES_ENUM_t' has an incorrect size!");

struct eda_app_lib_state_t {
  uint8_t  command; 
  uint8_t  status; 
  uint8_t  states[10]; 
};

struct eda_app_lcfg_op_t {
  uint8_t  field; 
  uint32_t  value; 
};

struct eda_app_lcfg_op_hdr_t {
  uint8_t  command; 
  uint8_t  status; 
  uint8_t  num_ops; 
  eda_app_lcfg_op_t  ops[1]; // NOTE: THIS FIELD IS INTENDED TO BE OF VARIABLE LENGTH! 
        // NOTE: Use offsetof(eda_app_lcfg_op_hdr_t, ops) instead of sizeof(eda_app_lcfg_op_hdr_t)
};

struct eda_app_dcfg_op_t {
  uint32_t  field;
  uint32_t  value;
};

struct eda_app_dcfg_op_hdr_t {
  uint8_t  command;
  uint8_t  status;
  uint8_t  num_ops;
  eda_app_dcfg_op_t  ops[0];
};

struct eda_data_set_t {
  uint32_t  timestamp; 
  int16_t  realdata; 
  int16_t  imgdata; 
};

struct eda_app_stream_t {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  sequence_num; 
  M2M2_SENSOR_EDA_RAW_DATA_TYPES_ENUM_t  datatype; 
  eda_data_set_t  eda_data[6]; 
};

struct eda_app_dynamic_scale_t {
  uint8_t  command; 
  uint8_t  status; 
  uint8_t  dscale; 
  uint16_t  minscale; 
  uint16_t  maxscale; 
  uint16_t  lprtia; 
};

struct eda_app_set_data_rate_t {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  datarate; 
};

struct eda_app_set_dft_num_t {
  uint8_t  command; 
  uint8_t  status; 
  uint8_t  dftnum; 
};

struct eda_app_set_baseline_imp_t {
  uint8_t  command; 
  uint8_t  status; 
  float  imp_real_dft16; 
  float  imp_img_dft16; 
  float  imp_real_dft8; 
  float  imp_img_dft8; 
  uint32_t  resistor_baseline; 
};

struct eda_app_dcb_lcfg_t {
  uint8_t  command; 
  uint8_t  status; 
};

struct m2m2_get_eda_debug_info_resp_cmd_t {
  uint8_t  command; 
  uint8_t  status; 
  uint32_t  ad5940_fifo_level; 
  uint32_t  Interrupts_time_gap; 
  uint32_t  rtia_calibration_time; 
  uint32_t  delay_in_first_measurements; 
  uint32_t  packets_time_gap; 
  uint32_t  first_voltage_measure_time; 
  uint32_t  first_current_measure_time; 
  uint32_t  voltage_measure_time_gap; 
  uint32_t  EDA_Init_Time; 
  uint32_t  current_measure_time_gap; 
  uint32_t  EDA_DeInit_Time; 
  uint8_t  ad5940_fifo_overflow_status; 
};

struct eda_app_rtia_cal_t {
  uint32_t  calibrated_res; 
  uint32_t  actual_res; 
};

struct eda_app_perform_rtia_cal_t {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  minscale; 
  uint16_t  maxscale; 
  uint16_t  lowpowerrtia; 
  uint16_t  num_calibrated_values; 
  eda_app_rtia_cal_t  rtia_cal_table_val[1]; // NOTE: THIS FIELD IS INTENDED TO BE OF VARIABLE LENGTH! 
        // NOTE: Use offsetof(eda_app_perform_rtia_cal_t, rtia_cal_table_val) instead of sizeof(eda_app_perform_rtia_cal_t)
};

struct m2m2_get_eda_debug_info_req_cmd_t {
  uint8_t  command; 
  uint8_t  status; 
};

// Reset struct packing outside of this file
#pragma pack()
