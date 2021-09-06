from ctypes import *

from common_application_interface_def import *

from common_sensor_interface_def import *


class M2M2_SENSOR_ADPD_COMMAND_ENUM_t(c_ubyte):
    _M2M2_SENSOR_ADPD_COMMAND_LOWEST = 0x40
    M2M2_SENSOR_ADPD_COMMAND_LOAD_CFG_REQ = 0x42
    M2M2_SENSOR_ADPD_COMMAND_LOAD_CFG_RESP = 0x43
    M2M2_SENSOR_ADPD_COMMAND_CLOCK_CAL_REQ = 0x44
    M2M2_SENSOR_ADPD_COMMAND_CLOCK_CAL_RESP = 0x45
    M2M2_SENSOR_ADPD_COMMAND_SET_SLOT_REQ = 0x46
    M2M2_SENSOR_ADPD_COMMAND_SET_SLOT_RESP = 0x47
    M2M2_SENSOR_ADPD_COMMAND_GET_SLOT_REQ = 0x48
    M2M2_SENSOR_ADPD_COMMAND_GET_SLOT_RESP = 0x49
    M2M2_SENSOR_ADPD_COMMAND_SET_DARK_OFFSET_REQ = 0x4A
    M2M2_SENSOR_ADPD_COMMAND_SET_DARK_OFFSET_RESP = 0x4B
    M2M2_SENSOR_ADPD_COMMAND_GET_CTR_REQ = 0x4C
    M2M2_SENSOR_ADPD_COMMAND_GET_CTR_RESP = 0x4D
    M2M2_SENSOR_ADPD_COMMAND_FLOATMODE_CFG_REQ = 0x4E
    M2M2_SENSOR_ADPD_COMMAND_FLOATMODE_CFG_RESP = 0x4F
    M2M2_SENSOR_ADPD_COMMAND_DO_TEST1_REQ = 0x60
    M2M2_SENSOR_ADPD_COMMAND_DO_TEST1_RESP = 0x61
    M2M2_SENSOR_ADPD_COMMAND_DO_TEST2_REQ = 0x62
    M2M2_SENSOR_ADPD_COMMAND_DO_TEST2_RESP = 0x63
    M2M2_SENSOR_ADPD_COMMAND_DO_TEST3_REQ = 0x64
    M2M2_SENSOR_ADPD_COMMAND_DO_TEST3_RESP = 0x65
    M2M2_SENSOR_ADPD_COMMAND_SET_PAUSE_REQ = 0x66
    M2M2_SENSOR_ADPD_COMMAND_SET_PAUSE_RESP = 0x67
    M2M2_SENSOR_ADPD_COMMUNICATION_MODE_REQ = 0x68
    M2M2_SENSOR_ADPD_COMMUNICATION_MODE_RESP = 0x69
    M2M2_SENSOR_ADPD_COMMAND_SET_SLOT_ACTIVE_REQ = 0x6A
    M2M2_SENSOR_ADPD_COMMAND_SET_SLOT_ACTIVE_RESP = 0x6B
    M2M2_SENSOR_ADPD_COMMAND_GET_SLOT_ACTIVE_REQ = 0x6C
    M2M2_SENSOR_ADPD_COMMAND_GET_SLOT_ACTIVE_RESP = 0x6D
    M2M2_SENSOR_ADPD_COMMAND_GET_EFUSE_REQ = 0x6E
    M2M2_SENSOR_ADPD_COMMAND_GET_EFUSE_RESP = 0x6F
    M2M2_SENSOR_ADPD_COMMAND_CREATE_DCFG_REQ = 0x70
    M2M2_SENSOR_ADPD_COMMAND_CREATE_DCFG_RESP = 0x71
    M2M2_SENSOR_ADPD_COMMAND_SET_FS_REQ = 0x72
    M2M2_SENSOR_ADPD_COMMAND_SET_FS_RESP = 0x73
    M2M2_SENSOR_ADPD_COMMAND_DISABLE_SLOTS_REQ = 0x74
    M2M2_SENSOR_ADPD_COMMAND_DISABLE_SLOTS_RESP = 0x75
    M2M2_SENSOR_ADPD_COMMAND_AGC_ON_OFF_REQ = 0x76
    M2M2_SENSOR_ADPD_COMMAND_AGC_ON_OFF_RESP = 0x77
    M2M2_SENSOR_ADPD_COMMAND_AGC_RECALIBRATE_REQ = 0x78
    M2M2_SENSOR_ADPD_COMMAND_AGC_RECALIBRATE_RESP = 0x79
    M2M2_SENSOR_ADPD_COMMAND_AGC_INFO_REQ = 0x7A
    M2M2_SENSOR_ADPD_COMMAND_AGC_INFO_RESP = 0x7B
    M2M2_SENSOR_ADPD_COMMAND_AGC_STATUS_REQ = 0x7C
    M2M2_SENSOR_ADPD_COMMAND_AGC_STATUS_RESP = 0x7D
    M2M2_SENSOR_ADPD_COMMAND_SET_EXT_DATA_STREAM_ODR_REQ = 0x7E
    M2M2_SENSOR_ADPD_COMMAND_SET_EXT_DATA_STREAM_ODR_RESP = 0x7F
    M2M2_SENSOR_ADPD_COMMAND_EXT_ADPD_DATA_STREAM = 0x80
    M2M2_SENSOR_ADPD_COMMAND_UC_HR_ENAB_REQ = 0x82
    M2M2_SENSOR_ADPD_COMMAND_UC_HR_ENAB_RESP = 0x83

