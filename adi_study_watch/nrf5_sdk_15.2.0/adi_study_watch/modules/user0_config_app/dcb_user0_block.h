#ifndef __DCB_USER0_BLK_H
#define __DCB_USER0_BLK_H

#include <stdint.h>
#include <printf.h>
#include "user0_config_app_task.h"

typedef enum {
  USER0_BLK_DCB_STATUS_OK = 0,
  USER0_BLK_DCB_STATUS_ERR,
  USER0_BLK_DCB_STATUS_NULL_PTR,
} USER0_BLK_DCB_STATUS_t;

void user0_blk_set_dcb_present_flag(bool set_flag);
bool user0_blk_get_dcb_present_flag(void);
void user0_blk_update_dcb_present_flag(void);
USER0_BLK_DCB_STATUS_t read_user0_blk_dcb(uint32_t *user0_blk_dcb_data, uint16_t *read_size);
USER0_BLK_DCB_STATUS_t write_user0_blk_dcb(uint32_t *user0_blk_dcb_data, uint16_t in_size);
USER0_BLK_DCB_STATUS_t delete_user0_blk_dcb(void);

USER0_BLK_DCB_STATUS_t load_user0_config_app_lcfg_dcb();
USER0_BLK_DCB_STATUS_t copy_user0_config_from_user0_blk_dcb(user0_config_app_lcfg_type_t *p_user0_config_app_lcfg);

#endif // __DCB_GEN_BLK_H
