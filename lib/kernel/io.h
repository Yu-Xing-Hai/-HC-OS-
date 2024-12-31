#ifndef __LIB_IO_H
#define __LIB_IO_H
#include "stdint.h"

/*
First function: outb()
function: 
   write one byte to port.
parameters:
   uint16_t: the type of unsigned int and it's 16bit value.
*/
static inline void outb(uint16_t port, uint8_t data) {
   asm volatile ("outb %b0, %w1" : : "a"(data), "Nd"(port)); //The parameter of "N" indicate that the port number is between 0 and 255.
}

/*
Second function: outsw()
function: 
   write more byte to port(the unit is 2byte.)
parameters: 
   void* addr: it means the parameter's type of addr is point.
   S: esi/si
directives:
   rep: repeat,and "cx" register self-subtraction unless it's value is 0.
   outsw(assembly language): write 16bit data from address which is pointed by ds:esi to port.
   cld: clean direction, this directive can set the flag of DF to 0, then, register's value of "si" will be increased by length of data.
*/
static inline void outsw(uint16_t port, const void* addr, uint32_t word_cnt) {
   asm volatile ("cld; rep outsw" : "+S"(addr), "+c"(word_cnt) : "d"(port));
}

/*
Third fuction: inb()
function: 
   read one byte from port.
*/
static inline uint8_t inb(uint16_t port) {
   uint8_t data;
   asm volatile ("inb %w1, %b0" : "=a"(data) : "Nd"(port));
   return data;
}

/*
Forth fuction: insw()
function: 
   read more byte from port.
parameters:
   D: edi/di
   "memory": we notice gcc that we changed memory.
directives:
   insw(assembly language): read 16bit data from port to address which is pointed by es:edi.
*/
static inline void insw(uint16_t port, void* addr, uint32_t word_cnt) {
   asm volatile ("cld; rep insw" : "+D"(addr), "+c"(word_cnt) : "d"(port) : "memory");
}

#endif