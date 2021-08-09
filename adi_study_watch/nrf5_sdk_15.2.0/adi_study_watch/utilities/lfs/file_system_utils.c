/*! *****************************************************************************
 * @file:    file_system_utils.c
 * @brief:   file_system utility file.
 * @details: This contains the helper functions for the LFS
 * @version: $Revision: 1.0.1
 * @date:    $Date:16-June-2020
 -----------------------------------------------------------------------------
Copyright (c) 2020 Analog Devices, Inc.

All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
  - Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
  - Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
  - Modified versions of the software must be conspicuously marked as such.
  - This software is licensed solely and exclusively for use with processors
    manufactured by or for Analog Devices, Inc.
  - This software may not be combined or merged with other code in any manner
    that would cause the software to become subject to terms and conditions
    which differ from those listed here.
  - Neither the name of Analog Devices, Inc. nor the names of its
    contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.
  - The use of this software may or may not infringe the patent rights of one
    or more patent holders.  This license does not release you from the
    requirement that you obtain separate licenses from these patent holders
    to use this software.

THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES, INC. AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-
INFRINGEMENT, TITLE, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL ANALOG DEVICES, INC. OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, PUNITIVE OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, DAMAGES ARISING OUT OF
CLAIMS OF INTELLECTUAL PROPERTY RIGHTS INFRINGEMENT; PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

*****************************************************************************/

#ifdef USE_FS

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "file_system_utils.h"
#include "hw_if_config.h"
#include "app_cfg.h"
#include "light_fs.h"
#include "nand_cmd.h"
#include "nand_functions.h"
#include "power_manager.h"

#include "nrf_log_ctrl.h"
#define NRF_LOG_MODULE_NAME FS_UTILS

#if FS_UTILS_CONFIG_LOG_ENABLED
#ifdef NRF_LOG_DEFAULT_LEVEL
#undef NRF_LOG_DEFAULT_LEVEL
#define NRF_LOG_DEFAULT_LEVEL FS_UTILS_CONFIG_LOG_LEVEL
#endif /* NRF_LOG_DEFAULT_LEVEL */
#define NRF_LOG_INFO_COLOR  FS_UTILS_CONFIG_INFO_COLOR
#define NRF_LOG_DEBUG_COLOR FS_UTILS_CONFIG_DEBUG_COLOR
#else /* FS_UTILS_CONFIG_LOG_ENABLED */
#define NRF_LOG_LEVEL       0
#endif /* FS_UTILS_CONFIG_LOG_ENABLED */
#ifdef PROFILE_TIME_ENABLED
#include "us_tick.h"
#endif /* PROFILE_TIME_ENABLED*/
#include "nrf_log.h"
/* logger module enable */
NRF_LOG_MODULE_REGISTER();

/* Exported types ----------------------------------------------------------*/
char gs_volume_name[] = "nand:0:";

static FS_FILE_STATE_ENUM_t ge_file_wr_access = FS_FILE_ACCESS_START;
static FS_FILE_STATE_ENUM_t ge_file_read_access = FS_FILE_ACCESS_START;

static _file_handler gh_fs_file_write_handler;
static _file_handler gh_fs_file_read_handler;
static struct _memory_buffer go_lfs_buffer;
_table_file_header gh_fs_table_file_header;
vol_info_buff_t vol_info_buff_var;

_memory_properties go_fs_mem_prop = {.mem_size = DATA_FLASH_SIZE,
                               .block_size = BLOCK_SIZE,
                               .page_size = PAGE_SIZE,
                               .pages_per_block = PAGES_PER_BLOCK,
                               .num_of_blocks = NUM_OF_BLOCKS};


/* config file memory */
uint32_t volatile gn_fs_used_config_memory = 0;

static uint8_t ga_file_list[64];
static uint8_t gn_file_count;

#ifdef UNUSED_CODES
static  const  char   *fs_hal_month_name[] = {
    "jan",    "feb",    "mar",    "apr",    "may",    "jun",
    "jul",    "aug",    "sep",    "oct",    "nov",    "dec"
};
#endif

/*!
  ****************************************************************************
  *@brief       Get Volume name of file system
  *@param       None
  *@return      character pointer to file name.
******************************************************************************/
const char *fs_hal_vol_name(void) {
  return gs_volume_name;
}

/*!
  ****************************************************************************
  *@brief       Get Size of volume name size of file system
  *@param       None
  *@return      Size of vol name.
******************************************************************************/
int fs_hal_vol_name_size(void) {
  return sizeof(gs_volume_name);
}

/*!
  ****************************************************************************
  *@brief       FS HAL initialization
  *@param       None
  *@return      FS_STATUS_ENUM_t:file system status.
******************************************************************************/
FS_STATUS_ENUM_t fs_hal_init(void) {
  bool fs_version = false;
  uint32_t fs_rem_space = 0;
  elfs_result version_error;

  /* Reset Nand Flash */
  if(nand_flash_init() == NAND_NRF_DRIVER_ERROR) {
      return FS_STATUS_INIT_DRIVER_ERR;
  }
  /* Unlock Memory */
  if(nand_func_set_block_lock(NAND_FUNC_BLOCK_LOCK_ALL_UNLOCKED) != NAND_FUNC_SUCCESS) {
    return FS_STATUS_ERR;
  }
  /* Initialize File system */
  if(lfs_openfs(&go_fs_mem_prop) != LFS_SUCCESS) {
    return FS_STATUS_ERR;
  }

  version_error = lfs_check_comp_version(&fs_version);

  /* Check Version Compatibility */
  if(version_error == LFS_READ_RESERVED_INFO_ERROR);

    if(fs_version != true) {
      if(fs_hal_flash_reset() == FS_STATUS_ERR){
        return FS_STATUS_ERR;
       }
    }
    /* Read table page in structure to preserve contents */
    memset(&gh_fs_table_file_header,0,sizeof(gh_fs_table_file_header));

  if(read_table_file_in_toc(&gh_fs_table_file_header) == LFS_ERROR) {
    return FS_STATUS_ERR;
  }

  /* Get remaining space */
  if(LFS_SUCCESS != lfs_get_remaining_space(&fs_rem_space,LFS_DATA_FILE,&gh_fs_table_file_header)) {
    return FS_STATUS_ERR;
  }

  /* temporary variables back up */
  memset(&vol_info_buff_var,0,sizeof(vol_info_buff_var));
  vol_info_buff_var.tmp_head_pointer = gh_fs_table_file_header.head_pointer;
  vol_info_buff_var.tmp_tail_pointer = gh_fs_table_file_header.tail_pointer;
  vol_info_buff_var.avail_memory = fs_rem_space;
  vol_info_buff_var.used_memory = (go_fs_mem_prop.mem_size - fs_rem_space);
  uint32_t bad_block_num=0;
  if(vol_info_buff_var.tmp_head_pointer < vol_info_buff_var.tmp_tail_pointer) {
      /* Find no of Blocks which have gone bad in between */
      if(get_bad_block_number(&bad_block_num,vol_info_buff_var.tmp_head_pointer,vol_info_buff_var.tmp_tail_pointer,\
                              &gh_fs_table_file_header) != LFS_SUCCESS)  {
        return FS_STATUS_ERR;
      }
  }
  else if(vol_info_buff_var.tmp_head_pointer > vol_info_buff_var.tmp_tail_pointer )  {
      uint32_t tmp_bad_blk_num=0;
      /* Find no of blocks which have gone bad in between current head till total number of blocks */
      if(get_bad_block_number(&bad_block_num,vol_info_buff_var.tmp_head_pointer,go_fs_mem_prop.num_of_blocks,\
                              &gh_fs_table_file_header) != LFS_SUCCESS)  {
        return FS_STATUS_ERR;
      }

      tmp_bad_blk_num=0;
      /* get bad block number from File Block to Tail Pointer */
      if(get_bad_block_number(&tmp_bad_blk_num,FILEBLOCK,vol_info_buff_var.tmp_tail_pointer,\
        &gh_fs_table_file_header) != LFS_SUCCESS)  {
        return FS_STATUS_ERR;
      }

      bad_block_num += tmp_bad_blk_num;
  }

  else if(vol_info_buff_var.tmp_head_pointer == vol_info_buff_var.tmp_tail_pointer )  {
      /* Get no of Bad blocks betweeb FILEBLOCK and total blocks in flash */
     if(get_bad_block_number(&bad_block_num,FILEBLOCK,go_fs_mem_prop.num_of_blocks,\
        &gh_fs_table_file_header) != LFS_SUCCESS)  {
     return FS_STATUS_ERR;
    }
  }


    /* copy bad block num in structure */
    vol_info_buff_var.bad_block_num = bad_block_num;
    vol_info_buff_var.used_memory += (bad_block_num*go_fs_mem_prop.block_size);
    vol_info_buff_var.avail_memory -= (bad_block_num*go_fs_mem_prop.block_size);
  return FS_STATUS_OK;
}


/*!
  ****************************************************************************
  *@brief       Remount memory partition
  *@param       None
  *@return      FS_STATUS_ENUM_t:file system status
******************************************************************************/
FS_STATUS_ENUM_t fs_hal_mount(void) {
    return FS_STATUS_ERR;
}

