#!/usr/bin/env python3

from ctypes import *

import common_sensor_interface

import common_application_interface

import m2m2_core

import dcb_interface

M2M2_PPG_APP_CMD_ENUM_t = {
  "type":c_uint8,
  "enum_values": [
    ("_M2M2_PPG_APP_CMD_LOWEST",                        0x40),
    ("M2M2_PPG_APP_CMD_GET_LAST_STATES_REQ",            0x42),
    ("M2M2_PPG_APP_CMD_GET_LAST_STATES_RESP",           0x43),
    ("M2M2_PPG_APP_CMD_GET_STATES_INFO_REQ",            0x44),
    ("M2M2_PPG_APP_CMD_GET_STATES_INFO_RESP",           0x45),
    ("M2M2_PPG_APP_CMD_GET_ALGO_VENDOR_VERSION_REQ",    0x46),
    ("M2M2_PPG_APP_CMD_GET_ALGO_VENDOR_VERSION_RESP",   0x47),
    ("M2M2_PPG_APP_CMD_SYNC_DATA_REQ",                  0x48),
    ("M2M2_PPG_APP_CMD_SYNC_DATA_RESP",                 0x49),
    ("M2M2_PPG_APP_CMD_DEBUG_DATA_REQ",                 0x4A),
    ("M2M2_PPG_APP_CMD_DEBUG_DATA_RESP",                0x4B),
    ("M2M2_PPG_APP_CMD_GET_CTRVALUE_REQ",               0x4C),
    ("M2M2_PPG_APP_CMD_GET_CTRVALUE_RESP",              0x4D),
    ("M2M2_PPG_APP_CMD_GET_SMETRICS_REQ",               0x4E),
    ("M2M2_PPG_APP_CMD_GET_SMETRICS_RESP",              0x4F),
    ]
}

M2M2_PPG_APP_SYNC_ENUM_t = {
  "type":c_uint8,
  "enum_values": [
    ("M2M2_PPG_APP_SOFTWARE_SYNC",   0x00),
    ("M2M2_PPG_APP_HARDWARE_SYNC",   0x03),
    ]
}

M2M2_SENSOR_PPG_SYNC_NSAMPLES_ENUM_t = {
  "type":c_uint8,
  "enum_values": [
    ("M2M2_SENSOR_PPG_SYNC_NSAMPLES",            0x03),
    ]
}

M2M2_SENSOR_HRV_NSAMPLES_ENUM_t = {
  "type":c_uint8,
  "enum_values": [
    ("M2M2_SENSOR_HRV_NSAMPLES",            0x04),
    ]
}
M2M2_SENSOR_PPG_SYNC_DATA_TYPES_ENUM_t = {
  "type":c_uint8,
  "enum_values": [
    ("M2M2_SENSOR_PPG_SYNC_DATA_TYPES_NO_SYNC", 0x00),
    ("M2M2_SENSOR_PPG_SYNC_DATA_TYPES_HW_SYNC1", 0x01),
    ("M2M2_SENSOR_PPG_SYNC_DATA_TYPES_HW_SYNC2", 0x02),
    ("M2M2_SENSOR_PPG_SYNC_DATA_TYPES_HW_SYNC3", 0x03),
    ("M2M2_SENSOR_PPG_SYNC_DATA_TYPES_SW_SYNC", 0x08),
    ]
}

M2M2_SENSOR_PPG_LCFG_ID_ENUM_t = {
  "type":c_uint8,
  "enum_values": [
    ("M2M2_SENSOR_PPG_LCFG_ID_ADPD107", 0x6B),
    ("M2M2_SENSOR_PPG_LCFG_ID_ADPD185", 0xB9),
    ("M2M2_SENSOR_PPG_LCFG_ID_ADPD108", 0x6C),
    ("M2M2_SENSOR_PPG_LCFG_ID_ADPD188", 0xBC),
    ("M2M2_SENSOR_PPG_LCFG_ID_ADPD4000", 0x28),
    ]
}

ppg_app_lib_state_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"states",
    "length":10,
    "type":c_ubyte},
  ]
}

ppg_app_ctrValue_t = {
  "struct_fields": [
    {"name":None,
     "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"ctrValue",
     "type":c_ushort},
  ]
}

ppg_app_signal_metrics_t = {
  "struct_fields": [
    {"name":None,
     "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"metrics",
    "length":3,
     "type":c_ushort},
  ]
}

ppg_app_lcfg_op_t = {
  "struct_fields": [
    {"name":"field",
     "type":c_ubyte},
    {"name":"value",
     "type":c_ulong},
  ]
}

ppg_app_lcfg_op_hdr_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"num_ops",
     "type":c_ubyte},
    {"name":"ops",
    "length":0,
    "type":ppg_app_lcfg_op_t},
  ]
}

ppg_app_hr_debug_stream_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_data_stream_hdr_t},
    {"name":"timestamp",
    "type":c_ulong},
    {"name":"adpdlibstate",
    "type":c_ushort},
    {"name":"hr",
    "type":c_ushort},
    {"name":"confidence",
    "type":c_ushort},
    {"name":"hr_type",
    "type":c_ushort},
    {"name":"rr_interval",
    "type":c_ushort},
    {"name":"debugInfo",
    "length":10,
    "type":c_ushort},
  ]
}

ppg_app_state_info_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"state",
    "type":c_ubyte},
    {"name":"info",
    "length":20,
    "type":c_ubyte},
  ]
}

ppg_app_cmd_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"commandVal",
    "type":c_ushort},
  ]
}

ppg_app_set_lcfg_req_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"lcfgid",
    "type":M2M2_SENSOR_PPG_LCFG_ID_ENUM_t},
  ]
}

ppg_app_set_lcfg_resp_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
  ]
}

ppg_app_agc_info_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_data_stream_hdr_t},
    {"name":"timestamp",
    "type":c_ulong},
    {"name":"mts",
    "length":6,
    "type":c_ushort},
    {"name":"setting",
    "length":10,
    "type":c_ushort},
  ]
}

hrv_data_set_t = {
  "struct_fields": [
    {"name":"timestamp",
    "type":c_ushort},
    {"name":"rr_interval",
    "type":c_short},
    {"name":"is_gap",
    "type":c_ushort},
  ]
}

ppg_app_hrv_info_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_data_stream_hdr_t},
    {"name":"timestamp",
    "type":c_ulong},
    {"name":"first_rr_interval",
    "type":c_short},
    {"name":"first_is_gap",
    "type":c_ushort},
    {"name":"hrv_data",
    "length":3,
    "type":hrv_data_set_t},
  ]
}

m2m2_ppg_lcfg_data_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"size",
    "type":c_ubyte},
    {"name":"lcfgdata",
    "length":MAXPPGDCBSIZE,
    "type":u_long},
  ]
}

