#include "ide.h"
#include "stdint.h"
#include "debug.h"
#include "global.h"
#include "stdio.h"
#include "stdio-kernel.h"
#include "sync.h"
#include "io.h"
#include "timer.h"
#include "interrupt.h"
#include "list.h"
#include "string.h"
#include "memory.h"

/*Define the disk's ports number*/
#define reg_data(channel) (channel->port_base + 0)
#define reg_error(channel) (channel->port_base + 1)
#define reg_sect_cnt(channel) (channel->port_base + 2)
#define reg_lba_l(channel) (channel->port_base + 3)
#define reg_lba_m(channel) (channel->port_base + 4)
#define reg_lba_h(channel) (channel->port_base + 5)
#define reg_dev(channel) (channel->port_base + 6)
#define reg_status(channel) (channel->port_base + 7)
#define reg_cmd(channel) reg_status(channel)
#define reg_alt_status(channel) (channel->port_base + 0x206)
#define reg_ctl(channel) reg_alt_status(channel)

/*Some key bits of reg_alt_status register*/
#define BIT_ALT_STAT_BSY 0x80  //The disk is busy
#define BIT_ALT_STAT_DRDY 0x40  //The disk driver is ready
#define BIT_ALT_STAT_DRQ 0x8  //Bit is 1, indicate the data is ready to be read.

/*Some key bits of device register*/
#define BIT_DEV_MBS 0xa0  //1010_0000,the 7th bit is 1, 6th bit is 0
#define BIT_DEV_LBA 0x40  //0100_0000,the LBA mode
#define BIT_DEV_DEV 0x10  //The device is a disk

/*Some commend to operate disk*/
#define CMD_IDENTIFY 0xec  //The command to identify the disk,get the disk's information
#define CMD_READ_SECTOR 0x20  //The command to read sector
#define CMD_WRITE_SECTOR 0x30  //The command to write sector

/*Used to debugging: define the biggest number of sectors to read and write*/
#define max_lba ((80*1024*1024/512) - 1)  //The biggest number of sectors in the disk, our disk is 80MB, and the first sector can't be used.

uint8_t channel_cnt;  //The number of channel which is caculated by disk_cnt
struct ide_channel channels[2];  //The two channels

int32_t ext_lba_base = 0;  //Extend partition's base sector recorded by LBA, it will be used in partition_scan().
uint8_t p_no = 0, l_no = 0;  //The index of primary partition and logic partition
struct list partition_list;  //The list of partition

/*Create an 16Byte structure(partition table entry).*/
struct partition_table_entry {
    uint8_t bootable;  //The flag of bootable
    uint8_t start_head;  //The start head of the partition
    uint8_t start_sec;  //The start sector of the partition
    uint8_t start_chs;  //The start cylinder of the partition
    uint8_t fs_type;  //The file system's type of the partition
    uint8_t end_head;  //The end head of the partition
    uint8_t end_sec;  //The end sector of the partition
    uint8_t end_chs;  //The end cylinder of the partition

    /*Pay attention*/
    uint32_t start_lba;  //The start LBA of the partition
    uint32_t sec_cnt;  //The number of sectors of the partition
} __attribute__((packed));  //The special commend in GCC, it means the structure will not be optimized,don't pad the structure to aling in memory.

/*Bootloader sector(Which include MBR or EBR)*/
struct boot_sector {
    uint8_t other[446];  //The program code of bootloader(just to Placeholding)
    struct partition_table_entry partition_table[4];  //The partition table entry, totally have 64Byte(4 entry * 16Byte)
    uint16_t signature;  //The end signature of the boot sector, 0x55AA
} __attribute__((packed));

/*The handler of disk's interrupt*/
void intr_hd_handler(uint8_t irq_no) {
    ASSERT(irq_no == 0x2e || irq_no == 0x2f);
    uint8_t channel_no = irq_no - 0x2e;
    struct ide_channel* channel = &channels[channel_no];
    ASSERT(channel->irq_no == irq_no);

    if(channel->expecting_intr == true) {
        channel->expecting_intr = false;
        sema_up(&channel->disk_done);

        /*If read status register, the disk's controler will make sure this interrupt has be processd.*/
        inb(reg_status(channel));
    }
}

