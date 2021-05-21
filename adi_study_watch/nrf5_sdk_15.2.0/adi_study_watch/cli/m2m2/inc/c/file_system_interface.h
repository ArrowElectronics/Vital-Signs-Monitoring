// #############################################################################
// THIS IS AN AUTOMATICALLY GENERATED FILE, DO NOT MODIFY IT!
// #############################################################################
#pragma once

#include "common_application_interface.h"
#include "m2m2_core.h"
#include <stdint.h>


/* Explicitly enforce struct packing so that the nested structs and unions are laid out
    as expected. */
#if defined __CC_ARM || defined __IAR_SYSTEMS_ICC__ || __clang__ || defined _MSC_VER || defined __GNUC__
// DELIBERATELY BLANK
#else
#error "WARNING! Your compiler might not support '#pragma pack(1)'! \
  You must add an equivalent compiler directive to the file generator!"
#endif  // defined __CC_ARM || defined __IAR_SYSTEMS_ICC__ || __clang__ || defined _MSC_VER || defined __GNUC__
#pragma pack(1)

#ifndef STATIC_ASSERT_PROJ
#define STATIC_ASSERT_PROJ(COND, MSG) typedef char static_assertion_##MSG[(COND)?1:-1]
#endif // STATIC_ASSERT_PROJ

typedef enum FILE_STOP_LOGGING_t {
  M2M2_FILE_SYS_STOP_LOGGING = 0,
  M2M2_FILE_SYS_MEMORY_FULL = 1,
  M2M2_FILE_SYS_BATTERY_LOW = 2,
  M2M2_FILE_SYS_POWER_STATE_SHUTDOWN = 3,
  M2M2_FILE_SYS_STOP_LOGGING_INVALID = 255,
} FILE_STOP_LOGGING_t;
STATIC_ASSERT_PROJ(sizeof(FILE_STOP_LOGGING_t) == 1, INCORRECT_SIZE_FILE_STOP_LOGGING_t);

