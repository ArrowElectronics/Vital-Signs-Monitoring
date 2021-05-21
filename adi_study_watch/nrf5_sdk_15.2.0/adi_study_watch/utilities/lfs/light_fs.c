/**
***************************************************************************
* @addtogroup Tasks
* @{
* @file         light_fs.c
* @author       ADI
* @version      V1.0.1
* @date         16-June-2020
* @brief        Source file contains major functionalities/modules
                required for file operations.
***************************************************************************
* @attention
***************************************************************************
*/
/*!
*  \copyright Analog Devices
* ****************************************************************************
*
* License Agreement
*
* Copyright (c) 2020 Analog Devices Inc.
* All rights reserved.
*
* This source code is intended for the recipient only under the guidelines of
* the non-disclosure agreement with Analog Devices Inc.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER  LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING  FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER  DEALINGS IN THE SOFTWARE.
* ****************************************************************************
*/
#ifdef USE_FS
#include "light_fs.h"
#include "hw_if_config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "file_system_utils.h"
#include "nrf_log_ctrl.h"
#ifdef PROFILE_TIME_ENABLED
#include "us_tick.h"
#endif

#define NRF_LOG_MODULE_NAME LFS
#if LFS_CONFIG_LOG_ENABLED
#ifdef NRF_LOG_DEFAULT_LEVEL
#undef NRF_LOG_DEFAULT_LEVEL
#define NRF_LOG_DEFAULT_LEVEL LFS_CONFIG_LOG_LEVEL
#endif
#define NRF_LOG_INFO_COLOR  LFS_CONFIG_INFO_COLOR
#define NRF_LOG_DEBUG_COLOR LFS_CONFIG_DEBUG_COLOR
#else /* LFS_CONFIG_LOG_ENABLED */
#define NRF_LOG_LEVEL       0
#endif /* LFS_CONFIG_LOG_ENABLED */

#include "nrf_log.h"

/* enable logger module */
NRF_LOG_MODULE_REGISTER();

/************************************************* Global variables ***********************************************************/
#ifdef PROFILE_TIME_ENABLED
uint64_t g_page_write_time;
uint64_t g_next_page_read_time;
uint64_t g_page_hold_write_time;
uint32_t nPageTick1,nGetNextPageReadTick1;

extern const nrfx_timer_t TIMER_INC_MICRO_SEC;
uint32_t get_micro_sec(void);
static uint32_t nTick1 = 0,nTick2 = 0;
uint32_t file_header_update_time=0;
#endif

/************************************************* Private functions ***********************************************************/
static elfs_result lfs_update_header(_file_handler *file_handler,
                                    _table_file_handler *table_file_handler);
static elfs_result lfs_set_read_pos(uint32_t read_pos,
                                   _file_handler *file_handler);
static elfs_result lfs_read_oob(uint32_t page,
                                struct _page_header *page_head);
static elfs_result lfs_delete_page(uint32_t page);
static elfs_result get_first_header_void(_table_file_handler *table_file_handler,
                                        elfs_file_type file_type);
static elfs_result get_first_config_page_free(uint32_t * first_page);
static elfs_result get_next_writable_page(_file_handler *file_handler,
                                        _table_file_handler *tmp_table_file_handler);
static elfs_result get_next_page_files(_file_handler *file_handler,
                                      _table_file_handler *tmp_table_file_handler);
static elfs_result update_table_file_in_toc(_table_file_handler *table_file_handler,
                                            elfs_update_type type[],uint16_t num_of_var_to_be_updated);

static elfs_result write_table_file_in_toc(_table_file_handler *table_file_handler);
static elfs_result create_bad_block_list (_table_file_handler *table_file_handler,
                                         uint32_t *bad_block_num);

static elfs_result check_current_block_is_bad(uint32_t blk_ind,
                                              _table_file_handler *table_file_handler,
                                              bool *is_bad);
static elfs_result read_bad_block_list (_table_file_handler *table_file_handler,
                                        uint32_t *bad_block_array,
                                        uint32_t *bad_blk_cnt);
static elfs_result lfs_update_config_header(_file_handler *file_handler);
static elfs_result get_next_writable_config_page(uint32_t * first_page);
static elfs_result lfs_delete_toc_pages(uint32_t page_pos_arr[],int16_t page_num_to_del);

_memory_properties * g_mem_prop;
struct _spi_nand_chip * g_nand_chip_driver_handler;
_fs_info file_system_info = {.foot_print = FSFOOTPRINT,
                         .version = FSVERSION,
                         .revision = FSREVISION};
_fs_info file_system_info;
extern vol_info_buff_t vol_info_buff_var;
static uint8_t g_file_indexes[MAXFILENUMBER];
static uint8_t g_file_count;
static volatile uint32_t g_total_file_size;
static uint32_t pos_ind_to_del[NUM_OF_TOC_PAGES_TO_DEL_FOR_HEADER_UPD];
elfs_update_type type[MAX_UPDATE_TYPE];
uint8_t volatile del_table_page=0;

#ifdef PROFILE_TIME_ENABLED
uint32_t page_update_flag, lfs_refresh_header_time=0,t1;
#endif


/*!
  ****************************************************************************
  *@brief      Open the filesystem. This function will assign the flash chip handler
               so that its parameters can be used along the file system.
  *@param      _memoryProperties * mem_prop: Nand chip properties struct
  *@return     elfs_result Function result: LFS_SUCCESS
  ******************************************************************************/
elfs_result lfs_openfs(_memory_properties * mem_prop)
{

  /* if structure pointer is not assigned, return error as no
    memory is assigned */
  if(mem_prop== NULL) {
    return LFS_ERROR;
  }
  g_mem_prop = mem_prop;

  /* enable ecc */
  nand_func_enable_ecc(true);

  return LFS_SUCCESS;
}


/*!
  **********************************************************************************
  *@brief      Read table page ( last page of toc ) to get pointer information,
                bad blocks and required flags information
  *@param      _table_file_handler *_table_file_handler: Table page struct pointer
  *@return     elfs_result Function result: LFS_SUCCESS/LFS_ERROR
  **********************************************************************************/
elfs_result read_table_file_in_toc(_table_file_handler *table_file_handler)
{
  eNand_Func_Result result=NAND_FUNC_SUCCESS;

  /* page position to read from toc block */
  uint32_t page_pos=TOCBLOCK*g_mem_prop->pages_per_block+TABLE_PAGE_INDEX_IN_TOC;
  /* read from memory */
  result = nand_func_read(g_mem_prop,page_pos*g_mem_prop->page_size,\
           sizeof(_table_file_handler),(uint8_t*)table_file_handler);

#ifdef PRINTS_OUT
  NRF_LOG_INFO("Size for reading:%d",sizeof(_table_file_header));
  NRF_LOG_INFO("Read Table File Contents In Structure: head_pointer=%d,\
                  tail_pointer=%d",
                  table_file_handler->table_file_info.head_pointer,
                  table_file_handler->table_file_info.tail_pointer);
#endif

  /* data read from flash success */
  if(result == NAND_FUNC_SUCCESS) {
#ifdef PRINTS_OUT
    NRF_LOG_INFO("Successfully reading Table File from TOC");
#endif
    return LFS_SUCCESS;
  }
  /* data read from flash failure */
  else  {
    NRF_LOG_INFO("Error in reading Table File from TOC");
    return LFS_ERROR;
  }
}

/*!
  **********************************************************************************
  *@brief      Read configuration file from configuration block
  *@param      *out_buffer: pointer to soft buffer where data is read,
  *@param      _file_handler *_file_handler: pointer to file handler information
               structure.
  *@return     elfs_result Function result: LFS_SUCCESS/LFS_ERROR
  **********************************************************************************/
elfs_result lfs_read_config_file(uint8_t *out_buffer,
                                _file_handler *file_handler) {
  eNand_Func_Result result=NAND_FUNC_SUCCESS;

   /* page position to read from config block */
  uint32_t page_pos=CFGBLOCK*g_mem_prop->pages_per_block+CONFIG_FILE_POSITION;
#ifdef PRINTS_OUT
  NRF_LOG_INFO("lfs_read_config_file:Page pos to read=%d",page_pos);
  NRF_LOG_INFO("lfs_read_config_file:File size to read =%d",file_handler->head.file_size);
#endif
  /* read from memory */
  result = nand_func_read(g_mem_prop,page_pos*g_mem_prop->page_size,\
                          file_handler->head.file_size,out_buffer);

  /* data read from flash success */
  if(result == NAND_FUNC_SUCCESS) {
#ifdef PRINTS_OUT
    NRF_LOG_INFO("Successfully reading Table File from TOC");
#endif
    return LFS_SUCCESS;
  }
  /* data read from flash failure */
  else  {
    NRF_LOG_INFO("Error in reading Table File from TOC");
    return LFS_ERROR;
  }
}


/*!
  **********************************************************************************
  *@brief      Write table page ( last page of toc ) to update pointer information,
                bad blocks and required flags information
  *@param      _table_file_handler *_table_file_handler: Table page struct pointer
  *@return     elfs_result Function result: LFS_SUCCESS/LFS_ERROR
  **********************************************************************************/
static elfs_result write_table_file_in_toc(_table_file_handler *table_file_handler) {
  eNand_Func_Result result = NAND_FUNC_SUCCESS;

   /* assign to temporary table file header */
  _table_file_header *tmp_table_file_header=&table_file_handler->table_file_info;
  struct _page_header page_info;

  /* page position to write from toc block */
  uint32_t page_pos=TOCBLOCK*g_mem_prop->pages_per_block+TABLE_PAGE_INDEX_IN_TOC;

  /*update spare area information */
  page_info.next_page=0xFFFFFFFF;
  page_info.occupied=1;

  /* write data plus spare area onto toc last page of flash */
  struct _page_write write_data = {.page_dest=page_pos,
  .data_buf=(uint8_t*)tmp_table_file_header,
  .data_size=sizeof(_table_file_header),
  .offset=DATA_OFFSET,
  .spare_buf=(uint8_t*)&page_info,
  .spare_size=sizeof(struct _page_header),
  .spare_offset=SPARE_OFFSET};

  result = nand_func_page_and_spare_write(&write_data);

  /* data write from flash success */
  if(result == NAND_FUNC_SUCCESS) {
#ifdef PRINTS_OUT
    NRF_LOG_INFO("**********************Writing sucessfully in TOC ******************");
#endif
    return LFS_SUCCESS;
  }

  /* data write from flash failure */
  else  {
    NRF_LOG_INFO("Writing error of Table File in TOC");
    return LFS_ERROR;
  }
}

/*!
  **********************************************************************************
  *@brief       Check current block is bad/good by reading last page of toc where
                bad block information is stored.
  *@param       blk_ind: block index to check bad/good
  *@param       _table_file_handler *_table_file_handler: Table page struct pointer
  *@param       is_bad: return bad block information if block is bad/good
  *@return     elfs_result Function result: LFS_SUCCESS/LFS_ERROR
  **********************************************************************************/
static elfs_result check_current_block_is_bad(uint32_t blk_ind,
                                              _table_file_handler *table_file_handler,
                                              bool *is_bad) {
   uint32_t word_pointer;
   uint32_t bit_pointer;
   uint8_t bit_val;
   uint32_t block_word;

   /* if block index is greater than total number of blocks (4096) block index passed\
    is error */
   if(blk_ind > g_mem_prop->num_of_blocks)  {
     return LFS_NUM_BLOCKS_OUT_OF_BOUNDS_ERROR;
   }
   /* calculate word pointer based on block index and size of each word */
   word_pointer=blk_ind / MAX_NO_OF_BITS_IN_WORD;
   /* calculate bit pointer based on block index and size of each word */
   bit_pointer=blk_ind % MAX_NO_OF_BITS_IN_WORD;
   /* access group of blocks information based on word pointer */
   block_word = table_file_handler->table_file_info.bad_block_marker[word_pointer];

   /* access block information based on bit pointer, and block word */
   bit_val = ((block_word & (1 << bit_pointer))==0) ? 0:1;

   /* if bit is '1', block has gone bad, return true */
   if(bit_val) {
     *is_bad = true;
   }
   /* if bit is '0', block has gone bad, return false */
   else {
     *is_bad = false;
   }

   return LFS_SUCCESS;
}


/*!
  **********************************************************************************
  *@brief       Create Bad block list by reading each block information from flash
                and updating on to soft buffer where all block information is
                stored.
  *@param      _table_file_handler *_table_file_handler: Table page struct pointer
  *@param      bad_block_num: number of bad blocks
  *@return     elfs_result Function result: LFS_SUCCESS/LFS_ERROR
  **********************************************************************************/
static elfs_result create_bad_block_list (_table_file_handler *table_file_handler,
                                         uint32_t *bad_block_num) {
  bool is_bad_block = false;
  uint32_t blk_index = 0;
  uint32_t bit_pointer = 0;
  uint32_t word_pointer = 0;
  /* initialize type array for bad block update */
  memset(type,0,MAX_UPDATE_TYPE);
  type[0] = LFS_BAD_BLOCK_MARKER_UPDATE;

  for(blk_index = RESERVEDBLOCK;blk_index < g_mem_prop->num_of_blocks;blk_index++)  {
    /* check if current block is bad/good by reading from driver */
    if(nand_func_is_bad_block(g_mem_prop,blk_index,&is_bad_block) \
                              != NAND_FUNC_SUCCESS) {
    /* if driver api returns failure, then return error */
#ifdef PRINTS_OUT
      NRF_LOG_INFO("Unable to check %d block is bad/good",blk_index);
#endif
      return LFS_BAD_BLOCK_HEADER_CHECK_ERROR;
    }
    else  {
      /* calculate word/bit pointers required for current block */
      word_pointer = blk_index/MAX_NO_OF_BITS_IN_WORD;
      bit_pointer = blk_index%MAX_NO_OF_BITS_IN_WORD;

      if(is_bad_block == true)  {
        /* Set bit for bad block */
        table_file_handler->table_file_info.bad_block_marker[word_pointer] |= (1 << bit_pointer);
        (*bad_block_num)++;
      }
      else  {
        /* Clear bit for good block */
        table_file_handler ->table_file_info.bad_block_marker[word_pointer] &= ~(1 << bit_pointer);
      }
    }
  }

  /* flag taken for writing on to toc */
  del_table_page = 1;
  /* Update table page in toc with bad block information */
  if(update_table_file_in_toc(table_file_handler,type,1) != LFS_SUCCESS)  {
    /* if updation of information is error, return error */
    NRF_LOG_INFO("Error in UpdateFileinTOC for Bad Block");
    return LFS_UPDATE_TABLE_FILE_ERROR;
  }
  else{
      /* if updation of information is success*/
#ifdef PRINTS_OUT
      NRF_LOG_INFO("Success in UpdateFileinTOC for Bad Block");
#endif
  }
    /* reset flag taken for next writing on to toc */
  del_table_page =0;
  return LFS_SUCCESS;
}


/*!
  **********************************************************************************
  *@brief       This provides list of Bad block Indexes
  *@param      _table_file_handler *_table_file_handler: Table page struct pointer
  *@param       bad_block_array: pointer to bad block aray
  *@param       bad_blk_cnt: pointer to number of bad blocks
  *@return     elfs_result Function result: LFS_SUCCESS/LFS_ERROR
  **********************************************************************************/
static elfs_result read_bad_block_list (_table_file_handler *table_file_handler,
                                        uint32_t *bad_block_array,
                                        uint32_t *bad_blk_cnt)  {
  uint32_t blk_index;
  uint32_t bit_pointer;
  uint32_t word_pointer;
  uint8_t bit_val;

  /* Read table page in structure to preserve contents */
  if(read_table_file_in_toc(table_file_handler) == LFS_ERROR) {
    return LFS_ERROR;
  }

  for(blk_index=RESERVEDBLOCK;blk_index < g_mem_prop->num_of_blocks;blk_index++)  {
    /* calculate word, bit pointers for corresponding block index */
    word_pointer = blk_index / MAX_NO_OF_BITS_IN_WORD;
    bit_pointer = blk_index % MAX_NO_OF_BITS_IN_WORD;

    /* calculate bit value stored from block index to check block is
        good / bad*/
    bit_val = (((table_file_handler->table_file_info.bad_block_marker[word_pointer])\
            &(1 << bit_pointer))==0) ? 0:1;

    /* if bit is '1', update bad block array, and increment bad block count */
    if(bit_val) {
      bad_block_array[(*bad_blk_cnt)] = blk_index;
      (*bad_blk_cnt)++;
    }
  }
  return LFS_SUCCESS;
}

/*!
  *******************************************************************************************
  *@brief       This function provides updation of different variables for last page of TOC.

  *@param      _table_file_handler *_table_file_handler: Table page struct pointer
  *@param       type: elfs_update_type, select from below parameters
                LFS_BAD_BLOCK_MARKER_UPDATE: Bad blocks update
                LFS_HEAD_POINTER_UPDATE: Head pointer updated
                LFS_TAIL_POINTER_UPDATE: Tail pointer update
                LFS_INITIALIZE_BUFFER_FLAG_UPDATE: Initialized circular buffer update
                LFS_MEM_FULL_FLAG_UPDATE: Memory full flag update
                LFS_OFFSET_UPDATE: Offset update
                LFS_CONFIG_FILE_POS_OCCUPIED_UPDATE: Low touch config position update
  *@param       num_of_var_to_be_updated: number of variables to be updated
  *@return     elfs_result Function result: LFS_SUCCESS/LFS_ERROR
  ********************************************************************************************/
