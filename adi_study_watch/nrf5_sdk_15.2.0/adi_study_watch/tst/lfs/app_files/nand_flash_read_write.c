#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "nrf_drv_rng.h"
#include "nrf_delay.h"
#include "app_error.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"


// test cases
#include "Test0.h"
#include "Test1.h"
//#include "Test6.h"

#define MEM_SIZE            536870912
#define BLOCK_SIZE          262144
#define PAGE_SIZE           4096
#define PAGES_PER_BLOCK     64

#define test_STACK_SIZE   3000
//#define test_STACK_SIZE   300
#define test_PRIORITY     7

_file_handler file_handler;
_table_file_handler table_file_handler;
struct _memory_buffer light_fs_mem;

static TaskHandle_t m_test2_thread;        /**< USB stack thread. */

void Nand_Flash_Test_thread(void * arg)
{    
    UNUSED_PARAMETER(arg);
    uint32_t err_code;
    nand_flash_init();

   // err_code = nrf_drv_rng_init(NULL);
   // APP_ERROR_CHECK(err_code);

    /********** Test beginning ******************************/
   _memory_properties mem_prop = {.mem_size = MEM_SIZE,
                              .block_size = BLOCK_SIZE,
                              .page_size = PAGE_SIZE,
                              .pages_per_block = PAGES_PER_BLOCK};
    //Unlock all the memory
  nand_func_set_block_lock(NAND_FUNC_BLOCK_LOCK_ALL_UNLOCKED);

  //Open the filesystem
  lfs_openfs(&mem_prop);

  while(1){
        //add test cases here 
          Test0(&file_handler,&table_file_handler,&light_fs_mem);
          vTaskDelay(5000);
          //Test1(&file_handler,&table_file_handler,&light_fs_mem);
          //vTaskDelay(5000);
        //  Test6(&file_handler,&table_file_handler,&light_fs_mem);
        //  vTaskDelay(5000);
     }
}


void Nand_Flash_Test_Init(void)
{
    if (pdPASS != xTaskCreate(Nand_Flash_Test_thread,"DISP",test_STACK_SIZE, NULL,test_PRIORITY,&m_test2_thread))
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }
    else
    {
        NRF_LOG_INFO("test_init create success");
    }
}