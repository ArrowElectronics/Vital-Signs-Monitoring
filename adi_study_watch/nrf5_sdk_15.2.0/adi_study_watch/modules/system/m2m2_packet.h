/*!
 * Copyright 2016 Analog Devices Inc.
 *******************************************************************************
 * @file:    m2m2_packet.h
 * @brief:   The specification of the m2m_packet protocol
 *------------------------------------------------------------------------------
 *
 This file contains all of the structs, typedefs, and checks needed to implement
 the m2m_packet protocol. Structures and enumerations that are associated with
 a particular command are located together with the command name commented
 nearby.
 *
 ******************************************************************************/
#ifndef M2M_PACKET_H
#define M2M_PACKET_H
#include "stdint.h"
#define M2M_PACKET_MAX_CMD_NUM  0x00FF  // The top 8 bits are reserved for now.

#define STATIC_ASSERT_PROJ(COND, MSG) typedef char static_assertion_##MSG[(COND)?1:-1]
#define MEMBER_SIZE(TYPE, MEMBER) (sizeof(((TYPE*)0)->MEMBER))

/* Enforce struct packing so that the nested structs and unions are laid out
    as expected. */
#if defined __CC_ARM || defined __IAR_SYSTEMS_ICC__ || defined __SES_ARM
#pragma pack(push,1)
#else
#error "WARNING! Your compiler might not support '#pragma pack(1)'! \
  You must add an equivalent compiler directive to pack structs in this file!"
#endif  // defined __CC_ARM || defined __IAR_SYSTEMS_ICC__
/*!
  *@brief       The enumeration of all valid commands.
  *
  * This enumeration defines the list of all valid commands supported by the
  * protocol. A command that is intended to be sent unsolicited shall have an
  * even command number, while responses to a particular command N shall be
  * given a command number N+1.
 */
typedef enum {
  M2M_CMD_ERROR               = 0x00, /**< Generic error code, recognized at the protocol level.*/
  M2M_CMD_ERROR_RESP          = 0x01, /**< Response to an error code, implementation specific.*/
  M2M_CMD_DATA_CTRL_REQ       = 0x02, /**< Request the device start sending data.*/
  M2M_CMD_DATA_CTRL_RESP      = 0x03, /**< Data streamed in response to a M2M_CMD_DATA_CTRL_REQ.*/
  M2M_CMD_DATA_ENCR_CTRL_REQ  = 0x04,
  M2M_CMD_DATA_ENCR_CTRL_RESP = 0x05,
  M2M_CMD_BLE_CTRL_REQ        = 0x06, /**< Request a BLE configuration change.*/
  M2M_CMD_BLE_CTRL_RESP       = 0x07, /**< Report the BLE configuration status.*/
  M2M_CMD_CFG_LOAD_REQ        = 0x08, /**< Load a default config.*/
  M2M_CMD_CFG_LOAD_RESP       = 0x09,
  M2M_CMD_CLK_CAL_REQ         = 0x0A, /**< Request a clock calibration be run. No payload.*/
  M2M_CMD_CLK_CAL_RESP        = 0x0B, /**< Completion status of a previous M2M_CMD_CLK_CAL_REQ.*/
  M2M_CMD_LIB_START_STOP_REQ  = 0x0C, /**< Start or stop the HRM library.*/
  M2M_CMD_LIB_START_STOP_RESP = 0x67,
  M2M_CMD_LIB_STATE_REQ       = 0x0E,
  M2M_CMD_LIB_STATE_RESP      = 0x0F, /**< Reports the state of the HRM library.*/
  M2M_CMD_XCFG_ACCESS_REQ     = 0x10, /**< Request a modification to the d/lcfg*/
  M2M_CMD_XCFG_ACCESS_RESP    = 0x11,
  M2M_CMD_DEV_REG_ACCESS_REQ  = 0x12,
  M2M_CMD_DEV_REG_ACCESS_RESP = 0x13,
  M2M_CMD_CFG_SLOT_MODE_REQ   = 0x14,
  M2M_CMD_CFG_SLOT_MODE_RESP  = 0x15,
  M2M_CMD_CFG_SYNC_MODE_REQ   = 0x16,
  M2M_CMD_CFG_SYNC_MODE_RESP  = 0x17,
  M2M_CMD_INFO_REQ            = 0x18,
  M2M_CMD_INFO_RESP           = 0x19,
  M2M_CMD_CALC_REQ            = 0x1A,
  M2M_CMD_CALC_RESP           = 0x1B,
  M2M_CMD_RAW_DATA_REQ        = 0x1C,
  M2M_CMD_RAW_DATA_RESP       = 0x1D,
  M2M_CMD_LOG_CFG_REQ         = 0x1E, /**< Configure logging.*/
  M2M_CMD_LOG_CFG_RESP        = 0x1F,
  M2M_CMD_HIGHEST_IMPL,	              /**< The highest command number.*/
  M2M_CMD_RETURN              = 0xFE,  /**< Return status of a command.*/
  M2M_CMD_MAX_NUM = M2M_PACKET_MAX_CMD_NUM
}M2M_PKT_CMD_t;