typedef enum M2M2_FILE_SYS_CMD_ENUM_t {
  __M2M2_FILE_SYS_CMD_LOWEST = 64,
  M2M2_FILE_SYS_CMD_MOUNT_REQ = 66,
  M2M2_FILE_SYS_CMD_MOUNT_RESP = 67,
  M2M2_FILE_SYS_CMD_FORMAT_REQ = 70,
  M2M2_FILE_SYS_CMD_FORMAT_RESP = 71,
  M2M2_FILE_SYS_CMD_LS_REQ = 72,
  M2M2_FILE_SYS_CMD_LS_RESP = 73,
  M2M2_FILE_SYS_CMD_GET_REQ = 76,
  M2M2_FILE_SYS_CMD_GET_RESP = 77,
  M2M2_FILE_SYS_CMD_VOL_INFO_REQ = 78,
  M2M2_FILE_SYS_CMD_VOL_INFO_RESP = 79,
  M2M2_FILE_SYS_CMD_GET_FS_STATUS_REQ = 80,
  M2M2_FILE_SYS_CMD_GET_FS_STATUS_RESP = 81,
  M2M2_FILE_SYS_CMD_GET_FS_STREAM_SUB_STATUS_REQ = 82,
  M2M2_FILE_SYS_CMD_GET_FS_STREAM_SUB_STATUS_RESP = 83,
  M2M2_FILE_SYS_CMD_STOP_FS_STREAM_REQ = 84,
  M2M2_FILE_SYS_CMD_STOP_FS_STREAM_RESP = 85,
  M2M2_FILE_SYS_CMD_GET_FS_DEBUG_INFO_REQ = 86,
  M2M2_FILE_SYS_CMD_GET_FS_DEBUG_INFO_RESP = 87,
  M2M2_FILE_SYS_CMD_TEST_LOG_REQ = 88,
  M2M2_FILE_SYS_CMD_TEST_LOG_RESP = 89,
  M2M2_FILE_SYS_CMD_FORCE_STOP_LOG_REQ = 90,
  M2M2_FILE_SYS_CMD_FORCE_STOP_LOG_RESP = 91,
  M2M2_FILE_SYS_CMD_SET_KEY_VALUE_PAIR_REQ = 100,
  M2M2_FILE_SYS_CMD_SET_KEY_VALUE_PAIR_RESP = 101,
  M2M2_FILE_SYS_CMD_DCFG_START_LOG_REQ = 102,
  M2M2_FILE_SYS_CMD_DCFG_START_LOG_RESP = 103,
  M2M2_FILE_SYS_CMD_DCFG_STOP_LOG_REQ = 104,
  M2M2_FILE_SYS_CMD_DCFG_STOP_LOG_RESP = 105,
  M2M2_FILE_SYS_CMD_LOG_USER_CONFIG_DATA_REQ = 106,
  M2M2_FILE_SYS_CMD_LOG_USER_CONFIG_DATA_RESP = 107,
  M2M2_FILE_SYS_CMD_FIND_CONFIG_FILE_REQ = 108,
  M2M2_FILE_SYS_CMD_FIND_CONFIG_FILE_RESP = 109,
  M2M2_FILE_SYS_CMD_DELETE_CONFIG_FILE_REQ = 110,
  M2M2_FILE_SYS_CMD_DELETE_CONFIG_FILE_RESP = 111,
  M2M2_FILE_SYS_CMD_GET_NUMBER_OF_FILE_REQ = 112,
  M2M2_FILE_SYS_CMD_GET_NUMBER_OF_FILE_RESP = 113,
  M2M2_FILE_SYS_CMD_LOG_STREAM_REQ = 114,
  M2M2_FILE_SYS_CMD_LOG_STREAM_RESP = 115,
  M2M2_FILE_SYS_CMD_STOP_STREAM_REQ = 116,
  M2M2_FILE_SYS_CMD_STOP_STREAM_RESP = 117,
  M2M2_FILE_SYS_CMD_START_LOGGING_REQ = 118,
  M2M2_FILE_SYS_CMD_START_LOGGING_RESP = 119,
  M2M2_FILE_SYS_CMD_STOP_LOGGING_REQ = 120,
  M2M2_FILE_SYS_CMD_STOP_LOGGING_RESP = 121,
  M2M2_FILE_SYS_CMD_DOWNLOAD_LOG_REQ = 122,
  M2M2_FILE_SYS_CMD_DOWNLOAD_LOG_RESP = 123,
  M2M2_FILE_SYS_CMD_CANCEL_DOWNLOAD_LOG_REQ = 124,
  M2M2_FILE_SYS_CMD_CANCEL_DOWNLOAD_LOG_RESP = 125,
  M2M2_FILE_SYS_CMD_GET_BAD_BLOCKS_REQ = 126,
  M2M2_FILE_SYS_CMD_GET_BAD_BLOCKS_RESP = 127,
  M2M2_FILE_SYS_CMD_FLASH_RESET_REQ = 128,
  M2M2_FILE_SYS_CMD_FLASH_RESET_RESP = 129,
  M2M2_FILE_SYS_CMD_DISPLAY_VOL_INFO_REQ = 130,
  M2M2_FILE_SYS_CMD_DISPLAY_VOL_INFO_RESP = 131,
  M2M2_FILE_SYS_CMD_CHUNK_RETRANSMIT_REQ = 132,
  M2M2_FILE_SYS_CMD_CHUNK_RETRANSMIT_RESP = 133,
  M2M2_FILE_SYS_BLOCKS_WRITE_REQ = 134,
  M2M2_FILE_SYS_BLOCKS_WRITE_RESP = 135,
  M2M2_FILE_SYS_CMD_GET_IMPT_DEBUG_INFO_REQ = 136,
  M2M2_FILE_SYS_CMD_GET_IMPT_DEBUG_INFO_RESP = 137,
  M2M2_FILE_SYS_CMD_PATTERN_WRITE_REQ = 138,
  M2M2_FILE_SYS_CMD_PATTERN_WRITE_RESP = 139,
  M2M2_FILE_SYS_CMD_GET_FILE_INFO_REQ = 140,
  M2M2_FILE_SYS_CMD_GET_FILE_INFO_RESP = 141,
  M2M2_FILE_SYS_CMD_PAGE_READ_TEST_REQ = 142,
  M2M2_FILE_SYS_CMD_PAGE_READ_TEST_RESP = 143,
} M2M2_FILE_SYS_CMD_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_FILE_SYS_CMD_ENUM_t) == 1, INCORRECT_SIZE_M2M2_FILE_SYS_CMD_ENUM_t);