static elfs_result update_table_file_in_toc(_table_file_handler *table_file_handler,
                                            elfs_update_type type[],
                                            uint16_t num_of_var_to_be_updated)  {
  _table_file_handler tmp_table_file_handler;
  uint32_t word_index=0;
  uint32_t bit_index=0;
  uint32_t bit_val=0;

  /* Read TOC File in structure to preserve contents */
  memset(&tmp_table_file_handler,0,sizeof(_table_file_handler));

  if(read_table_file_in_toc(&tmp_table_file_handler) == LFS_ERROR)  {
#ifdef PRINTS_OUT
    NRF_LOG_INFO("Error in reading Table file");
#endif
    return LFS_ERROR;
  }
  else
  {
    for(int i=0;i < num_of_var_to_be_updated;i++) {
      switch(type[i])
      {
        case LFS_BAD_BLOCK_MARKER_UPDATE:
        /*  Update Bad block Information */
        for(word_index=0;word_index < MAX_NO_OF_WORDS;word_index++) {
          /** compare input temp handler bad block word value with read temp handler
              bad block word, if change update **/
          if(table_file_handler->table_file_info.bad_block_marker[word_index] \
             != tmp_table_file_handler.table_file_info.bad_block_marker[word_index])  {

            for(bit_index=0;bit_index < MAX_NO_OF_BITS_IN_WORD;bit_index++) {
              /** calculate bit value to know whether bad block / good block **/
              bit_val = ((table_file_handler->table_file_info.bad_block_marker[word_index]\
                        & (1 << bit_index)) == 0) ? 0:1;

              /* Set bit in read data if that particular block bit needs updation */
              if( bit_val == 1 ) {
                tmp_table_file_handler.table_file_info.bad_block_marker[word_index] |= (1 << bit_index);
              }
              /* Clear bit in read data if that particular block bit needs updation */
              else  {
                tmp_table_file_handler.table_file_info.bad_block_marker[word_index] &= ~(1 << bit_index);
              }
            }
          }
        }
        break;
        case LFS_HEAD_POINTER_UPDATE:
        /** update head pointer **/
#ifdef PRINTS_OUT
        NRF_LOG_INFO("Head Pointer required to update:%d",table_file_handler->table_file_info.head_pointer);
#endif
        if(tmp_table_file_handler.table_file_info.head_pointer \
          != table_file_handler->table_file_info.head_pointer)  {
          tmp_table_file_handler.table_file_info.head_pointer=table_file_handler->table_file_info.head_pointer;
        }
        break;

        case LFS_TAIL_POINTER_UPDATE:
        /** update tail pointer **/
#ifdef PRINTS_OUT
        NRF_LOG_INFO("Tail Pointer required to update:%d",table_file_handler->table_file_info.tail_pointer);
#endif
        if(tmp_table_file_handler.table_file_info.tail_pointer \
          != table_file_handler->table_file_info.tail_pointer) {
          tmp_table_file_handler.table_file_info.tail_pointer=table_file_handler->table_file_info.tail_pointer;
        }
        break;

        case LFS_INITIALIZE_BUFFER_FLAG_UPDATE:
        /** circular buffer flag update **/
#ifdef PRINTS_OUT
        NRF_LOG_INFO("Initialize circular buffer flag to update:%d",table_file_handler->table_file_info.initialized_circular_buffer);
#endif
        if(tmp_table_file_handler.table_file_info.initialized_circular_buffer \
          != table_file_handler->table_file_info.initialized_circular_buffer) {
          tmp_table_file_handler.table_file_info.initialized_circular_buffer=table_file_handler->table_file_info.initialized_circular_buffer;
        }
        break;

        case LFS_MEM_FULL_FLAG_UPDATE:
        /** memory full flag update **/
#ifdef PRINTS_OUT
        NRF_LOG_INFO("Mem full flag to update:%d",table_file_handler->table_file_info.mem_full_flag);
#endif
        if(tmp_table_file_handler.table_file_info.mem_full_flag \
          != table_file_handler->table_file_info.mem_full_flag) {
          tmp_table_file_handler.table_file_info.mem_full_flag=table_file_handler->table_file_info.mem_full_flag;
        }
        break;

        case LFS_OFFSET_UPDATE:
         /** offset update **/
#ifdef PRINTS_OUT
        NRF_LOG_INFO("Offset required to update:%d",table_file_handler->table_file_info.offset);
#endif
        if(tmp_table_file_handler.table_file_info.offset \
          != table_file_handler->table_file_info.offset) {
          tmp_table_file_handler.table_file_info.offset=table_file_handler->table_file_info.offset;
        }
        break;

       case LFS_CONFIG_FILE_POS_OCCUPIED_UPDATE:
       /** config position occupied update **/
#ifdef PRINTS_OUT
        NRF_LOG_INFO("Config file pos req to update:%d",table_file_handler->table_file_info.config_low_touch_occupied);
#endif
        if(tmp_table_file_handler.table_file_info.config_low_touch_occupied \
          != table_file_handler->table_file_info.config_low_touch_occupied)
        {
          tmp_table_file_handler.table_file_info.config_low_touch_occupied=table_file_handler->table_file_info.config_low_touch_occupied;
        }
       break;

        default:
          break;
      }
    }
  }

 /* if set to '1' it will delete table page in toc ,
    this excludes update header where it deletes 2 consecutive pages */
 if(del_table_page){
  /* Delete table page in toc block */
  if(lfs_delete_page(TOCBLOCK*g_mem_prop->pages_per_block+TABLE_PAGE_INDEX_IN_TOC) == LFS_ERROR)  {
    NRF_LOG_INFO("Error deleting table file in TOC");
    return LFS_ERROR;
  }
 }
  /* Write Table page in TOC */
  if(write_table_file_in_toc(&tmp_table_file_handler) == LFS_ERROR) {
   /** return success if updation success **/
    NRF_LOG_INFO("Write Table File in Update TOC is Error");
    return LFS_ERROR;
  }
  /** return success if updation success **/
  else  {
    return LFS_SUCCESS;
  }
}

/*!
  **********************************************************************************
  *@brief       This function called to initialize circular buffer
  *@param      None
  *@return     elfs_result Function result: LFS_SUCCESS/LFS_ERROR
  **********************************************************************************/
elfs_result initialize_circular_buffer()  {
  _table_file_handler tmp_table_file_handler;
  uint32_t bad_block_num;
  memset(&tmp_table_file_handler,0,sizeof(_table_file_handler));

  bad_block_num=0;

  /* Run through the list and create bad block list*/
  if(create_bad_block_list (&tmp_table_file_handler,&bad_block_num) != LFS_SUCCESS) {
#ifdef PRINTS_OUT
      NRF_LOG_INFO("Error in creating bad block list");
#endif
     return LFS_ERROR;
  }


  /* Initialize head and tail pointers to FILEBLOCK */
  tmp_table_file_handler.table_file_info.head_pointer=FILEBLOCK*g_mem_prop->pages_per_block;
  tmp_table_file_handler.table_file_info.tail_pointer=FILEBLOCK;


  /*Set initialized circular buffer flag, to 1 indicating buffers inited FILEBLOCK,
   which is used to distinguish between memory full/empty */
  tmp_table_file_handler.table_file_info.initialized_circular_buffer = 1;

  /* Delete table page and write */
  if(lfs_delete_page(TOCBLOCK*g_mem_prop->pages_per_block+TABLE_PAGE_INDEX_IN_TOC) == LFS_ERROR)  {
     NRF_LOG_INFO("Error deleting table file in TOC Block");
     return LFS_ERROR;
  }

  /* Write Table page in TOC */
  if(write_table_file_in_toc(&tmp_table_file_handler) == LFS_ERROR) {
    /* return if writing page is error */
     NRF_LOG_INFO("Error in writing table file");
     return LFS_ERROR;
  }
#ifdef DEBUG_CODES
  memset(&tmp_table_file_handler,0,sizeof(tmp_table_file_handler));
  uint32_t bad_block_array[3],bad_blk_cnt=0;
  if(read_bad_block_list (&tmp_table_file_handler,bad_block_array,&bad_blk_cnt) != LFS_SUCCESS)
  {
    NRF_LOG_INFO("Error in reading bad block list");
    return LFS_ERROR;
  }
  NRF_LOG_INFO("******** Bad block no - %d ********",bad_blk_cnt);
#endif
  return LFS_SUCCESS;
}

/*!
  **********************************************************************************
  *@brief      Obtain the number of files in the memory based on the TOC

  *@param      *number_of_files: Pointer to number of files
  *@return     elfs_result Function result: LFS_SUCCESS/LFS_ERROR
  **********************************************************************************/
elfs_result lfs_get_number_of_files(uint8_t * number_of_files)  {
  uint32_t pages_per_block = g_mem_prop->block_size/g_mem_prop->page_size;
  *number_of_files = 0;
  uint8_t i;
  for(i = 0;i < pages_per_block;i++)  {
    /* read page header to check if page in TOC is occupied */
    struct _page_header  page_head;
    if(lfs_read_oob(TOCBLOCK*pages_per_block+i,&page_head) == LFS_ERROR)  {
      /* return error if error reading spare area */
      NRF_LOG_ERROR("Error in reading Total number of files");
      return LFS_ERROR;
    }
    /* if page occupied is '1', increment number of files to get file count
      in to toc */
    if(page_head.occupied == 1)
      (*number_of_files)++;
  }
#ifdef PRINTS_OUT
  NRF_LOG_INFO("Number of files in TOC=%d",*number_of_files);
#endif
  return LFS_SUCCESS;
}

/*!
  **********************************************************************************
  *@brief      Obtain the file size
  *@param      _file_handler * _file_handler: File handler to obtain the size of file
  *@param      *file_size: returned size of the file
  *@return     elfs_result Function result: LFS_SUCCESS/LFS_ERROR
  **********************************************************************************/
elfs_result lfs_get_file_size(_file_handler *file_handler,
                              uint32_t * file_size)
{
  /* return if file handler is null */
  if(file_handler==NULL)  {
    return LFS_ERROR;
  }
  /* copy file size from file handler */
  *file_size=file_handler->head.file_size;
  return LFS_SUCCESS;
}

/*!
  **************************************************************************************************
  *@brief      Get bad block number
  *@param       *bad_block_num: number of bad blocks
  *@param       src_ind: source block
  *@param       end_ind: end block index
  *@param       _table_file_handler *table_file_handler: table page handler to read from last page of toc
  *@return     elfs_result Function result: LFS_SUCCESS/LFS_ERROR
  **************************************************************************************************/
elfs_result get_bad_block_number(uint32_t *bad_block_num,
                                uint32_t src_ind,
                                uint32_t end_ind,
                                _table_file_handler *table_file_handler)  {
  bool is_bad;
  uint32_t loop_ind;
  /* run loop from source block index, to end block index */
  for(loop_ind=src_ind;loop_ind < end_ind;loop_ind++) {
    is_bad = false;
    /* Check whether current block is bad from list read */
    if(check_current_block_is_bad(loop_ind,table_file_handler,&is_bad) != LFS_SUCCESS)  {
     /* if current block index, is greater than block index 2048, return out of bounds */
#ifdef PRINTS_OUT
      NRF_LOG_INFO("Error in Checking Block %d is bad,number of blocks out of bounds",loop_ind);
#endif
      return LFS_NUM_BLOCKS_OUT_OF_BOUNDS_ERROR;
    }
    else  {
      /*  if block is bad, increment bad block number */
      if(is_bad == true)  {
        (*bad_block_num)++;
      }
    }
  }

#ifdef PRINTS_OUT
  NRF_LOG_INFO("No of Bad Blocks is %d between %d and %d",*bad_block_num,src_ind,end_ind);
#endif
  return LFS_SUCCESS;
}

/*!
  **************************************************************************************************
  *@brief      Get remaining space for data/config memory in flash
  *@param       *remaining_bytes: Pointer to remmianing bytes
  *@param       file_type: Configuration / data fle type
  *@param      _table_file_handler *table_file_handler: table page handler to read from last page of toc
  *@return     elfs_result Function result: LFS_SUCCESS/LFS_ERROR
  **************************************************************************************************/
elfs_result lfs_get_remaining_space(uint32_t * remaining_bytes,
                                  elfs_file_type file_type,
                                  _table_file_handler *table_file_handler)  {
  uint32_t bad_block_num=0;
  uint32_t used_space=0;
  uint16_t page_correction = 0;


  /* page correction based on files present/not */
  page_correction = ((table_file_handler->table_file_info.offset != 0)?1:0);

  /* if file type is config file */
  if(file_type == LFS_CONFIG_FILE)  {
    bool is_bad = false;
    struct _page_header next_page_head;
    uint16_t next_page = CFGBLOCK*g_mem_prop->pages_per_block;
    /* Check config block is bad */
    if(check_current_block_is_bad(CFGBLOCK,table_file_handler,&is_bad) != LFS_SUCCESS)  {
      /* if block index is greater than 2048, return out of bounds error */
#ifdef PRINTS_OUT
      NRF_LOG_WARNING("Error in Checking Block %d is bad,number of blocks out of bounds",CFGBLOCK);
#endif
      return LFS_NUM_BLOCKS_OUT_OF_BOUNDS_ERROR;
    }
    else  {
      /* if config block is bad, return remaining bytes as '0' */
      if(is_bad == true)  {
        *remaining_bytes = 0;
      }
      else  {
        /* find good page by reading number of pages occupied
          in configuration block */
        do  {
          /* read spare area of config block pages */
          if(lfs_read_oob(next_page,&next_page_head) != LFS_SUCCESS)  {
#ifdef PRINTS_OUT
            NRF_LOG_INFO("Error reading page header for config file");
#endif
            return LFS_ERROR;
          }
           /* if page occupied, increment next page pointer */
          if(next_page_head.occupied == 1)  {
            next_page++;
          }
          /* if next page is greater than config block , break loop*/
          if(next_page >= (CFGBLOCK+1)*g_mem_prop->pages_per_block)
            break;
        }while(next_page_head.occupied == 1);
#ifdef PRINTS_OUT
        NRF_LOG_INFO("*****Remaining Space: Next page no = %d*********",next_page);
#endif

        /* Check all pages in config block are over*/
        if(next_page >= (CFGBLOCK+1)*g_mem_prop->pages_per_block) {
          *remaining_bytes = 0;
          NRF_LOG_INFO("All config pages over");
        }
        /* subtract from total config memory, size of memory occupied */
        else  {
          *remaining_bytes = (((CFGBLOCK+1)*g_mem_prop->pages_per_block) - next_page);
          *remaining_bytes = (*remaining_bytes) * g_mem_prop->page_size;
        }
      }
    }
#ifdef PRINTS_OUT
    NRF_LOG_INFO("Remaining Config memory in bytes =%d",*remaining_bytes);
#endif
  }

   /* if file type is data file */
  else if(file_type == LFS_DATA_FILE) {
    /* calculate current head in block */
    uint32_t curr_head = (table_file_handler->table_file_info.head_pointer/g_mem_prop->pages_per_block);
    /* calculate current head in pages */
    uint32_t curr_head_in_pages = table_file_handler->table_file_info.head_pointer;
    /* calculate current tail in block */
    uint32_t curr_tail = table_file_handler->table_file_info.tail_pointer;
    /* calculate current tail in pages */
    uint32_t curr_tail_in_pages = (table_file_handler->table_file_info.tail_pointer*g_mem_prop->pages_per_block);

     /* Case 1: when Head Pointer < Tail Pointer */
    if(curr_head < curr_tail) {

      /* If Tail Pointer is at not FILEBLOCK, add offset corresponding to block size to remaining bytes */
      if(curr_tail != FILEBLOCK)  {
        /* remaining bytes is tail - head + offset */
        *remaining_bytes = (curr_tail_in_pages-curr_head_in_pages + g_mem_prop->pages_per_block)*g_mem_prop->page_size;
      }
      else  {
         /* remaining bytes is tail - head */
        *remaining_bytes = (curr_tail_in_pages-curr_head_in_pages)*g_mem_prop->page_size;
      }
    }
    /* Case 2: When Head pointer > Tail Pointer */
    else if(curr_head > curr_tail)  {

      /* If Tail Pointer is at not FILEBLOCK, subtract offset corresponding to block size to remaining bytes */
      if(curr_tail != FILEBLOCK)  {
        /* Remaining Space is head - tail -offset */
        *remaining_bytes = DATA_FLASH_SIZE - ((curr_head_in_pages -curr_tail_in_pages - g_mem_prop->pages_per_block)*g_mem_prop->page_size);
      }
      else  {
         /* Remaining Space is head - tail */
        *remaining_bytes = DATA_FLASH_SIZE - ((curr_head_in_pages -curr_tail_in_pages)*g_mem_prop->page_size);
      }
     }

     /* Case 3: When Head Pointer = Tail Pointer */
    else if(curr_head == curr_tail) {
      /* if memory full flag is not equal to '1' */
      if(table_file_handler->table_file_info.mem_full_flag != 1) {
        used_space = 0;
        /* if page correction is not zero, files present */
        if(page_correction != 0)  {
          /* used space = ( Head Pointer % 64 ) * page size */
          used_space = (table_file_handler->table_file_info.head_pointer % g_mem_prop->pages_per_block)*g_mem_prop->page_size;
        }

        /* Remaining space is entire area */
        *remaining_bytes = DATA_FLASH_SIZE - used_space;
      }
      else  {
          *remaining_bytes = 0;
      }
    }

     /* if files present and remaining bytes is not zero */
     if((page_correction != 0) && ((*remaining_bytes) != 0))  {
        *remaining_bytes -= (page_correction*g_mem_prop->page_size);
     }

#ifdef PRINTS_OUT
    NRF_LOG_INFO("**** No of bad blocks = %d in rem space api *****",bad_block_num);
    NRF_LOG_INFO("Remaining Data memory in bytes =%d",*remaining_bytes);
#endif
  }
  return LFS_SUCCESS;
}

/*!
  **************************************************************************************************
  *@brief       Obtain the file handler of a file by its index number (0-63). The file
  *             handlers do not necessarily have to be consecutive on the TOC. This means
  *             that there may be void pages with no file handlers. Take into account
  *             the return value to check if the index asked has a file or not.

  *@param       fileNo: file number to open
  *@param       *_file_handler: Pointer to file handler

  *@return     elfs_result Function result: LFS_SUCCESS/LFS_ERROR
  **************************************************************************************************/
elfs_result lfs_open_file_by_number(uint8_t fileNo,
                                   _file_handler *file_handler) {
  _file_header *file_header = &file_handler->head;
  uint32_t pages_per_block = g_mem_prop->block_size/g_mem_prop->page_size;

  /* file number greater than 63 */
  if(fileNo > (MAXFILENUMBER-1))  {
    NRF_LOG_INFO("No of Files is greater than Maximum number of files supported");
    return LFS_MAX_FILE_COUNT_ERROR;
  }

  /* read out of band page */
  struct _page_header page_head;
  if(lfs_read_oob(TOCBLOCK*pages_per_block+fileNo,&page_head) != LFS_SUCCESS) {
    NRF_LOG_INFO("Error in reading File Handler Space");
    return LFS_ERROR;
  }
  /* if page is occupied */
  if(page_head.occupied == 1) {
    /* read file handler information from toc */
    if(nand_func_read(g_mem_prop,TOCBLOCK*g_mem_prop->block_size+fileNo*g_mem_prop->page_size,\
                      sizeof(_file_header), (uint8_t*)file_header) != NAND_FUNC_SUCCESS)  {
      NRF_LOG_INFO("Error Opening File Handler Information in TOC");
      return LFS_ERROR;
    }
    /* initialize current read position based on starting page to read */
    file_handler->current_read_pos = file_header->mem_loc*g_mem_prop->page_size;
    file_handler->current_offset = 0;
#ifdef PRINTS_OUT
    NRF_LOG_INFO("Current read pos=%d",file_handler->current_read_pos);
#endif
  }
  else
  {
    return LFS_ERROR;
  }
  return LFS_SUCCESS;
}

/*!
  **************************************************************************************************
  *@brief       Returns status of configuration file

  *@param       *out_file_status: Pointer to configuraion file status boolean if
                low touch file is occupied/not

  *@return     elfs_result Function result: LFS_SUCCESS/LFS_ERROR
  **************************************************************************************************/

