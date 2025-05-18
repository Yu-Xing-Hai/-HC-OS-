#include "inode.h"
#include "stdint.h"
#include "string.h"
#include "ide.h"
#include "debug.h"
#include "list.h"
#include "thread.h"
#include "memory.h"
#include "interrupt.h"

/*Used to store the inode's position in disk*/
struct inode_position {
    bool two_sec;  //whether the inode cross the sector
    uint32_t sec_lba;  //The sector which inode's start address begin in
    uint32_t off_size;  //The offset of inode in sector(unit is byte)
};

/*Locate the inode's lba and offset in sector and write to inode_pos*/
/*
Pay attention: the inode's size is not equal to sector, we define the data block's size is equal to sector.
*/
static void inode_locate(struct partition* part, uint32_t inode_no, struct inode_position* inode_pos) {
    ASSERT(inode_no < 4096);
    uint32_t inode_table_lba = part->sb->inode_table_lba;  //The inode_table's each element is continual in disk

    uint32_t inode_size = sizeof(struct inode);
    uint32_t off_size = inode_no * inode_size;  //The offset of inode in inode table(unit is byte)
    uint32_t off_sec = off_size / 512;  //The offset of inode in inode table(unit is sector)
    uint32_t off_size_in_sec = off_size % 512;  //The offset of inode's start address in one sector

    //To recognize whether the inode cross two sector
    uint32_t left_in_sec = 512 - off_size_in_sec;
    if(left_in_sec < inode_size) {  //whether the setcor's remaining space can store one inode
        inode_pos->two_sec = true;
    }
    else {
        inode_pos->two_sec = false;
    }
    inode_pos->sec_lba = inode_table_lba + off_sec;
    inode_pos->off_size = off_size_in_sec;
}

/*Synchronize the inode to part by using buffer of io_buf.*/
void inode_sync(struct partition* part, struct inode* inode, void* io_buf) {
    //io_buf is used to be the I/O buffer of disk, it will be provided by main function
    uint8_t inode_no = inode->i_no;
    struct inode_position inode_pos;
    inode_locate(part, inode_no, &inode_pos);  //Locate the inode's position in disk and store it to inode_pos
    ASSERT(inode_pos.sec_lba <= (part->start_lba + part->sec_cnt));

    /*When we want to synchronoize inode, we must clean the i_open_cnts,write_deny,inode_tag in inode, 
    because these information only have meaning in memory rather than in disk.*/
    struct inode pure_inode;  //Temporary structure
    memcpy(&pure_inode, inode, sizeof(struct inode));  //copy inode to pure_inode

    //Clean these information and operate pure_inode in below code to make sure the source code's integrity
    pure_inode.i_open_cnts = 0;
    pure_inode.write_deny = false;
    pure_inode.inode_tag.prev = pure_inode.inode_tag.next = NULL;
    
    char* inode_buf = (char*)io_buf;
    if(inode_pos.two_sec == true) {  //whether the inode cross the sector
        ide_read(part->my_disk, inode_pos.sec_lba, inode_buf, 2);

        memcpy((inode_buf + inode_pos.off_size), &pure_inode, sizeof(struct inode));

        ide_write(part->my_disk, inode_pos.sec_lba, inode_buf, 2);
    }
    else {
        ide_read(part->my_disk, inode_pos.sec_lba, inode_buf, 1);
        memcpy((inode_buf + inode_pos.off_size), &pure_inode, sizeof(struct inode));
        ide_write(part->my_disk, inode_pos.sec_lba, inode_buf, 1);
    }
}

/*When we want to open inode, we use this function to return inode by using inode_no*/
struct inode* inode_open(struct partition* part, uint32_t inode_no) {
    struct list_elem* elem = part->open_inodes.head.next;  //Search inode in open_inodes firstly to upgrade opening speed.
    struct inode* inode_found;
    while(elem != &part->open_inodes.tail) {
        inode_found = elem2entry(struct inode, inode_tag, elem);
        if(inode_found->i_no == inode_no) {  //Found the inode which we want to operate
            inode_found->i_open_cnts++;
            return inode_found;
        }
        elem = elem->next;
    }

    /*If we cann't find inode in ope_inodes, we must read disk to load this inode to open_inodes*/
    struct inode_position inode_pos;  //Store the inode's information of position

    inode_locate(part, inode_no, &inode_pos);

    /*To see the function of sys_malloc's principle, so you can understand why*/
    struct task_struct* cur = running_thread();  //Get current task
    uint32_t* cur_pagedir_bak = cur->pgdir;  //Store cur->pgdir's value temporarily
    cur->pgdir = NULL;
    inode_found = (struct inode*)sys_malloc(sizeof(struct inode));  //Distribute an memory in kernel space rather than user space to make sure this inode can be shared by all tasks.
    cur->pgdir = cur_pagedir_bak;  //Recover this parameters

    char* inode_buf;
    if(inode_pos.two_sec == true) {
        inode_buf = (char*)sys_malloc(1024);
        ide_read(part->my_disk, inode_pos.sec_lba, inode_buf, 2);
    }
    else {
        inode_buf = (char*)sys_malloc(512);
        ide_read(part->my_disk, inode_pos.sec_lba, inode_buf, 1);
    }
    memcpy(inode_found, inode_buf + inode_pos.off_size, sizeof(struct inode));

    //Add this inode to the head of open_inodes
    list_push(&part->open_inodes, &inode_found->inode_tag);
    inode_found->i_open_cnts = 1;

    sys_free(inode_buf);
    return inode_found;
}

/*Close the inode or reduce the inode's open_cnts*/
void inode_close(struct inode* inode) {
    enum intr_status old_status = intr_disable();
    if(--inode->i_open_cnts == 0) {
        list_remove(&inode->inode_tag);

        struct task_struct* cur = running_thread();
        uint32_t* cur_pagedir_bak = cur->pgdir;
        cur->pgdir = NULL;
        sys_free(inode);
        cur->pgdir = cur_pagedir_bak;
    }
    intr_set_status(old_status);
}

/*Initialized the new_inode*/
void inode_init(uint32_t inode_no, struct inode* new_inode) {
    new_inode->i_no = inode_no;
    new_inode->i_size = 0;
    new_inode->i_open_cnts = 0;
    new_inode->write_deny = false;

    uint8_t sec_idx = 0;
    while(sec_idx < 13) {
        new_inode->i_sectors[sec_idx] = 0;
        sec_idx++;
    }
}