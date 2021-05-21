#!/usr/bin/env python3

from ctypes import *

import m2m2_core

import common_application_interface

FILE_STOP_LOGGING_t = {
  "type":c_uint8,
  "enum_values": [
    ("M2M2_FILE_SYS_STOP_LOGGING",                          0x0),
    ("M2M2_FILE_SYS_MEMORY_FULL",                           0x1),
    ("M2M2_FILE_SYS_BATTERY_LOW",                           0x2),
    ("M2M2_FILE_SYS_POWER_STATE_SHUTDOWN",                  0x3),
    ("M2M2_FILE_SYS_STOP_LOGGING_INVALID",                  0xFF),
    ]
}

M2M2_FILE_SYS_CMD_ENUM_t = {
  "type":c_uint8,
  "enum_values": [
    ("__M2M2_FILE_SYS_CMD_LOWEST",                      0x40),
    ("M2M2_FILE_SYS_CMD_MOUNT_REQ",                     0x42),
    ("M2M2_FILE_SYS_CMD_MOUNT_RESP",                    0x43),
    ("M2M2_FILE_SYS_CMD_FORMAT_REQ",                    0x46),
    ("M2M2_FILE_SYS_CMD_FORMAT_RESP",                   0x47),
    ("M2M2_FILE_SYS_CMD_LS_REQ",                        0x48),
    ("M2M2_FILE_SYS_CMD_LS_RESP",                       0x49),
    ("M2M2_FILE_SYS_CMD_GET_REQ",                       0x4C),
    ("M2M2_FILE_SYS_CMD_GET_RESP",                      0x4D),
    ("M2M2_FILE_SYS_CMD_VOL_INFO_REQ",                  0x4E),
    ("M2M2_FILE_SYS_CMD_VOL_INFO_RESP",                 0x4F),
    ("M2M2_FILE_SYS_CMD_GET_FS_STATUS_REQ",             0x50),
    ("M2M2_FILE_SYS_CMD_GET_FS_STATUS_RESP",            0x51),
    ("M2M2_FILE_SYS_CMD_GET_FS_STREAM_SUB_STATUS_REQ",  0x52),
    ("M2M2_FILE_SYS_CMD_GET_FS_STREAM_SUB_STATUS_RESP", 0x53),
    ("M2M2_FILE_SYS_CMD_STOP_FS_STREAM_REQ",            0x54),
    ("M2M2_FILE_SYS_CMD_STOP_FS_STREAM_RESP",           0x55),
    ("M2M2_FILE_SYS_CMD_GET_FS_DEBUG_INFO_REQ",         0x56),
    ("M2M2_FILE_SYS_CMD_GET_FS_DEBUG_INFO_RESP",        0x57),
    ("M2M2_FILE_SYS_CMD_TEST_LOG_REQ",                  0x58),
    ("M2M2_FILE_SYS_CMD_TEST_LOG_RESP",                 0x59),
    ("M2M2_FILE_SYS_CMD_FORCE_STOP_LOG_REQ",            0x5A),
    ("M2M2_FILE_SYS_CMD_FORCE_STOP_LOG_RESP",           0x5B),
    ("M2M2_FILE_SYS_CMD_SET_KEY_VALUE_PAIR_REQ",        0x64),
    ("M2M2_FILE_SYS_CMD_SET_KEY_VALUE_PAIR_RESP",       0x65),    
    ("M2M2_FILE_SYS_CMD_DCFG_START_LOG_REQ",            0x66),    
    ("M2M2_FILE_SYS_CMD_DCFG_START_LOG_RESP",           0x67),
    ("M2M2_FILE_SYS_CMD_DCFG_STOP_LOG_REQ",             0x68),
    ("M2M2_FILE_SYS_CMD_DCFG_STOP_LOG_RESP",            0x69),    
    ("M2M2_FILE_SYS_CMD_LOG_USER_CONFIG_DATA_REQ",      0x6A),
    ("M2M2_FILE_SYS_CMD_LOG_USER_CONFIG_DATA_RESP",     0x6B),
    ("M2M2_FILE_SYS_CMD_FIND_CONFIG_FILE_REQ",          0x6C),
    ("M2M2_FILE_SYS_CMD_FIND_CONFIG_FILE_RESP",         0x6D),
    ("M2M2_FILE_SYS_CMD_DELETE_CONFIG_FILE_REQ",        0x6E),
    ("M2M2_FILE_SYS_CMD_DELETE_CONFIG_FILE_RESP",       0x6F),
    ("M2M2_FILE_SYS_CMD_GET_NUMBER_OF_FILE_REQ",        0x70),
    ("M2M2_FILE_SYS_CMD_GET_NUMBER_OF_FILE_RESP",       0x71),
    ("M2M2_FILE_SYS_CMD_LOG_STREAM_REQ",                0x72),
    ("M2M2_FILE_SYS_CMD_LOG_STREAM_RESP",               0x73),
    ("M2M2_FILE_SYS_CMD_STOP_STREAM_REQ",               0x74),
    ("M2M2_FILE_SYS_CMD_STOP_STREAM_RESP",              0x75),
    ("M2M2_FILE_SYS_CMD_START_LOGGING_REQ",             0x76),
    ("M2M2_FILE_SYS_CMD_START_LOGGING_RESP",            0x77),
    ("M2M2_FILE_SYS_CMD_STOP_LOGGING_REQ",              0x78),
    ("M2M2_FILE_SYS_CMD_STOP_LOGGING_RESP",             0x79),
    ("M2M2_FILE_SYS_CMD_DOWNLOAD_LOG_REQ",              0x7A),
    ("M2M2_FILE_SYS_CMD_DOWNLOAD_LOG_RESP",             0x7B),
    ("M2M2_FILE_SYS_CMD_CANCEL_DOWNLOAD_LOG_REQ",       0x7C),
    ("M2M2_FILE_SYS_CMD_CANCEL_DOWNLOAD_LOG_RESP",      0x7D),
    ("M2M2_FILE_SYS_CMD_GET_BAD_BLOCKS_REQ",            0x7E),
    ("M2M2_FILE_SYS_CMD_GET_BAD_BLOCKS_RESP",           0x7F),
    ("M2M2_FILE_SYS_CMD_DISPLAY_VOL_INFO_REQ",          0x82),
    ("M2M2_FILE_SYS_CMD_DISPLAY_VOL_INFO_RESP",         0x83),
    ("M2M2_FILE_SYS_CMD_CHUNK_RETRANSMIT_REQ",          0x84),
    ("M2M2_FILE_SYS_CMD_CHUNK_RETRANSMIT_RESP",         0x85),
    ("M2M2_FILE_SYS_BLOCKS_WRITE_REQ",                  0x86),
    ("M2M2_FILE_SYS_BLOCKS_WRITE_RESP",                 0x87),
    ("M2M2_FILE_SYS_CMD_GET_IMPT_DEBUG_INFO_REQ",       0x88),
    ("M2M2_FILE_SYS_CMD_GET_IMPT_DEBUG_INFO_RESP",      0x89),
    ("M2M2_FILE_SYS_CMD_PATTERN_WRITE_REQ",             0x8A),
    ("M2M2_FILE_SYS_CMD_PATTERN_WRITE_RESP",            0x8B),
    ("M2M2_FILE_SYS_CMD_GET_FILE_INFO_REQ",             0x8C),
    ("M2M2_FILE_SYS_CMD_GET_FILE_INFO_RESP",            0x8D),
    ("M2M2_FILE_SYS_CMD_PAGE_READ_TEST_REQ",            0x8E),
    ("M2M2_FILE_SYS_CMD_PAGE_READ_TEST_RESP",           0x8F),
     ]
}

