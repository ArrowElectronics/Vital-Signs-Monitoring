#ifndef _ADI_POST_OFFICE_H_
#define _ADI_POST_OFFICE_H_

#include <includes.h>
#include <stdint.h>
#include <stddef.h>
#include <m2m2_core.h>
#include <post_office_interface.h>
#include <common_application_interface.h>

#define M2M2_HEADER_SZ	(offsetof(m2m2_hdr_t, data))
#define PM_PS_COLLISION_DEBUG

/* Swap bytes in 16 bit value.  */
#define BYTE_SWAP_16(x) \
  (((x) << 8) | (((x) >> 8) & 0xFF))

/* Swap bytes in 32 bit value.  */
#define BYTE_SWAP_32(x) \
  ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) | \
   (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))


#define PYLD_CST(P_PKT, PAYLOAD_TYPE, VAR_NAME) PAYLOAD_TYPE* VAR_NAME = (PAYLOAD_TYPE *)&P_PKT->data[0]
#define PKT_MALLOC(P_PKT, PAYLOAD_TYPE, EXTRA_BYTES) m2m2_hdr_t *P_PKT = post_office_create_msg(sizeof(PAYLOAD_TYPE) + M2M2_HEADER_SZ + EXTRA_BYTES)

/*!
 * @brief:  Callback function type
 */
typedef void (post_office_app_msg_send_func_t)(m2m2_hdr_t*);


/*!
 * @brief:  The data structure used to define routing table entry in the Post Office.
 */
typedef struct _post_office_routing_table_entry_t {
  M2M2_ADDR_ENUM_t           address;
  post_office_app_msg_send_func_t   *p_func;
}post_office_routing_table_entry_t;

typedef enum task_queue_index_t
{
  APP_OS_CFG_BOOT_TASK_INDEX                   =  0,
  APP_OS_CFG_POST_OFFICE_TASK_INDEX            =  1,
  APP_OS_CFG_SYS_TASK_INDEX                    =  2,
  APP_OS_CFG_FS_TASK_INDEX                     =  3,
  APP_OS_CFG_LED_TASK_INDEX                    =  4,
  APP_OS_CFG_USBD_TX_TASK_INDEX                =  5,
  APP_OS_CFG_WDT_TASK_INDEX                    =  6,
  APP_OS_CFG_ADPD4000_TASK_INDEX               =  7,
  APP_OS_CFG_ADXL_TASK_INDEX                   =  8,
  APP_OS_CFG_PPG_TASK_INDEX                    =  9,
  APP_OS_CFG_SYNC_DATA_TASK_INDEX              =  10,
  APP_OS_CFG_BLE_TASK_INDEX                    =  11,
  APP_OS_CFG_LOGGER_TASK_INDEX                 =  12,
  APP_OS_CFG_ECG_TASK_INDEX                    =  13,
  APP_OS_CFG_DISPLAY_TASK_INDEX                =  14,
  APP_OS_CFG_TEMPERATURE_TASK_INDEX            =  15,
  APP_OS_CFG_EDA_TASK_INDEX                    =  16,
  APP_OS_CFG_BCM_TASK_INDEX                    =  17,
  APP_OS_CFG_LT_TASK_INDEX                     =  18,
  APP_OS_CFG_PEDOMETER_TASK_INDEX              =  19,
  APP_OS_CFG_TOUCH_TASK_INDEX                  =  20,
  APP_OS_CFG_SQI_TASK_INDEX                    =  21,
  APP_OS_CFG_BLE_SERVICES_SENSOR_TASK_INDEX    =  22,
  APP_OS_CFG_MAX_TASKS,
}task_queue_index_t;
void update_task_queue_list (task_queue_index_t task_index, ADI_OSAL_QUEUE_HANDLE task_queue_handle);
ADI_OSAL_QUEUE_HANDLE get_task_queue_handler( task_queue_index_t task_index);

static M2M2_STATUS_ENUM_t mailbox_create(
                              M2M2_ADDR_ENUM_t mailbox_id);

static void send_to_mailbox(m2m2_hdr_t  *p_msg);

void post_office_main(void);
void post_office_send(m2m2_hdr_t *pkt, ADI_OSAL_STATUS *err);
m2m2_hdr_t *post_office_get(ADI_OSAL_TICKS timeout,task_queue_index_t nIndex);
m2m2_hdr_t *post_office_create_msg(uint16_t length);
void post_office_consume_msg(m2m2_hdr_t *p_msg);
void post_office_setup_subscriber(M2M2_ADDR_ENUM_t requester_addr,
                                  M2M2_ADDR_ENUM_t mailbox_addr,
                                  M2M2_ADDR_ENUM_t sub_addr,
                                  uint8_t add);
void post_office_add_mailbox(M2M2_ADDR_ENUM_t requester_addr, M2M2_ADDR_ENUM_t mailbox_addr);
void post_office_task_init(void);

void get_routing_table_size(uint8_t *pSize);
M2M2_ADDR_ENUM_t get_routing_table_element(uint8_t nElement);
uint8_t get_routing_table_index(M2M2_ADDR_ENUM_t m2m2_address);

#pragma once
#ifdef MEM_MGMT_DBG
#define MEM_DBG_SIZE  20
#pragma pack(push,1)
typedef struct {
uint16_t src;
uint16_t dst;
uint16_t size;
uint32_t tick;
uint8_t *pAddr;
//char task_name[8];
}msg_mgmt_info_t;

#pragma pack(pop)
//typedef msg_alloc_info_ msg_alloc_info_t;
extern msg_mgmt_info_t mem_create_arr,mem_free_arr[MEM_DBG_SIZE];
#endif

#ifdef DEBUG_PKT
void post_office_msg_cnt(m2m2_hdr_t *p_msg);
#endif

#endif  // _ADI_POST_OFFICE_H_
