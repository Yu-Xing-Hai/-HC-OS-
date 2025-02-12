#include "syscall-init.h"
#include "syscall.h"
#include "thread.h"
#include "stdint.h"
#include "print.h"

#define syscall_nr 32
typedef void* syscall;
syscall syscall_table[syscall_nr];  //the pointer array of handler.

/*return pid of current task.*/
uint32_t sys_getpid(void) {
    return running_thread()->pid;
}

/*Initialize syscall*/
void syscall_init(void) {
    put_str("syscall_init start\n");
    syscall_table[SYS_GETPID] = sys_getpid;  //Temporarily, only one sub-syscall-function.
    put_str("syscall_init done\n");
}