M2M2_FILE_SYS_STATUS_ENUM_t = {
  "type":c_uint8,
  "enum_values": [
    ("__M2M2_FILE_SYS_ERR_LOWEST",              0x40),
    ("M2M2_FILE_SYS_STATUS_OK",                 0x41),
    ("M2M2_FILE_SYS_STATUS_ERROR",              0x42),
    ("M2M2_FILE_SYS_END_OF_FILE",               0x43),
    ("M2M2_FILE_SYS_END_OF_DIR",                0x44),
    ("M2M2_FILE_SYS_ERR_INVALID",               0x45),
    ("M2M2_FILE_SYS_ERR_ARGS",                  0x46),
    ("M2M2_FILE_SYS_ERR_FORMAT",                0x47),
    ("M2M2_FILE_SYS_ERR_MEMORY_FULL",           0x48),
    ("M2M2_FILE_SYS_ERR_LOG_FORCE_STOPPED",     0x49),
    ("M2M2_FILE_SYS_ERR_MAX_FILE_COUNT",        0x4A),
    ("M2M2_FILE_SYS_CONFIG_FILE_FOUND",         0x4B),
    ("M2M2_FILE_SYS_CONFIG_FILE_NOT_FOUND",     0x4C),
    ("M2M2_FILE_SYS_STATUS_LOGGING_STOPPED",    0x4D),
    ("M2M2_FILE_SYS_STATUS_LOGGING_IN_PROGRESS",0x4E),
    ("M2M2_FILE_SYS_STATUS_LOGGING_ERROR",      0x4F),
    ("M2M2_FILE_SYS_STATUS_LOGGING_NOT_STARTED",0x50),
    ("M2M2_FILE_SYS_ERR_BATTERY_LOW",           0x51),
    ("M2M2_FILE_SYS_ERR_POWER_STATE_SHUTDOWN",  0x52),
    ("M2M2_FILE_SYS_ERR_CONFIG_FILE_POSITION",  0x53),      
    ("M2M2_FILE_SYS_ERR_NOT_CHKD",              0xFF),
    ]
}

