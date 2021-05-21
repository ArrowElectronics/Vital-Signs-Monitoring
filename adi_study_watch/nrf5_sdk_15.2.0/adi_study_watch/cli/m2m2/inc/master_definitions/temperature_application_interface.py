#!/usr/bin/env python3

from ctypes import *

import m2m2_core

import common_application_interface

M2M2_TEMPERATURE_APP_CMD_ENUM_t = {
  "type":c_uint8,
  "enum_values": [
    ("_M2M2_TEMPERATURE_APP_CMD_LOWEST",        0x60),
    ("M2M2_TEMPERATURE_APP_CMD_SET_FS_REQ",     0x62),
    ("M2M2_TEMPERATURE_APP_CMD_SET_FS_RESP",    0x63),
    ]
}

temperature_app_stream_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_data_stream_hdr_t},
    {"name":"status",
    "type":c_ubyte},
    {"name":"nTS",
    "type":c_ulong},
    {"name":"sequence_num",
    "type":c_ushort},
    {"name":"nTemperature1",
    "type":c_ushort},
    {"name":"nTemperature2",
    "type":c_ushort},
  ]
}
