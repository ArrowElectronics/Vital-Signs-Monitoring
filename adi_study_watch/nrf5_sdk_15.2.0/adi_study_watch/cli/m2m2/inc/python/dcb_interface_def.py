import array as arr
from ctypes import *

from common_application_interface_def import *

from m2m2_core_def import *

MAXADPD4000DCBSIZE = (57) #//Max size of adpd4000 DCB size in double word length; 57 uint32_t elements in dcfg
MAXADXLDCBSIZE = (25)
MAXPPGDCBSIZE = (53)
MAXECGDCBSIZE = (2)
MAXEDADCBSIZE = (2)
MAXBCMDCBSIZE = (2)
MAXGENBLKDCBSIZE = (57)
MAXAD7156DCBSIZE = (20)
MAXWRISTDETECTDCBSIZE = (4)

class M2M2_DCB_COMMAND_ENUM_t(c_ubyte):
    __M2M2_DCB_COMMAND_LOWEST = 0x96
    M2M2_DCB_COMMAND_READ_CONFIG_REQ = 0x97
    M2M2_DCB_COMMAND_READ_CONFIG_RESP = 0x98
    M2M2_DCB_COMMAND_WRITE_CONFIG_REQ = 0x99
    M2M2_DCB_COMMAND_WRITE_CONFIG_RESP = 0x9A
    M2M2_DCB_COMMAND_ERASE_CONFIG_REQ = 0x9B
    M2M2_DCB_COMMAND_ERASE_CONFIG_RESP = 0x9C
    M2M2_DCB_COMMAND_QUERY_STATUS_REQ = 0x9D
    M2M2_DCB_COMMAND_QUERY_STATUS_RESP = 0x9E

class M2M2_DCB_STATUS_ENUM_t(c_ubyte):
    __M2M2_DCB_STATUS_LOWEST = 0x96
    M2M2_DCB_STATUS_OK = 0x97
    M2M2_DCB_STATUS_ERR_ARGS = 0x98
    M2M2_DCB_STATUS_ERR_NOT_CHKD = 0xFF

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
              ("size", c_uint16),
              ("num_of_pkts", c_uint16),
              ("dcbdata", c_uint32 * MAXADPD4000DCBSIZE),
              ]

class m2m2_dcb_adxl_data_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("size", c_uint16),
              ("dcbdata", c_uint32 * MAXADXLDCBSIZE),
              ]

class m2m2_dcb_ppg_data_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("size", c_uint16),
              ("dcbdata", c_int32 * MAXPPGDCBSIZE),
              ]

class m2m2_dcb_ecg_data_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("size", c_uint16),
              ("dcbdata", c_uint32 * MAXECGDCBSIZE),
              ]

class m2m2_dcb_eda_data_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("size", c_uint16),
              ("dcbdata", c_uint32 * MAXEDADCBSIZE),
              ]
class m2m2_dcb_bcm_data_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("size", c_uint16),
              ("dcbdata", c_uint32 * MAXBCMDCBSIZE),
              ]              

class m2m2_dcb_gen_blk_data_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("size", c_uint16),
              ("num_of_pkts", c_uint16),
              ("dcbdata", c_uint32 * MAXGENBLKDCBSIZE),
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
              ("size", c_uint16),
              ("dcbdata", c_uint32 * MAXAD7156DCBSIZE),
              ]

class m2m2_dcb_wrist_detect_data_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("size", c_uint16),
              ("dcbdata", c_uint32 * MAXWRISTDETECTDCBSIZE),
              ]

class M2M2_DCB_CONFIG_BLOCK_INDEX_t(c_uint32):
    ADI_DCB_GENERAL_BLOCK_IDX = 0
    ADI_DCB_AD5940_BLOCK_IDX = 1
    ADI_DCB_ADPD4000_BLOCK_IDX = 2
    ADI_DCB_ADXL362_BLOCK_IDX = 3
    ADI_DCB_PPG_BLOCK_IDX = 4
    ADI_DCB_ECG_BLOCK_IDX = 5
    ADI_DCB_EDA_BLOCK_IDX = 6
    ADI_DCB_AD7156_BLOCK_IDX = 7
    ADI_DCB_PEDOMETER_BLOCK_IDX = 8
    ADI_DCB_TEMPERATURE_BLOCK_IDX = 9
    ADI_DCB_WRIST_DETECT_BLOCK_IDX = 10
    ADI_DCB_UI_CONFIG_BLOCK_IDX = 11
    ADI_DCB_USER0_BLOCK_IDX = 12
    ADI_DCB_USER1_BLOCK_IDX = 13
    ADI_DCB_USER2_BLOCK_IDX = 14
    ADI_DCB_USER3_BLOCK_IDX = 15
    ADI_DCB_BCM_BLOCK_IDX = 16
    ADI_DCB_MAX_BLOCK_IDX  = 17

class m2m2_dcb_block_status_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("dcb_blk_array", c_ubyte * (M2M2_DCB_CONFIG_BLOCK_INDEX_t.ADI_DCB_MAX_BLOCK_IDX)),
              ]