// Some common enums or structs that are used by multiple commands.
typedef enum {
  M2M_PKT_READ,
  M2M_PKT_WRITE,
}M2M_PKT_READ_WRITE_ENUM_t;

// #############################################################################
// M2M_CMD_ERROR

/*!
  *@brief       Some error codes to be used when passing packets between functions.
 */
typedef enum {
  M2M_PKT_ERROR_NONE,
  M2M_PKT_ERROR_UNKNOWN,
  M2M_PKT_ERROR_INVALID_CMD,
}M2M_PKT_ERROR_ENUM_t;

/*!
  *@brief       A generic error struct.
  * This structure is used to provide more details application-specific
  * information to go along with a M2M_CMD_ERROR.
 */
typedef struct _m2m_pkt_error_t {
  uint8_t   error_code; /**< Application-specific error code*/
  uint8_t   data[17];   /**< Application-specific error data*/
}m2m_pkt_error_t;

// #############################################################################
// M2M_CMD_DATA_CTRL_REQ
// M2M_CMD_DATA_CTRL_RESP

/*!
  *@brief       Defines the layout of the data for the M2M_CMD_DATA_REQ/M2M_CMD_DATA_RESP
  *             commands.
 */
typedef struct _m2m_pkt_data_t {
  uint8_t       data_type;        /**< Type of data being requested/sent. Type M2M_CMD_DATA_t*/
  uint8_t       rate_div;         /**< The device sends every "rate_div'th" data sample.*/
  uint8_t       tx_burst_size;    /**< Queue this many packets before sending. NOT IMPLEMENTED.*/
  union {
    struct {
      uint32_t  ppg;     /**< PPG heart rate data*/
      uint16_t  accel_x; /**< X-axis accelerometer data*/
      uint16_t  accel_y; /**< Y-axis accelerometer data*/
      uint16_t  accel_z; /**< Z-axis accelerometer data*/
    }PPG;
    struct {
      uint16_t HR;          /**< Heart rate value*/
      uint16_t confidence;  /**< Confidence rating of HR value*/
      uint16_t HR_type;     /**< The provenance of the HR: either spot algorithm or continuous*/
      uint16_t Activity;    /**< Acitivity */
      uint16_t StepCount;   /**< Step count during walking and running*/
      uint16_t RRinterval;  /**< RR Interval */
    }HRM;
  }data;
}m2m_pkt_data_t;

/*!
  *@brief       An enum of possible data payloads to request.
  *
  * Used with the M2M_CMD_DATA_REQ/M2M_CMD_DATA_RESP commands.
 */
typedef enum {
  M2M_PKT_DATA_HRM,        /**< Heart Rate*/
  M2M_PKT_DATA_PPG,        /**< Photo-Plethysmography data*/
}M2M_PKT_CMD_DATA_ENUM_t;

// #############################################################################
// M2M_CMD_BLE_CTRL_REQ
// M2M_CMD_BLE_CTRL_RESP

