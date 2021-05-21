#ifndef __USBD_TASK_H
#define __USBD_TASK_H

void usbd_task_init(void);
void send_message_usbd_tx_task(m2m2_hdr_t *p_pkt);

#endif  //  __USBD_TASK_H
