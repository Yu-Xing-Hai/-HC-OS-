[bits 32]
%define ERROR_CODE nop  ;nop: no operate.
%define ZERO push 0

extern put_str

section .data
intr_str db "interrupt occur!", 0xa, 0
global intr_entry_table
intr_entry_table:         ;this address will be used to store all entrances of interrupt program(it will be a array because of "section .data").

%macro VECTOR 2      ;macro of row and parameter's number is 2.
section .text
intr%1entry:        ;intr + %1 + entry to create this name.
   %2          ;if %2=ZERO,push 0, if %2=ERROR_CODE,nop.
   push intr_str
   call put_str
   add esp, 4     ;skip the intr_str in stack.
   
   mov al, 0x20   ;the commend of "EOI(end of interrupt)"
   out 0xa0, al
   out 0x20, al

   add esp, 4     ;skip the error_code or 0 in stack
   iret

section .data     ;after compile,these port of ".data" will be merged, so the commend of "dd" will write address of "intr%1entry" below "intr_entry_table:" and these address compose a array.
   dd intr%1entry

%endmacro

VECTOR 0x00,ZERO
VECTOR 0x01,ZERO
VECTOR 0x02,ZERO
VECTOR 0x03,ZERO
VECTOR 0x04,ZERO
VECTOR 0x05,ZERO
VECTOR 0x06,ZERO
VECTOR 0x07,ZERO
VECTOR 0x08,ERROR_CODE
VECTOR 0x09,ZERO
VECTOR 0x0a,ERROR_CODE
VECTOR 0x0b,ERROR_CODE
VECTOR 0x0c,ERROR_CODE
VECTOR 0x0d,ERROR_CODE
VECTOR 0x0e,ERROR_CODE
VECTOR 0x0f,ZERO
VECTOR 0x10,ERROR_CODE
VECTOR 0x11,ERROR_CODE
VECTOR 0x12,ERROR_CODE
VECTOR 0x13,ERROR_CODE
VECTOR 0x14,ERROR_CODE
VECTOR 0x15,ZERO
VECTOR 0x16,ZERO
VECTOR 0x17,ERROR_CODE
VECTOR 0x18,ZERO
VECTOR 0x19,ZERO
VECTOR 0x1a,ZERO
VECTOR 0x1b,ZERO
VECTOR 0x1c,ZERO
VECTOR 0x1d,ZERO
VECTOR 0x1e,ERROR_CODE
VECTOR 0x1f,ZERO

VECTOR 0x20,ZERO   ;this vector of interrupt can be defined by user and above all are used by CPU dafault.