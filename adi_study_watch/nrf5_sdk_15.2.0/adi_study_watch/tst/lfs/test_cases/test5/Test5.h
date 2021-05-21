#include "light_fs.h"
#include "light_fs_test_bench.h"

#define BAD_BLK_TEST
#define CIRCULAR_BUFFER_TEST
#define READ_WRITE_TEST

#ifndef CIRCULAR_BUFFER_TEST_CASES
#define CIRCULAR_BUFFER_TEST_CASES
#define LFS_CIRC_TRAV_1
#define LFS_CIRC_TRAV_2
#define LFS_CIRC_TRAV_3
#define LFS_CIRC_TRAV_5
#define LFS_CIRC_TRAV_6
#define LFS_CIRC_TRAV_12
#define LFS_CIRC_TRAV_13
#define LFS_CIRC_TRAV_14
#define LFS_CIRC_TRAV_15
#define BAD_BLOCK_SIZE          100
#endif

#ifndef TEST_BAD_BLOCK
#define TEST_BAD_BLOCK
#define LFS_BAD_BLK_MANAGE_1
#define LFS_BAD_BLK_MANAGE_3
#endif