typedef enum {
  M2M_PKT_BLE_DISCONN = 0,
  M2M_PKT_BLE_CONN
}M2M_PKT_CMD_BLE_STATE_ENUM_t;

/*!
  *@brief       The BLE control/response packet structure.
  *
  * Used to set and report the state of the DA14580 BLE chip.
 */
typedef struct _m2m_pkt_BLE_ctrl_t {
  uint8_t   connection_state; /**< BLE connection state. Type M2M_PKT_CMD_BLE_STATE_ENUM_t*/
  uint8_t   source_cpu;       /**< The CPU that sent the message. Not used. */
  uint8_t   MAC[6];           /**< A 6-byte BLE MAC address to be set. LSB first. */
}m2m_pkt_BLE_ctrl_t;

// #############################################################################
// M2M_CMD_CFG_LOAD_REQ
/*!
  *@brief       The type of config file to load, used with #m2m_pkt_cfg_t
 */
typedef enum {
  M2M_PKT_DCFG,  /**< Device config*/
  M2M_PKT_LCFG, /**< Library config. NOT YET IMPLEMENTED.*/
}M2M_PKT_CFG_ENUM_t;

/*!
  *@brief       Information needed to load a configuration for either the library
  * or the device.
 */
typedef struct _m2m_pkt_cfg_t {
  uint8_t     cfg_type;   /**< Type M2M_PKT_CFG_ENUM_t*/
  uint16_t    device_id;  /**< Device ID to load the config for*/
  uint8_t     sync_mode;  /**< Sync mode (None/SW/HW)*/
}m2m_pkt_cfg_t;


// #############################################################################
// M2M_CMD_CLK_CAL_REQ
// M2M_CMD_CLK_CAL_RESP
/*!
  *@brief       Information needed to perform/achnowledge clock calibration.
 */
typedef struct _m2m_pkt_clk_cal_data_t {
  uint8_t   status; /**< Completion status of the clock calibration*/
}m2m_pkt_clk_cal_data_t;


// #############################################################################
// M2M_CMD_LIB_START_STOP_REQ
// M2M_CMD_LIB_START_STOP_RESP

typedef enum {
  M2M_PKT_LIB_CTRL_STOP,
  M2M_PKT_LIB_CTRL_START,
}M2M_PKT_LIB_CTRL_ENUM_t;

/*!
  *@brief       Controls the HRM library.
 */
typedef struct _m2m_pkt_lib_ctrl_t {
  uint8_t start_stop; /**< Has type M2M_PKT_LIB_CTRL_ENUM_t*/
}m2m_pkt_lib_ctrl_t;

// #############################################################################
// M2M_CMD_LIB_STATE_REQ
// M2M_CMD_LIB_STATE_RESP

/*!
  *@brief       Notifies of a library state change
 */
typedef struct _m2m_pkt_lib_state_data_t {
  uint8_t new_state;  /**< The new library state*/
  uint8_t old_state;  /**< The old library state*/
  uint8_t data[6];    /**< Application-specific state debug information*/
}m2m_pkt_lib_state_data_t;

// #############################################################################
// M2M_CMD_XCFG_ACCESS_REQ
// M2M_CMD_XCFG_ACCESS_RESP
typedef struct _m2m_pkt_xcfg_access_t {
  uint8_t  cfg_type;  /**< Has type M2M_CFG_t*/
  uint8_t  rd_wr;    /**< Type M2M_PKT_READ_WRITE_ENUM_t*/
  uint32_t address;   /**< Address of the config field to be modified*/
  int32_t  value;     /**< New config field value to be set*/
  uint8_t  status;
}m2m_pkt_xcfg_access_t;

// #############################################################################

/*!
  *@brief       Number of registers to read/write
 */

typedef enum {
  M2M_PKT_MULTI_ACCESS_1 = 0x00,
  M2M_PKT_MULTI_ACCESS_2 = 0x02,
  M2M_PKT_MULTI_ACCESS_3 = 0x04,
  M2M_PKT_MULTI_ACCESS_4 = 0x08,
  M2M_PKT_MULTI_ACCESS_5 = 0x10
}M2M_PKT_MULTI_ACCESS_NUM_ENUM_t;