elfs_result lfs_get_config_file_status(bool *out_file_status) {
  struct _page_header page_head;

    /* read out of band page of config file pos */
  if(lfs_read_oob(TOCBLOCK*g_mem_prop->pages_per_block+CONFIG_FILE_POSITION,&page_head) == LFS_ERROR) {
    NRF_LOG_INFO("Error in reading File Index List");
    return LFS_ERROR;
  }

  /* if config pos is written, return true , else false */
  if(page_head.occupied==1) {
#ifdef PRINTS_OUT
    NRF_LOG_INFO("config pos page index occupied");
#endif
    *out_file_status = true;
  }
  else  {

#ifdef PRINTS_OUT
    NRF_LOG_INFO("config pos page index not occupied");
#endif
    *out_file_status = false;
  }
  return LFS_SUCCESS;
}

/*!
  ***********************************************************************************
  *@brief       Return an array with the index of the files present on the memory as
  *             well as the number of files returned.
  *
  *@param       uint8_t * out_file_indexes: list of the indexes within the TOC with
  *                                           a file.
  * @param      uint8_t * out_file_no: number of files returned.

  *@return     elfs_result Function result: LFS_SUCCESS/LFS_ERROR
  ***********************************************************************************/
elfs_result lfs_get_file_indexes_list(uint8_t * out_file_indexes,
                                      uint8_t * out_file_no)  {
  uint8_t i;
  struct _page_header page_head;

  *out_file_no = 0;

  for(i=0;i < MAXFILENUMBER;i++)  {
    /* read out of band page of toc */
    if(lfs_read_oob(TOCBLOCK*g_mem_prop->pages_per_block+i,&page_head) == LFS_ERROR)  {
      NRF_LOG_INFO("Error in reading File Index List");
      return LFS_ERROR;
    }
    /* if page is occupied , increment out file number */
    if(page_head.occupied == 1) {
#ifdef PRINTS_OUT
      NRF_LOG_INFO("page index occupied - %d",TOCBLOCK*g_mem_prop->pages_per_block+i);
#endif
      out_file_indexes[*out_file_no] = i;
      (*out_file_no)++;
    }
  }
#ifdef PRINTS_OUT
  NRF_LOG_INFO("out file no =%d",*out_file_no);
#endif
  return LFS_SUCCESS;
}


/*!
  ***********************************************************************************
  *@brief       Return data offset variable to know number of files present
  *@param       uint8_t * gn_file_count: number of files returned.

  *@return     elfs_result Function result: LFS_SUCCESS/LFS_ERROR
  ***********************************************************************************/
elfs_result lfs_get_file_count(uint8_t *p_file_count)  {
   _table_file_handler table_file_handler;
   memset(&table_file_handler,0,sizeof(_table_file_handler));

  /* Read table page for  */
  if(read_table_file_in_toc(&table_file_handler) == LFS_ERROR)  {
#ifdef PRINTS_OUT
    NRF_LOG_INFO("Error in reading Table file");
#endif
    return LFS_ERROR;
  }

  *p_file_count = table_file_handler.table_file_info.offset;

  return LFS_SUCCESS;
}

/*!
  **************************************************************************************************
  @brief        Create a file with a certain file name. This will not write anything
  *             to the memory. The file handler will be written to the TOC once the
  *             file is closed. This function will assign memory locations for both
  *             the TOC and the actual file.
  *
  * @param    char * file_name (input): Name that will be assigned to the file
  * @param    _file_handler * file_handler (input) : file handler that will be used
  * @param    _table_file_handler                  : table file handler that will be used
  * @param    struct MemoryBuffer * memory_location (input): location of the memory
  *           buffer that will be used along the handler
  * @param    elfs_file_type type type of the file to create. Valid values:
  *                 *   LFS_CONFIG_FILE
  *                 *   LFS_DATA_FILE
  * @return   elfs_result Function result: LFS_SUCCESS/LFS_ERROR
  **************************************************************************************************/
#ifdef PROFILE_TIME_ENABLED
uint32_t file_create_time=0,tick1=0,tick2=0;
uint32_t tick3=0,tick4=0;
#endif
elfs_result lfs_create_file(uint8_t * file_name,
                           _file_handler *file_handler,
                           _table_file_handler *table_file_handler,
                           struct _memory_buffer * memory_location,
                           elfs_file_type type) {
  elfs_result status;

  _file_header *file_header = &file_handler->head;

  /* Copy the file name */
  uint8_t i;
  for(i = 0; i < FILE_NAME_LEN;i++)  {
    file_header->file_name[i] = file_name[i];
    if(file_header->file_name[i] == '\0')
      break;
  }

  memset(table_file_handler,0,sizeof(_table_file_handler));

   /* Read TOC File in structure to preserve contents  */
  if(read_table_file_in_toc(table_file_handler) == LFS_ERROR)  {
    NRF_LOG_INFO("Reading table file error");
    return LFS_ERROR;
  }

#ifdef PRINTS_OUT
  NRF_LOG_INFO("file_name=%s",file_header->file_name);
#endif

#ifdef PROFILE_TIME_ENABLED
     tick1 = get_micro_sec();
#endif

  /* Get first void page header number which is available file number to create */
  status = get_first_header_void(table_file_handler,type);

  if(type == LFS_DATA_FILE){
    file_header->header_number = table_file_handler->table_file_info.offset;
  }

  if(status != LFS_SUCCESS)   {
    /* if error in getting file number, check for maximum file count error */
    if(status == LFS_MAX_FILE_COUNT_ERROR)  {
      NRF_LOG_INFO("Maximum no of files crossed,error in getting File Header No");
      return LFS_MAX_FILE_COUNT_ERROR;
    }
    /* if error for configuration file */
    if(status == LFS_CONFIG_FILE_POSITION_ERROR)  {
      NRF_LOG_INFO("Config File Position Error");
      return LFS_CONFIG_FILE_POSITION_ERROR;
    }
    /* default error */
    else  {
      return LFS_ERROR;
    }
  }

#ifdef PROFILE_TIME_ENABLED
     tick2 = get_micro_sec() - tick1;
     tick3 = get_micro_sec();
#endif

  /* get first page to create new file */
  switch(type)
  {
    case LFS_CONFIG_FILE:
    if(get_first_config_page_free(&(file_header->mem_loc)) != LFS_SUCCESS)
    {
      NRF_LOG_INFO("Error in getting next valid config page to write");
      return LFS_INVALID_NEXT_WRITABLE_CONFIG_PAGE_ERROR;
    }
    break;
    case LFS_DATA_FILE:
    if(get_next_writable_page(file_handler,table_file_handler) != LFS_SUCCESS)
    {
      NRF_LOG_INFO("Error in getting next valid data page to write");
      return LFS_INVALID_NEXT_WRITABLE_DATA_PAGE_ERROR;
    }
    break;
  }

#ifdef PROFILE_TIME_ENABLED
  tick4 = get_micro_sec() - tick3;
#endif
  /* Update File Handler Info based on type of file */
  if(type ==  LFS_CONFIG_FILE)  {
    file_handler->curr_write_mem_loc = file_header->mem_loc*g_mem_prop->page_size;
  }
  else if(type ==  LFS_DATA_FILE) {
    file_handler->head.mem_loc = table_file_handler->table_file_info.head_pointer;
    file_handler->curr_write_mem_loc = table_file_handler->table_file_info.head_pointer * g_mem_prop->page_size;
  }

  file_header->time_stamp = 0;
  file_header->file_size = 0;
  file_header->file_type = type;

  file_header->tmp_write_mem = memory_location;
  file_handler->tmp_write_mem_loc = 0;
  file_handler->op_mode = LFS_MODE_FAST;

  /* update file handler atleast once during creation */
  if(lfs_update_header(file_handler,table_file_handler) == LFS_ERROR){
    NRF_LOG_INFO("Error in updating header in TOC for closing of file");
    return LFS_UPDATE_HEADER_ERROR;
  }

#ifdef PROFILE_TIME_ENABLED
     file_create_time = get_micro_sec() - tick1;
#endif
  return LFS_SUCCESS;
}


/*!
  **************************************************************************************************
  @brief      Open file by its file_name. This will return the file handler read in
  *           the TOC. Right now the function will go through all the possible file
  *           headers on the TOCBLOCK, and check if they are filled. If this is the
  *           case, it will check if the name is the same. If two or more files
  *           contain the same name, it will return the one with a lower index within
  *           the TOCBLOCK.
  *
  * @param    char * file_name (input): Name that will be looked for in the TOC
  * @param    _file_header * _file_header : file handler that will be used
  * @return   elfs_result Function result: LFS_SUCCESS/LFS_ERROR
  **************************************************************************************************/
elfs_result lfs_open_file_by_name(char * file_name,
                                  _file_handler *file_handler)  {
  _file_header *file_header = &file_handler->head;
  struct _page_header tmp_page_head;
  uint8_t i;
  uint8_t j;
  uint8_t flag = 0;

  for(i=0;i<MAXFILENUMBER;i++)  {
    /* read spare area to check toc page is occupied or not */
    if(lfs_read_oob(TOCBLOCK*g_mem_prop->pages_per_block+i, &tmp_page_head) == LFS_ERROR) {
      NRF_LOG_INFO("Error in File Header Spare area");
      return LFS_ERROR;
    }
    /* if page is not occupied, skip current index and proceed reading */
    if(tmp_page_head.occupied != 1) {
      continue;
    }
    flag = 0;
    /* read toc block file handler information if page is occupied */
    if(nand_func_read(g_mem_prop,TOCBLOCK*g_mem_prop->block_size+i*g_mem_prop->page_size,\
                     sizeof(_file_header),(uint8_t*)file_header) != NAND_FUNC_SUCCESS)  {
      NRF_LOG_INFO("Error in reading File Header");
      return LFS_ERROR;
    }
    /* compare read file name from file handler with input argument file name which has
    to be found */
    for(j=0;j<FILE_NAME_LEN;j++)  {
      if((file_header->file_name[j] != file_name[j])) {
        flag = 1;
        break;
      }
      if(file_header->file_name[j] == '\0') {
        break;
      }
    }

    if(!flag){
      break;
    }
  }
  file_handler->current_read_pos = file_header->mem_loc*g_mem_prop->page_size;
  file_handler->current_offset = 0;
  /* if flag is '1' , file is not existent in toc */
  if(flag == 1) {
    NRF_LOG_INFO("File name %s not found",file_name);
    return LFS_FILE_NOT_FOUND_ERROR;
  }
  /* if flag is '0' , file is existent in toc */
  else  {
#ifdef PRINTS_OUT
    NRF_LOG_INFO("File %s existing and sucessfully opened",file_name);
#endif
    return LFS_SUCCESS;
  }
}

/*!
  **************************************************************************************************
  @brief  Read a file given the output buffer, the offset inside the file, the
  *         size to be read and the file handler of the file.
  *
  * @param      *outBuffer:   Pointer to soft buffer to read to data to
  * @param       offset    : Offset inside the file to start reading from
  * @param       size      : Number of bytes to read
  * @param      _file_header * _file_header : file handler that will be used
  * @return   elfs_result Function result: LFS_SUCCESS/LFS_ERROR
  **************************************************************************************************/
#ifdef PAGE_READ_DEBUG_INFO
uint32_t last_page_read=0;
uint32_t last_page_read_offset=0;
uint32_t bytes_processed_from_fs_task=0;
uint8_t last_page_read_status=0;
#endif
elfs_result lfs_read_file(uint8_t * outBuffer,
                          uint32_t offset,
                          uint32_t size,
                          _file_handler *file_handler)  {

  if(file_handler->current_offset != offset)  {
    /* set read position in page to read data bytes */
    if(lfs_set_read_pos(offset,file_handler) != LFS_SUCCESS)  {
      NRF_LOG_INFO("Error in setting File Read Position");
      return LFS_SET_FILE_READ_POSITION_ERROR;
    }
  }
   struct _page_header page_header;
   uint32_t bytes_processed = 0;
   /* read on to soft buffer till 'size; paramter is read from
   flash */
   while(bytes_processed<size)  {
     /* Do page aligning */
     uint32_t read_pos=file_handler->current_read_pos;
     uint32_t read_page = read_pos/g_mem_prop->page_size;
     uint32_t read_page_offset = read_pos-read_page*g_mem_prop->page_size;
#ifdef PAGE_READ_DEBUG_INFO
     last_page_read = read_page;
     last_page_read_offset = read_page_offset;
#endif
#ifdef PRINTS_OUT
     NRF_LOG_INFO("Read pos = %d, Read page = %d,Read page offset=%d",
      read_pos,
      read_page,
      read_page_offset);
#endif
     if((size-bytes_processed) >= (g_mem_prop->page_size-read_page_offset)) {
      /* if spare area of page to be read */
       if(lfs_read_oob(read_page,&page_header) != LFS_SUCCESS)
       {
         NRF_LOG_INFO("Error in reading page header for reading file");
#ifdef PAGE_READ_DEBUG_INFO
         last_page_read_status = 2;
#endif
         return LFS_ERROR;
       }
       /* if reading based on page offset */
       if(nand_func_read(g_mem_prop,read_pos,(g_mem_prop->page_size-read_page_offset),\
                        (uint8_t*)&outBuffer[bytes_processed]) != NAND_FUNC_SUCCESS)  {
         NRF_LOG_INFO("Error in reading file:%s",file_handler->head.file_name);
#ifdef PAGE_READ_DEBUG_INFO
         last_page_read_status = 1;
#endif
         return LFS_FILE_READ_ERROR;
       }

       bytes_processed+=g_mem_prop->page_size-read_page_offset;
       /* Next page write would have checked for bad block and accordingly written */
       file_handler->current_read_pos=page_header.next_page*g_mem_prop->page_size;
     }
     else {
        /* read bytes onto softbuffer */
       if(nand_func_read(g_mem_prop,read_pos,(size-bytes_processed),\
                        (uint8_t*)&outBuffer[bytes_processed]) != NAND_FUNC_SUCCESS)  {
         NRF_LOG_INFO("Error in reading File Name:%s",file_handler->head.file_name);
#ifdef PAGE_READ_DEBUG_INFO
         last_page_read_status = 3;
#endif
         return LFS_FILE_READ_ERROR;
       }
       file_handler->current_read_pos += size-bytes_processed;
       bytes_processed=size;
     }
   }
#ifdef PAGE_READ_DEBUG_INFO
  bytes_processed_from_fs_task += bytes_processed;
  last_page_read_status = 0;
#endif
  file_handler->current_offset+=size;
  return LFS_SUCCESS;
}

/*!
  **************************************************************************************************
  * @brief  Remove a configuration file
  *
  * @param    _file_header * _file_header : file handler that will be deleted
  * @return   elfs_result Function result: LFS_SUCCESS/LFS_ERROR
  **************************************************************************************************/
elfs_result lfs_delete_config_file(_file_handler *file_handler)
{
   _table_file_handler tmp_table_file_handler;
   memset(type,0,MAX_UPDATE_TYPE);
  type[0] = LFS_CONFIG_FILE_POS_OCCUPIED_UPDATE;

  memset(&tmp_table_file_handler,0,sizeof(_table_file_handler));

  /* Read TOC File in structure to preserve contents */
  if(read_table_file_in_toc(&tmp_table_file_handler) == LFS_ERROR)
  {
    NRF_LOG_INFO("Reading table file error");
    return LFS_ERROR;
  }

  /* if file type is configuration, delete page corresponding to
   starting page of file handler */
  if(file_handler->head.file_type == LFS_CONFIG_FILE) {
    if(lfs_delete_page(file_handler->head.mem_loc) == LFS_ERROR)  {
      NRF_LOG_INFO("Error in deleting config file in Config block");
      return LFS_ERROR;
    }

    /* delete file information & update flag low touch occupied flag
     as configuration deleted in table page */
    memset(pos_ind_to_del,0,sizeof(pos_ind_to_del));
    pos_ind_to_del[0]=TOCBLOCK*g_mem_prop->pages_per_block+file_handler->head.header_number;
    pos_ind_to_del[1]=TOCBLOCK*g_mem_prop->pages_per_block+TABLE_PAGE_INDEX_IN_TOC;


   /* delete 2 pages ( file handler & and table file ) */
   if(lfs_delete_toc_pages(pos_ind_to_del,NUM_OF_TOC_PAGES_TO_DEL_FOR_HEADER_UPD)
      != LFS_SUCCESS){
      return LFS_ERROR;
   }

   /* make low touch config file occupied as '0' */
    tmp_table_file_handler.table_file_info.config_low_touch_occupied =  0;
    /* Write Table page in TOC */
    if(write_table_file_in_toc(&tmp_table_file_handler) == LFS_ERROR) {
      NRF_LOG_INFO("Write Table File in Update TOC is Error");
      return LFS_ERROR;
    }
  }
  else  {
    return LFS_ERROR;
  }
  return LFS_SUCCESS;
}

/*!
  **************************************************************************************************
  * @brief    Update pattern file
  *
  * @param    _file_handler * file_handler : file handler that has to be updated while pattern data
              is written.
              _table_file_handler *table_file_handler: table page handler that has to be updated.
              start_block_num: starting block number to write
              *in_buffer: Input data to write
              first_time_write: first time write flag when '1' will move head pointer to block
              pointed out by starting block number
  * @return   elfs_result Function result: LFS_SUCCESS/LFS_ERROR
  **************************************************************************************************/
elfs_result lfs_update_pattern_file(uint8_t *in_buffer,
                          uint16_t start_block_num,
                          uint16_t size,
                          _file_handler *file_handler,
                          _table_file_handler *table_file_handler,
                          uint8_t first_time_write)
{
    if(first_time_write){
      /* read table page from toc */
      if(read_table_file_in_toc(table_file_handler) != LFS_SUCCESS) {
        NRF_LOG_INFO("Error in reading table file");
        return LFS_ERROR;
      }

      /* move head pointer to start block given */
      table_file_handler->table_file_info.head_pointer += start_block_num*g_mem_prop->pages_per_block;
    }
    /* Call file update for writing */
    if(lfs_update_file(in_buffer,size,file_handler,table_file_handler) != LFS_SUCCESS)  {
       NRF_LOG_INFO("Error in updating file");
      return LFS_ERROR;
    }
   return LFS_SUCCESS;
}

