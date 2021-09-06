#!/usr/bin/env python3

from ctypes import *

import m2m2_core

import common_application_interface

class M2M2_TEMPERATURE_APP_CMD_ENUM_t(c_uint8):
    _M2M2_TEMPERATURE_APP_CMD_LOWEST = 0x60
    M2M2_TEMPERATURE_APP_CMD_SET_FS_REQ = 0x62
    M2M2_TEMPERATURE_APP_CMD_SET_FS_RESP = 0x63

class temperature_app_stream_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("sequence_num", c_uint16),
              ("nTS", c_uint32),
              ("nTemperature1", c_uint16),
              ("nTemperature2", c_uint16),
              ]
