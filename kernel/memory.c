#include "memory.h"
#include "stdint.h"
#include "print.h"
#include "debug.h"
#include "string.h"
#include "bitmap.h"
#include "sync.h"
#include "thread.h"
#include "interrupt.h"
#include "list.h"

/*
The kernel stack's bottom is 0xc009f000， 0xc009e000 is kernel main thread's pcb.
one page can present 128MB memory(4KB * 1024 * 8 * 4KB),so, we put bitmap to address of 0xc009a000,
so,our system can offer 4(0x9b000-0x9a000 = 2^12 = 4KB) page of bitmap,these can indicate 512MB memory.
*/
#define MEM_BITMAP_BASE 0xc009a000
/*-------------------------------------------------*/
#define K_HEAP_START 0xc0100000  //pay attention: PAGE_DIR_TABLE_POS equ 0x100000 is an physic address and this is virtual address.
#define PDE_IDX(addr) ((addr & 0xffc00000) >> 22)  //get high 10 bit of 32 address
#define PTE_IDX(addr) ((addr & 0x003ff000) >> 12)  //get mid 10 bit of 32 address

/*The struct of memory pool, it will be used to create two object to manage kernel memory pool and user memory pool.*/
struct pool {   //used to manage physic/virtual memory.
    struct bitmap pool_bitmap;
    uint32_t phy_addr_start;
    uint32_t pool_size;
    struct lock lock;
};

/*Memory warehouse*/
/*
In the future, we will give more than one page frame to one arena's struct-pointer, and it's Yuan information will use 12Byte to record.
*/
struct arena {
    struct mem_block_desc* desc;  //This arena belongs to which memory block descriptor.
    uint32_t cnt;  //large = false: The number of free memory block in this arena, large = true: The number of page frame, we will use it when we free the memory in future.
    bool large;
};

/*The more index larger, the more bigger block size is.*/
struct mem_block_desc k_block_descs[DESC_CNT];  //This is kernel memory block descriptor array, u_block_descs will be define in PCB.

struct pool kernel_pool, user_pool; //manage physic
struct virtual_addr kernel_vaddr;   //manage virtual

/*applicate virtual page in pf(pool flag) with quantity of pg_cnt,if successful ,return the start address of virtual page,if fail,return NULL*/
static void* vaddr_get(enum pool_flags pf, uint32_t pg_cnt) {
    int vaddr_start = 0, bit_idx_start = -1;  //bit_idx is the index in bitmap.
    uint32_t cnt = 0;
    if(pf == PF_KERNEL) {
        bit_idx_start = bitmap_scan(&kernel_vaddr.vaddr_bitmap, pg_cnt);  //pay attention: bitmap_scan()'s parameter is an bitmap structure's address,not an bitmap's address.
        if(bit_idx_start == -1) {
            return NULL;
        }
        while(cnt < pg_cnt) {
            bitmap_set(&kernel_vaddr.vaddr_bitmap, bit_idx_start + cnt++, 1); //indicate these pages have be used.
        }
        vaddr_start = kernel_vaddr.vaddr_start + bit_idx_start * PG_SIZE;
    }
    else {
        // User memory pool
        struct task_struct* cur = running_thread();
        bit_idx_start = bitmap_scan(&cur->userprog_vaddr.vaddr_bitmap, pg_cnt);  //pay attention: bitmap_scan()'s parameter is an bitmap structure's address,not an bitmap's address.
        if(bit_idx_start == -1) {
            return NULL;
        }
        while(cnt < pg_cnt) {
            bitmap_set(&cur->userprog_vaddr.vaddr_bitmap, bit_idx_start + cnt++, 1); //indicate these pages have be used.
        }
        vaddr_start = cur->userprog_vaddr.vaddr_start + bit_idx_start * PG_SIZE;
        /*(0xc0000000 - PG_SIZE) has be distributed to the stack of level 3 by start_process() function.*/
        ASSERT((uint32_t)vaddr_start < (0xc0000000 - PG_SIZE));
    }
    return (void*)vaddr_start;
}

/*get the pte pointer of virtual address "vaddr"*/
/*remember!! pointer's address is vritual address*/
/*
remember!! pte_ptr() and pde_ptr() don't care whether pte and pde exist, they just caculate virtual address of pte and pde which are related with vaddr
******************************these two function are the key of edit page table.**************************
*/
uint32_t* pte_ptr(uint32_t vaddr) {
    /*0xffc00000: this is virtual address, and it is point to last PDE, last PDE point to PDT's physic address*/
    uint32_t* pte = (uint32_t*)(0xffc00000 + ((vaddr & 0xffc00000) >> 10) + PTE_IDX(vaddr) * 4);
    return pte;
}

