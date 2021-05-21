#!/usr/bin/env python3

from ctypes import *

import m2m2_core

import common_application_interface

M2M2_LT_COMMAND_ENUM_t = {
    "type":c_uint8,
    "enum_values":
    [
        ("__M2M2_LT_COMMAND_LOWEST",                            0x40),
        ("M2M2_LT_COMMAND_ACTIVATE_TOUCH_SENSOR_REQ",           0x42),
        ("M2M2_LT_COMMAND_ACTIVATE_TOUCH_SENSOR_RESP",          0x43),
        ("M2M2_LT_COMMAND_DEACTIVATE_TOUCH_SENSOR_REQ",         0x44),
        ("M2M2_LT_COMMAND_DEACTIVATE_TOUCH_SENSOR_RESP",        0x45),
        ("M2M2_LT_COMMAND_RD_CH2_CAP_REQ",                      0x46),
        ("M2M2_LT_COMMAND_RD_CH2_CAP_RESP",                     0x47),
    ]
}

M2M2_LT_STATUS_ENUM_t = {
    "type":c_uint8,
    "enum_values":
    [
        ("__M2M2_LT_COMMAND_LOWEST",                            0x40),
        ("M2M2_LT_STATUS_OK",                                   0x41),
        ("M2M2_LT_STATUS_ERR_ARGS",                             0x42),
        ("M2M2_LT_STATUS_ERR_NOT_CHKD",                         0xFF),
    ]
}

m2m2_lt_cmd_t = {
    "struct_fields":
    [
        {"name":"command",
        "type":c_uint8},
        {"name":"status",
        "type":c_uint8},
    ]
}

lt_app_lcfg_op_t = {
    "struct_fields":
    [
        {"name":"field",
        "type":c_uint8},
        {"name":"value",
        "type":c_uint16},
    ]
}

lt_app_lcfg_op_hdr_t = {
    "struct_fields":
    [
        {"name":"command",
        "type":c_uint8},
        {"name":"status",
        "type":c_uint8},
        {"name":"num_ops",
        "type":c_uint8},
        {"name":"ops",
        "length":0,
        "type":lt_app_lcfg_op_t},
    ]
}

lt_app_rd_ch2_cap = {
    "struct_fields":
    [
        {"name":"command",
        "type":c_uint8},
        {"name":"status",
        "type":c_uint8},
        {"name":"capVal",
        "type":c_uint16},
    ]
}
