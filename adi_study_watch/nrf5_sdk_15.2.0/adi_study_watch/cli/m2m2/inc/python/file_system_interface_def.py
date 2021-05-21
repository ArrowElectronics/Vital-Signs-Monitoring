from ctypes import *

from common_application_interface_def import *

from m2m2_core_def import *

class FILE_STOP_LOGGING_t(c_ubyte):
    M2M2_FILE_SYS_STOP_LOGGING = 0x0
    M2M2_FILE_SYS_MEMORY_FULL = 0x1
    M2M2_FILE_SYS_BATTERY_LOW = 0x2
    M2M2_FILE_SYS_POWER_STATE_SHUTDOWN = 0x3
    M2M2_FILE_SYS_STOP_LOGGING_INVALID = 0xFF

class M2M2_FILE_SYS_CMD_ENUM_t(c_ubyte):
    __M2M2_FILE_SYS_CMD_LOWEST = 0x40
    M2M2_FILE_SYS_CMD_MOUNT_REQ = 0x42
    M2M2_FILE_SYS_CMD_MOUNT_RESP = 0x43
    M2M2_FILE_SYS_CMD_FORMAT_REQ = 0x46
    M2M2_FILE_SYS_CMD_FORMAT_RESP = 0x47
    M2M2_FILE_SYS_CMD_LS_REQ = 0x48
    M2M2_FILE_SYS_CMD_LS_RESP = 0x49
    M2M2_FILE_SYS_CMD_GET_REQ = 0x4C
    M2M2_FILE_SYS_CMD_GET_RESP = 0x4D
    M2M2_FILE_SYS_CMD_VOL_INFO_REQ = 0x4E
    M2M2_FILE_SYS_CMD_VOL_INFO_RESP = 0x4F
    M2M2_FILE_SYS_CMD_GET_FS_STATUS_REQ = 0x50
    M2M2_FILE_SYS_CMD_GET_FS_STATUS_RESP = 0x51
    M2M2_FILE_SYS_CMD_GET_FS_STREAM_SUB_STATUS_REQ = 0x52
    M2M2_FILE_SYS_CMD_GET_FS_STREAM_SUB_STATUS_RESP = 0x53
    M2M2_FILE_SYS_CMD_STOP_FS_STREAM_REQ = 0x54
    M2M2_FILE_SYS_CMD_STOP_FS_STREAM_RESP = 0x55
    M2M2_FILE_SYS_CMD_GET_FS_DEBUG_INFO_REQ = 0x56
    M2M2_FILE_SYS_CMD_GET_FS_DEBUG_INFO_RESP = 0x57
    M2M2_FILE_SYS_CMD_TEST_LOG_REQ = 0x58
    M2M2_FILE_SYS_CMD_TEST_LOG_RESP = 0x59
    M2M2_FILE_SYS_CMD_FORCE_STOP_LOG_REQ = 0x5A
    M2M2_FILE_SYS_CMD_FORCE_STOP_LOG_RESP = 0x5B
    M2M2_FILE_SYS_CMD_SET_KEY_VALUE_PAIR_REQ = 0x64
    M2M2_FILE_SYS_CMD_SET_KEY_VALUE_PAIR_RESP = 0x65
    M2M2_FILE_SYS_CMD_DCFG_START_LOG_REQ = 0x66
    M2M2_FILE_SYS_CMD_DCFG_START_LOG_RESP = 0x67
    M2M2_FILE_SYS_CMD_DCFG_STOP_LOG_REQ = 0x68
    M2M2_FILE_SYS_CMD_DCFG_STOP_LOG_RESP = 0x69
    M2M2_FILE_SYS_CMD_LOG_USER_CONFIG_DATA_REQ = 0x6A
    M2M2_FILE_SYS_CMD_LOG_USER_CONFIG_DATA_RESP = 0x6B
    M2M2_FILE_SYS_CMD_FIND_CONFIG_FILE_REQ = 0x6C
    M2M2_FILE_SYS_CMD_FIND_CONFIG_FILE_RESP = 0x6D
    M2M2_FILE_SYS_CMD_DELETE_CONFIG_FILE_REQ = 0x6E
    M2M2_FILE_SYS_CMD_DELETE_CONFIG_FILE_RESP = 0x6F
    M2M2_FILE_SYS_CMD_GET_NUMBER_OF_FILE_REQ = 0x70
    M2M2_FILE_SYS_CMD_GET_NUMBER_OF_FILE_RESP = 0x71
    M2M2_FILE_SYS_CMD_LOG_STREAM_REQ = 0x72
    M2M2_FILE_SYS_CMD_LOG_STREAM_RESP = 0x73
    M2M2_FILE_SYS_CMD_STOP_STREAM_REQ = 0x74
    M2M2_FILE_SYS_CMD_STOP_STREAM_RESP = 0x75
    M2M2_FILE_SYS_CMD_START_LOGGING_REQ = 0x76
    M2M2_FILE_SYS_CMD_START_LOGGING_RESP = 0x77
    M2M2_FILE_SYS_CMD_STOP_LOGGING_REQ = 0x78
    M2M2_FILE_SYS_CMD_STOP_LOGGING_RESP = 0x79
    M2M2_FILE_SYS_CMD_DOWNLOAD_LOG_REQ = 0x7A
    M2M2_FILE_SYS_CMD_DOWNLOAD_LOG_RESP = 0x7B
    M2M2_FILE_SYS_CMD_CANCEL_DOWNLOAD_LOG_REQ = 0x7C
    M2M2_FILE_SYS_CMD_CANCEL_DOWNLOAD_LOG_RESP = 0x7D
    M2M2_FILE_SYS_CMD_GET_BAD_BLOCKS_REQ = 0x7E
    M2M2_FILE_SYS_CMD_GET_BAD_BLOCKS_RESP = 0x7F
    M2M2_FILE_SYS_CMD_DISPLAY_VOL_INFO_REQ = 0x82
    M2M2_FILE_SYS_CMD_DISPLAY_VOL_INFO_RESP = 0x83
    M2M2_FILE_SYS_CMD_CHUNK_RETRANSMIT_REQ = 0x84
    M2M2_FILE_SYS_CMD_CHUNK_RETRANSMIT_RESP = 0x85  
    M2M2_FILE_SYS_BLOCKS_WRITE_REQ = 0x86
    M2M2_FILE_SYS_BLOCKS_WRITE_RESP = 0x87
    M2M2_FILE_SYS_CMD_GET_IMPT_DEBUG_INFO_REQ = 0x88
    M2M2_FILE_SYS_CMD_GET_IMPT_DEBUG_INFO_RESP = 0x89
    M2M2_FILE_SYS_CMD_PATTERN_WRITE_REQ = 0x8A
    M2M2_FILE_SYS_CMD_PATTERN_WRITE_RESP = 0x8B
    M2M2_FILE_SYS_CMD_GET_FILE_INFO_REQ = 0x8C
    M2M2_FILE_SYS_CMD_GET_FILE_INFO_RESP = 0x8D
    M2M2_FILE_SYS_CMD_PAGE_READ_TEST_REQ = 0x8E
    M2M2_FILE_SYS_CMD_PAGE_READ_TEST_RESP = 0x8F

