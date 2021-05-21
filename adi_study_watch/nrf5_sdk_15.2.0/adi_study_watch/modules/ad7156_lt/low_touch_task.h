#ifndef __LOW_TOUCH_TASK_H
#define __LOW_TOUCH_TASK_H
#include <m2m2_core.h>
#include <adi_types.h>

typedef enum {
  OFF_WRIST = 0,
  ON_WRIST = 1,
} LOW_TOUCH_DETECTION_STATUS_ENUM_t;

typedef enum {
  LT_APP_LCFG_ONWR_TIME = 0,
  LT_APP_LCFG_OFFWR_TIME,
  LT_APP_LCFG_AIR_CAP_VAL,
  LT_APP_LCFG_SKIN_CAP_VAL,
  LT_APP_LCFG_MAX,
} LT_APP_LCFG_t;

typedef enum {
  LT_APP_ERROR = 0,
  LT_APP_SUCCESS = 1,
} LT_APP_ERROR_CODE_t;

typedef struct
{
  //Time value for detection of ON WR event in millisec
  uint16_t onWristTimeThreshold;
  //Time value for detection of OFF WR event in millisec
  uint16_t offWristTimeThreshold;
  //Capacitance Value from CH2 cap sensor is placed on Air
  uint16_t airCapVal;
  //Capacitance Value when CH2 cap sensor is placed on Skin
  uint16_t skinCapVal;
}lt_app_cfg_type;

void low_touch_task_init(void);
int low_touch_deinit();
void send_message_lt_task(m2m2_hdr_t *p_pkt);
int EnableLowTouchDetection(bool bFlag);
void lt_app_lcfg_set_fw_default();
void lt_app_lcfg_set_from_dcb();
int get_lt_app_status();
void bottom_touch_func_set(uint8_t en);
#endif  // __LOW_TOUCH_TASK_H