// M2M_CMD_DEV_REG_ACCESS_REQ
// M2M_CMD_DEV_REG_ACCESS_RESP
typedef struct _m2m_pkt_reg_access_t {
  uint8_t device;   /**< The device whose registers are to be accessed*/
  uint8_t rd_wr;    /**< Type M2M_PKT_READ_WRITE_ENUM_t  |  M2M_PKT_MULTI_ACCESS_NUM_ENUM_t */
  uint8_t status;
  uint8_t address1;  /**< The register address to be accessed*/
  uint16_t value1;   /**< The value written/read to/from the register*/
  uint8_t address2;  /**< The register address to be accessed*/
  uint16_t value2;   /**< The value written/read to/from the register*/
  uint8_t address3;  /**< The register address to be accessed*/
  uint16_t value3;   /**< The value written/read to/from the register*/
  uint8_t address4;  /**< The register address to be accessed*/
  uint16_t value4;   /**< The value written/read to/from the register*/
  uint8_t address5;  /**< The register address to be accessed*/
  uint16_t value5;   /**< The value written/read to/from the register*/
}m2m_pkt_reg_access_t;

// #############################################################################
// M2M_CMD_CFG_SLOT_MODE_REQ
// M2M_CMD_CFG_SLOT_MODE_RESP

/*!
  *@brief       The type of info to send, used with #m2m_pkt_info_ctrl_t
 */
typedef enum {
  M2M_PKT_SLOTMODE_DISABLED,  /**< Disabled */
  M2M_PKT_SLOTMODE_4CH_16b,  /**< 4 channels 16 bits */
  M2M_PKT_SLOTMODE_4CH_32b,  /**< 4 channels 32 bits */
  M2M_PKT_SLOTMODE_SUM_16b,  /**< SUM 16 bits */
  M2M_PKT_SLOTMODE_SUM_32b,  /**< SUM 32 bits */
  M2M_PKT_SLOTMODE_DI1_16b,  /**< DI mode 1 (only X1) 16 bits */
  M2M_PKT_SLOTMODE_DI1_32b,  /**< DI mode 1 (only X1) 32 bits */
  M2M_PKT_SLOTMODE_DI2_16b,  /**< DI mode 2 (X1 and X2) 16 bits */
  M2M_PKT_SLOTMODE_DI2_32b,  /**< DI mode 2 (X1 and X2) 32 bits */
}M2M_PKT_SLOTMODE_ENUM_t;


typedef struct _m2m_pkt_slot_mode_access_t {
  uint8_t rd_wr;        /**< Type M2M_PKT_READ_WRITE_ENUM_t*/
  uint8_t slot_mode_A;  /**< Type M2M_PKT_SLOTMODE_ENUM_t*/
  uint8_t slot_mode_B;  /**< Type M2M_PKT_SLOTMODE_ENUM_t*/
}m2m_pkt_slot_mode_access_t;

// #############################################################################
// M2M_CMD_CFG_SYNC_MODE_REQ
// M2M_CMD_CFG_SYNC_MODE_RESP

/*!
  *@brief       Sync mode, used with #m2m_pkt_sync_mode_access_t
 */
typedef enum {
  M2M_PKT_SYNC_MODE_NO_SYNC,   /**< No sync */
  M2M_PKT_SYNC_MODE_SW_SYNC1,  /**< SW sync 1 */
  M2M_PKT_SYNC_MODE_SW_SYNC2,  /**< SW sync 2 */
  M2M_PKT_SYNC_MODE_HW_SYNC1,  /**< HW sync 1 */
  M2M_PKT_SYNC_MODE_HW_SYNC2,  /**< HW sync 2 */
  M2M_PKT_SYNC_MODE_HW_SYNC3,  /**< HW sync 3 */
}M2M_PKT_SYNC_MODE_ENUM_t;


typedef struct _m2m_pkt_sync_mode_access_t {
  uint8_t rd_wr;        /**< Type M2M_PKT_READ_WRITE_ENUM_t*/
  uint8_t sync_mode;    /**< Type M2M_PKT_SYNC_MODE_ENUM_t*/
  uint8_t status;
}m2m_pkt_sync_mode_access_t;

