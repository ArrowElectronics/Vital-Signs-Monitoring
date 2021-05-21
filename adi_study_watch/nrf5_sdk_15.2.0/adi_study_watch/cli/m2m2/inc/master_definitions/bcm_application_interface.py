#!/usr/bin/env python3

from ctypes import *

import common_application_interface

import common_sensor_interface

M2M2_BCM_APP_CMD_ENUM_t = {
  "type":c_uint8,
  "enum_values": [
    ("_M2M2_BCM_APP_CMD_LOWEST",                    0x40),
    ("M2M2_BCM_APP_CMD_SWEEP_FREQ_ENABLE_REQ",      0x42),
    ("M2M2_BCM_APP_CMD_SWEEP_FREQ_ENABLE_RESP",     0x43),
    ("M2M2_BCM_APP_CMD_SWEEP_FREQ_DISABLE_REQ",     0x44),
    ("M2M2_BCM_APP_CMD_SWEEP_FREQ_DISABLE_RESP",    0x45),
    ("M2M2_BCM_APP_CMD_SET_DFT_NUM_REQ",            0x46),
    ("M2M2_BCM_APP_CMD_SET_DFT_NUM_RESP",           0x47),
    ("M2M2_BCM_APP_CMD_SET_HS_RTIA_CAL_REQ",        0x48),
    ("M2M2_BCM_APP_CMD_SET_HS_RTIA_CAL_RESP",       0x49),
    ("M2M2_DCB_COMMAND_FDS_STATUS_REQ",             0x4A),
    ("M2M2_DCB_COMMAND_FDS_STATUS_RESP",            0x4B),
    ("M2M2_APP_COMMON_CMD_DCB_TIMING_INFO_REQ",     0x4C),
    ("M2M2_APP_COMMON_CMD_DCB_TIMING_INFO_RESP",    0x4D),
    ]
}

M2M2_SENSOR_BCM_NSAMPLES_ENUM_t = {
  "type":c_uint8,
  "enum_values": [
    ("M2M2_SENSOR_BCM_NSAMPLES",        0x4),
    ]
}

M2M2_SENSOR_BCM_RAW_DATA_TYPES_ENUM_t = {
  "type":c_uint8,
  "enum_values": [
    ("M2M2_SENSOR_BCM_DATA",   0x00),
    ]
}

M2M2_SENSOR_BCM_SWEEP_FREQ_INDEX_ENUM_t = {
  "type":c_uint8,
  "enum_values": [
    ("M2M2_SENSOR_BCM_FREQ_50KHZ",     0xFF),
    ("M2M2_SENSOR_BCM_FREQ_1000HZ",    0x00),
    ("M2M2_SENSOR_BCM_FREQ_3760HZ",    0x01),
    ("M2M2_SENSOR_BCM_FREQ_14140HZ",   0x02),
    ("M2M2_SENSOR_BCM_FREQ_53180HZ",   0x03),
    ("M2M2_SENSOR_BCM_FREQ_200KHZ",    0x04),
    ]
}

bcm_app_set_dft_num_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"dftnum",
    "type":c_uint16},
  ]
}

bcm_app_lib_state_t = {
  "struct_fields": [
    {
    "name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"states",
    "length":10,
    "type":c_uint8},
  ]
}

bcm_app_lcfg_op_t = {
  "struct_fields": [
    {"name":"field",
    "type":c_uint8},
    {"name":"value",
    "type":c_uint32},
  ]
}

bcm_app_lcfg_op_hdr_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"num_ops",
    "type":c_uint8},
    {"name":"ops",
    "length":0,
    "type":bcm_app_lcfg_op_t},
  ]
}

bcm_data_set_t = {
  "struct_fields": [
    {"name": "timestamp",
    "type":c_uint32},
    {"name":"real",
    "type":c_int32},
    {"name":"img",
    "type":c_int32},
    {"name":"freq_index",
    "type":M2M2_SENSOR_BCM_SWEEP_FREQ_INDEX_ENUM_t},
  ]
}

bcm_app_stream_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_data_stream_hdr_t},
    {"name":"datatype",
    "type":M2M2_SENSOR_BCM_RAW_DATA_TYPES_ENUM_t},
    {"name":"bcm_data",
    "length":4,
    "type":bcm_data_set_t},
  ]
}

m2m2_bcm_app_sweep_freq_resp_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
  ]
}

bcm_app_hs_rtia_sel_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"hsritasel",
    "type":c_uint16},
  ]
}

m2m2_dcb_fds_status_info_req_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
  ]
}

m2m2_dcb_fds_timing_info_req_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
  ]
}

m2m2_dcb_fds_timing_info_resp_t  = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"adi_dcb_clear_entries_time",
    "type":c_uint16},
    {"name":"adi_dcb_check_entries_time",
    "type":c_uint16},
    {"name":"adi_dcb_delete_record_time",
    "type":c_uint16},
    {"name":"adi_dcb_read_entry_time",
    "type":c_uint16},
    {"name":"adi_dcb_update_entry_time",
    "type":c_uint16},
  ]
}

m2m2_dcb_fds_status_info_resp_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t}, 
    {"name":"dirty_records", 
    "type":c_uint16}, 
    {"name":"open_records",
    "type":c_uint16}, 
    {"name":"valid_records", 
    "type":c_uint16}, 
    {"name":"pages_available",
    "type":c_uint16}, 
    {"name":"num_blocks",
    "type":c_uint16},
    {"name":"blocks_free",
    "type":c_uint16},
  ]
}

