#!/usr/bin/env python3

from ctypes import *

import m2m2_core

import common_application_interface

import common_sensor_interface

M2M2_SENSOR_AD7156_COMMAND_ENUM_t = {
    "type":c_uint8,
    "enum_values":
    [
        ("_M2M2_SENSOR_AD7156_COMMAND_LOWEST",                0x40),
        ("M2M2_SENSOR_AD7156_COMMAND_LOAD_CFG_REQ",           0x42),
        ("M2M2_SENSOR_AD7156_COMMAND_LOAD_CFG_RESP",          0x43),
    ]
}

m2m2_sensor_ad7156_resp_t = {
    "struct_fields":
    [
        {"name":"command",
        "type":c_ubyte},
        {"name":"status",
        "type":c_ubyte},
    ]
}

m2m2_sensor_ad7156_data_t = {
    "struct_fields":
    [
        {"name":"command",
        "type":c_ubyte},
        {"name":"status",
        "type":c_ubyte},
        {"name":"sequence_num",
        "type":c_ushort},
        {"name": "timestamp",
        "type":c_ulong},
        {"name":"touch_position",
        "type":c_ubyte},
        {"name":"touch_value",
        "type":c_ubyte},
    ]
}

pedometer_app_stream_t = {
    "struct_fields":
    [
        {"name":"command",
        "type":c_ubyte},
        {"name":"status",
        "type":c_ushort},
        {"name":"sequence_num",
        "type":c_ushort},
        {"name":"uNumSteps",
        "type":c_long},
        {"name":"nAlgoStatus",
        "type":c_ushort},
        {"name":"nTimeStamp",
        "type":c_ulong},
        {"name":"nReserved",
        "type":c_byte},
    ]
}

