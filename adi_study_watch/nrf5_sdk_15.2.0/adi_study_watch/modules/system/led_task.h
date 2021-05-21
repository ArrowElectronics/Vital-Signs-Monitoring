#ifndef __LED_TASK_H
#define __LED_TASK_H

void led_task_init(void);
void send_message_led_task(m2m2_hdr_t *p_pkt);

#endif  //  __LED_TASK_H