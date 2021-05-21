#include "Test0.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"

// All Test cases 
// File handler Memory buffer ( data and pointer to next page , Infomation Msg )
void Test0(_file_handler *file_handler,_table_file_handler *table_file_handler,struct _memory_buffer *light_fs_mem)
{

  bool is_mem_ok;
  NRF_LOG_INFO("Start of Test 0. Format memory and check it");

  //Format the memory
  if(lfs_format(true) != LFS_SUCCESS)
  {
    NRF_LOG_INFO("Error formating the memory");
  }
  else
  {
    NRF_LOG_INFO("Success formating the memory");
  }
  //Check it
  if(lfs_check_comp_version(&is_mem_ok) != LFS_SUCCESS)
  {
    NRF_LOG_INFO("Error checking the memory compatibility");
  }
  else
  {
    NRF_LOG_INFO("lfs check version complete");
  }
  NRF_LOG_INFO("is_mem_ok=%d,false = %d,true = %d",is_mem_ok,false,true);

  if(is_mem_ok == false)
  {
    NRF_LOG_INFO("Error checking the FS version compatibility");
  }
  else
  {
    NRF_LOG_INFO("Success in FS Version compatibility");
  }

  NRF_LOG_INFO("Finished Test 0");
}