/*Select the disk to read or write*/
static void select_disk(struct disk* hd) {
    uint8_t reg_device = BIT_DEV_MBS | BIT_DEV_LBA;
    if(hd->dev_no == 1) {
        reg_device |= BIT_DEV_DEV;  //if the device is slave, the bit is 1.
    }
    outb(reg_dev(hd->my_channel), reg_device);  //Finsh the selection of disk.
}

/*Push the LBA and sector's number to ports*/
static void select_sector(struct disk* hd, uint32_t lba, uint8_t sec_cnt) {
    ASSERT(lba <= max_lba);
    struct ide_channel* channel = hd->my_channel;

    /*Push the sector's number which will be wanted to read or write to port.*/
    outb(reg_sect_cnt(channel), sec_cnt);  //if the sector's number is 0, it means 256 sectors.

    /*Push the LBA address to port*/
    outb(reg_lba_l(channel), lba);
    outb(reg_lba_m(channel), lba >> 8);
    outb(reg_lba_h(channel), lba >> 16);

    /*LBA's bit is 0~27, so, we use register of device to store LBA's 24~27bit by using 0~3 of device register.*/
    outb(reg_dev(channel), BIT_DEV_MBS | BIT_DEV_LBA | (hd->dev_no == 1 ? BIT_DEV_DEV : 0) | lba >> 24);
}

/*Push commend to channel*/
static void cmd_out(struct ide_channel* channel, uint8_t cmd) {
    channel->expecting_intr = true;
    outb(reg_cmd(channel), cmd);
}

/*Writ from disk to buffer*/
static void read_from_sector(struct disk* hd, void* buf, uint8_t sec_cnt) {
    uint32_t size_in_byte;
    if(sec_cnt == 0) {
        size_in_byte = 256 * 512;
    }
    else {
        size_in_byte = sec_cnt * 512;
    }
    insw(reg_data(hd->my_channel), buf, size_in_byte / 2);
}

/*Writ from buffer to disk*/
static void write2sector(struct disk* hd, void* buf, uint8_t sec_cnt) {
    uint32_t size_in_byte;
     if(sec_cnt == 0) {
        size_in_byte = 256 * 512;
    }
    else {
        size_in_byte = sec_cnt * 512;
    }
    outsw(reg_data(hd->my_channel), buf, size_in_byte / 2);
}

/*Delay 30s, yield the CPU, if the disk is not ready after 30s, return false to PANIC*/
static bool busy_wait(struct disk* hd) {  //Make sure the disk is free.
    struct ide_channel* channel = hd->my_channel;
    uint16_t time_limit = 30 * 1000;
    while(time_limit -= 10 >= 0) {
        if(!(inb(reg_status(channel)) & BIT_ALT_STAT_BSY)) {  //bit is 1, the disk is busy.
            return (inb(reg_status(channel)) & BIT_ALT_STAT_DRQ);
        }
        else {
            mtime_sleep(10);  //Sleep 10ms
        }
    }
    return false;
}

/*Encapsulation_1: Read the number of sec_cnt data from dask to buffer*/
void ide_read(struct disk* hd, uint32_t lba, void* buf, uint32_t sec_cnt) {
    ASSERT(lba <= max_lba);
    ASSERT(sec_cnt > 0);
    lock_acquire(&hd->my_channel->lock);

    /*First: select disk*/
    select_disk(hd);

    /*Second: Push the sector's number which will be wanted to read or write and LBA address to port.*/
    uint32_t secs_op;  //The number of sectors which will be read or write in one operation.
    uint32_t secs_done = 0;  //The number of sectors which have been read or write.
    while(secs_done < sec_cnt) {
        if((secs_done + 256) <= sec_cnt) {  //Special: sec_cnt = 0.
            secs_op = 256;
        }
        else {
            secs_op = sec_cnt - secs_done;
        }
        select_sector(hd, lba + secs_done, secs_op);

        /*Third: Push commend to reg_cmd register*/
        cmd_out(hd->my_channel, CMD_READ_SECTOR);  //Ready to read data.

        /*Forth: The disk start to busy, so,we block current process, when disk finish it's work, it will use interrupt to wake up this process.*/
        sema_down(&hd->my_channel->disk_done);

        /*Fifth: Check the disk's state to make sure whether the disk could be read.*/
        if(!busy_wait(hd)) {
            char error[64];
            sprintf(error, "%s read sector %d failed!!!\n", hd->name, lba);
            PANIC(error);
        }

        /*Sixth: Read data from disk to buffer.*/
        read_from_sector(hd, (void*)((uint32_t)buf + secs_done * 512), secs_op);
        secs_done += secs_op;
    }
    lock_release(&hd->my_channel->lock);
}

