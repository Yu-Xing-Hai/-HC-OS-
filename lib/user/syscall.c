#include "syscall.h"
#include "stdint.h"

/*We will achieve the syscall's interface which will be used in User-process.*/

/*The syscall of No-parameters*/
//In Brace, the last value will become the return value of this Brace.
#define _syscall0(NUMBER) ({ \
    int retval; \
    asm volatile("int $0x80" : "=a"(retval) : "a"(NUMBER) : "memory"); \
    retval; \
})

/*The syscall of One-parameter*/
#define _syscall1(NUMBER, ARG1) ({ \
    int retval; \
    asm volatile("int $0x80" : "=a"(retval) : "a"(NUMBER), "b"(ARG1) : "memory"); \
    retval; \
})

/*The syscall of Two-parameters*/
#define _syscall2(NUMBER, ARG1, ARG2) ({ \
    int retval; \
    asm volatile("int $0x80" : "=a"(retval) : "a"(NUMBER), "b"(ARG1), "c"(ARG2) : "memory"); \
    retval; \
})

/*The syscall of Three-parameters*/
#define _syscall3(NUMBER, ARG1, ARG2, ARG3) ({ \
    int retval; \
    asm volatile("int $0x80" : "=a"(retval) : "a"(NUMBER), "b"(ARG1), "c"(ARG2), "d"(ARG3) : "memory"); \
    retval; \
})

/*Define the User's system-call-interface which will be used in User-process*/

/*Return PID of current task*/
uint32_t getpid(void) {
    return _syscall0(SYS_GETPID);
}