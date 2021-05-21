#include "display_app.h"
/* FreeRTOS related */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "nrf_drv_gpiote.h"
#include "key_detect.h"
#include "dis_driver.h"

#include "app_timer.h"
#include "GUI.h"
#include "lcd_driver.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

/**
 * The size of the stack for the USB task (in 32-bit words).
 */
#define DISPLAY_STACK_SIZE   2048
/**
 * The priority of the USBD task.
 */
#define DISPLAY_PRIORITY   4
static TaskHandle_t m_display_thread;        /**< USB stack thread. */

static QueueHandle_t xDisplayQueue = NULL;
//void lcd_display_refresh_all(void);
//void lcd_display_refresh_all_1bit(void);
//void lcd_display_refresh_all_3bit(void);
void lcd_init(void);
//void Lcd_color(uint8_t value);

extern GUI_CONST_STORAGE GUI_BITMAP bmwearable;
extern GUI_CONST_STORAGE GUI_BITMAP bmmeinu;

void Send_key_value(uint8_t  k_value)
{
    BaseType_t * pxHigherPriorityTaskWoken = pdFALSE;
    display_signal_t key_signal;
    key_signal.signal_type = DIS_KEY_SIGNAL;
    key_signal.signal_value = k_value;
    xQueueSendFromISR( xDisplayQueue, &key_signal, pxHigherPriorityTaskWoken );
}

void Send_global_type_value(uint8_t       value)
{
    BaseType_t * pxHigherPriorityTaskWoken = pdFALSE;
    display_signal_t global_signal;
    global_signal.signal_type = DIS_GLOBLE_SIGNAL;
    global_signal.signal_value = value;
    xQueueSendFromISR( xDisplayQueue, &global_signal, pxHigherPriorityTaskWoken );
}

void NandFlash_send_data(void);
void NandFlash_receive_data(void);

const PAGE_HANDLE *Current_page = NULL;

APP_TIMER_DEF(m_dis_refresh_tmr);
uint16_t refresh_number = 0xffff;
uint16_t refresh_time_ms = 0xffff;

static void dis_refresh_timeout_handler(void * p_context)
{
    app_timer_start(m_dis_refresh_tmr, APP_TIMER_TICKS(refresh_time_ms), NULL);
    Send_global_type_value(0);
    if(refresh_number > 0)
    {
        refresh_number--;
        if(refresh_number == 0)
        {
            app_timer_stop(m_dis_refresh_tmr);
        }
    }
}

void EnableDynamicRefresh(uint16_t refresh_ms,uint16_t refresh_num)
{
    refresh_number = refresh_num;
    refresh_time_ms = refresh_ms;
    app_timer_start(m_dis_refresh_tmr, APP_TIMER_TICKS(refresh_time_ms), NULL);
}

void JumpPage(const PAGE_HANDLE *page)
{
    app_timer_stop(m_dis_refresh_tmr);
    Current_page = page;
    Current_page->display_func();
}