/*Encapsulation_2: Write the number of sec_cnt data from buffer to disk*/
void ide_write(struct disk* hd, uint32_t lba, void* buf, uint32_t sec_cnt) {
    ASSERT(lba <= max_lba);
    ASSERT(sec_cnt > 0);
    lock_acquire(&hd->my_channel->lock);

    /*First: select disk*/
    select_disk(hd);

    /*Second: Push the sector's number which will be wanted to read or write and LBA address to port.*/
    uint32_t secs_op;  //The number of sectors which will be read or write in one operation.
    uint32_t secs_done = 0;  //The number of sectors which have been read or write.
    while(secs_done < sec_cnt) {
        if((secs_done + 256) <= sec_cnt) {  //Special: sec_cnt = 0.
            secs_op = 256;
        }
        else {
            secs_op = sec_cnt - secs_done;
        }
        select_sector(hd, lba + secs_done, secs_op);

        /*Third: Push commend to reg_cmd register*/
        cmd_out(hd->my_channel, CMD_WRITE_SECTOR);  //Ready to write data.

        /*Forth: Check the disk's state to make sure whether the disk could be read.*/
        if(!busy_wait(hd)) {
            char error[64];
            sprintf(error, "%s write sector %d failed!!!\n", hd->name, lba);
            PANIC(error);
        }

        /*Fifth: Write data from buffer to disk.*/
        write2sector(hd, (void*)((uint32_t)buf + secs_done * 512), secs_op);

        /*Sixth: The disk start to busy, so,we block current process, when disk finish it's work, it will use interrupt to wake up this process.*/
        sema_down(&hd->my_channel->disk_done);
        secs_done += secs_op;
    }
    lock_release(&hd->my_channel->lock);
}

static void swap_pairs_bytes(const char* dst, char* buf, uint32_t len) {
    uint8_t idx;
    for(idx = 0; idx < len; idx += 2) {
        buf[idx + 1] = *dst++;
        buf[idx] = *dst++;
    }
    buf[idx] = '\0';
}

/*Get the disk's parameters information*/
static void identify_disk(struct disk* hd) {
    char id_info[512];  //Store the information which return from identify commend.
    select_disk(hd);
    cmd_out(hd->my_channel, CMD_IDENTIFY);  //Write the commend to disk's port.

    sema_down(&hd->my_channel->disk_done);  //When disk finish it's work, it will send interrupt,and intr_handler has function of sema_up.

    if(!busy_wait(hd)) {
        char error[64];
        sprintf(error, "%s identify failed!!!\n", hd->name);
        PANIC(error);
    }
    read_from_sector(hd, id_info, 1);

    char buf[64];  //Used by swap_pairs_bytes(), store the translated answer.
    uint8_t sn_start = 10 * 2, sn_len = 20, md_start = 27 * 2, md_len = 40;

    swap_pairs_bytes(&id_info[sn_start], buf, sn_len);
    printk("  disk %s info:\n    SN: %s\n", hd->name, buf);

    memset(buf, 0, sizeof(buf));

    swap_pairs_bytes(&id_info[md_start], buf, md_len);
    printk("    MODULE: %s\n", buf);

    uint32_t sectors = *(uint32_t*)&id_info[60 * 2];
    printf("    SECTORS: %d\n", sectors);
    printk("    CAPACITY: %dMB\n", sectors * 512 / 1024 / 1024);
}

