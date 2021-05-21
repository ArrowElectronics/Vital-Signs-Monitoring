#!/usr/bin/env python3

from ctypes import *

import common_sensor_interface

import common_application_interface

M2M2_SENSOR_ADPD_COMMAND_ENUM_t = {
  "type":c_uint8,
  "enum_values": [
    ("_M2M2_SENSOR_ADPD_COMMAND_LOWEST",                0x40),
    ("M2M2_SENSOR_ADPD_COMMAND_LOAD_CFG_REQ",           0x42),
    ("M2M2_SENSOR_ADPD_COMMAND_LOAD_CFG_RESP",          0x43),
    ("M2M2_SENSOR_ADPD_COMMAND_CLOCK_CAL_REQ",          0x44),
    ("M2M2_SENSOR_ADPD_COMMAND_CLOCK_CAL_RESP",         0x45),
    ("M2M2_SENSOR_ADPD_COMMAND_SET_SLOT_REQ",           0x46),
    ("M2M2_SENSOR_ADPD_COMMAND_SET_SLOT_RESP",          0x47),
    ("M2M2_SENSOR_ADPD_COMMAND_GET_SLOT_REQ",           0x48),
    ("M2M2_SENSOR_ADPD_COMMAND_GET_SLOT_RESP",          0x49),
    ("M2M2_SENSOR_ADPD_COMMAND_SET_DARK_OFFSET_REQ",    0x4A),
    ("M2M2_SENSOR_ADPD_COMMAND_SET_DARK_OFFSET_RESP",   0x4B),
    ("M2M2_SENSOR_ADPD_COMMAND_GET_CTR_REQ",            0x4C),
    ("M2M2_SENSOR_ADPD_COMMAND_GET_CTR_RESP",           0x4D),
    ("M2M2_SENSOR_ADPD_COMMAND_FLOATMODE_CFG_REQ",      0x4E),
    ("M2M2_SENSOR_ADPD_COMMAND_FLOATMODE_CFG_RESP",     0x4F),
    ("M2M2_SENSOR_ADPD_COMMAND_DO_TEST1_REQ",           0x60),
    ("M2M2_SENSOR_ADPD_COMMAND_DO_TEST1_RESP",          0x61),
    ("M2M2_SENSOR_ADPD_COMMAND_DO_TEST2_REQ",           0x62),
    ("M2M2_SENSOR_ADPD_COMMAND_DO_TEST2_RESP",          0x63),
    ("M2M2_SENSOR_ADPD_COMMAND_DO_TEST3_REQ",           0x64),
    ("M2M2_SENSOR_ADPD_COMMAND_DO_TEST3_RESP",          0x65),
    ("M2M2_SENSOR_ADPD_COMMAND_SET_PAUSE_REQ",          0x66),
    ("M2M2_SENSOR_ADPD_COMMAND_SET_PAUSE_RESP",         0x67),
    ("M2M2_SENSOR_ADPD_COMMUNICATION_MODE_REQ",         0x68),
    ("M2M2_SENSOR_ADPD_COMMUNICATION_MODE_RESP",        0x69),
    ("M2M2_SENSOR_ADPD_COMMAND_SET_SLOT_ACTIVE_REQ",    0x6A),
    ("M2M2_SENSOR_ADPD_COMMAND_SET_SLOT_ACTIVE_RESP",   0x6B),
    ("M2M2_SENSOR_ADPD_COMMAND_GET_SLOT_ACTIVE_REQ",    0x6C),
    ("M2M2_SENSOR_ADPD_COMMAND_GET_SLOT_ACTIVE_RESP",   0x6D),
    ("M2M2_SENSOR_ADPD_COMMAND_GET_EFUSE_REQ",          0x6E),
    ("M2M2_SENSOR_ADPD_COMMAND_GET_EFUSE_RESP",         0x6F),
    ("M2M2_SENSOR_ADPD_COMMAND_CREATE_DCFG_REQ",        0x70),
    ("M2M2_SENSOR_ADPD_COMMAND_CREATE_DCFG_RESP",       0x71),
    ("M2M2_SENSOR_ADPD_COMMAND_SET_FS_REQ",             0x72),
    ("M2M2_SENSOR_ADPD_COMMAND_SET_FS_RESP",            0x73),
    ("M2M2_SENSOR_ADPD_COMMAND_DISABLE_SLOTS_REQ",      0x74),
    ("M2M2_SENSOR_ADPD_COMMAND_DISABLE_SLOTS_RESP",     0x75),
    ("M2M2_SENSOR_ADPD_COMMAND_AGC_ON_OFF_REQ",         0x76),
    ("M2M2_SENSOR_ADPD_COMMAND_AGC_ON_OFF_RESP",        0x77),
    ("M2M2_SENSOR_ADPD_COMMAND_AGC_RECALIBRATE_REQ",    0x78),
    ("M2M2_SENSOR_ADPD_COMMAND_AGC_RECALIBRATE_RESP",   0x79),
    ("M2M2_SENSOR_ADPD_COMMAND_AGC_INFO_REQ",           0x7A),
    ("M2M2_SENSOR_ADPD_COMMAND_AGC_INFO_RESP",          0x7B),
    ("M2M2_SENSOR_ADPD_COMMAND_AGC_STATUS_REQ",         0x7C),
    ("M2M2_SENSOR_ADPD_COMMAND_AGC_STATUS_RESP",        0x7D),
    ("M2M2_SENSOR_ADPD_COMMAND_SET_EXT_DATA_STREAM_ODR_REQ",  0x7E),
    ("M2M2_SENSOR_ADPD_COMMAND_SET_EXT_DATA_STREAM_ODR_RESP", 0x7F),
    ("M2M2_SENSOR_ADPD_COMMAND_EXT_ADPD_DATA_STREAM",   0x80),
    ("M2M2_SENSOR_ADPD_COMMAND_UC_HR_ENAB_REQ"          0x82),
    ("M2M2_SENSOR_ADPD_COMMAND_UC_HR_ENAB_RESP"         0x83),
    ]
}

