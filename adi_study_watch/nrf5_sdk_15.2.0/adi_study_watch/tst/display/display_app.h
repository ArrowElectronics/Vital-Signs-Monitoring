#ifndef _DISPLAY_APP_H_
#define _DISPLAY_APP_H_

#include "stdint.h"
typedef struct{
    void (*display_func)(void);
    void (*key_handle)(uint8_t key_value);
    void (*signal_handle)(uint8_t signal_value);
}PAGE_HANDLE;

typedef enum{
    DIS_KEY_SIGNAL = 0,
    DIS_PRIVATE_SIGNAL,
    DIS_GLOBLE_SIGNAL
}DISPLAY_SIGNAL_TYPE;

typedef enum{
    DIS_RESET_SIGNAL = 0,
    DIS_REFRESH_SIGNAL
}DIS_GLOBLE_SIGNAL_TYPE;

typedef struct{
    uint8_t signal_type;
    uint8_t signal_value;
}display_signal_t;

void Lcd_color(uint8_t value);
void lcd_display_refresh_all(void);

/*all page handle */
extern const PAGE_HANDLE page_hr;
extern const PAGE_HANDLE page_memu;



void display_app_init(void);
void JumpPage(const PAGE_HANDLE *page);
void EnableDynamicRefresh(uint16_t refresh_ms,uint16_t refresh_num);

#endif




