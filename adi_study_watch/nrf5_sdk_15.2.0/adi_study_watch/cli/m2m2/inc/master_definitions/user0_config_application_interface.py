#!/usr/bin/env python3

from ctypes import *

import m2m2_core

import common_application_interface

class M2M2_USER0_CONFIG_APP_COMMAND_ENUM_t(c_uint8):
    _M2M2_USER0_CONFIG_APP_COMMAND_LOWEST = 0x40
    M2M2_USER0_CONFIG_APP_COMMAND_SET_STATE_REQ = 0x42
    M2M2_USER0_CONFIG_APP_COMMAND_SET_STATE_RESP = 0x43
    M2M2_USER0_CONFIG_APP_COMMAND_GET_STATE_REQ = 0x44
    M2M2_USER0_CONFIG_APP_COMMAND_GET_STATE_RESP = 0x45
    M2M2_USER0_CONFIG_APP_COMMAND_ID_OP_REQ = 0x46
    M2M2_USER0_CONFIG_APP_COMMAND_ID_OP_RESP = 0x47

class M2M2_USER0_CONFIG_APP_STATUS_ENUM_t(c_uint8):
    _M2M2_USER0_CONFIG_APP_STATUS_LOWEST = 0x40
    M2M2_USER0_CONFIG_APP_STATUS_OK = 0x41
    M2M2_USER0_CONFIG_APP_STATUS_ERR_ARGS = 0x42
    M2M2_USER0_CONFIG_APP_STATUS_ERR_NOT_CHKD = 0xFF

class USER0_CONFIG_APP_STATE_t(c_uint8):
    STATE_ADMIT_STANDBY = 0x0
    STATE_START_MONITORING = 0x1
    STATE_SLEEP = 0x2
    STATE_INTERMITTENT_MONITORING = 0x3
    STATE_INTERMITTENT_MONITORING_START_LOG = 0x4
    STATE_INTERMITTENT_MONITORING_STOP_LOG = 0x5
    STATE_END_MONITORING = 0x6
    STATE_CHARGING_BATTERY = 0x7
    STATE_OUT_OF_BATTERY_STATE_BEFORE_START_MONITORING = 0x8
    STATE_OUT_OF_BATTERY_STATE_DURING_INTERMITTENT_MONITORING = 0x9

class m2m2_user0_config_app_cmd_t(Structure):
    fields = [
        ("command", c_uint8),
        ("status", c_uint8),
    ]

class user0_config_app_lcfg_op_t(Structure):
    fields = [
        ("field", c_uint8),
        ("value", c_uint32),
    ]

class user0_config_app_lcfg_op_hdr_t(Structure):
    fields = [
        ("command", c_uint8),
        ("status", c_uint8),
        ("num_ops", c_uint8),
        ("ops", user0_config_app_lcfg_op_t * 0),
    ]

class user0_config_app_set_state_t(Structure):
    fields = [
        ("command", c_uint8),
        ("status", c_uint8),
        ("state", USER0_CONFIG_APP_STATE_t),
    ]

class ID_SELECTION_ENUM_t(c_uint8):
    ID_HW_ID = 0x0
    ID_EXP_ID = 0x1

class ID_OPERATION_MODE_ENUM_t(c_uint8):
    ID_OPERATION_MODE_READ = 0x0
    ID_OPERATION_MODE_WRITE = 0x1
    ID_OPERATION_MODE_DELETE = 0x2

class user0_config_app_id_t(Structure):
    fields = [
        ("command", c_uint8),
        ("status", c_uint8),
        ("id_sel", ID_SELECTION_ENUM_t),
        ("id_op", ID_OPERATION_MODE_ENUM_t),
        ("id_num", c_uint16),
    ]