FILE_TYPE_ENUM_t = {
  "type":c_uint8,
  "enum_values": [
    ("M2M2_FILE_SYS_IS_DIR",        0x0),
    ("M2M2_FILE_SYS_IS_FILE",       0x1),
    ("M2M2_FILE_SYS_INVALID_TYPE",  0x2),
    ]
}

FILE_SYS_STREAM_SUBS_STATE_ENUM_t = {
  "type":c_uint8,
  "enum_values": [
    ("M2M2_FILE_SYS_UNSUBSCRIBED",  0x0),
    ("M2M2_FILE_SYS_SUBSCRIBED",    0x1),
    ("M2M2_FILE_SYS_SUBS_INVALID",  0xFF),
    ]
}

m2m2_file_sys_cmd_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
  ]
}

m2m2_file_sys_ls_req_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"dir_path",
    "length":0,
    "type":c_uint8},
  ]
}

m2m2_file_sys_ls_resp_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"full_file_name",
    "length":40,
    "type":c_uint8},
    {"name":"filetype",
    "type":FILE_TYPE_ENUM_t},
    {"name":"filesize",
    "type":c_uint32},
  ]
}

m2m2_file_sys_get_req_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"file_name",
    "length":0,
    "type":c_uint8},
  ]
}

m2m2_file_sys_get_resp_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"len_stream",
    "type":c_uint16},
    {"name":"byte_stream",
    "length":512,
    "type":c_uint8},
    {"name":"crc16",
    "type":c_uint16},
  ]
}

m2m2_file_sys_pkt_retransmit_req_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"Roll_over",
    "type":c_uint8},
    {"name":"chunk_number",
    "type":c_uint16},
    {"name":"file_name",
    "length":0,
    "type":c_uint8},
  ]
}

m2m2_file_sys_download_log_stream_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"len_stream",
    "type":c_uint16},
    {"name":"byte_stream",
    "length":512,
    "type":c_uint8},
    {"name":"crc16",
    "type":c_uint16},
  ]
}

m2m2_file_sys_app_ref_hr_stream_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"refhr",
    "type":c_uint16},
    {"name":"year",
    "type":c_uint16},
    {"name":"month",
    "type":c_uint8},
    {"name":"day",
    "type":c_uint8},
    {"name":"hour",
    "type":c_uint8},
    {"name":"minute",
    "type":c_uint8},
    {"name":"second",
    "type":c_uint8},
    {"name":"TZ_sec",
    "type":c_uint32},
  ]
}
m2m2_file_sys_set_key_value_pair_req_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"valueID",
    "length":16,
    "type":c_uint8},
  ]
}

m2m2_file_sys_set_key_value_pair_resp_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"keyID",
    "type":c_uint16},
    {"name":"valueID",
    "length":16,
    "type":c_uint8},
    {"name":"year",
    "type":c_uint16},
    {"name":"month",
    "type":c_uint8},
    {"name":"day",
    "type":c_uint8},
    {"name":"hour",
    "type":c_uint8},
    {"name":"minute",
    "type":c_uint8},
    {"name":"second",
    "type":c_uint8},
    {"name":"TZ_sec",
    "type":c_uint32},
  ]
}

m2m2_file_sys_pattern_write_req_pkt_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"file_size",
    "type":c_uint32},
    {"name":"scale_type",
    "type":c_uint8},
    {"name":"scale_factor",
    "type":c_uint16},
    {"name":"num_files_to_write",
    "type":c_uint16},
  ]
}

m2m2_file_sys_pattern_write_resp_pkt_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
  ]
}

