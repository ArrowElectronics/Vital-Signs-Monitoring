#include "Test1.h"
#include "string.h"
#include "stdio.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"

#define RANDOM_BUFF_SIZE    100     /**< Random numbers buffer size. */
#define MAX_NO_FILES_TO_SIMULATE        4
#define SIZE_OF_EACH_FILE(x,y)          ((x)*(y))
#define MAX_NO_OF_PAGES_PER_ONE_BLOCK   64// change 64
#define SIZE_OF_PAGE                    4096
#define MAX_NO_FILE_NAME_LEN            20


void Test1(_file_handler * file_handler, _table_file_handler *table_file_handler, struct _memory_buffer * light_fs_mem)
{
  uint32_t file_size;
  uint8_t number_of_files;
  uint32_t total_number_of_files=0;
  uint32_t total_file_size = 0;
  uint32_t eraseIter;
  uint32_t total_errors=0;
  uint8_t random_bad_blocks_test[NUMBEROFBADBLOCKS];
  uint8_t length;
  uint32_t iter=0;
 
  NRF_LOG_INFO("Start of Test 1. Write multiple data files with bad blocks in between. Several iterations");
  
  
#ifdef MARK_BAD_BLOCK
        while(iter < 2)
        {
          //Get random number of bytes to write in the current iteration
          length = random_vector_generate(random_bad_blocks_test,NUMBEROFBADBLOCKS);
          NRF_LOG_INFO("Random Vector:");
          NRF_LOG_HEXDUMP_INFO(random_bad_blocks_test, length);
          NRF_LOG_INFO("");
          NRF_LOG_FLUSH();
          nrf_delay_ms(1000);
          iter++;
        }


    NRF_LOG_DEBUG("Marking bad blocks:");
 
    uint8_t j;
    for(j=0;j<NUMBEROFBADBLOCKS;j++)
    {
      random_bad_blocks_test[j]= random_bad_blocks_test[j]%BADBLOCKINDEXLIMIT+FILEBLOCK;
      if(lfs_mark_bad(random_bad_blocks_test[j]) != LFS_SUCCESS)
      {
        NRF_LOG_DEBUG("Error marking block as bad");  
      }
      NRF_LOG_DEBUG("%i,",random_bad_blocks_test[j]);
    }
#endif      

    if(lfs_format(true) != LFS_SUCCESS)
    {
      NRF_LOG_INFO("Error formating the memory");
    }
    else
    {
      NRF_LOG_INFO("Success formating the memory");
    }

    _table_file_handler tmp_table_file_handler;
    memset(&tmp_table_file_handler,0,sizeof(_table_file_handler));

    initialize_circular_buffer(true);
  
    number_of_files = MAX_NO_FILES_TO_SIMULATE;
    file_size = SIZE_OF_EACH_FILE(MAX_NO_OF_PAGES_PER_ONE_BLOCK,SIZE_OF_PAGE);
    NRF_LOG_INFO("Number of files =%d, size of each file=%d",number_of_files,file_size);

    //Create random number of files with random sizes
    uint8_t i;
    char file_name[MAX_NO_FILE_NAME_LEN];

    for(i=1;i<=number_of_files;i++)
    {
      memset(file_name,0,MAX_NO_FILE_NAME_LEN);
      snprintf(file_name,MAX_NO_FILE_NAME_LEN,"file_name_%d",i);
      
      if(create_and_write_file(file_handler,table_file_handler,light_fs_mem,LFS_DATA_FILE,file_size,file_name,LFS_MODE_FAST)  != LFS_TESTBENCH_SUCCESS)
      {
        NRF_LOG_INFO("Error creating new file");
       // while(1);
      }
      total_file_size+=file_size;

      //NRF_LOG_INFO("Erasing memory");
      //Erase the memory
      /*if(lfs_erase_memory(false) != LFS_SUCCESS)
      {
         NRF_LOG_DEBUG("Error erasing the memory");      
      }*/
    }
    NRF_LOG_INFO("======================================================");
    //read files
    check_files(&total_errors);
    //Check the files
    total_number_of_files+=number_of_files;

    NRF_LOG_INFO("Number of files processed until now: %u, last: %u. Total size: %u.Errors:%u",total_number_of_files,number_of_files,total_file_size,total_errors);
    
#ifdef  MARK_BAD_BLOCK 
    for(j=0;j<NUMBEROFBADBLOCKS;j++)
    {
      if(lfs_mark_good(random_bad_blocks_test[j]) != LFS_SUCCESS)
      {
          NRF_LOG_DEBUG("Error marking block as good");   
      }
    }
#endif
#ifdef  MARK_BAD_BLOCK 
    //Erase the memory
    if(lfs_erase_memory(false) != LFS_SUCCESS)
    {
        NRF_LOG_DEBUG("Error erasing the memory\r\n");    
    }
#endif
  NRF_LOG_INFO("Finished Test 1");
}

