#include "stdio.h"
#include "stdint.h"
#include "string.h"
#include "syscall.h"

/*Translate integer to ascii*/
//buf_ptr_addr: The pointer of a buffer which is used to store the answer of the translation.
//base: The base of the number system, such as 2, 8, 10, 16.
//This function has two parts:One:convert the number's base;Two:convert the number to ascii.
static void i_to_a(uint32_t value, char** buf_ptr_addr, uint8_t base) {
    //tase one:convert the number's base.
    //tase two:convert the number to ascii.
    uint32_t m = value % base;
    uint32_t i = value / base;
    if (i != 0) {
        i_to_a(i, buf_ptr_addr, base);
    }

    if (m < 10) {  //m is less than 10
        *((*buf_ptr_addr)++) = m + '0';  //convert int "0~9" to ascii '0'~'9'.
    }
    else {
        *((*buf_ptr_addr)++) = m - 10 + 'A';  //convert int "A~F" to ascii 'A'~'F'.
    }
}

uint32_t vsprintf(char* str, const char* format, va_list ap) {
    char* buf_ptr = str;
    const char* index_ptr = format;
    char index_char = *index_ptr;  //Use index_char to find the '%' in format.
    int32_t arg_int;
    char* arg_str;  //Used to process the '%s'(string).
    while (index_char) {  //Until the index_char is '\0'.
        if (index_char != '%') {
            *(buf_ptr++) = index_char;
            index_char = *(++index_ptr);
            continue;
        }
        index_char = *(++index_ptr);  //get the next char from '%'.
        switch (index_char) {
            case 's':
                arg_str = va_arg(ap, char*);
                strcpy(buf_ptr, arg_str);
                buf_ptr += strlen(arg_str);
                index_char = *(++index_ptr);
                break;
            case 'c':
                *(buf_ptr++) = va_arg(ap, char);
                index_char = *(++index_ptr);
                break;
            case 'd':
                arg_int = va_arg(ap, int);
                if (arg_int < 0) {
                    arg_int = 0 - arg_int;
                    *buf_ptr++ = '-';
                }
                i_to_a(arg_int, &buf_ptr, 10);
                index_char = *(++index_ptr);
                break;
            case 'x':
                arg_int = va_arg(ap, int);
                i_to_a(arg_int, &buf_ptr, 16);
                index_char = *(++index_ptr);
                break;
        }
    }
    return strlen(str);
}

/*Print string of 'format'*/
uint32_t printf(const char* format, ...) {  //'...' means that the number of arguments is uncertain.
    va_list args;
    va_start(args, format);
    char buf[1024] = {0};
    vsprintf(buf, format, args);
    va_end(args);
    return write(buf);
}

uint32_t sprintf(char* buf, const char* format, ...) {  //'...' means that the number of arguments is uncertain.
    va_list args;
    uint32_t retval;
    va_start(args, format);
    retval = vsprintf(buf, format, args);
    va_end(args);
    return retval;
}