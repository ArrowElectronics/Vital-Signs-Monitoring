#include "Test4.h"

#define MAX_NO_FILES_TO_SIMULATE        3
#define MAX_NO_OF_PAGES_PER_ONE_BLOCK   64
#define SIZE_OF_PAGE                    4096
#define FILE_SIZE(x,y)                       ((x)*(y))

char * file_names_test4[] = {"File 1","File 2"};
void Test4(_file_handler *file_handler, _table_file_handler *table_file_handler, struct _memory_buffer *light_fs_mem)
{
  uint32_t file_size;
  uint32_t totalErrors=0;
  _file_handler tmp_file_handler1;
  _file_handler tmp_file_handler2;
  uint32_t total_errors;

  NRF_LOG_INFO("Start of Test 5. Read multiple files at a time");
  
  //Create two random data files
 // memset(table_file_handler,0,sizeof(_table_file_handler));
  file_size = FILE_SIZE(SIZE_OF_PAGE,MAX_NO_OF_PAGES_PER_ONE_BLOCK);
  create_and_write_file(&tmp_file_handler1,table_file_handler,light_fs_mem,LFS_DATA_FILE,file_size,file_names_test4[0],LFS_MODE_FAST);
 // memset(table_file_handler,0,sizeof(_table_file_handler));
  create_and_write_file(&tmp_file_handler2,table_file_handler,light_fs_mem,LFS_DATA_FILE,file_size,file_names_test4[1],LFS_MODE_FAST);

  //Open the files created
  if(lfs_open_file_by_name(file_names_test4[0],file_handler) ==LFS_SUCCESS)
  {
      check_file(file_handler,&total_errors);
  }
  else
  {
      NRF_LOG_INFO("File \"%s\" not found",file_names_test4[0]);
  }

  if(lfs_open_file_by_name(file_names_test4[1],file_handler) ==LFS_SUCCESS)
  {
      check_file(file_handler,&total_errors);
  }
  else
  {
      NRF_LOG_INFO("File \"%s\" not found",file_names_test4[1]);
  }

  //Erase the memory
  if(lfs_erase_memory(true) != LFS_SUCCESS)
  {
      NRF_LOG_INFO("Error erasing the memory");      
  }
  
  NRF_LOG_INFO("Finished Test 4.");
}