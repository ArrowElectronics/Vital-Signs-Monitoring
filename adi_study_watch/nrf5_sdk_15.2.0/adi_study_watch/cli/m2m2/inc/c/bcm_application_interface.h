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
typedef enum M2M2_BCM_APP_CMD_ENUM_t {
  _M2M2_BCM_APP_CMD_LOWEST = 64,
  M2M2_BCM_APP_CMD_SWEEP_FREQ_ENABLE_REQ = 66,
  M2M2_BCM_APP_CMD_SWEEP_FREQ_ENABLE_RESP = 67,
  M2M2_BCM_APP_CMD_SWEEP_FREQ_DISABLE_REQ = 68,
  M2M2_BCM_APP_CMD_SWEEP_FREQ_DISABLE_RESP = 69,
  M2M2_BCM_APP_CMD_SET_DFT_NUM_REQ = 70,
  M2M2_BCM_APP_CMD_SET_DFT_NUM_RESP = 71,
  M2M2_BCM_APP_CMD_SET_HS_RTIA_CAL_REQ = 72,
  M2M2_BCM_APP_CMD_SET_HS_RTIA_CAL_RESP = 73,
  M2M2_DCB_COMMAND_FDS_STATUS_REQ = 74,
  M2M2_DCB_COMMAND_FDS_STATUS_RESP = 75,
  M2M2_APP_COMMON_CMD_DCB_TIMING_INFO_REQ = 76,
  M2M2_APP_COMMON_CMD_DCB_TIMING_INFO_RESP = 77,
} M2M2_BCM_APP_CMD_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_BCM_APP_CMD_ENUM_t) == 1, INCORRECT_SIZE_M2M2_BCM_APP_CMD_ENUM_t);

typedef enum M2M2_SENSOR_BCM_NSAMPLES_ENUM_t {
  M2M2_SENSOR_BCM_NSAMPLES = 4,
} M2M2_SENSOR_BCM_NSAMPLES_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_SENSOR_BCM_NSAMPLES_ENUM_t) == 1, INCORRECT_SIZE_M2M2_SENSOR_BCM_NSAMPLES_ENUM_t);

typedef enum M2M2_SENSOR_BCM_RAW_DATA_TYPES_ENUM_t {
  M2M2_SENSOR_BCM_DATA = 0,
} M2M2_SENSOR_BCM_RAW_DATA_TYPES_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_SENSOR_BCM_RAW_DATA_TYPES_ENUM_t) == 1, INCORRECT_SIZE_M2M2_SENSOR_BCM_RAW_DATA_TYPES_ENUM_t);

typedef enum M2M2_SENSOR_BCM_SWEEP_FREQ_INDEX_ENUM_t {
  M2M2_SENSOR_BCM_FREQ_50KHZ = 255,
  M2M2_SENSOR_BCM_FREQ_1000HZ = 0,
  M2M2_SENSOR_BCM_FREQ_3760HZ = 1,
  M2M2_SENSOR_BCM_FREQ_14140HZ = 2,
  M2M2_SENSOR_BCM_FREQ_53180HZ = 3,
  M2M2_SENSOR_BCM_FREQ_200KHZ = 4,
} M2M2_SENSOR_BCM_SWEEP_FREQ_INDEX_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_SENSOR_BCM_SWEEP_FREQ_INDEX_ENUM_t) == 1, INCORRECT_SIZE_M2M2_SENSOR_BCM_SWEEP_FREQ_INDEX_ENUM_t);

typedef struct _bcm_app_set_dft_num_t {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  dftnum; 
} bcm_app_set_dft_num_t;

typedef struct _bcm_app_lib_state_t {
  uint8_t  command; 
  uint8_t  status; 
  uint8_t  states[10]; 
} bcm_app_lib_state_t;

typedef struct _bcm_app_lcfg_op_t {
  uint8_t  field; 
  uint32_t  value; 
} bcm_app_lcfg_op_t;

typedef struct _bcm_app_lcfg_op_hdr_t {
  uint8_t  command; 
  uint8_t  status; 
  uint8_t  num_ops; 
  bcm_app_lcfg_op_t  ops[0]; 
} bcm_app_lcfg_op_hdr_t;

typedef struct _bcm_data_set_t {
  uint32_t  timestamp; 
  int32_t  real; 
  int32_t  img; 
  M2M2_SENSOR_BCM_SWEEP_FREQ_INDEX_ENUM_t  freq_index; 
} bcm_data_set_t;

typedef struct _bcm_app_dcb_lcfg_t {
  uint8_t  command; 
  uint8_t  status; 
} bcm_app_dcb_lcfg_t;

typedef struct _bcm_app_stream_t {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  sequence_num; 
  M2M2_SENSOR_BCM_RAW_DATA_TYPES_ENUM_t  datatype; 
  bcm_data_set_t  bcm_data[4]; 
} bcm_app_stream_t;

typedef struct _m2m2_bcm_app_sweep_freq_resp_t {
  uint8_t  command; 
  uint8_t  status; 
} m2m2_bcm_app_sweep_freq_resp_t;

typedef struct _bcm_app_hs_rtia_sel_t {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  hsritasel; 
} bcm_app_hs_rtia_sel_t;

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