/*!
  ****************************************************************************
  *@brief      Format the partition and initialize memory partition
  *@param      bool_t bfmt_config_blk: true - Remove both config and data files
  *                                    false - Remove only data files
  *@return     file system status.
******************************************************************************/
FS_STATUS_ENUM_t fs_hal_format(bool_t bfmt_config_blk) {
  uint32_t fs_rem_space=0;
  uint32_t bad_block_num=0;
  bool fs_version = false;

  /* Reset the memory */
  nand_flash_reset();

  /* reset strcucture */
  memset(&vol_info_buff_var,0,sizeof(vol_info_buff_var));
  if(lfs_format(bfmt_config_blk) != LFS_SUCCESS) {
    return   FS_STATUS_ERR;
  }

  /* Check Version Compatibility */
  if(lfs_check_comp_version(&fs_version) != LFS_SUCCESS) {
    return FS_STATUS_ERR;
  }

  if(fs_version != true) {
    return FS_STATUS_ERR;
  }

  /* update temp variables */
   /* Read table page in structure to preserve contents */
    memset(&gh_fs_table_file_header,0,sizeof(gh_fs_table_file_header));

  if(read_table_file_in_toc(&gh_fs_table_file_header) == LFS_ERROR) {
    return FS_STATUS_ERR;
  }

  /* Get remaining space */
  if(LFS_SUCCESS != lfs_get_remaining_space(&fs_rem_space,LFS_DATA_FILE,&gh_fs_table_file_header)) {
    return FS_STATUS_ERR;
  }

  /* temporary variables back up */
  vol_info_buff_var.tmp_head_pointer = gh_fs_table_file_header.head_pointer;
  vol_info_buff_var.tmp_tail_pointer = gh_fs_table_file_header.tail_pointer;
  vol_info_buff_var.used_memory = (go_fs_mem_prop.mem_size - fs_rem_space);
  vol_info_buff_var.avail_memory = fs_rem_space;

  /* check if after format list is updated */
  if(vol_info_buff_var.bad_block_updated != 0){
      if(get_bad_block_number(&bad_block_num,FILEBLOCK,go_fs_mem_prop.num_of_blocks,\
        &gh_fs_table_file_header) != LFS_SUCCESS)  {
      return FS_STATUS_ERR;
      }
      /* subtract mem bytes */
      vol_info_buff_var.avail_memory -= (bad_block_num*go_fs_mem_prop.block_size);
      vol_info_buff_var.used_memory += (bad_block_num*go_fs_mem_prop.block_size);
      vol_info_buff_var.bad_block_num = bad_block_num;
      vol_info_buff_var.bad_block_updated = 0;
 }
  return FS_STATUS_OK;
}

/*!
  ****************************************************************************
  *@brief      fs_block_erase
  *@param      block_no: block no to erase
  *@return     file system status.
******************************************************************************/
FS_STATUS_ENUM_t fs_block_erase(uint16_t block_no) {

  /* Reset the memory */
  nand_flash_reset();

  if((nand_func_erase(&go_fs_mem_prop,block_no,1)) != NAND_FUNC_SUCCESS) {
        NRF_LOG_INFO("Error in block erase");
       return FS_STATUS_ERR;
  }
  return FS_STATUS_OK;
}

/*!
  ****************************************************************************
  *@brief      fs_write_rsd_block
  *@param      data: data bytes to be written
               size: size of data bytes
  *@return     file system status.
******************************************************************************/
FS_STATUS_ENUM_t fs_write_rsd_block(uint32_t data[], uint16_t size) {

  /* Reset the memory */
  nand_flash_reset();

   /* write file handler information */
  if(nand_func_write(&go_fs_mem_prop,(RESERVEDBLOCK*go_fs_mem_prop.block_size),\
                              size,(uint8_t *)&data) !=  NAND_FUNC_SUCCESS) {
    NRF_LOG_WARNING("Error in writing reserved block first page");
    return FS_STATUS_ERR;
  }

  return FS_STATUS_OK;
}

/*!
  ****************************************************************************
  *@brief      Write blocks of pattern data
  *@param     pbuff: buffer with pattern data
  *@param     start_block_num: block number to start writing data
  *@param     nbuff_size: size of buffer
  *@param     first_time_write: first time write flag
  *@return     FS_STATUS_ENUM_t:file system status.
*****************************************************************************/
FS_STATUS_ENUM_t fs_hal_write_blocks(uint8_t *pbuff, uint16_t start_block_num,\
                                    uint16_t nbuff_size, uint8_t first_time_write) {

  FS_STATUS_ENUM_t fs_err_status = FS_STATUS_ERR;
  if (fs_hal_write_access_state() == FS_FILE_ACCESS_IN_PROGRESS) {
    fs_err_status = fs_hal_fixed_pattern_write_file((uint8_t *)pbuff,
                                      start_block_num,
                                      &nbuff_size, &gh_fs_file_write_handler,first_time_write);
  }
  return fs_err_status;
}

/*!
  ****************************************************************************
  *@brief      Reset the head and tail pointers and flash
  *@param      None
  *@return     FS_STATUS_ENUM_t:file system status.
******************************************************************************/
FS_STATUS_ENUM_t fs_hal_flash_reset() {
  bool fs_version = false;
  uint8_t status=0;
  uint32_t fs_rem_space=0;
  uint32_t bad_block_num=0;

  /* Reset the memory */
  nand_flash_reset();
  /* read ecc */
  if(nand_func_check_ecc(&status) != NAND_FUNC_ERROR){
    if(!status) {
      nand_func_enable_ecc(true);
    }
  }
  if(lfs_flash_reset() != LFS_SUCCESS) {
    NRF_LOG_INFO("Error in lfs_flash_reset");
    return   FS_STATUS_ERR;
  }

  /* Check Version Compatibility */
  if(lfs_check_comp_version(&fs_version) != LFS_SUCCESS) {
    return FS_STATUS_ERR;
  }

  if(fs_version != true) {
    NRF_LOG_INFO("version written wrong after reset");
    return FS_STATUS_ERR;
  }

  /* update temp variables */
   /* Read table page in structure to preserve contents */
    memset(&gh_fs_table_file_header,0,sizeof(gh_fs_table_file_header));

  if(read_table_file_in_toc(&gh_fs_table_file_header) == LFS_ERROR) {
    return FS_STATUS_ERR;
  }

  /* Get remaining space */
  if(LFS_SUCCESS != lfs_get_remaining_space(&fs_rem_space,LFS_DATA_FILE,&gh_fs_table_file_header)) {
    return FS_STATUS_ERR;
  }

  /* temporary variables back up */
  memset(&vol_info_buff_var,0,sizeof(vol_info_buff_var));
  vol_info_buff_var.tmp_head_pointer = gh_fs_table_file_header.head_pointer;
  vol_info_buff_var.tmp_tail_pointer = gh_fs_table_file_header.tail_pointer;
  vol_info_buff_var.used_memory = (go_fs_mem_prop.mem_size - fs_rem_space);
  vol_info_buff_var.avail_memory = fs_rem_space;

  /* check if after format list is updated */
  if(get_bad_block_number(&bad_block_num,FILEBLOCK,go_fs_mem_prop.num_of_blocks,\
        &gh_fs_table_file_header) != LFS_SUCCESS)  {
      return FS_STATUS_ERR;
  }
  /* subtract mem bytes */
  vol_info_buff_var.avail_memory -= (bad_block_num*go_fs_mem_prop.block_size);
  vol_info_buff_var.used_memory += (bad_block_num*go_fs_mem_prop.block_size);
  vol_info_buff_var.bad_block_num = bad_block_num;

  return FS_STATUS_OK;
}


#ifdef UNUSED_CODES
/*!
  *@brief       Internal function determines displayed string format
  *@param       p_dirent   pointer to directory entry to be listed
  *@param       p_out_buffer   pointer to buffer to get file info string
  *@param       length   length of buffer
  *@return      file system status.
 */