class M2M2_SENSOR_ADPD_SLOTMODE_ENUM_t(c_ubyte):
    M2M2_SENSOR_ADPD_SLOTMODE_DISABLED = 0x0
    M2M2_SENSOR_ADPD_SLOTMODE_4CH_16b = 0x1
    M2M2_SENSOR_ADPD_SLOTMODE_4CH_32b = 0x4
    M2M2_SENSOR_ADPD_SLOTMODE_SUM_16b = 0x11
    M2M2_SENSOR_ADPD_SLOTMODE_SUM_32b = 0x14

class M2M2_SENSOR_ADPD_NSAMPLES_ENUM_t(c_ubyte):
    M2M2_SENSOR_ADPD_NSAMPLES_4CH_32 = 0x2
    M2M2_SENSOR_ADPD_NSAMPLES_4CH_16 = 0x4
    M2M2_SENSOR_ADPD_NSAMPLES_SUM_32 = 0x7
    M2M2_SENSOR_ADPD_NSAMPLES_SUM_16 = 0xB
    M2M2_SENSOR_ADPD_NSAMPLES_SUM_32_COMPRESS = 0x15

class M2M2_SENSOR_ADPD_RAW_DATA_TYPES_ENUM_t(c_ubyte):
    M2M2_SENSOR_RAW_DATA_ADPD_A_4CH_16b = 0x0
    M2M2_SENSOR_RAW_DATA_ADPD_A_4CH_32b = 0x1
    M2M2_SENSOR_RAW_DATA_ADPD_A_SUM_16b = 0x2
    M2M2_SENSOR_RAW_DATA_ADPD_A_SUM_32b = 0x3
    M2M2_SENSOR_RAW_DATA_ADPD_B_4CH_16b = 0x8
    M2M2_SENSOR_RAW_DATA_ADPD_B_4CH_32b = 0x9
    M2M2_SENSOR_RAW_DATA_ADPD_B_SUM_16b = 0xA
    M2M2_SENSOR_RAW_DATA_ADPD_B_SUM_32b = 0xB