/*get the pde pointer of virtual address "vaddr"*/
uint32_t* pde_ptr(uint32_t vaddr) {
    uint32_t* pde = (uint32_t*)((0xfffff000) + PDE_IDX(vaddr) * 4);
    return pde;
}
/******************************************************************************************* */

/*destribute one physic page from physic memory pool which is pointed by m_pool*/
/*if success, return physic address of page,if fail,return NULL.*/
static void* palloc(struct pool* m_pool) {
    int bit_idx = bitmap_scan(&m_pool->pool_bitmap, 1);
    if(bit_idx == -1) {
        return NULL;
    }
    bitmap_set(&m_pool->pool_bitmap, bit_idx, 1);
    uint32_t page_phyaddr = ((bit_idx * PG_SIZE) + m_pool->phy_addr_start);
    return (void*)page_phyaddr;
}

/*add mapping from _vaddr to _page_phyaddr in page table.*/
static void page_table_add(void* _vaddr, void* _page_phyaddr) {
    uint32_t vaddr = (uint32_t)_vaddr, page_phyaddr = (uint32_t)_page_phyaddr;
    uint32_t* pde = pde_ptr(vaddr);
    uint32_t* pte = pte_ptr(vaddr);

/**************************************8pay attention!!*******************************/
/*if we want execute *pte, we must make sure we have finished create pde,if we don's create pde and execute *pte, we will make page_fault*/
    if(*pde & 0x00000001) {  //the attribute of "P",it indicate whether this pte exist.
        //ASSERT(!(*pte & 0x00000001));  //if we want create pte,so,it's "P" must 0.
        if(!(*pte & 0x00000001)) {  //check more times to make sure safety.
            *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);  //structure of pte.
        }
        else {
            PANIC("pte repeat");
            *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
        }
    }
    else { //the page space of PT all be dectributed from kernel space.
        uint32_t pde_phyaddr = (uint32_t)palloc(&kernel_pool);  //apply for a page space from kernel pool to create PT.
        *pde = (pde_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
        memset((void*)((int)pte & 0xfffff000), 0, PG_SIZE);   //clean the page space of pde_phyaddr

        ASSERT(!(*pte & 0x00000001));
        *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
    }
}

/*destribute page space and the quantities are pg_cnt.If success,return start virtual address,else return NULL.*/
void* malloc_page(enum pool_flags pf, uint32_t pg_cnt) {
    ASSERT(pg_cnt > 0 && pg_cnt < 3840);  //3840： kernel or user pyhsic space have about 16MB,we use 15MB to limit. pg_cnt < 15 * 1024 * 1024 / 4096(PG_SIZE) = 3840 (page)
/*************************the principle of malloc_page()***************************/
/*      First: use vaddr_get() to apply vaitual address from virtual memory pool.
        Second: use palloc() to apply physic page from physic memory pool.
        Thired: use page_table_add() to finish mapping from virtual address to physic address in PT.
*/
    void* vaddr_start = vaddr_get(pf, pg_cnt);
    if(vaddr_start == NULL) {
        return NULL;
    }

    uint32_t vaddr = (uint32_t)vaddr_start, cnt = pg_cnt;
    struct pool* mem_pool = pf & PF_KERNEL ? &kernel_pool : &user_pool;  //make sure to use which type of pool to destribute physic page,this pointer(mem_pool) point to pool structure.

    /*virtual address is continuous,but physic address could not be continuous,so we divide these three function from one cycle,
    and we can apply more virtual page onetime instead of applying virtual page one by one,
    if we failed to apply virtual page,we can decrease steps to apply physic page.*/
    while(cnt-- > 0) {
        void* page_phyaddr = palloc(mem_pool);
        if(page_phyaddr == NULL) {
            //if failed, we must rollback all address(virtual/physic page) which are applied, we will achieve it in the future.
            return NULL;
        }
        page_table_add((void*)vaddr, page_phyaddr);  //make mapping in PT.
        vaddr += PG_SIZE;    //next virtual page.
    }
    return vaddr_start;
}

