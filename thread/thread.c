#include "thread.h"
#include "stdint.h"
#include "string.h"
#include "global.h"
#include "memory.h"
#include "list.h"
#include "interrupt.h"
#include "print.h"
#include "debug.h"
#include "process.h"
#include "sync.h"

#define PG_SIZE 4096

struct task_struct* idle_thread;  //idle thread

struct task_struct* main_thread;  //main thread's PCB
struct list thread_ready_list;  //the list of ready, ready queue.
struct list thread_all_list;   //the list of all task,it is a list include all thread we have created.
/*we use PCB to manage thread, so we need to translate thread's tag to thread's PCB.*/
static struct list_elem* thread_tag;  //Be used to store thread's tag when we want to translate. 

struct lock pid_lock;  //lock of pid

extern void switch_to(struct task_struct* cur, struct task_struct* next);

/*The thread whcih is running when system's status is free*/
static void idle(void* arg) {
    UNUSED(arg);
    while(1) {
        thread_block(TASK_BLOCKED);
        //sti: open interrupt, hlt: close CPU(Only outside interrupt can wake up CPU)
        asm volatile ("sti; hlt" : : : "memory");  //musk make surn interrupt is open when we execute 'halt'.
    }
}

/*get the PCB pointer of current thread*/
struct task_struct* running_thread() {
    uint32_t esp;
    asm ("mov %%esp, %0" : "=g"(esp));
    /*get the start address of PCB*/
    return (struct task_struct*)(esp & 0xfffff000);
}

/*destribute the pid.*/
static pid_t allocate_pid(void) {
    static pid_t next_pid = 0;
    lock_acquire(&pid_lock);
    next_pid++;
    lock_release(&pid_lock);
    return next_pid;
}

static void kernel_thread(thread_func* function, void* func_arg) {
    /*after enter interrupt, CPU will close interrupt automatically,
    but our task's schedule is based on clock interrupt,so we need enable interrupt.
    */
    intr_enable();  //set IF bit to 1,CPU begin to receive clock interrupt.
    function(func_arg);
}

/*initlalize thread_stack*/
void thread_create(struct task_struct* pthread, thread_func* function, void* func_arg) { //task_struct: struct of PCB
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
    memset(pthread, 0, sizeof(*pthread));  //the value of sizeof(*pthread) is just a structure's size of task_struct instead of a page's size, so don't warry about return address.

    pthread->pid = allocate_pid();
    pthread->self_kstack = (uint32_t*)((uint32_t)pthread + PG_SIZE);
    if(pthread == main_thread) {
        pthread->status = TASK_RUNNING;
    }
    else {
        pthread->status = TASK_READY;
    }
    strcpy(pthread->name, name);
    pthread->priority = prio;
    pthread->ticks = prio;
    pthread->elapsed_ticks = 0;
    pthread->pgdir = NULL;
    pthread->stack_magic = 0x12345678;
}

/*The entry of creating thread: create an thread which priority is prio,name is name.*/
struct task_struct* thread_start(char* name, int prio, thread_func function, void* func_arg) {
    struct task_struct* thread = get_kernel_pages(1);  //use one page to create an PCB
    init_thread(thread, name, prio); //initialize PCB by base information
    thread_create(thread, function, func_arg); //initialize thread_stack in PCB

/*Pay attention: we use general_tag to represent PCB in thread_ready_list*/
    ASSERT(!elem_find(&thread_ready_list, &thread->general_tag));
    list_append(&thread_ready_list, &thread->general_tag);
    ASSERT(elem_find(&thread_ready_list, &thread->general_tag)); //locate the mistake
    ASSERT(!elem_find(&thread_all_list, &thread->all_list_tag));
    list_append(&thread_all_list, &thread->all_list_tag);

    return thread;
}

/*give main()(is main thread^^) an identity card(yes, is PCB)*/
static void make_main_thread(void) {
    /*when we enter kernel, we make esp to 0xc009f000,
    it point to the botton of stack in PCB which is used by main_thread, 
    actually, we have already reserved main_thread's PCB in 0xc009e000, 
    so, we don't need to use get_kernel_page() function to create PCB.*/
    main_thread = running_thread();
    init_thread(main_thread, "main", 31);

    /*this thread is already running, so we don't need to put it to thread_ready_list.*/
    ASSERT(!elem_find(&thread_all_list, &main_thread->all_list_tag));
    list_append(&thread_all_list, &main_thread->all_list_tag);
}

void schedule() {
    //replace current thread
    ASSERT(intr_get_status() == INTR_OFF);

    struct task_struct* cur = running_thread();
    if(cur->status == TASK_RUNNING) {
        ASSERT(!elem_find(&thread_ready_list, &cur->general_tag));
        list_append(&thread_ready_list, &cur->general_tag);
        cur->ticks = cur->priority;
        cur->status = TASK_READY;
    }
    else {
        //temporary empty
    }

    //change new thread which is the first element in thread_ready_list.
    if(list_empty(&thread_ready_list)) {
        thread_unblock(idle_thread);
    }
    thread_tag = NULL;
    thread_tag = list_pop(&thread_ready_list);
    struct task_struct* next = elem2entry(struct task_struct, general_tag, thread_tag); //general_tag is the name of member.
    next->status = TASK_RUNNING;

    /*Enable the task's page table and so on.*/
    process_activate(next);
    
    switch_to(cur, next);
}

void thread_block(enum task_status stat) {
    ASSERT(((stat == TASK_BLOCKED) || (stat == TASK_WAITING) || (stat == TASK_HANGING)));
    enum intr_status old_status = intr_disable();
    struct task_struct* cur_thread = running_thread();
    cur_thread->status = stat;  //change current thread's status.
    schedule();  //replace current thread from CPU.

    intr_set_status(old_status);
}

void thread_unblock(struct task_struct* pthread) {
    enum intr_status old_status = intr_disable();
    ASSERT(((pthread->status == TASK_BLOCKED) || (pthread->status== TASK_WAITING) || (pthread->status == TASK_HANGING)));
    if(pthread->status != TASK_READY) {
        ASSERT(!elem_find(&thread_ready_list, &pthread->general_tag));
        if(elem_find(&thread_ready_list, &pthread->general_tag)) {
            PANIC("thread_unblock: blocked thread in ready_list\n");
        }
        list_push(&thread_ready_list, &pthread->general_tag);  //pay attention: we use "list_push" to make this thread get quickly schedule.
        pthread->status = TASK_READY;
    }
    intr_set_status(old_status);
}

/*Yield the cpu, change other thread to running.*/
void thread_yield(void) {
    struct task_struct* cur = running_thread();
    enum intr_status old_status = intr_disable();

    ASSERT(!elem_find(&thread_ready_list, &cur->general_tag));
    list_append(&thread_ready_list, &cur->general_tag);
    cur->status = TASK_READY;
    schedule();

    intr_set_status(old_status);
}

/*initial the environment of thread*/
void thread_init(void) {
    put_str("thread_init start\n");

    list_init(&thread_ready_list);
    list_init(&thread_all_list);
    lock_init(&pid_lock);

/*create current main() function to main thread.*/
    make_main_thread();

/*Create idle thread*/
    idle_thread = thread_start("idle", 10, idle, NULL);

    put_str("thread_init done\n");
}