class M2M2_FILE_SYS_STATUS_ENUM_t(c_ubyte):
    __M2M2_FILE_SYS_ERR_LOWEST = 0x40
    M2M2_FILE_SYS_STATUS_OK = 0x41
    M2M2_FILE_SYS_STATUS_ERROR = 0x42
    M2M2_FILE_SYS_END_OF_FILE = 0x43
    M2M2_FILE_SYS_END_OF_DIR = 0x44
    M2M2_FILE_SYS_ERR_INVALID = 0x45
    M2M2_FILE_SYS_ERR_ARGS = 0x46
    M2M2_FILE_SYS_ERR_FORMAT = 0x47
    M2M2_FILE_SYS_ERR_MEMORY_FULL = 0x48
    M2M2_FILE_SYS_ERR_LOG_FORCE_STOPPED = 0x49
    M2M2_FILE_SYS_ERR_MAX_FILE_COUNT = 0x4A
    M2M2_FILE_SYS_CONFIG_FILE_FOUND = 0x4B
    M2M2_FILE_SYS_CONFIG_FILE_NOT_FOUND = 0x4C
    M2M2_FILE_SYS_STATUS_LOGGING_STOPPED = 0x4D
    M2M2_FILE_SYS_STATUS_LOGGING_IN_PROGRESS = 0x4E
    M2M2_FILE_SYS_STATUS_LOGGING_ERROR = 0x4F
    M2M2_FILE_SYS_STATUS_LOGGING_NOT_STARTED = 0x50
    M2M2_FILE_SYS_ERR_BATTERY_LOW = 0x51
    M2M2_FILE_SYS_ERR_POWER_STATE_SHUTDOWN = 0x52
    M2M2_FILE_SYS_ERR_CONFIG_FILE_POSITION = 0x53
    M2M2_FILE_SYS_ERR_NOT_CHKD = 0xFF

