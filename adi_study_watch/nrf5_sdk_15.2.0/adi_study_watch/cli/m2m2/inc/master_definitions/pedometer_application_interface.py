#!/usr/bin/env python3

from ctypes import *

import m2m2_core

import common_application_interface

import common_sensor_interface #not needed

class M2M2_PED_APP_CMD_ENUM_t(c_uint8):
    _M2M2_PED_APP_CMD_LOWEST = 0x5A
    M2M2_PED_APP_CMD_GET_ALGO_VENDOR_VERSION_REQ = 0x5C
    M2M2_PED_APP_CMD_GET_ALGO_VENDOR_VERSION_RESP = 0x5D

class ped_app_lcfg_op_t(Structure):
    fields = [
              ("field", c_uint8),
              ("value", c_uint16),
              ]

class ped_app_lcfg_op_hdr_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("num_ops", c_uint8),
              ("ops", ped_app_lcfg_op_t * 0),
              ]

class pedometer_app_stream_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("sequence_num", c_uint16),
              ("nNumSteps", c_int32),
              ("nAlgoStatus", c_uint16),
              ("nTimeStamp", c_uint32),
              ("nReserved", c_int8),
              ]