typedef enum M2M2_FILE_SYS_STATUS_ENUM_t {
  __M2M2_FILE_SYS_ERR_LOWEST = 64,
  M2M2_FILE_SYS_STATUS_OK = 65,
  M2M2_FILE_SYS_STATUS_ERROR = 66,
  M2M2_FILE_SYS_END_OF_FILE = 67,
  M2M2_FILE_SYS_END_OF_DIR = 68,
  M2M2_FILE_SYS_ERR_INVALID = 69,
  M2M2_FILE_SYS_ERR_ARGS = 70,
  M2M2_FILE_SYS_ERR_FORMAT = 71,
  M2M2_FILE_SYS_ERR_MEMORY_FULL = 72,
  M2M2_FILE_SYS_ERR_LOG_FORCE_STOPPED = 73,
  M2M2_FILE_SYS_ERR_MAX_FILE_COUNT = 74,
  M2M2_FILE_SYS_CONFIG_FILE_FOUND = 75,
  M2M2_FILE_SYS_CONFIG_FILE_NOT_FOUND = 76,
  M2M2_FILE_SYS_STATUS_LOGGING_STOPPED = 77,
  M2M2_FILE_SYS_STATUS_LOGGING_IN_PROGRESS = 78,
  M2M2_FILE_SYS_STATUS_LOGGING_ERROR = 79,
  M2M2_FILE_SYS_STATUS_LOGGING_NOT_STARTED = 80,
  M2M2_FILE_SYS_ERR_BATTERY_LOW = 81,
  M2M2_FILE_SYS_ERR_POWER_STATE_SHUTDOWN = 82,
  M2M2_FILE_SYS_ERR_CONFIG_FILE_POSITION = 83,
  M2M2_FILE_SYS_STATUS_BLOCKS_WRITE_ERROR = 84,
  M2M2_FILE_SYS_ERR_NOT_CHKD = 255,
} M2M2_FILE_SYS_STATUS_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_FILE_SYS_STATUS_ENUM_t) == 1, INCORRECT_SIZE_M2M2_FILE_SYS_STATUS_ENUM_t);

typedef enum FILE_TYPE_ENUM_t {
  M2M2_FILE_SYS_IS_CONFIG_FILE = 0,
  M2M2_FILE_SYS_IS_DATA_FILE = 1,
  M2M2_FILE_SYS_INVALID_TYPE = 2,
} FILE_TYPE_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(FILE_TYPE_ENUM_t) == 1, INCORRECT_SIZE_FILE_TYPE_ENUM_t);

typedef enum FILE_SYS_STREAM_SUBS_STATE_ENUM_t {
  M2M2_FILE_SYS_UNSUBSCRIBED = 0,
  M2M2_FILE_SYS_SUBSCRIBED = 1,
  M2M2_FILE_SYS_SUBS_INVALID = 255,
} FILE_SYS_STREAM_SUBS_STATE_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(FILE_SYS_STREAM_SUBS_STATE_ENUM_t) == 1, INCORRECT_SIZE_FILE_SYS_STREAM_SUBS_STATE_ENUM_t);

typedef struct _m2m2_file_sys_cmd_t {
  uint8_t  command;
  uint8_t  status;
} m2m2_file_sys_cmd_t;

typedef struct _m2m2_file_sys_ls_req_t {
  uint8_t  command;
  uint8_t  status;
  uint8_t  dir_path[0];
} m2m2_file_sys_ls_req_t;

