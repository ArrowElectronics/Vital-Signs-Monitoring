#ifndef _KEY_DETECT_H_
#define _KEY_DETECT_H_
#include <stdint.h>

#define KEY_NUMBER  (2)

#define APP_KEY_PUSH        (0)                               /**< Indicates that a button is pushed. */
#define APP_KEY_RELEASE    (1)                               /**< Indicates that a button is released. */
#define APP_KEY_LONG_PUSH   (2) 

#define KEY_SELECT_VALUE  (0x01)
#define KEY_NAVIGATION_VALUE  (0x02)
#define KEY_SELECT_NAVIGATION_VALUE (0x03)
#define KEY_SELECT_LONG_VALUE  (0x21)
#define KEY_NAVIGATION_LONG_VALUE  (0x22)

#define SHORT_PRESS_TIME_MS  (50)
#define LONG_PRESS_TIMEOUT_MS (3000) /**< The time to hold for a long push (in milliseconds). */

//#define APP_KEY_PUSH        (0)                               /**< Indicates that a button is pushed. */
//#define APP_KEY_RELEASE    (1)                               /**< Indicates that a button is released. */
//#define APP_KEY_LONG_PUSH   (2) 

typedef void (*Send_key_func)(uint8_t value);
void Register_key_send_func(Send_key_func hander);

uint32_t Key_detect_init(void);
#endif

