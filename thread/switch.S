[bits 32]
section .text
global switch_to
switch_to:
    ;now, esp register point to the switch_to()'s return address
    push esi
    push edi
    push ebx
    push ebp

    mov eax, [esp + 20]  ;get the pointer of cur
    mov [eax], esp  ;now, [eax] is the structure of task_struct's first element,yes, it is self_kstack

    mov eax, [esp + 24] ;get the pointer of next
    mov esp, [eax] ;point to the botton of thread_stack.

    pop ebp
    pop ebx
    pop edi
    pop esi
    ret  ;to execute objective function.
