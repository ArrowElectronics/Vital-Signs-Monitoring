// #############################################################################
// THIS IS AN AUTOMATICALLY GENERATED FILE, DO NOT MODIFY IT!
// #############################################################################
#pragma once

//#include "common_application_interface.h"
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

#define MAXADPD4000DCBSIZE    (57) //Max size of adpd4000 DCB size in double word length; 57 uint32_t elements in dcfg
#define MAXADXLDCBSIZE        (25) //Max size of adxl DCB size in double word length; 25 uint32_t elements in dcfg
#define MAXPPGDCBSIZE        (53)
#define MAXECGDCBSIZE         (2)
#define MAXEDADCBSIZE         (2)
#define MAXBCMDCBSIZE         (2)
#define MAXGENBLKDCBSIZE      (57) //Max size of general Block DCB size in double word length; 57 uint32_t elements in dcfg
#define MAXAD7156DCBSIZE      (20) //Max size of AD7156 DCB in double word length; 20 uint32_t elements in dcfg
#define MAXWRISTDETECTDCBSIZE (4)  //Max size of wrist detect DCB in double word length; 4 uint32_t elements in lcfg

typedef enum M2M2_DCB_COMMAND_ENUM_t {
    __M2M2_DCB_COMMAND_LOWEST = 150,
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
  __M2M2_DCB_STATUS_LOWEST = 150,
  M2M2_DCB_STATUS_OK = 151,
  M2M2_DCB_STATUS_ERR_ARGS = 152,
  M2M2_DCB_STATUS_ERR_NOT_CHKD = 255,
} M2M2_DCB_STATUS_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_DCB_STATUS_ENUM_t) == 1, INCORRECT_SIZE_M2M2_DCB_STATUS_ENUM_t);

typedef struct _m2m2_dcb_cmd_t {
  uint8_t  command;
  uint8_t  status;
} m2m2_dcb_cmd_t;

typedef struct _m2m2_dcb_adpd4000_data_t {
    uint8_t  command;
    uint8_t  status;
    uint16_t  size; //Size of dcbdata in each packet
    uint16_t num_of_pkts; //No: of packets in total that will be sent
    uint32_t  dcbdata[MAXADPD4000DCBSIZE];
} m2m2_dcb_adpd4000_data_t;

typedef struct _m2m2_dcb_adxl_data_t {
    uint8_t  command;
    uint8_t  status;
    uint16_t  size;
    uint32_t  dcbdata[MAXADXLDCBSIZE];
} m2m2_dcb_adxl_data_t;


typedef struct _m2m2_dcb_ppg_data_t {
    uint8_t  command;
    uint8_t  status;
    uint16_t  size;
    int32_t dcbdata[MAXPPGDCBSIZE];
} m2m2_dcb_ppg_data_t;

typedef struct _m2m2_dcb_ecg_data_t {
    uint8_t  command;
    uint8_t  status;
    uint16_t  size;
    uint32_t dcbdata[MAXECGDCBSIZE];
} m2m2_dcb_ecg_data_t;

typedef struct _m2m2_dcb_eda_data_t {
    uint8_t  command;
    uint8_t  status;
    uint16_t  size;
    uint32_t dcbdata[MAXEDADCBSIZE];
} m2m2_dcb_eda_data_t;

typedef struct _m2m2_dcb_bcm_data_t {
    uint8_t  command;
    uint8_t  status;
    uint16_t  size;
    uint32_t dcbdata[MAXBCMDCBSIZE];
} m2m2_dcb_bcm_data_t;

typedef struct _m2m2_dcb_gen_blk_data_t {
    uint8_t  command;
    uint8_t  status;
    uint16_t  size; //Size of dcbdata in each packet
    uint16_t num_of_pkts; //No: of packets in total that will be sent
    uint32_t dcbdata[MAXGENBLKDCBSIZE];
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
    uint32_t dcbdata[MAXAD7156DCBSIZE];
} m2m2_dcb_ad7156_data_t;

typedef struct _m2m2_dcb_wrist_detect_data_t {
    uint8_t  command;
    uint8_t  status;
    uint16_t  size;
    uint32_t dcbdata[MAXWRISTDETECTDCBSIZE];
} m2m2_dcb_wrist_detect_data_t;

typedef enum M2M2_DCB_CONFIG_BLOCK_INDEX_t
{
    ADI_DCB_GENERAL_BLOCK_IDX = 0,
    ADI_DCB_AD5940_BLOCK_IDX,
    ADI_DCB_ADPD4000_BLOCK_IDX,
    ADI_DCB_ADXL362_BLOCK_IDX,
    ADI_DCB_PPG_BLOCK_IDX,
    ADI_DCB_ECG_BLOCK_IDX,
    ADI_DCB_EDA_BLOCK_IDX,
    ADI_DCB_AD7156_BLOCK_IDX,
    ADI_DCB_PEDOMETER_BLOCK_IDX,
    ADI_DCB_TEMPERATURE_BLOCK_IDX,
    ADI_DCB_WRIST_DETECT_BLOCK_IDX,
    ADI_DCB_UI_CONFIG_BLOCK_IDX,
    ADI_DCB_USER0_BLOCK_IDX,
    ADI_DCB_USER1_BLOCK_IDX,
    ADI_DCB_USER2_BLOCK_IDX,
    ADI_DCB_USER3_BLOCK_IDX,
    ADI_DCB_BCM_BLOCK_IDX,
    ADI_DCB_MAX_BLOCK_IDX
} M2M2_DCB_CONFIG_BLOCK_INDEX_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_DCB_CONFIG_BLOCK_INDEX_t) == 1, INCORRECT_SIZE_M2M2_DCB_CONFIG_BLOCK_INDEX_t);

typedef struct _m2m2_dcb_block_status_t {
    uint8_t  command;
    uint8_t  status;
    uint8_t  dcb_blk_array[ADI_DCB_MAX_BLOCK_IDX];
} m2m2_dcb_block_status_t;

// Reset struct packing outside of this file
#pragma pack()
