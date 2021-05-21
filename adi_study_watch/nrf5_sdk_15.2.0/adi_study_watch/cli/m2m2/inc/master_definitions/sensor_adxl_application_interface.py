#!/usr/bin/env python3

from ctypes import *

import common_application_interface

import common_sensor_interface

M2M2_SENSOR_ADXL_COMMAND_ENUM_t = {
  "type":c_uint8,
  "enum_values": [
    ("_M2M2_SENSOR_ADXL_COMMAND_LOWEST",        0x40),
    ("M2M2_SENSOR_ADXL_COMMAND_LOAD_CFG_REQ",   0x42),
    ("M2M2_SENSOR_ADXL_COMMAND_LOAD_CFG_RESP",  0x43),
    ("M2M2_SENSOR_ADXL_COMMAND_SELF_TEST_REQ",  0x44),
    ("M2M2_SENSOR_ADXL_COMMAND_SELF_TEST_RESP", 0x45),
    ]
}

M2M2_SENSOR_ADXL_RAW_DATA_TYPES_ENUM_t = {
  "type":c_uint8,
  "enum_values": [
    ("M2M2_SENSOR_RAW_DATA_ADXL",   0x00),
    ]
}

M2M2_SENSOR_ADXL_NSAMPLES_ENUM_t = {
  "type":c_uint8,
  "enum_values": [
    ("M2M2_SENSOR_ADXL_NSAMPLES_NO_COMPRESS",   0x05),
    ]
}
M2M2_SENSOR_ADXL_DEVICE_ID_ENUM_t = {
  "type":c_uint16,
  "enum_values": [
    ("M2M2_SENSOR_ADXL_DEVICE_362", 362),
    ]
}
m2m2_sensor_adxl_resp_t = {
  "struct_fields": [
    {
    "name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"deviceid",
    "type":M2M2_SENSOR_ADXL_DEVICE_ID_ENUM_t},
  ]
}

adxl_no_compress_format_t = {
  "struct_fields": [
    {"name":"incTS",
    "type":c_uint16},
    {"name":"xdata",
    "type":c_int16},
    {"name":"ydata",
    "type":c_int16},
    {"name":"zdata",
    "type":c_int16},
  ]
}

adxl_data_header_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_data_stream_hdr_t},
    {"name":"data_type",
    "type":M2M2_SENSOR_ADXL_RAW_DATA_TYPES_ENUM_t},
    {"name":"timestamp",
    "type":c_uint32},
    {"name":"first_xdata",
    "type":c_int16},
    {"name":"first_ydata",
    "type":c_int16},
    {"name":"first_zdata",
    "type":c_int16},
  ]
}

m2m2_sensor_adxl_data_no_compress_stream_t = {
  "struct_fields": [
    {"name": None,
    "type":adxl_data_header_t},
    {"name":"adxldata",
    "length":4,
    "type":adxl_no_compress_format_t},
  ]
}