FS_STATUS_ENUM_t _format_file_name_display(struct  fs_dirent   *p_dirent, char *p_out_buffer,\
                                          uint32_t length, uint8_t *file_type, uint32_t *file_size) {
  char                  out_str[41];
  CLK_DATE_TIME         stime;
  CPU_BOOLEAN           ok;
  fs_time_t           ts;

  /* Chk if file is dir.            */
  if (DEF_BIT_IS_SET(p_dirent->Info.Attrib, FS_ENTRY_ATTRIB_DIR) == DEF_YES) {
    out_str[0] = 'd';
  }
  /* Chk if file is rd only.        */
  if (DEF_BIT_IS_SET(p_dirent->Info.Attrib, FS_ENTRY_ATTRIB_WR) == DEF_YES) {
    out_str[2] = 'w';
    out_str[5] = 'w';
    out_str[8] = 'w';
  }
  /* Get file size.                 */
  if (p_dirent->Info.Size == 0) {
    if (DEF_BIT_IS_CLR(p_dirent->Info.Attrib, FS_ENTRY_ATTRIB_DIR) == DEF_YES) {
      *file_type = 1;
      *file_size = 0;
      Str_Copy(&out_str[11],"          0");
    }
  } else {
    Str_FmtNbr_Int32U(p_dirent->Info.Size,
                      10, 10, '0', DEF_NO, DEF_NO, &out_str[11]);
    *file_type = 1;
    *file_size = (uint32_t)p_dirent->Info.Size;
  }
  /* Get file date/time.            */
  if (p_dirent->Info.DateTimeWr != FS_TIME_TS_INVALID) {
    ok =  Clk_TS_UnixToDateTime  (p_dirent->Info.DateTimeWr,
                                  (CLK_TZ_SEC)  0,
                                  &stime);

    if (ok == DEF_YES) {
      (void)Str_Copy(&out_str[22], (CPU_CHAR *)fs_hal_month_name[stime.Month - 1u]);
      out_str[25] = (CPU_CHAR)ASCII_CHAR_SPACE;
      (void)Str_FmtNbr_Int32U((CPU_INT32U)stime.Day,        2u, 10u, ' ', DEF_NO, DEF_NO, &out_str[26]);
      (void)Str_FmtNbr_Int32U((CPU_INT32U)stime.Yr,         4u, 10u, ' ', DEF_NO, DEF_NO, &out_str[29]);
      (void)Str_FmtNbr_Int32U((CPU_INT32U)stime.Hr,         2u, 10u, ' ', DEF_NO, DEF_NO, &out_str[34]);
      out_str[36] = (CPU_CHAR)':';
      (void)Str_FmtNbr_Int32U((CPU_INT32U)stime.Min,        2u, 10u, ' ', DEF_NO, DEF_NO, &out_str[37]);
      out_str[39] = (CPU_CHAR)' ';
      out_str[40] = (CPU_CHAR)'\0';
    }
  } else {
    Mem_Set(&out_str[22], (CPU_INT08U)' ', 17u);
    out_str[40] = (CPU_CHAR)'\0';
  }
  if (Str_Len(out_str) > length) {
    return FS_STATUS_ERR;
  }
  /* Output info for entry.         */
  snprintf(p_out_buffer,length,"%s",p_dirent->Name);
  return FS_STATUS_OK;
}
#endif

typedef enum {
  FS_DIR_ACCESS_START = 0x00,
  FS_DIR_ACCESS_IN_PROGRESS = 0x01,
} FS_DIR_STATUS_ENUM_t;

/*!
  ****************************************************************************
  *@brief       Searches for the given config file in file directory
  *@param       pfile_name: pointer to character array of config file
  *@param       nfile_nameLen: length of config file name
  *@param       bdelete_config_file: bool variable if set,
                given config file will be deleted from the LFS directory
  *@param       pconfig_file_found: pointer to the fileFound result flag
  *@return      FS_STATUS_ENUM_t:file system status.
******************************************************************************/
FS_STATUS_ENUM_t fs_hal_find_config_file( uint8_t *pfile_name, uint8_t nfile_nameLen,\
                                        bool_t bdelete_config_file, uint8_t *pconfig_file_found)
{
  uint8_t fileindex = 0;
  int  nFileCmpResult =-1;
  _file_handler tmp_file_handler;
  uint8_t status = 0;
  /* Reset the memory */
    nand_flash_reset();
  /* read ecc */
  if(nand_func_check_ecc(&status) != NAND_FUNC_ERROR){
    if(!status) {
      nand_func_enable_ecc(true);
    }
  }
  *pconfig_file_found = false;
    /* Get list of File indexes */
    if(lfs_get_file_indexes_list(ga_file_list, &gn_file_count) != LFS_SUCCESS) {
      return FS_STATUS_ERR;
    }

    while(fileindex < gn_file_count) {
      /* Open file by number */
      if(lfs_open_file_by_number(ga_file_list[fileindex], &tmp_file_handler) == LFS_SUCCESS) {
        /* Get file type */
        if(tmp_file_handler.head.file_type == LFS_CONFIG_FILE){
            nFileCmpResult = memcmp(pfile_name,tmp_file_handler.head.file_name,nfile_nameLen);
            if(!nFileCmpResult) {
             *pconfig_file_found = true;   /* Config File found */
             if(bdelete_config_file) {      /* Delete the config file */
              /* Get list of File indexes */
              if(lfs_delete_config_file(&tmp_file_handler) != LFS_SUCCESS) {
                 return FS_STATUS_ERR;
              }
            }
            break;
          }
        }
        else{
          *pconfig_file_found = false;   /* keep updating as Config file not found until its found */
        }
      } else {
        gn_file_count = 0;
        return FS_STATUS_ERR;
      }
      fileindex++;
    }
    /* if config file found and properly deleted, send start file status ok*/
    if(*pconfig_file_found == true){
      return FS_STATUS_OK;
    }
    else{
      /* send config file not found */
      return FS_STATUS_ERR_CONFIG_FILE_NOT_FOUND;
    }
}

/*!
  ****************************************************************************
  *@brief       Deletes the given config file in File directory
  *@param       pfile_name: pointer to character array of config file
  *@param       nfile_nameLen: length of config file name
  *@return      FS_STATUS_ENUM_t:file system status.
******************************************************************************/
FS_STATUS_ENUM_t fs_hal_delete_config_file( uint8_t *pfile_name, uint8_t nfile_nameLen) {
    uint8_t ncfg_file_flag = 0;
    FS_STATUS_ENUM_t fs_status;
    fs_status = fs_hal_find_config_file(pfile_name,nfile_nameLen,true, &ncfg_file_flag);
    return fs_status;
}

/*!
  ****************************************************************************
  *@brief       List files in directory
  *@param       p_dir_path: pointer to character array of directory path
  *@param       p_file_path: pointer to character array of file path
  *@param       length:ength
  *@param       file_type: pointer to file type
  *@param       file_size: pointer to file size
  *@return      FS_STATUS_ENUM_t: file system status.
*****************************************************************************/
FS_STATUS_ENUM_t fs_hal_list_dir(const char* p_dir_path, char *p_file_path, \
                                uint32_t length,uint8_t *file_type,uint32_t *file_size)
{
  static FS_DIR_STATUS_ENUM_t dir_access_status = FS_DIR_ACCESS_START;
  static uint8_t fileindex = 0;
  _file_handler tmp_file_handler;
  uint8_t status = 0;
  /* Reset the memory */
  nand_flash_reset();
  /* read ecc */
  if(nand_func_check_ecc(&status) != NAND_FUNC_ERROR) {
    if(!status) {
      nand_func_enable_ecc(true);
    }
  }
  if (dir_access_status == FS_DIR_ACCESS_START) {
    dir_access_status = FS_DIR_ACCESS_IN_PROGRESS;
    /* Get list of File indexes */
    if(lfs_get_file_indexes_list(ga_file_list, &gn_file_count) != LFS_SUCCESS) {
      dir_access_status = FS_DIR_ACCESS_START;
      return FS_STATUS_ERR;
    }
    if(gn_file_count > 0) {
      /* Open file by number */
      if(lfs_open_file_by_number(ga_file_list[fileindex], &tmp_file_handler) == LFS_SUCCESS) {
        /* Get file size */
        *file_size = tmp_file_handler.head.file_size;
        /* Get file name */
        for(int i = 0; i < 16; i++) {
          *p_file_path = tmp_file_handler.head.file_name[i];
          if(*p_file_path == '\0')
            break;
          p_file_path++;
        }
        /* Get file type */
        *file_type = tmp_file_handler.head.file_type;
      } else {
        gn_file_count = 0;
        dir_access_status = FS_DIR_ACCESS_START;
        return FS_STATUS_ERR;
      }
      fileindex++;
      return FS_STATUS_OK;
    } else {
      dir_access_status = FS_DIR_ACCESS_START;
      return FS_STATUS_ERR_EOD;
    }
  } else if (dir_access_status == FS_DIR_ACCESS_IN_PROGRESS) {
    if(fileindex < gn_file_count) {
      /* Open file by number */
      if(lfs_open_file_by_number(ga_file_list[fileindex], &tmp_file_handler) == LFS_SUCCESS) {
        /* Get file size */
        *file_size = tmp_file_handler.head.file_size;
        /* Get file name */
        for(int i = 0; i < 16; i++) {
          *p_file_path = tmp_file_handler.head.file_name[i];
          if(*p_file_path == '\0')
            break;
          p_file_path++;
        }
        /* Get file type */
        *file_type = tmp_file_handler.head.file_type;
      } else {
        fileindex = 0;
        gn_file_count = 0;
        dir_access_status = FS_DIR_ACCESS_START;
        return FS_STATUS_ERR;
      }
      fileindex++;
      return FS_STATUS_OK;
    } else if(fileindex == gn_file_count) {
      MCU_HAL_Delay(2);
      fileindex = 0;
      gn_file_count = 0;
      dir_access_status = FS_DIR_ACCESS_START;
      return FS_STATUS_ERR_EOD;
    }
  }
  return FS_STATUS_ERR;
}