class FILE_TYPE_ENUM_t(c_ubyte):
    M2M2_FILE_SYS_IS_CONFIG_FILE = 0x0
    M2M2_FILE_SYS_IS_DATA_FILE = 0x1
    M2M2_FILE_SYS_INVALID_TYPE = 0x2

class FILE_SYS_STREAM_SUBS_STATE_ENUM_t(c_ubyte):
    M2M2_FILE_SYS_UNSUBSCRIBED = 0x0
    M2M2_FILE_SYS_SUBSCRIBED = 0x1
    M2M2_FILE_SYS_SUBS_INVALID = 0xFF

class FILE_STOP_LOGGING_t(c_ubyte):
    M2M2_FILE_SYS_STOP_LOGGING = 0x0
    M2M2_FILE_SYS_MEMORY_FULL = 0x1
    M2M2_FILE_SYS_BATTERY_LOW = 0x2
    M2M2_FILE_SYS_POWER_STATE_SHUTDOWN = 0x3
    M2M2_FILE_SYS_STOP_LOGGING_INVALID = 0xFF
    
class m2m2_file_sys_cmd_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ]    

def m2m2_file_sys_ls_req_t(array_size):
  class m2m2_file_sys_ls_req_t_internal(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("dir_path", c_ubyte * array_size),
              ]
  return m2m2_file_sys_ls_req_t_internal()

class m2m2_file_sys_ls_resp_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("full_file_name", c_ubyte * 40),
              ("filetype", c_ubyte),
              ("filesize", c_ulong),
              ]

def m2m2_file_sys_get_req_t(array_size):
  class m2m2_file_sys_get_req_t_internal(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("file_name", c_ubyte * array_size),
              ]
  return m2m2_file_sys_get_req_t_internal()

class m2m2_file_sys_download_log_stream_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("len_stream", c_ushort),
              ("byte_stream", c_ubyte * 512),
              ("crc16", c_ushort),
              ]

class m2m2_file_sys_app_ref_hr_stream_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("refhr", c_ushort),
              ("year", c_ushort),
              ("month", c_ubyte),
              ("day", c_ubyte),
              ("hour", c_ubyte),
              ("minute", c_ubyte),
              ("second", c_ubyte),
              ("TZ_sec", c_ulong),
              ]

class m2m2_file_sys_set_key_value_pair_req_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("valueID", c_ubyte * 16),
              ]

class m2m2_file_sys_set_key_value_pair_resp_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("keyID", c_ushort),
              ("valueID", c_ubyte * 16),
              ("year", c_ushort),
              ("month", c_ubyte),
              ("day", c_ubyte),
              ("hour", c_ubyte),
              ("minute", c_ubyte),
              ("second", c_ubyte),
              ("TZ_sec", c_ulong),
              ]

