#ifndef __DCB_LT_APP_LCFG_H
#define __DCB_LT_APP_LCFG_H

#include <stdint.h>
#include <printf.h>
#include "low_touch_task.h"

typedef enum {
  LT_APP_LCFG_DCB_STATUS_OK = 0,
  LT_APP_LCFG_DCB_STATUS_ERR,
  LT_APP_LCFG_DCB_STATUS_NULL_PTR,
} LT_APP_LCFG_DCB_STATUS_t;

void lt_app_lcfg_set_dcb_present_flag(bool set_flag);
bool lt_app_lcfg_get_dcb_present_flag(void);
void lt_app_lcfg_update_dcb_present_flag(void);
LT_APP_LCFG_DCB_STATUS_t load_lt_app_lcfg_dcb();
LT_APP_LCFG_DCB_STATUS_t read_lt_app_lcfg_dcb(uint32_t *lt_app_lcfg_dcb_data, uint16_t* read_size);
LT_APP_LCFG_DCB_STATUS_t write_lt_app_lcfg_dcb(uint32_t *lt_app_lcfg_dcb_data, uint16_t in_size);
LT_APP_LCFG_DCB_STATUS_t delete_lt_app_lcfg_dcb(void);

void copy_lt_lcfg_from_lt_app_lcfg_dcb(lt_app_cfg_type *p_lt_app_cfg);

#endif // __DCB_LT_APP_LCFG_H
