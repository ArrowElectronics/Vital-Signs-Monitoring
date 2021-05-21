#ifndef __DCB_WRIST_DETECT_H
#define __DCB_WRIST_DETECT_H

#include <stdint.h>
#include <printf.h>
#include "low_touch_task.h"

typedef enum {
  WRIST_DETECT_DCB_STATUS_OK = 0,
  WRIST_DETECT_DCB_STATUS_ERR,
  WRIST_DETECT_DCB_STATUS_NULL_PTR,
} WRIST_DETECT_DCB_STATUS_t;

void wrist_detect_set_dcb_present_flag(bool set_flag);
bool wrist_detect_get_dcb_present_flag(void);
void wrist_detect_update_dcb_present_flag(void);
WRIST_DETECT_DCB_STATUS_t load_wrist_detect_dcb();
WRIST_DETECT_DCB_STATUS_t read_wrist_detect_dcb(uint32_t *wrist_detect_dcb_data, uint16_t* read_size);
WRIST_DETECT_DCB_STATUS_t write_wrist_detect_dcb(uint32_t *wrist_detect_dcb_data, uint16_t in_size);
WRIST_DETECT_DCB_STATUS_t delete_wrist_detect_dcb(void);

void copy_lt_lcfg_from_wrist_detect_dcb(lt_app_cfg_type *p_lt_app_cfg);

#endif // __DCB_WRIST_DETECT_H