/*!
  ****************************************************************************
  *@brief       Get the file information
  *@param[in]       pfile_index: pointer to file index
  *@param[out]      pfile_info: pointer to file information
  *@return      FS_STATUS_ENUM_t: file system status.
******************************************************************************/
FS_STATUS_ENUM_t fs_hal_get_file_info(uint8_t* pfile_index, file_info_t *pfile_info)
{
  _file_handler file_handler;
  if(LFS_SUCCESS != lfs_open_file_by_number(*pfile_index, &file_handler)) {
    return FS_STATUS_ERR;
  }

  for(uint8_t i=0; i < FILE_NAME_LEN; i++)
  {
    pfile_info->file_name[i] = file_handler.head.file_name[i];
    if(file_handler.head.file_name[i] == '\0') /* end of the file reached */
      break;
  }
  pfile_info->start_page = file_handler.head.mem_loc;
  pfile_info->end_page = file_handler.head.last_used_page;
  pfile_info->file_size = file_handler.head.file_size;

  return FS_STATUS_OK;
}

/*!
  ****************************************************************************
  *@brief      It does sanity test of reading the given page from flash memory
  *@param[in]       ppage_num: page number to be tested
  *                 num_bytes: number of bytes to be read; maximum we can 100 bytes
  *@param[out]      ppage_read_test_info: pointer to page read test info
  *@return      FS_STATUS_ENUM_t: file system status.
******************************************************************************/
FS_STATUS_ENUM_t fs_hal_page_read_test(uint32_t* ppage_num, m2m2_file_sys_page_read_test_resp_pkt_t *ppage_read_test_info,uint8_t num_bytes)
{
  //calculate block no baased on page no, read it proper format, memcpy and send it out
  uint32_t block_ind = *ppage_num / go_fs_mem_prop.pages_per_block;
  memset(ppage_read_test_info->sample_data,0,num_bytes);

  if(block_ind == RESERVEDBLOCK){
    _fs_info tmp_file_info;

    /* Read within the reserved block */
    if(nand_func_read(&go_fs_mem_prop,(RESERVEDBLOCK*go_fs_mem_prop.block_size+*ppage_num),\
                    num_bytes,(uint8_t*)&tmp_file_info) != NAND_FUNC_SUCCESS)  {
      NRF_LOG_INFO("Error in reading version Information");
      ppage_read_test_info->data_region_status = -1;
    }
    else{
        ppage_read_test_info->data_region_status = 0;
    }

    NRF_LOG_INFO("foot print = %s, version = %d, revision = %d",
    nrf_log_push(tmp_file_info.foot_print),
    tmp_file_info.version,
    tmp_file_info.revision);

    memcpy(ppage_read_test_info->sample_data,(uint8_t *)&tmp_file_info,num_bytes);
  }

  else if(block_ind == TOCBLOCK){
    if((TOC_BLOCK_FIRST_PAGE <= *ppage_num) && (*ppage_num < (TOC_BLOCK_LAST_PAGE(TOC_BLOCK_FIRST_PAGE,TABLE_PAGE_INDEX_IN_TOC)) )){
      _file_header fileheader;
      /* file handler type */
       if(nand_func_read(&go_fs_mem_prop,(*ppage_num) * go_fs_mem_prop.page_size,\
                    num_bytes,(uint8_t*)&fileheader) != NAND_FUNC_SUCCESS)  {
              NRF_LOG_INFO("Error in reading file handler information");
              ppage_read_test_info->data_region_status = -1;
        }
        else{
        ppage_read_test_info->data_region_status = 0;
       }
       memcpy(ppage_read_test_info->sample_data,(uint8_t *)&fileheader,num_bytes);
    }
    else{
      _table_file_header tableheader;
      /* table file handler */
       if(nand_func_read(&go_fs_mem_prop,(*ppage_num) * go_fs_mem_prop.page_size,num_bytes,\
          (uint8_t*)&tableheader) != NAND_FUNC_SUCCESS){
          ppage_read_test_info->data_region_status = -1;
          NRF_LOG_INFO("Error in reading file handler information");
       }
       else{
        ppage_read_test_info->data_region_status = 0;
       }
          ppage_read_test_info->sample_data[0] = (uint8_t )tableheader.tail_pointer;
          ppage_read_test_info->sample_data[1] = (uint8_t )tableheader.head_pointer;
          ppage_read_test_info->sample_data[2] = (uint8_t )tableheader.initialized_circular_buffer;
          ppage_read_test_info->sample_data[3] = (uint8_t )tableheader.mem_full_flag;
          ppage_read_test_info->sample_data[4] = (uint8_t )tableheader.offset;
          ppage_read_test_info->sample_data[5] = (uint8_t )tableheader.config_low_touch_occupied;
      }
    }
  else{
    ppage_read_test_info->data_region_status = read_page_data(*ppage_num, (uint8_t *)&ppage_read_test_info->sample_data, num_bytes);  
  }
  for(int i=0;i < num_bytes;i++){
      NRF_LOG_INFO("%d",ppage_read_test_info->sample_data[i]);
    }

  ppage_read_test_info->ecc_zone_status = read_page_ecc_zone(*ppage_num, &ppage_read_test_info->next_page, &ppage_read_test_info->occupied);
  return (ppage_read_test_info->data_region_status | ppage_read_test_info->ecc_zone_status);
}


/*!
  ****************************************************************************
  *@brief       Get Size of file or directory
  *@param       p_path: pointer to character array of file path
  *@param       p_size: pointer to output file size
  *@return      FS_STATUS_ENUM_t: file system status.
******************************************************************************/
FS_STATUS_ENUM_t fs_hal_get_size(char* p_path, uint32_t *p_size)
{
  _file_handler file_handler;
  if(LFS_SUCCESS != lfs_open_file_by_name(p_path, &file_handler)) {
    return FS_STATUS_ERR;
  }
  /* Read File size info */
  *p_size = file_handler.head.file_size;
  return FS_STATUS_OK;
}

/*!
  ****************************************************************************
  *@brief       Get FS Volume info
  *@param       p_size   pointer to output FS volume info
  *@return      file system status.
*****************************************************************************/
FS_STATUS_ENUM_t fs_hal_get_vol_info(uint32_t *p_size) {

  /* usage of temp variables for vol info*/
  p_size[0] = DATA_FLASH_SIZE;
  p_size[1] = vol_info_buff_var.used_memory;
  p_size[2] = (((vol_info_buff_var.avail_memory/1024)*100)/(p_size[0]/1024));;
  NRF_LOG_INFO("Total memory=%d",p_size[0]);
  NRF_LOG_INFO("Used memory=%d",p_size[1]);
  NRF_LOG_INFO("Free memory=%d",p_size[2]);
  return FS_STATUS_OK;
}

/*!
  ****************************************************************************
  *@brief       read contents of file
  *@param       p_file_path: pointer to character array of file path
  *@param       p_buffer: pointer to buffer
  *@param       p_length: pointer to length
  *@return      FS_STATUS_ENUM_t: file system status.
*****************************************************************************/
#ifdef PROFILE_TIME_ENABLED
  uint32_t min_file_read_time,avg_file_read_time,max_file_read_time;
  uint32_t bytes_read_start_time,bytes_read_end_time;
  uint16_t num_times_read;
#endif
/* Start node to be scanned (also used as work area) */
FS_STATUS_ENUM_t fs_hal_read_file(char* p_file_path, uint8_t *p_buffer, uint32_t *p_length) {
  static uint32_t nfile_size = 0;
  static uint32_t read_size = 0;
  static uint32_t bytes_read = 0;
  if (ge_file_read_access == FS_FILE_ACCESS_START) {
    if (p_file_path == NULL) {
      return FS_STATUS_ERR;
    }
    /* Open file for reading */
    if(LFS_SUCCESS == lfs_open_file_by_name(p_file_path, &gh_fs_file_read_handler)) {
      /* Get File size */
      nfile_size = gh_fs_file_read_handler.head.file_size;
      /* Check for file length */
      if(*p_length < nfile_size) {
        read_size = *p_length;
      } else {
        read_size = nfile_size;
      }

//t1
#if PROFILE_TIME_ENABLED
    bytes_read_start_time = get_micro_sec();
#endif
      /* Read Data from file */
      if(lfs_read_file(p_buffer, bytes_read, read_size, &gh_fs_file_read_handler) != LFS_SUCCESS) {
        ge_file_read_access = FS_FILE_ACCESS_START;
        bytes_read = 0;
        return FS_STATUS_ERR_STDIO;
      }
//t2
#if PROFILE_TIME_ENABLED
    bytes_read_end_time = get_micro_sec();
    avg_file_read_time += (bytes_read_end_time - bytes_read_start_time);
    if((bytes_read_end_time - bytes_read_start_time) < min_file_read_time )
    {
      min_file_read_time = (bytes_read_end_time - bytes_read_start_time);
    }
    if((bytes_read_end_time - bytes_read_start_time) > max_file_read_time)
    {
      max_file_read_time = (bytes_read_end_time - bytes_read_start_time);
    }
    num_times_read++;
#endif
      bytes_read += read_size;
      if (bytes_read == nfile_size) {
        *p_length = read_size;
        ge_file_read_access = FS_FILE_ACCESS_START;
        bytes_read = 0;
        return FS_STATUS_ERR_EOF;
      }
      *p_length = read_size;
      ge_file_read_access = FS_FILE_ACCESS_IN_PROGRESS;
      return FS_STATUS_OK;
    } else {      /* ... dir does not exist.        */
      ge_file_read_access = FS_FILE_ACCESS_START;
      bytes_read = 0;
      return FS_STATUS_ERR_INVALID_DIR;
    }
  } else if (ge_file_read_access == FS_FILE_ACCESS_IN_PROGRESS) {

      /* Check for file length */
      if((*p_length + bytes_read) < nfile_size) {
        read_size = *p_length;
      } else {
        read_size = (nfile_size - bytes_read);
      }

      /* Read Data from file */
      if(lfs_read_file(p_buffer, bytes_read, read_size, &gh_fs_file_read_handler) != LFS_SUCCESS) {
        ge_file_read_access = FS_FILE_ACCESS_START;
        bytes_read = 0;
        return FS_STATUS_ERR_STDIO;
      }

      bytes_read += read_size;

      if (bytes_read == nfile_size) {
        *p_length = read_size;
        ge_file_read_access = FS_FILE_ACCESS_START;
        bytes_read = 0;
        return FS_STATUS_ERR_EOF;
      }
      *p_length = read_size;
      ge_file_read_access = FS_FILE_ACCESS_IN_PROGRESS;
      return FS_STATUS_OK;
  }
  return FS_STATUS_ERR;
}

