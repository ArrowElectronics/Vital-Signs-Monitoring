#!/usr/bin/env python3

from ctypes import *

import m2m2_core

import common_application_interface

M2M2_DISPLAY_APP_CMD_ENUM_t = {
    "type":c_uint8,
    "enum_values":
    [
        ("_M2M2_DISPLAY_APP_CMD_LOWEST",                   0x40),
        ("M2M2_DISPLAY_APP_CMD_SET_DISPLAY_REQ",           0x42),
        ("M2M2_DISPLAY_APP_CMD_SET_DISPLAY_RESP",          0x43),
        ("M2M2_DISPLAY_APP_CMD_BACKLIGHT_CNTRL_REQ",       0x44),
        ("M2M2_DISPLAY_APP_CMD_BACKLIGHT_CNTRL_RESP",      0x45),
        ("M2M2_DISPLAY_APP_CMD_KEY_TEST_REQ",              0x46),
        ("M2M2_DISPLAY_APP_CMD_KEY_TEST_RESP",             0x47),
        ("M2M2_DISPLAY_APP_CMD_KEY_STREAM_DATA",           0x48),
    ]
}

M2M2_DISPLAY_SET_COMMAND_ENUM_t = {
    "type":c_uint8,
    "enum_values":
    [
        ("M2M2_DISPLAY_SET_WHITE",                         0x0),
        ("M2M2_DISPLAY_SET_BLACK",                         0x1),
        ("M2M2_DISPLAY_SET_RED",                           0x2),
        ("M2M2_DISPLAY_SET_GREEN",                         0x3),
        ("M2M2_DISPLAY_SET_BLUE",                          0x4),
    ]
}

M2M2_BACKLIGHT_CNTRL_COMMAND_ENUM_t = {
    "type":c_uint8,
    "enum_values":
    [
        ("M2M2_BACKLIGHT_CNTRL_OFF",                       0x0),
        ("M2M2_BACKLIGHT_CNTRL_ON",                        0x1),
    ]
}

M2M2_KEY_TEST_COMMAND_ENUM_t = {
    "type":c_uint8,
    "enum_values":
    [
        ("M2M2_KEY_TEST_SELECT_BUTTON",                    0x0),
        ("M2M2_KEY_TEST_NAVIGATE_BUTTON",                  0x1),
    ]
}

m2m2_display_set_command_t = {
    "struct_fields":
    [
        {"name":"command",
        "type":c_uint8},
        {"name":"status",
        "type":c_uint8},
        {"name":"colour",
        "type":M2M2_DISPLAY_SET_COMMAND_ENUM_t},
    ]
}

m2m2_backlight_cntrl_command_t = {
    "struct_fields":
    [
        {"name":"command",
        "type":c_uint8},
        {"name":"status",
        "type":c_uint8},
        {"name":"control",
        "type":M2M2_BACKLIGHT_CNTRL_COMMAND_ENUM_t},
    ]
}

m2m2_key_test_command_t = {
    "struct_fields":
    [
        {"name":"command",
        "type":c_uint8},
        {"name":"status",
        "type":c_uint8},
        {"name":"enable",
        "type":c_uint8},
    ]
}

m2m2_pm_sys_key_test_data_t = {
    "struct_fields":
    [
        {"name":"command",
        "type":c_uint8},
        {"name":"status",
        "type":c_uint8},
        {"name":"key_value",
        "type":c_uint8},
    ]
}
