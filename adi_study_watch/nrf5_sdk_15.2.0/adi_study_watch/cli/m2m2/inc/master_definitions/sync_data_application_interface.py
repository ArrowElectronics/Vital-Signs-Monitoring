#!/usr/bin/env python3

from ctypes import *

import m2m2_core

import common_application_interface

class M2M2_SENSOR_SYNC_DATA_TYPES_ENUM_t(c_uint8):
    M2M2_SENSOR_ADPD_ADXL_SYNC_DATA = 0x0

class adpd_adxl_sync_data_format_t(Structure):
    fields = [
              ("ppgTS", c_uint32),
              ("adxlTS", c_uint32),
              ("incPpgTS", c_uint16 * 3),
              ("incAdxlTS", c_uint16 * 3),
              ("ppgData", c_uint32 * 4),
              ("xData", c_int16 * 4),
              ("yData", c_int16 * 4),
              ("zData", c_int16 * 4),
              ]

class adpd_adxl_sync_data_stream_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_data_stream_hdr_t),
              ("syncData", adpd_adxl_sync_data_format_t),
              ]