
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "light_fs_test_bench.h"
#include "light_fs.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"


uint8_t wr_temp_buffer[TEMPBUFFERSIZE];
uint8_t rd_temp_buffer[TEMPBUFFERSIZE];
static uint8_t check_buffer[2*TEMPBUFFERSIZE];
static  uint32_t ind=0;

elfs_testbench_result create_and_write_file(_file_handler *file_handler,_table_file_handler *tmp_table_file_handler,struct _memory_buffer *light_fs_mem,elfs_file_type file_type, uint32_t in_file_size,char *file_name_ext, elfs_op_mode op_mode)
{
  elfs_result result;
  //char *file_name[16];
  char *file_name;

  if(file_name_ext==NULL)
  {
    NRF_LOG_INFO("Enter valid file name");
  }
  else
  {
    //strncpy(file_name,file_name_ext,16);
    file_name = file_name_ext;
  }
  uint32_t file_size;
  
  switch(file_type)
  {
  case LFS_DATA_FILE:
    file_size = in_file_size;
    break;
  case LFS_CONFIG_FILE:
    file_size = 4096;
    break;
  }
  
  NRF_LOG_INFO("Starting new file creation...");
  
  /* Create the first file assigining the file name */
  result = lfs_create_file((uint8_t*)file_name,file_handler,tmp_table_file_handler,light_fs_mem,file_type);
  if(result != LFS_SUCCESS)
  {
    if(result == LFS_CONFIG_FILE_POSITION_ERROR)
      return LFS_TESTBENCH_CONFIG_FILE_POSITION_ERROR;
    
    else if(result == LFS_MAX_FILE_COUNT_ERROR) 
       return LFS_TESTBENCH_ERROR_MAX_FILE_COUNT_ERROR;
      
    else
      return LFS_TESTBENCH_ERROR_CREATING_FILE;
  }
  NRF_LOG_INFO("size: %i, type:%i, handlerNumber:%i, mem_loc:%i,name: \"%s\""  ,file_size,file_type,file_handler->head.header_number,file_handler->head.mem_loc,file_handler->head.file_name);
  lfs_set_operating_mode(file_handler, op_mode);
    
  uint32_t i;
  for(i=0;i<file_size/TEMPBUFFERSIZE+1;i++)
  {
    int32_t write_size = (file_size-i*TEMPBUFFERSIZE) > TEMPBUFFERSIZE ? TEMPBUFFERSIZE : (file_size-i*TEMPBUFFERSIZE);
    
    if(write_size>0)
    {
      gen_file_chunk(file_handler->curr_write_mem_loc+file_handler->tmp_write_mem_loc,write_size,file_size,wr_temp_buffer);
     
      result = lfs_update_file(wr_temp_buffer,write_size,file_handler,tmp_table_file_handler);
      if(result != LFS_SUCCESS)
      {
        if(result == LFS_FILE_WRITE_ERROR)
        {
          NRF_LOG_INFO("File write error");
          return LFS_TESTBENCH_ERROR_CORRUPTED_FILE;
        }
        else 
        {
          NRF_LOG_INFO(" Error updating file");
          return LFS_TESTBENCH_ERROR_UPDATING_FILE;
        }
      }
//      if(op_mode == LFS_MODE_MANUAL && ((i&0xF)==0))
      if(op_mode == LFS_MODE_MANUAL && ((i&0x7)==0))
        lfs_refresh_header(file_handler,tmp_table_file_handler);
    }
  }
  if(lfs_end_file(file_handler,tmp_table_file_handler) != LFS_SUCCESS)
  {
    NRF_LOG_INFO("Error in End of file");
    return LFS_TESTBENCH_ERROR_ENDING_FILE;
  }
  return LFS_TESTBENCH_SUCCESS;
}