typedef struct _m2m2_file_sys_get_resp_t {
  uint8_t  command;
  uint8_t  status;
  uint16_t  len_stream;
  uint8_t  byte_stream[512];
  uint16_t  crc16;
} m2m2_file_sys_get_resp_t;

typedef struct _m2m2_file_sys_pkt_retransmit_req_t {
  uint8_t  command;
  uint8_t  status;
  uint8_t  Roll_over;
  uint16_t  chunk_number;
  uint8_t  file_name[0];
} m2m2_file_sys_pkt_retransmit_req_t;


typedef struct _m2m2_file_sys_ls_resp_t {
  uint8_t  command;
  uint8_t  status;
  uint8_t  full_file_name[40];
  FILE_TYPE_ENUM_t  filetype;
  uint32_t  filesize;
} m2m2_file_sys_ls_resp_t;

typedef struct _m2m2_file_sys_get_req_t {
  uint8_t  command;
  uint8_t  status;
  uint8_t  file_name[0];
} m2m2_file_sys_get_req_t;

typedef struct _m2m2_file_sys_download_log_stream_t {
  uint8_t  command;
  uint8_t  status;
  uint16_t  len_stream;
  uint8_t  byte_stream[512];
  //uint8_t  byte_stream[32];
  uint16_t  crc16;
} m2m2_file_sys_download_log_stream_t;

typedef struct _m2m2_file_sys_app_ref_hr_stream_t {
  uint8_t  command;
  uint8_t  status;
  uint16_t  refhr;
  uint16_t  year;
  uint8_t  month;
  uint8_t  day;
  uint8_t  hour;
  uint8_t  minute;
  uint8_t  second;
  uint32_t  TZ_sec;
} m2m2_file_sys_app_ref_hr_stream_t;

typedef struct _m2m2_file_sys_set_key_value_pair_req_t {
  uint8_t  command;
  uint8_t  status;
  uint8_t  valueID[16];
} m2m2_file_sys_set_key_value_pair_req_t;

typedef struct _m2m2_file_sys_set_key_value_pair_resp_t {
  uint8_t  command;
  uint8_t  status;
  uint16_t  keyID;
  uint8_t  valueID[16];
  uint16_t  year;
  uint8_t  month;
  uint8_t  day;
  uint8_t  hour;
  uint8_t  minute;
  uint8_t  second;
  uint32_t  TZ_sec;
} m2m2_file_sys_set_key_value_pair_resp_t;

typedef struct _m2m2_file_sys_pattern_write_req_pkt_t{
 uint8_t command;
 uint8_t status;
 uint32_t file_size;
 uint8_t scale_type;//linear -> 0, log -> 1, exp -> 2
 uint16_t scale_factor;//scale factor 2,3
 uint16_t num_files_to_write;
}m2m2_file_sys_pattern_write_req_pkt_t;

typedef struct _m2m2_file_sys_pattern_write_resp_pkt_t{
 uint8_t command;
 uint8_t status;
}m2m2_file_sys_pattern_write_resp_pkt_t;

typedef struct _m2m2_file_sys_vol_info_resp_t {
  uint8_t  command;
  uint8_t  status;
  uint32_t  totalmemory;
  uint32_t  usedmemory;
  uint16_t  availmemory;
} m2m2_file_sys_vol_info_resp_t;

typedef struct _m2m2_file_sys_get_subs_status_resp_t {
  uint8_t  command;
  uint8_t  status;
  M2M2_ADDR_ENUM_t  stream;
  FILE_SYS_STREAM_SUBS_STATE_ENUM_t  subs_state;
} m2m2_file_sys_get_subs_status_resp_t;

typedef struct _m2m2_file_sys_debug_info_req_t {
  uint8_t  command;
  uint8_t  status;
  M2M2_ADDR_ENUM_t  stream;
} m2m2_file_sys_debug_info_req_t;

typedef struct _m2m2_file_sys_debug_info_resp_t {
  uint8_t  command;
  uint8_t  status;
  M2M2_ADDR_ENUM_t  stream;
  uint32_t  packets_received;
  uint32_t  packets_missed;
  uint32_t  last_page_read;
  uint32_t  last_page_read_offset;
  uint8_t 	last_page_read_status;
  uint32_t num_bytes_transferred;
  uint32_t bytes_read;
  uint8_t usb_cdc_write_failed;
} m2m2_file_sys_debug_info_resp_t;