/*apply one page memory from virtual kernel memory pool, if success, return virtual address,else,return NULL.*/
void* get_kernel_pages(uint32_t pg_cnt) {
    void* vaddr = malloc_page(PF_KERNEL, pg_cnt);
    if(vaddr != NULL) {
        memset(vaddr, 0, pg_cnt * PG_SIZE);  //clean this memory space which you have applied.
    }
    return vaddr;
}

/*apply one page memory from virtual user's memory pool, if success, return virtual address,else,return NULL.*/
void* get_user_pages(uint32_t pg_cnt) {
    lock_acquire(&user_pool.lock);
    void* vaddr = malloc_page(PF_USER, pg_cnt);
    if(vaddr != NULL) {
        memset(vaddr, 0, pg_cnt * PG_SIZE);  //clean this memory space which you have applied.
    }
    lock_release(&user_pool.lock);
    return vaddr;
}

/*get_a_page(enum pool_flags pf, uint32_t vaddr) mapping vaddr and paddr in pf autonomously, and only support one physic page's distribution.
get_user_pages(uint32_t pg_cnt) and get_user_pages(uint32_t pg_cnt) automatically designate the vaddr and mapping it to a paddr.
*/
void* get_a_page(enum pool_flags pf, uint32_t vaddr) {
    struct pool* mem_pool = pf & PF_KERNEL ? &kernel_pool : &user_pool;  //physic memory pool's manmger.
    lock_acquire(&mem_pool->lock);

    struct task_struct* cur = running_thread();
    int32_t bit_idx = -1;

    if(cur->pgdir != NULL && pf == PF_USER) { //User's process apply memory.
        bit_idx = (vaddr - cur->userprog_vaddr.vaddr_start) / PG_SIZE;
        ASSERT(bit_idx > 0);
        bitmap_set(&cur->userprog_vaddr.vaddr_bitmap, bit_idx, 1);
    }
    else if(cur->pgdir == NULL && pf == PF_KERNEL) { //Kernel's thread apply memory.
        bit_idx = (vaddr - kernel_vaddr.vaddr_start) / PG_SIZE;
        ASSERT(bit_idx > 0);
        bitmap_set(&kernel_vaddr.vaddr_bitmap, bit_idx, 1);
    }
    else {
        PANIC("get_a_page: not allow kernel alloc userspace or user alloc kernelspace by get_a_page.");
    }

    void* page_phyaddr = palloc(mem_pool);
    if(page_phyaddr == NULL) {
        return NULL;
    }
    page_table_add((void*)vaddr, page_phyaddr);
    lock_release(&mem_pool->lock);
    return (void*)vaddr;
}

/*Get the physic address which is mapped with designate virtual address.*/
uint32_t addr_v2p(uint32_t vaddr) {
    uint32_t* pte = pte_ptr(vaddr);
    return ((*pte & 0xfffff000) + (vaddr & 0x00000fff));
}

