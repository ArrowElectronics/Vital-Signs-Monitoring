#!/usr/bin/env python3

from ctypes import *

import common_application_interface

import common_sensor_interface #not needed

class M2M2_SENSOR_ADXL_COMMAND_ENUM_t(c_uint8):
    _M2M2_SENSOR_ADXL_COMMAND_LOWEST = 0x40
    M2M2_SENSOR_ADXL_COMMAND_LOAD_CFG_REQ = 0x42
    M2M2_SENSOR_ADXL_COMMAND_LOAD_CFG_RESP = 0x43
    M2M2_SENSOR_ADXL_COMMAND_SELF_TEST_REQ = 0x44
    M2M2_SENSOR_ADXL_COMMAND_SELF_TEST_RESP = 0x45

class M2M2_SENSOR_ADXL_RAW_DATA_TYPES_ENUM_t(c_uint8):
    M2M2_SENSOR_RAW_DATA_ADXL = 0x0

class M2M2_SENSOR_ADXL_NSAMPLES_ENUM_t(c_uint8):
    M2M2_SENSOR_ADXL_NSAMPLES_NO_COMPRESS = 0x5

class M2M2_SENSOR_ADXL_DEVICE_ID_ENUM_t(c_uint16):
    M2M2_SENSOR_ADXL_DEVICE_362 = 0x16A

class m2m2_sensor_adxl_resp_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("deviceid", M2M2_SENSOR_ADXL_DEVICE_ID_ENUM_t),
              ]

class adxl_no_compress_format_t(Structure):
    fields = [
              ("incTS", c_uint16),
              ("xdata", c_int16),
              ("ydata", c_int16),
              ("zdata", c_int16),
              ]

class adxl_data_header_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("sequence_num", c_uint16),
              ("data_type", M2M2_SENSOR_ADXL_RAW_DATA_TYPES_ENUM_t),
              ("timestamp", c_uint32),
              ("first_xdata", c_int16),
              ("first_ydata", c_int16),
              ("first_zdata", c_int16),
              ]

class m2m2_sensor_adxl_data_no_compress_stream_t(Structure):
    fields = [
              (None, adxl_data_header_t),
              ("adxldata", adxl_no_compress_format_t * 4),
              ]