/*!
  ****************************************************************************
  *@brief       read contents of file with page offset
  *@param       p_file_path: pointer to character array of directory path
  *@param       p_buffer: pointer to character array to get the content of file
  *@param       p_length: number bytes to be read
  *@param       p_filesize: pointer to integer to get file size in bytes
  *@param       offset: specify the page-offset to move the file pointer to that particular
                              byte position
  *@return      FS_STATUS_ENUM_t:file system status.
*****************************************************************************/
FS_STATUS_ENUM_t fs_hal_read_pageoffset(char* p_file_path, uint8_t *p_buffer, \
                                        uint32_t *p_length,  uint32_t *p_filesize,\
                                        uint32_t offset) {
  uint32_t nfilesize = 0;
  uint32_t read_size = 0;
  uint32_t bytes_read = 0;
  uint32_t maximum_offset = 0;

  if (ge_file_read_access == FS_FILE_ACCESS_START) {
    if (p_file_path == NULL) {
      return FS_STATUS_ERR;
    }

    /* Open file for reading */
    if(LFS_SUCCESS == lfs_open_file_by_name(p_file_path, &gh_fs_file_read_handler)) {
      /* Get File size */
      nfilesize = gh_fs_file_read_handler.head.file_size;
      *p_filesize = nfilesize;
      /* Check for file length */
      if((*p_length + offset) < nfilesize) {
        read_size = *p_length;
      } else {
        read_size = (nfilesize - offset);
      }
      bytes_read += offset;
      gh_fs_file_read_handler.current_offset = offset;
      /* moving the file poniter read position with offset value */
      gh_fs_file_read_handler.current_read_pos += offset;
      maximum_offset = gh_fs_file_read_handler.head.last_used_page * go_fs_mem_prop.page_size;
      /* return error if user has given offset maximum than file size */
      if (gh_fs_file_read_handler.current_read_pos > maximum_offset){
        return FS_STATUS_ERR;
      }
      /* Read Data from file */
      if(lfs_read_file(p_buffer, bytes_read, read_size, &gh_fs_file_read_handler) != LFS_SUCCESS) {
        bytes_read = 0;
        return FS_STATUS_ERR_STDIO;
      }
      bytes_read += read_size;
      if (bytes_read == nfilesize) {
        *p_length = read_size;
        bytes_read = 0;
        return FS_STATUS_ERR_EOF;
      }
      *p_length = read_size;
      return FS_STATUS_OK;
    } else {     /* ... dir does not exist.        */
      bytes_read = 0;
      return FS_STATUS_ERR_INVALID_DIR;
    }
  }
  return FS_STATUS_ERR;
}

/*!
  ****************************************************************************
  *@brief       open file
  *@param       p_filer_path:   pointer to character array of file path
  *@return      FS_STATUS_ENUM_t:file system status.
*****************************************************************************/
FS_STATUS_ENUM_t fs_hal_open_file(char* p_file_path)
{
  uint32_t fs_rem_space = 0;
  elfs_result lfs_status;
  uint8_t status = 0;
  bool mem_full = false;

   /* Reset the memory */
  nand_flash_reset();
  /* read ecc */
  if(nand_func_check_ecc(&status) != NAND_FUNC_ERROR){
    if(!status){
      nand_func_enable_ecc(true);
    }
  }

  /* Check for memory full */
  if(get_memory_status(&mem_full) != LFS_SUCCESS) {
    return FS_STATUS_ERR;
  }
  else  {
    /* check for memory full flag is set */
    if(mem_full == true){
      return FS_STATUS_ERR_MEMORY_FULL;
    }
  }

  /* File object */
  if (p_file_path == NULL) {
    return FS_STATUS_ERR;
  }

  NRF_LOG_INFO("********** Creating new file ****************");
  /* Create file */
  lfs_status = lfs_create_file((uint8_t *)p_file_path, &gh_fs_file_write_handler, \
                              &gh_fs_table_file_header,&go_lfs_buffer, LFS_DATA_FILE);
  if(lfs_status != LFS_SUCCESS) {
    if(lfs_status == LFS_MAX_FILE_COUNT_ERROR) {
      return FS_STATUS_ERR_MAX_FILE_COUNT;
    }
    else if(lfs_status == LFS_CONFIG_FILE_POSITION_ERROR) {
      return FS_STATUS_ERR_CONFIG_FILE_POSITION;
    } else {
      return FS_STATUS_ERR;
    }
  }
  /* Set operating mode to Manual */
  lfs_set_operating_mode(&gh_fs_file_write_handler, LFS_MODE_MANUAL);
  /* Open file for writing */
  ge_file_wr_access = FS_FILE_ACCESS_IN_PROGRESS;
  return FS_STATUS_OK;
}

/*!
  ****************************************************************************
  *@brief       open config file
  *@param       p_file_path: pointer to character array of file path
  *@return      FS_STATUS_ENUM_t:file system status.
*****************************************************************************/
FS_STATUS_ENUM_t fs_hal_open_config_file(uint8_t* p_file_path) {
  uint8_t status = 0;
  /* File object */
  if (p_file_path == NULL) {
    return FS_STATUS_ERR;
  }
  /* Reset the memory */
  nand_flash_reset();
  /* read ecc */
  if(nand_func_check_ecc(&status) != NAND_FUNC_ERROR){
    if(!status){
      nand_func_enable_ecc(true);
    }
  }
  /* Create file */
  if(LFS_SUCCESS != lfs_create_file(p_file_path, &gh_fs_file_write_handler,\
                                  &gh_fs_table_file_header,&go_lfs_buffer,\
                                   LFS_CONFIG_FILE)) {
    return FS_STATUS_ERR;
  }
  lfs_set_operating_mode(&gh_fs_file_write_handler, LFS_MODE_FAST);
  ge_file_wr_access = FS_FILE_ACCESS_IN_PROGRESS;
  return FS_STATUS_OK;
}

/*!
  ****************************************************************************
  *@brief       Close file
  *@param       file_handler: Handler to file
  *@return      FS_STATUS_ENUM_t: file system status.
*****************************************************************************/
FS_STATUS_ENUM_t fs_hal_close_file(_file_handler *file_handler) {
  uint32_t fs_rem_space = 0;
  elfs_result end_res;
  if(file_handler != NULL) {
  end_res = lfs_end_file(file_handler,&gh_fs_table_file_header);
    if (end_res == LFS_SUCCESS) {
      ge_file_wr_access = FS_FILE_ACCESS_START;
      memset(file_handler, 0, sizeof(_file_handler));
    } else {
      return FS_STATUS_ERR;
    }
  }
  /* Get remaining space */
  if(LFS_SUCCESS != lfs_get_remaining_space(&fs_rem_space,LFS_DATA_FILE,&gh_fs_table_file_header)) {
    return FS_STATUS_ERR;
  }

  /* update temp varaibles */
  /* temporary variables back up */
  vol_info_buff_var.tmp_head_pointer = gh_fs_table_file_header.head_pointer;
  vol_info_buff_var.tmp_tail_pointer = gh_fs_table_file_header.tail_pointer;
  vol_info_buff_var.avail_memory = fs_rem_space;
  vol_info_buff_var.used_memory = (go_fs_mem_prop.mem_size - fs_rem_space);

   /* Subtract area corresponding to Bad blocks */
  vol_info_buff_var.used_memory += (vol_info_buff_var.bad_block_num*go_fs_mem_prop.block_size);
  vol_info_buff_var.avail_memory -= (vol_info_buff_var.bad_block_num*go_fs_mem_prop.block_size);

  return FS_STATUS_OK;
}

