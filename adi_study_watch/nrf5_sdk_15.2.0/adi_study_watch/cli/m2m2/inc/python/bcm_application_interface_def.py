from ctypes import *

from common_application_interface_def import *

from common_sensor_interface_def import *


class M2M2_BCM_APP_CMD_ENUM_t(c_ubyte):
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

class M2M2_SENSOR_BCM_NSAMPLES_ENUM_t(c_ubyte):
    M2M2_SENSOR_BCM_NSAMPLES = 0x4

class M2M2_SENSOR_BCM_RAW_DATA_TYPES_ENUM_t(c_ubyte):
    M2M2_SENSOR_BCM_DATA = 0x0

class M2M2_SENSOR_BCM_SWEEP_FREQ_INDEX_ENUM_t(c_ubyte):
    M2M2_SENSOR_BCM_FREQ_1000HZ = 0x0
    M2M2_SENSOR_BCM_FREQ_3760HZ = 0x1
    M2M2_SENSOR_BCM_FREQ_14140HZ = 0x2
    M2M2_SENSOR_BCM_FREQ_53180HZ = 0x3
    M2M2_SENSOR_BCM_FREQ_200KHZ = 0x4
    M2M2_SENSOR_BCM_FREQ_50KHZ = 0xFF

class bcm_app_set_dft_num_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("dftnum", c_ushort),
              ]

class bcm_app_lib_state_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("states", c_ubyte * 10),
              ]

class bcm_app_lcfg_op_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("field", c_ubyte),
              ("value", c_ulong),
              ]

class bcm_app_dcb_lcfg_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ]

def bcm_app_lcfg_op_hdr_t(array_size):
  class bcm_app_lcfg_op_hdr_t_internal(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("num_ops", c_ubyte),
              ("ops", bcm_app_lcfg_op_t * array_size),
              ]
  return bcm_app_lcfg_op_hdr_t_internal()

class bcm_data_set_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("timestamp", c_ulong),
              ("real", c_long),
              ("img", c_long),
              ("freq_index", c_ubyte),
              ]

class bcm_app_stream_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("sequence_num", c_ushort),
              ("datatype", c_ubyte),
              ("bcm_data", bcm_data_set_t * 4),
              ]

class m2m2_bcm_app_sweep_freq_resp_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ]

class bcm_app_hs_rtia_sel_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("hsritasel", c_ushort),
              ]

class m2m2_dcb_fds_status_info_req_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ]

class m2m2_dcb_fds_timing_info_req_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ]

class m2m2_dcb_fds_timing_info_resp_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("adi_dcb_clear_entries_time", c_ushort),
              ("adi_dcb_check_entries_time", c_ushort),
              ("adi_dcb_delete_record_time", c_ushort),
              ("adi_dcb_read_entry_time", c_ushort),
              ("adi_dcb_update_entry_time", c_ushort),
              ]

class m2m2_dcb_fds_status_info_resp_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("dirty_records", c_ushort),
              ("open_records", c_ushort),
              ("valid_records", c_ushort),
              ("pages_available", c_ushort),
              ("num_blocks", c_ushort),
              ("blocks_free", c_ushort),
              ]

