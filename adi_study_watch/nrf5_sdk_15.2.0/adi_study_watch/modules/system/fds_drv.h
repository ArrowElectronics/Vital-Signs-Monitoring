#ifndef FDS_DRV_H__
#define FDS_DRV_H__

#include "sdk_errors.h"

#define FDS_DIRTY_REC_CNT (10) //Allowed dirty records in FDS before garbage collection can happen

/* File name on FDS to store RTC data */
#define ADI_RTC_FILE        (0xACCC)

/* Record index within RTC File on FDS */
typedef enum eRtcConfigIndex {
  ADI_DCB_RTC_TIME_BLOCK_IDX = 0x2000,
  MANUFACTURE_DATE_BLOCK_IDX = 0x3000,
  
}eRtcConfigIndex;

ret_code_t adi_fds_init(void);
ret_code_t adi_fds_clear_entries(uint16_t fds_file);
ret_code_t adi_fds_update_entry(const uint16_t file_id, const uint16_t fds_index, const void *data,const uint16_t len);
ret_code_t adi_fds_read_entry(const uint16_t file_id, const uint16_t fds_index,void *data,uint16_t *len);
ret_code_t adi_fds_delete_record(const uint16_t file_id, const uint16_t fds_index);
ret_code_t adi_fds_check_entry(const uint16_t file_id, const uint16_t fds_index);
 
#endif // FDS_DRV_H__