#ifndef __FS__SUPER_BLOCK_H
#define __FS__SUPER_BLOCK_H

#include "stdint.h"

/*super block*/
struct super_block {
    uint32_t magic;
    
    uint32_t sec_cnt;  //we define that the data block's size is equal with sector's size
    uint32_t inode_cnt;
    uint32_t part_lba_base;

    uint32_t block_bitmap_lba;
    uint32_t block_bitmap_sects;  //the bitmap's size(the number of sectors whick is conquered)

    uint32_t inode_bitmap_lba;
    uint32_t inode_bitmap_sects;

    uint32_t inode_table_lba;
    uint32_t inode_table_sects;

    uint32_t data_start_lba;  //the root directory's start address also is the data area's begin address
    uint32_t root_inode_no;  //the inode index which is included root directory
    uint32_t dir_entry_size;

    uint8_t pad[460];  //padding to one sector
} __attribute__((packed));

#endif