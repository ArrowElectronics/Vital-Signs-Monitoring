#ifndef __FILE_SYSTEM_TASK_H
#define __FILE_SYSTEM_TASK_H
#include "crc16.h"
//typedef struct{
//  uint16_t tool_addr;
//  uint8_t chunk_size;
//}fs_download_info;
//
typedef enum FS_DOWNLOAD_MODE_t
{
  FS_STREAM_USB_MODE = 0,
  FS_STREAM_BLE_MODE = 1, 
}FS_DOWNLOAD_MODE_t;

void file_system_task_init(void);
void send_message_file_system_task(m2m2_hdr_t *p_pkt);
void FindConfigFile(volatile uint8_t* pCfgFileFoundFlag);
M2M2_ADDR_ENUM_t get_file_routing_table_entry_stream(M2M2_ADDR_ENUM_t address);
void get_fs_format_debug_info(m2m2_file_sys_format_debug_info_resp_t *debug_info);
#endif  //  __FILE_SYSTEM_TASK_H
