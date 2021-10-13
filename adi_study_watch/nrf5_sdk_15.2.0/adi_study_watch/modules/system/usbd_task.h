#ifndef __USBD_TASK_H
#define __USBD_TASK_H

void usbd_task_init(void);
void send_message_usbd_tx_task(m2m2_hdr_t *p_pkt);

#ifdef CUST4_SM
typedef enum {
  USBD_CONNECTED = 0,
  USBD_DISCONNECTED = 1,
} USBD_CONN_STATUS_t;

USBD_CONN_STATUS_t usbd_get_cradle_disconnection_status();
#endif

#endif  //  __USBD_TASK_H