#ifdef FS_TEST_BLOCK_READ_DEBUG
uint16_t gBlkNumTest = TMPBLOCK;
int read_tmp_blk(uint32_t page_number,uint32_t *pdata_memory,uint8_t file_type)
{
    eNand_Func_Result result=NAND_FUNC_SUCCESS;
    uint32_t page_pos =gBlkNumTest*g_mem_prop->pages_per_block+page_number;

  /* read table file handler type */
    if(file_type == 0) {
      _table_file_handler gsTmpBlkPageContentHdr;
      memset(&gsTmpBlkPageContentHdr,0,sizeof(gsTmpBlkPageContentHdr));
      result = nand_func_read(g_mem_prop,page_pos*g_mem_prop->page_size,\
        sizeof(_table_file_handler),(uint8_t*)&gsTmpBlkPageContentHdr);
    }
    /* config file */
    else if(file_type == 1){
      _file_header gsTmpBlkPageContentHdr;
      memset(&gsTmpBlkPageContentHdr,0,sizeof(gsTmpBlkPageContentHdr));
        result = nand_func_read(g_mem_prop,page_pos * g_mem_prop->page_size,\
          sizeof(_file_header),(uint8_t*)&gsTmpBlkPageContentHdr);
    }
    /* data page */
    else {
      uint32_t page_arr[PAGE_SIZE];
      memset(page_arr,0,PAGE_SIZE);
      result = nand_func_read(g_mem_prop,page_pos * g_mem_prop->page_size,\
          PAGE_SIZE,(uint8_t*)&page_arr[0]);
    }

    if(result != NAND_FUNC_SUCCESS)
    {
      NRF_LOG_INFO("Error in page %d read",page_pos);
      return -1;
    }

    //copy data to array
//    pdata_memory[0] = gsTmpBlkPageContentHdr.table_file_info.head_pointer;//head pointer
//    pdata_memory[1] = gsTmpBlkPageContentHdr.table_file_info.tail_pointer;//tail pointer
//    pdata_memory[2] = gsTmpBlkPageContentHdr.table_file_info.mem_full_flag;//mem full flag
//    pdata_memory[3] = gsTmpBlkPageContentHdr.table_file_info.initialized_circular_buffer;//init circular buffer flag
//    pdata_memory[4] = gsTmpBlkPageContentHdr.table_file_info.offset;//offset
    return 0;
}

int read_page_data(uint32_t page_num, uint8_t *pdata_mem, uint16_t page_size)
{
    eNand_Func_Result result=NAND_FUNC_SUCCESS;
    result = nand_func_read(g_mem_prop,page_num * g_mem_prop->page_size,\
          page_size,pdata_mem);
    if(result != NAND_FUNC_SUCCESS)
    {
      NRF_LOG_INFO("Error in page %d read",page_num);
      return -1;
    }
    return result;
}

int read_page_ecc_zone(uint32_t page_num, uint32_t *pnext_page, uint8_t *poccupied)
{
    elfs_result res;
    struct _page_header spare_head;

    res = lfs_read_oob(page_num,&spare_head);
    *pnext_page =  spare_head.next_page;
    *poccupied = spare_head.occupied;
    return res;
}

#endif //FS_TEST_BLOCK_READ_DEBUG

/*!
  **************************************************************************************************
  * @brief    Get pointer information
  *
  * @param      *   head_pointer : Pointer to head pointer.
  *             *   tail_pointer:  Pointer to tail pointer.
  * @return         0/-1
  **************************************************************************************************/
int get_pointers_info(uint32_t *head_pointer,uint32_t *tail_pointer,uint16_t table_page_flags[])  {
   _table_file_handler table_file_handler;
   memset(&table_file_handler,0,sizeof(table_file_handler));

    /* read table page from toc */
  if(read_table_file_in_toc(&table_file_handler) != LFS_SUCCESS) {
     NRF_LOG_INFO("Error in reading table file");
     return -1;
  }
  /* if read offset is '0', no files present , update head pointer
  for display by rounding off at boundary of current block */
  if((table_file_handler.table_file_info.offset) == 0)  {
    *head_pointer = (table_file_handler.table_file_info.head_pointer/g_mem_prop->pages_per_block)*g_mem_prop->pages_per_block;
  }
  /* else read head pointer as it is */
  else  {
    *head_pointer = table_file_handler.table_file_info.head_pointer;
  }
  /* read tail pointer */
  *tail_pointer = table_file_handler.table_file_info.tail_pointer;

  /* read other variables of table page */
  table_page_flags[0] = table_file_handler.table_file_info.initialized_circular_buffer;
  table_page_flags[1] = table_file_handler.table_file_info.mem_full_flag;
  table_page_flags[2] = table_file_handler.table_file_info.offset;
  table_page_flags[3] = table_file_handler.table_file_info.config_low_touch_occupied;

  return 0;
}


/*!
  **************************************************************************************************
  * @brief      Update a file with new data. This function may not write the data
  *             when it is called. It will wait until the 4096 bytes buffer is filled
  *             to write. Anyway this is managed by the light_fs and the user just has to
  *             call this function with the new data and finish the file to ensure that
  *             all the bytes and the TOC is written.
  *
  * @param      *in_buffer: Buffer with the data to be written
  * @param      size : Number of bytes to write
  * @param      _file_header * _file_header : file handler that will be used
  * @return     elfs_result Function result LFS_SUCCESS/LFS_ERROR
  **************************************************************************************************/
elfs_result lfs_update_file(uint8_t *in_buffer,
                          uint16_t size,
                          _file_handler *file_handler,
                          _table_file_handler *table_file_handler)
{
  int32_t i;
  int8_t hold_buff = 0;
  bool is_bad_block = false;
  uint32_t word_pointer;
  uint32_t bit_pointer;

  /* Sanity check */
  _file_header *file_header = &file_handler->head;

  /* check whether current memory location is at boundary */
  if((file_handler->curr_write_mem_loc & (g_mem_prop->page_size-1))!=0) {
    return LFS_ERROR;
  }
  uint16_t remaining_bytes=file_handler->tmp_write_mem_loc+size;
  uint16_t new_bytes_written=0;

  /* minmimum page buffer is filled to write page */
  while(remaining_bytes>=g_mem_prop->page_size) {
#ifdef PROFILE_TIME_ENABLED
     nTick1 = get_micro_sec();
#endif
    /* if hold buffer flag is '0', new data to write */
    if(!hold_buff)  {
      /* Fill the temporal buffer with some of the new bytes */
      for(i=g_mem_prop->page_size-file_handler->tmp_write_mem_loc-1;i>=0;i--) {
        file_header->tmp_write_mem->data[file_handler->tmp_write_mem_loc+i] = in_buffer[i];
      }
      new_bytes_written += g_mem_prop->page_size-file_handler->tmp_write_mem_loc;

      /* bytes written initially is page_size */
      remaining_bytes -= g_mem_prop->page_size;
   }
   else {
      /* Clear Flag */
      hold_buff = 0;
   }

    /* Write next page information */
    /* Check in first place that the page of the next byte is ok: */
    table_file_handler->table_file_info.head_pointer=(file_handler->curr_write_mem_loc/g_mem_prop->page_size)+1;

#ifdef PROFILE_TIME_ENABLED
    nGetNextPageReadTick1 =  get_micro_sec();
#endif

    /* get next good page to write by traversing through bad blocks if any */
    if(get_next_page_files(file_handler,table_file_handler) != LFS_SUCCESS) {
      NRF_LOG_INFO("Error in finding Valid page to write");
      return LFS_INVALID_NEXT_WRITABLE_DATA_PAGE_ERROR;
    }

#ifdef PROFILE_TIME_ENABLED
    g_next_page_read_time = get_micro_sec() - nGetNextPageReadTick1;
    NRF_LOG_INFO("****** next page read api time = %d ********",g_next_page_read_time);
#endif

    /* Update current page information (Which is the next page and set the current as occupied) */
    file_header->last_used_page=file_handler->curr_write_mem_loc/g_mem_prop->page_size;

    /* Update current page information (Which is the next page and set the current as occupied)
    history of head pointers are stored, in every page to smoothen file reading */
    file_header->tmp_write_mem->next_page_data.next_page = table_file_handler->table_file_info.head_pointer;
    /* set current page occupied */
    file_header->tmp_write_mem->next_page_data.occupied = 1;

    NRF_LOG_INFO("Page destination address for writing:%d",file_handler->curr_write_mem_loc/g_mem_prop->page_size);


    /* Write the header and the data */
    struct _page_write write_data = {.page_dest=file_handler->curr_write_mem_loc/g_mem_prop->page_size,
    .data_buf = (uint8_t*)file_header->tmp_write_mem->data,
    .data_size = g_mem_prop->page_size,
    .offset = DATA_OFFSET,
    .spare_buf = (uint8_t*)&file_header->tmp_write_mem->next_page_data,
    .spare_size=sizeof(struct _page_header),
    .spare_offset=SPARE_OFFSET};

#ifdef PROFILE_TIME_ENABLED
    nPageTick1 = get_micro_sec();
    page_update_flag = 1;
#endif

    eNand_Func_Result res = nand_func_page_and_spare_write(&write_data);

#ifdef PROFILE_TIME_ENABLED
    page_update_flag = 0;
    g_page_write_time = get_micro_sec() - nPageTick1;
    NRF_LOG_INFO("****** data page write api time = %d*******",g_page_write_time);
#endif

    /* In process of writing has gone bad */
    if(res != NAND_FUNC_SUCCESS)  {
      /*Check current memory loc block index and page index , if boundary of block,check
       current block is bad  skip current block and start wrting from next good block
       available,update file handler info required for file traversal , if not boundary
       close file and ensure  erase is called and tail pointer is updated prior to writing
       also update bad block list */

      uint32_t curr_mem_loc_in_pages = file_handler->curr_write_mem_loc/g_mem_prop->page_size;
      uint32_t curr_mem_loc_in_blk = curr_mem_loc_in_pages/g_mem_prop->pages_per_block;

      /* Check current block is bad */
      if(nand_func_is_bad_block(g_mem_prop,curr_mem_loc_in_blk,&is_bad_block) != NAND_FUNC_SUCCESS) {
      /* return error if driver api to check current block is bad */
#ifdef PRINTS_OUT
        NRF_LOG_INFO("Check for Bad block %d failed",curr_mem_loc_in_blk);
#endif
        return LFS_BAD_BLOCK_HEADER_CHECK_ERROR;
      }
      /* Update Bad Block list with Current block Info */
      else  {
        /* calculate word/bit pointers */
        word_pointer = curr_mem_loc_in_blk / MAX_NO_OF_BITS_IN_WORD;
        bit_pointer = curr_mem_loc_in_blk % MAX_NO_OF_BITS_IN_WORD;

#ifdef PRINTS_OUT
        NRF_LOG_INFO("Word Pointer:%d,Bit Pointer:%d",word_pointer,bit_pointer);
#endif

        /* if current block is bad */
        if(is_bad_block == true)  {
          table_file_handler ->table_file_info.bad_block_marker[word_pointer] |= (1 << bit_pointer);
          del_table_page = 1;
          memset(type,0,MAX_UPDATE_TYPE);
          type[0] = LFS_BAD_BLOCK_MARKER_UPDATE;
          /* update table page with new bad block information */
          if(update_table_file_in_toc(table_file_handler,type,1) != LFS_SUCCESS)  {
            NRF_LOG_INFO("Error in updating Table File in TOC");
            return LFS_UPDATE_TABLE_FILE_ERROR;
          }

          /* bad block list updated */
          vol_info_buff_var.bad_block_updated =1;

          del_table_page = 0;
          /* Check boundary of block where error happened */
          /* if error occured at boundary of block */
          if(!(curr_mem_loc_in_pages % g_mem_prop->pages_per_block))  {
            /* Update current memory loc Info to increment to skip current block */
            file_handler->curr_write_mem_loc = ((curr_mem_loc_in_blk+1)*g_mem_prop->pages_per_block*g_mem_prop->page_size);
            /* set hold buff flag '1' to write same soft buffer on to next good block boundary */
            hold_buff = 1;
#ifdef PRINTS_OUT
            NRF_LOG_INFO("current page written:%d, \
            writing page by skipping to next block as bad block found at boundary",curr_mem_loc_in_pages);
#endif
            new_bytes_written -= g_mem_prop->page_size-file_handler->tmp_write_mem_loc;
            /* bytes written initially is page_size */
            remaining_bytes += g_mem_prop->page_size;
            /* skip current loop */
            continue;
          }
          else  {
            /* Get to know if file starting file position is spread across blocks
               if current start of file block is not the current block which has gone bad,
              indicates file already written from previous block, recover file by moving
              pointer back to start of block closing file here itself */
            if((file_handler->head.mem_loc/g_mem_prop->pages_per_block) != curr_mem_loc_in_blk) {
              file_handler->head.last_used_page=curr_mem_loc_in_pages-(curr_mem_loc_in_pages % g_mem_prop->pages_per_block);
              file_handler->head.file_size-=(curr_mem_loc_in_pages % g_mem_prop->pages_per_block)*g_mem_prop->page_size;
#ifdef PRINTS_OUT
              NRF_LOG_INFO("Last used page=%d,File size=%d updated till boundary",file_handler->head.last_used_page,file_handler->head.file_size);
#endif
              /* Reinitialize Head Pointer to next block */
              table_file_handler ->table_file_info.head_pointer = (curr_mem_loc_in_blk+1)*g_mem_prop->pages_per_block;
#ifdef PRINTS_OUT
              NRF_LOG_INFO("Head Pointer Initialized to beginning of next block=%d",table_file_handler ->table_file_info.head_pointer);
#endif
              /*  Close File */
              if(lfs_update_header(file_handler,table_file_handler) == LFS_ERROR)
              {
                NRF_LOG_INFO("Error in updating header in TOC for closing of file");
                return LFS_UPDATE_HEADER_ERROR;
              }
              return LFS_FILE_PARTIALLY_WRITTEN_ERROR;// to distinguish between partial file that can be retrieved
            }
            /* multiple files are written within block which went bad */
            else  {
              /* Reinitialize Head Pointer to next block */
             table_file_handler ->table_file_info.head_pointer = (curr_mem_loc_in_blk+1)*g_mem_prop->pages_per_block;
#ifdef PRINTS_OUT
               NRF_LOG_INFO("Header Number to be deleted=%d",file_handler->head.header_number);
#endif
              /* delete current file handler which is opened in toc, also update head pointer in table page */
              memset(pos_ind_to_del,0,sizeof(pos_ind_to_del));
              pos_ind_to_del[0] = TOCBLOCK*g_mem_prop->pages_per_block+file_handler->head.header_number;
              pos_ind_to_del[1] = TOCBLOCK*g_mem_prop->pages_per_block+TABLE_PAGE_INDEX_IN_TOC;

              /* delete 2 pages ( file handler & and table file ) */
              if(lfs_delete_toc_pages(pos_ind_to_del,NUM_OF_TOC_PAGES_TO_DEL_FOR_HEADER_UPD) != LFS_SUCCESS){
                return LFS_ERROR;
              }

              /* write table page */
              if(write_table_file_in_toc(table_file_handler) == LFS_ERROR)  {
                NRF_LOG_INFO("Write Table File in Update TOC is Error");
                return LFS_ERROR;
              }
            }
            return LFS_FILE_WRITE_ERROR;
          }
        }
      }
    }

    /* Update file handler variables */
    file_handler->tmp_write_mem_loc = 0;
    file_handler->curr_write_mem_loc = table_file_handler->table_file_info.head_pointer*g_mem_prop->page_size;
    file_header->file_size += g_mem_prop->page_size;


    /* Update the file handler to memory if secure mode is set */
    switch(file_handler->op_mode)
    {
      case LFS_MODE_SECURE:
        /* if secure mode , header is updated every page */
        if(lfs_update_header(file_handler,table_file_handler) != LFS_SUCCESS) {
          NRF_LOG_ERROR("Error in updating header in TOC for file handler updation in secure mode");
          return LFS_UPDATE_HEADER_ERROR;
        }
     break;
    }

#ifdef PROFILE_TIME_ENABLED
     nTick2 = get_micro_sec();
     g_page_hold_write_time = nTick2 - nTick1;
     NRF_LOG_INFO("Time taken for buffer update with read next page to write = %d",g_page_hold_write_time);
#endif
  }
  /* Store in the buffer the reamaining bytes less than a page */
  for(i = size-new_bytes_written-1;i >= 0;i--)  {
    file_header->tmp_write_mem->data[file_handler->tmp_write_mem_loc+i] = in_buffer[new_bytes_written+i];
  }
  // update temp mem loc for remaining bytes to write */
  file_handler->tmp_write_mem_loc+= size-new_bytes_written;
  return LFS_SUCCESS;
}


/*!
  **************************************************************************************************
  * @brief      Get the next unused page within a good config block.
  *
  * @param      *first_page: Closest page found
  * @return     elfs_result Function result LFS_SUCCESS/LFS_ERROR
  **************************************************************************************************/
static elfs_result get_next_writable_config_page(uint32_t * first_page) {
  struct _page_header curr_page_head = {0, 0};
  bool_t block_status = 0;

  /* check if config block s bad */
  if(nand_func_is_bad_block(g_mem_prop,CFGBLOCK,&block_status)!=NAND_FUNC_SUCCESS)
  {
     NRF_LOG_INFO("Error in checkng bad block");
     return LFS_BAD_BLOCK_HEADER_CHECK_ERROR;
   }
  /* Check if block has gone bad */
  if(block_status)  {
    /* return error as whole config block has gone bad */
    NRF_LOG_INFO("Config block has gone bad");
    return LFS_ERROR;
  }

  do  {
    /* if first page found crosses, configuration memory full error */
    if(*first_page >= CFGBLOCK*g_mem_prop->pages_per_block) {
      NRF_LOG_INFO("Config pages to write is over");
      return LFS_MEMORY_FULL_ERROR;
    }
    /* Check that the current page is not occupied */
    if(lfs_read_oob(*first_page,&curr_page_head) != LFS_SUCCESS){
      return LFS_OOB_ERROR;
    }
    /* if current page is occupied, increment first page pointer */
    if(curr_page_head.occupied == 1)  {
      (*first_page)++;
    }
  } while(curr_page_head.occupied == 1);
  return LFS_SUCCESS;
}

/*!
  **************************************************************************************************
  * @brief  When using manual mode, update the header of the file.
  *
  * @param    file_header * file_header : file handler of the file to update the file
  * @return     elfs_result Function result LFS_SUCCESS/LFS_ERROR
  **************************************************************************************************/
static elfs_result lfs_update_config_header(_file_handler *file_handler) {
  eNand_Func_Result result = NAND_FUNC_SUCCESS;
  struct _page_header header_page_info;
  header_page_info.next_page = 0xFFFFFFFF;
  header_page_info.occupied = 1;
  _file_header *file_header = &file_handler->head;

  /* calculate page position based on config file position */
  uint32_t page_pos = TOCBLOCK*g_mem_prop->pages_per_block+CONFIG_FILE_POSITION;
  /* write data and spare area on to toc regarding information of config file */
  result |= nand_func_write(g_mem_prop, page_pos*g_mem_prop->page_size,\
                            sizeof(_file_header), (uint8_t*)file_header);
  result |= nand_func_write_ecc_zone(page_pos,SPARE_OFFSET, \
                            sizeof(struct _page_header), (uint8_t*)&header_page_info);

  if(result != NAND_FUNC_SUCCESS) {
  /*if updation is error, return error */
    return LFS_ERROR;
  }
  return LFS_SUCCESS;
}