elfs_testbench_result create_and_write_config_file(uint8_t *wr_buff,_file_handler *file_handler,_table_file_handler *tmp_table_file_handler,struct _memory_buffer *light_fs_mem,uint32_t in_file_size,char *file_name_ext, elfs_op_mode op_mode)
{
  elfs_result result;
  //char *file_name[16];
  char *file_name;

  if(file_name_ext==NULL)
  {
    NRF_LOG_INFO("Enter valid file name");
  }
  else
  {
    //strncpy(file_name,file_name_ext,16);
    file_name = file_name_ext;
  }
  uint32_t file_size = in_file_size;
  
  NRF_LOG_INFO("Starting new config file creation...,file name=%s",file_name);
  
  /* Create the first file assigining the file name */
  result = lfs_create_file((uint8_t*)file_name,file_handler,tmp_table_file_handler,light_fs_mem,LFS_CONFIG_FILE);
  if(result != LFS_SUCCESS)
  {
    if(result == LFS_CONFIG_FILE_POSITION_ERROR)
      return LFS_TESTBENCH_CONFIG_FILE_POSITION_ERROR;
    
    else if(result == LFS_MAX_FILE_COUNT_ERROR) 
       return LFS_TESTBENCH_ERROR_MAX_FILE_COUNT_ERROR;
      
    else
      return LFS_TESTBENCH_ERROR_CREATING_FILE;
  }
  NRF_LOG_INFO("size: %i, type:%i, handlerNumber:%i, mem_loc:%i,name: \"%s\""  ,file_size,LFS_CONFIG_FILE,file_handler->head.header_number,file_handler->head.mem_loc,file_handler->head.file_name);
  lfs_set_operating_mode(file_handler, op_mode);
    
  uint32_t i;
  //for(i=0;i<file_size/TEMPBUFFERSIZE+1;i++)
 // {
    //int32_t write_size = (file_size-i*TEMPBUFFERSIZE) > TEMPBUFFERSIZE ? TEMPBUFFERSIZE : (file_size-i*TEMPBUFFERSIZE);
    
    //if(write_size>0)
   // {
    //  gen_file_chunk(file_handler->curr_write_mem_loc+file_handler->tmp_write_mem_loc,write_size,file_size,wr_temp_buffer);
    
      NRF_LOG_INFO("In file size = %d",in_file_size);
      result = lfs_update_config_file(wr_buff,in_file_size,file_handler);
      if(result != LFS_SUCCESS)
      {
        if(result == LFS_FILE_WRITE_ERROR)
        {
          NRF_LOG_INFO("File write error");
          return LFS_TESTBENCH_ERROR_CORRUPTED_FILE;
        }
        else 
        {
          NRF_LOG_INFO(" Error updating config file");
          return LFS_TESTBENCH_ERROR_UPDATING_FILE;
        }
      }
//      if(op_mode == LFS_MODE_MANUAL && ((i&0xF)==0))
      if(op_mode == LFS_MODE_MANUAL && ((i&0x7)==0))
        lfs_refresh_config_header(file_handler);
    //}
 // }

  if(lfs_end_config_file(file_handler) != LFS_SUCCESS)
  {
    NRF_LOG_INFO("Error in End of file");
    return LFS_TESTBENCH_ERROR_ENDING_FILE;
  }
  return LFS_TESTBENCH_SUCCESS;
}


elfs_testbench_result check_files(uint32_t * error_counter)
{
  _file_handler tmp_file_handler;
  
  uint8_t file_list[64];
  uint8_t file_number;

  if(lfs_get_file_indexes_list(file_list, &file_number) != LFS_SUCCESS)
  {
    NRF_LOG_INFO("Error in getting file list");
    return LFS_TESTBENCH_ERROR_GETING_FILE_LIST;
  }
  uint8_t i;
  memset(check_buffer,0,sizeof(check_buffer));

  for(i=0;i<file_number;i++)
  {
    if(lfs_open_file_by_number(file_list[i], &tmp_file_handler) ==LFS_ERROR)
    {
      NRF_LOG_INFO("Error in opening file no:%d",file_number);
      return LFS_TESTBENCH_ERROR_OPENING_FILE;  
    }

    if(LFS_TESTBENCH_SUCCESS != check_file(&tmp_file_handler,error_counter))
    { 
      NRF_LOG_INFO("Error in opening file no:%d",file_number);
      return LFS_TESTBENCH_ERROR;
    }
  }
  return LFS_TESTBENCH_SUCCESS;
}

