#include "display_app.h"
#include "GUI.h"
#include "lcd_driver.h"
#include "key_detect.h"
#include "dis_driver.h"
#include "dis_image.h"

static void display_func(void)
{
    Lcd_color(0xff);
            
    Display_image(&bmdianluban,0,0);
    lcd_display_refresh_all();
}

static void key_handle(uint8_t key_value)
{
    switch(key_value)
    {
        case KEY_SELECT_VALUE:
        {
            JumpPage(&page_memu);
        }
        break;
        case KEY_NAVIGATION_VALUE:
        {
            
        }
        break;
        default:break;
    }

}
const PAGE_HANDLE page_hr = {
.display_func = &display_func,
.key_handle = &key_handle
};