/*!
  **************************************************************************************************
  * @brief  Update a file with new data. This function may not write the data
  *         when it is called. It will wait until the 2048 bytes buffer is filled
  *         to write. Anyway this is managed by the LightFS and the user just has to
  *         call this function with the new data and finish the file to ensure that
  *         all the bytes and the TOC is written.
  *
  * @param    uint8_t *in_buffer: Buffer with the data to be written
  * @param    uint32_t size : Number of bytes to write
  * @param    _file_handler * _file_handler : file handler that will be used
  * @return     elfs_result Function result LFS_SUCCESS/LFS_ERROR
  **************************************************************************************************/
elfs_result lfs_update_config_file(uint8_t *in_buffer,
                          uint16_t size,
                          _file_handler *file_handler)  {
  int32_t i;
  elfs_result res;

  /* Sanity check */
  _file_header * file_header = &file_handler->head;

  if((file_handler->curr_write_mem_loc & (g_mem_prop->page_size-1))!=0) {
    return LFS_ERROR;
  }

  uint16_t reamaining_bytes = file_handler->tmp_write_mem_loc+size;
  uint16_t new_bytes_written = 0;

  while(reamaining_bytes >= g_mem_prop->page_size) {
    /* Fill the temporal buffer with some of the new bytes */
    for(i = g_mem_prop->page_size-file_handler->tmp_write_mem_loc-1;i >= 0;i--) {
       file_header->tmp_write_mem->data[file_handler->tmp_write_mem_loc+i]=in_buffer[i];
    }
    new_bytes_written += g_mem_prop->page_size-file_handler->tmp_write_mem_loc;
    reamaining_bytes -= g_mem_prop->page_size;

    /* Write next page information */
    /* Check in first place that the page of the next byte is ok: */
    uint32_t next_page = file_handler->curr_write_mem_loc/g_mem_prop->page_size+1;
    /* if next writable config page is error */
    res = get_next_writable_config_page(&next_page);

   if(res != LFS_SUCCESS) {
     /* process memory full error if getting next good config page is not found */
     if(res == LFS_MEMORY_FULL_ERROR)
         return LFS_MEMORY_FULL_ERROR;
     /* out of band page error */
     else if(res == LFS_OOB_ERROR)
         return LFS_OOB_ERROR;
     /* bad block header check error */
     else if(res == LFS_BAD_BLOCK_HEADER_CHECK_ERROR)
       return LFS_BAD_BLOCK_HEADER_CHECK_ERROR;
     /* default error */
     else
       return LFS_ERROR;
   }

   /* Update current page information (Which is the next page and set the current as occupied) */
   file_header->tmp_write_mem->next_page_data.next_page = next_page;
   file_header->tmp_write_mem->next_page_data.occupied = 1;
   file_header->last_used_page = file_handler->curr_write_mem_loc/g_mem_prop->page_size;

   /* Write the header and the data */
   struct _page_write write_data = {.page_dest = file_handler->curr_write_mem_loc/g_mem_prop->page_size,
                                      .data_buf = (uint8_t*)file_header->tmp_write_mem->data,
                                      .data_size = g_mem_prop->page_size,
                                      .offset = DATA_OFFSET,
                                      .spare_buf = (uint8_t*)&file_header->tmp_write_mem->next_page_data,
                                      .spare_size = sizeof(struct _page_header),
                                      .spare_offset = SPARE_OFFSET};
   /* write page */
   nand_func_page_and_spare_write(&write_data);

   /* Update the handler variables */
   file_handler->tmp_write_mem_loc = 0;
   file_handler->curr_write_mem_loc = next_page*g_mem_prop->page_size;
   file_header->file_size += g_mem_prop->page_size;

   /* Update the file handler to memory if secure mode is set */
  switch(file_handler->op_mode) {
    case LFS_MODE_SECURE:
    if(lfs_delete_page(TOCBLOCK*g_mem_prop->pages_per_block + \
                      file_handler->head.header_number) != LFS_SUCCESS)
       return LFS_ERROR;
    if(lfs_update_config_header(file_handler) != LFS_SUCCESS)
       return LFS_ERROR;
    break;
   }
 }

  /* Store the remaining bytes in the buffer */
  for(i = size-new_bytes_written-1;i >= 0;i--){
      file_header->tmp_write_mem->data[file_handler->tmp_write_mem_loc+i] = in_buffer[new_bytes_written+i];
  }
  file_handler->tmp_write_mem_loc += size-new_bytes_written;
  return LFS_SUCCESS;
}



/*!
  **************************************************************************************************
  @brief  End the file by writing the remaining bytes and the TOC.
  *
  * @param    _file_header * _file_header : file handler of the file.
  * @return     elfs_result Function result LFS_SUCCESS/LFS_ERROR
  **************************************************************************************************/
#ifdef PROFILE_TIME_ENABLED
uint32_t file_end_start_time1,file_end_start_time2;
uint32_t upd_header_start_time,upd_header_time;
#endif
elfs_result lfs_end_file(_file_handler *file_handler,
                        _table_file_handler *table_file_handler)  {
#ifdef PROFILE_TIME_ENABLED
    file_end_start_time1 = get_micro_sec();
#endif
  _file_header *file_header = &file_handler->head;
  /* if trailing bytes left, write */
  if(file_handler->tmp_write_mem_loc>0) {
    /* Write the last bytes of the file */
    file_header->tmp_write_mem->next_page_data.next_page = 0xFFFFFFFF;
    file_header->tmp_write_mem->next_page_data.occupied = 1;

    struct _page_write write_data = {.page_dest = file_handler->curr_write_mem_loc/g_mem_prop->page_size,
                                      .data_buf = (uint8_t*)file_header->tmp_write_mem->data,
                                      .data_size = file_handler->tmp_write_mem_loc,
                                      .offset = DATA_OFFSET,
                                      .spare_buf = (uint8_t*)&file_header->tmp_write_mem->next_page_data,
                                      .spare_size = sizeof(struct _page_header),
                                      .spare_offset = SPARE_OFFSET};
#ifdef PRINTS_OUT
    NRF_LOG_INFO("*****************size of complete write end of file=%d******************************",sizeof(write_data));
#endif

    /* write remaining bytes */
    if(nand_func_page_and_spare_write(&write_data) != NAND_FUNC_SUCCESS)  {
      NRF_LOG_INFO("Error in closing of file");
      return LFS_FILE_WRITE_ERROR;
    }
    file_header->file_size += file_handler->tmp_write_mem_loc;
    file_handler->tmp_write_mem_loc = 0;
    file_header->last_used_page = file_handler->curr_write_mem_loc/g_mem_prop->page_size;
  }
  /* else close file with last used page */
  else  {
    file_header->last_used_page=file_handler->curr_write_mem_loc/g_mem_prop->page_size-1;
  }

#ifdef PROFILE_TIME_ENABLED
  upd_header_start_time = get_micro_sec();
#endif

  /* update header for fast/manual mode as secure mode already updated while writing */
  switch(file_handler->op_mode) {
    case LFS_MODE_FAST:
    if(lfs_update_header(file_handler,table_file_handler) == LFS_ERROR) {
      NRF_LOG_INFO("Error in updating header in TOC for file closing");
      return LFS_UPDATE_HEADER_ERROR;
    }
    break;
    case LFS_MODE_SECURE:
    case LFS_MODE_MANUAL:
    if(lfs_update_header(file_handler,table_file_handler) == LFS_ERROR) {
      NRF_LOG_INFO("Error in updating header in TOC for closing of file");
      return LFS_UPDATE_HEADER_ERROR;
    }
    break;
  }
#ifdef PROFILE_TIME_ENABLED
    file_end_start_time2 = get_micro_sec()- file_end_start_time1;
    upd_header_time = get_micro_sec() - upd_header_start_time;
#endif
  return LFS_SUCCESS;
}

/*!
  **************************************************************************************************
  * @brief  End the file by writing the remaining bytes and the TOC.
  *
  * @param     _file_handler *file_handler : file handler of the file.
  * @return     elfs_result Function result LFS_SUCCESS/LFS_ERROR
  **************************************************************************************************/
elfs_result lfs_end_config_file(_file_handler *file_handler)  {
  _file_header *file_header = &file_handler->head;
  /* write last few bytes */
  if(file_handler->tmp_write_mem_loc > 0) {
    /* Write the last bytes of the file */
    file_header->tmp_write_mem->next_page_data.next_page = 0xFFFFFFFF;
    file_header->tmp_write_mem->next_page_data.occupied=1;

    struct _page_write write_data = {.page_dest = file_handler->curr_write_mem_loc/g_mem_prop->page_size,
                                      .data_buf = (uint8_t*)file_header->tmp_write_mem->data,
                                      .data_size = file_handler->tmp_write_mem_loc,
                                      .offset = DATA_OFFSET,
                                      .spare_buf = (uint8_t*)&file_header->tmp_write_mem->next_page_data,
                                      .spare_size = sizeof(struct _page_header),
                                      .spare_offset = SPARE_OFFSET};
#ifdef PRINTS_OUT
    NRF_LOG_INFO("*****************size of complete write end of file=%d******************************",sizeof(write_data));
#endif

    /* write page */
    if(nand_func_page_and_spare_write(&write_data) != NAND_FUNC_SUCCESS)  {
      NRF_LOG_INFO("Error in closing of file");
      return LFS_FILE_WRITE_ERROR;
    }
    file_header->file_size += file_handler->tmp_write_mem_loc;
    file_handler->tmp_write_mem_loc = 0;
    file_header->last_used_page = file_handler->curr_write_mem_loc/g_mem_prop->page_size;
  }
  /* close file with last used page */
  else  {
    file_header->last_used_page = file_handler->curr_write_mem_loc/g_mem_prop->page_size-1;
  }

  /* update header for fast/manual mode */
  switch(file_handler->op_mode){
    case LFS_MODE_FAST:
    if(lfs_delete_page(TOCBLOCK*g_mem_prop->pages_per_block+\
                      file_handler->head.header_number) == LFS_ERROR){
      NRF_LOG_INFO("Error in page deletion from TOC");
      return LFS_ERROR;
    }
    if(lfs_update_config_header(file_handler) == LFS_ERROR) {
      NRF_LOG_INFO("Error in updating header in TOC for file closing");
      return LFS_UPDATE_HEADER_ERROR;
    }
    break;
  case LFS_MODE_SECURE:
  case LFS_MODE_MANUAL:
    if(lfs_delete_page(TOCBLOCK*g_mem_prop->pages_per_block+\
                      file_handler->head.header_number) == LFS_ERROR) {
      NRF_LOG_INFO("Error in page deletion from TOC");
      return LFS_ERROR;
    }
    if(lfs_update_config_header(file_handler) == LFS_ERROR) {
      NRF_LOG_INFO("Error in updating header in TOC for closing of file");
      return LFS_UPDATE_HEADER_ERROR;
    }
    break;
  }
  return LFS_SUCCESS;
}

/*!
  **************************************************************************************************
  * @brief      Full flash format
  *
  * @param     _None
  * @return     elfs_result Function result LFS_SUCCESS/LFS_ERROR
  **************************************************************************************************/
elfs_result lfs_flash_reset() {
  uint32_t loop_ind = 0;
  eNand_Result result = NAND_SUCCESS;
  uint8_t status = 0;

  /* erase complete flash */
  for(loop_ind=RESERVEDBLOCK;loop_ind < g_mem_prop->num_of_blocks;loop_ind++) {
    result = nand_flash_block_erase(loop_ind*g_mem_prop->pages_per_block);
    result = nand_flash_get_status(&status);

    if((status & FEAT_STATUS_E_FAIL) == FEAT_STATUS_E_FAIL) {
        NRF_LOG_INFO("Error in Block Erase block no:%d",loop_ind);
      }
  }

#ifdef PRINTS_OUT
   NRF_LOG_INFO("Successful reset");
#endif
   /* if full format success */
   return (result == NAND_SUCCESS ) ? LFS_SUCCESS : LFS_ERROR;
}

/*!
  **************************************************************************************************
  @brief        Erase the memory. This function will skip some of the blocks. The
  *             reserved block will always be skipped. This contains information about
  *             the version of the memory for now, but in the future it could also be
  *             used for other purposes. Also, the function will keep configuration files
  *             if the argument of the function (force) is set to false. In this mode
  *             a temporal block will be used to temporaly back up the pages of the
  *             TOCBLOCK containing headers of configuration files and their content.
  *
  * @param      bool force: true-  Remove both config and data files
  *                           false- Remove only data files
  * @return     elfs_result Function result LFS_SUCCESS/LFS_ERROR
  **************************************************************************************************/
