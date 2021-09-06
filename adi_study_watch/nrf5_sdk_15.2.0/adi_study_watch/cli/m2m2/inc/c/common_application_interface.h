// #############################################################################
// THIS IS AN AUTOMATICALLY GENERATED FILE, DO NOT MODIFY IT!
// #############################################################################
#pragma once

#include "dcb_interface.h"
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

#define MAXTXRXDCFGSIZE	57
#define PING_PKT_SZ	230

typedef enum M2M2_APP_COMMON_STATUS_ENUM_t {
  M2M2_APP_COMMON_STATUS_OK = 0,
  M2M2_APP_COMMON_STATUS_ERROR = 1,
  M2M2_APP_COMMON_STATUS_STREAM_STARTED = 2,
  M2M2_APP_COMMON_STATUS_STREAM_STOPPED = 3,
  M2M2_APP_COMMON_STATUS_STREAM_IN_PROGRESS = 4,
  M2M2_APP_COMMON_STATUS_STREAM_DEACTIVATED = 5,
  M2M2_APP_COMMON_STATUS_STREAM_COUNT_DECREMENT = 6,
  M2M2_APP_COMMON_STATUS_STREAM_NOT_STARTED = 7,
  M2M2_APP_COMMON_STATUS_STREAM_NOT_STOPPED = 8,
  M2M2_APP_COMMON_STATUS_SUBSCRIBER_ADDED = 9,
  M2M2_APP_COMMON_STATUS_SUBSCRIBER_REMOVED = 10,
  M2M2_APP_COMMON_STATUS_SUBSCRIBER_COUNT_DECREMENT = 11,
  _M2M2_APP_COMMON_STATUS_ENUM_t__M2M2_APP_COMMON_STATUS_HIGHEST = 32,
} M2M2_APP_COMMON_STATUS_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_APP_COMMON_STATUS_ENUM_t) == 1, INCORRECT_SIZE_M2M2_APP_COMMON_STATUS_ENUM_t);

typedef enum M2M2_APP_COMMON_CMD_ENUM_t {
  M2M2_APP_COMMON_CMD_GET_VERSION_REQ = 0,
  M2M2_APP_COMMON_CMD_GET_VERSION_RESP = 1,
  M2M2_APP_COMMON_CMD_GET_STATUS_REQ = 2,
  M2M2_APP_COMMON_CMD_GET_STATUS_RESP = 3,
  M2M2_APP_COMMON_CMD_STREAM_START_REQ = 4,
  M2M2_APP_COMMON_CMD_STREAM_START_RESP = 5,
  M2M2_APP_COMMON_CMD_STREAM_STOP_REQ = 6,
  M2M2_APP_COMMON_CMD_STREAM_STOP_RESP = 7,
  M2M2_APP_COMMON_CMD_STREAM_PAUSE_REQ = 8,
  M2M2_APP_COMMON_CMD_STREAM_PAUSE_RESP = 9,
  M2M2_APP_COMMON_CMD_STREAM_UNPAUSE_REQ = 10,
  M2M2_APP_COMMON_CMD_STREAM_UNPAUSE_RESP = 11,
  M2M2_APP_COMMON_CMD_STREAM_SUBSCRIBE_REQ = 12,
  M2M2_APP_COMMON_CMD_STREAM_SUBSCRIBE_RESP = 13,
  M2M2_APP_COMMON_CMD_STREAM_UNSUBSCRIBE_REQ = 14,
  M2M2_APP_COMMON_CMD_STREAM_UNSUBSCRIBE_RESP = 15,
  M2M2_APP_COMMON_CMD_SENSOR_STATUS_QUERY_REQ = 16,
  M2M2_APP_COMMON_CMD_SENSOR_STATUS_QUERY_RESP = 17,
  M2M2_APP_COMMON_CMD_GET_LCFG_REQ = 18,
  M2M2_APP_COMMON_CMD_GET_LCFG_RESP = 19,
  M2M2_APP_COMMON_CMD_SET_LCFG_REQ = 20,
  M2M2_APP_COMMON_CMD_SET_LCFG_RESP = 21,
  M2M2_APP_COMMON_CMD_READ_LCFG_REQ = 22,
  M2M2_APP_COMMON_CMD_READ_LCFG_RESP = 23,
  M2M2_APP_COMMON_CMD_WRITE_LCFG_REQ = 24,
  M2M2_APP_COMMON_CMD_WRITE_LCFG_RESP = 25,
  M2M2_APP_COMMON_CMD_PING_REQ = 26,
  M2M2_APP_COMMON_CMD_PING_RESP = 27,
  _M2M2_APP_COMMON_CMD_ENUM_t__M2M2_APP_COMMON_CMD_HIGHEST = 32,
} M2M2_APP_COMMON_CMD_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_APP_COMMON_CMD_ENUM_t) == 1, INCORRECT_SIZE_M2M2_APP_COMMON_CMD_ENUM_t);

typedef struct __m2m2_app_common_cmd_t {
  uint8_t  command; 
  uint8_t  status; 
} _m2m2_app_common_cmd_t;

typedef struct _m2m2_app_common_ver_req_t {
  uint8_t  command; 
  uint8_t  status; 
} m2m2_app_common_ver_req_t;

typedef struct _m2m2_app_common_ping_t {
  uint8_t  command; 
  uint8_t  status; 
  uint32_t  sequence_num; 
  uint8_t  data[230]; 
} m2m2_app_common_ping_t;

typedef struct _m2m2_app_common_version_t {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  major; 
  uint16_t  minor; 
  uint16_t  patch; 
  uint8_t  verstr[10]; 
  uint8_t  str[40]; 
} m2m2_app_common_version_t;

typedef struct _m2m2_app_common_status_t {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  stream; 
  uint8_t  num_subscribers; 
  uint8_t  num_start_reqs; 
} m2m2_app_common_status_t;

typedef struct _m2m2_app_common_sub_op_t {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  stream; 
} m2m2_app_common_sub_op_t;

typedef struct _m2m2_sensor_dcfg_data_t {
  uint8_t  command; 
  uint8_t  status; 
  uint8_t  size; 
  uint8_t  num_tx_pkts; 
  uint32_t  dcfgdata[57]; 
} m2m2_sensor_dcfg_data_t;

typedef struct _m2m2_app_lcfg_data_t {
  uint8_t  command; 
  uint8_t  status; 
  uint8_t  size; 
  uint8_t  lcfgdata[224]; 
} m2m2_app_lcfg_data_t;

typedef struct __m2m2_app_data_stream_hdr_t {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  sequence_num; 
} _m2m2_app_data_stream_hdr_t;

// Reset struct packing outside of this file
#pragma pack()
