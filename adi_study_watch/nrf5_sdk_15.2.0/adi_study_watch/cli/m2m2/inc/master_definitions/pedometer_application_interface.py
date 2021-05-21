#!/usr/bin/env python3

from ctypes import *

import m2m2_core

import common_application_interface

import common_sensor_interface

M2M2_PED_APP_CMD_ENUM_t = {
    "type":c_uint8,
    "enum_values":
    [
        ("_M2M2_PED_APP_CMD_LOWEST",                               0x5A),
        ("M2M2_PED_APP_CMD_GET_ALGO_VENDOR_VERSION_REQ",           0x5C),
        ("M2M2_PED_APP_CMD_GET_ALGO_VENDOR_VERSION_RESP",          0x5D),
    ]
}

ped_app_lcfg_op_t = {
    "struct_fields":
    [
        {"name":"field",
        "type":c_ubyte},
        {"name":"value",
        "type":c_ushort},
    ]
}

ped_app_lcfg_op_hdr_t = {
    "struct_fields":
    [
        {"name":"command",
        "type":c_ubyte},
        {"name":"status",
        "type":c_ubyte},
        {"name":"num_ops",
        "type":c_ubyte},
        {"name": "ops",
        "length":0,
        "type": ped_app_lcfg_op_t},
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