void display_page_init(void)
{
    uint32_t err_code = NRF_SUCCESS;
    err_code = app_timer_create(&m_dis_refresh_tmr,APP_TIMER_MODE_SINGLE_SHOT,dis_refresh_timeout_handler);
    if(NRF_SUCCESS != err_code)
    {
        return;
    }
    
    Set_color_bit(DISPLAY_OUT_4BIT);
    Current_page = &page_memu;
    Current_page->display_func();
}
static void display_thread(void * arg)
{
//    ret_code_t ret;
    display_signal_t display_signal;
//    uint8_t dis_color = 0xff;
    static uint8_t dis_page_key_up = 0;
    static uint8_t dis_page_key_down = 0;
    UNUSED_PARAMETER(arg);

    lcd_init();
    display_page_init();
    Register_key_send_func(Send_key_value);
    // Enter main loop.
    for (;;)
    {
        /* Waiting for event */
        xQueueReceive(xDisplayQueue,&display_signal,portMAX_DELAY);
//        NRF_LOG_INFO("detected key value:signale = %d,key_value = %d",display_signal.signal_type,display_signal.signal_value); 
        switch(display_signal.signal_type)
        {
            case DIS_KEY_SIGNAL:
            {
#if 0 //flash test code
                if(display_signal.signal_value == KEY_SELECT_VALUE)
                {
                    NandFlash_send_data();
                }
                if(display_signal.signal_value == KEY_NAVIGATION_VALUE)
                {
                    NandFlash_receive_data();
                }

#endif
#if 0 //interrupt test code
                if(display_signal.signal_value == KEY_SELECT_VALUE)
                {
//                    void interrupt_test(void);
//                    interrupt_test();
                }
                if(display_signal.signal_value == KEY_NAVIGATION_VALUE)
                {
                    
                }

#endif
                if(NULL != Current_page->key_handle)
                {
                    Current_page->key_handle(display_signal.signal_value);
                }
            }
            break;
            case DIS_PRIVATE_SIGNAL:
            {
                if(NULL != Current_page->signal_handle)
                {
                    Current_page->signal_handle(display_signal.signal_value);
                }
            }
            break;
            case DIS_GLOBLE_SIGNAL:
            {
                if(NULL != Current_page->display_func)
                {
                    Current_page->display_func();
                }
            }
            break;
            /*add other type signal at here*/
            default:break;
        }
        
#if 0       
        if(key_value == KEY_SELECT_VALUE)
        {
            NandFlash_send_data();
            dis_page_key_up++;
            if(dis_page_key_up >= 6)
            {
                dis_page_key_up = 0;
            }
            
            Set_color_bit(DISPLAY_OUT_4BIT);
            if(0 == dis_page_key_up)
            {
                Lcd_color(0xff);
            
                Display_string(&GUI_Fontweiruanyahei48,40,10,COLOR_DEFAULT,"Hello");
                Display_image(&bmfenda,0,50);
                lcd_display_refresh_all();
            }
            else if(1 == dis_page_key_up)
            {
                Lcd_color(0xff);
            
                Display_string(&GUI_Fontweiruanyahei48,40,10,COLOR_DEFAULT,"fenda");
                Display_image(&bmwearable,0,50);
                lcd_display_refresh_all();
            }
            else if(2 == dis_page_key_up)
            {
                Lcd_color(0xff);
            
                Display_image(&bmmeinu,0,0);
                lcd_display_refresh_all();
            }
            else if(3 == dis_page_key_up)
            {
                Lcd_color(0xff);
            
                Display_image(&bmdianluban,0,0);
                lcd_display_refresh_all();
            }
            else if(4 == dis_page_key_up)
            {
                Lcd_color(0x00);
            
                lcd_display_refresh_all();
            }
            else if(5 == dis_page_key_up)
            {
                Lcd_color(0xff);
            
                lcd_display_refresh_all();
            }

          
        }
        if(key_value == KEY_NAVIGATION_VALUE)
        {
            NandFlash_receive_data();
            dis_page_key_down++;
            if(dis_page_key_down >= 4)
            {
                dis_page_key_down = 0;
            }

            Set_color_bit(DISPLAY_OUT_1BIT);
            if(0 == dis_page_key_down)
            {
                Lcd_color(0xff);//FF:°×É«
            
                Display_string_middle(&GUI_Fontweiruanyahei32,104,104,COLOR_BLACK,"hello ADI");
                lcd_display_refresh_all_1bit();
            }
            else if(1 == dis_page_key_down)
            {
                Lcd_color(0x00);
            
                Display_string_middle(&GUI_Fontweiruanyahei32,104,104,COLOR_WHITE,"hello ADI");
                lcd_display_refresh_all_1bit();
            }
            else if(2 == dis_page_key_down)
            {
                LCD_disp_off();
            }
            else if(3 == dis_page_key_down)
            {
                LCD_disp_on();
            }
            
            
        }
#endif        
    }
}
void display_app_init(void)
{
    xDisplayQueue = xQueueCreate( ( UBaseType_t ) 16, sizeof( display_signal_t ) );

    if (pdPASS != xTaskCreate(display_thread,"DISP",DISPLAY_STACK_SIZE, NULL,DISPLAY_PRIORITY,&m_display_thread))
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }
}