// #############################################################################
// M2M_CMD_INFO_REQ
// M2M_CMD_INFO_RESP

/*!
  *@brief       The type of info to send, used with #m2m_pkt_info_ctrl_t
 */
typedef enum {
  M2M_PKT_INFO_HW,  /**< HW info */
  M2M_PKT_INFO_FW, /**< FW info.*/
  M2M_PKT_INFO_APP, /**< Application info. */
}M2M_PKT_INFO_ENUM_t;


/*!
  *@brief       The MCU, used with #m2m_pkt_info_ctrl_t
 */
typedef enum {
  M2M_PKT_INFO_MCU_ADUCM302X,  /**< ADUCM302X */
  M2M_PKT_INFO_MCU_STM4,        /**< STM4.*/
}M2M_PKT_INFO_MCU_ENUM_t;


/*!
  *@brief       ADXL, used with #m2m_pkt_info_ctrl_t
 */
typedef enum {
  M2M_PKT_INFO_ADXL_NOT_EXIST,  /**< No ADXL */
  M2M_PKT_INFO_ADXL_EXIST,        /**< ADXL.*/
}M2M_PKT_INFO_ADXL_ENUM_t;


typedef enum {
  M2M_PKT_APP_LABVIEW,
}M2M_PKT_APP_TYPE_ENUM_t;

typedef struct _m2m_pkt_info_ctrl_t {
  uint8_t info_type;  /**< Type M2M_PKT_INFO_ENUM_t*/
  union {
    struct {
      uint8_t MCU_id;  /**< Type M2M_PKT_INFO_MCU_ENUM_t*/
      uint8_t ADPD_id;
      uint8_t ADXL_id; /**< Type M2M_PKT_INFO_ADXL_ENUM_t*/
    }HW;
    struct {
      uint32_t FW_id;
      uint32_t algo_id;
      uint32_t M2M_lib_id;
      uint8_t algo_type;
    }FW;
    struct {
      uint8_t type;
      uint8_t phy;
      uint8_t data_modes;
    }APP;
  }info;
}m2m_pkt_info_ctrl_t;

// #############################################################################
// M2M_CMD_CALC_REQ
// M2M_CMD_CALC_RESP


/*!
  *@brief       ADXL, used with #m2m_pkt_info_ctrl_t
 */
typedef enum {
  M2M_PKT_CALCULATION_DARKOFFSET,  /**< dark offset */
  M2M_PKT_CALCULATION_NOISE,  /**< Noise */
  M2M_PKT_CALCULATION_NA_MA,  /**< nA per mA */
}M2M_PKT_CALCULATION_ENUM_t;


typedef struct _m2m_pkt_calc_access_t {
  uint8_t data_type;          /**< Type M2M_PKT_CALCULATION_ENUM_t*/
  union {
    struct {
      uint32_t noise_value;
    }noise;
    struct {
      uint16_t offset_slot_A_ch1;
      uint16_t offset_slot_A_ch2;
      uint16_t offset_slot_A_ch3;
      uint16_t offset_slot_A_ch4;
      uint16_t offset_slot_B_ch1;
      uint16_t offset_slot_B_ch2;
      uint16_t offset_slot_B_ch3;
      uint16_t offset_slot_B_ch4;
    }chnl_offset;
    struct {
      uint32_t nApermA_value;
    }current;
  }calc;
}m2m_pkt_calc_access_t;

// #############################################################################
// M2M_CMD_RAW_DATA_REQ
// M2M_CMD_RAW_DATA_RESP

/*!
  *@brief       ADXL, used with #m2m_pkt_info_ctrl_t
 */
typedef enum {
  M2M_PKT_RAWDATA_MODE_STOP,  /**< STOP */
  M2M_PKT_RAWDATA_MODE_ADPD,  /**< adpd */
  M2M_PKT_RAWDATA_MODE_ADPD_ADXL,  /**< adpd + adxl */
}M2M_PKT_RAWDATA_MODE_ENUM_t;



