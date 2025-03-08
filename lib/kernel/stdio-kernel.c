#include "stdio-kernel.h"
#include "stdio.h"
#include "console.h"

void printk(const char* format, ...) {  //The function of printing in kernel.
    va_list args;
    va_start(args, format);
    char buf[1024] = {0};
    vsprintf(buf, format, args);
    va_end(args);
    console_put_str(buf);
}