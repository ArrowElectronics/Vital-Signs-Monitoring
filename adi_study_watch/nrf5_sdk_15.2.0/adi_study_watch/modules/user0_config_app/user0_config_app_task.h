#ifndef __USER0_CONFIG_APP_TASK_H
#define __USER0_CONFIG_APP_TASK_H
#include <m2m2_core.h>
#include <adi_types.h>
#include "user0_config_application_interface.h"

typedef enum {
  USER0_CONFIG_ERROR = 0,
  USER0_CONFIG_SUCCESS = 1,
} USER0_CONFIG_ERROR_CODE_t;

typedef enum {
  USER0_CONFIG_LCFG_AGC_UP_TH = 0,           //!< Upper limit of LED light intensity for Static AGC
  USER0_CONFIG_LCFG_AGC_LOW_TH,              //!< Lower limit of LED light intensity for Static AGC
  USER0_CONFIG_LCFG_ADV_TIMEOUT_MONITOR,     /* Elapsed time from the start of advertising after finishing PPG,
                                              *  EDA, skin temperature, and acceleration measurement during
                                              *  intermittent operation to the time out */
  USER0_CONFIG_LCFG_HW_ID,              //!< Unique identification number assigned to each wristband
  USER0_CONFIG_LCFG_EXP_ID,                 //!< Patient-specific identification number assigned to each case

  USER0_CONFIG_LCFG_ADXL_START_TIME,          //!< Time to give delayed start time for ADXL app
  USER0_CONFIG_LCFG_ADXL_TON,                 //!< Time tON for ADXL app
  USER0_CONFIG_LCFG_ADXL_TOFF,                //!< Time tOFF for ADXL app

  USER0_CONFIG_LCFG_TEMP_START_TIME,          //!< Time to give delayed start time for measure skin temperature
  USER0_CONFIG_LCFG_TEMP_TON,                 //!< Time tON to measure skin temperature
  USER0_CONFIG_LCFG_TEMP_TOFF,                //!< Time tOFF to stop measure skin temperature

  USER0_CONFIG_LCFG_ADPD_START_TIME,          //!< Time to give delayed start time for ADPD app
  USER0_CONFIG_LCFG_ADPD_TON,                 //!< Time tON for ADPD app
  USER0_CONFIG_LCFG_ADPD_TOFF,                //!< Time tOFF for ADPD app

  USER0_CONFIG_LCFG_EDA_START_TIME,          //!< Time to give delayed start time for EDA app
  USER0_CONFIG_LCFG_EDA_TON,                 //!< Time tON for EDA app
  USER0_CONFIG_LCFG_EDA_TOFF,                //!< Time tOFF for EDA app

  USER0_CONFIG_LCFG_SLEEP_MIN,              //!< Sleep time of intermittent operation
  USER0_CONFIG_LCFG_SIGNAL_THRESHOLD,       //!< AD threshold for determining light intensity and TIA gain with static AGC
  USER0_CONFIG_LCFG_MAX,
} USER0_CONFIG_LCFG_t;

#define USER0_CFG_AGC_UP_TH   (66)
#define USER0_CFG_AGC_LOW_TH  (1)
#define USER0_CFG_ADV_TIMEOUT_MONITOR (60)
#define USER0_CFG_HW_ID (01)
#define USER0_CFG_EXP_ID (0001)

#define USER0_CFG_ADXL_START_TIME (0)
#define USER0_CFG_ADXL_TON (0)
#define USER0_CFG_ADXL_TOFF (0)

#define USER0_CFG_TEMP_START_TIME (0)
#define USER0_CFG_TEMP_TON (0)
#define USER0_CFG_TEMP_TOFF (0)

#define USER0_CFG_ADPD_START_TIME (0)
#define USER0_CFG_ADPD_TON (0)
#define USER0_CFG_ADPD_TOFF (0)

#define USER0_CFG_EDA_START_TIME (0)
#define USER0_CFG_EDA_TON (0) //EDA app ON time=6sec
#define USER0_CFG_EDA_TOFF (0) //EDA app OFF time=1sec

#define USER0_CFG_SLEEP_MIN     (0)
#define USER0_CFG_SIGNAL_THRESHOLD (150000)

#define GET_ID_SUCCESS (0)
#define GET_ID_FAILURE (1)

#define SET_ID_SUCCESS (0)
#define SET_ID_FAILURE (1)

