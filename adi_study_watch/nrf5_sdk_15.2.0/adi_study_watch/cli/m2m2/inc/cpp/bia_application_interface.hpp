// #############################################################################
// THIS IS AN AUTOMATICALLY GENERATED FILE, DO NOT MODIFY IT!
// #############################################################################
#pragma once

#include "common_application_interface.hpp"
#include "common_sensor_interface.hpp"
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


enum M2M2_BIA_APP_CMD_ENUM_t:uint8_t {
  _M2M2_BIA_APP_CMD_LOWEST = 64,
  M2M2_BIA_APP_CMD_SWEEP_FREQ_ENABLE_REQ = 66,
  M2M2_BIA_APP_CMD_SWEEP_FREQ_ENABLE_RESP = 67,
  M2M2_BIA_APP_CMD_SWEEP_FREQ_DISABLE_REQ = 68,
  M2M2_BIA_APP_CMD_SWEEP_FREQ_DISABLE_RESP = 69,
  M2M2_BIA_APP_CMD_SET_DFT_NUM_REQ = 70,
  M2M2_BIA_APP_CMD_SET_DFT_NUM_RESP = 71,
  M2M2_BIA_APP_CMD_SET_HS_RTIA_CAL_REQ = 72,
  M2M2_BIA_APP_CMD_SET_HS_RTIA_CAL_RESP = 73,
  M2M2_DCB_COMMAND_FDS_STATUS_REQ = 74,
  M2M2_DCB_COMMAND_FDS_STATUS_RESP = 75,
  M2M2_APP_COMMON_CMD_DCB_TIMING_INFO_REQ = 76,
  M2M2_APP_COMMON_CMD_DCB_TIMING_INFO_RESP = 77,
  M2M2_BCM_APP_CMD_ALGO_STREAM_RESP = 78,
  M2M2_BIA_APP_CMD_LOAD_DCFG_REQ = 79,
  M2M2_BIA_APP_CMD_LOAD_DCFG_RESP = 80,
  M2M2_BIA_APP_COMMON_CMD_WRITE_DCFG_REQ = 81,
  M2M2_BIA_APP_COMMON_CMD_WRITE_DCFG_RESP = 82,
  M2M2_BIA_APP_COMMON_CMD_READ_DCFG_REQ = 83,
  M2M2_BIA_APP_COMMON_CMD_READ_DCFG_RESP = 84,
};
static_assert(sizeof(M2M2_BIA_APP_CMD_ENUM_t) == 1, "Enum 'M2M2_BIA_APP_CMD_ENUM_t' has an incorrect size!");

enum M2M2_SENSOR_BIA_NSAMPLES_ENUM_t:uint8_t {
  M2M2_SENSOR_BIA_NSAMPLES = 4,
};
static_assert(sizeof(M2M2_SENSOR_BIA_NSAMPLES_ENUM_t) == 1, "Enum 'M2M2_SENSOR_BIA_NSAMPLES_ENUM_t' has an incorrect size!");

enum M2M2_SENSOR_BIA_RAW_DATA_TYPES_ENUM_t:uint8_t {
  M2M2_SENSOR_BIA_DATA = 0,
};
static_assert(sizeof(M2M2_SENSOR_BIA_RAW_DATA_TYPES_ENUM_t) == 1, "Enum 'M2M2_SENSOR_BIA_RAW_DATA_TYPES_ENUM_t' has an incorrect size!");

enum M2M2_SENSOR_BIA_SWEEP_FREQ_INDEX_ENUM_t:uint8_t {
  M2M2_SENSOR_BIA_FREQ_1000HZ = 0,
  M2M2_SENSOR_BIA_FREQ_3760HZ = 1,
  M2M2_SENSOR_BIA_FREQ_14140HZ = 2,
  M2M2_SENSOR_BIA_FREQ_53180HZ = 3,
  M2M2_SENSOR_BIA_FREQ_200KHZ = 4,
  M2M2_SENSOR_BIA_FREQ_50KHZ = 255,
};
static_assert(sizeof(M2M2_SENSOR_BIA_SWEEP_FREQ_INDEX_ENUM_t) == 1, "Enum 'M2M2_SENSOR_BIA_SWEEP_FREQ_INDEX_ENUM_t' has an incorrect size!");

enum M2M2_SENSOR_BIA_APP_INFO_BITSET_ENUM_t:uint8_t {
  M2M2_SENSOR_BIA_APP_INFO_BITSET_LEADSOFF = 0,
  M2M2_SENSOR_BIA_APP_INFO_BITSET_LEADSON = 1,
};
static_assert(sizeof(M2M2_SENSOR_BIA_APP_INFO_BITSET_ENUM_t) == 1, "Enum 'M2M2_SENSOR_BIA_APP_INFO_BITSET_ENUM_t' has an incorrect size!");

struct bia_app_set_dft_num_t {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  dftnum; 
};

struct bia_app_lib_state_t {
  uint8_t  command; 
  uint8_t  status; 
  uint8_t  states[10]; 
};

struct bia_app_lcfg_op_t {
  uint8_t  field; 
  float  value; 
};

struct bia_app_dcb_lcfg_t {
  uint8_t  command; 
  uint8_t  status; 
};

struct bia_app_lcfg_op_hdr_t {
  uint8_t  command; 
  uint8_t  status; 
  uint8_t  num_ops; 
  bia_app_lcfg_op_t  ops[0];
};

struct bia_app_dcfg_op_t {
  uint32_t  field; 
  uint32_t  value; 
};

struct bia_app_dcfg_op_hdr_t {
  uint8_t  command; 
  uint8_t  status; 
  uint8_t  num_ops; 
  bia_app_dcfg_op_t  ops[1]; // NOTE: THIS FIELD IS INTENDED TO BE OF VARIABLE LENGTH! 
        // NOTE: Use offsetof(bia_app_dcfg_op_hdr_t, ops) instead of sizeof(bia_app_dcfg_op_hdr_t)
};

struct bia_data_set_t {
  uint32_t  timestamp; 
  int32_t  real; 
  int32_t  img; 
  uint32_t  excitation_freq; 
};

struct bia_app_stream_t {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  sequence_num; 
  uint8_t  datatype; 
  M2M2_SENSOR_BIA_APP_INFO_BITSET_ENUM_t  bia_info; 
  bia_data_set_t  bia_data[4]; 
};

struct m2m2_bia_app_sweep_freq_resp_t {
  uint8_t  command; 
  uint8_t  status; 
};

struct bia_app_hs_rtia_sel_t {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  hsritasel; 
};

struct bcm_app_algo_out_stream_t {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  sequence_num; 
  float  ffm_estimated; 
  float  bmi; 
  float  fat_percent; 
  uint32_t  time_stamp; 
};

struct m2m2_dcb_fds_status_info_req_t {
  uint8_t  command; 
  uint8_t  status; 
};

struct m2m2_dcb_fds_timing_info_req_t {
  uint8_t  command; 
  uint8_t  status; 
};

struct m2m2_dcb_fds_timing_info_resp_t {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  adi_dcb_clear_entries_time; 
  uint16_t  adi_dcb_check_entries_time; 
  uint16_t  adi_dcb_delete_record_time; 
  uint16_t  adi_dcb_read_entry_time; 
  uint16_t  adi_dcb_update_entry_time; 
};

struct m2m2_dcb_fds_status_info_resp_t {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  dirty_records; 
  uint16_t  open_records; 
  uint16_t  valid_records; 
  uint16_t  pages_available; 
  uint16_t  num_blocks; 
  uint16_t  blocks_free; 
};

// Reset struct packing outside of this file
#pragma pack()
