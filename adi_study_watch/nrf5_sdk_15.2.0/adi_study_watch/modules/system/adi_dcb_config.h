#ifndef __ADI_DCB_CONFIG_H__
#define __ADI_DCB_CONFIG_H__

#include "fds.h"
#include "fds_drv.h"

#define ADI_DCB_FILE        (0xADCB)

uint8_t adi_dcb_clear_fds_settings();
uint8_t adi_dcb_write_to_fds(uint16_t dcb_rcrd_key, uint32_t *wd_dcb_data,uint16_t len_DWORD);
uint8_t adi_dcb_read_from_fds(uint16_t dcb_rcrd_key,uint32_t *rd_dcb_data, uint16_t *len_DWORD);
uint8_t adi_dcb_clear_fds_settings();
uint8_t adi_dcb_delete_fds_settings(uint16_t dcb_rec_key);
uint8_t adi_dcb_check_fds_entry(uint16_t dcb_rec_key);
void get_fds_status(fds_stat_t *stat) ;
#endif 