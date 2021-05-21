#include "Test2.h"

#define RANDOM_BUFF_SIZE    100     /**< Random numbers buffer size. */
#define MAX_NO_FILES_TO_SIMULATE        3
#define NUM_FILES_TO_DELETE             1

void Test2(_file_handler * file_handler, _table_file_handler *table_file_handler, struct _memory_buffer *light_fs_mem)
{
  uint32_t file_size;
  uint32_t total_number_of_files=0;
  uint8_t number_of_files;
  uint8_t number_of_files_to_delete=0;
  uint32_t total_file_size = 0;
  uint32_t erase_iter;
  uint8_t file_number;
  uint32_t total_errors=0;
  uint32_t iter,length;
  char file_name[16];
  NRF_LOG_INFO("Start of Test 2. Write several configuration files and remove them");
  
  for(erase_iter=0;erase_iter<ITERATIONNUMBERT2;erase_iter++)
  {
    // erase completely
    //Format the memory
    if(lfs_format(true) != LFS_SUCCESS)
    {
      NRF_LOG_DEBUG("Error formating the memory");
    }
  
    NRF_LOG_DEBUG("Success formating the memory");
    
    //Create random number of config files with random sizes
#ifdef DATA_FILE_TO_TEST
    uint8_t i;
    number_of_files = MAX_NO_FILES_TO_SIMULATE;
    for(i=0;i<number_of_files;i++)
    {
      memset(file_name,0,16);
      snprintf(file_name,16,"file_name_%d",i);
      if(create_and_write_file(file_handler,table_file_handler,light_fs_mem,LFS_DATA_FILE,file_size,file_name,LFS_MODE_FAST)  != LFS_TESTBENCH_SUCCESS)
      {
        NRF_LOG_DEBUG("Error creating new data file");
        while(1);
      }
      total_file_size+=file_size;
      file_number++;
    }
#else
    for(i=0;i<number_of_files && file_number<64;i++)
    {
      memset(file_name,0,16);
      snprintf(file_name,16,"file_name_%d",i);
      if(create_and_write_file(file_handler,table_file_handler,light_fs_mem,LFS_CONFIG_FILE,file_size,file_name,LFS_MODE_FAST) != LFS_TESTBENCH_SUCCESS)
      {
        NRF_LOG_DEBUG("Error creating new data file");
        while(1);
      }
        total_file_size+=file_size;
        file_number++;
    }
#endif
    if(lfs_get_number_of_files(&file_number) != LFS_SUCCESS)
    {
        NRF_LOG_DEBUG("Error geting the number of files in the memory");
    }

    //Check the files
    check_files(&total_errors);
    
    //Delete some of them
    NRF_LOG_INFO("Removing some of the files...");
    
    _file_handler tmp_file_handler;
    number_of_files_to_delete = NUM_FILES_TO_DELETE;
    uint8_t file_indexes[64];

    if(lfs_get_file_indexes_list(file_indexes, &file_number) != LFS_SUCCESS)
    {
      NRF_LOG_DEBUG("Error geting the file indexes list"); 
    }

#ifndef DATA_FILE_TO_TEST
    uint32_t j=0;
    while(number_of_files_to_delete>0)
    {
      if(lfs_open_file_by_number(file_indexes[j++], &tmp_file_handler)==LFS_SUCCESS)
      {
        if(tmp_file_handler.head.file_type == LFS_CONFIG_FILE)
        {
          NRF_LOG_DEBUG("Removing file headNumber:%i, mem_loc:%i",tmp_file_handler.head.header_number,tmp_file_handler.head.mem_loc);

          if(lfs_delete_config_file(&tmp_file_handler) != LFS_SUCCESS)
          {
            NRF_LOG_DEBUG("Error deleting a config file");
          }
          file_number--;
          number_of_files_to_delete--;
        }
      }
    }
    
    total_number_of_files+=number_of_files;
    NR_LOG_DEBUG("Number of files processed until now: %i, last: %i. Total size: %i",total_number_of_files,number_of_files,total_file_size);
  }
  
#endif
}
  //Erase the memory
  if(lfs_erase_memory(true) != LFS_SUCCESS)
  {
      NRF_LOG_DEBUG("Error erasing the memory");      
  }
  NRF_LOG_DEBUG("Finished Test 2");
}