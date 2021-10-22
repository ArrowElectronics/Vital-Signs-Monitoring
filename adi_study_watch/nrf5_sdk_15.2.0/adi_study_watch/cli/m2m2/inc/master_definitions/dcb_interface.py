#!/usr/bin/env python3

from ctypes import *

import m2m2_core

import common_application_interface


DCB_BLK_WORD_SZ = (4) #Word size in bytes, for an entry in DCB block
MAXADPD4000DCBSIZE = (57) #Max size of adpd4000 DCB size in double word length; 57 uint32_t elements in dcfg
MAXTEMPRLCFGDCBSIZE = (57) #Max size of temperature app lcfg DCB size in double word length; 57 uint32_t elements in dcfg
MAXADXLDCBSIZE = (25) #Max size of adxl DCB size in double word length; 25 uint32_t elements in dcfg
MAXPPGDCBSIZE = (56)
MAXECGDCBSIZE = (4)
MAXEDADCBSIZE = (2)
MAXBIADCBSIZE = (18)

"""
Size of gen Blk DCB contents passed in 1 pkt for the read/write/delete DCB
M2M2 command, in double word length;
57 uint32_t elements in 1 m2m2 pkt
"""
MAXGENBLKDCBSIZE = (57)

MAXAD7156DCBSIZE = (20)
MAXLTAPPLCFGDCBSIZE = (5)
MAXUSER0BLKDCBSIZE = (19)  #Max size of USER0 block DCB in double word length; 19 uint32_t elements in lcfg

"""
MAX_GEN_BLK_DCB_PKTS is the max no: of pkts which can be send by the tool
for the read/write/delete DCB M2M2 command
Max gen block DCB size = MAXGENBLKDCBSIZE * DCB_BLK_WORD_SZ * MAX_GEN_BLK_DCB_PKTS = 57* 4 * 18 = 4104 bytes
"""
MAX_GEN_BLK_DCB_PKTS = (18)

"""
MAX_ADPD4000_DCB_PKTS is the max no: of pkts which can be send by the tool
for the read/write/delete DCB M2M2 command
Max ADPD4000 DCB size = MAXADPD4000DCBSIZE * DCB_BLK_WORD_SZ * MAX_ADPD4000_DCB_PKTS = 57* 4 * 4 = 912 bytes
"""
MAX_ADPD4000_DCB_PKTS = (4)

"""
MAX_TEMPRLCFG_DCB_PKTS is the max no: of pkts which can be send by the tool
for the read/write/delete DCB M2M2 command
Max TEMPERATURE LCFG DCB size = MAXTEMPRLCFGDCBSIZE * DCB_BLK_WORD_SZ * MAX_TEMPRLCFG_DCB_PKTS = 57 * 4 * 2 = 456 bytes
"""
MAX_TEMPRLCFG_DCB_PKTS = (2)

class M2M2_DCB_COMMAND_ENUM_t(c_uint8):
    __M2M2_DCB_COMMAND_LOWEST = 0x96
    M2M2_DCB_COMMAND_READ_CONFIG_REQ = 0x97
    M2M2_DCB_COMMAND_READ_CONFIG_RESP = 0x98
    M2M2_DCB_COMMAND_WRITE_CONFIG_REQ = 0x99
    M2M2_DCB_COMMAND_WRITE_CONFIG_RESP = 0x9A
    M2M2_DCB_COMMAND_ERASE_CONFIG_REQ = 0x9B
    M2M2_DCB_COMMAND_ERASE_CONFIG_RESP = 0x9C
    M2M2_DCB_COMMAND_QUERY_STATUS_REQ = 0x9D
    M2M2_DCB_COMMAND_QUERY_STATUS_RESP = 0x9E

class M2M2_DCB_STATUS_ENUM_t(c_uint8):
    __M2M2_DCB_STATUS_LOWEST = 0x96
    M2M2_DCB_STATUS_OK = 0x97
    M2M2_DCB_STATUS_ERR_ARGS = 0x98
    M2M2_DCB_STATUS_ERR_NOT_CHKD = 0xFF

class m2m2_dcb_cmd_t(Structure):
    fields = [
              ("command", c_uint8),
              ("status", c_uint8),
              ]

class m2m2_dcb_adpd4000_data_t(Structure):
    fields = [
              ("command", c_uint8),
              ("status", c_uint8),
              ("size", c_uint16),
              ("num_of_pkts", c_uint16),
              ("dcbdata", c_uint32 * MAXADPD4000DCBSIZE),
              ]