/*Scan the patitions in sector which address is ext_lba*/
static void partition_scan(struct disk* hd, uint32_t ext_lba) {
    struct boot_sector* bs = sys_malloc(sizeof(struct boot_sector));
    ide_read(hd, ext_lba, bs, 1);  //Read the boot sector(have be created) from disk to bs.
    uint8_t part_idx = 0;
    struct partition_table_entry* p = bs->partition_table;

    /*Traverse 4 patition table entries*/
    while(part_idx++ < 4) {
        if(p->fs_type == 0x5) {  //If the file system's type is extend patition
            if(ext_lba_base != 0) {
                partition_scan(hd, p->start_lba + ext_lba_base);  //The start_lba of Sub-extend-patition is the offset by address of total-extend-patition.
            }
            else {  //ext_lba_base = 0 indicate that we first times to read bootsector.
                ext_lba_base = p->start_lba;  //Total-extend-patition's start address.
                partition_scan(hd, p->start_lba);
            }
        }
        else if(p->fs_type != 0) {  //The patition's type is effective.
            if(ext_lba == 0) {  //Not is extend, so, it is primary patition.
                hd->prim_parts[p_no].start_lba = ext_lba + p->start_lba;
                hd->prim_parts[p_no].sec_cnt = p->sec_cnt;
                hd->prim_parts[p_no].my_disk = hd;
                list_append(&partition_list, &hd->prim_parts[p_no].part_tag);
                sprintf(hd->prim_parts[p_no].name, "%s%d", hd->name, p_no + 1);
                p_no++;
                ASSERT(p_no < 4);
            }
            else {
                hd->logic_parts[l_no].start_lba = ext_lba + p->start_lba;
                hd->logic_parts[l_no].sec_cnt = p->sec_cnt;
                hd->logic_parts[l_no].my_disk = hd;
                list_append(&partition_list, &hd->logic_parts[l_no].part_tag);
                sprintf(hd->logic_parts[l_no].name, "%s%d", hd->name, l_no + 5);
                l_no++;
                if(l_no >= 8)  //only support 8 logic partitions at most.
                    return;
            }
        }
        p++;
    }
    sys_free(bs);
}

/*Print th information of patition*/
static bool partition_info(struct list_elem* pelem, int arg) {
    UNUSED(arg);
    struct partition* part = elem2entry(struct partition, part_tag, pelem);
    printk("  %s start_lba:0x%x, sec_cnt:0x%x\n", part->name, part->start_lba, part->sec_cnt);

    return false;
}

/*Initialize the disk's structure*/
void ide_init() {
    printk("ide_init start\n");

    uint8_t hd_cnt = *((uint8_t*)(0x475));  //Get the number of hard disk
    ASSERT(hd_cnt > 0);
    channel_cnt = DIV_ROUND_UP(hd_cnt, 2);  //One channel has two disks

    struct ide_channel* channel;
    uint8_t channel_no = 0;
    uint8_t dev_no = 0;

    /*Process channel and each channel's disk.*/
    while(channel_no < channel_cnt) {
        channel = &channels[channel_no];
        sprintf(channel->name, "ide%d", channel_no);  //Write the channel's name to channel->name.

        /*Initialize port base address and interrupt vector for each ide channel*/
        switch (channel_no) {
            case 0:
                channel->port_base = 0x1f0;
                channel->irq_no = 0x20 + 14;
                break;
            case 1:
                channel->port_base = 0x170;
                channel->irq_no = 0x20 + 15;  //Interrupt number is 0x2f, it mapping The last pin in 8259A's slaver piece.
                break;
        }

        register_handler(channel->irq_no, intr_hd_handler);  //Register the handler in idt_table
        
        channel->expecting_intr = false;  //It is false when we don't write commend to disk.
        lock_init(&channel->lock);  //Initialize the lock of the channel

        /*Used to block or wake up the process.*/
        sema_init(&channel->disk_done, 1);  //Initialize the semaphore of the channel, the initial value is 1.

        list_init(&partition_list);  //The author is forget to initialize the partition_list, so, we initialize it here.

        /*Process all disk in each channel*/
        while(dev_no < 2) {
            struct disk* hd = &channel->devices[dev_no];
            hd->my_channel = channel;
            hd->dev_no = dev_no;  //Master is 0 and is slaver 1
            sprintf(hd->name, "sd%c", 'a' + channel_no * 2 + dev_no);  //Write the disk's name to disk->name.

            identify_disk(hd);  //Get the disk's parameters
            
            if(dev_no != 0) {  //we just have two disk, hd60M.img(sda) and hd80M.img(sdb),and hd60M.img(sda) don't have patitions.
                partition_scan(hd, 0);
            }
            p_no = 0, l_no = 0;
            dev_no++;
        }
        dev_no = 0;

        channel_no++;
    }

    printk("\n  all partition info\n");

    /*Print all partition's information*/
    list_traversal(&partition_list, partition_info, (int)NULL);
    printk("ide_init done\n");
}