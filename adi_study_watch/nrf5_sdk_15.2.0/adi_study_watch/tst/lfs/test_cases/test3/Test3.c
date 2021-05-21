#include "Test3.h"

#define MAX_NO_FILES_TO_SIMULATE        3
#define MAX_NO_OF_PAGES_PER_ONE_BLOCK   64// change and check as test proceeds
#define SIZE_OF_PAGE                    4096
#define FILE_SIZE(x,y)                       ((x)*(y))

char * file_names[] = {"File name 1","Test file 2", "another file","another one","This is what happens with long names","numbers Too123","that's it"};
void Test3(_file_handler *file_handler, _table_file_handler *table_file_handler, struct _memory_buffer *light_fs_mem)
{
  uint32_t file_size;
  uint32_t total_errors=0;
  NRF_LOG_INFO("Start of Test 3. Write several files with different names and check them");

  //Create the random files given the file name
  uint8_t j;
  file_size = FILE_SIZE(SIZE_OF_PAGE,MAX_NO_OF_PAGES_PER_ONE_BLOCK);
  for(j=0;j<NUMBEROFFILES;j++)
  {
    if(create_and_write_file(file_handler,table_file_handler,light_fs_mem,LFS_CONFIG_FILE,file_size,file_names[j],LFS_MODE_FAST)  != LFS_TESTBENCH_SUCCESS)
    {
        NRF_LOG_DEBUG("Error creating new data file");
        while(1);
    }
  }

  //Open the files just created given the file names
  for(j=0;j<NUMBEROFFILES;j++)
  {
    if(lfs_open_file_by_name(file_names[j],file_handler) ==LFS_SUCCESS)
    {
      check_file(file_handler,&total_errors);
    }
    else
    {
      NRF_LOG_INFO("File \"%s\" not found",file_names[j]);
    }
  }  
  //Erase the memory
  if(lfs_erase_memory(true) != LFS_SUCCESS)
  {
      NRF_LOG_INFO("Error erasing the memory");      
  }

  NRF_LOG_INFO("Finished Test 3");
}