#include "thread.h"
#include "stdint.h"
#include "string.h"
#include "global.h"
#include "memory.h"

#define PG_SIZE 4096

static void kernel_thread(thread_func* function, void* func_arg) {
    function(func_arg);
}

/*initlalize thread_stack*/
void thread_create(struct task_struct* pthread,thread_func function, void* func_arg) { //task_struct: struct of PCB
    /*reserved space of intr_stack*/
    pthread->self_kstack -= sizeof(struct intr_stack);
    /*reserved space of thread_stack*/
    pthread->self_kstack -= sizeof(struct thread_stack);
    struct thread_stack* kthread_stack = (struct thread_stack*)pthread->self_kstack;  //at this time, self_kstack(also is kthread_stack) is pointed to the structure of thread_stack.
    kthread_stack->eip = kernel_thread;
    kthread_stack->function = function;
    kthread_stack->func_arg = func_arg;
    kthread_stack->ebp = kthread_stack->ebx = kthread_stack->esi = kthread_stack->edi = 0;
}

/*initialize the base information of thread.
you can say initialize PCB*/
void init_thread(struct task_struct* pthread, char* name, int prio) {
    memset(pthread, 0, sizeof(*pthread));
    strcpy(pthread->name, name);
    pthread->status = TASK_RUNNING;
    pthread->priority = prio;

    pthread->self_kstack = (uint32_t*)((uint32_t)pthread + PG_SIZE);
    pthread->stack_magic = 0x12345678;
}

/*create an thread which priority is prio,name is name.*/
struct task_struct* thread_start(char* name, int prio, thread_func function, void* func_arg) {
    struct task_struct* thread = get_kernel_pages(1);  //use one page to create an PCB
    init_thread(thread, name, prio); //initialize PCB by base information
    thread_create(thread, function, func_arg); //initialize thread_stack in PCB

    asm volatile ("movl %0, %%esp; pop %%ebp; pop %%ebx; pop %%edi; pop %%esi; ret": : "g"(thread->self_kstack) : "memory"); //self_kstack is pointed to the TOP of PCB(also is botton of intr_stack.)
    return thread;
}