#!/usr/bin/env python3

from ctypes import *

import m2m2_core

import common_application_interface

class M2M2_LT_COMMAND_ENUM_t(c_uint8):
    __M2M2_LT_COMMAND_LOWEST = 0x40
    M2M2_LT_COMMAND_ACTIVATE_TOUCH_SENSOR_REQ = 0x42
    M2M2_LT_COMMAND_ACTIVATE_TOUCH_SENSOR_RESP = 0x43
    M2M2_LT_COMMAND_DEACTIVATE_TOUCH_SENSOR_REQ = 0x44
    M2M2_LT_COMMAND_DEACTIVATE_TOUCH_SENSOR_RESP = 0x45
    M2M2_LT_COMMAND_RD_CH2_CAP_REQ = 0x46
    M2M2_LT_COMMAND_RD_CH2_CAP_RESP = 0x47

class M2M2_LT_STATUS_ENUM_t(c_uint8):
    __M2M2_LT_STATUS_LOWEST = 0x40
    M2M2_LT_STATUS_OK = 0x41
    M2M2_LT_STATUS_ERR_ARGS = 0x42
    M2M2_LT_STATUS_ERR_NOT_CHKD = 0xFF

#seems like this struct is not needed? (just use m2m2_app_common_cmd_t)
class m2m2_lt_cmd_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ]

class lt_app_lcfg_op_t(Structure):
    fields = [
              ("field", c_uint8),
              ("value", c_uint16),
              ]

class lt_app_lcfg_op_hdr_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("num_ops", c_uint8),
              ("ops", lt_app_lcfg_op_t * 0),
              ]

class lt_app_rd_ch2_cap(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("capVal", c_uint16),
              ]