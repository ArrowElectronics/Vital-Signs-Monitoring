/*
*********************************************************************************************************
*
*
*
* File    : APP_CFG.H
*********************************************************************************************************
*/
/*!
 *  \copyright Analog Devices
 * ****************************************************************************
 *
 * License Agreement
 *
 * Copyright (c) 2019 Analog Devices Inc.
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
#if !defined(APP_CFG_H)
#define APP_CFG_H

/*
*********************************************************************************************************
*                                              INCLUDE FILES
*********************************************************************************************************
*/
#include <printf.h>



/*
*********************************************************************************************************
*                                       MODULE ENABLE / DISABLE
*********************************************************************************************************
*/

#define  APP_CFG_FS_EN                          DEF_ENABLED

/*
*********************************************************************************************************
*                                           TASKS PRIORITIES
*********************************************************************************************************
*/
/*Low priority numbers denote low priority tasks. The idle task has priority zero (tskIDLE_PRIORITY) */
//#define  APP_OS_CFG_BOOT_TASK_PRIO          10
#define  APP_OS_CFG_POST_OFFICE_TASK_PRIO   9
#define  APP_OS_CFG_USBD_TX_TASK_PRIO       6
#define  APP_OS_CFG_PM_TASK_PRIO            4
#define  APP_OS_CFG_FS_TASK_PRIO            4
//#define  APP_OS_CFG_LED_TASK_PRIO           3
//#define  APP_OS_CFG_SPI_COMMS_TASK_PRIO     5
//#define  APP_OS_CFG_AD7156_TASK_PRIO        4

#define  APP_OS_CFG_DISPLAY_TASK_PRIO       4
#define  APP_OS_CFG_KEY_TASK_PRIO           5
#define  APP_OS_CFG_TOUCH_TASK_PRIO         5
#define  APP_OS_CFG_LT_APP_TASK_PRIO        5
#define  APP_OS_CFG_USER0_CONFIG_APP_TASK_PRIO  5
#define  APP_OS_BATTERY_TEMP_DETECT_TASK_PRIO         4

//#define  APP_OS_CFG_USBD_RX_TASK_PRIO       8
#define  APP_OS_CFG_WDT_TASK_PRIO           1
#define  APP_OS_CFG_RTOS_TEST_TASK_PRIO         10
#define  APP_OS_CFG_BLE_TASK_PRIO           5
#define  APP_OS_CFG_BLE_SERVICES_SENSOR_TASK_PRIO  5
#define  APP_OS_CFG_USBD_APP_TASK_PRIO      8
#define  APP_OS_CFG_LOGGER_TASK_PRIO        1

#define  APP_OS_CFG_SENSOR_ADPD4000_TASK_PRIO          6
#define  APP_OS_CFG_SENSOR_ADXL_TASK_PRIO              6
#define  APP_OS_CFG_BCM_TASK_PRIO                      6
#define  APP_OS_CFG_ECG_TASK_PRIO                      7
#define  APP_OS_CFG_EDA_TASK_PRIO                      6
#define  APP_OS_CFG_PEDOMETER_APP_TASK_PRIO            6
#define  APP_OS_CFG_PPG_APPLICATION_TASK_PRIO          6
#define  APP_OS_CFG_TEMPERATURE_APP_TASK_PRIO          6
#define  APP_OS_CFG_SQI_APP_TASK_PRIO                  5

/*
*********************************************************************************************************
*                                              STACK SIZES
*                             Size of the task stacks (# of CPU_STK entries)
*********************************************************************************************************
*/

#define  APP_OS_CFG_POST_OFFICE_TASK_STK_SIZE     2048
#define  APP_OS_CFG_USBD_TX_TASK_STK_SIZE         2048
#define  APP_OS_CFG_DISPLAY_TASK_STK_SIZE         2048
#define  APP_OS_CFG_TOUCH_TASK_STK_SIZE           1024
#define  APP_OS_CFG_LT_APP_TASK_STK_SIZE          2048
#define  APP_OS_CFG_USER0_CONFIG_APP_TASK_STK_SIZE 1024
//#define  APP_OS_CFG_AD7156_TASK_STK_SIZE          512
#define  APP_OS_BATTERY_TEMP_DETECT_TASK_STK_SIZE           1024

#define  APP_OS_CFG_KEY_TASK_STK_SIZE             512
#define  APP_OS_CFG_PM_TASK_STK_SIZE              (1024+256)
#define  APP_OS_CFG_FS_TASK_STK_SIZE              (1024+512)
#define  APP_OS_CFG_WDT_TASK_STK_SIZE             512
#define  APP_OS_CFG_BLE_TASK_STK_SIZE             2048
#define  APP_OS_CFG_BLE_SERVICES_SENSOR_TASK_STK_SIZE  1024
#define  APP_OS_CFG_USBD_APP_TASK_STK_SIZE        512
#define  APP_OS_CFG_LOGGER_TASK_STK_SIZE          2048
#define  APP_OS_CFG_RTOS_TEST_TASK_STK_SIZE       1024

