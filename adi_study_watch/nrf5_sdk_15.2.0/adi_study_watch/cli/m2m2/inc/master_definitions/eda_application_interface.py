#!/usr/bin/env python3

from ctypes import *

import m2m2_core

import common_sensor_interface

import common_application_interface

M2M2_EDA_APP_CMD_ENUM_t = {
  "type":c_uint8,
  "enum_values": [
    ("_M2M2_EDA_APP_CMD_LOWEST",                0x40),
    ("M2M2_EDA_APP_CMD_DYNAMIC_SCALE_REQ",      0x42),
    ("M2M2_EDA_APP_CMD_DYNAMIC_SCALE_RESP",     0x43),
	  ("M2M2_EDA_APP_CMD_SET_DATA_RATE_REQ",      0x44),
	  ("M2M2_EDA_APP_CMD_SET_DATA_RATE_RESP",     0x45),
	  ("M2M2_EDA_APP_CMD_SET_DFT_NUM_REQ",        0x46),
	  ("M2M2_EDA_APP_CMD_SET_DFT_NUM_RESP",       0x47),
    ("M2M2_EDA_APP_CMD_REQ_DEBUG_INFO_REQ",     0x48),
    ("M2M2_EDA_APP_CMD_REQ_DEBUG_INFO_RESP",    0x49),
    ("M2M2_EDA_APP_CMD_RTIA_CAL_REQ",           0x4A),
    ("M2M2_EDA_APP_CMD_RTIA_CAL_RESP",          0x4B),
    ]
}

M2M2_SENSOR_EDA_NSAMPLES_ENUM_t = {
  "type":c_uint8,
  "enum_values": [
    ("M2M2_SENSOR_EDA_NSAMPLES",    0x6),
    ]
}

M2M2_SENSOR_EDA_RAW_DATA_TYPES_ENUM_t = {
  "type":c_uint8,
  "enum_values": [
    ("M2M2_SENSOR_EDA_DATA",    0x0),
    ]
}

eda_app_lib_state_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"states",
    "length":10,
    "type":c_uint8},
  ]
}

eda_app_lcfg_op_t = {
  "struct_fields": [
    {"name":"field",
    "type":c_uint8},
    {"name":"value",
    "type":c_uint32},
  ]
}

eda_app_lcfg_op_hdr_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"num_ops",
    "type":c_uint8},
    {"name":"ops",
    "length":0,
    "type":eda_app_lcfg_op_t},
  ]
}

eda_data_set_t = {
  "struct_fields": [
    {"name":"timestamp",
    "type":c_uint32},
    {"name":"realdata",
    "type":c_int16},
    {"name":"imgdata",
    "type":c_int16},
  ]
}

eda_app_stream_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_data_stream_hdr_t},
    {"name":"datatype",
    "type":M2M2_SENSOR_EDA_RAW_DATA_TYPES_ENUM_t},
    {"name":"eda_data",
    "length":6,
    "type":eda_data_set_t},
  ]
}

eda_app_dynamic_scale_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"dscale",
    "type":c_uint8},
    {"name":"minscale",
    "type":c_uint16},
    {"name":"maxscale",
    "type":c_uint16},
    {"name":"lprtia",
    "type":c_uint16},
  ]
}

eda_app_set_data_rate_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"datarate",
    "type":c_uint16},
  ]
}

eda_app_set_dft_num_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"dftnum",
    "type":c_uint8},
  ]
}

eda_app_dcb_lcfg_t = {
  "struct_fields": [
    {"name":None,
      "type":common_application_interface._m2m2_app_common_cmd_t},
  ]
}

m2m2_get_eda_debug_info_resp_cmd_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"ad5940_fifo_level",
    "type":c_uint32},
    {"name":"Interrupts_time_gap",
    "type":c_uint32},
    {"name":"rtia_calibration_time",
    "type":c_uint32},
    {"name":"delay_in_first_measurements",
    "type":c_uint32},
    {"name":"packets_time_gap",
    "type":c_uint32},
    {"name":"first_voltage_measure_time",
    "type":c_uint32},
    {"name":"first_current_measure_time",
    "type":c_uint32},
    {"name":"voltage_measure_time_gap",
    "type":c_uint32},
    {"name":"EDA_Init_Time",
    "type":c_uint32},
    {"name":"current_measure_time_gap",
    "type":c_uint32},
    {"name":"EDA_DeInit_Time",
    "type":c_uint32},
    {"name":"ad5940_fifo_overflow_status",
    "type":c_uint8},
  ]
}

eda_app_rtia_cal_t  =  {
  "struct_fields": [
  {"name":"calibrated_res",
  "type":c_uint32},
  {"name":"actual_res",
  "type":c_uint32},
  ]
}

eda_app_perform_rtia_cal_t  = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"minscale",
    "type":c_uint16},
    {"name":"maxscale",
    "type":c_uint16},
    {"name":"lowpowerrtia",
    "type":c_uint16},
    {"name":"num_calibrated_values",
    "type":c_uint16},
    {"name":"rtia_cal_table_val",
    "length":0,
    "type":eda_app_rtia_cal_t},
  ]
}

m2m2_get_eda_debug_info_req_cmd_t = {
  "struct_fields": [
    {"name":None,
      "type":common_application_interface._m2m2_app_common_cmd_t},
  ]
}
