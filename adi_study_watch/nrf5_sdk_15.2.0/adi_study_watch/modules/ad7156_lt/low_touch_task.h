#ifndef __LOW_TOUCH_TASK_H
#define __LOW_TOUCH_TASK_H
#include <m2m2_core.h>
#include <adi_types.h>

typedef enum {
  OFF_WRIST = 0,
  ON_WRIST = 1,
} LOW_TOUCH_DETECTION_STATUS_ENUM_t;

typedef enum {
  LT_APP_LCFG_ONWR_TIME = 0,   //!< Time kept for OnWrist detection
  LT_APP_LCFG_OFFWR_TIME,      //!< Time kept for OffWrist detection
  LT_APP_LCFG_AIR_CAP_VAL,     //!< Tuning parameter - Air capacitance
  LT_APP_LCFG_SKIN_CAP_VAL,    //!< Tuning parameter - Skin capacitance
  LT_APP_LCFG_TRIGGER_METHOD,  //!< LT App tigger method
  LT_APP_LCFG_MAX,
} LT_APP_LCFG_t;

typedef enum {
  LT_APP_ERROR = 0,
  LT_APP_SUCCESS = 1,
} LT_APP_ERROR_CODE_t;

typedef enum {
  LT_APP_CAPSENSE_TUNED_TRIGGER = 0,   //!< Tuning required, Capacitance sensing will trigger LT app
  LT_APP_CAPSENSE_DISPLAY_TRIGGER = 1, //!< Tuning not required, LT app triggered from Watch display/m2m2 command, LT App still based on Capacitance sensing
  LT_APP_BUTTON_TRIGGER = 2,           //!< LT app triggered from button press, not based on Capacitance sensing
#ifdef CUST4_SM
  LT_APP_INTERMITTENT_TRIGGER = 3,     //!< LT app triggered after wakeup from intermittent sleep
  LT_APP_TRIGGER_INVALID = 4,          //!< Invalid State
#else
  LT_APP_TRIGGER_INVALID = 3,          //!< Invalid State
#endif
} LT_APP_LCFG_TRIGGER_METHOD_t;

typedef struct
{
  //!< Time value for detection of ON WR event in millisec
  uint16_t onWristTimeThreshold;
  //!< Time value for detection of OFF WR event in millisec
  uint16_t offWristTimeThreshold;
  //!< Capacitance Value from CH2 cap sensor is placed on Air
  uint16_t airCapVal;
  //!< Capacitance Value when CH2 cap sensor is placed on Skin
  uint16_t skinCapVal;
  LT_APP_LCFG_TRIGGER_METHOD_t ltAppTrigMethd;
}lt_app_cfg_type;

void low_touch_task_init(void);
int low_touch_deinit();
void send_message_lt_task(m2m2_hdr_t *p_pkt);
int EnableLowTouchDetection(bool bFlag);
void lt_app_lcfg_set_fw_default();
void lt_app_lcfg_set_from_dcb();
int get_lt_app_status();
void bottom_touch_func_set(uint8_t en);
bool get_LT_mode2_log_enable_status(void);
void set_LT_mode2_log_enable_status(bool b_status);
bool get_lt_mode2_selection_status(void);
void reset_lt_mode2_selection_status(void);
void resume_low_touch_task(void);
void resume_key_and_lt_task(void);
bool check_lt_app_capsense_tuned_trigger_status();
bool get_low_touch_trigger_mode2_status(void);
bool get_low_touch_trigger_mode3_status(void);
LT_APP_LCFG_TRIGGER_METHOD_t get_lt_app_trigger_method();
#endif  // __LOW_TOUCH_TASK_H
