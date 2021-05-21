// #############################################################################
// THIS IS AN AUTOMATICALLY GENERATED FILE, DO NOT MODIFY IT!
// #############################################################################
#pragma once

//#include "common_application_interface.hpp"
#include "m2m2_core.hpp"
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

#define MAXADPD4000DCBSIZE    (58) //Max size of adpd4000 DCB size in double word length; 58 uint32_t elements in dcfg
#define MAXADXLDCBSIZE        (25) //Max size of adxl DCB size in double word length; 25 uint32_t elements in dcfg
#define MAXPPGDCBSIZE        (53)
#define MAXECGDCBSIZE         (2)
#define MAXEDADCBSIZE         (2)
#define MAXGENBLKDCBSIZE      (58) //Max size of general Block DCB size in double word length; 50 uint32_t elements in dcfg

enum M2M2_DCB_COMMAND_ENUM_t:uint8_t {
    __M2M2_DCB_COMMAND_LOWEST = 150,
    M2M2_DCB_COMMAND_READ_CONFIG_REQ = 151,
    M2M2_DCB_COMMAND_READ_CONFIG_RESP = 152,
    M2M2_DCB_COMMAND_WRITE_CONFIG_REQ = 153,
    M2M2_DCB_COMMAND_WRITE_CONFIG_RESP = 154,
    M2M2_DCB_COMMAND_ERASE_CONFIG_REQ = 155,
    M2M2_DCB_COMMAND_ERASE_CONFIG_RESP = 156,
} M2M2_DCB_COMMAND_ENUM_t;
static_assert(sizeof(M2M2_DCB_COMMAND_ENUM_t) == 1, "Enum 'M2M2_DCB_COMMAND_ENUM_t' has an incorrect size!");

enum M2M2_DCB_STATUS_ENUM_t:uint8_t {
  __M2M2_DCB_STATUS_LOWEST = 150,
  M2M2_DCB_STATUS_OK = 151,
  M2M2_DCB_STATUS_ERR_ARGS = 152,
  M2M2_DCB_STATUS_ERR_NOT_CHKD = 255,
} M2M2_DCB_STATUS_ENUM_t;
static_assert(sizeof(M2M2_DCB_STATUS_ENUM_t) == 1, "Enum 'M2M2_DCB_STATUS_ENUM_t' has an incorrect size!");

struct m2m2_file_sys_cmd_t {
  uint8_t  command; 
  uint8_t  status; 
};

struct m2m2_dcb_cmd_t {
  uint8_t  command; 
  uint8_t  status; 
} ;

struct m2m2_dcb_adpd4000_data_t {
    uint8_t  command; 
    uint8_t  status;
    uint16_t  size;
    uint32_t  dcbdata[MAXADPD4000DCBSIZE];
};

struct m2m2_dcb_adxl_data_t {
    uint8_t  command; 
    uint8_t  status;
    uint16_t  size; 
    uint32_t  dcbdata[MAXADXLDCBSIZE]; 
};


struct m2m2_dcb_ppg_data_t {
    uint8_t  command; 
    uint8_t  status;
    uint16_t  size; 
    int32_t dcbdata[MAXPPGDCBSIZE]; 
};

struct m2m2_dcb_ecg_data_t {
    uint8_t  command; 
    uint8_t  status;
    uint16_t  size; 
    uint32_t dcbdata[MAXECGDCBSIZE]; 
};

struct m2m2_dcb_eda_data_t {
    uint8_t  command; 
    uint8_t  status;
    uint16_t  size; 
    uint32_t dcbdata[MAXEDADCBSIZE]; 
};

struct m2m2_dcb_gen_blk_data_t {
    uint8_t  command; 
    uint8_t  status;
    uint16_t  size; 
    uint32_t dcbdata[MAXGENBLKDCBSIZE]; 
};

struct m2m2_dcb_gen_blk_user_cfg_summary_pkt_t {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  start_cmd_len; 
  uint16_t  start_cmd_cnt; 
  uint16_t  stop_cmd_len; 
  uint16_t  stop_cmd_cnt; 
  uint16_t  crc16; 
};

// Reset struct packing outside of this file
#pragma pack()