class m2m2_file_sys_vol_info_resp_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("totalmemory", c_ulong),
              ("usedmemory", c_ulong),
              ("availmemory", c_ushort),
              ]

class m2m2_file_sys_get_subs_status_resp_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("stream", c_ushort),
              ("subs_state", c_ubyte),
              ]
            
class m2m2_file_sys_log_stream_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("stream", c_ushort),
              ]
                        

class m2m2_file_sys_stop_log_cmd_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("stop_type", FILE_STOP_LOGGING_t),
              ]
           
class m2m2_file_sys_debug_info_req_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("stream", c_ushort),
              ]

class m2m2_file_sys_debug_info_resp_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("stream", c_ushort),
              ("packets_received", c_ulong),
              ("packets_missed", c_ulong),
              ("last_page_read", c_uint32),
              ("last_page_read_offset",c_uint32),
   	          ("last_page_read_status", c_uint8),
              ("num_bytes_transferred", c_uint32),
              ("bytes_read", c_uint32),
              ("usb_cdc_write_failed", c_uint8),
              ]

class m2m2_file_sys_user_config_data(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("len_configstream", c_ushort),
              ("byte_configstream", c_ubyte * 70),
              ]

class m2m2_file_sys_impt_debug_info_req_t(Structure):
    _pack_ = 1
    _fields_ = [
                ("command", c_ubyte), 
                ("status", c_ubyte), 
               ]

class m2m2_file_sys_impt_debug_info_resp_t(Structure):
    _pack_ = 1
    _fields_ = [
                ("command", c_ubyte),
                ("status", c_ubyte),
                ("head_pointer", c_uint32),
                ("tail_pointer", c_uint32),
                ("usb_avg_tx_time",c_uint32),
                ("usb_avg_port_write_time",c_uint32),
                ("page_read_time",c_uint32),
                ("init_circular_buffer_flag",c_uint16),
                ("mem_full_flag",c_uint16),
                ("data_offset",c_uint16),
                ("config_file_occupied",c_uint16),
                ("page_write_time",c_uint32),
               ]
class m2m2_file_sys_user_cfg_summary_pkt_t(Structure):
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

class m2m2_file_sys_get_file_count_pkt_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("file_count", c_ushort),
              ] 
              
class m2m2_file_sys_get_file_info_req_pkt_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("file_index", c_ubyte),
              ]              
      
class m2m2_file_sys_get_file_info_resp_pkt_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("file_name", c_ubyte * 16),
              ("start_page", c_uint32),
              ("end_page", c_uint32),
              ("file_size", c_uint32),
              ]       
              
class m2m2_file_sys_page_test_req_pkt_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("page_num", c_uint32),
              ("num_bytes", c_ubyte),
              ]              
      
class m2m2_file_sys_page_test_resp_pkt_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("page_num", c_uint32),
              ("ecc_zone_status", c_ubyte),
              ("next_page", c_uint32),
              ("occupied", c_ubyte),
              ("data_region_status", c_ubyte),
              ("sample_data",  c_ubyte * 100),
              ("num_bytes",  c_ubyte),
              ]  
              
class m2m2_file_sys_pattern_write_req_pkt_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("file_size", c_uint32),
              ("scale_type", c_ubyte),
              ("scale_factor", c_uint16),
              ("num_files_to_write", c_uint16),
              ]

class m2m2_file_sys_pattern_write_resp_pkt_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ]
              
def m2m2_file_sys_pkt_retransmit_req_t(array_size):
  class m2m2_file_sys_pkt_retransmit_req_t_internal(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("Roll_over", c_ubyte),
              ("chunk_number", c_ushort),
              ("file_name", c_ubyte * array_size),
              ]
  return m2m2_file_sys_pkt_retransmit_req_t_internal()

class m2m2_file_sys_get_bad_blocks_cmd_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("bad_blocks", c_ulong),
              ]

class m2m2_file_sys_get_resp_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("len_stream", c_ushort),
              ("byte_stream", c_ubyte * 512),
              ("crc16", c_ushort),
              ]