typedef struct _m2m_pkt_raw_data_req_t {
  uint8_t data_type;
  uint8_t data_mode;       /**< Type M2M_PKT_RAWDATA_MODE_ENUM_t*/
}m2m_pkt_raw_data_req_t;

typedef struct _m2m_pkt_raw_data_t {
  uint8_t data_type;
  uint32_t timestamp;
  union {
    struct {
      uint16_t ch1_0;
      uint16_t ch2_0;
      uint16_t ch3_0;
      uint16_t ch4_0;
    }ch4_16;
    struct {
      uint8_t ch1[3];
      uint8_t ch2[3];
      uint8_t ch3[3];
      uint8_t ch4[3];
    }ch4_32;
    struct {
      uint16_t sum[6];
//      uint16_t sum0;
//      uint16_t sum1;
//      uint16_t sum2;
//      uint16_t sum3;
//      uint16_t sum4;
//      uint16_t sum5;
    }sum_16;
    struct {
      uint32_t sum[3];
//      uint32_t sum0;
//      uint32_t sum1;
//      uint32_t sum2;
    }sum_32;
    struct {
      uint16_t ch1[6];
//      uint16_t ch1_0;
//      uint16_t ch1_1;
//      uint16_t ch1_2;
//      uint16_t ch1_3;
//      uint16_t ch1_4;
//      uint16_t ch1_5;
    }DI_16;
    struct {
      uint16_t ch1_0;
      uint16_t ch2_0;
      uint16_t ch1_1;
      uint16_t ch2_1;
      uint16_t ch1_2;
      uint16_t ch2_2;
    }DI_32;
    struct {
      int16_t X_0;
      int16_t Y_0;
      int16_t Z_0;
      int8_t dX_1;
      int8_t dY_1;
      int8_t dZ_1;
      int8_t dX_2;
      int8_t dY_2;
      int8_t dZ_2;
    }adxl;
    struct {
      int16_t HR;
      int16_t confidence;
      int16_t HR_type;
      int16_t RRinterval;
    }HR;
    struct {
      uint8_t sum24[13];
    }sum_24;
  }raw_data;
}m2m_pkt_raw_data_t;

typedef enum {
  M2M_PKT_ADPD_NSAMPLES_4CH_16_ENUM_t = 1,
  M2M_PKT_ADPD_NSAMPLES_4CH_32_ENUM_t = 1,
  M2M_PKT_ADPD_NSAMPLES_SUM_16_ENUM_t = 6,
  M2M_PKT_ADPD_NSAMPLES_SUM_32_ENUM_t = 6,
  M2M_PKT_ADPD_NSAMPLES_DI1_16_ENUM_t = 6,
  M2M_PKT_ADPD_NSAMPLES_DI2_32_ENUM_t = 3,
  M2M_PKT_ADPD_NSAMPLES_SUM_24_ENUM_t = 6,
}M2M_PKT_ADPD_NSAMPLES_ENUM_t;

typedef enum {
  M2M_PKT_RAW_DATA_ADPD_A_4CH_16b       = 0x00,
  M2M_PKT_RAW_DATA_ADPD_A_4CH_32b       = 0x01,
  M2M_PKT_RAW_DATA_ADPD_A_SUM_16b       = 0x02,
  M2M_PKT_RAW_DATA_ADPD_A_SUM_32b       = 0x03,
  M2M_PKT_RAW_DATA_ADPD_A_DI1_16b       = 0x04,
  M2M_PKT_RAW_DATA_ADPD_A_DI1_32b       = 0x05,
  M2M_PKT_RAW_DATA_ADPD_A_DI2_16b       = 0x06,
  M2M_PKT_RAW_DATA_ADPD_A_DI2_32b       = 0x07,
  M2M_PKT_RAW_DATA_ADPD_B_4CH_16b       = 0x08,
  M2M_PKT_RAW_DATA_ADPD_B_4CH_32b       = 0x09,
  M2M_PKT_RAW_DATA_ADPD_B_SUM_16b       = 0x0A,
  M2M_PKT_RAW_DATA_ADPD_B_SUM_32b       = 0x0B,
  M2M_PKT_RAW_DATA_ADPD_B_DI1_16b       = 0x0C,
  M2M_PKT_RAW_DATA_ADPD_B_DI1_32b       = 0x0D,
  M2M_PKT_RAW_DATA_ADPD_B_DI2_16b       = 0x0E,
  M2M_PKT_RAW_DATA_ADPD_B_DI2_32b       = 0x0F,
  M2M_PKT_RAW_DATA_ADXL                 = 0x10,
  M2M_PKT_RAW_DATA_HR                   = 0x11,
  M2M_PKT_RAW_DATA_ADPD_A_SUM_24b       = 0x12,
  M2M_PKT_RAW_DATA_ADPD_B_SUM_24b       = 0x13,
}M2M_PKT_RAW_DATA_TYPES_ENUM_t;

