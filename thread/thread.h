#ifndef __THREAD_THREAD_H
#define __THREAD_THREAD_H
#include "stdint.h"

/*will be used as parameter in create-thread function*/
typedef void thread_func(void*);

/*the status of process or thread.*/
enum task_status {
    TASK_RUNNING,
    TASK_READY,
    TASK_BLOCKED,
    TASK_WAITING,
    TASK_HANGING,
    TASK_DIED
};

/*This structure is used to store environment of process or thread when interrupt occured.*/
struct intr_stack {
    uint32_t vec_no;  //vector number
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp_dummy; //popad will igonre esp.
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;

    /*Sitiuation: when interrupt occured, the privilige level has changed*/
    uint32_t err_code;
    void (*eip) (void);
    uint32_t cs;
    uint32_t eflags;
    void* esp;
    uint32_t ss;
};

/*thread stack*/
struct thread_stack {
    uint32_t ebp;
    uint32_t ebx;
    uint32_t edi;
    uint32_t esi;

/*When thread first run,eip point to kernel_thread(), other time, eip point to the return address of suitch_to().*/
    void (*eip)(thread_func* func, void* func_arg);  //seem to declare an function?

    void (*unused_retaddr);  //placeholding
    thread_func* function;
    void* func_arg;
};

/*process or thread's PCB(program control block)*/
struct task_struct {
    uint32_t* self_kstack;
    enum task_status status;
    uint8_t priority;
    char name[16];
    uint32_t stack_magic;  //the boundary of stack.
};

struct task_struct* thread_start(char* name, int prio, thread_func function, void* func_arg);

#endif