m2m2_file_sys_vol_info_resp_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"totalmemory",
    "type":c_uint32},
    {"name":"usedmemory",
    "type":c_uint32},
    {"name":"availmemory",
    "type":c_uint16},
  ]
}

m2m2_file_sys_get_subs_status_resp_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"stream",
    "type":m2m2_core.M2M2_ADDR_ENUM_t},
    {"name":"subs_state",
    "type":FILE_SYS_STREAM_SUBS_STATE_ENUM_t},
  ]
}

m2m2_file_sys_debug_info_req_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"stream",
    "type":m2m2_core.M2M2_ADDR_ENUM_t},
  ]
}

m2m2_file_sys_debug_info_resp_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"stream",
    "type":m2m2_core.M2M2_ADDR_ENUM_t},
    {"name":"packets_received",
    "type":c_uint32},
    {"name":"packets_missed",
    "type":c_uint32},
  ]
}

m2m2_file_sys_impt_debug_info_req_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
  ]
}

m2m2_file_sys_impt_debug_info_resp_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"head_pointer",
    "type":c_uint32},
    {"name":"tail_pointer",
    "type":c_uint32},
    {"name":"usb_avg_tx_time",
    "type":c_uint32},
    {"name":"usb_avg_port_write_time",
    "type":c_uint32},
    {"name":"page_read_time",
    "type":c_uint32},
    {"name":"page_write_time",
    "type":c_uint32},
    {"name":"init_circular_buffer_flag",
    "type":c_uint16},
    {"name":"mem_full_flag",
    "type":c_uint16},
    {"name":"data_offset",
    "type":c_uint16},
    {"name":"config_file_occupied",
    "type":c_uint16},
  ]
}

m2m2_file_sys_user_config_data = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"len_configstream",
    "type":c_uint16},
    {"name":"byte_configstream",
    "length":80,
    "type":c_uint8},
  ]
}

m2m2_file_sys_user_cfg_summary_pkt_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"start_cmd_len",
    "type":c_ushort},
    {"name":"start_cmd_cnt",
    "type":c_ushort},
    {"name":"stop_cmd_len",
    "type":c_ushort},
    {"name":"stop_cmd_cnt",
    "type":c_ushort},
    {"name":"crc16",
    "type":c_ushort},
  ]
}

m2m2_file_sys_get_file_count_pkt_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"file_count",
    "type":c_ushort},
  ]
}

m2m2_file_sys_get_file_info_req_pkt_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"file_index",
    "type":c_uint8},
  ]
}

m2m2_file_sys_get_file_info_resp_pkt_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"file_name",
    "length":16,
    "type":c_uint8},    
    {"name":"start_page",
    "type":c_uint32},
    {"name":"end_page",
    "type":c_uint32}, 
    {"name":"file_size",
    "type":c_uint32}, 
  ]
}

m2m2_file_sys_page_test_req_pkt_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},
    {"name":"page_num",
    "type":c_uint32},
    {"name":"num_bytes",
    "type":c_uint8}, 
  ]
}

m2m2_file_sys_page_test_resp_pkt_t = {
  "struct_fields": [
    {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},   
    {"name":"page_num",
    "type":c_uint32},
    {"name":"ecc_zone_status",
    "type":c_uint8}, 
    {"name":"next_page",
    "type":c_uint32},  
    {"name":"occupied",
    "type":c_uint8},     
    {"name":"data_region_status",
    "type":c_uint8}, 
    {"name":"sample_data",
    "length":100,
    "type":c_uint8},
    {"name":"num_bytes",
    "type":c_uint8},    
  ]
}

m2m2_file_sys_log_stream_t  = {
  "struct_fields": [
  {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},   
  {"name":"stream",
    "type":m2m2_core.M2M2_ADDR_ENUM_t},    
  ]
}

m2m2_file_sys_stop_log_cmd_t  = {
  "struct_fields": [
  {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},   
  {"name":"stop_type",
    "type":FILE_STOP_LOGGING_t},    
  ]
}

m2m2_file_sys_get_bad_blocks_cmd_t  = {
  "struct_fields": [
  {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},   
  {"name":"bad_blocks",
    "type":c_uint32},    
  ]
}

m2m2_file_sys_write_blocks_t = {
  "struct_fields": [
  {"name":None,
    "type":common_application_interface._m2m2_app_common_cmd_t},   
  {"name":"num_blocks_write",
    "type":c_uint16},    
  {"name":"start_block_num",
    "type":c_uint16},    
  ]
}
