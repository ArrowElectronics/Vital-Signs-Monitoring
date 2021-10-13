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

#define DCB_BLK_WORD_SZ	4
#define MAXAD7156DCBSIZE	20
#define MAXADPD4000DCBSIZE	57
#define MAXADXLDCBSIZE	25
#define MAXBIADCBSIZE	18
#define MAXECGDCBSIZE	4
#define MAXEDADCBSIZE	2
#define MAXGENBLKDCBSIZE	57
#define MAXLTAPPLCFGDCBSIZE	5
#define MAXPPGDCBSIZE	56
#define MAXUSER0BLKDCBSIZE	19
#define MAX_ADPD4000_DCB_PKTS	4
#define MAX_GEN_BLK_DCB_PKTS	18

typedef enum M2M2_DCB_COMMAND_ENUM_t {
  _M2M2_DCB_COMMAND_ENUM_t__M2M2_DCB_COMMAND_LOWEST = 150,
  M2M2_DCB_COMMAND_READ_CONFIG_REQ = 151,
  M2M2_DCB_COMMAND_READ_CONFIG_RESP = 152,
  M2M2_DCB_COMMAND_WRITE_CONFIG_REQ = 153,
  M2M2_DCB_COMMAND_WRITE_CONFIG_RESP = 154,
  M2M2_DCB_COMMAND_ERASE_CONFIG_REQ = 155,
  M2M2_DCB_COMMAND_ERASE_CONFIG_RESP = 156,
  M2M2_DCB_COMMAND_QUERY_STATUS_REQ = 157,
  M2M2_DCB_COMMAND_QUERY_STATUS_RESP = 158,
} M2M2_DCB_COMMAND_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_DCB_COMMAND_ENUM_t) == 1, INCORRECT_SIZE_M2M2_DCB_COMMAND_ENUM_t);

typedef enum M2M2_DCB_STATUS_ENUM_t {
  _M2M2_DCB_STATUS_ENUM_t__M2M2_DCB_STATUS_LOWEST = 150,
  M2M2_DCB_STATUS_OK = 151,
  M2M2_DCB_STATUS_ERR_ARGS = 152,
  M2M2_DCB_STATUS_ERR_NOT_CHKD = 255,
} M2M2_DCB_STATUS_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_DCB_STATUS_ENUM_t) == 1, INCORRECT_SIZE_M2M2_DCB_STATUS_ENUM_t);

typedef enum M2M2_DCB_CONFIG_BLOCK_INDEX_t {
  ADI_DCB_GENERAL_BLOCK_IDX = 0,
  ADI_DCB_AD5940_BLOCK_IDX = 1,
  ADI_DCB_ADPD4000_BLOCK_IDX = 2,
  ADI_DCB_ADXL362_BLOCK_IDX = 3,
  ADI_DCB_PPG_BLOCK_IDX = 4,
  ADI_DCB_ECG_BLOCK_IDX = 5,
  ADI_DCB_EDA_BLOCK_IDX = 6,
  ADI_DCB_AD7156_BLOCK_IDX = 7,
  ADI_DCB_PEDOMETER_BLOCK_IDX = 8,
  ADI_DCB_TEMPERATURE_BLOCK_IDX = 9,
  ADI_DCB_LT_APP_LCFG_BLOCK_IDX = 10,
  ADI_DCB_UI_CONFIG_BLOCK_IDX = 11,
  ADI_DCB_USER0_BLOCK_IDX = 12,
  ADI_DCB_USER1_BLOCK_IDX = 13,
  ADI_DCB_USER2_BLOCK_IDX = 14,
  ADI_DCB_USER3_BLOCK_IDX = 15,
  ADI_DCB_BIA_BLOCK_IDX = 16,
  ADI_DCB_MAX_BLOCK_IDX = 17,
} M2M2_DCB_CONFIG_BLOCK_INDEX_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_DCB_CONFIG_BLOCK_INDEX_t) == 1, INCORRECT_SIZE_M2M2_DCB_CONFIG_BLOCK_INDEX_t);

typedef struct _m2m2_dcb_cmd_t {
  uint8_t  command; 
  uint8_t  status; 
} m2m2_dcb_cmd_t;

typedef struct _m2m2_dcb_adpd4000_data_t {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  size; 
  uint16_t  num_of_pkts; 
  uint32_t  dcbdata[57]; 
} m2m2_dcb_adpd4000_data_t;

typedef struct _m2m2_dcb_adxl_data_t {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  size; 
  uint32_t  dcbdata[25]; 
} m2m2_dcb_adxl_data_t;

typedef struct _m2m2_dcb_ppg_data_t {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  size; 
  int32_t  dcbdata[56]; 
} m2m2_dcb_ppg_data_t;

typedef struct _m2m2_dcb_ecg_data_t {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  size; 
  uint32_t  dcbdata[4]; 
} m2m2_dcb_ecg_data_t;

typedef struct _m2m2_dcb_eda_data_t {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  size; 
  uint32_t  dcbdata[2]; 
} m2m2_dcb_eda_data_t;

typedef struct _m2m2_dcb_bia_data_t {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  size; 
  uint32_t  dcbdata[18]; 
} m2m2_dcb_bia_data_t;

typedef struct _m2m2_dcb_gen_blk_data_t {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  size; 
  uint16_t  num_of_pkts; 
  uint32_t  dcbdata[57]; 
} m2m2_dcb_gen_blk_data_t;

typedef struct _m2m2_dcb_gen_blk_user_cfg_summary_pkt_t {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  start_cmd_len; 
  uint16_t  start_cmd_cnt; 
  uint16_t  stop_cmd_len; 
  uint16_t  stop_cmd_cnt; 
  uint16_t  crc16; 
} m2m2_dcb_gen_blk_user_cfg_summary_pkt_t;

typedef struct _m2m2_dcb_ad7156_data_t {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  size; 
  uint32_t  dcbdata[20]; 
} m2m2_dcb_ad7156_data_t;

typedef struct _m2m2_dcb_lt_app_lcfg_data_t {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  size; 
  uint32_t  dcbdata[5]; 
} m2m2_dcb_lt_app_lcfg_data_t;

typedef struct _m2m2_dcb_user0_blk_data_t {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  size; 
  uint32_t  dcbdata[19]; 
} m2m2_dcb_user0_blk_data_t;

typedef struct _m2m2_dcb_block_status_t {
  uint8_t  command; 
  uint8_t  status; 
  uint8_t  dcb_blk_array[17]; 
} m2m2_dcb_block_status_t;

// Reset struct packing outside of this file
#pragma pack()