elfs_testbench_result check_config_files(uint8_t *wr_buffer,
                                          uint32_t * error_counter)
{
  _file_handler tmp_file_handler;
  
  bool file_status;

  //consider config file pos 0
  if(lfs_get_config_file_status(file_status) != LFS_SUCCESS)
  {
    NRF_LOG_INFO("Error in getting config file status");
    return LFS_TESTBENCH_ERROR_GETING_CONFIG_FILE_STATUS;
  }
  
  if(lfs_open_file_by_number(CONFIG_FILE_POSITION, &tmp_file_handler) ==LFS_ERROR)
  {
    NRF_LOG_INFO("Error in opening config file no");
    return LFS_TESTBENCH_ERROR_OPENING_FILE;
  }

    //read only configuration files
  if(LFS_TESTBENCH_SUCCESS != check_config_file(wr_buffer,
                                        &tmp_file_handler,
                                        error_counter))
  { 
    NRF_LOG_INFO("Error in opening config file no");
    return LFS_TESTBENCH_ERROR;
  }
  return LFS_TESTBENCH_SUCCESS;
}

elfs_testbench_result check_config_file(uint8_t *wr_buffer,
                                       _file_handler *tmp_file_handler,
                                        uint32_t *error_counter)
{
   NRF_LOG_INFO("Checking file number: %i,type: %i, size: %i, mem_loc=%i,name= \"%s\"...",tmp_file_handler->head.header_number,
   tmp_file_handler->head.file_type,
   tmp_file_handler->head.file_size,
   tmp_file_handler->head.mem_loc,
   nrf_log_push(tmp_file_handler->head.file_name));
   
   uint32_t bytes_read= 0;
     
#if 1  
    while(bytes_read<tmp_file_handler->head.file_size)
    {
      uint32_t read_size = (tmp_file_handler->head.file_size-bytes_read>TEMPBUFFERSIZE) ? TEMPBUFFERSIZE : tmp_file_handler->head.file_size-bytes_read;
          
      memset(rd_temp_buffer,0,read_size);
      if(lfs_read_file(rd_temp_buffer,bytes_read,read_size,tmp_file_handler) != LFS_SUCCESS)
      {
         (*error_counter)++;
        NRF_LOG_INFO("Error reading the file");
        return LFS_TESTBENCH_ERROR_READING_FILE;
      }
      bytes_read += read_size;
    }

    NRF_LOG_INFO("Bytes read=%d",bytes_read);
#endif
#if 0
    if(lfs_read_config_file(rd_temp_buffer,tmp_file_handler) != LFS_SUCCESS)
      {
         (*error_counter)++;
        NRF_LOG_INFO("Error reading the file");
        return LFS_TESTBENCH_ERROR_READING_FILE;
      }
#endif

    uint32_t k;
    for(k=0;k<tmp_file_handler->head.file_size;k++)
    {  
      //NRF_LOG_INFO("wr_buffer[%d] = %d, Rdbuffer[%d]=%d\n",k,wr_buffer[k],k,rd_temp_buffer[k]);
      if(rd_temp_buffer[k] != wr_buffer[k])
      {
       // NRF_LOG_INFO("wr_buffer[%d] = %d, Rdbuffer[%d]=%d\n",k,wr_buffer[k],k,rd_temp_buffer[k]);
        (*error_counter)++;
        NRF_LOG_INFO("Error found after reading %i bytes, position %i: %i/%i, TotalErrors:%i",bytes_read,k,rd_temp_buffer[k],wr_temp_buffer[k],*error_counter);
        if(*error_counter>10);
        {
          NRF_LOG_INFO("Error found after reading %i bytes, position %i: %i/%i, TotalErrors:%i",bytes_read,k,rd_temp_buffer[k],wr_temp_buffer[k],*error_counter);
         // while(1);
        }
      }
    }

    return LFS_TESTBENCH_SUCCESS;
}


