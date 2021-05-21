#!/usr/bin/env python3

from ctypes import *

import m2m2_core

import common_application_interface

import common_sensor_interface

M2M2_ECG_APP_CMD_ENUM_t = {
  "type":c_uint8,
  "enum_values": [
    ("_M2M2_ECG_APP_CMD_LOWEST",                        0x5D),
    ("M2M2_ECG_APP_CMD_GET_ALGO_VENDOR_VERSION_REQ",    0x5E),
    ("M2M2_ECG_APP_CMD_GET_ALGO_VENDOR_VERSION_RESP",   0x5F),
    ]
}

M2M2_SENSOR_ECG_NSAMPLES_ENUM_t = {
  "type":c_uint8,
  "enum_values": [
    ("M2M2_SENSOR_ECG_NSAMPLES",    0xB),
    ]
}

M2M2_SENSOR_ECG_RAW_DATA_TYPES_ENUM_t = {
  "type":c_uint8,
  "enum_values": [
    ("M2M2_SENSOR_ECG_MONITOR", 0x0),
    ("M2M2_SENSOR_ECG_SPORT",   0x1),
    ]
}

M2M2_SENSOR_ECG_APP_INFO_BITSET_ENUM_t = {
  "type":c_uint8,
  "enum_values": [
    ("M2M2_SENSOR_ECG_APP_INFO_BITSET_LEADSOFF", 0x00),
    ("M2M2_SENSOR_ECG_APP_INFO_BITSET_LEADSON", 0x01),
    ]
}


ecg_app_lib_state_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"states",
    "length":10,
    "type":c_uint8},
  ]
}

ecg_app_lcfg_op_t = {
  "struct_fields": [
    {"name":"field",
    "type":c_uint8},
    {"name":"value",
    "type":c_uint16},
  ]
}

ecg_app_lcfg_op_hdr_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"num_ops",
    "type":c_uint8},
    {"name":"ops",
    "length":0,
    "type":ecg_app_lcfg_op_t},
  ]
}

ecg_data_set_t = {
  "struct_fields": [
    {"name":"timestamp",
    "type":c_uint16},
    {"name":"ecgdata",
    "type":c_uint16},
  ]
}

ecg_app_stream_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_data_stream_hdr_t},
    {"name":"datatype",
    "type":M2M2_SENSOR_ECG_RAW_DATA_TYPES_ENUM_t},
    {"name":"timestamp",
    "type":c_uint32},
    {"name":"ecg_info",
    "type":M2M2_SENSOR_ECG_APP_INFO_BITSET_ENUM_t},
    {"name":"HR",
    "type":c_uint8},
    {"name":"firstecgdata",
    "type":c_uint16},
    {"name":"ecg_data",
    "length":10,
    "type":ecg_data_set_t},
  ]
}

ecg_app_sync_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"sync",
    "type":c_uint8},
  ]
}

ecg_app_dcb_lcfg_t = {
    "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
  ]
}