/*!
  ****************************************************************************
  *@brief       Get file count
  *@param       p_file_count: pointer in which the number of files
                present is returned
  *@return      FS_STATUS_ENUM_t: file system status.
*****************************************************************************/
FS_STATUS_ENUM_t fs_hal_get_file_count(uint8_t *p_file_count) {
    /* Reset the memory */
    nand_flash_reset();

    // data offset variable
    if(lfs_get_file_count(&gn_file_count) != LFS_ERROR){
      *p_file_count = gn_file_count;
    }
    else{
      *p_file_count=0;
      return FS_STATUS_ERR;
    }

  return FS_STATUS_OK;
}

/*!
  ****************************************************************************
  *@brief       write contents of buffer to config file
  *@param       p_buffer: pointer to buffer
  *@param       n_items: Number of items to write to file
  *@param       file_handler: handlr to file
  *@return      file system status.
******************************************************************************/
FS_STATUS_ENUM_t fs_hal_write_config_file(uint8_t *p_buffer,
                                          uint32_t *nitems,
                                          _file_handler *file_handler) {
  elfs_result      FS_Error;
  static uint32_t nBytes_written = 0;
  if (ge_file_wr_access != FS_FILE_ACCESS_IN_PROGRESS){
    return FS_STATUS_ERR;
  }
  /* Write data to File */
  FS_Error = lfs_update_config_file(p_buffer, *nitems, file_handler);
  /* Update Used memory */
  gn_fs_used_config_memory += *nitems;
  /* Updated bytes written to Flash */
  nBytes_written += *nitems;
  // TO DO
  if(nBytes_written >= 16384) { /* greater than 4 pages */
    nBytes_written = 0;
    /* Refresh Handler */
    if(file_handler->op_mode == LFS_MODE_MANUAL) {
      lfs_refresh_config_header(file_handler);
    }
  }
  // TO DO
  /* Check for memory full */
  /* block of data =  2044 * 64 * 4096 */
  if(gn_fs_used_config_memory >= BLOCK_SIZE) {
    return FS_STATUS_ERR_MEMORY_FULL;
  }
  if (FS_Error != LFS_SUCCESS) {
    if (lfs_end_config_file(file_handler) == LFS_SUCCESS) {
      ge_file_wr_access = FS_FILE_ACCESS_START;
        return FS_STATUS_ERR_EOF;
    } else {
        return FS_STATUS_ERR;
    }
  }
  return FS_STATUS_OK;
}

/*!
  ****************************************************************************
  *@brief       write contents of buffer to file
  *@param       p_buffer: pointer to buffer of data
  *@param       n_items: number of data items
  *@param       file_handler: handler to file
  *@return      FS_STATUS_ENUM_t: file system status.
*****************************************************************************/
FS_STATUS_ENUM_t fs_hal_write_file(uint8_t *p_buffer,
                                   uint32_t *nitems,
                                   _file_handler *file_handler) {
  elfs_result FS_Error;
  static uint32_t nBytes_written = 0;
  uint32_t fs_rem_space = 0;
  uint32_t bad_block_num = 0;

  if (ge_file_wr_access != FS_FILE_ACCESS_IN_PROGRESS) {
    return FS_STATUS_ERR;
  }
#ifdef PROFILE_TIME_ENABLED
uint32_t nTick = 0;
nTick = MCU_HAL_GetTick();
#endif

  /* Write data to File */
   FS_Error = lfs_update_file(p_buffer, *nitems, file_handler,&gh_fs_table_file_header);

#ifdef PROFILE_TIME_ENABLED
     uint32_t pkt_update_time = MCU_HAL_GetTick() - nTick;
   //  NRF_LOG_INFO("Time taken for lfs update file = %d",pkt_update_time);
#endif

  /* Updated bytes written to Flash */
  nBytes_written += *nitems;

  if(nBytes_written >= BLOCK_SIZE) { /* updating header for block */
    nBytes_written = 0;
    /* Refresh Handler */
    if(file_handler->op_mode == LFS_MODE_MANUAL) {
      lfs_refresh_header(file_handler,&gh_fs_table_file_header);
    }

     /* estimate for every block */
    if(LFS_SUCCESS != lfs_get_remaining_space(&fs_rem_space,LFS_DATA_FILE,&gh_fs_table_file_header)) {
      return FS_STATUS_ERR;
    }

   /* update temp variables while writing */
   vol_info_buff_var.tmp_head_pointer = gh_fs_table_file_header.head_pointer;
   vol_info_buff_var.tmp_tail_pointer = gh_fs_table_file_header.tail_pointer;
   vol_info_buff_var.avail_memory = fs_rem_space;
   vol_info_buff_var.used_memory = (go_fs_mem_prop.mem_size - fs_rem_space);

   if(vol_info_buff_var.bad_block_updated != 0){
        /* subtract mem bytes 1 block of data*/
        vol_info_buff_var.used_memory += go_fs_mem_prop.block_size;
        vol_info_buff_var.avail_memory -= go_fs_mem_prop.block_size;
        vol_info_buff_var.bad_block_num += 1;
        vol_info_buff_var.bad_block_updated=0;
     }
  }
  
  /* process memory full error, here file is already closed with  file updation  */
  if(FS_Error == LFS_MEMORY_FULL_ERROR){
    
    /* estimate for every block */
    if(LFS_SUCCESS != lfs_get_remaining_space(&fs_rem_space,LFS_DATA_FILE,&gh_fs_table_file_header)) {
      return FS_STATUS_ERR;
    }

    /* update temp variables while writing */
    vol_info_buff_var.tmp_head_pointer = gh_fs_table_file_header.head_pointer;
    vol_info_buff_var.tmp_tail_pointer = gh_fs_table_file_header.tail_pointer;
    vol_info_buff_var.avail_memory = fs_rem_space;
    vol_info_buff_var.used_memory = (go_fs_mem_prop.mem_size - fs_rem_space);

    if(vol_info_buff_var.bad_block_updated != 0){
      /* subtract mem bytes 1 block of data*/
      vol_info_buff_var.used_memory += go_fs_mem_prop.block_size;
      vol_info_buff_var.avail_memory -= go_fs_mem_prop.block_size;
      vol_info_buff_var.bad_block_num += 1;
      vol_info_buff_var.bad_block_updated=0;
    }
    return FS_STATUS_ERR_MEMORY_FULL;
  }
 
  /* close file if error, except memory full as file is already closed inside */
  if (FS_Error != LFS_SUCCESS) {
    /* file closed as cannot be recovered */
    if(FS_Error == LFS_FILE_WRITE_ERROR)
      /* update in file header as invalid */
      file_handler->head.file_type = LFS_INVALID_FILE;
    if (lfs_end_file(file_handler,&gh_fs_table_file_header) == LFS_SUCCESS) {
      ge_file_wr_access = FS_FILE_ACCESS_START;
      if(FS_Error == LFS_FILE_WRITE_ERROR)
        return FS_STATUS_ERR_INVALID_FILE;
      else
        return FS_STATUS_ERR_EOF;
    } else {
       if(FS_Error == LFS_FILE_WRITE_ERROR)
        return FS_STATUS_ERR_INVALID_FILE;
      else
        return FS_STATUS_ERR;
    }
  }

  return FS_STATUS_OK;
}

