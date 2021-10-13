from ctypes import *

from common_application_interface_def import *

from m2m2_core_def import *

MAX_NUM_OF_CHAR = (20)

class FILE_STOP_LOGGING_t(c_ubyte):
    M2M2_FILE_SYS_STOP_LOGGING = 0x0
    M2M2_FILE_SYS_MEMORY_FULL = 0x1
    M2M2_FILE_SYS_BATTERY_LOW = 0x2
    M2M2_FILE_SYS_POWER_STATE_SHUTDOWN = 0x3
    M2M2_FILE_SYS_STOP_LOGGING_INVALID = 0xFF

class M2M2_FILE_SYS_CMD_ENUM_t(c_ubyte):
    _M2M2_FILE_SYS_CMD_ENUM_t__M2M2_FILE_SYS_CMD_LOWEST = 0x40
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
    M2M2_FILE_SYS_CMD_FLASH_RESET_REQ = 0x80
    M2M2_FILE_SYS_CMD_FLASH_RESET_RESP = 0x81
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
    M2M2_FILE_SYS_CMD_BLOCK_ERASE_REQ = 0x90
    M2M2_FILE_SYS_CMD_BLOCK_ERASE_RESP = 0x91
    M2M2_FILE_SYS_WRITE_RANDOM_DATA_TO_RSD_BLK_REQ = 0x92
    M2M2_FILE_SYS_WRITE_RANDOM_DATA_TO_RSD_BLK_RESP = 0x93
    M2M2_FILE_SYS_CMD_GET_FS_FORMAT_INFO_REQ = 0x94
    M2M2_FILE_SYS_CMD_GET_FS_FORMAT_INFO_RESP = 0x95
    M2M2_FILE_SYS_CMD_APPEND_FILE_REQ = 0xA0
    M2M2_FILE_SYS_CMD_APPEND_FILE_RESP = 0xA1
    M2M2_FILE_SYS_CMD_FILE_READ_TEST_REQ = 0xA2
    M2M2_FILE_SYS_CMD_FILE_READ_TEST_RESP = 0xA3

class M2M2_FILE_SYS_STATUS_ENUM_t(c_ubyte):
    _M2M2_FILE_SYS_STATUS_ENUM_t__M2M2_FILE_SYS_ERR_LOWEST = 0x40
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
    M2M2_FILE_SYS_STATUS_BLOCKS_WRITE_ERROR = 0x54
    M2M2_FILE_SYS_NO_FILE_TO_APPEND = 0x55
    M2M2_FILE_SYS_ERR_NOT_CHKD = 0xFF

class FILE_TYPE_ENUM_t(c_ubyte):
    M2M2_FILE_SYS_IS_CONFIG_FILE = 0x0
    M2M2_FILE_SYS_IS_DATA_FILE = 0x1
    M2M2_FILE_SYS_INVALID_TYPE = 0x2

class FILE_SYS_STREAM_SUBS_STATE_ENUM_t(c_ubyte):
    M2M2_FILE_SYS_UNSUBSCRIBED = 0x0
    M2M2_FILE_SYS_SUBSCRIBED = 0x1
    M2M2_FILE_SYS_SUBS_INVALID = 0xFF

class FILE_PAGE_CHUNK_RETRANSMIT_TYPE_ENUM_t(c_ubyte):
    M2M2_FILE_SYS_PAGE_CHUNK_CRC_ERROR = 0x0
    M2M2_FILE_SYS_PAGE_CHUNK_LOST = 0x1

class m2m2_file_sys_cmd_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ]

class m2m2_file_sys_blk_erase_cmd_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("block_no", c_ushort),
              ]

class m2m2_file_sys_write_rsd_blk_cmd_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("data", c_ulong * 20),
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

def m2m2_file_sys_page_chunk_retransmit_req_t(array_size):
  class m2m2_file_sys_page_chunk_retransmit_req_t_internal(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("retransmit_type", c_ubyte),
              ("page_roll_over", c_ubyte),
              ("page_chunk_number", c_ubyte),
              ("page_number", c_ushort),
              ("file_name", c_ubyte * array_size),
              ]
  return m2m2_file_sys_page_chunk_retransmit_req_t_internal()