elfs_result lfs_erase_memory(bool force){
  uint32_t loop_ind = 0;
  uint32_t bad_blk_cnt = 0;

  /* if bool flag set to true */
  if(force) {
    /* full flash from TOCBLOCK */
    for(loop_ind=TOCBLOCK;loop_ind < g_mem_prop->num_of_blocks;loop_ind++)  {
      if((nand_func_erase(g_mem_prop,loop_ind,1)) != NAND_FUNC_SUCCESS) {
        NRF_LOG_INFO("Error in formatting memory");
        bad_blk_cnt++;
      }
    }
  }
  /* partial erase */
  else  {
    /* read table page to note down pointers information */
    _table_file_handler tmp_table_file_handler;
    memset(&tmp_table_file_handler,0,sizeof(_table_file_handler));

    /* read table page */
    if(read_table_file_in_toc(&tmp_table_file_handler) != LFS_SUCCESS)  {
      NRF_LOG_INFO("Error in reading table file");
      return LFS_ERROR;
    }

    uint8_t wrap_around_condition = 0;
    uint32_t src_blk_ind = 0;
    uint32_t dst_blk_ind = 0;
    uint32_t curr_head = tmp_table_file_handler.table_file_info.head_pointer;
    uint32_t curr_head_in_blk = tmp_table_file_handler.table_file_info.head_pointer / g_mem_prop->pages_per_block;
    uint32_t curr_tail = tmp_table_file_handler.table_file_info.tail_pointer;
    uint32_t tail_pointer_in_pages = tmp_table_file_handler.table_file_info.tail_pointer * g_mem_prop->pages_per_block;

    /* current head = tail pointer in pages assuming head pointer closes and updates at boundary */
    if(curr_head == tail_pointer_in_pages)  {
      /* No erase */
      if(tmp_table_file_handler.table_file_info.mem_full_flag != 1) {
        NRF_LOG_WARNING("Nothing is written to erase");
        return LFS_SUCCESS;
      }
      /* full flash format from FILEBLOCK , TOC Block has only config if any */
      else  {
        NRF_LOG_WARNING("Memory is full : Formatting Complete Flash except config");
        bad_blk_cnt = 0;
        bool is_bad;

        /* Format Complete flash */
        for(loop_ind = FILEBLOCK;loop_ind < g_mem_prop->num_of_blocks;loop_ind++) {
          if((nand_func_erase(g_mem_prop,loop_ind,1)) != NAND_FUNC_SUCCESS) {
            NRF_LOG_INFO("Error in formatting block ind %d",loop_ind);
            bad_blk_cnt++;
            /* if erase fails, check current block if bad */
             is_bad = false;
             if(nand_func_is_bad_block(g_mem_prop,loop_ind,&is_bad) != NAND_FUNC_SUCCESS) {
                NRF_LOG_INFO("Error in checking Bad block header for %d block",loop_ind);
                return LFS_BAD_BLOCK_HEADER_CHECK_ERROR;
             } else {
                /* if current block is bad, then update bad block list */
                if(is_bad == true)  {
                  uint32_t word_pointer = loop_ind / MAX_NO_OF_BITS_IN_WORD;
                  uint32_t bit_pointer = loop_ind % MAX_NO_OF_BITS_IN_WORD;

                  /* Update Bad block list */
                  tmp_table_file_handler.table_file_info.bad_block_marker[word_pointer] |=  (1 << bit_pointer);
                  del_table_page = 1;
                  memset(type,0,MAX_UPDATE_TYPE);
                  type[0] = LFS_BAD_BLOCK_MARKER_UPDATE;
                  if(update_table_file_in_toc(&tmp_table_file_handler,type,1) != LFS_SUCCESS) {
                    NRF_LOG_INFO("Error in updating Table File %d block",loop_ind);
                    return LFS_UPDATE_TABLE_FILE_ERROR;
                  }
                  del_table_page = 0;
                }
             }
          }
        }

        /* Take back of TOC for config file */
        if(erase_toc_memory(false) != LFS_SUCCESS)  {
          NRF_LOG_INFO("Updated in formatting TOC Block");
          return LFS_TOC_FORMAT_ERROR;
        }

        /* Delete Table page to write */
        if(lfs_delete_page(TOCBLOCK*g_mem_prop->pages_per_block + TABLE_PAGE_INDEX_IN_TOC) == LFS_ERROR)  {
          NRF_LOG_INFO("Error in deleting table file in toc");
          return LFS_ERROR;
        }

        /* Update Mem full flag for erased and for next use  */
        tmp_table_file_handler.table_file_info.mem_full_flag = 0;

        /* offset reset to '0' as count starts from '1'*/
        tmp_table_file_handler.table_file_info.offset = 0;

        /* reset to '1' as its empty */
        tmp_table_file_handler.table_file_info.initialized_circular_buffer = 1;

        /* Write Table File in TOC  */
        if(write_table_file_in_toc(&tmp_table_file_handler) == LFS_ERROR) {
          NRF_LOG_INFO("Write Table File in Update TOC is Error");
          return LFS_ERROR;
        }
      }
    }
    else  {
      /* Calculate Source and Destination Block Index for File Erase based on head and tail positions */
      /* Data written but not erased or repeated erase */
      if((curr_tail == FILEBLOCK) && (curr_head > tail_pointer_in_pages))   {
        src_blk_ind = FILEBLOCK;
        dst_blk_ind = curr_head_in_blk;
      }
      /* both have moved relative to each other */
      else if((curr_tail > FILEBLOCK) && (curr_head_in_blk > FILEBLOCK))  {
        /* Tail have moved foward then head */
        if(tail_pointer_in_pages > curr_head) {
          src_blk_ind = tmp_table_file_handler.table_file_info.tail_pointer + 1;
          dst_blk_ind = g_mem_prop->mem_size/g_mem_prop->block_size-1;
          wrap_around_condition = 1;
        }
        /* Head has moved forward than tail */
        else if(curr_head > tail_pointer_in_pages) {
          src_blk_ind = tmp_table_file_handler.table_file_info.tail_pointer + 1;
          dst_blk_ind = curr_head_in_blk;
        }
      }
      /* invalid erase condition */
      else  {
        NRF_LOG_WARNING("Invalid erase");
        return LFS_SUCCESS;
      }

      /* Erase Data */
      bool is_bad = false;
#ifdef PRINTS_OUT
      NRF_LOG_INFO("Block src ind = %d, Block dst ind = %d for erasal",
                             src_blk_ind,
                             dst_blk_ind);
#endif
      for(loop_ind = src_blk_ind;loop_ind <= dst_blk_ind;loop_ind++)  {
        /* Check Bad Block list if to be erased is bad or good */
        if(check_current_block_is_bad(loop_ind,&tmp_table_file_handler,&is_bad) != LFS_SUCCESS) {
          /* current block index ix greater than 2048 blocks */
          NRF_LOG_WARNING("Error in Checking Block %d is bad,number of blocks out of bounds",loop_ind);
          break;
        } else  {
          /* if bad block, skip erase of current block and proceed with other blocks */
          if(is_bad == true)  {
            continue;
          }
        }
        /*  erase those blocks as pointed out by source and destination block
            indexes */
        if((nand_func_erase(g_mem_prop,loop_ind,1)) == NAND_FUNC_ERROR) {
          NRF_LOG_INFO("Error formating Block No:%i,Updating Bad block list",loop_ind);

          /* Check if block has gone bad */
          is_bad = false;
          if(nand_func_is_bad_block(g_mem_prop,loop_ind,&is_bad) != NAND_FUNC_SUCCESS)  {
            NRF_LOG_INFO("Error in checking Bad block header for %d block",loop_ind);
            return LFS_BAD_BLOCK_HEADER_CHECK_ERROR;
          } else  {
            /* if bad block is found during erase is performed */
            if(is_bad == true)  {
              /* calculate word & bit pointers to update as bad block */
              uint32_t word_pointer = loop_ind / MAX_NO_OF_BITS_IN_WORD;
              uint32_t bit_pointer = loop_ind % MAX_NO_OF_BITS_IN_WORD;

              /* bad block updated , update again */
              vol_info_buff_var.bad_block_updated = 1;

              /* Update Bad block list */
              tmp_table_file_handler.table_file_info.bad_block_marker[word_pointer] |=  (1 << bit_pointer);
              del_table_page = 1;
              memset(type,0,MAX_UPDATE_TYPE);
              type[0] = LFS_BAD_BLOCK_MARKER_UPDATE;
              if(update_table_file_in_toc(&tmp_table_file_handler,type,1) != LFS_SUCCESS) {
                NRF_LOG_INFO("Error in updating Table File %d block",loop_ind);
                return LFS_UPDATE_TABLE_FILE_ERROR;
              }
              del_table_page = 0;
            }
          }
        }
      }

      /* Update Tail Pointer with dest block index as its erased */
      tmp_table_file_handler.table_file_info.tail_pointer = dst_blk_ind;

      /* If tail pointer has reached maximum available blocks, reset tail pointer*/
      if((tmp_table_file_handler.table_file_info.tail_pointer == (g_mem_prop->num_of_blocks-1))\
          && wrap_around_condition) {
        tmp_table_file_handler.table_file_info.tail_pointer = FILEBLOCK;
        wrap_around_condition = 0;
      }
#ifdef PRINTS_OUT
      NRF_LOG_INFO("Head Pointer:%d,Tail Pointer:%d before further erasal",
                             tmp_table_file_handler.table_file_info.head_pointer / g_mem_prop->pages_per_block,
                             tmp_table_file_handler.table_file_info.tail_pointer);
#endif

     /* update calculation of tail pointer variable */
      tail_pointer_in_pages = tmp_table_file_handler.table_file_info.tail_pointer * g_mem_prop->pages_per_block;

      /* check if tail still lags behind head, perform format from
        current tail till head */
      if( tail_pointer_in_pages <= curr_head) {
        src_blk_ind = FILEBLOCK;
        dst_blk_ind = curr_head_in_blk;

        /* format as pointed out from source and destination block index */
        for(loop_ind = src_blk_ind;loop_ind <= dst_blk_ind;loop_ind++)  {
          /* Check Bad Block list for current block if bad */
          if(check_current_block_is_bad(loop_ind,&tmp_table_file_handler,&is_bad) != LFS_SUCCESS) {
#ifdef PRINTS_OUT
            NRF_LOG_WARNING("Error in Checking Block %d is bad,out of bounds",loop_ind);
#endif
            break;
          } else  {
            /* if bad block , skip current block and continue */
            if(is_bad == true)  {
              continue;
            }
          }

          if((nand_func_erase(g_mem_prop,loop_ind,1)) == NAND_FUNC_ERROR) {
#ifdef PRINTS_OUT
            NRF_LOG_INFO("Error formating Block No:%i,Updating Bad block list",loop_ind);
#endif
            /* Check if block has gone bad during format */
            is_bad = false;
            if(nand_func_is_bad_block(g_mem_prop,loop_ind,&is_bad) != NAND_FUNC_SUCCESS)  {
              NRF_LOG_INFO("Error in checking Bad block %d header",loop_ind);
              return LFS_BAD_BLOCK_HEADER_CHECK_ERROR;
            }
            else  {
                /* update list if block has gone bad */
              if(is_bad == true)  {
                /* calculate word/bit pointers for bad block update */
                uint32_t word_pointer = loop_ind / MAX_NO_OF_BITS_IN_WORD;
                uint32_t bit_pointer = loop_ind % MAX_NO_OF_BITS_IN_WORD;

                /* Update Bad block list */
                tmp_table_file_handler.table_file_info.bad_block_marker[word_pointer] |=  (1 << bit_pointer);
                del_table_page = 1;
                memset(type,0,MAX_UPDATE_TYPE);
                type[0] = LFS_BAD_BLOCK_MARKER_UPDATE;
                
                /* bad block updated, update again */
                vol_info_buff_var.bad_block_updated = 1;

                if(update_table_file_in_toc(&tmp_table_file_handler,type,1) != LFS_SUCCESS)
                {
                  NRF_LOG_INFO("Error in updating Table File for Bad Block");
                  return LFS_UPDATE_TABLE_FILE_ERROR;
                }
                del_table_page = 0;
              }
            }
          }
        }
        /* Update Tail Pointer after erase */
        tmp_table_file_handler.table_file_info.tail_pointer = dst_blk_ind;
      }
#ifdef PRINTS_OUT
      NRF_LOG_INFO("Updated Tail Pointer:%d",tmp_table_file_handler.table_file_info.tail_pointer);
#endif

      /* Take back of TOC for config file */
      if(erase_toc_memory(false) != LFS_SUCCESS)  {
        NRF_LOG_ERROR("Updated in formatting TOC Block");
        return LFS_TOC_FORMAT_ERROR;
      }

#ifdef DEBUG_CODES
       /* get no of bad blocks */
       uint32_t bad_block_num = 0;
      if(get_bad_block_number(&bad_block_num,FILEBLOCK,g_mem_prop->mem_size/g_mem_prop->block_size,&tmp_table_file_handler) != LFS_SUCCESS)
      {
         return LFS_NUM_BLOCKS_OUT_OF_BOUNDS_ERROR;
      }
#endif
     /* Reset Mem full flag for erased and for next use  */
    tmp_table_file_handler.table_file_info.mem_full_flag=0;

    /* offset reset to '0' as count starts from '1' */
    tmp_table_file_handler.table_file_info.offset = 0;

    /* reset to '1' as its empty */
    tmp_table_file_handler.table_file_info.initialized_circular_buffer = 1;

    memset(type,0,MAX_UPDATE_TYPE);
    type[0] = LFS_TAIL_POINTER_UPDATE;
    type[1] = LFS_OFFSET_UPDATE;
    type[2] = LFS_MEM_FULL_FLAG_UPDATE;
    type[3] = LFS_INITIALIZE_BUFFER_FLAG_UPDATE;
    del_table_page = 1;

    /* Update Tail Pointer on to TOC */
    if(update_table_file_in_toc(&tmp_table_file_handler,type,4) != LFS_SUCCESS) {
        NRF_LOG_INFO("Updated in updating Table File");
        return LFS_UPDATE_TABLE_FILE_ERROR;
    }
    else  {
#ifdef PRINTS_OUT
        NRF_LOG_INFO("Updated Tail Pointer:%d",tmp_table_file_handler.table_file_info.tail_pointer);
#endif
      }
       del_table_page = 0;
    }
#ifdef DEBUG_CODES
       /* get no of bad blocks */
       uint32_t bad_block_num = 0;
      if(get_bad_block_number(&bad_block_num,FILEBLOCK,g_mem_prop->mem_size/g_mem_prop->block_size,&tmp_table_file_handler) != LFS_SUCCESS)
      {
         return LFS_NUM_BLOCKS_OUT_OF_BOUNDS_ERROR;
      }
#endif
  }
  return LFS_SUCCESS;
}

/*!
  **************************************************************************************************
  @brief            Erase the memory. This function will skip some of the blocks. The
  *                 reserved block will always be skipped. This contains information about
  *                 the version of the memory for now, but in the future it could also be
  *                 used for other purposes. Also, the function will keep configuration files
  *                 if the argument of the function (force) is set to false. In this mode
  *                 a temporal block will be used to temporaly back up the pages of the
  *                 TOCBLOCK containing headers of configuration files and their content.
  *
  * @param      bool force: true-  Remove both config and data files
  *                         false- Remove only data files
  * @return     elfs_result Function result LFS_SUCCESS/LFS_ERROR
  **************************************************************************************************/
elfs_result erase_toc_memory(bool force)  {
  uint32_t i;
  uint8_t new_file_number = 0;
  _file_header tmp_file_header;
  _table_file_handler tmp_table_file_handler;
  _table_file_header *tmp_table_file_header = &tmp_table_file_handler.table_file_info;
  struct _page_header page_info;

  if(force == false)  {
    // Write Next Page Info
    page_info.next_page = 0xFFFFFFFF;
    page_info.occupied = 1;
    uint32_t page_pos;

    /* Erase Temp block for back up */
    if(nand_func_erase(g_mem_prop,TMPBLOCK,1) != NAND_FUNC_SUCCESS) {
      NRF_LOG_INFO("Error in erasing TOC block");
      return LFS_ERROR;
    }

    /* Backup the handlers of the config files to the TMPBLOCKs */
    for(i = 0;i < MAXFILENUMBER;i++)  {
      /* read toc block data */
      if(nand_func_read(g_mem_prop,TOCBLOCK*g_mem_prop->block_size+\
                  i*g_mem_prop->page_size,sizeof(_file_header),\
                  (uint8_t*)&tmp_file_header) != NAND_FUNC_SUCCESS) {
        NRF_LOG_ERROR("Error in reading config file Information %d",
                    TOCBLOCK*g_mem_prop->pages_per_block+new_file_number);
        return LFS_ERROR;
      }

      /* if config file only take back up */
      if(tmp_file_header.file_type == LFS_CONFIG_FILE)  {
        tmp_file_header.header_number = new_file_number;
          /* write data from toc to temp */
         if(nand_func_write(g_mem_prop,TMPBLOCK*g_mem_prop->block_size+\
                            new_file_number*g_mem_prop->page_size, sizeof(_file_header),
                            (uint8_t*)&tmp_file_header) != NAND_FUNC_SUCCESS) {
            NRF_LOG_INFO("Error in writing Temporary Block with config file no %d",\
                        TMPBLOCK*g_mem_prop->pages_per_block+new_file_number);
            return LFS_ERROR;
         }
        new_file_number++;
      }
    }
    /* Read TOC File in structure to preserve contents */
    if(read_table_file_in_toc(&tmp_table_file_handler) == LFS_ERROR)  {
      NRF_LOG_INFO("Error in read table file in TOC");
      return LFS_ERROR;
    }

     page_pos = TMPBLOCK*g_mem_prop->pages_per_block+TABLE_PAGE_INDEX_IN_TOC;

    /*  Write the header and the data */
    struct _page_write write_data = {.page_dest=page_pos,
    .data_buf = (uint8_t*)tmp_table_file_header,
    .data_size = sizeof(_table_file_header),
    .offset = DATA_OFFSET,
    .spare_buf = (uint8_t*)&page_info,
    .spare_size = sizeof(struct _page_header),
    .spare_offset = SPARE_OFFSET};
    if(nand_func_page_and_spare_write(&write_data) != NAND_FUNC_SUCCESS)
    {
      NRF_LOG_INFO("Error in taking back up of table file in TMP block");
      return LFS_ERROR;
    }
  }
  /* Erase TOC */
  if(nand_func_erase(g_mem_prop,TOCBLOCK,1) != NAND_FUNC_SUCCESS) {
    NRF_LOG_INFO("Error in erasing TOC block");
    return LFS_ERROR;
  }
  /* if false, after toc erase replace back to toc from temp */
  if(force == false)  {
    /* Move to the TOC the file handlers of the config files */
    struct _page_header header_page_info;
    header_page_info.next_page = 0xFFFFFFFF;
    header_page_info.occupied = 1;
    for(i = 0;i < new_file_number;i++)  {
      /* move data area from tmp to toc */
      if(nand_func_page_move(TMPBLOCK*g_mem_prop->pages_per_block+i, \
                            TOCBLOCK*g_mem_prop->pages_per_block+i) != NAND_FUNC_SUCCESS) {
         NRF_LOG_INFO("Error in moving page %d from temporary block and TOC block ",TMPBLOCK*g_mem_prop->pages_per_block+i);
         return LFS_ERROR;
      }
      /* write spare area to toc from temp */
      if(nand_func_write_ecc_zone(TOCBLOCK*g_mem_prop->pages_per_block+i,\
                                  SPARE_OFFSET, sizeof(struct _page_header), \
                                  (uint8_t*)&header_page_info) != NAND_FUNC_SUCCESS)  {
        NRF_LOG_INFO("Error in writing spare area of page %d on to TOC block",TOCBLOCK*g_mem_prop->pages_per_block+i);
        return LFS_ERROR;
      }
    }
    /* move data area of table page from temp to toc */
    if(nand_func_page_move(TMPBLOCK*g_mem_prop->pages_per_block+TABLE_PAGE_INDEX_IN_TOC,\
                          TOCBLOCK*g_mem_prop->pages_per_block+TABLE_PAGE_INDEX_IN_TOC) != NAND_FUNC_SUCCESS) {
      NRF_LOG_INFO("Error in writing Table File from temporary block to TOC");
      return LFS_ERROR;
    }
    /* move spare area of table page from temp to toc */
    if(nand_func_write_ecc_zone(TOCBLOCK * g_mem_prop->pages_per_block+TABLE_PAGE_INDEX_IN_TOC,\
                                SPARE_OFFSET, sizeof(struct _page_header), \
                                (uint8_t*)&header_page_info) != NAND_FUNC_SUCCESS)  {
       NRF_LOG_INFO("Error in writing Table File to spare area of TOC");
      return LFS_ERROR;
    }
  }

  return LFS_SUCCESS;
}

/*!
  **************************************************************************************************
  *@brief                    Function to check if the current version on the memory is compatible
  *                         with the software version. If it is not compatible it is not recommended
  *                         to keep using it unless the user knows that even having different
  *                         versions it will perform fine. This function should be called whenever
  *                         the system starts. It just takes some milliseconds.
  *@params                   is_compatible - version compatable/not
  *@return     elfs_result Function result LFS_SUCCESS/LFS_ERROR
  **************************************************************************************************/
elfs_result lfs_check_comp_version(bool * is_compatible) {
  _fs_info tmp_file_info;

  /* Read within the reserved block */
  if(nand_func_read(g_mem_prop,(RESERVEDBLOCK*g_mem_prop->block_size),\
                    sizeof(_fs_info),(uint8_t*)&tmp_file_info) != NAND_FUNC_SUCCESS)  {
    NRF_LOG_INFO("Error in reading version Information for version");
    return LFS_READ_RESERVED_INFO_ERROR;
  }

  NRF_LOG_INFO("foot print = %s, version = %d, revision = %d",
  nrf_log_push(tmp_file_info.foot_print),
  tmp_file_info.version,
  tmp_file_info.revision);

  uint8_t i;

  /* check for foot print match */
  for(i = 0;i < strlen(FSFOOTPRINT);i++)  {
    if(file_system_info.foot_print[i] != tmp_file_info.foot_print[i])
    {
      *is_compatible = false;
      return LFS_SUCCESS;
    }
  }

  /* check for version/revision match */
  if((tmp_file_info.version == FSVERSION) && (tmp_file_info.revision == FSREVISION))  {
    *is_compatible = true;
  } else  {
    *is_compatible = false;
  }
  return LFS_SUCCESS;
}

/*!
  **************************************************************************************************
  *@brief                   Erase the memory and write the FS version/revision information. This
  *                         also erases the config files. This function should be called the first
  *                         time the filesystem is going to operate on the memory. It can also be
  *                         called whenever the version of the filesystem is upgraded and therefore
  *                         the version information has to be updated.
  *@param      bfmt_config_blk: block to format
  *@return     elfs_result Function result LFS_SUCCESS/LFS_ERROR
  **************************************************************************************************/
elfs_result lfs_format(bool bfmt_config_blk)  {
    eNand_Func_Result result;

    /* erase memory by forcing */
    if(lfs_erase_memory(bfmt_config_blk) != LFS_SUCCESS)  {
      NRF_LOG_ERROR("Error in formatting memory");
      return LFS_FORMAT_ERROR;
    }

    /* Erase the reserved block */
    result = nand_func_erase(g_mem_prop,RESERVEDBLOCK,1);

    /* Then, write the reserved block information */
    result |= nand_func_write(g_mem_prop,(RESERVEDBLOCK*g_mem_prop->block_size), \
                             sizeof(_fs_info), (uint8_t*)&file_system_info);

    if(result != NAND_FUNC_SUCCESS) {
      NRF_LOG_ERROR("Error in writing  File System Information");
      return LFS_WRITE_FAILED_RESERVED_INFO_ERROR;
    }

    return LFS_SUCCESS;
}

/*!
  **************************************************************************************************
  *@brief                   Reset fs task after full flash erase
  *@param                   None
  *@return                  elfs_result Function result LFS_SUCCESS/LFS_ERROR
  **************************************************************************************************/
elfs_result lfs_reset_file_system() {
    eNand_Func_Result result = NAND_FUNC_SUCCESS;

    /* Write the reserved block information */
    result |= nand_func_write(g_mem_prop,(RESERVEDBLOCK*g_mem_prop->block_size),\
                              sizeof(_fs_info), (uint8_t*)&file_system_info);

    if(result != NAND_FUNC_SUCCESS) {
      NRF_LOG_ERROR("Error in writing  File System Information");
      return LFS_WRITE_FAILED_RESERVED_INFO_ERROR;
    }

    /* init pointers, bad block info */
    if(initialize_circular_buffer() != LFS_SUCCESS) {
      NRF_LOG_INFO("Error in resetting pointers");
      return LFS_ERROR;
    }
    return LFS_SUCCESS;
}