/*initialize memory pool and ralated structures*/
static void mem_pool_init(uint32_t all_mem) {
    put_str("mem_pool_init start\n");
    uint32_t page_table_size = PG_SIZE * 256;  //used to record the Byte's quantity of (PDT add PT). The PDT's 769~1022(kernel space) totally have 254 PDE,these point to 254 page,0 and 769 point to same one page(this page point to low 1MB physic memory),1023 point to PDT,so,totally have 256 page,these page have be occupied.
    uint32_t used_mem = page_table_size + 0x100000;  //0x100000 is low 1MB physic memory.
    uint32_t free_mem = all_mem - used_mem;
    uint16_t all_free_page = free_mem / PG_SIZE;
    uint16_t kernel_free_pages = all_free_page / 2;
    uint16_t user_free_pages = all_free_page - kernel_free_pages;

    uint32_t kbm_length = kernel_free_pages / 8;   //length of kernel Bitmap.
    uint32_t ubm_length = user_free_pages / 8;

    uint32_t kp_start = used_mem;  //The low 1MB + PDT + PT are continuous in memory,so,this is kernel pool start address.
    uint32_t up_start = kp_start + kernel_free_pages * PG_SIZE;

    kernel_pool.phy_addr_start = kp_start;
    user_pool.phy_addr_start = up_start;

    kernel_pool.pool_size = kernel_free_pages * PG_SIZE;
    user_pool.pool_size = user_free_pages * PG_SIZE;

    kernel_pool.pool_bitmap.btmp_bytes_len = kbm_length;
    user_pool.pool_bitmap.btmp_bytes_len = ubm_length;

    kernel_pool.pool_bitmap.bits = (void*)MEM_BITMAP_BASE;
    user_pool.pool_bitmap.bits = (void*)(MEM_BITMAP_BASE + kbm_length);

    lock_init(&kernel_pool.lock);
    lock_init(&user_pool.lock);

    put_str("kernel_pool_bitmap_start: ");
    put_int((int)kernel_pool.pool_bitmap.bits);
    put_str("  kernel_pool_phy_addr_start: ");
    put_int(kernel_pool.phy_addr_start);
    put_str("\n");
    put_str("user_pool_bitmap_start: ");
    put_int((int)user_pool.pool_bitmap.bits);
    put_str("  user_pool_phy_addr_start: ");
    put_int(user_pool.phy_addr_start);
    put_str("\n");

    /*set bitmap to 0*/
    bitmap_init(&kernel_pool.pool_bitmap);
    bitmap_init(&user_pool.pool_bitmap);

    /*initialize kernel virtual address's bitmap,because this is used to control physic-kernel-heap,so, it's size is same to kernel memory pool.*/
    kernel_vaddr.vaddr_bitmap.btmp_bytes_len = kbm_length;
    kernel_vaddr.vaddr_bitmap.bits = (void*)(MEM_BITMAP_BASE + kbm_length + ubm_length);
    put_str("kernel_vaddr_bitmap_start: ");
    put_int((int)kernel_vaddr.vaddr_bitmap.bits);

    kernel_vaddr.vaddr_start = K_HEAP_START;
    put_str("  kernel_vaddr_start: ");
    put_int(K_HEAP_START);
    put_str("\n");
    bitmap_init(&kernel_vaddr.vaddr_bitmap);
    put_str("mem_pool_init done\n");
}

/*Prepare for malloc*/
void block_desc_init(struct mem_block_desc* desc_array) {
    uint16_t desc_idx, block_size = 16;
    for(desc_idx = 0; desc_idx < DESC_CNT; desc_idx++) {
        desc_array[desc_idx].block_size = block_size;
        desc_array[desc_idx].blocks_per_arena = (PG_SIZE - sizeof(struct arena)) / block_size;
        list_init(&desc_array[desc_idx].free_list);
        block_size *= 2;  //Update to the next block size.
    }
}

/*Return the address of 'NO. idx' memory block from arena*/
static struct mem_block* arena2block(struct arena* a, uint32_t idx) {
    return (struct mem_block*)((uint32_t)a + sizeof(struct arena) + idx * a->desc->block_size);
}

/*Return the address of arena which has the block of 'b'*/
static struct arena* block2arena(struct mem_block* b) {
    return (struct arena*)((uint32_t)b & 0xfffff000);   //arena's unit is page(4KB).
}

