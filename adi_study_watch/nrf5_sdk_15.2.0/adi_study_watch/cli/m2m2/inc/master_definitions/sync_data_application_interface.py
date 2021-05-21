#!/usr/bin/env python3

from ctypes import *

import m2m2_core

import common_application_interface

M2M2_SENSOR_SYNC_DATA_TYPES_ENUM_t = {
    "type":c_ubyte,
    "enum_values":
    [
        ("M2M2_SENSOR_ADPD_ADXL_SYNC_DATA",                     0x0),
    ]
}

adpd_adxl_sync_data_format_t = {
    "struct_fields":
    [
        {"name":"ppgTS",
        "type":c_ulong},
        {"name":"adxlTS",
        "type":c_ulong},
        {"name":"incPpgTS",
         "length":3,
        "type":c_ushort},
        {"name":"incAdxlTS",
         "length":3,
        "type":c_ushort},
        {"name":"ppgData",
         "length":4,
        "type":c_ulong},
        {"name":"xData",
         "length":4,
        "type":c_short},
        {"name":"yData",
         "length":4,
        "type":c_short},
        {"name":"zData",
         "length":4,
        "type":c_short},
    ]
}

adpd_adxl_sync_data_stream_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_data_stream_hdr_t},
    {"name":"syncData",
    "type":adpd_adxl_sync_data_format_t},
  ]
}
