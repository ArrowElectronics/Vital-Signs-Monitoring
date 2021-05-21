#ifndef _ADI_MAILBOX_H_
#define _ADI_MAILBOX_H_

#include <includes.h>
#include <stdint.h>
#include <stdbool.h>
#include <m2m2_core.h>
// Total size taken up by mailbox structure is num_boxes*num_subs*2
#define DEFAULT_MAX_NUM_MAILBOXES   5
#define DEFAULT_MAX_MAILBOX_SUBSCRIBER_NUM  5

typedef enum {
  ADI_MAILBOX_OK = 0,
  ADI_MAILBOX_ERROR_UNKNOWN,
  ADI_MAILBOX_ERROR_ALREADY_INIT,
  ADI_MAILBOX_ERROR_BOX_NOT_EXIST,
  ADI_MAILBOX_ERROR_ITEM_EXISTS,
  ADI_MAILBOX_ERROR_ITEM_NOT_EXIST,
  ADI_MAILBOX_ERROR_LIST_FULL,
  ADI_MAILBOX_MALLOC_FAILURE,
}ADI_MAILBOX_STATUS_t;

ADI_MAILBOX_STATUS_t mailbox_add_sub(
                              M2M2_ADDR_ENUM_t mailbox_addr,
                              M2M2_ADDR_ENUM_t sub_addr);
ADI_MAILBOX_STATUS_t mailbox_remove_sub(
                              M2M2_ADDR_ENUM_t mailbox_addr,
                              M2M2_ADDR_ENUM_t sub_addr);
M2M2_ADDR_ENUM_t *get_subscriber_list(
                                        M2M2_ADDR_ENUM_t mailbox_addr,
                                        uint8_t         *num_subs,
                                        uint8_t         *subs_list_size);
ADI_MAILBOX_STATUS_t add_mailbox(M2M2_ADDR_ENUM_t addr,
                                  uint8_t num_subs);
ADI_MAILBOX_STATUS_t remove_mailbox(M2M2_ADDR_ENUM_t addr);
ADI_MAILBOX_STATUS_t mailbox_list_init(uint8_t num_mailboxes);
#endif  // _ADI_MAILBOX_H_
