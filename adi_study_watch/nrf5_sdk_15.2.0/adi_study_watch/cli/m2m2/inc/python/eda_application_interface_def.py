from ctypes import *

from common_application_interface_def import *

from common_sensor_interface_def import *

from m2m2_core_def import *


class M2M2_EDA_APP_CMD_ENUM_t(c_ubyte):
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

class M2M2_SENSOR_EDA_NSAMPLES_ENUM_t(c_ubyte):
    M2M2_SENSOR_EDA_NSAMPLES = 0x6

class M2M2_SENSOR_EDA_RAW_DATA_TYPES_ENUM_t(c_ubyte):
    M2M2_SENSOR_EDA_DATA = 0x0

class eda_app_lib_state_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("states", c_ubyte * 10),
              ]

class eda_app_lcfg_op_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("field", c_ubyte),
              ("value", c_ulong),
              ]

def eda_app_lcfg_op_hdr_t(array_size):
  class eda_app_lcfg_op_hdr_t_internal(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("num_ops", c_ubyte),
              ("ops", eda_app_lcfg_op_t * array_size),
              ]
  return eda_app_lcfg_op_hdr_t_internal()

class eda_data_set_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("timestamp", c_ulong),
              ("realdata", c_short),
              ("imgdata", c_short),
              ]

class eda_app_stream_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("sequence_num", c_ushort),
              ("datatype", c_ubyte),
              ("eda_data", eda_data_set_t * 6),
              ]

class eda_app_dynamic_scale_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("dscale", c_ubyte),
              ("minscale", c_ushort),
              ("maxscale", c_ushort),
              ("lprtia", c_ushort),
              ]

class eda_app_set_data_rate_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("datarate", c_ushort),
              ]

class eda_app_set_dft_num_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("dftnum", c_ubyte),
              ]

class eda_app_set_baseline_imp_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("imp_real_dft16", c_float),
              ("imp_img_dft16", c_float),
              ("imp_real_dft8", c_float),
              ("imp_img_dft8", c_float),
              ("resistor_baseline", c_ulong),
              ]

class eda_app_dcb_lcfg_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ]

class m2m2_get_eda_debug_info_resp_cmd_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("ad5940_fifo_level", c_ulong),
              ("Interrupts_time_gap", c_ulong),
              ("rtia_calibration_time", c_ulong),
              ("delay_in_first_measurements", c_ulong),
              ("packets_time_gap", c_ulong),
              ("first_voltage_measure_time", c_ulong),
              ("first_current_measure_time", c_ulong),
              ("voltage_measure_time_gap", c_ulong),
              ("EDA_Init_Time", c_ulong),
              ("current_measure_time_gap", c_ulong),
              ("EDA_DeInit_Time", c_ulong),
              ("ad5940_fifo_overflow_status", c_ubyte),
              ]

class eda_app_rtia_cal_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("calibrated_res", c_ulong),
              ("actual_res", c_ulong),
              ]

def eda_app_perform_rtia_cal_t(array_size):
  class eda_app_perform_rtia_cal_t_internal(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("minscale", c_ushort),
              ("maxscale", c_ushort),
              ("lowpowerrtia", c_ushort),
              ("num_calibrated_values", c_ushort),
              ("rtia_cal_table_val", eda_app_rtia_cal_t * array_size),
              ]
  return eda_app_perform_rtia_cal_t_internal()

class m2m2_get_eda_debug_info_req_cmd_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ]

