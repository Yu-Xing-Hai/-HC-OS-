#ifndef __FS_INODE_H
#define __FS_INODE_H

#include "stdint.h"
#include "list.h"
#include "ide.h"

/*the structure of inode*/
struct inode {
    uint32_t i_no;  //the number of inode
    /*
    When the inode indicated the file, the i_size is indicated the size of file,
    when the inode indicated the directory, the i_size is indicated the title size of directory entry.
    */
    uint32_t i_size;  //the unite is Byte

    uint32_t i_open_cnts;  //recode the number of times of file be opened
    bool write_deny;  //When the thread want to write this file, it must check the flag to make sure mutual

    /*i_sectors[0~11] is the direct data block's pointer, the i_secotrs[12] be used to store one-level data indirect block's pointer*/
    uint32_t i_sectors[13];  //Our data block's size is euqal with sector, so i_sectors is same to i_block;
    struct list_elem inode_tag;
};

/*Used to store the inode's position in disk*/
struct inode_position {
    bool two_sec;  //whether the inode cross the sector
    uint32_t sec_lba;  //The sector which inode's start address begin in
    uint32_t off_size;  //The offset of inode in sector(unit is byte)
};

static void inode_locate(struct partition* part, uint32_t inode_no, struct inode_position* inode_pos);
void inode_sync(struct partition* part, struct inode* inode, void* io_buf);
struct inode* inode_open(struct partition* part, uint32_t inode_no);
void inode_close(struct inode* inode);
void inode_init(uint32_t inode_no, struct inode* new_inode);

#endif