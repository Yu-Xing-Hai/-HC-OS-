#include "fs.h"
#include "stdint.h"
#include "global.h"
#include "ide.h"
#include "inode.h"
#include "super_block.h"
#include "dir.h"

/*Initialized the partition's meta-information, create the file system*/
static void partition_format(struct partition* part) {
    uint32_t boot_sector_sects = 1;
    uint32_t super_block_sects = 1;
    uint32_t inode_bitmap_sects = DIV_ROUND_UP(MAX_FILES_PER_PART, BITS_PER_SECTOR);  //The sectors mumber of inode holding, it suport max 4096 flies.
    uint32_t inode_table_sects = DIV_ROUND_UP(((sizeof(struct inode) * MAX_FILES_PER_PART)), SECTOR_SIZE);
    uint32_t used_sects = boot_sector_sects + super_block_sects + inode_bitmap_sects + inode_table_sects;
    uint32_t free_sects = part->sec_cnt - used_sects;

    uint32_t block_bitmap_sects;
    block_bitmap_sects = DIV_ROUND_UP(free_sects, BITS_PER_SECTOR);
    uint32_t block_bitmap_bit_len = free_sects - block_bitmap_sects;
    block_bitmap_sects = DIV_ROUND_UP(block_bitmap_bit_len, BITS_PER_SECTOR);

    /*Initialized the super block*/
    struct super_block sb;
    sb.magic = 0x19590318;
    sb.sec_cnt = part->sec_cnt;
    sb.inode_cnt = MAX_FILES_PER_PART;
    sb.part_lba_base = part->start_lba;

    sb.block_bitmap_lba = sb.part_lba_base + 2;  //The first is boot block, second is super block.
    sb.block_bitmap_sects = block_bitmap_sects;

    sb.inode_bitmap_lba = sb.block_bitmap_lba + sb.block_bitmap_sects;
    sb.inode_bitmap_sects = inode_bitmap_sects;

    sb.inode_table_lba = sb.inode_bitmap_lba + sb.inode_bitmap_sects;
    sb.inode_table_sects = inode_table_sects;

    sb.data_start_lba = sb.inode_table_lba + sb.inode_table_sects;
    sb.root_inode_no = 0;
    sb.dir_entry_size = sizeof(struct dir_entry);

    printk("%s info:\n", part->name);
    printk("  magic:0x%x\n  part_lba_base:0x%x\n  all_sectors:0x%x\n  inode_cnt:0x%x\n  block_bitmap_lba:0x%x\n  block_bitmap_sectors:0x%x\n  inode_bitmap_lba:0x%x\n  inode_bitmap_sectors:0x%x\n  inode_table_lba:0x%x\n  inode_table_sectors:0x%x\n  data_start_lba:0x%x\n",  sb.magic, sb.part_lba_base, sb.sec_cnt, sb.inode_cnt, sb.block_bitmap_lba, sb.block_bitmap_sects, sb.inode_bitmap_lba, sb.inode_bitmap_sects, sb.inode_table_lba, sb.inode_table_sects, sb.data_start_lba);

    struct disk* hd = part->my_disk;
    //1. write the super block to first sector in this partition
    ide_write(hd, part->start_lba + 1, &sb, 1);  //Write the number of sec_cnt data(1) from buffer to disk,skip the boot sector
    printk("  super_block_lba:0x%x\n", part->start_lba + 1);

    //now, we want to write these meta-information from memory to disk, so we need one buffer to store temporary information and we must find the max-size of meta-information.
    uint32_t buf_size = (sb.block_bitmap_sects >= sb.inode_bitmap_sects ? sb.block_bitmap_sects : sb.inode_bitmap_sects);
    buf_size = (buf_size >= sb.inode_table_sects ? buf_size : sb.inode_table_sects) * SECTOR_SIZE;

    uint8_t* buf = (uint8_t*)sys_malloc(buf_size);

    //2. Initialized the block_bitmap and write to sb.block_bitmap_lba
    buf[0] |= 0x01;  //We reserve the 0 block to root directory
    //We must set the remain bits to 1 in last sector of block_bitmap
    uint32_t block_bitmap_last_byte = block_bitmap_bit_len / 8;
    uint8_t block_bitmap_last_bit = block_bitmap_bit_len % 8;
    uint32_t last_size = SECTOR_SIZE - (block_bitmap_last_byte % SECTOR_SIZE);  //last_size is the ramain part of last sector of block_bitmap
    memset(&buf[block_bitmap_last_byte], 0xff, last_size);
    uint8_t bit_idx = 0;
    while(bit_idx <= block_bitmap_last_bit) {
        buf[block_bitmap_last_byte] &= ~(1 << bit_idx++);
    }
    ide_write(hd, sb.block_bitmap_lba, buf, sb.block_bitmap_sects);

    //3. Initialized the inode_bitmap and write to sb.inode_bitmap_lba
    
}