// #############################################################################
// M2M_CMD_LOG_CFG_REQ
// M2M_CMD_LOG_CFG_RESP
typedef enum {
  M2M_PKT_LOG_TYPE_NONE   = 0x00,
  M2M_PKT_LOG_TYPE_SD     = 0x01,
}M2M_PKT_LOG_TYPE_ENUM_t;

typedef struct _m2m_pkt_logging_config_t {
  uint8_t log_type;   /**< M2M_PKT_LOG_TYPE_ENUM_t */
  uint8_t month;      /**< Month as a number 1-12 */
  uint8_t day;        /**< Day as a number 1-31 */
  uint16_t time;      /**< Time with fractional minutes: 3:20.5PM = 15205 */
}m2m_pkt_logging_config_t;

// #############################################################################
// M2M_CMD_RETURN

/*!
  *@brief  Information used to acknowledge the receipt of a M2M_PKT_CMD_t packet.
 */
typedef struct _m2m_pkt_return_struct_t {
  uint16_t  return_cmd; /**< The M2M_PKT_CMD_t that is being acknowledged.*/
  uint8_t   return_status;
  uint8_t   return_data;
}m2m_pkt_return_struct_t;

typedef enum {
  M2M_PKT_RETURN_OK           = 0x00,
  M2M_PKT_RETURN_NOT_CHECKED  = 0xFF,
}M2M_PKT_RETURN_STATUS_t;

 /*!
   *@brief       The packet definition.
   *@note        The @cmd field shall be given a value of type M2M_CMD_T.
   * Unfortunately C doesn't guarantee the size of an enumerated type across
   * machines, so we can't declare the struct field as M2M_PKT_CMD_t type and
   * guarantee that it will always be 16 bits wide.
  */
typedef struct _m2m_packet_t {
  uint16_t cmd;  /**< The command number for the packet.*/
  union {
    m2m_pkt_error_t             error;
    m2m_pkt_data_t              data_ctrl;
    m2m_pkt_BLE_ctrl_t          ble_ctrl;
    m2m_pkt_cfg_t               cfg_load;
    m2m_pkt_xcfg_access_t       xcfg_access;
    m2m_pkt_clk_cal_data_t      clk_cal;
    m2m_pkt_lib_ctrl_t          lib_start_stop;
    m2m_pkt_lib_state_data_t    lib_state;
    m2m_pkt_reg_access_t        reg_access;
    m2m_pkt_slot_mode_access_t  cfg_slot_mode;
    m2m_pkt_sync_mode_access_t  cfg_sync_mode;
    m2m_pkt_info_ctrl_t         info;
    m2m_pkt_calc_access_t       calc_access;
    m2m_pkt_raw_data_req_t      raw_data_req;
    m2m_pkt_raw_data_t          raw_data;
    m2m_pkt_logging_config_t    log_config;
    m2m_pkt_return_struct_t     pkt_return;
    uint8_t                     _DATA[18];  /**< Forces sizeof(m2m_packet_t)=20*/
  }payload;
}m2m_packet_t;


