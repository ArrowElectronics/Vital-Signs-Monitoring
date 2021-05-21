#!/usr/bin/env python3

from ctypes import *

import common_sensor_interface

import common_application_interface

M2M2_SQI_APP_CMD_ENUM_t = {
  "type":c_uint8,
  "enum_values": [
    ("_M2M2_SQI_APP_CMD_LOWEST",                      0x5A),
    ("M2M2_SQI_APP_CMD_SET_SLOT_REQ",                 0x5E),
    ("M2M2_SQI_APP_CMD_SET_SLOT_RESP",                0x5F),
    ("M2M2_SQI_APP_CMD_GET_ALGO_VENDOR_VERSION_REQ",  0x60),
    ("M2M2_SQI_APP_CMD_GET_ALGO_VENDOR_VERSION_RESP", 0x61),
    ]
}

sqi_app_lcfg_op_t = {
  "struct_fields": [
    {"name":field,
    "type":c_ubyte},
    {"name":"value",
    "type":c_ushort}
  ]
}

sqi_app_lcfg_op_hdr_t = {
  "struct_fields": [
    {"name":command,
    "type":c_ubyte},
    {"name":"status",
    "type":c_ubyte},
    {"name":"num_ops",
    "type":c_ubyte},
    {"name":"ops",
    "length":0,
    "type":sqi_app_lcfg_op_t}
  ]
}

sqi_app_stream_t = {
  "struct_fields": [
    {"name":command,
    "type":c_ubyte},
    {"name":status,
    "type":c_ubyte},
    {"name":"sequence_num",
    "type":c_ushort},
    {"name":"sqi",
    "type":c_float},
    {"name":"nSQISlot",
    "type":c_ushort},
    {"name":"nAlgoStatus",
    "type":c_ushort},
    {"name":"nTimeStamp",
    "type":c_ulong},
    {"name":"nReserved",
    "type":c_byte},
  ]
}

sqi_app_set_slot_t = {
  "struct_fields": [
    {"name":command,
    "type":c_ubyte},
    {"name":status,
    "type":c_ubyte},
    {"name":"nSQISlot",
    "type":c_ushort}
  ]
}