/*!
  ****************************************************************************
  *@brief       write contents of pattern data buffer to file
  *@param       p_buffer: pointer to pattern buffer
  *@param       start_block_num: start block where data to be written
  *@param       nitems: number fo items to write
  *@param       file_handler: handlrer to file
  *@param       first_time_write: first time write flag
  *@return      FS_STATUS_ENUM_t: file system status.
*****************************************************************************/
FS_STATUS_ENUM_t fs_hal_fixed_pattern_write_file(uint8_t *p_buffer,uint16_t start_block_num,\
                                                uint16_t *nitems, _file_handler *file_handler,\
                                                uint8_t first_time_write) {
  elfs_result      FS_Error;
  static uint32_t nBytes_written = 0;
  uint32_t fs_rem_space = 0;

  if (ge_file_wr_access != FS_FILE_ACCESS_IN_PROGRESS){
    return FS_STATUS_ERR;
  }

 
  /* Write data to File */
  FS_Error = lfs_update_pattern_file(p_buffer, start_block_num,*nitems,file_handler,\
                                    &gh_fs_table_file_header,first_time_write);

  /* Updated bytes written to Flash */
  nBytes_written += *nitems;

  /* update toc & fs vol info variables for block size */
  if(nBytes_written >= TOC_REPEATED_UPDATE_SIZE) { 
    nBytes_written = 0;
    /* Refresh Handler */
    if(file_handler->op_mode == LFS_MODE_MANUAL) {
      lfs_refresh_header(file_handler,&gh_fs_table_file_header);
    }

     /* estimate for every block */
    if(LFS_SUCCESS != lfs_get_remaining_space(&fs_rem_space,LFS_DATA_FILE,&gh_fs_table_file_header)) {
      return FS_STATUS_ERR;
    }

   /* update temp variables while writing */
   vol_info_buff_var.tmp_head_pointer = gh_fs_table_file_header.head_pointer;
   vol_info_buff_var.tmp_tail_pointer = gh_fs_table_file_header.tail_pointer;
   vol_info_buff_var.avail_memory = fs_rem_space;
   vol_info_buff_var.used_memory = (go_fs_mem_prop.mem_size - fs_rem_space);

   if(vol_info_buff_var.bad_block_updated != 0){
        /* subtract mem bytes 1 block of data*/
        vol_info_buff_var.used_memory += go_fs_mem_prop.block_size;
        vol_info_buff_var.avail_memory -= go_fs_mem_prop.block_size;
        vol_info_buff_var.bad_block_num += 1;
        vol_info_buff_var.bad_block_updated=0;
     }
  }
  
   /* process memory full error, here file is already closed with  file updation  */
  if(FS_Error == LFS_MEMORY_FULL_ERROR){
    /* update temporary variables */
    /* estimate for every block */
    if(LFS_SUCCESS != lfs_get_remaining_space(&fs_rem_space,LFS_DATA_FILE,&gh_fs_table_file_header)) {
      return FS_STATUS_ERR;
    }

   /* update temp variables while writing */
   vol_info_buff_var.tmp_head_pointer = gh_fs_table_file_header.head_pointer;
   vol_info_buff_var.tmp_tail_pointer = gh_fs_table_file_header.tail_pointer;
   vol_info_buff_var.avail_memory = fs_rem_space;
   vol_info_buff_var.used_memory = (go_fs_mem_prop.mem_size - fs_rem_space);

   if(vol_info_buff_var.bad_block_updated != 0){
        /* subtract mem bytes 1 block of data*/
        vol_info_buff_var.used_memory += go_fs_mem_prop.block_size;
        vol_info_buff_var.avail_memory -= go_fs_mem_prop.block_size;
        vol_info_buff_var.bad_block_num += 1;
        vol_info_buff_var.bad_block_updated=0;
     }
    return FS_STATUS_ERR_MEMORY_FULL;
  }
  
  /* close file if error, except memory full as file is already closed inside */
  if (FS_Error != LFS_SUCCESS) {
    /* file closed as cannot be recovered */
    if(FS_Error == LFS_FILE_WRITE_ERROR)
    /* update in file header as invalid */
      file_handler->head.file_type = LFS_INVALID_FILE;
    if (lfs_end_file(file_handler,&gh_fs_table_file_header) == LFS_SUCCESS) {
      ge_file_wr_access = FS_FILE_ACCESS_START;
      if(FS_Error == LFS_FILE_WRITE_ERROR)
        return FS_STATUS_ERR_INVALID_FILE;
      else
        return FS_STATUS_ERR_EOF;
    } else {
       if(FS_Error == LFS_FILE_WRITE_ERROR)
        return FS_STATUS_ERR_INVALID_FILE;
      else
        return FS_STATUS_ERR;
    }
  }
  return FS_STATUS_OK;
}

/*!
  ****************************************************************************
  *@brief       get file write access state
  *@param       None
  *@return      FS_STATUS_ENUM_t: file system status.
*****************************************************************************/
FS_FILE_STATE_ENUM_t fs_hal_write_access_state(void) {
  return ge_file_wr_access;
}

/*!
  ****************************************************************************
  *@brief       get file write pointer
  *@param       None
  *@return      _file_handler: file handler
*****************************************************************************/
_file_handler *fs_hal_write_file_pointer(void) {
  return &gh_fs_file_write_handler;
}

/*!
  ****************************************************************************
  *@brief       get file read pointer
  *@param       None
  *@return      _file_handler: file handler
*****************************************************************************/
_file_handler *fs_hal_get_read_file_pointer(void) {
  return &gh_fs_file_read_handler;
}

