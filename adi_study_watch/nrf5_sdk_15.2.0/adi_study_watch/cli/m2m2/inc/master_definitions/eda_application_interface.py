#!/usr/bin/env python3

from ctypes import *

import m2m2_core

import common_sensor_interface

import common_application_interface

class M2M2_EDA_APP_CMD_ENUM_t(c_uint8):
    _M2M2_EDA_APP_CMD_LOWEST = 0x40
    M2M2_EDA_APP_CMD_DYNAMIC_SCALE_REQ = 0x42
    M2M2_EDA_APP_CMD_DYNAMIC_SCALE_RESP = 0x43
    M2M2_EDA_APP_CMD_SET_DATA_RATE_REQ = 0x44
    M2M2_EDA_APP_CMD_SET_DATA_RATE_RESP = 0x45
    M2M2_EDA_APP_CMD_SET_DFT_NUM_REQ = 0x46
    M2M2_EDA_APP_CMD_SET_DFT_NUM_RESP = 0x47
    M2M2_EDA_APP_CMD_REQ_DEBUG_INFO_REQ = 0x48
    M2M2_EDA_APP_CMD_REQ_DEBUG_INFO_RESP = 0x49
    M2M2_EDA_APP_CMD_RTIA_CAL_REQ = 0x4A
    M2M2_EDA_APP_CMD_RTIA_CAL_RESP = 0x4B
    M2M2_EDA_APP_CMD_BASELINE_IMP_SET_REQ = 0x4C
    M2M2_EDA_APP_CMD_BASELINE_IMP_SET_RESP = 0x4D
    M2M2_EDA_APP_CMD_BASELINE_IMP_RESET_REQ = 0x4E
    M2M2_EDA_APP_CMD_BASELINE_IMP_RESET_RESP = 0x4F

class M2M2_SENSOR_EDA_NSAMPLES_ENUM_t(c_uint8):
    M2M2_SENSOR_EDA_NSAMPLES = 0x6

class M2M2_SENSOR_EDA_RAW_DATA_TYPES_ENUM_t(c_uint8):
    M2M2_SENSOR_EDA_DATA = 0x0

class eda_app_lib_state_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("states", c_uint8 * 10),
              ]

class eda_app_lcfg_op_t(Structure):
    fields = [
              ("field", c_uint8),
              ("value", c_uint32),
              ]

class eda_app_lcfg_op_hdr_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("num_ops", c_uint8),
              ("ops", eda_app_lcfg_op_t * 0),
              ]

class eda_data_set_t(Structure):
    fields = [
              ("timestamp", c_uint32),
              ("realdata", c_int16),
              ("imgdata", c_int16),
              ]

class eda_app_stream_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_data_stream_hdr_t),
              ("datatype", M2M2_SENSOR_EDA_RAW_DATA_TYPES_ENUM_t),
              ("eda_data", eda_data_set_t * 6),
              ]

class eda_app_dynamic_scale_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("dscale", c_uint8),
              ("minscale", c_uint16),
              ("maxscale", c_uint16),
              ("lprtia", c_uint16),
              ]

class eda_app_set_data_rate_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("datarate", c_uint16),
              ]

class eda_app_set_dft_num_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("dftnum", c_uint8),
              ]

class eda_app_set_baseline_imp_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("imp_real_dft16", c_float),
              ("imp_img_dft16", c_float),
              ("imp_real_dft8", c_float),
              ("imp_img_dft8", c_float),
              ("resistor_baseline", c_uint32),
              ]

class eda_app_dcb_lcfg_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ]

class m2m2_get_eda_debug_info_resp_cmd_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("ad5940_fifo_level", c_uint32),
              ("Interrupts_time_gap", c_uint32),
              ("rtia_calibration_time", c_uint32),
              ("delay_in_first_measurements", c_uint32),
              ("packets_time_gap", c_uint32),
              ("first_voltage_measure_time", c_uint32),
              ("first_current_measure_time", c_uint32),
              ("voltage_measure_time_gap", c_uint32),
              ("EDA_Init_Time", c_uint32),
              ("current_measure_time_gap", c_uint32),
              ("EDA_DeInit_Time", c_uint32),
              ("ad5940_fifo_overflow_status", c_uint8),
              ]

class eda_app_rtia_cal_t(Structure):
    fields = [
              ("calibrated_res", c_uint32),
              ("actual_res", c_uint32),
              ]

class eda_app_perform_rtia_cal_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("minscale", c_uint16),
              ("maxscale", c_uint16),
              ("lowpowerrtia", c_uint16),
              ("num_calibrated_values", c_uint16),
              ("rtia_cal_table_val", eda_app_rtia_cal_t * 0),
              ]

class m2m2_get_eda_debug_info_req_cmd_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ]