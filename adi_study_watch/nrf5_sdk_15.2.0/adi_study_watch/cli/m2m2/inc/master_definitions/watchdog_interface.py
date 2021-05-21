#!/usr/bin/env python3

from ctypes import *

import m2m2_core

import common_application_interface

M2M2_WDT_CMD_ENUM_t = {
  "type":c_uint8,
  "enum_values": [
    ("M2M2_WDT_WDT_CMD_LOWEST",         0xC0),
    ("M2M2_WDT_WDT_TASK_BROADCAST_CMD", 0xC1),
    ("M2M2_WDT_PM_IS_ALIVE_CMD",        0xC2),
    ("M2M2_WDT_PS_IS_ALIVE_CMD",        0xC3),
    ]
}

M2M2_WDT_STATUS_ENUM_t = {
  "type":c_uint8,
  "enum_values": [
    ("M2M2_WDT_STATUS_OK",      0x00),
    ("M2M2_WDT_STATUS_ERROR",   0x01),
    ]
}

m2m2_wdt_cmd_t = {
  "struct_fields": [
    {"name":"command",
    "type":M2M2_WDT_CMD_ENUM_t},
    {"name":"status",
    "type":M2M2_WDT_STATUS_ENUM_t},
  ]
}
