#!/usr/bin/env python3

from ctypes import *

import common_sensor_interface

import common_application_interface

import m2m2_core

import dcb_interface

class M2M2_PPG_APP_CMD_ENUM_t(c_uint8):
    _M2M2_PPG_APP_CMD_LOWEST = 0x40
    M2M2_PPG_APP_CMD_GET_LAST_STATES_REQ = 0x42
    M2M2_PPG_APP_CMD_GET_LAST_STATES_RESP = 0x43
    M2M2_PPG_APP_CMD_GET_STATES_INFO_REQ = 0x44
    M2M2_PPG_APP_CMD_GET_STATES_INFO_RESP = 0x45
    M2M2_PPG_APP_CMD_GET_ALGO_VENDOR_VERSION_REQ = 0x46
    M2M2_PPG_APP_CMD_GET_ALGO_VENDOR_VERSION_RESP = 0x47
    M2M2_PPG_APP_CMD_SYNC_DATA_REQ = 0x48
    M2M2_PPG_APP_CMD_SYNC_DATA_RESP = 0x49
    M2M2_PPG_APP_CMD_DEBUG_DATA_REQ = 0x4A
    M2M2_PPG_APP_CMD_DEBUG_DATA_RESP = 0x4B
    M2M2_PPG_APP_CMD_GET_CTRVALUE_REQ = 0x4C
    M2M2_PPG_APP_CMD_GET_CTRVALUE_RESP = 0x4D
    M2M2_PPG_APP_CMD_GET_SMETRICS_REQ = 0x4E
    M2M2_PPG_APP_CMD_GET_SMETRICS_RESP = 0x4F

class M2M2_PPG_APP_SYNC_ENUM_t(c_uint8):
    M2M2_PPG_APP_SOFTWARE_SYNC = 0x0
    M2M2_PPG_APP_HARDWARE_SYNC = 0x3

class M2M2_SENSOR_PPG_SYNC_NSAMPLES_ENUM_t(c_uint8):
    M2M2_SENSOR_PPG_SYNC_NSAMPLES = 0x3

class M2M2_SENSOR_HRV_NSAMPLES_ENUM_t(c_uint8):
    M2M2_SENSOR_HRV_NSAMPLES = 0x4

class M2M2_SENSOR_PPG_SYNC_DATA_TYPES_ENUM_t(c_uint8):
    M2M2_SENSOR_PPG_SYNC_DATA_TYPES_NO_SYNC = 0x0
    M2M2_SENSOR_PPG_SYNC_DATA_TYPES_HW_SYNC1 = 0x1
    M2M2_SENSOR_PPG_SYNC_DATA_TYPES_HW_SYNC2 = 0x2
    M2M2_SENSOR_PPG_SYNC_DATA_TYPES_HW_SYNC3 = 0x3
    M2M2_SENSOR_PPG_SYNC_DATA_TYPES_SW_SYNC = 0x4

class M2M2_SENSOR_PPG_LCFG_ID_ENUM_t(c_uint8):
    M2M2_SENSOR_PPG_LCFG_ID_ADPD107 = 0x6B
    M2M2_SENSOR_PPG_LCFG_ID_ADPD185 = 0xB9
    M2M2_SENSOR_PPG_LCFG_ID_ADPD108 = 0x6C
    M2M2_SENSOR_PPG_LCFG_ID_ADPD188 = 0xBC
    M2M2_SENSOR_PPG_LCFG_ID_ADPD4000 = 0x28

class ppg_app_lib_state_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("states", c_uint8 * 10),
              ]

class ppg_app_ctrValue_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("ctrValue", c_uint16),
              ]

class ppg_app_signal_metrics_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("metrics", c_uint16 * 3),
              ]

class ppg_app_lcfg_op_t(Structure):
    fields = [
              ("field", c_uint8),
              ("value", c_uint32),
              ]

class ppg_app_lcfg_op_hdr_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("num_ops", c_uint8),
              ("ops", ppg_app_lcfg_op_t * 0),
              ]

class ppg_app_hr_debug_stream_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_data_stream_hdr_t),
              ("timestamp", c_uint32),
              ("adpdlibstate", c_uint16),
              ("hr", c_uint16),
              ("confidence", c_uint16),
              ("hr_type", c_uint16),
              ("rr_interval", c_uint16),
              ("debugInfo", c_uint16 * 10),
              ]

class ppg_app_state_info_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("state", c_uint8),
              ("info", c_uint8 * 20),
              ]

class ppg_app_cmd_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("commandVal", c_uint16),
              ]

class ppg_app_set_lcfg_req_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("lcfgid", M2M2_SENSOR_PPG_LCFG_ID_ENUM_t),
              ]

class ppg_app_set_lcfg_resp_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ]

class ppg_app_agc_info_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_data_stream_hdr_t),
              ("timestamp", c_uint32),
              ("mts", c_uint16 * 6),
              ("setting", c_uint16 * 10),
              ]

class hrv_data_set_t(Structure):
    fields = [
              ("timestamp", c_uint16),
              ("rr_interval", c_int16),
              ("is_gap", c_uint16),
              ("rmssd", c_uint16),
              ]

class ppg_app_hrv_info_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_data_stream_hdr_t),
              ("timestamp", c_uint32),
              ("first_rr_interval", c_int16),
              ("first_is_gap", c_uint16),
              ("first_rmssd", c_uint16),
              ("hrv_data", hrv_data_set_t * 3),
              ]

class m2m2_ppg_lcfg_data_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("size", c_uint8),
              ("lcfgdata", c_int32 * dcb_interface.MAXPPGDCBSIZE),
              ]