#define  APP_OS_CFG_SENSOR_ADPD4000_TASK_STK_SIZE      (1024 + 512)
#define  APP_OS_CFG_SENSOR_ADXL_TASK_STK_SIZE           1024
#define  APP_OS_CFG_BCM_TASK_STK_SIZE                   2048
#ifdef ECG_HR_ALGO
#define  APP_OS_CFG_ECG_TASK_STK_SIZE                   (1024+2000)
#else
#define  APP_OS_CFG_ECG_TASK_STK_SIZE                   (1024)
#endif
#define  APP_OS_CFG_EDA_TASK_STK_SIZE                   2048
#define  APP_OS_CFG_PEDOMETER_APP_TASK_STK_SIZE         1024
#define  APP_OS_CFG_PPG_APPLICATION_TASK_STK_SIZE       1024
#define  APP_OS_CFG_TEMPERATURE_APP_TASK_STK_SIZE       (1024 + 512)
#define  APP_OS_CFG_SQI_APP_TASK_STK_SIZE               1024
/*
*********************************************************************************************************
*                                          FS CONFIGURATION
*
* Note(s) : This section define various preprocessor constant used by the example initialization code
*           located in fs_app.c to configure the file system.
*********************************************************************************************************
*/

#define  APP_CFG_FS_DEV_CNT          1                          /* Maximum number of opened devices.                    */
#define  APP_CFG_FS_VOL_CNT          1                          /* Maximum number of opened volumes.                    */
#define  APP_CFG_FS_FILE_CNT         1                          /* Maximum number of opened files.                      */
#define  APP_CFG_FS_DIR_CNT          1                          /* Maximum number of opened directories.                */
#define  APP_CFG_FS_BUF_CNT          (4 * APP_CFG_FS_VOL_CNT)   /* Internal buffer count.                               */
#define  APP_CFG_FS_DEV_DRV_CNT      1                          /* Maximum number of different device drivers.          */
#define  APP_CFG_FS_WORKING_DIR_CNT  1                          /* Maximum number of active working directories.        */
#define  APP_CFG_FS_MAX_SEC_SIZE     2048                        /* Maximum sector size supported.                       */

#define  APP_CFG_FS_IDE_EN           DEF_DISABLED               /* Enable/disable the IDE\CF initialization.            */
#define  APP_CFG_FS_MSC_EN           DEF_DISABLED               /* Enable/disable the MSC initialization.               */
#define  APP_CFG_FS_NAND_EN          DEF_ENABLED                /* Enable/disable the NAND initialization.              */
#define  APP_CFG_FS_NOR_EN           DEF_DISABLED               /* Enable/disable the NOR initialization.               */
#define  APP_CFG_FS_RAM_EN           DEF_DISABLED               /* Enable/disable the RAMDisk initialization.           */
#define  APP_CFG_FS_SD_EN            DEF_DISABLED               /* Enable/disable the SD (SPI) initialization.          */
#define  APP_CFG_FS_SD_CARD_EN       DEF_DISABLED               /* Enable/disable the SD (Card) initialization.         */


/*
*********************************************************************************************************
*                                    NAND DRIVER CONFIGURATION
*********************************************************************************************************
*/

#define APP_CFG_FS_NAND_BSP             FS_NAND_BSP_M29F4G
#define APP_CFG_FS_NAND_CTRLR_IMPL      CTRLR_GEN

#define APP_CFG_FS_NAND_CTRLR_GEN_EXT   FS_NAND_CtrlrGen_MicronECC
#define APP_CFG_FS_NAND_PART_TYPE       ONFI
//#define APP_CFG_FS_NAND_PART_TYPE       STATIC
//#define APP_CFG_FS_NAND_FREE_SPARE_START        0u
//#define APP_CFG_FS_NAND_FREE_SPARE_LEN          63u

#define APP_CFG_FS_NAND_FREE_SPARE_MAP       {{0x04u, 4u}, \
                                              {0x14u, 4u}, \
                                              {0x24u, 4u}, \
                                              {0x34u, 4u}, \
                                              { -1 , -1}}

#define APP_CFG_FS_NAND_BLK_CNT                 4096u
#define APP_CFG_FS_NAND_PG_PER_BLK              64u
#define APP_CFG_FS_NAND_PG_SIZE                 2048u
#define APP_CFG_FS_NAND_SPARE_SIZE              64u
#define APP_CFG_FS_NAND_NBR_PGM_PER_PG          4
#define APP_CFG_FS_NAND_BUS_WIDTH               8u
#define APP_CFG_FS_NAND_ECC_NBR_CORR_BITS       4
#define APP_CFG_FS_NAND_ECC_CODEWORD_SIZE       (512 + 16)
#define APP_CFG_FS_NAND_DEFECT_MARK_TYPE        DEFECT_SPARE_L_1_PG_1_OR_N_ALL_0
#define APP_CFG_FS_NAND_MAX_BAD_BLK_CNT         80u
#define APP_CFG_FS_NAND_MAX_BLK_ERASE           100000u

#define APP_CFG_FS_NAND_PART_SEC_SIZE           2048u
#define APP_CFG_FS_NAND_PART_BLK_CNT            1024u
#define APP_CFG_FS_NAND_PART_BLK_IX_FIRST       0u
#define APP_CFG_FS_NAND_PART_UB_CNT_MAX         10u
#define APP_CFG_FS_NAND_PART_RUB_MAX_ASSOC      2u
#define APP_CFG_FS_NAND_PART_MAX_BLK_TBL_ENTRY  10u

/*
*********************************************************************************************************
*                                     TRACE / DEBUG CONFIGURATION
*********************************************************************************************************
*/

#ifndef  TRACE_LEVEL_OFF
#define  TRACE_LEVEL_OFF                        0u
#endif

#ifndef  TRACE_LEVEL_INFO
#define  TRACE_LEVEL_INFO                       1u
#endif

#ifndef  TRACE_LEVEL_DBG
#define  TRACE_LEVEL_DBG                        2u
#endif

#endif    /* APP_CFG_H */
