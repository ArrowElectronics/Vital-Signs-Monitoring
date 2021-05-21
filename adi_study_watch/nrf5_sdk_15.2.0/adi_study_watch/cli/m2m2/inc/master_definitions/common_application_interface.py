#!/usr/bin/env python3

from ctypes import *

import m2m2_core

M2M2_APP_COMMON_STATUS_ENUM_t = {
  "type":c_uint8,
  "enum_values": [
    ("M2M2_APP_COMMON_STATUS_OK",                           0x00),
    ("M2M2_APP_COMMON_STATUS_ERROR",                        0x01),
    ("M2M2_APP_COMMON_STATUS_STREAM_STARTED",               0x02),
    ("M2M2_APP_COMMON_STATUS_STREAM_STOPPED",               0x03),
    ("M2M2_APP_COMMON_STATUS_STREAM_IN_PROGRESS",           0x04),
    ("M2M2_APP_COMMON_STATUS_STREAM_DEACTIVATED",           0x05),
    ("M2M2_APP_COMMON_STATUS_STREAM_COUNT_DECREMENT",       0x06),
    ("M2M2_APP_COMMON_STATUS_STREAM_NOT_STARTED",           0x07),
    ("M2M2_APP_COMMON_STATUS_STREAM_NOT_STOPPED",           0x08),
    ("M2M2_APP_COMMON_STATUS_SUBSCRIBER_ADDED",             0x09),
    ("M2M2_APP_COMMON_STATUS_SUBSCRIBER_REMOVED",           0x0A),
    ("M2M2_APP_COMMON_STATUS_SUBSCRIBER_COUNT_DECREMENT",   0x0B),
    ("__M2M2_APP_COMMON_STATUS_HIGHEST",                    0x20),
    ]
}

M2M2_APP_COMMON_CMD_ENUM_t = {
  "type":c_uint8,
  "enum_values": [
    ("M2M2_APP_COMMON_CMD_GET_VERSION_REQ",             0x00),
    ("M2M2_APP_COMMON_CMD_GET_VERSION_RESP",            0x01),
    ("M2M2_APP_COMMON_CMD_GET_STATUS_REQ",              0x02),
    ("M2M2_APP_COMMON_CMD_GET_STATUS_RESP",             0x03),
    ("M2M2_APP_COMMON_CMD_STREAM_START_REQ",            0x04),
    ("M2M2_APP_COMMON_CMD_STREAM_START_RESP",           0x05),
    ("M2M2_APP_COMMON_CMD_STREAM_STOP_REQ",             0x06),
    ("M2M2_APP_COMMON_CMD_STREAM_STOP_RESP",            0x07),
    ("M2M2_APP_COMMON_CMD_STREAM_PAUSE_REQ",            0x08),
    ("M2M2_APP_COMMON_CMD_STREAM_PAUSE_RESP",           0x09),
    ("M2M2_APP_COMMON_CMD_STREAM_UNPAUSE_REQ",          0x0A),
    ("M2M2_APP_COMMON_CMD_STREAM_UNPAUSE_RESP",         0x0B),
    ("M2M2_APP_COMMON_CMD_STREAM_SUBSCRIBE_REQ",        0x0C),
    ("M2M2_APP_COMMON_CMD_STREAM_SUBSCRIBE_RESP",       0x0D),
    ("M2M2_APP_COMMON_CMD_STREAM_UNSUBSCRIBE_REQ",      0x0E),
    ("M2M2_APP_COMMON_CMD_STREAM_UNSUBSCRIBE_RESP",     0x0F),
    ("M2M2_APP_COMMON_CMD_SENSOR_STATUS_QUERY_REQ",     0x10),
    ("M2M2_APP_COMMON_CMD_SENSOR_STATUS_QUERY_RESP",    0x11),
    ("M2M2_APP_COMMON_CMD_GET_LCFG_REQ",                0x12),
    ("M2M2_APP_COMMON_CMD_GET_LCFG_RESP",               0x13),
    ("M2M2_APP_COMMON_CMD_SET_LCFG_REQ",                0x14),
    ("M2M2_APP_COMMON_CMD_SET_LCFG_RESP",               0x15),
    ("M2M2_APP_COMMON_CMD_READ_LCFG_REQ",               0x16),
    ("M2M2_APP_COMMON_CMD_READ_LCFG_RESP",              0x17),
    ("M2M2_APP_COMMON_CMD_WRITE_LCFG_REQ",              0x18),
    ("M2M2_APP_COMMON_CMD_WRITE_LCFG_RESP",             0x19),
    ("M2M2_APP_COMMON_CMD_PING_REQ",                    0x1A),
    ("M2M2_APP_COMMON_CMD_PING_RESP",                   0x1B),
    ("__M2M2_APP_COMMON_CMD_HIGHEST",                   0x20),
    ]
}

_m2m2_app_common_cmd_t = {
  "struct_fields": [
    {"name":"command",
    "type":c_uint8},
    {"name":"status",
    "type":c_uint8},
  ]
}

m2m2_app_common_ver_req_t = {
  "struct_fields": [
    {"name":"command",
    "type":c_uint8},
    {"name":"status",
    "type":c_uint8},
  ]
}

m2m2_app_common_ping_t = {
  "struct_fields": [
    {"name":None,
    "type":_m2m2_app_common_cmd_t},
    {"name":"sequence_num",
    "type":c_uint32},
    {"name":"data",
    "length":230,
    "type":c_uint8},
  ]
}

m2m2_app_common_version_t = {
  "struct_fields": [
    {"name":None,
    "type":_m2m2_app_common_cmd_t},
    {"name":"major",
    "type":c_uint16},
    {"name":"minor",
    "type":c_uint16},
    {"name":"patch",
    "type":c_uint16},
    {"name":"verstr",
    "length":10,
    "type":c_uint8},
    {"name":"str",
    "length":40,
    "type":c_uint8},
  ]
}

m2m2_app_common_status_t = {
  "struct_fields": [
    {"name":None,
    "type":_m2m2_app_common_cmd_t},
    {"name":"stream",
    "type":m2m2_core.M2M2_ADDR_ENUM_t},
    {"name":"num_subscribers",
    "type":c_uint8},
    {"name":"num_start_reqs",
    "type":c_uint8},
  ]
}

m2m2_app_common_sub_op_t = {
  "struct_fields": [
    {"name":None,
    "type":_m2m2_app_common_cmd_t},
    {"name":"stream",
    "type":m2m2_core.M2M2_ADDR_ENUM_t},
  ]
}

m2m2_sensor_dcfg_data_t = {
  "struct_fields": [
    {"name":None,
    "type":_m2m2_app_common_cmd_t},
    {"name":"size",
    "type":c_uint8},
    {"name":"num_tx_pkts",
    "type":c_uint8},
    {"name":"dcfgdata",
    "length":228,
    "type":c_uint8},
  ]
}

m2m2_app_lcfg_data_t = {
  "struct_fields": [
    {"name":None,
    "type":_m2m2_app_common_cmd_t},
    {"name":"size",
    "type":c_uint8},
    {"name":"lcfgdata",
    "length":224,
    "type":c_uint8},
  ]
}

_m2m2_app_data_stream_hdr_t = {
  "struct_fields": [
    {"name":None,
    "type":_m2m2_app_common_cmd_t},
    {"name":"sequence_num",
    "type":c_uint16},
  ]
}
