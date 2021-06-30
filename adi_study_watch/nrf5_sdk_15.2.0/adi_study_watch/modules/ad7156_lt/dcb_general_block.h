#ifndef __DCB_GEN_BLK_H
#define __DCB_GEN_BLK_H

#include <stdint.h>
#include <printf.h>

typedef enum {
  GEN_BLK_DCB_STATUS_OK = 0,
  GEN_BLK_DCB_STATUS_ERR,
  GEN_BLK_DCB_STATUS_NULL_PTR,
} GEN_BLK_DCB_STATUS_t;

void gen_blk_set_dcb_present_flag(bool set_flag);
bool gen_blk_get_dcb_present_flag(void);
void gen_blk_update_dcb_present_flag(void);
GEN_BLK_DCB_STATUS_t read_gen_blk_dcb(uint32_t *gen_blk_dcb_data, uint16_t* read_size);
GEN_BLK_DCB_STATUS_t write_gen_blk_dcb(uint32_t *gen_blk_dcb_data, uint16_t in_size);
GEN_BLK_DCB_STATUS_t delete_gen_blk_dcb(void);

GEN_BLK_DCB_STATUS_t copy_lt_config_from_gen_blk_dcb(uint8_t *dest_ptr, uint16_t *dest_len);

#endif // __DCB_GEN_BLK_H