class M2M2_SENSOR_ADPD_DEVICE_ID_ENUM_t(c_ubyte):
    M2M2_SENSOR_ADPD_DEVICE_107 = 0x2
    M2M2_SENSOR_ADPD_DEVICE_185 = 0x3
    M2M2_SENSOR_ADPD_DEVICE_ECG_PPG_185 = 0x4
    M2M2_SENSOR_ADPD_DEVICE_ECG_185 = 0x5
    M2M2_SENSOR_ADPD_DEVICE_105 = 0x6
    M2M2_SENSOR_ADPD_DEVICE_188 = 0x7
    M2M2_SENSOR_ADPD_DEVICE_108 = 0x8
    M2M2_SENSOR_ADPD_DEVICE_188F = 0x9
    M2M2_SENSOR_ADPD_DEVICE_ECG_PPG_188 = 0xA
    M2M2_SENSOR_ADPD_DEVICE_ECG_188 = 0xB
    M2M2_SENSOR_ADPD4000_DEVICE_4000_G = 0x28
    M2M2_SENSOR_ADPD4000_DEVICE_4000_R = 0x29
    M2M2_SENSOR_ADPD4000_DEVICE_4000_IR = 0x2A
    M2M2_SENSOR_ADPD4000_DEVICE_4000_B = 0x2B
    M2M2_SENSOR_ADPD4000_DEVICE_4000_G_R_IR_B = 0x2C
    M2M2_SENSOR_ADPD4000_DEVICE_4100_SWO2 = 0x2D

class m2m2_sensor_adpd_resp_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("deviceid", c_ubyte),
              ("retdata", c_ushort * 5),
              ]

class m2m2_sensor_adpd_testcommand_resp_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("retdata", c_ulong * 3),
              ]

class adpd_data_header_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("sequence_num", c_ushort),
              ("data_type", c_ubyte),
              ("timestamp", c_ulong),
              ]

class adpd4000_data_header_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("sequence_num", c_ushort),
              ("data_format", c_ushort),
              ("channel_num", c_ubyte),
              ("timestamp", c_ulong),
              ]

class m2m2_sensor_adpd4000_data_stream_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("sequence_num", c_ushort),
              ("data_format", c_ushort),
              ("channel_num", c_ubyte),
              ("timestamp", c_ulong),
              ("sample_num", c_ubyte),
              ("adpddata", c_ubyte * 24),
              ]

class m2m2_sensor_adpd4000_impulse_stream_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("sequence_num", c_ushort),
              ("data_format", c_ushort),
              ("channel_num", c_ubyte),
              ("timestamp", c_ulong),
              ("sample_num", c_ubyte),
              ("adpddata", c_ubyte * 256),
              ]

class adpd_4ch32_data_set_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("ch1", c_ulong),
              ("ch2", c_ulong),
              ("ch3", c_ulong),
              ("ch4", c_ulong),
              ]

class adpd_4ch16_data_set_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("ch1", c_ushort),
              ("ch2", c_ushort),
              ("ch3", c_ushort),
              ("ch4", c_ushort),
              ]

class adpd_sum32_data_no_compress_format_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("incTS", c_ushort),
              ("sum32data", c_ulong),
              ]

class adpd_4ch32_data_no_compress_format_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("incTS", c_ushort),
              ("ch32data", adpd_4ch32_data_set_t),
              ]

class adpd_sum16_data_no_compress_format_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("incTS", c_ushort),
              ("sum16data", c_ushort),
              ]

class adpd_4ch16_data_no_compress_format_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("incTS", c_ushort),
              ("ch16data", adpd_4ch16_data_set_t),
              ]

class m2m2_sensor_adpd_data_sum32_no_compress_stream_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("sequence_num", c_ushort),
              ("data_type", c_ubyte),
              ("timestamp", c_ulong),
              ("first_adpddata", c_ulong),
              ("adpddata", adpd_sum32_data_no_compress_format_t * 6),
              ]

class m2m2_sensor_adpd_data_4ch32_no_compress_stream_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("sequence_num", c_ushort),
              ("data_type", c_ubyte),
              ("timestamp", c_ulong),
              ("first_adpddata", adpd_4ch32_data_set_t),
              ("adpddata", adpd_4ch32_data_no_compress_format_t * 1),
              ]