M2M2_SENSOR_ADPD_SLOTMODE_ENUM_t = {
  "type":c_uint8,
  "enum_values": [
    ("M2M2_SENSOR_ADPD_SLOTMODE_DISABLED",  0x00),
    ("M2M2_SENSOR_ADPD_SLOTMODE_4CH_16b",   0x01),
    ("M2M2_SENSOR_ADPD_SLOTMODE_4CH_32b",   0x04),
    ("M2M2_SENSOR_ADPD_SLOTMODE_SUM_16b",   0x11),
    ("M2M2_SENSOR_ADPD_SLOTMODE_SUM_32b",   0x14),
    ]
}

M2M2_SENSOR_ADPD_NSAMPLES_ENUM_t = {
  "type":c_uint8,
  "enum_values": [
    ("M2M2_SENSOR_ADPD_NSAMPLES_4CH_32",            0x02),
    ("M2M2_SENSOR_ADPD_NSAMPLES_4CH_16",            0x04),
    ("M2M2_SENSOR_ADPD_NSAMPLES_SUM_32",            0x07),
    ("M2M2_SENSOR_ADPD_NSAMPLES_SUM_16",            0x0B),
    ("M2M2_SENSOR_ADPD_NSAMPLES_SUM_32_COMPRESS",   0x15),
    ]
}
M2M2_SENSOR_ADPD_RAW_DATA_TYPES_ENUM_t = {
  "type":c_uint8,
  "enum_values": [
    ("M2M2_SENSOR_RAW_DATA_ADPD_A_4CH_16b", 0x00),
    ("M2M2_SENSOR_RAW_DATA_ADPD_A_4CH_32b", 0x01),
    ("M2M2_SENSOR_RAW_DATA_ADPD_A_SUM_16b", 0x02),
    ("M2M2_SENSOR_RAW_DATA_ADPD_A_SUM_32b", 0x03),
    ("M2M2_SENSOR_RAW_DATA_ADPD_B_4CH_16b", 0x08),
    ("M2M2_SENSOR_RAW_DATA_ADPD_B_4CH_32b", 0x09),
    ("M2M2_SENSOR_RAW_DATA_ADPD_B_SUM_16b", 0x0A),
    ("M2M2_SENSOR_RAW_DATA_ADPD_B_SUM_32b", 0x0B),
    ]
}
M2M2_SENSOR_ADPD_DEVICE_ID_ENUM_t = {
  "type":c_uint8,
  "enum_values": [
    ("M2M2_SENSOR_ADPD_DEVICE_107", 0x02),
    ("M2M2_SENSOR_ADPD_DEVICE_185", 0x03),
    ("M2M2_SENSOR_ADPD_DEVICE_ECG_PPG_185", 0x04),
    ("M2M2_SENSOR_ADPD_DEVICE_ECG_185", 0x05),
    ("M2M2_SENSOR_ADPD_DEVICE_105", 0x06),
    ("M2M2_SENSOR_ADPD_DEVICE_188", 0x07),
    ("M2M2_SENSOR_ADPD_DEVICE_108", 0x08),
    ("M2M2_SENSOR_ADPD_DEVICE_188F", 0x09),
    ("M2M2_SENSOR_ADPD_DEVICE_ECG_PPG_188", 0x0A),
    ("M2M2_SENSOR_ADPD_DEVICE_ECG_188", 0x0B),
    ("M2M2_SENSOR_ADPD4000_DEVICE_4000_G", 0x28),
    ("M2M2_SENSOR_ADPD4000_DEVICE_4000_R", 0x29),
    ("M2M2_SENSOR_ADPD4000_DEVICE_4000_IR",  0x2A),
    ("M2M2_SENSOR_ADPD4000_DEVICE_4000_B",  0x2B),
    ("M2M2_SENSOR_ADPD4000_DEVICE_4000_G_R_IR_B",  0x2C),
    ]
}

