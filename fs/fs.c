#include "fs.h"
#include "stdint.h"
#include "debug.h"
#include "global.h"
#include "inode.h"
#include "super_block.h"
#include "dir.h"
#include "stdio-kernel.h"
#include "ide.h"
#include "string.h"
#include "memory.h"

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

    struct disk* hd = part->my_disk;  //Get the disk which is this partition belong to
    //1. write the super block to the index of 1 sector in this partition
    ide_write(hd, part->start_lba + 1, &sb, 1);  //Write the super block from buffer to disk, skip the boot sector
    printk("  super_block_lba:0x%x\n", part->start_lba + 1);

    //now, we want to write these meta-information in this function from memory to disk, so we need one buffer to store temporary information and we must find the max-size of meta-information to make sure the buffer is enough.
    uint32_t buf_size = (sb.block_bitmap_sects >= sb.inode_bitmap_sects ? sb.block_bitmap_sects : sb.inode_bitmap_sects);
    buf_size = (buf_size >= sb.inode_table_sects ? buf_size : sb.inode_table_sects) * SECTOR_SIZE;

    uint8_t* buf = (uint8_t*)sys_malloc(buf_size);  //the size's unit is byte

    //2. Initialized the block_bitmap and write to disk
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

    //3. Initialized the inode_bitmap and write to disk
    memset(buf, 0, buf_size);
    buf[0] |= 0x1;
    ide_write(hd, sb.inode_bitmap_lba, buf, sb.inode_bitmap_sects);

    //4. Initialized the inode table and write to disk
    // write the zero entry's inode, root's inode
    memset(buf, 0, buf_size);
    struct inode* i = (struct inode*)buf;
    i->i_size = sb.dir_entry_size * 2;  // . and .., when the inode indicated the directory, the i_size is indicated the title size of directory entry.
    i->i_no = 0;
    i->i_sectors[0] = sb.data_start_lba;  //the root directory's address pointer
    ide_write(hd, sb.inode_table_lba, buf, sb.inode_table_sects);

    //5. write root directory to sb.data_start_lba
    //write two directory entries to root directory, . and ..
    memset(buf, 0, buf_size);
    struct dir_entry* p_de = (struct dir_entry*)buf;

    //Initialized the current directory ".", like "./include"
    memcpy(p_de->filename, ".", 1);
    p_de->i_no = 0;
    p_de->f_type = FT_DIRECTORY;
    p_de++;

    //Initialized the current directory's parent directory "..", like "cd .."
    memcpy(p_de->filename, "..", 2);
    p_de->i_no = 0;  //root directory's parent directory is itself
    p_de->f_type = FT_DIRECTORY;
    ide_write(hd, sb.data_start_lba, buf, 1);

    printk("  root_dir_lba:0x%x\n", sb.data_start_lba);
    printk("%s format done\n", part->name);
    sys_free(buf);
}

/*Search file-system in disk, if don't have file-system, then, format the partition and create file-system*/
void filesys_init() {
    uint8_t channel_no = 0, dev_no, part_idx = 0;

    /*sb_buf is used to store the super-block's information which is read from disk*/
    struct super_block* sb_buf = (struct super_block*)sys_malloc(SECTOR_SIZE);

    if(sb_buf == NULL) {
        PANIC("alloc memory failed!");
    }
    printk("searching file-system......\n");
    while(channel_no < channel_cnt) {
        dev_no = 0;
        while(dev_no < 2) {
            if(dev_no == 0) {  //Skip the disk of hd60M.img
                dev_no++;
                continue;
            }
            struct disk* hd = &channels[channel_no].devices[dev_no];
            struct partition* part = hd->prim_parts;
            while(part_idx < 12) {  //four prim partition and eight logic partition
                if(part_idx == 4) {  //begin to process logic partition
                    part = hd->logic_parts;
                }
                if(part->sec_cnt != 0) {  //if partition is exist, then do next code
                    memset(sb_buf, 0, SECTOR_SIZE);

                    ide_read(hd, part->start_lba + 1, sb_buf, 1);

                    if(sb_buf->magic == 0x19590318) {  //we only support the file-systemp which is created by partition_format()
                        printk("%s has file-system\n", part->name);
                    }
                    else {
                        printk("formatting %s's partition %s......\n", hd->name, part->name);
                        partition_format(part);
                    }
                }
                part_idx++;
                part++;  //next partition
            }
            dev_no++;  //next device
        }
        channel_no++;  //next channel
    }
    sys_free(sb_buf);
}