class m2m2_sensor_adpd_data_sum16_no_compress_stream_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("sequence_num", c_ushort),
              ("data_type", c_ubyte),
              ("timestamp", c_ulong),
              ("first_adpddata", c_ushort),
              ("adpddata", adpd_sum16_data_no_compress_format_t * 10),
              ]

class m2m2_sensor_adpd_data_4ch16_no_compress_stream_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("sequence_num", c_ushort),
              ("data_type", c_ubyte),
              ("timestamp", c_ulong),
              ("first_adpddata", adpd_4ch16_data_set_t),
              ("adpddata", adpd_4ch16_data_no_compress_format_t * 3),
              ]

class m2m2_sensor_adpd_data_sum32_compress_stream_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("sequence_num", c_ushort),
              ("data_type", c_ubyte),
              ("timestamp", c_ulong),
              ("first_adpddata", c_ubyte * 3),
              ("inc_sum", c_short * 21),
              ]

class m2m2_sensor_adpd_slot_resp_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("slotA", c_ubyte),
              ("slotB", c_ubyte),
              ]

class m2m2_sensor_adpd4000_slot_resp_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("slot_num", c_ubyte),
              ("slot_enable", c_ubyte),
              ("slot_format", c_ushort),
              ("channel_num", c_ubyte),
              ]

class m2m2_sensor_adpd4000_slot_active_resp_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("slot_num", c_ubyte),
              ("slot_active", c_ubyte),
              ]

class m2m2_sensor_clockcal_resp_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("clockcalid", c_ubyte),
              ]

class m2m2_sensor_com_mode_resp_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("com_mode", c_ubyte),
              ]

class m2m2_sensor_adpd4000_resp_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("deviceid", c_ubyte),
              ("retdata", c_ushort * 5),
              ]

class m2m2_sensor_efuse_resp_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("efusedata", c_ubyte * 16),
              ]

class m2m2_sensor_fifo_status_bytes_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("timestamp", c_ulong),
              ("sequence_num", c_ubyte),
              ("data_int", c_ushort),
              ("level0_int", c_ushort),
              ("level1_int", c_ushort),
              ("tia_ch1_int", c_ushort),
              ("tia_ch2_int", c_ushort),
              ]

class m2m2_sensor_adpd4000_set_fs_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("odr", c_ushort),
              ]

class m2m2_adpd4k_slot_info_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("slots", c_ubyte),
              ]

class m2m2_adpd_dcfg_resp_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("slotid", c_ushort),
              ("appid", c_ushort),
              ]

class m2m2_adpd_dcfg_op_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("slotid", c_ushort),
              ("appid", c_ushort),
              ]

def m2m2_adpd_dcfg_op_hdr_t(array_size):
  class m2m2_adpd_dcfg_op_hdr_t_internal(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("num_ops", c_ubyte),
              ("ops", m2m2_adpd_dcfg_op_t * array_size),
              ]
  return m2m2_adpd_dcfg_op_hdr_t_internal()

class m2m2_adpd_agc_info_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("led_index", c_ubyte),
              ("led_ch1", c_ulong * 10),
              ("led_ch2", c_ulong * 10),
              ("DC0_LEDcurrent", c_ushort),
              ("TIA_ch1_i", c_ushort),
              ("TIA_ch2_i", c_ushort),
              ]

class m2m2_adpd_agc_cntrl_data_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("agc_cntrl", c_ubyte),
              ("agc_type", c_ubyte),
              ]

def m2m2_adpd_agc_cntrl_t(array_size):
  class m2m2_adpd_agc_cntrl_t_internal(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("num_ops", c_ubyte),
              ("ops", m2m2_adpd_agc_cntrl_data_t * array_size),
              ]
  return m2m2_adpd_agc_cntrl_t_internal()

class adpd_ext_data_stream_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("sequence_num", c_ulong),
              ("data", c_ulong),
              ("timestamp", c_ulong),
              ]

class adpd_ext_data_stream_odr_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("sampling_freq", c_ushort),
              ]

class m2m2_adpd_set_uc_hr_enab_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("control", c_ubyte),
              ("slotNum", c_ushort),
              ]

