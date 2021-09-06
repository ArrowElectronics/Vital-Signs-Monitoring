#!/usr/bin/env python3

from ctypes import *

import common_sensor_interface

import common_application_interface

class M2M2_SENSOR_ADPD_COMMAND_ENUM_t(c_uint8):
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

class M2M2_SENSOR_ADPD_SLOTMODE_ENUM_t(c_uint8):
    M2M2_SENSOR_ADPD_SLOTMODE_DISABLED = 0x0
    M2M2_SENSOR_ADPD_SLOTMODE_4CH_16b = 0x1
    M2M2_SENSOR_ADPD_SLOTMODE_4CH_32b = 0x4
    M2M2_SENSOR_ADPD_SLOTMODE_SUM_16b = 0x11
    M2M2_SENSOR_ADPD_SLOTMODE_SUM_32b = 0x14

class M2M2_SENSOR_ADPD_NSAMPLES_ENUM_t(c_uint8):
    M2M2_SENSOR_ADPD_NSAMPLES_4CH_32 = 0x2
    M2M2_SENSOR_ADPD_NSAMPLES_4CH_16 = 0x4
    M2M2_SENSOR_ADPD_NSAMPLES_SUM_32 = 0x7
    M2M2_SENSOR_ADPD_NSAMPLES_SUM_16 = 0xB
    M2M2_SENSOR_ADPD_NSAMPLES_SUM_32_COMPRESS = 0x15

class M2M2_SENSOR_ADPD_RAW_DATA_TYPES_ENUM_t(c_uint8):
    M2M2_SENSOR_RAW_DATA_ADPD_A_4CH_16b = 0x0
    M2M2_SENSOR_RAW_DATA_ADPD_A_4CH_32b = 0x1
    M2M2_SENSOR_RAW_DATA_ADPD_A_SUM_16b = 0x2
    M2M2_SENSOR_RAW_DATA_ADPD_A_SUM_32b = 0x3
    M2M2_SENSOR_RAW_DATA_ADPD_B_4CH_16b = 0x8
    M2M2_SENSOR_RAW_DATA_ADPD_B_4CH_32b = 0x9
    M2M2_SENSOR_RAW_DATA_ADPD_B_SUM_16b = 0xA
    M2M2_SENSOR_RAW_DATA_ADPD_B_SUM_32b = 0xB

class M2M2_SENSOR_ADPD_DEVICE_ID_ENUM_t(c_uint8):
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
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("deviceid", M2M2_SENSOR_ADPD_DEVICE_ID_ENUM_t),
              ("retdata", c_uint16 * 5),
              ]

class m2m2_sensor_adpd_testcommand_resp_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("retdata", c_uint32 * 3),
              ]

class adpd_data_header_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_data_stream_hdr_t),
              ("data_type", M2M2_SENSOR_ADPD_RAW_DATA_TYPES_ENUM_t),
              ("timestamp", c_uint32),
              ]

class adpd4000_data_header_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_data_stream_hdr_t),
              ("data_format", c_uint16),
              ("channel_num", c_uint8),
              ("timestamp", c_uint32),
              ]

class m2m2_sensor_adpd4000_data_stream_t(Structure):
    fields = [
              (None, adpd4000_data_header_t),
              ("sample_num", c_uint8),
              ("adpddata", c_uint8 * 24),
              ]

class m2m2_sensor_adpd4000_impulse_stream_t(Structure):
    fields = [
              (None, adpd4000_data_header_t),
              ("sample_num", c_uint8),
              ("adpddata", c_uint8 * 256),
              ]

class adpd_4ch32_data_set_t(Structure):
    fields = [
              ("ch1", c_uint32),
              ("ch2", c_uint32),
              ("ch3", c_uint32),
              ("ch4", c_uint32),
              ]

class adpd_4ch16_data_set_t(Structure):
    fields = [
              ("ch1", c_uint16),
              ("ch2", c_uint16),
              ("ch3", c_uint16),
              ("ch4", c_uint16),
              ]

class adpd_sum32_data_no_compress_format_t(Structure):
    fields = [
              ("incTS", c_uint16),
              ("sum32data", c_uint32),
              ]

class adpd_4ch32_data_no_compress_format_t(Structure):
    fields = [
              ("incTS", c_uint16),
              ("ch32data", adpd_4ch32_data_set_t),
              ]

class adpd_sum16_data_no_compress_format_t(Structure):
    fields = [
              ("incTS", c_uint16),
              ("sum16data", c_uint16),
              ]

class adpd_4ch16_data_no_compress_format_t(Structure):
    fields = [
              ("incTS", c_uint16),
              ("ch16data", adpd_4ch16_data_set_t),
              ]