class m2m2_file_sys_download_log_stream_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("page_chunk_number", c_ubyte),
              ("page_number", c_ushort),
              ("page_chunk_size", c_ushort),
              ("page_chunk_bytes", c_ubyte * 512),
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

class m2m2_file_sys_pattern_write_req_pkt_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("file_size", c_ulong),
              ("scale_type", c_ubyte),
              ("scale_factor", c_ushort),
              ("num_files_to_write", c_ushort),
              ]

class m2m2_file_sys_pattern_write_resp_pkt_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
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
              ("last_page_read", c_ulong),
              ("last_page_read_offset", c_ulong),
              ("last_page_read_status", c_ubyte),
              ("num_bytes_transferred", c_ulong),
              ("bytes_read", c_ulong),
              ("usb_cdc_write_failed", c_ubyte),
              ]

class m2m2_file_sys_debug_impt_info_req_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ]

class m2m2_file_sys_user_config_data(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("len_configstream", c_ushort),
              ("byte_configstream", c_ubyte * 70),
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

class m2m2_file_sys_page_read_test_req_pkt_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("page_num", c_ulong),
              ("num_bytes", c_ubyte),
              ]

class m2m2_file_sys_page_read_test_resp_pkt_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("page_num", c_ulong),
              ("ecc_zone_status", c_ubyte),
              ("next_page", c_ulong),
              ("occupied", c_ubyte),
              ("num_bytes_written", c_ushort),
              ("data_region_status", c_ubyte),
              ("sample_data", c_ubyte * 100),
              ("num_bytes", c_ubyte),
              ]

class m2m2_file_sys_sample_data_file_read_req_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("start_page_ind", c_ulong),
              ("end_page_ind", c_ulong),
              ]

class m2m2_file_sys_sample_data_file_read_resp_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("sample_data", c_ubyte * 202),
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
              ("stop_type", c_ubyte),
              ]

class m2m2_file_sys_get_bad_blocks_cmd_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("bad_blocks", c_ulong),
              ]

class m2m2_file_sys_write_blocks_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("num_blocks_write", c_ushort),
              ("start_block_num", c_ushort),
              ]

class m2m2_file_sys_format_debug_info_req_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ]

class m2m2_file_sys_debug_impt_info_resp_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("head_pointer", c_ulong),
              ("tail_pointer", c_ulong),
              ("usb_avg_tx_time", c_ulong),
              ("usb_avg_port_write_time", c_ulong),
              ("page_read_time", c_ulong),
              ("init_circular_buffer_flag", c_ushort),
              ("mem_full_flag", c_ushort),
              ("data_offset", c_ushort),
              ("config_file_occupied", c_ushort),
              ("page_write_time", c_ulong),
              ("fs_display_query_cnt", c_ushort),
              ("min_timer_cnt", c_ushort),
              ]

class m2m2_file_sys_format_debug_info_resp_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("erase_failed_due_bad_block_check", c_ubyte),
              ("wrap_around_cond", c_ubyte),
              ("nothing_is_written_to_erase_error", c_ubyte),
              ("mem_full_in_partial_erase", c_ubyte),
              ("toc_mem_erased_flag", c_ubyte),
              ("succesfull_erase_flag", c_ubyte),
              ("num_blocks_erased_in_mem_full_partial_erase", c_ushort),
              ("num_blocks_erased_in_partial_erase_1", c_ushort),
              ("num_blocks_erased_in_partial_erase_2", c_ushort),
              ("num_times_format_failed_due_bad_blocks_1", c_ushort),
              ("num_times_format_failed_due_bad_blocks_2", c_ushort),
              ("format_src_blk_ind", c_ulong),
              ("format_dest_blk_ind_1", c_ulong),
              ("format_dest_blk_ind_2", c_ulong),
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
              ("start_page", c_ulong),
              ("end_page", c_ulong),
              ("file_size", c_ulong),
              ]

