#ifndef _TOUCH_DETECT_H_
#define _TOUCH_DETECT_H_
#include <stdint.h>

#define APP_TOUCH_PUSH        (0)                               /**< Indicates that a button is pushed. */
#define APP_TOUCH_RELEASE    (1)                               /**< Indicates that a button is released. */
#define APP_TOUCH_LONG_PUSH   (2)

#define TOUCH_TOP_VALUE  (0x01)
#define TOUCH_BUTTOM_VALUE  (0x02)

#define TOUCH_TOP_SHORT (0x11)//short press
#define TOUCH_BUTTOM_SHORT (0x12) //short press

#define TOUCH_TOP_LONG_VALUE  (0x21)
#define TOUCH_BUTTOM_LONG_VALUE  (0x22)

#define TOUCH_SHORT_PRESS_TIME_MS  (8)//add detect time, filter disturb from the display screen.
#define TOUCH_LONG_PRESS_TIMEOUT_MS (500) /**< The time to hold for a long push (in milliseconds). */

typedef void (*Send_touch_func)(uint8_t value);

void Register_touch_send_func(Send_touch_func hander);
void Unregister_touch_send_func(Send_touch_func hander);
void Clear_register_touc_func(void);
void top_touch_func_set(uint8_t en);
void touch_detect_init(void);
int top_touch_init();
int top_touch_deinit();

void send_message_top_touch_task(m2m2_hdr_t *p_pkt);
#endif