class m2m2_sensor_adpd_data_sum32_no_compress_stream_t(Structure):
    fields = [
              (None, adpd_data_header_t),
              ("first_adpddata", c_uint32),
              ("adpddata", adpd_sum32_data_no_compress_format_t * 6),
              ]

class m2m2_sensor_adpd_data_4ch32_no_compress_stream_t(Structure):
    fields = [
              (None, adpd_data_header_t),
              ("first_adpddata", adpd_4ch32_data_set_t),
              ("adpddata", adpd_4ch32_data_no_compress_format_t * 1),
              ]

class m2m2_sensor_adpd_data_sum16_no_compress_stream_t(Structure):
    fields = [
              (None, adpd_data_header_t),
              ("first_adpddata", c_uint16),
              ("adpddata", adpd_sum16_data_no_compress_format_t * 10),
              ]

class m2m2_sensor_adpd_data_4ch16_no_compress_stream_t(Structure):
    fields = [
              (None, adpd_data_header_t),
              ("first_adpddata", adpd_4ch16_data_set_t),
              ("adpddata", adpd_4ch16_data_no_compress_format_t * 3),
              ]

class m2m2_sensor_adpd_data_sum32_compress_stream_t(Structure):
    fields = [
              (None, adpd_data_header_t),
              ("first_adpddata", c_uint8 * 3),
              ("inc_sum", c_int16 * 21),
              ]

class m2m2_sensor_adpd_slot_resp_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("slotA", M2M2_SENSOR_ADPD_SLOTMODE_ENUM_t),
              ("slotB", M2M2_SENSOR_ADPD_SLOTMODE_ENUM_t),
              ]

class m2m2_sensor_adpd4000_slot_resp_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("slot_num", c_uint8),
              ("slot_enable", c_uint8),
              ("slot_format", c_uint16),
              ("channel_num", c_uint8),
              ]

class m2m2_sensor_adpd4000_slot_active_resp_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("slot_num", c_uint8),
              ("slot_active", c_uint8),
              ]

class m2m2_sensor_clockcal_resp_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("clockcalid", c_uint8),
              ]

class m2m2_sensor_com_mode_resp_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("com_mode", c_uint8),
              ]

class m2m2_sensor_adpd4000_resp_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("deviceid", M2M2_SENSOR_ADPD_DEVICE_ID_ENUM_t),
              ("retdata", c_uint16 * 5),
              ]

class m2m2_sensor_efuse_resp_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("efusedata", c_uint8 * 16),
              ]

class m2m2_sensor_fifo_status_bytes_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("timestamp", c_uint32),
              ("sequence_num", c_uint8),
              ("data_int", c_uint16),
              ("level0_int", c_uint16),
              ("level1_int", c_uint16),
              ("tia_ch1_int", c_uint16),
              ("tia_ch2_int", c_uint16),
              ]

class m2m2_sensor_adpd4000_set_fs_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("odr", c_uint16),
              ]

class m2m2_adpd4k_slot_info_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("slots", c_uint8),
              ]

class m2m2_adpd_dcfg_resp_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("slotid", c_uint16),
              ("appid", c_uint16),
              ]

class m2m2_adpd_dcfg_op_t(Structure):
    fields = [
              ("slotid", c_uint16),
              ("appid", c_uint16),
              ]

class m2m2_adpd_dcfg_op_hdr_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("num_ops", c_uint8),
              ("ops", m2m2_adpd_dcfg_op_t * 0),
              ]

class m2m2_adpd_agc_info_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("led_index", c_uint8),
              ("led_ch1", c_uint32 * 10),
              ("led_ch2", c_uint32 * 10),
              ("DC0_LEDcurrent", c_uint16),
              ("TIA_ch1_i", c_uint16),
              ("TIA_ch2_i", c_uint16),
              ]

class m2m2_adpd_agc_cntrl_data_t(Structure):
    fields = [
              ("agc_cntrl", c_uint8),
              ("agc_type", c_uint8),
              ]

class m2m2_adpd_agc_cntrl_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("num_ops", c_uint8),
              ("ops", m2m2_adpd_agc_cntrl_data_t * 0),
              ]

#Not using _m2m2_app_data_stream_hdr_t because data type for sequence_num is different
class adpd_ext_data_stream_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("sequence_num", c_uint32),
              ("data", c_uint32),
              ("timestamp", c_uint32),
              ]

class adpd_ext_data_stream_odr_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("sampling_freq", c_uint16),
              ]

class m2m2_adpd_set_uc_hr_enab_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("control", c_uint8),
              ("slotNum", c_uint16),
                ]
