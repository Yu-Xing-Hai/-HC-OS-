#ifndef __DEVICE_IDE_H
#define __DEVICE_IDE_H
#include "stdint.h"
#include "list.h"
#include "bitmap.h"
#include "sync.h"

/*The strcuture of Partition*/
struct partition {
    uint32_t start_lba;
    uint32_t sec_cnt;  //The mumber os sectors
    struct disk* my_disk;  //The disk which the partition belongs to.
    struct list_elem part_tag;  //The tag of this partition(used in queue)
    char name[8];  //The name of the partition
    struct super_block* sb;  //We define that one sector is one block.
    struct bitmap block_bitmap;  //The bitmap which is used to manage the block.
    struct bitmap inode_bitmap;
    struct list open_inodes;
};

/*The structure of Disk*/
struct disk {
    char name[8];
    struct ide_channel* my_channel;
    uint8_t dev_no;  //Master is 0, Slave is 1.
    struct partition prim_parts[4];  //The main partition's number is 4.
    struct partition logic_parts[8];  //The logic partition's number is 8.
};

/*The structure of ata Channel*/
struct ide_channel {
    char name[8];  //IDE channel name,such as ata0\ide0
    uint16_t port_base;  //The base address of port in this channel.,such as 0x1F0 is the base port of channel 1.
    uint8_t irq_no;  //Different interrupt number indicate the different channel.
    struct lock lock;  //Make sure that intr_handler know which disk send the signal.
    bool expecting_intr;  //True: this channel is waitting for disk's interrupt(after process, disk will send interrupt signal).
    struct semaphore disk_done;  //The semaphore of the channel, used to block or wake up the drive-program.
    struct disk devices[2];  //The two devices of the channel(master and slave)
};

void intr_hd_handler(uint8_t irq_no);
void ide_read(struct disk* hd, uint32_t lba, void* buf, uint32_t sec_cnt);
void ide_write(struct disk* hd, uint32_t lba, void* buf, uint32_t sec_cnt);
void ide_init(void);

#endif