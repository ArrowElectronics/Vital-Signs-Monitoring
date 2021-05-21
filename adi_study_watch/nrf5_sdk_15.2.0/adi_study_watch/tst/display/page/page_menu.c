#include "display_app.h"
#include "GUI.h"
#include "lcd_driver.h"
#include "key_detect.h"
#include "dis_driver.h"
#include "dis_image.h"
#include "ADP5360/ADP5360.h"

static void display_func(void)
{
    static uint16_t bat_vol = 8;
    Lcd_color(0xff);      
    Display_string(&GUI_Fontweiruanyahei48,40,10,COLOR_DEFAULT,"Hello");
    Display_image(&bmfenda,0,50);

    
    if(ADP5360_SUCCESS != Adp5360_getBatVoltage(&bat_vol))
    {
    bat_vol++;
    }
    Display_value_middle(&GUI_Fontweiruanyahei48,102,150,COLOR_DEFAULT,bat_vol);
    lcd_display_refresh_all();

    EnableDynamicRefresh(1000,0);
}

static void key_handle(uint8_t key_value)
{
    switch(key_value)
    {
        case KEY_SELECT_VALUE:
        {
            JumpPage(&page_hr);
        }
        break;
        case KEY_NAVIGATION_VALUE:
        {
            
        }
        break;
        /*can add other key handle*/
        default:break;
    }

}
/*used to handle signal except key,
  for example
*/
static void signal_handle(uint8_t signal_value)
{
    switch(signal_value)
    {
        case 0:
        {
            
        }
        break;
        case 1:
        {
            
        }
        break;
        /*can add other key handle*/
        default:break;
    }
}
const PAGE_HANDLE page_memu = {
.display_func = &display_func,
.key_handle = &key_handle,
.signal_handle = &signal_handle
};