class m2m2_dcb_temperature_data_t(Structure):
    fields = [
              ("command", c_uint8),
              ("status", c_uint8),
              ("size", c_uint16),
              ("num_of_pkts", c_uint16),
              ("dcbdata", c_uint32 * MAXADPD4000DCBSIZE),
              ]

class m2m2_dcb_adxl_data_t(Structure):
    fields = [
              ("command", c_uint8),
              ("status", c_uint8),
              ("size", c_uint16),
              ("dcbdata", c_uint32 * MAXADXLDCBSIZE),
              ]

class m2m2_dcb_ppg_data_t(Structure):
    fields = [
              ("command", c_uint8),
              ("status", c_uint8),
              ("size", c_uint16),
              ("dcbdata", c_int32 * MAXPPGDCBSIZE),
              ]

class m2m2_dcb_ecg_data_t(Structure):
    fields = [
              ("command", c_uint8),
              ("status", c_uint8),
              ("size", c_uint16),
              ("dcbdata", c_uint32 * MAXECGDCBSIZE),
              ]

class m2m2_dcb_eda_data_t(Structure):
    fields = [
              ("command", c_uint8),
              ("status", c_uint8),
              ("size", c_uint16),
              ("dcbdata", c_uint32 * MAXEDADCBSIZE),
              ]

class m2m2_dcb_bia_data_t(Structure):
    fields = [
              ("command", c_uint8),
              ("status", c_uint8),
              ("size", c_uint16),
              ("dcbdata", c_uint32 * MAXBIADCBSIZE),
              ]

class m2m2_dcb_gen_blk_data_t(Structure):
    fields = [
              ("command", c_uint8),
              ("status", c_uint8),
              ("size", c_uint16),
              ("num_of_pkts", c_uint16),
              ("dcbdata", c_uint32 * MAXGENBLKDCBSIZE),
              ]

class m2m2_dcb_gen_blk_user_cfg_summary_pkt_t(Structure):
    fields = [
              ("command", c_uint8),
              ("status", c_uint8),
              ("start_cmd_len", c_uint16),
              ("start_cmd_cnt", c_uint16),
              ("stop_cmd_len", c_uint16),
              ("stop_cmd_cnt", c_uint16),
              ("crc16", c_uint16),
              ]

class m2m2_dcb_ad7156_data_t(Structure):
    fields = [
              ("command", c_uint8),
              ("status", c_uint8),
              ("size", c_uint16),
              ("dcbdata", c_uint32 * MAXAD7156DCBSIZE),
              ]

class m2m2_dcb_lt_app_lcfg_data_t(Structure):
    fields = [
              ("command", c_uint8),
              ("status", c_uint8),
              ("size", c_uint16),
              ("dcbdata", c_uint32 * MAXLTAPPLCFGDCBSIZE),
              ]

class m2m2_dcb_user0_blk_data_t(Structure):
    fields = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("size", c_uint16),
              ("dcbdata", c_uint32 * MAXUSER0BLKDCBSIZE),
              ]

class M2M2_DCB_CONFIG_BLOCK_INDEX_t(c_uint8):
    ADI_DCB_GENERAL_BLOCK_IDX = 0x0
    ADI_DCB_AD5940_BLOCK_IDX = 0x1
    ADI_DCB_ADPD4000_BLOCK_IDX = 0x2
    ADI_DCB_ADXL362_BLOCK_IDX = 0x3
    ADI_DCB_PPG_BLOCK_IDX = 0x4
    ADI_DCB_ECG_BLOCK_IDX = 0x5
    ADI_DCB_EDA_BLOCK_IDX = 0x6
    ADI_DCB_AD7156_BLOCK_IDX = 0x7
    ADI_DCB_PEDOMETER_BLOCK_IDX = 0x8
    ADI_DCB_TEMPERATURE_BLOCK_IDX = 0x9
    ADI_DCB_LT_APP_LCFG_BLOCK_IDX = 0xA
    ADI_DCB_UI_CONFIG_BLOCK_IDX = 0xB
    ADI_DCB_USER0_BLOCK_IDX = 0xC
    ADI_DCB_USER1_BLOCK_IDX = 0xD
    ADI_DCB_USER2_BLOCK_IDX = 0xE
    ADI_DCB_USER3_BLOCK_IDX = 0xF
    ADI_DCB_BIA_BLOCK_IDX = 0x10
    ADI_DCB_MAX_BLOCK_IDX = 0x11

class m2m2_dcb_block_status_t(Structure):
    fields = [
              ("command", c_uint8),
              ("status", c_uint8),
              ("dcb_blk_array", c_uint8 * (M2M2_DCB_CONFIG_BLOCK_INDEX_t.ADI_DCB_MAX_BLOCK_IDX)),
              ]
