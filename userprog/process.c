#include "process.h"
#include "thread.h"
#include "global.h"
#include "stdint.h"
#include "memory.h"
#include "kernel.h"
#include "tss.h"
#include "console.h"
#include "string.h"
#include "interrupt.h"
#include "debug.h"
#include "list.h"

extern void intr_exit(void);

/*Building User-process' initialize messages.*/
void start_process(void* filename_) {
    void* function = filename_;  //Address of User program.
    struct task_struct* cur = running_thread();
    cur->self_kstack += sizeof(struct thread_stack);  //self_kstack point to the top of kthread_stack before.
    struct intr_stack* proc_stack = (struct intr_stack*)cur->self_kstack;  //use intr_stack sapce to store the User-program's information when first enter the User-program temporarily.
    proc_stack->edi = proc_stack->esi = proc_stack->ebp = proc_stack->esp_dummy = 0;
    proc_stack->ebx = proc_stack->edx = proc_stack->ecx = proc_stack->eax = 0;

    proc_stack->gs = 0;
    proc_stack->ds = proc_stack->es = proc_stack->fs = SELECTOR_U_DATA;
    proc_stack->eip = function;  //the address of User-program which will be executed.
    proc_stack->cs = SELECTOR_U_CODE;
    proc_stack->eflags = (EFLAGS_IOPL_0 | EFLAGS_MBS | EFLAGS_IF_1);
    proc_stack->esp = (void*)((uint32_t)get_a_page(PF_USER, USER_STACK3_VADDR) + PG_SIZE);
    proc_stack->ss = SELECTOR_U_DATA;
    asm volatile("movl %0, %%esp; jmp intr_exit" : : "g"(proc_stack) : "memory");  //enter to the User's space which privilege level is 3.
}

/*Enable the page table.*/
void page_dir_activate(struct task_struct* p_thread) {
    /*If the current task is a kernel thread, then the PDT is the same as the kernel's PDT,else,we should change CR3's value.*/
    uint32_t pagedir_phy_addr = 0x100000;
    if (p_thread->pgdir != NULL) { //jodge whether the current task is a kernel thread or a user process.
        pagedir_phy_addr = addr_v2p((uint32_t)p_thread->pgdir); //translate PDT's virtual address to physical address.
    }

    /*Reloade the CR3.*/
    asm volatile("movl %0, %%cr3" : : "r"(pagedir_phy_addr) : "memory");
}

/*
Enable the task.
1. Enable the page table.
2. Update the TSS's ESP0 to the current process' kernel stack.
*/
void process_activate(struct task_struct* p_thread) {
    ASSERT(p_thread != NULL);
    page_dir_activate(p_thread); //*Enable the page table.
    if (p_thread->pgdir != NULL) {
        update_tss_esp0(p_thread);
    }
}

uint32_t* create_page_dir(void) {
    /*User can't visit User process' PDT directly,so,we need create PDT in kernel memory pool.*/
    uint32_t* page_dir_vaddr = get_kernel_pages(1);
    if (page_dir_vaddr == NULL) {
        console_put_str("create_page_dir: get_kernel_page failed!");
        return NULL;
    }

    /*Copy the PDEs which are point to the kernel to the new User-process' PDT.*/
    /*768th and above PDE all default point to kernel, (1023 - 768 + 1) * 1024 * 4KB = 1GB .*/
    memcpy((uint32_t*)((uint32_t)page_dir_vaddr + 0x300 * 4), (uint32_t*)(0xfffff000 + 0x300 * 4), 1024);  //(1023 - 768 + 1) * 4 = 1024, make sure all User-process can share the kernel.
    /*Update the page directory's physical address.*/
    uint32_t new_page_dir_phy_addr = addr_v2p((uint32_t)page_dir_vaddr);

    /*The 1023th PDE store the physic address of PDT.*/
    page_dir_vaddr[1023] = new_page_dir_phy_addr | PG_US_U | PG_RW_W | PG_P_1;

    return page_dir_vaddr;
}

void create_user_vaddr_bitmap(struct task_struct* user_prog) {
    user_prog->userprog_vaddr.vaddr_start = USER_VADDR_START;  //userprog_vaddr is a virturl address pool manager, it include an bitmap and vaddr_start.
    uint32_t bitmap_pg_cnt = DIV_ROUND_UP((0xc0000000 - USER_VADDR_START) / PG_SIZE / 8, PG_SIZE);  //!pay attention! The number of pages which are used to store the bitmap(Round up).
    user_prog->userprog_vaddr.vaddr_bitmap.bits = get_kernel_pages(bitmap_pg_cnt);  //get_kernel_pages()'s return type is an void*, and bits type is uint8_t*.
    user_prog->userprog_vaddr.vaddr_bitmap.btmp_bytes_len = (0xc0000000 - USER_VADDR_START) / PG_SIZE / 8;
    bitmap_init(&user_prog->userprog_vaddr.vaddr_bitmap);
}