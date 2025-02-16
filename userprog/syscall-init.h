#ifndef __USERPROG_SYSCALL_INIT_H
#define __USERPROG_SYSCALL_INIT_H

#include "stdint.h"

enum SYSCALL_NR {  //The structure is used to define the syscall's sub-number.
    SYS_GETPID,  //Default value is 0.
    SYS_WRITE,   //Default value is 1.
    SYS_MALLOC,  //Default value is 2.
    SYS_FREE     //Default value is 3.
};

uint32_t sys_getpid(void);
uint32_t sys_write(char* str);
void syscall_init(void);

#endif