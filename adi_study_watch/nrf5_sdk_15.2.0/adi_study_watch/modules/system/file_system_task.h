#ifndef __FILE_SYSTEM_TASK_H
#define __FILE_SYSTEM_TASK_H
#include "crc16.h"

void file_system_task_init(void);
void send_message_file_system_task(m2m2_hdr_t *p_pkt);
void FindConfigFile(volatile uint8_t* pCfgFileFoundFlag);
M2M2_ADDR_ENUM_t get_file_routing_table_entry_stream(M2M2_ADDR_ENUM_t address);

#endif  //  __FILE_SYSTEM_TASK_H