typedef struct
{
  /**
   * Desc: Upper limit of LED light intensity for Static AGC
   * Setting range: 1-127
   * Increment: 1
   * Unit: LSB
   */
  uint8_t agc_up_th;
  /**
   * Desc: Lower limit of LED light intensity for Static AGC
   * Setting range: 1-127
   * Increment: 1
   * Unit: LSB
   */
  uint8_t agc_low_th;
  /**
   * Desc: Elapsed time from the start of advertising after finishing PPG,
   *       EDA, body temperature, and acceleration measurement during
   *       intermittent operation to the time out
   * Setting range: 10-300
   * Increment: 10
   * Unit: second
   */
  uint16_t adv_timeout_monitor;
  /**
   * Desc: Unique identification number assigned to each wristband
   * Setting range: 00-99
   * Increment: 1
   * Unit: NA
   */
  uint16_t hw_id;
  /**
   * Desc: Patient-specific identification number assigned to each case
   *       (from entry operation to exit operation)
   * Setting range: 0000-9999
   * Increment: 1
   * Unit: NA
   */
  uint16_t exp_id;
  /**
   * Desc: Time to measure ADXL
   * Setting range: 1-180
   * Increment: 1
   * Unit: second
   */
  uint16_t adxl_start_time;
  uint16_t adxl_tON;
  uint16_t adxl_tOFF;

  /**
   * Desc: Time to measure skin temperature
   * Setting range: 1-60
   * Increment: 1
   * Unit: second
   */
  uint16_t temp_start_time;
  uint16_t temp_tON;
  uint16_t temp_tOFF;

  /**
   * Desc: Time to measure ADPD
   * Setting range: 1-180
   * Increment: 1
   * Unit: second
   */
  uint16_t adpd_start_time;
  uint16_t adpd_tON;
  uint16_t adpd_tOFF;

  /**
   * Desc: Time to measure EDA
   * Setting range: 1-180
   * Increment: 1
   * Unit: second
   */
  uint16_t eda_start_time;
  uint16_t eda_tON;
  uint16_t eda_tOFF;
  /**
   * Desc: Sleep time of intermittent operation
   * Setting range: 1-180
   * Increment: 1
   * Unit: minute
   */
  uint16_t sleep_min;
  /**
   * Desc: AD threshold for determining light intensity and TIA gain with static AGC
   * Setting range: 1-(2^(14-1)*127)
   * Increment: 1
   * Unit: LSB
   */
  uint32_t signal_threshold;
}user0_config_app_lcfg_type_t;

/**
* Structure which holds the timing related info for application,
* which can be used for interval based and intermittent operation
*/
typedef struct {
  uint32_t on_time_count;   /* Counter to check if tON time is reached or not */
  uint32_t off_time_count;  /* Counter to check if tOFF time is reached or not */
  uint32_t start_time_count;/* Counter to check if delated start time is reached or not */
  bool delayed_start;       /* Flag which is true if delayed-start_time counter is running */
  bool check_timer_started; /* Flag which is true when the timer is running */
  bool app_mode_on;         /* Flag which is true when the app is running(during tON time) */
  uint16_t start_time;      /* Time is secs to delay the application start */
  uint16_t on_time;         /* Time in secs for tON */
  uint16_t off_time;        /* Time in secs for tOFF */
}user0_config_app_timing_params_t;

#define TIMER_ONE_SEC_INTERVAL (1000) /* valie in millisec, to count 1 sec. interval */

void user0_config_app_task_init(void);
void send_message_user0_config_app(m2m2_hdr_t *p_pkt);
USER0_CONFIG_APP_STATE_t get_user0_config_app_state();
void set_user0_config_app_state(USER0_CONFIG_APP_STATE_t state);
USER0_CONFIG_APP_STATE_t get_user0_config_app_state();

void user0_config_app_enter_state_admit_standby();
void user0_config_app_enter_state_start_monitoring();
void user0_config_app_enter_state_sleep();
void user0_config_app_enter_state_intermittent_monitoring();
void user0_config_app_enter_state_end_monitoring();

bool is_adxl_app_mode_continuous();
void get_adxl_app_timings_from_user0_config_app_lcfg(uint16_t *start_time, uint16_t *tON, uint16_t *tOFF);

bool is_temp_app_mode_continuous();
void get_temp_app_timings_from_user0_config_app_lcfg(uint16_t *start_time, uint16_t *tON, uint16_t *tOFF);

bool is_adpd_app_mode_continuous();
void get_adpd_app_timings_from_user0_config_app_lcfg(uint16_t *start_time, uint16_t *tON, uint16_t *tOFF);

bool is_eda_app_mode_continuous();
void get_eda_app_timings_from_user0_config_app_lcfg(uint16_t *start_time, uint16_t *tON, uint16_t *tOFF);

uint8_t get_id_num(USER0_CONFIG_LCFG_t id_index, uint16_t * id_num);
void get_id_num_fw_lcfg(uint16_t * id_num);
#endif  // __USER0_CONFIG_APP_TASK_H
