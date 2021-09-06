#ifndef __FILE_SYSTEM_TASK_H
#define __FILE_SYSTEM_TASK_H
#include "crc16.h"

void file_system_task_init(void);
void send_message_file_system_task(m2m2_hdr_t *p_pkt);
void FindConfigFile(volatile uint8_t* pCfgFileFoundFlag);
M2M2_ADDR_ENUM_t get_file_routing_table_entry_stream(M2M2_ADDR_ENUM_t address);
void get_fs_format_debug_info(m2m2_file_sys_format_debug_info_resp_t *debug_info);
#endif  //  __FILE_SYSTEM_TASK_H