m2m2_sensor_adpd_resp_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"deviceid",
    "type":M2M2_SENSOR_ADPD_DEVICE_ID_ENUM_t},
    {"name":"retdata",
    "length":5,
    "type":c_uint16},
  ]
}

m2m2_sensor_adpd_testcommand_resp_t = {
  "struct_fields": [
    {"name":None,
     "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"retdata",
     "length":3,
     "type":c_uint32},
  ]
}

adpd_data_header_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_data_stream_hdr_t},
    {"name":"data_type",
    "type":M2M2_SENSOR_ADPD_RAW_DATA_TYPES_ENUM_t},
    {"name":"timestamp",
    "type":c_uint32},
  ]
}

adpdCl_data_header_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_data_stream_hdr_t},
    {"name":"data_format",
    "type":c_uint16},
    {"name":"channel_num",
    "type":c_uint8},
    {"name":"timestamp",
    "type":c_uint32},
  ]
}

m2m2_sensor_adpdCl_data_stream_t = {
  "struct_fields": [
    {"name":None,
    "type":adpdCl_data_header_t},
    {"name":"sample_num",
    "type":c_uint8},
    {"name":"adpddata",
    "length":24,
    "type":c_uint8},
  ]
}

m2m2_sensor_adpdCl_impulse_stream_t = {
  "struct_fields": [
    {"name":None,
    "type":adpdCl_data_header_t},
    {"name":"sample_num",
    "type":c_uint8},
    {"name":"adpddata",
    "length":256,
    "type":c_uint8},
  ]
}

adpd_4ch32_data_set_t = {
  "struct_fields": [
    {"name":"ch1",
    "type":c_uint32},
    {"name":"ch2",
    "type":c_uint32},
    {"name":"ch3",
    "type":c_uint32},
    {"name":"ch4",
    "type":c_uint32},
  ]
}

adpd_4ch16_data_set_t = {
  "struct_fields": [
    {"name":"ch1",
    "type":c_uint16},
    {"name":"ch2",
    "type":c_uint16},
    {"name":"ch3",
    "type":c_uint16},
    {"name":"ch4",
    "type":c_uint16},
  ]
}

adpd_sum32_data_no_compress_format_t = {
  "struct_fields": [
    {"name":"incTS",
    "type":c_uint16},
    {"name":"sum32data",
    "type":c_uint32},
  ]
}

