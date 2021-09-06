#!/usr/bin/env python3

from ctypes import *

import m2m2_core

import common_application_interface

import common_sensor_interface

class M2M2_ECG_APP_CMD_ENUM_t(c_uint8):
    _M2M2_ECG_APP_CMD_LOWEST = 0x5D
    M2M2_ECG_APP_CMD_GET_ALGO_VENDOR_VERSION_REQ = 0x5E
    M2M2_ECG_APP_CMD_GET_ALGO_VENDOR_VERSION_RESP = 0x5F

class M2M2_SENSOR_ECG_NSAMPLES_ENUM_t(c_uint8):
    M2M2_SENSOR_ECG_NSAMPLES = 0xB

class M2M2_SENSOR_ECG_RAW_DATA_TYPES_ENUM_t(c_uint8):
    M2M2_SENSOR_ECG_MONITOR = 0x0
    M2M2_SENSOR_ECG_SPORT = 0x1

class M2M2_SENSOR_ECG_APP_INFO_BITSET_ENUM_t(c_uint8):
    M2M2_SENSOR_ECG_APP_INFO_BITSET_LEADSOFF = 0x0
    M2M2_SENSOR_ECG_APP_INFO_BITSET_LEADSON = 0x1

class ecg_app_lib_state_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("states", c_uint8 * 10),
              ]

class ecg_app_lcfg_op_t(Structure):
    fields = [
              ("field", c_uint8),
              ("value", c_uint16),
              ]

class ecg_app_lcfg_op_hdr_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("num_ops", c_uint8),
              ("ops", ecg_app_lcfg_op_t * 0),
              ]

class ecg_data_set_t(Structure):
    fields = [
              ("timestamp", c_uint16),
              ("ecgdata", c_uint16),
              ]

class ecg_app_stream_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_data_stream_hdr_t),
              ("datatype", M2M2_SENSOR_ECG_RAW_DATA_TYPES_ENUM_t),
              ("timestamp", c_uint32),
              ("ecg_info", M2M2_SENSOR_ECG_APP_INFO_BITSET_ENUM_t),
              ("HR", c_uint8),
              ("firstecgdata", c_uint16),
              ("ecg_data", ecg_data_set_t * 10),
              ]

class ecg_app_sync_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("sync", c_uint8),
              ]

class ecg_app_dcb_lcfg_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ]