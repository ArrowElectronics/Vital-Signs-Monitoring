#!/usr/bin/env python3

from ctypes import *

import common_application_interface

import common_sensor_interface

class M2M2_BCM_APP_CMD_ENUM_t(c_uint8):
    _M2M2_BCM_APP_CMD_LOWEST = 0x40
    M2M2_BCM_APP_CMD_SWEEP_FREQ_ENABLE_REQ = 0x42
    M2M2_BCM_APP_CMD_SWEEP_FREQ_ENABLE_RESP = 0x43
    M2M2_BCM_APP_CMD_SWEEP_FREQ_DISABLE_REQ = 0x44
    M2M2_BCM_APP_CMD_SWEEP_FREQ_DISABLE_RESP = 0x45
    M2M2_BCM_APP_CMD_SET_DFT_NUM_REQ = 0x46
    M2M2_BCM_APP_CMD_SET_DFT_NUM_RESP = 0x47
    M2M2_BCM_APP_CMD_SET_HS_RTIA_CAL_REQ = 0x48
    M2M2_BCM_APP_CMD_SET_HS_RTIA_CAL_RESP = 0x49
    M2M2_DCB_COMMAND_FDS_STATUS_REQ = 0x4A
    M2M2_DCB_COMMAND_FDS_STATUS_RESP = 0x4B
    M2M2_APP_COMMON_CMD_DCB_TIMING_INFO_REQ = 0x4C
    M2M2_APP_COMMON_CMD_DCB_TIMING_INFO_RESP = 0x4D

class M2M2_SENSOR_BCM_NSAMPLES_ENUM_t(c_uint8):
    M2M2_SENSOR_BCM_NSAMPLES = 0x4

class M2M2_SENSOR_BCM_RAW_DATA_TYPES_ENUM_t(c_uint8):
    M2M2_SENSOR_BCM_DATA = 0x0

class M2M2_SENSOR_BCM_SWEEP_FREQ_INDEX_ENUM_t(c_uint8):
    M2M2_SENSOR_BCM_FREQ_50KHZ = 0xFF
    M2M2_SENSOR_BCM_FREQ_1000HZ = 0x0
    M2M2_SENSOR_BCM_FREQ_3760HZ = 0x1
    M2M2_SENSOR_BCM_FREQ_14140HZ = 0x2
    M2M2_SENSOR_BCM_FREQ_53180HZ = 0x3
    M2M2_SENSOR_BCM_FREQ_200KHZ = 0x4

class bcm_app_set_dft_num_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("dftnum", c_uint16),
              ]

class bcm_app_lib_state_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("states", c_uint8 * 10),
              ]

class bcm_app_lcfg_op_t(Structure):
    fields = [
              ("field", c_uint8),
              ("value", c_uint32),
              ]

class bcm_app_dcb_lcfg_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
               ]

class bcm_app_lcfg_op_hdr_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("num_ops", c_uint8),
              ("ops", bcm_app_lcfg_op_t * 0),
              ]

class bcm_data_set_t(Structure):
    fields = [
              ("timestamp", c_uint32),
              ("real", c_int32),
              ("img", c_int32),
              ("freq_index", c_uint8),
              ]

class bcm_app_stream_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_data_stream_hdr_t),
              ("datatype", c_uint8),
              ("bcm_data", bcm_data_set_t * 4),
              ]

class m2m2_bcm_app_sweep_freq_resp_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ]

class bcm_app_hs_rtia_sel_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("hsritasel", c_uint16),
              ]

class m2m2_dcb_fds_status_info_req_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ]

class m2m2_dcb_fds_timing_info_req_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ]

class m2m2_dcb_fds_timing_info_resp_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("adi_dcb_clear_entries_time", c_uint16),
              ("adi_dcb_check_entries_time", c_uint16),
              ("adi_dcb_delete_record_time", c_uint16),
              ("adi_dcb_read_entry_time", c_uint16),
              ("adi_dcb_update_entry_time", c_uint16),
              ]

class m2m2_dcb_fds_status_info_resp_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("dirty_records", c_uint16),
              ("open_records", c_uint16),
              ("valid_records", c_uint16),
              ("pages_available", c_uint16),
              ("num_blocks", c_uint16),
              ("blocks_free", c_uint16),
              ]