#!/usr/bin/env python3

from ctypes import *

import m2m2_core

import common_application_interface

MAXADPD4000DCBSIZE = (57) #//Max size of adpd4000 DCB size in double word length; 57 uint32_t elements in dcfg
MAXADXLDCBSIZE = (25)
MAXPPGDCBSIZE = (56)
MAXECGDCBSIZE = (4)
MAXEDADCBSIZE = (2)
MAXBCMDCBSIZE = (5)
MAXGENBLKDCBSIZE = (57)
MAXAD7156DCBSIZE = (20)
MAXLTAPPLCFGDCBSIZE = (5)

M2M2_DCB_COMMAND_ENUM_t = {
    "type":c_uint8,
    "enum_values":
    [
        ("__M2M2_DCB_COMMAND_LOWEST",                      0x96),
        ("M2M2_DCB_COMMAND_READ_CONFIG_REQ",               0x97),
        ("M2M2_DCB_COMMAND_READ_CONFIG_RESP",              0x98),
        ("M2M2_DCB_COMMAND_WRITE_CONFIG_REQ",              0x99),
        ("M2M2_DCB_COMMAND_WRITE_CONFIG_RESP",             0x9A),
        ("M2M2_DCB_COMMAND_ERASE_CONFIG_REQ",              0x9B),
        ("M2M2_DCB_COMMAND_ERASE_CONFIG_RESP",             0x9C),
        ("M2M2_DCB_COMMAND_QUERY_STATUS_REQ",              0x9D),
        ("M2M2_DCB_COMMAND_QUERY_STATUS_RESP",             0x9E),
    ]
}

M2M2_DCB_STATUS_ENUM_t = {
    "type":c_uint8,
    "enum_values":
    [
        ("__M2M2_DCB_STATUS_LOWEST",                       0x96),
        ("M2M2_DCB_STATUS_OK",                             0x97),
        ("M2M2_DCB_STATUS_ERR_ARGS",                       0x98),
        ("M2M2_DCB_STATUS_ERR_NOT_CHKD",                   0xFF),
    ]
}

m2m2_dcb_cmd_t = {
    "struct_fields":
    [
        {"name":"command",
        "type":c_uint8},
        {"name":"status",
        "type":c_uint8},
    ]
}

m2m2_dcb_adpd4000_data_t = {
    "struct_fields":
    [
        {"name":"command",
         "type":c_uint8},
        {"name":"status",
         "type":c_uint8},
        {"name":"size",
         "type":c_uint16},
        {"name":"num_of_pkts",
         "type":c_uint16},
        {"name":"dcbdata",
         "length":MAXADPD4000DCBSIZE,
         "type":c_uint32},

    ]
}

m2m2_dcb_adxl_data_t = {
    "struct_fields":
    [
        {"name":"command",
         "type":c_uint8},
        {"name":"status",
         "type":c_uint8},
        {"name":"size",
         "type":c_uint16},
        {"name":"dcbdata",
         "length":MAXADXLDCBSIZE,
         "type":c_uint32},

    ]
}

m2m2_dcb_ppg_data_t = {
    "struct_fields":
    [
        {"name":"command",
         "type":c_uint8},
        {"name":"status",
         "type":c_uint8},
        {"name":"size",
         "type":c_uint16},
        {"name":"dcbdata",
         "length":MAXPPGDCBSIZE,
         "type":c_int32},

    ]
}

m2m2_dcb_ecg_data_t = {
    "struct_fields":
    [
        {"name":"command",
         "type":c_uint8},
        {"name":"status",
         "type":c_uint8},
        {"name":"size",
         "type":c_uint16},
        {"name":"dcbdata",
         "length":MAXECGDCBSIZE,
         "type":c_uint32},

    ]
}