/*!
  **************************************************************************************************
  *@brief                   Set the time_stamp for a file. This has to be called before ending the
  *                         file if FAST Mode is chosen. If the _file_handler is being updated using
  *                         manual or secure mode, it can be updated at any point.
  * @param      _file_handler * _file_handler:  file to set the mode
  * @param       time_stamp:                    Time stamp value
  *@return     elfs_result Function result LFS_SUCCESS/LFS_ERROR
  **************************************************************************************************/
elfs_result lfs_set_time_stamp(_file_handler *file_handler,
                              uint32_t time_stamp)  {
  /* if fule handler null, return error */
  if(file_handler ==NULL) {
    return LFS_ERROR;
  }
  file_handler->head.time_stamp = time_stamp;

  return LFS_SUCCESS;
}

/*!
  **************************************************************************************************
  * @brief      Set manual/secure/fast mode for a _file_handler
  *
  * @param      _file_handler * _file_handler: file to set the mode
  * @param      LFS_OP_MODE op_mode: Mode to set
  *
  *@return      elfs_result Function result LFS_SUCCESS/LFS_ERROR
  **************************************************************************************************/
elfs_result lfs_set_operating_mode(_file_handler *file_handler,
                                  elfs_op_mode op_mode) {
  if(op_mode > LFS_MODE_NUMBER) {
    NRF_LOG_WARNING("File Operation mode not defined");
    return LFS_ERROR;
  }
  file_handler->op_mode = op_mode;
  return LFS_SUCCESS;
}


/*!
  **************************************************************************************************
  *@brief                   Update the file header
  * @param                _file_handler * _file_handler: file to update the header
  *@return     elfs_result Function result LFS_SUCCESS/LFS_ERROR
  **************************************************************************************************/
elfs_result lfs_refresh_header(_file_handler *file_handler,
                              _table_file_handler *table_file_handler)  {
  if(file_handler == NULL)
    return LFS_ERROR;
  if(file_handler->op_mode == LFS_MODE_MANUAL)  {
#ifdef PROFILE_TIME_ENABLED
uint32_t nTick = 0;
nTick = MCU_HAL_GetTick();
#endif
    /* if manual mode , update header done for every block */
    if(lfs_update_header(file_handler,table_file_handler) != LFS_SUCCESS) {
      NRF_LOG_INFO("Error in updating File header Information");
      return LFS_UPDATE_HEADER_ERROR;
    }
#ifdef PROFILE_TIME_ENABLED
     file_header_update_time = MCU_HAL_GetTick() - nTick;
     NRF_LOG_INFO("Time taken refresh header = %d",file_header_update_time);
#endif
  }
  /* other mode set */
  else  {
    NRF_LOG_INFO("Error in refreshing header , choose manual mode");
    return LFS_ERROR;
  }
  return LFS_SUCCESS;
}


/*!
  **************************************************************************************************
  *@brief                   Update the config file header
  * @param                _file_handler * _file_handler: file to update the header
  *@return     elfs_result Function result LFS_SUCCESS/LFS_ERROR
  **************************************************************************************************/
elfs_result lfs_refresh_config_header(_file_handler *file_handler)  {
  if(file_handler == NULL)
    return LFS_ERROR;
  if(file_handler->op_mode == LFS_MODE_MANUAL)  {
    /* if manual mode is set, update config header */
    if(lfs_delete_page(TOCBLOCK*g_mem_prop->pages_per_block+file_handler->head.header_number) != LFS_SUCCESS) {
      return LFS_ERROR;
    }
    if(lfs_update_config_header(file_handler) != LFS_SUCCESS) {
      NRF_LOG_INFO("Error in updating File header Information");
      return LFS_UPDATE_HEADER_ERROR;
    }
  }
  /* other mode set */
  else {
    NRF_LOG_INFO("Error in refreshing header , choose manual mode");
    return LFS_ERROR;
  }
  return LFS_SUCCESS;
}


/*!
  **************************************************************************************************
   @brief       Mark a block as bad
  *
  * @param      uint32_t block_index: block to mark as bad
  *@return      elfs_result Function result LFS_SUCCESS/LFS_ERROR
  **************************************************************************************************/
elfs_result lfs_mark_bad(uint32_t block_index)  {
  /* return error if block > 2048 */
  if(block_index > g_mem_prop->num_of_blocks) {
    return LFS_NUM_BLOCKS_OUT_OF_BOUNDS_ERROR;
  }
  /* mark block as bad */
  if(nand_func_mark_bad_block(g_mem_prop,block_index) != NAND_FUNC_SUCCESS) {
    NRF_LOG_INFO("Error in refreshing header , choose manual mode");
    return LFS_ERROR;
  }
  return LFS_SUCCESS;
}

/*!
  **************************************************************************************************
   @brief       Mark a block as good
  *
  * @param      uint32_t block_index: block to mark as good
  *@return      elfs_result Function result LFS_SUCCESS/LFS_ERROR
  **************************************************************************************************/
elfs_result lfs_mark_good(uint32_t block_index) {
  /* return error if block > 2048 */
  if(block_index > g_mem_prop->num_of_blocks) {
    NRF_LOG_INFO("Block Number out of bounds");
    return LFS_NUM_BLOCKS_OUT_OF_BOUNDS_ERROR;
  }
  /* mark block as good */
  if(nand_func_bad_block_mark_reset(g_mem_prop,block_index) != NAND_FUNC_SUCCESS) {
    NRF_LOG_INFO("Error in Bad block reset");
    return LFS_ERROR;
  }
  return LFS_SUCCESS;
}

/*!
  **************************************************************************************************
  *@brief       Obtain the next page while writing. If bad blocks are found, block is skipped,
                also function checks for memory full
  *
  *@param       *file_handler: Pointer to file handler opened to write
  *             *table_file_handler: Pointer to table file handler
  *@return      elfs_result Function result LFS_SUCCESS/LFS_ERROR
  **************************************************************************************************/
static elfs_result get_next_page_files(_file_handler *file_handler,
                                      _table_file_handler *tmp_table_file_handler)  {
   uint32_t current_block;
   bool is_bad = false;
   uint8_t init_roll_over_flag = 0;

   /* start bad block check from current block as indicated by head pointer */
   current_block = tmp_table_file_handler->table_file_info.head_pointer / g_mem_prop->pages_per_block;
    do  {
      /* If current block is same when reached maximum number of blocks in flash */
      if(current_block >= (g_mem_prop->num_of_blocks))  {
        /* roll over to FILEBLOCK  */
        current_block = FILEBLOCK;
        init_roll_over_flag = 1;
      }

      /* check whether current block which is about to be written is bad */
      if(check_current_block_is_bad(current_block,tmp_table_file_handler,&is_bad) != LFS_SUCCESS) {
        NRF_LOG_INFO("Error in checking %d Bad block in Bad block list",current_block);

      }
      /* If bad, increment bad block pointer */
      if(is_bad == true)  {
        current_block++;
      }
    }while(is_bad == true);

    /* Update Head Pointer based on Bad block check if Current block has incremented bad block while skipping */
    if(current_block != (tmp_table_file_handler->table_file_info.head_pointer / g_mem_prop->pages_per_block)) {
      /* then modify Head Pointer to boundary of block which has been updated */
      tmp_table_file_handler->table_file_info.head_pointer = current_block * g_mem_prop->pages_per_block;
    }
    NRF_LOG_INFO("Updated head_pointer after Bad Block Check:%d",tmp_table_file_handler->table_file_info.head_pointer);
    /* While traversing If Head reaches Max, roll it back to FILEBLOCK  */
    if(init_roll_over_flag == 1)  {
      /* reinitialized to 0 after roll back of Head Pointer */
      tmp_table_file_handler->table_file_info.initialized_circular_buffer=0;
      del_table_page = 1;
      memset(type,0,MAX_UPDATE_TYPE);
      type[0] = LFS_INITIALIZE_BUFFER_FLAG_UPDATE;
      /* Update in TOC roll over flag */
      if(update_table_file_in_toc(tmp_table_file_handler,type,1) != LFS_SUCCESS)  {
        NRF_LOG_INFO("Error in updating Table File for Circular Buffer Initialization");
        return LFS_UPDATE_TABLE_FILE_ERROR;
      }
      del_table_page = 0;
      init_roll_over_flag = 0;
    }

    /* If Head and Tail both are initialized to FILEBLOCK , then do not write
       as its memory full */
    if(tmp_table_file_handler->table_file_info.initialized_circular_buffer != 1)  {
      if(((tmp_table_file_handler->table_file_info.head_pointer / g_mem_prop->pages_per_block) == FILEBLOCK) &&\
       (tmp_table_file_handler->table_file_info.tail_pointer == FILEBLOCK)) {
        NRF_LOG_WARNING("Case 1: Memory is full, call format");
        tmp_table_file_handler->table_file_info.mem_full_flag=1;
        /* close file as its memory full */
        if(lfs_end_file(file_handler,tmp_table_file_handler) != LFS_SUCCESS)  {
          NRF_LOG_WARNING("Error in closing File %s",file_handler->head.file_name);
          return LFS_FILE_CLOSE_ERROR;
        }
        return LFS_MEMORY_FULL_ERROR;
      }
      /* When T and H Coincide ,  then do not write as its memory full */
      else if(((tmp_table_file_handler->table_file_info.head_pointer/g_mem_prop->pages_per_block) > FILEBLOCK) && \
              (tmp_table_file_handler->table_file_info.tail_pointer > FILEBLOCK)) {

        if((tmp_table_file_handler->table_file_info.head_pointer/g_mem_prop->pages_per_block) == tmp_table_file_handler->table_file_info.tail_pointer)  {
          NRF_LOG_WARNING("Case 2: Memory is full, call format");
          tmp_table_file_handler->table_file_info.mem_full_flag=1;

          /* close file as its memory full */
          if(lfs_end_file(file_handler,tmp_table_file_handler) != LFS_SUCCESS)  {
            NRF_LOG_WARNING("Error in closing File %s",file_handler->head.file_name);
            return LFS_FILE_CLOSE_ERROR;
          }
          return LFS_MEMORY_FULL_ERROR;
        }
      }
    }
    return LFS_SUCCESS;
}

/*!
  **************************************************************************************************
  *@brief       Get the first file number found
  *
  *@param       * position: Location of the first void slot on the TOC found
  *               file_type: LFS_DATA_FILE / LFS_CONFIG_FILE
  *@return      elfs_result Function result LFS_SUCCESS/LFS_ERROR
  **************************************************************************************************/
static elfs_result get_first_header_void(_table_file_handler *table_file_handler,
                                        elfs_file_type file_type) {

  /* consider offset only if its data file */
  if(file_type == LFS_DATA_FILE){
   /* If offset > 63 ( config + data ) return error as max files that can be stored is reached */
    if(table_file_handler->table_file_info.offset == (MAXFILENUMBER-1))  {
#ifdef PRINTS_OUT
      NRF_LOG_ERROR("Maximum file count reached");
#endif
      return LFS_MAX_FILE_COUNT_ERROR;
    }
  }

  /* low touch config pos if already occupied, return error as only one config file can be written */
  if((file_type == LFS_CONFIG_FILE) &&\
      (table_file_handler->table_file_info.config_low_touch_occupied==1)) {
    NRF_LOG_INFO("Error in obtaining File Position for Low touch config file \
                because low touch cnfig file occupied");
    return LFS_CONFIG_FILE_POSITION_ERROR;
  }

  /* Increment offset from previous value for new file creation */
  if(file_type == LFS_DATA_FILE)  {
    table_file_handler->table_file_info.offset += 1;
  }
  else if(file_type == LFS_CONFIG_FILE) {
    /* config file pos occupied */
    table_file_handler->table_file_info.config_low_touch_occupied =  1;
  }

  NRF_LOG_INFO("Position header number= %d",table_file_handler->table_file_info.offset);
  return LFS_SUCCESS;
}


/*!
  **************************************************************************************************
  *@brief       Obtain first available page for config file
  *@param       * first_page: Location of the first void slot on the TOC found
  *@return      elfs_result Function result LFS_SUCCESS/LFS_ERROR
  **************************************************************************************************/
static elfs_result get_first_config_page_free(uint32_t * first_page)  {
  uint32_t next_page;
  next_page = CFGBLOCK*g_mem_prop->pages_per_block;
  bool block_status;

  /* check configuration block if good */
  if(nand_func_is_bad_block(g_mem_prop,CFGBLOCK,&block_status) != NAND_FUNC_SUCCESS)  {
    NRF_LOG_INFO("Errorin checking Config block is bad");
    return LFS_BAD_BLOCK_HEADER_CHECK_ERROR;
  }

  /* return error, if config block is bad */
  if(block_status)  {
    NRF_LOG_INFO("Config block is bad");
    return LFS_ERROR;
  }

  struct _page_header next_page_head;
  do  {
    *first_page = next_page;
    /* Check that the current page is not occupied */
    if(lfs_read_oob(next_page,&next_page_head) != LFS_SUCCESS)  {
      NRF_LOG_INFO("Error reading page header for config file");
      return LFS_ERROR;
    }
    /* Incrment pointer till config pages are occupied in config block */
    if(next_page_head.occupied == 1)
     next_page++;

    /* if pointer goes beyond config block , return error */
    if(next_page >= (CFGBLOCK+1)*g_mem_prop->pages_per_block) {
      NRF_LOG_INFO("Invalid Config Page to write");
      return LFS_INVALID_NEXT_WRITABLE_CONFIG_PAGE_ERROR;
    }
  } while(next_page_head.occupied == 1);
  return LFS_SUCCESS;
}

/*!
  **************************************************************************************************
  *@brief         Set the read position to continue reading the file.
  * @param        read_pos: Position to set the pointer
  * @param        _file_header * _file_header : file handler that will be used
  *@return      elfs_result Function result LFS_SUCCESS/LFS_ERROR
  **************************************************************************************************/
static elfs_result lfs_set_read_pos(uint32_t read_pos,
                                  _file_handler *file_handler)  {
  _file_header const *file_header = &file_handler->head;
  /* go through the file until we reach read_pos: */
  file_handler->current_read_pos = TOCBLOCK*g_mem_prop->block_size + \
                                   file_header->mem_loc*g_mem_prop->page_size;
  uint32_t bytes_processed = 0;
  struct _page_header page_status;

  while(bytes_processed+g_mem_prop->page_size < read_pos) {
    /* read spare area of page based on currrent read pos of file */
    if(lfs_read_oob(file_handler->current_read_pos/g_mem_prop->page_size,&page_status) == LFS_ERROR)  {
      NRF_LOG_INFO("Error in reading file header to set position pointer");
      return LFS_ERROR;
    }
    /* keep incrementing in bytes till read pos argument is obtained */
    file_handler->current_read_pos = page_status.next_page*g_mem_prop->page_size;
    bytes_processed += g_mem_prop->page_size;
  }
  file_handler->current_read_pos+=read_pos-bytes_processed;
  return LFS_SUCCESS;
}

/*!
  **************************************************************************************************
  *@brief         When using manual mode, update the header of the file.
  * @param       _file_handler *file_handler : file handler that will be used
  * @param       _table_file_handler *table_file_handler : table file handler that will be used
  *@return        elfs_result Function result LFS_SUCCESS/LFS_ERROR
  **************************************************************************************************/
static elfs_result lfs_update_header(_file_handler *file_handler,
                                    _table_file_handler *table_file_handler)  {
  eNand_Func_Result result = NAND_FUNC_SUCCESS;
  struct _page_header header_page_info;
  header_page_info.next_page=0xFFFFFFFF;
  header_page_info.occupied=1;
  _file_header *file_header = &file_handler->head;

  uint32_t page_pos = (TOCBLOCK*g_mem_prop->pages_per_block) + \
                        file_header->header_number;
  memset(pos_ind_to_del,0,sizeof(pos_ind_to_del));
  pos_ind_to_del[0]=page_pos;
  pos_ind_to_del[1]=(TOCBLOCK*g_mem_prop->pages_per_block) + \
                      TABLE_PAGE_INDEX_IN_TOC;

  /* delete 2 pages ( file handler & and table file ) */
   if(lfs_delete_toc_pages(pos_ind_to_del,NUM_OF_TOC_PAGES_TO_DEL_FOR_HEADER_UPD) != LFS_SUCCESS){
      return LFS_ERROR;
   }

  struct _page_write write_data = {.page_dest=page_pos,
  .data_buf=(uint8_t*)file_header,
  .data_size=sizeof(_file_header),
  .offset=DATA_OFFSET,
  .spare_buf=(uint8_t*)&header_page_info,
  .spare_size=sizeof(struct _page_header),
  .spare_offset=SPARE_OFFSET};
#ifdef PRINTS_OUT
  NRF_LOG_INFO("********Size of Data buffer = %d,spare buffer=%d***********",write_data.data_size,write_data.spare_size);
#endif
#ifdef PROFILE_TIME_ENABLED
  t1 = get_micro_sec();
#endif
  /* write file handler information */
  result = nand_func_page_and_spare_write(&write_data);

#ifdef PROFILE_TIME_ENABLED
  lfs_refresh_header_time = get_micro_sec() - t1;
  NRF_LOG_INFO("********* refresh header time for single page= %d *************",lfs_refresh_header_time);
#endif

  /* If writing file information, return error from header update */
  if(result != NAND_FUNC_SUCCESS) {
    NRF_LOG_WARNING("Error in updating File header %s",file_handler->head.file_name);
    return LFS_ERROR;
  }

   NRF_LOG_INFO("Head pointer during closing of file = %d, Tail pointer during closing of file = %d",
    table_file_handler->table_file_info.head_pointer,table_file_handler->table_file_info.tail_pointer);

  /* write table page information */
  if(write_table_file_in_toc(table_file_handler) == LFS_ERROR)  {
    /* If writing table page information, return error from header update */
    NRF_LOG_INFO("Write Table File in Update TOC is Error");
    return LFS_ERROR;
  }
  return LFS_SUCCESS;
}

/*!
  **************************************************************************************************
  * @brief          Read the Out of band of a page.
  *
  * @param          page: Page to read the OOB from
  * @param          struct _page_header * page_head: Page header to store the information
>>>>>>> PERSEUS-338-add-doxygen-style-comments-for-all-the-functions-in-studywatch-source-code
  *                                          in
  *@return        elfs_result Function result LFS_SUCCESS/LFS_ERROR
  **************************************************************************************************/