adpd_4ch32_data_no_compress_format_t = {
  "struct_fields": [
    {"name":"incTS",
    "type":c_uint16},
    {"name":"ch32data",
    "type":adpd_4ch32_data_set_t},
  ]
}

adpd_sum16_data_no_compress_format_t = {
  "struct_fields": [
    {"name":"incTS",
    "type":c_uint16},
    {"name":"sum16data",
    "type":c_uint16},
  ]
}

adpd_4ch16_data_no_compress_format_t = {
  "struct_fields": [
    {"name":"incTS",
    "type":c_uint16},
    {"name":"ch16data",
    "type":adpd_4ch16_data_set_t},
  ]
}

m2m2_sensor_adpd_data_sum32_no_compress_stream_t = {
  "struct_fields": [
    {"name":None,
    "type":adpd_data_header_t},
    {"name":"first_adpddata",
    "type":c_uint32},
    {"name":"adpddata",
    "length":6,
    "type":adpd_sum32_data_no_compress_format_t},
  ]
}

m2m2_sensor_adpd_data_4ch32_no_compress_stream_t = {
  "struct_fields": [
    {"name":None,
    "type":adpd_data_header_t},
    {"name":"first_adpddata",
    "type":adpd_4ch32_data_set_t},
    {"name":"adpddata",
    "length":1,
    "type":adpd_4ch32_data_no_compress_format_t},
  ]
}

m2m2_sensor_adpd_data_sum16_no_compress_stream_t = {
  "struct_fields": [
    {"name":None,
    "type":adpd_data_header_t},
    {"name":"first_adpddata",
    "type":c_uint16},
    {"name":"adpddata",
    "length":10,
    "type":adpd_sum16_data_no_compress_format_t},
  ]
}

m2m2_sensor_adpd_data_4ch16_no_compress_stream_t = {
  "struct_fields": [
    {"name":None,
    "type":adpd_data_header_t},
    {"name":"first_adpddata",
    "type":adpd_4ch16_data_set_t},
    {"name":"adpddata",
    "length":3,
    "type":adpd_4ch16_data_no_compress_format_t},
  ]
}

m2m2_sensor_adpd_data_sum32_compress_stream_t = {
  "struct_fields": [
    {"name":None,
    "type":adpd_data_header_t},
    {"name":"first_adpddata",
    "length":3,
    "type":c_uint8},
    {"name":"inc_sum",
    "length":21,
    "type":c_int16},
  ]
}

m2m2_sensor_adpd_slot_resp_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"slotA",
    "type":M2M2_SENSOR_ADPD_SLOTMODE_ENUM_t},
    {"name":"slotB",
    "type":M2M2_SENSOR_ADPD_SLOTMODE_ENUM_t},
  ]
}

m2m2_sensor_adpdCl_slot_resp_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"slot_num",
    "type":c_uint8},
    {"name":"slot_enable",
    "type":c_uint8},
    {"name":"slot_format",
    "type":c_uint16},
    {"name":"channel_num",
    "type":c_uint8},
  ]
}

m2m2_sensor_adpdCl_slot_active_resp_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"slot_num",
    "type":c_uint8},
    {"name":"slot_active",
    "type":c_uint8},
  ]
}

m2m2_sensor_clockcal_resp_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"clockcalid",
    "type":c_uint8},
  ]
}

m2m2_sensor_com_mode_resp_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"com_mode",
    "type":c_uint8},
  ]
}

m2m2_sensor_adpdCl_resp_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"deviceid",
    "type":M2M2_SENSOR_ADPD_DEVICE_ID_ENUM_t},
    {"name":"retdata",
    "length":5,
    "type":c_uint16},
  ]
}
m2m2_sensor_efuse_resp_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"efusedata",
    "length":16,
    "type":c_uint8},
  ]
}

m2m2_sensor_fifo_status_bytes_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"timestamp",
     "type":c_ulong},
    {"name":"sequence_num",
     "type":c_ubyte},
    {"name":"data_int",
     "type":c_ushort},
    {"name":"level0_int",
     "type":c_ushort},
    {"name":"level1_int",
     "type":c_ushort},
    {"name":"tia_ch1_int",
     "type":c_ushort},
    {"name":"tia_ch2_int",
     "type":c_ushort}
  ]
}