//  Sanity check the structure sizes.
#define M2M_PACKET_MAX_PAYLOAD (sizeof(m2m_packet_t) - MEMBER_SIZE(m2m_packet_t, cmd))
STATIC_ASSERT_PROJ(sizeof(m2m_packet_t) == 20, INCORRECT_SIZE_m2m_packet_t);
STATIC_ASSERT_PROJ(sizeof(m2m_pkt_error_t) <= M2M_PACKET_MAX_PAYLOAD,
                                      TOO_LARGE_m2m_pkt_error_t);
STATIC_ASSERT_PROJ(sizeof(m2m_pkt_data_t) <= M2M_PACKET_MAX_PAYLOAD,
                                      TOO_LARGE_m2m_pkt_data_t);
STATIC_ASSERT_PROJ(sizeof(m2m_pkt_BLE_ctrl_t) <= M2M_PACKET_MAX_PAYLOAD,
                                      TOO_LARGE_m2m_pkt_BLE_ctrl_t);
STATIC_ASSERT_PROJ(sizeof(m2m_pkt_cfg_t) <= M2M_PACKET_MAX_PAYLOAD,
                                      TOO_LARGE_m2m_pkt_cfg_t);
STATIC_ASSERT_PROJ(sizeof(m2m_pkt_xcfg_access_t) <= M2M_PACKET_MAX_PAYLOAD,
                                      TOO_LARGE_m2m_pkt_xcfg_mod_t);
STATIC_ASSERT_PROJ(sizeof(m2m_pkt_clk_cal_data_t) <= M2M_PACKET_MAX_PAYLOAD,
                                      TOO_LARGE_m2m_pkt_clk_cal_data_t);
STATIC_ASSERT_PROJ(sizeof(m2m_pkt_lib_ctrl_t) <= M2M_PACKET_MAX_PAYLOAD,
                                      TOO_LARGE_m2m_pkt_lib_ctrl_t);
STATIC_ASSERT_PROJ(sizeof(m2m_pkt_lib_state_data_t) <= M2M_PACKET_MAX_PAYLOAD,
                                      TOO_LARGE_m2m_pkt_lib_state_data_t);
STATIC_ASSERT_PROJ(sizeof(m2m_pkt_reg_access_t) <= M2M_PACKET_MAX_PAYLOAD,
                                      TOO_LARGE_m2m_pkt_reg_access_t);
STATIC_ASSERT_PROJ(sizeof(m2m_pkt_slot_mode_access_t) <= M2M_PACKET_MAX_PAYLOAD,
                                      TOO_LARGE_m2m_pkt_slot_mode_access_t);
STATIC_ASSERT_PROJ(sizeof(m2m_pkt_sync_mode_access_t) <= M2M_PACKET_MAX_PAYLOAD,
                                      TOO_LARGE_m2m_pkt_sync_mode_access_t);
STATIC_ASSERT_PROJ(sizeof(m2m_pkt_info_ctrl_t) <= M2M_PACKET_MAX_PAYLOAD,
                                      TOO_LARGE_m2m_pkt_info_ctrl_t);
STATIC_ASSERT_PROJ(sizeof(m2m_pkt_calc_access_t) <= M2M_PACKET_MAX_PAYLOAD,
                                      TOO_LARGE_m2m_pkt_calc_access_t);
STATIC_ASSERT_PROJ(sizeof(m2m_pkt_raw_data_t) <= M2M_PACKET_MAX_PAYLOAD,
                                      TOO_LARGE_m2m_pkt_raw_data_t);
STATIC_ASSERT_PROJ(sizeof(m2m_pkt_logging_config_t) <= M2M_PACKET_MAX_PAYLOAD,
                                      TOO_LARGE_m2m_pkt_logging_config_t);
STATIC_ASSERT_PROJ(sizeof(m2m_pkt_return_struct_t) <= M2M_PACKET_MAX_PAYLOAD,
                                      TOO_LARGE_m2m_pkt_return_struct_t);


// Reset packing outside of this file.
#if defined __CC_ARM || defined __IAR_SYSTEMS_ICC__ || defined __SES_ARM
#pragma pack(pop)
#else
#endif  // defined __CC_ARM || defined __IAR_SYSTEMS_ICC__
#endif  // M2M_PACKET_H
