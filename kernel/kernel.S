[bits 32]
%define ERROR_CODE nop  ;nop: no operate.
%define ZERO push 0

extern put_str
extern idt_table

section .data
intr_str db "interrupt occur!", 0xa, 0
global intr_entry_table
intr_entry_table:         ;this address will be used to store all entrances of interrupt program(it will be a array because of "section .data").

%macro VECTOR 2      ;macro of row and parameter's number is 2.
section .text
intr%1entry:        ;intr + %1 + entry to create this name.
   %2          ;if %2=ZERO,push 0, if %2=ERROR_CODE,nop.
   push ds
   push es
   push fs
   push gs
   pushad
   
   mov al, 0x20   ;the commend of "EOI(end of interrupt)",however, when enter interrupt handler,the IF bit is set 0, CPU don's receive clock interrupt.
   out 0xa0, al
   out 0x20, al

   push %1

   call [idt_table + %1*4]
   jmp intr_exit

section .data     ;after compile,these port of ".data" will be merged, so the commend of "dd" will write address of "intr%1entry" below "intr_entry_table:" and these address compose a array.
   dd intr%1entry

%endmacro

section .text
global intr_exit
intr_exit:
   add esp, 4   ;skip the number of interrupt
   popad
   pop gs
   pop fs
   pop es
   pop ds
   add esp, 4   ;skip the error_code or 0 in stack
   iret
   
;--------------------  The interrupt-number of 0x80 --------------------
[bits 32]
extern syscall_table
section .text
global syscall_handler
syscall_handler:
;first: store the environment of task.
   push 0

   push ds
   push es
   push fs
   push gs
   pushad  ;the queue of push-commend is: EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI,!payattention! the way of push is esp--,then write value.

   push 0x80

;second: put the parameters for syscall.
   push edx  ;the third parameter
   push ecx  ;the second parameter
   push ebx  ;the first parameter

;third: call the sub-function of syscall.
   call [syscall_table + eax*4]
   add esp, 12  ;skip three parameters of syscall

;fourth: store the call-function's return value to the position of eax in current kernel stack.
   mov [esp + (1+7)*4], eax  ;we have used 'pushad' to push 8 registers and push 0x80 in stack, so the position of eax in kernel stack is (1+7)*4.
   jmp intr_exit ;recover the environment of task.

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

VECTOR 0x20,ZERO   ;clock interrupt
VECTOR 0x21,ZERO   ;keyboard interrupt
VECTOR 0x22,ZERO   ;be used to link master and sliver
VECTOR 0x23,ZERO   ;Serial of 2
VECTOR 0x24,ZERO   ;Serial of 1
VECTOR 0x25,ZERO   ;Parallel port of 2
VECTOR 0x26,ZERO   ;diskette
VECTOR 0x27,ZERO   ;Parallel port of 1
VECTOR 0x28,ZERO   ;real time clock
VECTOR 0x29,ZERO   ;redirect
VECTOR 0x2a,ZERO   ;retain
VECTOR 0x2b,ZERO   ;retain
VECTOR 0x2c,ZERO   ;mouse of ps/2
VECTOR 0x2d,ZERO   ;the error of fpu float unit
VECTOR 0x2e,ZERO   ;disk
VECTOR 0x2f,ZERO   ;retain