elfs_testbench_result check_file(_file_handler *tmp_file_handler,uint32_t *error_counter)
{
   NRF_LOG_INFO("Checking file number: %i,type: %i, size: %i, mem_loc=%i,name= \"%s\"...",tmp_file_handler->head.header_number,
   tmp_file_handler->head.file_type,
   tmp_file_handler->head.file_size,
   tmp_file_handler->head.mem_loc,
   nrf_log_push(tmp_file_handler->head.file_name));
   
   uint32_t bytes_read= 0;
    
    while(bytes_read<tmp_file_handler->head.file_size)
    {
      uint32_t read_size = (tmp_file_handler->head.file_size-bytes_read>TEMPBUFFERSIZE) ? TEMPBUFFERSIZE : tmp_file_handler->head.file_size-bytes_read;
      gen_file_chunk(tmp_file_handler->current_read_pos,read_size,tmp_file_handler->head.file_size,wr_temp_buffer);
      
      memset(rd_temp_buffer,0,read_size);
      if(lfs_read_file(rd_temp_buffer,bytes_read,read_size,tmp_file_handler) != LFS_SUCCESS)
      {
         (*error_counter)++;
        NRF_LOG_INFO("Error reading the file");
        return LFS_TESTBENCH_ERROR_READING_FILE;
      }
      
      uint32_t k;
      for(k=0;k<read_size;k++)
      {  
        if(rd_temp_buffer[k] != wr_temp_buffer[k])
        {
          (*error_counter)++;
          NRF_LOG_INFO("Error found after reading %i bytes, position %i: %i/%i, TotalErrors:%i",bytes_read,k,rd_temp_buffer[k],wr_temp_buffer[k],*error_counter);
          if(*error_counter>10);
          {
            //while(1);
            NRF_LOG_INFO("Error counter while reading file");
            return LFS_TESTBENCH_ERROR_READING_FILE;
          }
        }
      }
      for(k=0;k<read_size;k++)
      {
        check_buffer[ind++]=rd_temp_buffer[k];
      }
      bytes_read += read_size;
    }
    return LFS_TESTBENCH_SUCCESS;
}

elfs_testbench_result delete_all_files()
{
  _file_handler tmp_file_handler;
  uint8_t file_number;
  lfs_get_number_of_files(&file_number);

  uint32_t i;
  for(i=0;i<file_number;i++)
  {
    if(lfs_open_file_by_number(i, &tmp_file_handler) != LFS_SUCCESS)
      return LFS_TESTBENCH_ERROR;
    if(tmp_file_handler.head.file_type != LFS_CONFIG_FILE)
    {
      //LFS_DeleteFile(&tmp_file_handler);
    }
    else
    {
      if(lfs_delete_config_file(&tmp_file_handler) != LFS_SUCCESS)
        return LFS_TESTBENCH_ERROR;
    }
  }
  return LFS_TESTBENCH_SUCCESS;
}

elfs_testbench_result create_bad_block(uint32_t bad_block_index)
{
  if(lfs_mark_bad(bad_block_index) != LFS_SUCCESS)
    return LFS_TESTBENCH_ERROR;
  return LFS_TESTBENCH_SUCCESS;
}

void gen_file_chunk(uint32_t start_pos, uint32_t size, uint32_t file_size, uint8_t * out_buffer)
{
  uint32_t i,j=4;
  //NRF_LOG_INFO("size=%d",size);
    for(int i=0;i<size;i++)
    {
      if(i%0xff==0){j=0;}
      out_buffer[i]=j++;
    }
}
