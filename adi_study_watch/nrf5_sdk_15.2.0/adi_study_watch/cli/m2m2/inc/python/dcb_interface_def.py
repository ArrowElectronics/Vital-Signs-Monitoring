from ctypes import *

from common_application_interface_def import *

from m2m2_core_def import *

DCB_BLK_WORD_SZ = (4)
MAXAD7156DCBSIZE = (20)
MAXADPD4000DCBSIZE = (57)
MAXTEMPRLCFGDCBSIZE = (57)
MAXADXLDCBSIZE = (25)
MAXBIADCBSIZE = (18)
MAXECGDCBSIZE = (4)
MAXEDADCBSIZE = (2)
MAXGENBLKDCBSIZE = (57)
MAXLTAPPLCFGDCBSIZE = (5)
MAXPPGDCBSIZE = (56)
MAXUSER0BLKDCBSIZE = (19)
MAX_ADPD4000_DCB_PKTS = (4)
MAX_TEMPRLCFG_DCB_PKTS = (2)
MAX_GEN_BLK_DCB_PKTS = (18)

class M2M2_DCB_COMMAND_ENUM_t(c_ubyte):
    _M2M2_DCB_COMMAND_ENUM_t__M2M2_DCB_COMMAND_LOWEST = 0x96
    M2M2_DCB_COMMAND_READ_CONFIG_REQ = 0x97
    M2M2_DCB_COMMAND_READ_CONFIG_RESP = 0x98
    M2M2_DCB_COMMAND_WRITE_CONFIG_REQ = 0x99
    M2M2_DCB_COMMAND_WRITE_CONFIG_RESP = 0x9A
    M2M2_DCB_COMMAND_ERASE_CONFIG_REQ = 0x9B
    M2M2_DCB_COMMAND_ERASE_CONFIG_RESP = 0x9C
    M2M2_DCB_COMMAND_QUERY_STATUS_REQ = 0x9D
    M2M2_DCB_COMMAND_QUERY_STATUS_RESP = 0x9E

class M2M2_DCB_STATUS_ENUM_t(c_ubyte):
    _M2M2_DCB_STATUS_ENUM_t__M2M2_DCB_STATUS_LOWEST = 0x96
    M2M2_DCB_STATUS_OK = 0x97
    M2M2_DCB_STATUS_ERR_ARGS = 0x98
    M2M2_DCB_STATUS_ERR_NOT_CHKD = 0xFF

class M2M2_DCB_CONFIG_BLOCK_INDEX_t(c_ubyte):
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

class m2m2_dcb_cmd_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ]

class m2m2_dcb_adpd4000_data_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("size", c_ushort),
              ("num_of_pkts", c_ushort),
              ("dcbdata", c_ulong * 57),
              ]

class m2m2_dcb_temperature_data_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("size", c_ushort),
              ("num_of_pkts", c_ushort),
              ("dcbdata", c_ulong * 57),
              ]

class m2m2_dcb_adxl_data_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("size", c_ushort),
              ("dcbdata", c_ulong * 25),
              ]

class m2m2_dcb_ppg_data_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("size", c_ushort),
              ("dcbdata", c_long * 56),
              ]

class m2m2_dcb_ecg_data_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("size", c_ushort),
              ("dcbdata", c_ulong * 4),
              ]

class m2m2_dcb_eda_data_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("size", c_ushort),
              ("dcbdata", c_ulong * 2),
              ]

class m2m2_dcb_bia_data_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("size", c_ushort),
              ("dcbdata", c_ulong * 18),
              ]

class m2m2_dcb_gen_blk_data_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("size", c_ushort),
              ("num_of_pkts", c_ushort),
              ("dcbdata", c_ulong * 57),
              ]

class m2m2_dcb_gen_blk_user_cfg_summary_pkt_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("start_cmd_len", c_ushort),
              ("start_cmd_cnt", c_ushort),
              ("stop_cmd_len", c_ushort),
              ("stop_cmd_cnt", c_ushort),
              ("crc16", c_ushort),
              ]

class m2m2_dcb_ad7156_data_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("size", c_ushort),
              ("dcbdata", c_ulong * 20),
              ]

class m2m2_dcb_lt_app_lcfg_data_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("size", c_ushort),
              ("dcbdata", c_ulong * 5),
              ]

class m2m2_dcb_user0_blk_data_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("size", c_ushort),
              ("dcbdata", c_ulong * 19),
              ]

class m2m2_dcb_block_status_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("dcb_blk_array", c_ubyte * 17),
              ]

