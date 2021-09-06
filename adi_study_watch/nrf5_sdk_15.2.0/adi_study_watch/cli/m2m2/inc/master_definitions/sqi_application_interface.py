#!/usr/bin/env python3

from ctypes import *

import common_sensor_interface

import common_application_interface

class M2M2_SQI_APP_CMD_ENUM_t(c_uint8):
    _M2M2_SQI_APP_CMD_LOWEST = 0x5A
    M2M2_SQI_APP_CMD_SET_SLOT_REQ = 0x5E
    M2M2_SQI_APP_CMD_SET_SLOT_RESP =  0x5F
    M2M2_SQI_APP_CMD_GET_ALGO_VENDOR_VERSION_REQ = 0x60
    M2M2_SQI_APP_CMD_GET_ALGO_VENDOR_VERSION_RESP = 0x61

class sqi_app_lcfg_op_t(Structure):
    fields = [
              ("field", c_uint8),
              ("value", c_uint16),
              ]

class sqi_app_lcfg_op_hdr_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("num_ops", c_uint8),
              ("ops", sqi_app_lcfg_op_t * 0),
              ]

class sqi_app_stream_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_data_stream_hdr_t),
              ("sqi", c_float),
              ("nSQISlot", c_uint16),
              ("nAlgoStatus", c_uint16),
              ("nTimeStamp", c_uint32),
              ("nReserved", c_int8),
              ]

class sqi_app_set_slot_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("nSQISlot", c_uint16),
              ]