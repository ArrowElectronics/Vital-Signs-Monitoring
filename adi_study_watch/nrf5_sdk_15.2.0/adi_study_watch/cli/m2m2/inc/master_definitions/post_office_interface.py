#!/usr/bin/env python3

from ctypes import *

import m2m2_core

POST_OFFICE_CFG_CMD_ENUM_t = {
  "type":c_uint8,
  "enum_values": [
    ("POST_OFFICE_CFG_CMD_ADD_MAILBOX",          0x01),
    ("POST_OFFICE_CFG_CMD_REMOVE_MAILBOX",       0x02),
    ("POST_OFFICE_CFG_CMD_MAILBOX_SUBSCRIBE",    0x03),
    ("POST_OFFICE_CFG_CMD_MAILBOX_UNSUBSCRIBE",  0x04),
    ]
}

post_office_config_t = {
  "struct_fields": [
    {"name":"cmd",
    "type":POST_OFFICE_CFG_CMD_ENUM_t},
    {"name":"box",
    "type":m2m2_core.M2M2_ADDR_ENUM_t},
    {"name":"sub",
    "type":m2m2_core.M2M2_ADDR_ENUM_t},
  ]
}