static elfs_result lfs_read_oob(uint32_t page,
                                struct _page_header *page_head) {
  /* read spare area */
  if(nand_func_read_ecc_zone(page,SPARE_OFFSET,\
                            sizeof(struct _page_header),\
                            (uint8_t*)page_head) != NAND_FUNC_SUCCESS)  {
    NRF_LOG_INFO("Error in reading Spare area of page %d",page);
    return LFS_ERROR;
  }

  return LFS_SUCCESS;
}

#ifdef PROFILE_TIME_ENABLED
static uint32_t backup_time,erase_time,replace_time;
static uint32_t ReadSpareAreaTime=0,WriteSpareAreaTime=0,PageMovTime=0;
#endif
/*!
  **************************************************************************************************
  * @brief          Delete a single page. This will result in bytes of page set to 0xFF
  *
  * @param           page: Page to delete
  * @return        elfs_result Function result LFS_SUCCESS/LFS_ERROR
  **************************************************************************************************/
static elfs_result lfs_delete_page(uint32_t page) {
  uint32_t current_block = page / g_mem_prop->pages_per_block;
  uint32_t page_init = current_block*g_mem_prop->pages_per_block;
  eNand_Func_Result result = NAND_FUNC_SUCCESS;

  /* Delete the TMPBLOCK */
  result |= nand_func_erase(g_mem_prop,TMPBLOCK,1);
  if(result != NAND_FUNC_SUCCESS) {
     NRF_LOG_INFO("Error in erasing Tmp block odd");
  }

  uint8_t read_ecc[ sizeof(struct _page_header)];
  uint32_t i;
#ifdef PROFILE_TIME_ENABLED
uint32_t nTick = 0;
nTick = MCU_HAL_GetTick();
#endif
  /* iterate through all 64 pages of current block */
  for(i=0;i<g_mem_prop->pages_per_block;i++)  {
    if((page_init+i)!=page) {
#ifdef PROFILE_TIME_ENABLED
     uint32_t tick1=0;
     tick1 = get_micro_sec();
#endif
      /* read spare area of temp */
      result |= nand_func_read_ecc_zone(page_init+i,SPARE_OFFSET, \
                                        sizeof(struct _page_header), \
                                        read_ecc);
      if(result != NAND_FUNC_SUCCESS) {
        NRF_LOG_INFO("Error in reading ECC of page %d",page_init+i);
      }
#ifdef PROFILE_TIME_ENABLED
      ReadSpareAreaTime = get_micro_sec() - tick1;
      tick1 = get_micro_sec();
#endif
      /* write spare area into tmp */
      result |= nand_func_write_ecc_zone(TMPBLOCK*g_mem_prop->pages_per_block+i,\
                                        SPARE_OFFSET, sizeof(struct _page_header),\
                                        read_ecc);
      if(result != NAND_FUNC_SUCCESS) {
          NRF_LOG_INFO("Error in writing ECC Zone tmp block odd");
      }
#ifdef PROFILE_TIME_ENABLED
      WriteSpareAreaTime = get_micro_sec() - tick1;
      tick1 = get_micro_sec();
#endif
      /* page move of data area from current block pages to temp's block pages */
      result |= nand_func_page_move(page_init+i, \
                                    TMPBLOCK*g_mem_prop->pages_per_block+i);
      if(result != NAND_FUNC_SUCCESS) {
          NRF_LOG_INFO("Error in page move operation");
      }
#ifdef PROFILE_TIME_ENABLED
      PageMovTime = get_micro_sec() - tick1;
#endif
    }
  }
#ifdef PROFILE_TIME_ENABLED
     backup_time = MCU_HAL_GetTick() - nTick;
     NRF_LOG_INFO("Time taken to take back up = %d",backup_time);
     nTick = 0;
     nTick = MCU_HAL_GetTick();
#endif

  /* Erase current block  */
  result |= nand_func_erase(g_mem_prop,current_block,1);
  if(result != NAND_FUNC_SUCCESS) {
    NRF_LOG_INFO("Error in erasing current block=%d",current_block);
  }
#ifdef PROFILE_TIME_ENABLED
     erase_time = MCU_HAL_GetTick() - nTick;
     NRF_LOG_INFO("Time taken for erase = %d",erase_time);
     nTick = 0;
     nTick = MCU_HAL_GetTick();
#endif
  /* Iterate through all 64 pages of temp */
  for(i=0;i < g_mem_prop->pages_per_block;i++)  {
    if((page_init+i)!=page) {
      /* read spare area of temp */
      result |= nand_func_read_ecc_zone(TMPBLOCK*g_mem_prop->pages_per_block+i,\
                                        SPARE_OFFSET, sizeof(struct _page_header), \
                                        read_ecc);
      if(result != NAND_FUNC_SUCCESS) {
         NRF_LOG_INFO("Error in reading ECC back from tmp block odd");
      }

      /* page move for data area from temp to current block */
      result  |= nand_func_page_move(TMPBLOCK*g_mem_prop->pages_per_block+i, page_init+i);
      if(result != NAND_FUNC_SUCCESS) {
        NRF_LOG_INFO("Error in page move operation");
      }

      /* write spare area to current block */
      result |= nand_func_write_ecc_zone(page_init+i,SPARE_OFFSET,\
                                        sizeof(struct _page_header), \
                                        read_ecc);
      if(result != NAND_FUNC_SUCCESS) {
        NRF_LOG_INFO("Error in writing ecc zone %d",page_init+i);
      }
    }
  }
#ifdef PROFILE_TIME_ENABLED
     replace_time = MCU_HAL_GetTick() - nTick;
     NRF_LOG_INFO("Time taken for replacing back = %d",replace_time);
#endif
  if(result == NAND_FUNC_SUCCESS)
    return LFS_SUCCESS;

#ifdef PRINTS_OUT
  NRF_LOG_INFO("success in deleting page %d",page);
#endif
  return LFS_ERROR;

}

/*!
  **************************************************************************************************
  * @brief          Get the next unused page within a good block.
  *
  * @param         *file_handler: Pointer to file handler
  *                *tmp_table_file_handler: Pointer to table file handler
  * @return        elfs_result Function result LFS_SUCCESS/LFS_ERROR
  **************************************************************************************************/
#ifdef PROFILE_TIME_ENABLED
uint32_t do_while_loop_tick2,do_while_loop_tick1;
uint32_t spare_area_time_start,spare_area_time;
uint32_t check_bad_block_time_start,check_bad_block_time;
#endif
static elfs_result get_next_writable_page(_file_handler *file_handler,
                                          _table_file_handler *tmp_table_file_handler)  {
  uint32_t current_block;
  bool is_bad=false;
  struct _page_header nextPageHead = {0, 0};
  uint32_t firstPage;
  uint8_t init_roll_over_flag=0;
   uint8_t page_offset_enable=0;

  /* Increment Head Pointer till we get WritablePage skipping all bad blocks
     If erased, write from next block,else write from current block next page
     check tail pointer has erased complete block if so, start from next block else next page */
  uint32_t curr_head_in_blk = (tmp_table_file_handler->table_file_info.head_pointer/g_mem_prop->pages_per_block);
  uint32_t curr_tail_in_blk = tmp_table_file_handler->table_file_info.tail_pointer;
  uint32_t curr_head_in_pages = tmp_table_file_handler->table_file_info.head_pointer;
  uint32_t curr_tail_in_pages = tmp_table_file_handler->table_file_info.tail_pointer*g_mem_prop->pages_per_block;

  if((curr_head_in_blk == FILEBLOCK) && (curr_tail_in_blk == FILEBLOCK))  {
    current_block = curr_head_in_blk;
    page_offset_enable = 0;
  }
  /*!comes here immediately after the erase*/
  else if(curr_head_in_blk == curr_tail_in_blk) {
    /*! Move it to new block after the first erase*/
    if(tmp_table_file_handler->table_file_info.mem_full_flag != 1)  {
        if(curr_head_in_blk == (g_mem_prop->num_of_blocks-1)) {
          current_block = FILEBLOCK;
          init_roll_over_flag = 1;
        }
        else  {
          current_block = (curr_head_in_blk + 1);
        }
        page_offset_enable =1;
    }
    else  {
        /*! return mem-full-error*/
        return LFS_MEMORY_FULL_ERROR;
    }
  }
  /* comes here if no erase, starts writing from same block */
  else  {
    current_block = curr_head_in_blk;
    page_offset_enable=0;
  }

  if(page_offset_enable == 1){
  /* select first page from current block boundary */
  firstPage = current_block * g_mem_prop->pages_per_block;
  }
  else{
    firstPage=tmp_table_file_handler->table_file_info.head_pointer;
  }

#ifdef PROFILE_TIME_ENABLED
    do_while_loop_tick1=get_micro_sec();
#endif
  /* Iterate through bad block list to select good block,
    also go through the pages of good block to find unoccupied page,
     casused by improper closing of file */
  do {
    /* If current block is same when reached maximum blocks, roll over to
     FILEBLOCK*/
    if(current_block >= (g_mem_prop->num_of_blocks))  {
        /* initialized to file block  */
        current_block = FILEBLOCK;
        firstPage = current_block * g_mem_prop->pages_per_block;
        init_roll_over_flag = 1;
    }

#ifdef PROFILE_TIME_ENABLED
    check_bad_block_time_start = get_micro_sec();
#endif

    if(check_current_block_is_bad(current_block,tmp_table_file_handler,&is_bad) != LFS_SUCCESS) {
      NRF_LOG_INFO("Error in Checking %d Block is bad:Out of boundary conditions",current_block);
    }

#ifdef PROFILE_TIME_ENABLED
    check_bad_block_time = get_micro_sec() - check_bad_block_time_start;
#endif

    if(is_bad == true)  {
      current_block++;
      /* if bad block found, increment first page pointer to boundary of next block */
      firstPage = current_block * g_mem_prop->pages_per_block;
    }

#ifdef PROFILE_TIME_ENABLED
    spare_area_time_start = get_micro_sec();
#endif
    /* read spare are to find occupancy of page in current good block */
    if(lfs_read_oob(firstPage,&nextPageHead) != LFS_SUCCESS)  {
      NRF_LOG_INFO("get_next_writable_page:OOB Error");
      return LFS_ERROR;
    }

#ifdef PROFILE_TIME_ENABLED
    spare_area_time = (get_micro_sec() - spare_area_time_start);
#endif
    /* If occupied increment to next page */
    if(nextPageHead.occupied == 1)  {
      firstPage += 1;
    }
  }while((is_bad == true) || (nextPageHead.occupied == 1));

#ifdef PROFILE_TIME_ENABLED
   do_while_loop_tick2=get_micro_sec() - do_while_loop_tick1;
#endif
  /* Head pointer updated based on skipping bad block and occupied pages */
  tmp_table_file_handler->table_file_info.head_pointer = firstPage;

#ifdef PRINTS_OUT
   NRF_LOG_INFO("Updated head_pointer after Bad Block Check and erasal if any %d",tmp_table_file_handler->table_file_info.head_pointer);
#endif
  /* While traversing If Head reaches Max, roll it back to FILEBLOCK */
  if(init_roll_over_flag == 1)  {
    // reinitialized to 0 after roll back of Head Pointer
    tmp_table_file_handler->table_file_info.initialized_circular_buffer = 0;
    del_table_page = 1;
    memset(type,0,MAX_UPDATE_TYPE);
    type[0] = LFS_INITIALIZE_BUFFER_FLAG_UPDATE;
    /* Update in TOC for roll over flag update */
    if(update_table_file_in_toc(tmp_table_file_handler,type,1) != LFS_SUCCESS)  {
      NRF_LOG_INFO("Error in updating Table File for Circular buffer pointer");
      return LFS_UPDATE_TABLE_FILE_ERROR;
    }
    del_table_page =0;
    init_roll_over_flag = 0;
  }

  /* update variable as head pointer is updated in do while loop above */
  curr_head_in_blk = (tmp_table_file_handler->table_file_info.head_pointer/g_mem_prop->pages_per_block);

  /* If Head and Tail both are initialized to FILEBLOCK ,and roll over has happened
    memory is full then do not write */
  if(tmp_table_file_handler->table_file_info.initialized_circular_buffer != 1)  {
    if((curr_head_in_blk == FILEBLOCK) && (curr_tail_in_blk == FILEBLOCK))  {
      tmp_table_file_handler->table_file_info.mem_full_flag=1;

      memset(type,0,MAX_UPDATE_TYPE);
      type[0] = LFS_HEAD_POINTER_UPDATE;
      type[1] = LFS_MEM_FULL_FLAG_UPDATE;
       del_table_page = 1;
      /* Update Head Pointer and memory full flag update in TOC */
      if(update_table_file_in_toc(tmp_table_file_handler,type,2) != LFS_SUCCESS)  {
        NRF_LOG_INFO("Error in Updating Table file for Head Pointer");
        return LFS_UPDATE_TABLE_FILE_ERROR;
      }
       del_table_page = 0;
      return LFS_MEMORY_FULL_ERROR;
    }

    /* If Head and Tail both coincide  ,and roll over has happened
      memory is full then do not write */
    else if((curr_head_in_blk > FILEBLOCK) &&  (curr_tail_in_blk > FILEBLOCK))  {
      if(curr_head_in_blk == curr_tail_in_blk)  {
        tmp_table_file_handler->table_file_info.mem_full_flag=1;

         memset(type,0,MAX_UPDATE_TYPE);
         type[0] =  LFS_HEAD_POINTER_UPDATE;
         type[1] = LFS_MEM_FULL_FLAG_UPDATE;

         del_table_page = 1;
        /* Update Head Pointer and memory full flag update in TOC */
        if(update_table_file_in_toc(tmp_table_file_handler,type,2) != LFS_SUCCESS)  {
          NRF_LOG_INFO("Error in Updating Head Pointer");
          return LFS_UPDATE_TABLE_FILE_ERROR;
        }
         del_table_page = 0;
        return LFS_MEMORY_FULL_ERROR;
      }
    }
  }
  return LFS_SUCCESS;
}

/*!
  **************************************************************************************************
  * @brief          Get number of bad blocks
  *
  * @param         *bad_block_num: Pointer to variable which holds bad block number
  * @return        elfs_result Function result LFS_SUCCESS/LFS_ERROR
  **************************************************************************************************/
elfs_result get_bad_block(uint32_t *bad_block_num)  {
  /*  Get bad blocks number  */
  _table_file_handler table_file_handler;
  *bad_block_num  = 0;
  memset(&table_file_handler,0,sizeof(_table_file_handler));

  /* Read table page for bad block count */
  if(read_table_file_in_toc(&table_file_handler) == LFS_ERROR)  {
#ifdef PRINTS_OUT
    NRF_LOG_INFO("Error in reading Table file");
#endif
    return LFS_ERROR;
  }

  /* get no of bad blocks */
  if(get_bad_block_number(bad_block_num,FILEBLOCK,g_mem_prop->num_of_blocks,\
    &table_file_handler) != LFS_SUCCESS)  {
     return LFS_NUM_BLOCKS_OUT_OF_BOUNDS_ERROR;
  }
  return LFS_SUCCESS;
}

/*!
  **************************************************************************************************
  * @brief         delete more than one page in TOC block
  *
  * @param          page_pos_arr: Array holding page indexes to be deleted
  *                 page_num_to_del: Number of pages to delete
  * @return        elfs_result Function result LFS_SUCCESS/LFS_ERROR
  **************************************************************************************************/
static elfs_result lfs_delete_toc_pages(uint32_t page_pos_arr[],int16_t page_num_to_del)  {
  uint32_t page_init = TOCBLOCK*g_mem_prop->pages_per_block;
  eNand_Func_Result result = NAND_FUNC_SUCCESS;
  uint16_t i=0,j=0;

  /* erase the TMPBLOCK */
  result |= nand_func_erase(g_mem_prop,TMPBLOCK,1);
  if(result != NAND_FUNC_SUCCESS) {
     NRF_LOG_INFO("Error in erasing Tmp block odd");
  }

  i=0,j=0;
  uint8_t read_ecc[ sizeof(struct _page_header)];
  while(i< g_mem_prop->pages_per_block) {
    /* page to del, dont take back up */
    if((page_init+i)== page_pos_arr[j]) {
      i = i+1;
      j = j+1;
    }
    /* rest pages take back up */
    else  {
      /* read spare area to tmp from toc */
      result |= nand_func_read_ecc_zone(page_init+i,SPARE_OFFSET,\
                                        sizeof(struct _page_header), \
                                        read_ecc);
      if(result != NAND_FUNC_SUCCESS) {
        NRF_LOG_INFO("Error in reading ECC of page %d",page_init+i);
      }
      /* write spare area to tmp from toc */
      result |= nand_func_write_ecc_zone(TMPBLOCK*g_mem_prop->pages_per_block+i,SPARE_OFFSET, \
                                          sizeof(struct _page_header),(uint8_t *) read_ecc);
      if(result != NAND_FUNC_SUCCESS) {
          NRF_LOG_INFO("Error in writing ECC Zone tmp block odd");
      }
      /* page move from toc to tmp */
      result |= nand_func_page_move(page_init+i, TMPBLOCK*g_mem_prop->pages_per_block+i);
      if(result != NAND_FUNC_SUCCESS) {
          NRF_LOG_INFO("Error in page move operation");
      }

      i = i+1;
    }
  }
  /* Erase toc */
  result |= nand_func_erase(g_mem_prop,TOCBLOCK,1);
  if(result != NAND_FUNC_SUCCESS) {
    NRF_LOG_INFO("Error in erasing current block=%d",TOCBLOCK);
  }

  i=0,j=0;
  while(i < g_mem_prop->pages_per_block)  {
    /* deleted page , nothing to replace */
    if((page_init+i)== page_pos_arr[j]) {
      i = i+1;
      j = j+1;
    }
    else  {
      /* read spare area from temp */
      result |= nand_func_read_ecc_zone(TMPBLOCK*g_mem_prop->pages_per_block+i,\
                                        SPARE_OFFSET, sizeof(struct _page_header),\
                                        read_ecc);
      if(result != NAND_FUNC_SUCCESS) {
         NRF_LOG_INFO("Error in reading ECC back from tmp block odd");
      }

      /* page move from tmp to toc */
      result  |= nand_func_page_move(TMPBLOCK*g_mem_prop->pages_per_block+i, page_init+i);
      if(result != NAND_FUNC_SUCCESS) {
          NRF_LOG_INFO("Error in page move operation");
      }

      /* write spare area to toc */
      result |= nand_func_write_ecc_zone(page_init+i,SPARE_OFFSET, \
                                          sizeof(struct _page_header),\
                                          (uint8_t *) read_ecc);
      if(result != NAND_FUNC_SUCCESS) {
         NRF_LOG_INFO("Error in writing ecc zone %d",page_init+i);
      }
        i = i + 1;
    }
  }

  if(result == NAND_FUNC_SUCCESS)
    return LFS_SUCCESS;

#ifdef PRINTS_OUT
  NRF_LOG_INFO("success in deleting page %d",page);
#endif
  return LFS_ERROR;

}
#endif