m2m2_sensor_adpd4000_set_fs_t = {
    "struct_fields": [
        {"name": "command",
          "type": c_ubyte},
        {"name": "status",
          "type": c_ubyte},
        {"name": "odr",
          "type": c_ushort}
    ]
}

m2m2_adpd4k_slot_info_t = {
    "struct_fields": [
        {"name": "command",
          "type": c_ubyte},
        {"name": "status",
          "type": c_ubyte},
        {"name": "slots",
          "type": c_ushort}
    ]
}

m2m2_adpd_dcfg_resp_t = {
    "struct_fields": [
        {"name": "command",
          "type": c_ubyte},
        {"name": "status",
          "type": c_ubyte},
        {"name": "slotid",
          "type": c_ushort},
        {"name": "appid",
          "type": c_ushort}
    ]
}

m2m2_adpd_dcfg_op_t = {
    "struct_fields": [
        {"name": "slotid",
          "type": c_ushort},
        {"name": "appid",
          "type": c_ushort}
    ]
}

m2m2_adpd_dcfg_op_hdr_t = {
    "struct_fields": [
        {"name": "command",
          "type": c_ubyte},
        {"name": "status",
          "type": c_ubyte},
        {"name": "num_ops",
          "type": c_ubyte},
        {"name": "ops",
          "length":0,
          "type": m2m2_adpd_dcfg_op_t},
    ]
}

m2m2_adpd_agc_recalibrate_t = {
    "struct_fields": [
        {"name": "command",
          "type": c_ubyte},
        {"name": "status",
          "type": c_ubyte}
    ]
}

m2m2_adpd_agc_info_t = {
    "struct_fields": [
        {"name": "command",
          "type": c_ubyte},
        {"name": "status",
          "type": c_ubyte},
        {"name": "led_index",
          "type": c_ubyte},
        {"name":"green_ch1",
          "length":10,
          "type":c_ulong},
        {"name":"green_ch2",
          "length":10,
          "type":c_ulong},
        {"name": "DC0_LEDcurrent",
          "type": c_ushort},
        {"name": "TIA_ch1_i",
          "type": c_ushort},
        {"name": "TIA_ch2_i",
          "type": c_ushort}
    ]
}

m2m2_adpd_agc_cntrl_data_t = {
    "struct_fields": [
        {"name": "agc_cntrl",
          "type": c_ubyte},
        {"name": "agc_type",
          "type": c_ubyte}
    ]
}

m2m2_adpd_agc_cntrl_t = {
    "struct_fields": [
        {"name": "command",
          "type": c_ubyte},
        {"name": "status",
          "type": c_ubyte},
        {"name": "num_ops",
          "type": c_ubyte},
        {"name":"ops",
          "length":0,
          "type":m2m2_adpd_agc_cntrl_data_t},
    ]
}

m2m2_adpd_agc_status_t = {
    "struct_fields": [
        {"name": "command",
          "type": c_ubyte},
        {"name": "status",
          "type": c_ubyte},
        {"name": "agc_type",
          "type": c_ubyte},
        {"name": "agc_status",
          "type": c_ubyte}
    ]
}

adpd_ext_data_stream_t = {
    "struct_fields": [
        {"name": "command",
          "type": c_ubyte},
        {"name": "status",
          "type": c_ubyte},
        {"name": "sequence_num",
          "type": c_ulong},
        {"name": "data",
          "type": c_ulong},
        {"name": "timestamp",
          "type": c_ulong},


adpd_ext_data_stream_odr_t = {
    "struct_fields": [
        {"name": "command",
          "type": c_ubyte},
        {"name": "status",
          "type": c_ushort},
        {"name": "sampling_freq",
    ]
}

m2m2_adpd_set_uc_hr_enab_t = {
    "struct_fields": [
        {"name": "command",
          "type":  c_ubyte},
        {"name": "status",
          "type":  c_ubyte},
        {"name": "control",
          "type":  c_ubyte},
        {"name": "slotNum",
          "type":  c_ushort},
                ]
}