/*Apply memory of 'size' size in heap.*/
void* sys_malloc(uint32_t size) {
    //Preparation
    enum pool_flags PF;
    struct pool* mem_pool;
    uint32_t pool_size;
    struct mem_block_desc* descs;
    struct task_struct* cur_thread = running_thread();

    /*Judge to use whcih memory-pool.*/
    if(cur_thread->pgdir == NULL) {  //kernel thread
        PF = PF_KERNEL;
        pool_size = kernel_pool.pool_size;
        mem_pool = &kernel_pool;
        descs = k_block_descs;
    }
    else {  //user process
        PF = PF_USER;
        pool_size = user_pool.pool_size;
        mem_pool = &user_pool;
        descs = cur_thread->u_block_descs;
    }

    if(!(size > 0 && size < pool_size)) {  //size must be in the range of (0, pool_size).
        return NULL;
    }
    struct arena* a;
    struct mem_block* b;
    lock_acquire(&mem_pool->lock);

    /*Condition One: The size of applying is more than 1024*/
    if(size > 1024) {
        uint32_t page_cnt = DIV_ROUND_UP(size + sizeof(struct arena), PG_SIZE);  //page_cnt is the number of page which we need to apply.

        a = malloc_page(PF, page_cnt);  //apply page.

        if(a != NULL) {
            memset(a, 0, page_cnt * PG_SIZE);  //clean this memory space which you have applied.
            /*For large page frame, the desc is NULL, and large is true, the cnt is the number of page frame.*/
            a->desc = NULL;
            a->cnt = page_cnt;
            a->large = true;
            lock_release(&mem_pool->lock);
            return (void*)(a + 1);  //skip the size of arena return the remainder memory.
        }
        else {
            lock_release(&mem_pool->lock);
            return NULL;
        }
    }
    
    /*Condition Two: The size of applying is less than 1024*/
    else {
        uint8_t desc_idx;
        /*Match the suitable size of memory block in memory-block-descriptor.*/
        for(desc_idx = 0; desc_idx < DESC_CNT; desc_idx++) {  //The parameter of desc_idx will be changed.
            if(size <= descs[desc_idx].block_size) {
                break;
            }
        }
        /*if mem_block_desc's free_list don't have mem_block to use, then, create new arena to provide mem_block.*/
        if(list_empty(&descs[desc_idx].free_list) == true) {
            a = malloc_page(PF, 1);  //apply page from virtual memory pool to be new arena.
            if(a == NULL) {
                lock_release(&mem_pool->lock);
                return NULL;
            }
            memset(a, 0, PG_SIZE);
            /*For little memory block, the desc is the corresponding mem_block_desc, and large is false, the cnt is the number of blocks in one arena.*/
            a->desc = &descs[desc_idx];
            a->large = false;
            a->cnt = descs[desc_idx].blocks_per_arena;
            uint32_t block_idx;           
            enum intr_status old_status = intr_disable();

            /*Begin to split the arena to little mem_block, and add them to free_list in mem_block_desc.*/             
            for(block_idx = 0; block_idx < descs[desc_idx].blocks_per_arena; block_idx++) {
                b = arena2block(a, block_idx);
                ASSERT(!elem_find(&a->desc->free_list, &b->free_elem));
                list_append(&a->desc->free_list, &b->free_elem);
            }
            intr_set_status(old_status);
        }

        /*Begin to distribute mem_block.*/
        /*Get one memory block from free_list in mem_block_desc.*/
        b = elem2entry(struct mem_block, free_elem, list_pop(&(descs[desc_idx].free_list)));
        memset(b, 0, descs[desc_idx].block_size);

        a = block2arena(b);   //Get the arena which has the block of 'b'.
        a->cnt--;  //The number of free memory block in this arena will decrease one.
        lock_release(&mem_pool->lock);
        return (void*)b;
    }
}

/*Recycle the physic address of pg_phy_addr to physic memory pool.*/
/*Physic pool has two type: kernel physic pool; user physic pool.*/
void pfree(uint32_t pg_phy_addr) {
    struct pool* mem_pool;
    uint32_t bit_idx = 0;
    if(pg_phy_addr >= user_pool.phy_addr_start) {  //User's physic memory pool.
        mem_pool = &user_pool;
        bit_idx = (pg_phy_addr - user_pool.phy_addr_start) / PG_SIZE;
    }
    else {  //Kernel's physic memory pool.
        mem_pool = &kernel_pool;
        bit_idx = (pg_phy_addr - kernel_pool.phy_addr_start) / PG_SIZE;
    }
    bitmap_set(&mem_pool->pool_bitmap, bit_idx, 0);  //set the corresponding bit to 0.
}

/*Cancle the mapping from PTE to physic address in PT(don't change PDE)*/
static void page_table_pte_remove(uint32_t vaddr) {
    uint32_t* pte = pte_ptr(vaddr);
    *pte &= ~PG_P_1;  //set the 'P' attribute bit to 0,"~" is set 1 to 0.
    asm volatile ("invlpg %0"::"m"(vaddr):"memory");  //update TLB,"m": virtual address memory.
}

/*Remove virtual address*/
static void vaddr_remove(enum pool_flags pf, void* _vaddr, uint32_t pg_cnt) {
    uint32_t bit_idx_start = 0, vaddr = (uint32_t)_vaddr, cnt = 0;

    if(pf == PF_KERNEL) {  //Kernel's virtual memory pool
        bit_idx_start = (vaddr - kernel_vaddr.vaddr_start) / PG_SIZE;
        while(cnt < pg_cnt) {
            bitmap_set(&kernel_vaddr.vaddr_bitmap, bit_idx_start + cnt++, 0);
        }
    }
    else {  //User's virtual memory pool
        struct task_struct* cur = running_thread();
        bit_idx_start = (vaddr - cur->userprog_vaddr.vaddr_start) / PG_SIZE;
        while(cnt < pg_cnt) {
            bitmap_set(&cur->userprog_vaddr.vaddr_bitmap, bit_idx_start + cnt++, 0);
        }
    }
}

