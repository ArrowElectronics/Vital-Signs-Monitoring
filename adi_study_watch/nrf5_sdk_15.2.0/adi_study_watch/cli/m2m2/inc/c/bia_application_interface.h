// #############################################################################
// THIS IS AN AUTOMATICALLY GENERATED FILE, DO NOT MODIFY IT!
// #############################################################################
#pragma once

#include "common_application_interface.h"
#include "common_sensor_interface.h"
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


typedef enum M2M2_BIA_APP_CMD_ENUM_t {
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
} M2M2_BIA_APP_CMD_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_BIA_APP_CMD_ENUM_t) == 1, INCORRECT_SIZE_M2M2_BIA_APP_CMD_ENUM_t);

typedef enum M2M2_SENSOR_BIA_NSAMPLES_ENUM_t {
  M2M2_SENSOR_BIA_NSAMPLES = 4,
} M2M2_SENSOR_BIA_NSAMPLES_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_SENSOR_BIA_NSAMPLES_ENUM_t) == 1, INCORRECT_SIZE_M2M2_SENSOR_BIA_NSAMPLES_ENUM_t);

typedef enum M2M2_SENSOR_BIA_RAW_DATA_TYPES_ENUM_t {
  M2M2_SENSOR_BIA_DATA = 0,
} M2M2_SENSOR_BIA_RAW_DATA_TYPES_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_SENSOR_BIA_RAW_DATA_TYPES_ENUM_t) == 1, INCORRECT_SIZE_M2M2_SENSOR_BIA_RAW_DATA_TYPES_ENUM_t);

typedef enum M2M2_SENSOR_BIA_SWEEP_FREQ_INDEX_ENUM_t {
  M2M2_SENSOR_BIA_FREQ_1000HZ = 0,
  M2M2_SENSOR_BIA_FREQ_3760HZ = 1,
  M2M2_SENSOR_BIA_FREQ_14140HZ = 2,
  M2M2_SENSOR_BIA_FREQ_53180HZ = 3,
  M2M2_SENSOR_BIA_FREQ_200KHZ = 4,
  M2M2_SENSOR_BIA_FREQ_50KHZ = 255,
} M2M2_SENSOR_BIA_SWEEP_FREQ_INDEX_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_SENSOR_BIA_SWEEP_FREQ_INDEX_ENUM_t) == 1, INCORRECT_SIZE_M2M2_SENSOR_BIA_SWEEP_FREQ_INDEX_ENUM_t);

typedef enum M2M2_SENSOR_BIA_APP_INFO_BITSET_ENUM_t {
  M2M2_SENSOR_BIA_APP_INFO_BITSET_LEADSOFF = 0,
  M2M2_SENSOR_BIA_APP_INFO_BITSET_LEADSON = 1,
} M2M2_SENSOR_BIA_APP_INFO_BITSET_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_SENSOR_BIA_APP_INFO_BITSET_ENUM_t) == 1, INCORRECT_SIZE_M2M2_SENSOR_BIA_APP_INFO_BITSET_ENUM_t);

typedef struct _bia_app_set_dft_num_t {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  dftnum; 
} bia_app_set_dft_num_t;

typedef struct _bia_app_lib_state_t {
  uint8_t  command; 
  uint8_t  status; 
  uint8_t  states[10]; 
} bia_app_lib_state_t;

typedef struct _bia_app_lcfg_op_t {
  uint8_t  field; 
  float  value; 
} bia_app_lcfg_op_t;

typedef struct _bia_app_dcb_lcfg_t {
  uint8_t  command; 
  uint8_t  status; 
} bia_app_dcb_lcfg_t;

typedef struct _bia_app_lcfg_op_hdr_t {
  uint8_t  command; 
  uint8_t  status; 
  uint8_t  num_ops; 
  bia_app_lcfg_op_t  ops[0]; 
} bia_app_lcfg_op_hdr_t;

typedef struct _bia_data_set_t {
  uint32_t  timestamp; 
  int32_t  real; 
  int32_t  img; 
  M2M2_SENSOR_BIA_SWEEP_FREQ_INDEX_ENUM_t  freq_index; 
} bia_data_set_t;

typedef struct _bia_app_stream_t {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  sequence_num; 
  uint8_t  datatype; 
  M2M2_SENSOR_BIA_APP_INFO_BITSET_ENUM_t  bia_info; 
  bia_data_set_t  bia_data[4]; 
} bia_app_stream_t;

typedef struct _m2m2_bia_app_sweep_freq_resp_t {
  uint8_t  command; 
  uint8_t  status; 
} m2m2_bia_app_sweep_freq_resp_t;

typedef struct _bia_app_hs_rtia_sel_t {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  hsritasel; 
} bia_app_hs_rtia_sel_t;

typedef struct _bcm_app_algo_out_stream_t {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  sequence_num; 
  float  ffm_estimated; 
  float  bmi; 
  float  fat_percent; 
  uint32_t time_stamp;
} bcm_app_algo_out_stream_t;

typedef struct _m2m2_dcb_fds_status_info_req_t {
  uint8_t  command; 
  uint8_t  status; 
} m2m2_dcb_fds_status_info_req_t;

typedef struct _m2m2_dcb_fds_timing_info_req_t {
  uint8_t  command; 
  uint8_t  status; 
} m2m2_dcb_fds_timing_info_req_t;

typedef struct _m2m2_dcb_fds_timing_info_resp_t {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  adi_dcb_clear_entries_time; 
  uint16_t  adi_dcb_check_entries_time; 
  uint16_t  adi_dcb_delete_record_time; 
  uint16_t  adi_dcb_read_entry_time; 
  uint16_t  adi_dcb_update_entry_time; 
} m2m2_dcb_fds_timing_info_resp_t;

typedef struct _m2m2_dcb_fds_status_info_resp_t {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  dirty_records; 
  uint16_t  open_records; 
  uint16_t  valid_records; 
  uint16_t  pages_available; 
  uint16_t  num_blocks; 
  uint16_t  blocks_free; 
} m2m2_dcb_fds_status_info_resp_t;

// Reset struct packing outside of this file
#pragma pack()