/*!
  ****************************************************************************
  *@brief       write m2m packet structure to file
  *@param       pkt: m2m2_hdr_t packet structure
  *@return      M2M2_APP_COMMON_STATUS_ENUM_t: Status of file write
*****************************************************************************/
M2M2_APP_COMMON_STATUS_ENUM_t fs_hal_write_packet_stream(m2m2_hdr_t *pkt)
{
  uint32_t buffersize;
  FS_STATUS_ENUM_t fs_err_status;
  if (fs_hal_write_access_state() == FS_FILE_ACCESS_IN_PROGRESS) {
    buffersize = pkt->length;
    /* swap from network byte order to little endian */
    pkt->src = (M2M2_ADDR_ENUM_t)BYTE_SWAP_16(pkt->src);
    pkt->dest = (M2M2_ADDR_ENUM_t)BYTE_SWAP_16(pkt->dest);
    pkt->length = BYTE_SWAP_16(pkt->length);
    pkt->checksum = BYTE_SWAP_16(pkt->checksum);
#ifdef PROFILE_TIME_ENABLED
uint32_t nTick = 0;
nTick = MCU_HAL_GetTick();
#endif
    fs_err_status = fs_hal_write_file((uint8_t *)pkt,
                                      &buffersize, &gh_fs_file_write_handler);

#ifdef PROFILE_TIME_ENABLED
          uint32_t hal_pkt_write_time = MCU_HAL_GetTick() - nTick;
         // NRF_LOG_INFO("Time taken for packet write time = %d",hal_pkt_write_time);
#endif
    /* swap back from network byte order to little endian */
    /* TODO: Check if length swap back is sufficient */
    pkt->src = (M2M2_ADDR_ENUM_t)BYTE_SWAP_16(pkt->src);
    pkt->dest = (M2M2_ADDR_ENUM_t)BYTE_SWAP_16(pkt->dest);
    pkt->length = BYTE_SWAP_16(pkt->length);
    pkt->checksum = BYTE_SWAP_16(pkt->checksum);
    if (fs_err_status != FS_STATUS_OK) {
      if (fs_err_status == FS_STATUS_ERR_EOF){
        return M2M2_APP_COMMON_STATUS_STREAM_STOPPED;
      } 
      else if (fs_err_status == FS_STATUS_ERR_MEMORY_FULL){
        /* reset flag write access */
        ge_file_wr_access = FS_FILE_ACCESS_START;
        return (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_FILE_SYS_ERR_MEMORY_FULL;
      }
      else if(fs_err_status == FS_STATUS_ERR_INVALID_FILE) {
        return (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_FILE_SYS_ERR_INVALID;
      }
      else{
        return  M2M2_APP_COMMON_STATUS_ERROR;
      }
    } 
    else {
      return  M2M2_APP_COMMON_STATUS_OK;
    }
  } 
  else {
    return M2M2_APP_COMMON_STATUS_STREAM_NOT_STARTED;
  }
}

/*!
  ****************************************************************************
  *@brief       write test pattern to file
  *@param       pbuff: pointer to the test pattern array
  *@param       nbuff_size: size of the test pattern array
  *@return      M2M2_APP_COMMON_STATUS_ENUM_t: file system status.
*****************************************************************************/
M2M2_APP_COMMON_STATUS_ENUM_t fs_hal_test_pattern_write(uint8_t* pbuff,
                                                        uint32_t nbuff_size) {
  FS_STATUS_ENUM_t fs_err_status;
	if (fs_hal_write_access_state() == FS_FILE_ACCESS_IN_PROGRESS) {
          fs_err_status = fs_hal_write_file((uint8_t *)pbuff,
                                      &nbuff_size, &gh_fs_file_write_handler);
          if (fs_err_status != FS_STATUS_OK) {
            if (fs_err_status == FS_STATUS_ERR_EOF) {
		return M2M2_APP_COMMON_STATUS_STREAM_STOPPED;
            } 
            else if (fs_err_status == FS_STATUS_ERR_MEMORY_FULL)  {
              /* reset flag write access */
              ge_file_wr_access = FS_FILE_ACCESS_START;
              return (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_FILE_SYS_ERR_MEMORY_FULL;
            }
            else  {
              return  M2M2_APP_COMMON_STATUS_ERROR;
            }
          } else {
            return  M2M2_APP_COMMON_STATUS_OK;
          }
	}
  return  M2M2_APP_COMMON_STATUS_ERROR;
}

/*!
  ****************************************************************************
  *@brief       write test pattern to config file
  *@param       pbuff: pointer to the test pattern array
  *@param       nbuff_size: size of the test pattern array
  *@return      M2M2_APP_COMMON_STATUS_ENUM_t: file system status.
******************************************************************************/
M2M2_APP_COMMON_STATUS_ENUM_t fs_hal_test_pattern_config_write(uint8_t* pbuff,
                                                               uint32_t nbuff_size) {
  FS_STATUS_ENUM_t fs_err_status;
  if (fs_hal_write_access_state() == FS_FILE_ACCESS_IN_PROGRESS) {

    fs_err_status = fs_hal_write_config_file((uint8_t *)pbuff,
                                      &nbuff_size, &gh_fs_file_write_handler);
    if (fs_err_status != FS_STATUS_OK) {
      if (fs_err_status == FS_STATUS_ERR_EOF) {
        return M2M2_APP_COMMON_STATUS_STREAM_STOPPED;
      } else if (fs_err_status == FS_STATUS_ERR_MEMORY_FULL) {
        return (M2M2_APP_COMMON_STATUS_ENUM_t)M2M2_FILE_SYS_ERR_MEMORY_FULL;
      } else {
        return  M2M2_APP_COMMON_STATUS_ERROR;
      }
    } else {
      return  M2M2_APP_COMMON_STATUS_OK;
    }
  } else {
    return M2M2_APP_COMMON_STATUS_STREAM_NOT_STARTED;
  }
}

/*!
  ****************************************************************************
  *@brief       make response packet for fs write error
  *@param       src: m2m2_hdr_t packet structure, source address
  *@return      None
*****************************************************************************/
void fs_hal_write_error_resp(m2m2_hdr_t *response_pkt,M2M2_ADDR_ENUM_t *src) {
  _m2m2_app_common_cmd_t *stream_resp = (_m2m2_app_common_cmd_t *)&response_pkt->data[0];
  stream_resp->command = M2M2_APP_COMMON_CMD_STREAM_START_RESP;
  stream_resp->status = M2M2_APP_COMMON_STATUS_ERROR;
  response_pkt->src = M2M2_ADDR_SYS_FS;
  response_pkt->dest = *src;
  *src = M2M2_ADDR_UNDEFINED;
}

/*!
  ****************************************************************************
  *@brief       Unblock flash
  *@param       None
  *@return      FS_STATUS_ENUM_t: file system status
*****************************************************************************/
FS_STATUS_ENUM_t fs_hal_unblock_flash(void) {
  if(nand_func_set_block_lock(NAND_FUNC_BLOCK_LOCK_ALL_UNLOCKED) != NAND_FUNC_SUCCESS) {
    return FS_STATUS_ERR;
  }
  return FS_STATUS_OK;
}

/*!
  ****************************************************************************
  *@brief       Check for availability of file
  *@param       file_name: name of file
  *@return      FS_STATUS_ENUM_t: file system status.
*****************************************************************************/
FS_STATUS_ENUM_t fs_hal_find_file(char* file_name) {
  _file_handler tmp_file_handler;
  /* Get list of File indexes */
  if(lfs_get_file_indexes_list(ga_file_list, &gn_file_count) != LFS_SUCCESS){
    return FS_STATUS_ERR;
  }
  /* compare for file name */
  if(gn_file_count > 0){
    for(int i = 0; i < gn_file_count; i++) {
      /* Open file by number */
      if(lfs_open_file_by_number(ga_file_list[i], &tmp_file_handler) == LFS_SUCCESS){
        /* Compare file names */
        if(strcmp(file_name, (char *)&tmp_file_handler.head.file_name[0]) == 0) {
          return FS_STATUS_OK;
        }
      }
    }
  }
  return FS_STATUS_ERR;
}

/*!
  ****************************************************************************
  *@brief       Power on flash
  *@param       benable: flag to power on/off flash
  *@return      FS_STATUS_ENUM_t: file system status.
*****************************************************************************/
FS_STATUS_ENUM_t fs_flash_power_on (bool benable) {
  if(benable && (adp5360_is_ldo_enable(FS_LDO) == false)) {
    /* Switch On LDO3 */
    adp5360_enable_ldo(FS_LDO, true);
    /* Reset the memory */
    nand_flash_reset();
    /* Unblock locked flash module */
    if(fs_hal_unblock_flash() != FS_STATUS_OK) {
      return FS_STATUS_ERR;
    }
  }
  else if ((!benable) &&  (adp5360_is_ldo_enable(FS_LDO) == true)) {
    /* Check for file write and read in progress */
    if((fs_hal_write_access_state() == FS_FILE_ACCESS_START) && \
                     (ge_file_read_access == FS_FILE_ACCESS_START)) {
        adp5360_enable_ldo(FS_LDO, false);
    }
  }
  return FS_STATUS_OK;
}



#ifdef TEST_FS_NAND
#define MAX_BLOCKSIZE           2048
void fs_hal_test_file_write_read(uint32_t inp_num_iter, uint32_t block_size)
{
  FS_FILE *test_file;     /* File object */
  uint32_t byteswritten, bytesread;                     /* File write/read counts */
  uint32_t num_iter, i, time_ms, max_time;
  uint32_t cpu_cycle_scale = (MCU_HAL_GetCoreClock() / 1000000);
  uint64_t total_time;
  uint8_t tstbuf[MAX_BLOCKSIZE];

  FS_TRACE_INFO(("\r\nPerforming FS test. Number of iterations = %d. Block Size = %d bytes.\r\n", inp_num_iter, block_size));
  do {
    if(block_size > MAX_BLOCKSIZE)
    {
      FS_TRACE_INFO((":FAIL. Entered block size is greater than MAX_BLOCKSIZE.\r\n"));
      break;
    }
    for(i = 0; i < sizeof(tstbuf); i++)
      tstbuf[i] = 'A' + i%26;
    FS_TRACE_INFO(("\r\nWriting to file.\r\n"));
    if((test_file = fs_fopen("nand:0:\\TEST.LOG", "ab+")) == NULL)
    {
      FS_TRACE_INFO((":FAIL. fs_fopen failed.\r\n"));
      break;
    }
    if (fs_setvbuf(test_file, (void *)fileAccessBuf, FS__IOFBF, sizeof(fileAccessBuf)) != 0) {
      Debug_Handler();
    }

    num_iter = inp_num_iter;
    total_time = 0;
    max_time = 0;
    FS_TRACE_INFO((":BENCHMARK, %s,%s\r\n", "fs_fwrite iteration", "Time in milli seconds" ));
    for (; num_iter > 0; num_iter--)
    {
      time_ms = MCU_HAL_GetTick();
      byteswritten = fs_fwrite(tstbuf, sizeof(uint8_t), block_size, test_file);
      time_ms = MCU_HAL_GetTick() - time_ms;
      total_time += time_ms;
      if (time_ms > max_time)
        max_time = time_ms;

      FS_TRACE_INFO((":BENCHMARK, %d,%d\r\n", num_iter, time_ms));

      if((byteswritten != block_size))
      {
        FS_TRACE_INFO((":FAIL. fs_fwrite failed.\r\n"));
        break;
      }
    }
    FS_TRACE_INFO(("Average fs_fwrite time = %dms. Max fs_fwrite time = %dms. \r\n", (int)(total_time/inp_num_iter), max_time));
    FS_TRACE_INFO((":BENCHMARK, Average fs_fwrite time = %dms. Max fs_fwrite time = %dms. \r\n", (int)(total_time/inp_num_iter), max_time));
    if(num_iter == 0)
    {
      FS_TRACE_INFO((":PASS. File write PASS.\r\n"));
    }
    else
    {
      FS_TRACE_INFO(("Remaining count = %d\r\n",num_iter));
      break;
    }

    fs_fflush(test_file);                         /* Make sure data is written to file.   */
    if (fs_fclose(test_file) != 0)
    {
      FS_TRACE_INFO((":FAIL. fs_fclose failed.\r\n"));
      break;
    }

    FS_TRACE_INFO(("\r\nReading file and comparing.\r\n"));
    if((test_file = fs_fopen("nand:0:\\TEST.LOG", "r")) == NULL)
    {
      FS_TRACE_INFO((":FAIL. fs_fopen failed.\r\n"));
      break;
    }

    num_iter = inp_num_iter;
    total_time = 0;
    max_time = 0;
    FS_TRACE_INFO((":BENCHMARK, %s,%s\r\n", "fs_fread iteration", "Time in milli seconds"));
    for (; num_iter > 0; num_iter--)
    {
      memset((void *)tstbuf, 0 , block_size);
      time_ms = MCU_HAL_GetTick();
      bytesread = fs_fread(tstbuf, sizeof(uint8_t), block_size, test_file);
      time_ms = MCU_HAL_GetTick() - time_ms;
      total_time += time_ms;
      if (time_ms > max_time)
        max_time = time_ms;

      FS_TRACE_INFO((":BENCHMARK, %d,%d\r\n", num_iter, time_ms));

      if((bytesread != block_size))
      {
        FS_TRACE_INFO((":FAIL. fs_fread failed.\r\n"));
        break;
      }
      for(i = 0; i < block_size; i++)
      {
        if(tstbuf[i] != 'A' + i%26)
        {
          FS_TRACE_INFO((":FAIL. File data mismatch.\r\n"));
          break;
        }
      }
    }
    FS_TRACE_INFO(("Average fs_fread time = %dms. Max fs_fread time= %dms. \r\n", (int)(total_time/inp_num_iter), max_time));
    FS_TRACE_INFO((":BENCHMARK, Average fs_fread time = %dms. Max fs_fread time = %dms. \r\n", (int)(total_time/inp_num_iter), max_time));
    if((num_iter == 0) && (i == block_size)) {
      FS_TRACE_INFO((":PASS. File read and compare PASS.\r\n"));
    } else {
      FS_TRACE_INFO(("Remaining count = %d\r\n",num_iter));
      break;
    }

    if (fs_fclose(test_file) != 0)
    {
      FS_TRACE_INFO((":FAIL. fs_fopen failed.\r\n"));
      break;
    }
    FS_TRACE_INFO((":PASS. R/W test PASS.\r\n"));
  } while (0);
  FS_TRACE_INFO((" tests complete.\r\n\r\n"));
}
void fs_hal_test_features(void) {
  fs_hal_mount();
  fs_hal_format(true);
  fs_hal_test_file_write_read(300, 2048);
}
#endif //TEST_FS_NAND
#endif // USE_FS