typedef struct _m2m2_file_sys_impt_debug_info_req_t {
  uint8_t  command;
  uint8_t  status;
} m2m2_file_sys_impt_debug_info_req_t;

typedef struct _m2m2_file_sys_impt_debug_info_resp_t {
  uint8_t  command;
  uint8_t  status;
  uint32_t head_pointer;
  uint32_t tail_pointer;
  uint32_t usb_avg_tx_time;
  uint32_t usb_avg_port_write_time;
  uint32_t page_read_time;
  uint16_t init_circular_buffer_flag;
  uint16_t mem_full_flag;
  uint16_t data_offset;
  uint16_t config_file_occupied;
  uint32_t page_write_time;
} m2m2_file_sys_debug_impt_info_resp_t;

typedef struct _m2m2_file_sys_user_config_data {
  uint8_t  command;
  uint8_t  status;
  uint16_t  len_configstream;
  uint8_t  byte_configstream[70];
} m2m2_file_sys_user_config_data;

typedef struct _m2m2_file_sys_user_cfg_summary_pkt_t {
  uint8_t  command;
  uint8_t  status;
  uint16_t  start_cmd_len;
  uint16_t  start_cmd_cnt;
  uint16_t  stop_cmd_len;
  uint16_t  stop_cmd_cnt;
  uint16_t  crc16;
} m2m2_file_sys_user_cfg_summary_pkt_t;

typedef struct _m2m2_file_sys_get_file_count_pkt_t {
  uint8_t  command;
  uint8_t  status;
  uint16_t  file_count;
} m2m2_file_sys_get_file_count_pkt_t;

typedef struct _m2m2_file_sys_get_file_info_req_pkt_t {
  uint8_t  command;
  uint8_t  status;
  uint8_t  file_index;
} m2m2_file_sys_get_file_info_req_pkt_t;

typedef struct _m2m2_file_sys_get_file_info_resp_pkt_t {
  uint8_t  command;
  uint8_t  status;
  uint8_t  file_name[16];
  uint32_t start_page;
  uint32_t end_page;
  uint32_t file_size;
} m2m2_file_sys_get_file_info_resp_pkt_t;

typedef struct _m2m2_file_sys_page_test_req_pkt_t {
  uint8_t  command;
  uint8_t  status;
  uint32_t  page_num;
  uint8_t  num_bytes;
} m2m2_file_sys_page_read_test_req_pkt_t;

typedef struct _m2m2_file_sys_page_test_resp_pkt_t {
  uint8_t  command;
  uint8_t  status;
  uint32_t page_num;
  uint8_t  ecc_zone_status;
  uint32_t next_page;
  uint8_t occupied;
  uint8_t  data_region_status;
  uint8_t sample_data[100];
  uint8_t num_bytes;
} m2m2_file_sys_page_read_test_resp_pkt_t;

typedef struct _m2m2_file_sys_log_stream_t {
  uint8_t  command;
  uint8_t  status;
  M2M2_ADDR_ENUM_t  stream;
} m2m2_file_sys_log_stream_t;

typedef struct _m2m2_file_sys_stop_log_cmd_t {
  uint8_t  command;
  uint8_t  status;
  FILE_STOP_LOGGING_t  stop_type;
} m2m2_file_sys_stop_log_cmd_t;

typedef struct _m2m2_file_sys_get_bad_blocks_cmd_t {
  uint8_t  command;
  uint8_t  status;
  uint32_t bad_blocks;
} m2m2_file_sys_get_bad_blocks_cmd_t;

typedef struct _m2m2_file_sys_write_blocks_t {
  uint8_t  command;
  uint8_t  status;
  uint16_t  num_blocks_write;
  uint16_t  start_block_num;
} m2m2_file_sys_write_blocks_t;

// Reset struct packing outside of this file
#pragma pack()