m2m2_dcb_eda_data_t = {
    "struct_fields":
    [
        {"name":"command",
         "type":c_uint8},
        {"name":"status",
         "type":c_uint8},
        {"name":"size",
         "type":c_uint16},
        {"name":"dcbdata",
         "length":MAXEDADCBSIZE,
         "type":c_uint32},

    ]
}

m2m2_dcb_gen_blk_data_t = {
    "struct_fields":
    [
        {"name":"command",
         "type":c_uint8},
        {"name":"status",
         "type":c_uint8},
        {"name":"size",
         "type":c_uint16},
        {"name":"num_of_pkts",
         "type":c_uint16},
        {"name":"dcbdata",
         "length":MAXGENBLKDCBSIZE,
         "type":c_uint32},

    ]
}

m2m2_dcb_gen_blk_user_cfg_summary_pkt_t = {
    "struct_fields":
    [
        {"name":"command",
         "type":c_uint8},
        {"name":"status",
         "type":c_uint8},
        {"name":"start_cmd_len",
         "type":c_uint16},
        {"name":"start_cmd_cnt",
         "type":c_uint16},
        {"name":"stop_cmd_len",
         "type":c_uint16},
        {"name":"stop_cmd_cnt",
         "type":c_uint16},
        {"name":"crc_16",
         "type":c_uint16},

    ]
}

m2m2_dcb_ad7156_data_t = {
    "struct_fields":
    [
        {"name":"command",
         "type":c_uint8},
        {"name":"status",
         "type":c_uint8},
        {"name":"size",
         "type":c_uint16},
        {"name":"dcbdata",
         "length":MAXAD7156DCBSIZE,
         "type":c_uint32},

    ]
}

m2m2_dcb_lt_app_lcfg_data_t = {
    "struct_fields":
    [
        {"name":"command",
         "type":c_uint8},
        {"name":"status",
         "type":c_uint8},
        {"name":"size",
         "type":c_uint16},
        {"name":"dcbdata",
         "length":MAXLTAPPLCFGDCBSIZE,
         "type":c_uint32},
    ]
}

M2M2_DCB_CONFIG_BLOCK_INDEX_t = {
    "type":c_uint8,
    "enum_values":
    [
        ("ADI_DCB_GENERAL_BLOCK_IDX",                      0x0),
        ("ADI_DCB_AD5940_BLOCK_IDX",                       0x1),
        ("ADI_DCB_ADPD4000_BLOCK_IDX",                     0x2),
        ("ADI_DCB_ADXL362_BLOCK_IDX",                      0x3),
        ("ADI_DCB_PPG_BLOCK_IDX",                          0x4),
        ("ADI_DCB_ECG_BLOCK_IDX",                          0x5),
        ("ADI_DCB_EDA_BLOCK_IDX",                          0x6),
        ("ADI_DCB_AD7156_BLOCK_IDX",                       0x7),
        ("ADI_DCB_PEDOMETER_BLOCK_IDX",                    0x8),
        ("ADI_DCB_TEMPERATURE_BLOCK_IDX",                  0x9),
        ("ADI_DCB_LT_APP_LCFG_BLOCK_IDX",                  0xA),
        ("ADI_DCB_UI_CONFIG_BLOCK_IDX",                    0xB),
        ("ADI_DCB_USER0_BLOCK_IDX",                        0xC),
        ("ADI_DCB_USER1_BLOCK_IDX",                        0xD),
        ("ADI_DCB_USER2_BLOCK_IDX",                        0xE),
        ("ADI_DCB_USER3_BLOCK_IDX",                        0xF),
        ("ADI_DCB_BCM_BLOCK_IDX",                          0x10),
        ("ADI_DCB_MAX_BLOCK_IDX",                          0x11),
    ]
}

m2m2_dcb_block_status_t = {
    "struct_fields":
    [
        {"name":"command",
         "type":c_uint8},
        {"name":"status",
         "type":c_uint8},
        {"name":"dcb_blk_array",
         "length":ADI_DCB_MAX_BLOCK_IDX,
         "type":c_uint8},

    ]
}