/*Encapsulation: Remove physic page frame*/
void mfree_page(enum pool_flags pf, void* _vaddr, uint32_t pg_cnt) {
    uint32_t pg_phy_addr;
    uint32_t vaddr = (uint32_t)_vaddr, page_cnt = 0;
    ASSERT(pg_cnt >= 1 && vaddr % PG_SIZE == 0);
    pg_phy_addr = addr_v2p(vaddr);  //get the physic address of vaddr.

    /*Make sure the physic address which will be released is at low-1MB + 1KB PDT + 1KB PT outside.*/
    ASSERT((pg_phy_addr % PG_SIZE) == 0 && pg_phy_addr >= 0x102000);
    
    if(pg_phy_addr >= user_pool.phy_addr_start) {  //User's physic memory pool.
        vaddr -= PG_SIZE;
        while(page_cnt < pg_cnt) {
            vaddr += PG_SIZE;
            pg_phy_addr = addr_v2p(vaddr);

            ASSERT((pg_phy_addr % PG_SIZE) == 0 && pg_phy_addr >= user_pool.phy_addr_start);
            /*First: Recycle the physic address of pg_phy_addr to physic memory pool.*/
            pfree(pg_phy_addr);
            /*Second: Cancle the mapping from PTE to physic address in PT(don't change PDE)*/
            page_table_pte_remove(vaddr);

            page_cnt++;
        }
        /*Third: Remove virtual address*/
        vaddr_remove(pf, _vaddr, pg_cnt);
    }
    else {  //Kernel's physic memory pool.
        vaddr -= PG_SIZE;
        while(page_cnt < pg_cnt) {
            vaddr += PG_SIZE;
            pg_phy_addr = addr_v2p(vaddr);

            ASSERT((pg_phy_addr % PG_SIZE) == 0 && pg_phy_addr >= kernel_pool.phy_addr_start && pg_phy_addr < user_pool.phy_addr_start);

            /*First: Recycle the physic address of pg_phy_addr to physic memory pool.*/
            pfree(pg_phy_addr);
            /*Second: Cancle the mapping from PTE to physic address in PT(don't change PDE)*/
            page_table_pte_remove(vaddr);

            page_cnt++;
        }
        /*Third: Remove virtual address*/
        vaddr_remove(pf, _vaddr, pg_cnt);
    }
}

/*Recycle the memory which is pointed by ptr*/
void sys_free(void* ptr) {
    ASSERT(ptr != NULL);
    if(ptr != NULL) {
        enum pool_flags PF;
        struct pool* mem_pool;

        /*Judge whether is thread or process.*/
        if(running_thread()->pgdir == NULL) {  //kernel thread
            ASSERT((uint32_t)ptr >= K_HEAP_START);
            PF = PF_KERNEL;
            mem_pool = &kernel_pool;
        }
        else {  //user process
            PF = PF_USER;
            mem_pool = &user_pool;
        }

        lock_acquire(&mem_pool->lock);
        struct mem_block* b = ptr;
        struct arena* a = block2arena(b);

        ASSERT(a->large == false || a->large == true);
        if(a->desc == NULL && a->large == true) {  //large memory block(more than 1024Byte)
            mfree_page(PF, a, a->cnt);
        }
        else {  //little memory block(less than 1024Byte)
            /*First: Add the memory block to free_list in mem_block_desc.*/
            list_append(&a->desc->free_list, &b->free_elem);
            a->cnt++;
            
            /*Second: If all memory block in this arena are free, then, release this arena.*/
            if(a->cnt == a->desc->blocks_per_arena) {  //it means all memory block in this arena are free.
                uint32_t block_idx;
                for(block_idx = 0; block_idx < a->desc->blocks_per_arena; block_idx++) {
                    struct mem_block* b = arena2block(a, block_idx);
                    ASSERT(elem_find(&a->desc->free_list, &b->free_elem));
                    list_remove(&b->free_elem);
                }
                mfree_page(PF, a, 1);  //release this arena.
            }
        }
        lock_release(&mem_pool->lock);
    }
}

/*memory manage port's initial entry*/
void mem_init() {
    put_str("mem_init start\n");
    uint32_t mem_bytes_total = (*(uint32_t*)(0xb00));  //0xb00: total_mem_bytes's address ,this value's defination can see loader.S
    mem_pool_init(mem_bytes_total);  //initialize memory pool.
    /*Initialize the array of mem_block_desc(is k_block_descs), prepare for malloc*/
    block_desc_init(k_block_descs);
    put_str("mem_init done\n");
}
