#!/usr/bin/env python3

from ctypes import *

import m2m2_core

import common_application_interface

import common_sensor_interface #not needed?

class M2M2_SENSOR_AD7156_COMMAND_ENUM_t(c_uint8):
    _M2M2_SENSOR_AD7156_COMMAND_LOWEST = 0x40
    M2M2_SENSOR_AD7156_COMMAND_LOAD_CFG_REQ = 0x42
    M2M2_SENSOR_AD7156_COMMAND_LOAD_CFG_RESP = 0x43

class m2m2_sensor_ad7156_resp_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ]

class m2m2_sensor_ad7156_data_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("sequence_num", c_uint16),
              ("timestamp", c_uint32),
              ("ch1_cap", c_uint16),
              ("ch2_cap", c_uint16),
              ]

