#include "Test5.h"

#define CAL_FILE_SIZE(x,y)      ((x)*(y))
//#define BAD_BLOCK_SIZE          100
#define MAX_NO_OF_ITER          100000 //1 lakh writes erases 


void Test5(_file_handler *file_handler, _table_file_handler *table_file_handler, struct _memory_buffer *light_fs_mem)
{
//  uint32_t initialize_flag=0;
//  uint32_t ind;
//  uint16_t bad_block_num;
//  uint8_t write_bit;
  uint32_t in_file_size;
  char *file_name_ext;
  elfs_testbench_result file_res = LFS_TESTBENCH_SUCCESS;
  elfs_result err_res = LFS_SUCCESS;
  uint32_t avail_space = 0;
  uint16_t bad_block_array[BAD_BLOCK_SIZE];
  uint16_t bad_block_num;
  uint32_t ind;
  uint8_t write_bit;

#ifdef BAD_BLK_TEST
  
   initialize_circular_buffer();
   
    bool is_bad=false;
   // uint16_t bad_block_array[BAD_BLOCK_SIZE];
    //   Create Bad blocks for test
    for(ind = (TOCBLOCK+1);ind < BAD_BLOCK_SIZE;ind++)
    {
      if(lfs_mark_bad(ind) != LFS_SUCCESS)
      {
        NRF_LOG_INFO("Error in marking %d block bad",ind)
      }
    }
    
    // Create Bad block list
    bad_block_num = 0;
    write_bit=1;
    if(create_bad_block_list (table_file_handler,&bad_block_num,&write_bit)!=LFS_SUCCESS)
    {
      NRF_LOG_INFO("Error in creating bad block list");
    }
    else
    {
      NRF_LOG_INFO("Success, Print all the bad blocks");
      memset(table_file_handler,0,sizeof(_table_file_handler));
      
      // Read Bad Block List
      bad_block_num = 0;
      memset(bad_block_array,0,BAD_BLOCK_SIZE);
      if(read_bad_block_list(table_file_handler,bad_block_array,&bad_block_num) != LFS_SUCCESS)
      {
        NRF_LOG_INFO("Error in reading list");
      }

      NRF_LOG_INFO("List Of Bad Blocks");
      NRF_LOG_INFO("NumOfBad:%d",bad_block_num);
      
      for(ind=0;ind < bad_block_num;ind++)
      {
        NRF_LOG_INFO("%d",bad_block_array[ind]);
      }
      // Free memory
      //free(bad_block_array);
    }
    // Read API
    memset(table_file_handler,0,sizeof(_table_file_handler));
    if(read_table_file_in_toc(table_file_handler) != LFS_SUCCESS)
    {
      NRF_LOG_INFO("Error in reading table");
    }
    
    // Check current block indexes are bad
    for(ind = TOCBLOCK;ind < BAD_BLOCK_SIZE;ind++)
    {
      if(check_current_block_is_bad(ind,table_file_handler,&is_bad) != LFS_SUCCESS)
      {
        NRF_LOG_INFO("Error in checking %d block as bad");
      }
      else
      {
        if(is_bad)
        {
          NRF_LOG_INFO("Block %d is Bad",ind);
        }
        else
        {
          NRF_LOG_INFO("Block %d is good",ind);
        }
      }
    }
    
    // Create All Good before testing further
    for(ind = (TOCBLOCK+1);ind < BAD_BLOCK_SIZE;ind++)
    {
      if(lfs_mark_good(ind) != LFS_SUCCESS)
      {
        NRF_LOG_INFO("Error in checking %d block as good");
      }
    }
    // Create Bad block list
    bad_block_num=0;
    write_bit=1;
    if(create_bad_block_list (table_file_handler,&bad_block_num,&write_bit)!=LFS_SUCCESS)
    {
      NRF_LOG_INFO("Error in creating bad block list");
    }
    else
    {
      bad_block_num=0;
      memset(table_file_handler,0,sizeof(_table_file_handler));
      memset(bad_block_array,0,BAD_BLOCK_SIZE);
      NRF_LOG_INFO("Success,Print all Bad blocks");
      
      read_bad_block_list(table_file_handler,bad_block_array,&bad_block_num);
      
      for(ind=0;ind < bad_block_num;ind++)
      {
        NRF_LOG_INFO("%d",bad_block_array[ind]);
      }
      //    Free memory
      //      free(bad_block_array);
    }
#endif
    
#ifdef CIRCULAR_BUFFER_TEST
    initialize_circular_buffer();
    file_name_ext = (char *)malloc(sizeof(char)*16);
//    in_file_size = CAL_FILE_SIZE(64,2048);
//    in_file_size = CAL_FILE_SIZE(4096,2048);
    
    uint32_t file_counter=0;
       //Call Partial Erase
    if(lfs_erase_memory(true) != LFS_SUCCESS) // for testing head meets tail when partial erase happens 
    {
      NRF_LOG_INFO("Error in Partial Erase");
    }
    
    
        //Create Bad blocks for test
    for(ind = (TOCBLOCK+1);ind < 10;ind++)
    {
      if(lfs_mark_bad(ind) != LFS_SUCCESS)
      {
        NRF_LOG_INFO("Error in marking %d block as bad",ind);
      }
    }
      //Create Bad block list
    bad_block_num = 0;
    write_bit=1;
    
     memset(table_file_handler,0,sizeof(_table_file_handler));
    if(create_bad_block_list (table_file_handler,&bad_block_num,&write_bit)!=LFS_SUCCESS)
    {
      NRF_LOG_INFO("Error in creating bad block list");
    }
    
    //uint32_t avail_space=0;
    //Get the available space left
    if(lfs_get_remaining_space(&avail_space,LFS_DATA_FILE,table_file_handler) != LFS_SUCCESS)
    {
      NRF_LOG_INFO("Error in getting remaining space");
    }

    NRF_LOG_INFO("Remaining Space in bytes=%d",avail_space);
    
    // can be used for testing other than boundary
//    in_file_size = CAL_FILE_SIZE(70,2048);// 8192 pages for 1 file, so 32 files make 4096 blocks
    in_file_size = CAL_FILE_SIZE(64,2048);// 8192 pages for 1 file, so 32 files make 4096 blocks
    for(ind=1;ind <= 5;ind++) // write 5 blocks and erase
    {
      snprintf(file_name_ext,16,"Filename_%d",ind); 
      if(create_and_write_file(file_handler,table_file_handler,light_fs_mem,LFS_DATA_FILE,in_file_size,file_name_ext,LFS_MODE_FAST) != LFS_TESTBENCH_SUCCESS)
      {
        NRF_LOG_INFO("Error in File_%d Creation and Writing",ind);
      }
      else
      {
        file_counter++;
      }
    }
    
  
    //Create Bad blocks for test
    for(ind = 20;ind < 25;ind++)
    {
      if(lfs_mark_bad(ind) != LFS_SUCCESS)
      {
        NRF_LOG_INFO("Error in marking %d block as bad",NULL);
      }
    }

    memset(table_file_handler,0,sizeof(_table_file_handler));
    bad_block_num = 0;
    write_bit = 1;
    if(create_bad_block_list (table_file_handler,&bad_block_num,&write_bit)!=LFS_SUCCESS)
    {
      NRF_LOG_INFO("Error in creating bad block list");
    }
    
    avail_space = 0;
    //Get the available space left
    if(lfs_get_remaining_space(&avail_space,LFS_DATA_FILE,table_file_handler) != LFS_SUCCESS)
    {
      NRF_LOG_INFO("Error in getting remaining space");
    }
    
    // Call Partial Erase
    elfs_result result = lfs_erase_memory(false);
    if(result != LFS_SUCCESS)
    {
      if(result == LFS_TOC_FORMAT_ERROR)
      {
        NRF_LOG_INFO("Error in TOC Memory format");
      }
      else if(result == LFS_FORMAT_ERROR)
      {
        NRF_LOG_INFO("Error in Memory format");
      }
      else if(result == LFS_UPDATE_TABLE_FILE_ERROR)
      {
        NRF_LOG_INFO("Error Updating TOC Table File");
      }
    }
    
    NRF_LOG_INFO("Remaining Space in bytes=%d",avail_space);
    
    in_file_size = CAL_FILE_SIZE(70,2048);// 8192 pages for 1 file, so 32 files make 4096 blocks
    for(ind=1;ind <= 5;ind++) // write 5 blocks and erase
    {
      snprintf(file_name_ext,16,"Filename_%d",ind); 
      if(create_and_write_file(file_handler,table_file_handler,light_fs_mem,LFS_DATA_FILE,in_file_size,file_name_ext,LFS_MODE_FAST) != LFS_TESTBENCH_SUCCESS)
//      if(create_and_write_file(_file_handler,_table_file_handler,light_fs_mem,LFS_DATA_FILE,in_file_size,file_name_ext,LFS_MODE_MANUAL) != LFS_TESTBENCH_SUCCESS)
      {
        NRF_LOG_INFO("Error in File_%d Creation and Writing",ind);
      }
      else
      {
        file_counter++;
      }
    }
    
    avail_space = 0;
    //Get the available space left
    if(lfs_get_remaining_space(&avail_space,LFS_DATA_FILE,table_file_handler) != LFS_SUCCESS)
    {
      NRF_LOG_INFO("Error in getting remaining space");
    }
    
    // Call Partial Erase
    result = lfs_erase_memory(false);
    if(result != LFS_SUCCESS)
    {
      if(result == LFS_TOC_FORMAT_ERROR)
      {
        NRF_LOG_INFO("Error in TOC Memory format");
      }
      else if(result == LFS_FORMAT_ERROR)
      {
        NRF_LOG_INFO("Error in Memory format");
      }
      else if(result == LFS_UPDATE_TABLE_FILE_ERROR)
      {
        NRF_LOG_INFO("Error Updating TOC Table File");
      }
    }
    
    // write complete flash for T>H test
    in_file_size = CAL_FILE_SIZE(8192,2048);// 8192 pages for 1 file, so 32 files make 4096 blocks
    for(ind=6;ind <= 36;ind++) 
    {
      snprintf(file_name_ext,16,"Filename_%d",ind); 
      if(create_and_write_file(file_handler,table_file_handler,light_fs_mem,LFS_DATA_FILE,in_file_size,file_name_ext,LFS_MODE_FAST) != LFS_TESTBENCH_SUCCESS)
      {
        NRF_LOG_INFO("Error in File_%d Creation and Writing",ind);
      }
      else
      {
        file_counter++;
      }
    }
    
    // Create a file of 6500 pages for T<H test
    in_file_size = CAL_FILE_SIZE(6500,2048);// 8192 pages for 1 file, so 32 files make 4096 blocks
    
    snprintf(file_name_ext,16,"Filename_37"); 
    if(create_and_write_file(file_handler,table_file_handler,light_fs_mem,LFS_DATA_FILE,in_file_size,file_name_ext,LFS_MODE_FAST) != LFS_TESTBENCH_SUCCESS)
    {
      NRF_LOG_INFO("Error in File_%d Creation and Writing",ind);
    }
  
    
    avail_space = 0;
    //Get the available space left
    if(lfs_get_remaining_space(&avail_space,LFS_DATA_FILE,table_file_handler) != LFS_SUCCESS)
    {
      NRF_LOG_INFO("Error in getting remaining space");
    }
    
    //Mark  Bad block after write
    
    if(lfs_mark_bad(((table_file_handler->table_file_info.head_pointer)/64+1)) != LFS_SUCCESS)
    {
      NRF_LOG_INFO("Error in marking bad block");
    }

    // Create Bad block list
    bad_block_num =0;
    write_bit = 1;
    if(create_bad_block_list (table_file_handler,&bad_block_num,&write_bit)!=LFS_SUCCESS)
    {
      NRF_LOG_INFO("Error in creating bad block list");
    }
    
    //Get the available space left
    if(lfs_get_remaining_space(&avail_space,LFS_DATA_FILE,table_file_handler) != LFS_SUCCESS)
    {
      NRF_LOG_INFO("Error in getting remaining space");
    }
    
    // Call Partial Erase
    result = lfs_erase_memory(false);
    if(result != LFS_SUCCESS)
    {
      if(result == LFS_TOC_FORMAT_ERROR)
      {
        NRF_LOG_INFO("Error in TOC Memory format");
      }
      else if(result == LFS_FORMAT_ERROR)
      {
        NRF_LOG_INFO("Error in Memory format");
      }
      else if(result == LFS_UPDATE_TABLE_FILE_ERROR)
      {
        NRF_LOG_INFO("Error Updating TOC Table File");
      }
    }
    
    avail_space = 0;
    //Get the available space left
    if(lfs_get_remaining_space(&avail_space,LFS_DATA_FILE,table_file_handler) != LFS_SUCCESS)
    {
      NRF_LOG_INFO("Error in getting remaining space");
    }
    
#endif
  
#ifdef READ_WRITE_TEST
    //Call Partial Erase
    if(lfs_erase_memory(true) != LFS_SUCCESS) // for testing head meets tail when partial erase happens 
    {
      NRF_LOG_INFO("Error in Partial Erase");
    }
    
    //Initialize Circular Buffer 
    if(initialize_circular_buffer() != LFS_SUCCESS)
    {
      NRF_LOG_INFO("Error in Initialize Circular Buffer");
    }
    
    // can be used for testing other than boundary
    in_file_size = CAL_FILE_SIZE(1,2048);// 8192 pages for 1 file, so 32 files make 4096 blocks
//    for(unsigned int ind=1;ind <= 1;ind++) // write 5 blocks and erase
//    {
//      snprintf(file_name_ext,16,"Filename_%d",1); 
      if(create_and_write_file(file_handler,table_file_handler,light_fs_mem,LFS_DATA_FILE,in_file_size,"Filename_1",LFS_MODE_FAST) != LFS_TESTBENCH_SUCCESS)
      {
        NRF_LOG_INFO("Error in File_%d Creation and Writing\r\n",1);
      }
//    }
    
    // writing and reading test
    // open file
//    for(int j=1;j<=1;j++)
//    {
//      uint32_t total_errors=0;
//      sprintf(DebugMsg,"Filename_%d\r\n",1);
      uint32_t total_errors=0;
      if(lfs_open_file_by_name("Filename_1",file_handler) ==LFS_SUCCESS)
      {
        check_file(file_handler,&total_errors);
        NRF_LOG_INFO("Total errors = %d after reading filename_%d\r\n",total_errors,1);
      }
      else
      {
        NRF_LOG_INFO("Filename_%d not found\r\n",1);
      }
//    }  
#endif

#ifdef TEST_BAD_BLOCK
int loop_ind=0;
long int file_write_counter=0;
long int file_erase_counter=0;
file_name_ext = (char *)malloc(sizeof(char)*16);

// remove this while loop if not testing for Bad block
while(loop_ind <= MAX_NO_OF_ITER)
{
   // use this count to have no of Bad blocks
  uint16_t num_bad_blocks = 0;
#endif
  
#ifdef CIRCULAR_BUFFER_TEST_CASES
#ifdef LFS_CIRC_TRAV_1
  
  //Call Partial Erase
  if(lfs_erase_memory(true) != LFS_SUCCESS) // for testing head meets tail when partial erase happens 
  {
    PrintStr("Error in Partial Erase",NULL);
    file_erase_counter++;
  }
  
  //Initialize Circular Buffer 
  if(initialize_circular_buffer() != LFS_SUCCESS)
  {
    NRF_LOG_INFO("Error in Initialize Circular Buffer");
  }
  
#ifdef LFS_BAD_BLK_MANAGE_1
  // Check Initial values
  // Read TOC File in structure to preserve contents
  if(read_table_file_in_toc(table_file_handler) == LFS_ERROR)
  {
    NRF_LOG_INFO("Error in reading Table File for Bad block check");
  }
    
  
  for(ind=0;ind < bad_block_num;ind++)
  {
    NRF_LOG_INFO("bad_block_marker[%d]=%d",ind,table_file_handler->table_file_info.bad_block_marker[ind]);
  }
  
#endif
  
#ifdef LFS_BAD_BLK_MANAGE_3
  // create bad block list
  // Create Bad block list
  bad_block_num = 0;
  uint8_t write_bit=1;
  if(create_bad_block_list (table_file_handler,&bad_block_num,&write_bit)!=LFS_SUCCESS)
  {
    PrintStr("Failure",NULL);
  }
  else
  {
    if(bad_block_num > 0)
    {
#ifdef TEST_BAD_BLOCK
      // No of Bad blocks
      num_bad_blocks = bad_block_num;      
      if(num_bad_blocks > BAD_BLOCK_SIZE)
      {
        NRF_LOG_INFO("100 Bad blocks formed");
        break;
      }
      
      NRF_LOG_INFO("Number Of Bad Blocks:%d",num_bad_blocks);
#endif      
      
      NRF_LOG_INFO("Success,Print all Bad blocks");
      memset(table_file_handler,0,sizeof(_table_file_handler));
      
      // Read Bad Block List
      bad_block_num = 0;
      memset(bad_block_array,0,BAD_BLOCK_SIZE);
      if(read_bad_block_list(table_file_handler,bad_block_array,&bad_block_num) != LFS_SUCCESS)
      {
        NRF_LOG_INFO("Error in reading bad block list",NULL);
      }
      
      for(ind=0;ind < bad_block_num;ind++)
      {
        NRF_LOG_INFO("%d",bad_block_array[ind]);
      }
    }
    else
    {
      NRF_LOG_INFO("No bad blocks");
    }
  }
#endif
  
#endif
#ifdef LFS_CIRC_TRAV_2
  
#ifdef LFS_CIRC_TRAV_12
  
  //Get the available space left
  if(lfs_get_remaining_space(&avail_space,LFS_DATA_FILE,table_file_handler) != LFS_SUCCESS)
  {
    NRF_LOG_INFO("Error in getting remaining space");
  }
#endif    
  in_file_size = CAL_FILE_SIZE(8192,2048);// 8192 pages for 1 file, so 32 files make 4096 blocks
  for(ind=1;ind <= 32;ind++) 
  {
    snprintf(file_name_ext,16,"Filename_%d",ind); 
    if((file_res = create_and_write_file(file_handler,table_file_handler,light_fs_mem,LFS_DATA_FILE,in_file_size,file_name_ext,LFS_MODE_FAST)) != LFS_TESTBENCH_SUCCESS)
    {
      NRF_LOG_INFO("Error in File_%d Creation and Writing",ind);
      
      if(file_res == LFS_TESTBENCH_ERROR_CORRUPTED_FILE)
      {
        NRF_LOG_INFO("Error in File_%d Creation and Writing due to dynamic Bad block",ind);

        // print newly formed Bad block
        // Read Bad Block List
        bad_block_num = 0;
        memset(bad_block_array,0,BAD_BLOCK_SIZE);
        if(read_bad_block_list(table_file_handler,bad_block_array,&bad_block_num) != LFS_SUCCESS)
        {
          NRF_LOG_INFO("Error in reading bad block list",NULL);
        }
        else
        {
          if(bad_block_num > 0)
          {
            NRF_LOG_INFO("Number Of Bad blocks:%d",bad_block_num);
            
            NRF_LOG_INFO("List Of Bad Blocks");
            
            for(ind=0;ind < bad_block_num;ind++)
            {
              NRF_LOG_INFO("%d",bad_block_array[ind]);
            }
          }
        }
      }
    }
    
#ifdef LFS_CIRC_TRAV_14
    //Get the available space left after each file is written
    // H>T case
    avail_space=0;
    if(lfs_get_remaining_space(&avail_space,LFS_DATA_FILE,table_file_handler) != LFS_SUCCESS)
    {
      NRF_LOG_INFO("Error in getting remaining space");
    }
  }
  // increment
  file_write_counter++;
#endif
  
#ifdef LFS_CIRC_TRAV_13
  //Get the available space left after each file is written
  // H>T case
  avail_space=0;
  if(lfs_get_remaining_space(&avail_space,LFS_DATA_FILE,table_file_handler) != LFS_SUCCESS)
  {
    NRF_LOG_INFO("Error in getting remaining space");
  }
#endif
  
#ifdef LFS_CIRC_TRAV_5
  
  if((err_res = lfs_erase_memory(false)) != LFS_SUCCESS) 
  {
    NRF_LOG_INFO("Error in Partial Erase");
    file_erase_counter++;
    
    if(err_res == LFS_BAD_BLOCK_HEADER_CHECK_ERROR)
    {
      NRF_LOG_INFO("Error in Erase due to Bad block");

      // print newly formed Bad block
      // Read Bad Block List
      bad_block_num = 0;
      memset(bad_block_array,0,BAD_BLOCK_SIZE);
      if(read_bad_block_list(table_file_handler,bad_block_array,&bad_block_num) != LFS_SUCCESS)
      {
        NRF_LOG_INFO("Error in reading bad block list",NULL);
      }
      else
      {
        if(bad_block_num > 0)
        {
          NRF_LOG_INFO("Number Of Bad blocks:%d",bad_block_num);
          
          NRF_LOG_INFO("List Of Bad Blocks");
          
          for(ind=0;ind < bad_block_num;ind++)
          {
            NRF_LOG_INFO("%d",bad_block_array[ind]);
          }
        }
      }
    }
  }
  
#endif   
#ifdef LFS_CIRC_TRAV_6
  
  //Call Partial Erase when nothing is written to erase
  if((err_res = lfs_erase_memory(false)) != LFS_SUCCESS) // for testing head meets tail when partial erase happens 
  {
    NRF_LOG_INFO("Error in Partial Erase");
    if(err_res == LFS_NOT_VALID_FORMAT_ERROR)
    {
      NRF_LOG_INFO("Nothing is written to erase");
    }
  }
  
#endif
  
#endif
#ifdef LFS_CIRC_TRAV_3
  
  // Uncomment if LFS_CIRC_12 is undifined
  //      uint32_t avail_space = 0;
  //      uint32_t ind;
  //      uint16_t bad_block_array[BAD_BLOCK_SIZE];
  //      uint16_t bad_block_num;
  //        elfs_result err_res = LFS_SUCCESS;
  
  // write and erase where T>H
  // write few blocks and erase it
  in_file_size = CAL_FILE_SIZE(70,2048);// 70 pages is 1 file
  for(ind=1;ind <= 5;ind++) // write 5 blocks and erase
  {
    snprintf(file_name_ext,16,"Filename_%d",ind); 
    if((file_res = create_and_write_file(file_handler,table_file_handler,light_fs_mem,LFS_DATA_FILE,in_file_size,file_name_ext,LFS_MODE_FAST)) != LFS_TESTBENCH_SUCCESS)
    {
      NRF_LOG_INFO("Error in File_%d Creation and Writing",ind);
    }
    
    if(file_res != LFS_TESTBENCH_SUCCESS)
    {
      NRF_LOG_INFO("Error in File_%d Creation and Writing",ind);
      
      if(file_res == LFS_TESTBENCH_ERROR_CORRUPTED_FILE)
      {
        NRF_LOG_INFO("Error in File_%d Creation and Writing due to dynamic Bad block",ind);

        // print newly formed Bad block
        // Read Bad Block List
        bad_block_num = 0;
        memset(bad_block_array,0,BAD_BLOCK_SIZE);
        if(read_bad_block_list(table_file_handler,bad_block_array,&bad_block_num) != LFS_SUCCESS)
        {
          NRF_LOG_INFO("Error in reading bad block list",NULL);
        }
        else
        {
          if(bad_block_num > 0)
          {
            NRF_LOG_INFO("Number Of Bad blocks:%d",bad_block_num);
            
            NRF_LOG_INFO("List Of Bad Blocks");
            
            for(ind=0;ind < bad_block_num;ind++)
            {
              NRF_LOG_INFO("%d",bad_block_array[ind]);
            }
          }
        }
      }
    }
  }
  // increment write counter
    file_write_counter++;
  
  // Erase memory
  //Call Partial Erase 
  if((err_res = lfs_erase_memory(false)) != LFS_SUCCESS) 
  {
    NRF_LOG_INFO("Error in Partial Erase");
    file_erase_counter++;
    
    if(err_res == LFS_BAD_BLOCK_HEADER_CHECK_ERROR)
    {
      NRF_LOG_INFO("Error in Erase due to Bad block");

      // print newly formed Bad block
      // Read Bad Block List
      bad_block_num = 0;
      memset(bad_block_array,0,BAD_BLOCK_SIZE);
      if(read_bad_block_list(table_file_handler,bad_block_array,&bad_block_num) != LFS_SUCCESS)
      {
        NRF_LOG_INFO("Error in reading bad block list");
      }
      else
      {
        if(bad_block_num > 0)
        {
          NRF_LOG_INFO("Number Of Bad blocks:%d",bad_block_num);
          
          NRF_LOG_INFO("List Of Bad Blocks");
          
          for(ind=0;ind < bad_block_num;ind++)
          {
            NRF_LOG_INFO("%d",bad_block_array[ind]);
          }
        }
      }
    }
  }
  
  in_file_size = CAL_FILE_SIZE(8192,2048);// 8192 pages for 1 file, so 32 files make 4096 blocks
  for(ind=1;ind <= 31;ind++) 
  {
    snprintf(file_name_ext,16,"Filename_%d",ind); 
    if((file_res = create_and_write_file(file_handler,table_file_handler,light_fs_mem,LFS_DATA_FILE,in_file_size,file_name_ext,LFS_MODE_FAST)) != LFS_TESTBENCH_SUCCESS)
    {
      NRF_LOG_INFO("Error in File_%d Creation and Writing",ind);
    }
    
    if(file_res != LFS_TESTBENCH_SUCCESS)
    {
      NRF_LOG_INFO("Error in File_%d Creation and Writing",ind);
      
      if(file_res == LFS_TESTBENCH_ERROR_CORRUPTED_FILE)
      {
        
        NRF_LOG_INFO("Error in File_%d Creation and Writing due to dynamic Bad block",ind);

        // print newly formed Bad block
        // Read Bad Block List
        bad_block_num = 0;
        memset(bad_block_array,0,BAD_BLOCK_SIZE);
        if(read_bad_block_list(table_file_handler,bad_block_array,&bad_block_num) != LFS_SUCCESS)
        {
          NRF_LOG_INFO("Error in reading bad block list",NULL);
        }
        else
        {
          if(bad_block_num > 0)
          {
            NRF_LOG_INFO("Number Of Bad blocks:%d",bad_block_num);
            
            NRF_LOG_INFO("List Of Bad Blocks");
            
            for(ind=0;ind < bad_block_num;ind++)
            {
              NRF_LOG_INFO("%d",bad_block_array[ind]);
            }
          }
        }
      }
    }
   // increment write counter
    file_write_counter++;
    
#ifdef LFS_CIRC_TRAV_15
    //Get the available space left after each file is written
    // T>H case
    avail_space=0;
    if(lfs_get_remaining_space(&avail_space,LFS_DATA_FILE,table_file_handler) != LFS_SUCCESS)
    {
      NRF_LOG_INFO("Error in getting remaining space");
    }
  }
#endif
  
  //Call Partial Erase for T>H case
  if((err_res = lfs_erase_memory(false)) != LFS_SUCCESS) // for testing head meets tail when partial erase happens 
  {
    NRF_LOG_INFO("Error in Partial Erase");
    file_erase_counter++;
    
    if(err_res == LFS_BAD_BLOCK_HEADER_CHECK_ERROR)
    {
      NRF_LOG_INFO("Error in Erase due to Bad block");

      // print newly formed Bad block
      // Read Bad Block List
      bad_block_num = 0;
      memset(bad_block_array,0,BAD_BLOCK_SIZE);

      if(read_bad_block_list(table_file_handler,bad_block_array,&bad_block_num) != LFS_SUCCESS)
      {
        NRF_LOG_INFO("Error in reading bad block list");
      }
      else
      {
        if(bad_block_num > 0)
        {
          NRF_LOG_INFO("Number Of Bad blocks:%d",bad_block_num);
          
          NRF_LOG_INFO("List Of Bad Blocks",NULL);
          
          for(ind=0;ind < bad_block_num;ind++)
          {
            NRF_LOG_INFO("%d",bad_block_array[ind]);
          }
        }
      }
    }
  }
  
#endif
#endif

#ifdef TEST_BAD_BLOCK
  loop_ind++;
  // File write and erase counters
  NRF_LOG_INFO("File write counter=%d, File erase counter = %d",file_write_counter,file_erase_counter);
#endif
}

  NRF_LOG_INFO("Test 5 completed successfully");
}