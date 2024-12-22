TI_GDT equ 0
RPL0 equ 0
SELECTOR_VIDEO equ (0x0003<<3) + TI_GDT + RPL0 ;create SELECTOR_VIDEO.

section .data
put_int_buffer dq 0  ;this buffer is used to translate int to char

[bits 32]
section .text
;---------------------put_char--------------
;The function's behavior:write one char from stack to the address where the cursor point.
;-------------------------------------------
global put_char ;extern document can invoke this function.
put_char:
    pushad   ;pushad:push all double.Back the environment of all "double byte registers" in 32 mode(totlally have numbers of 8).
    mov ax, SELECTOR_VIDEO
    mov gs, ax    ;we can't push number to fragment register directly.

;-----------get the cursor's address--------
;first: get the high 8bit.
    mov dx, 0x03d4
    mov al, 0x0e  ;Cursor Location High Register
    out dx, al   ;designate offset of register group.
    mov dx, 0x03d5
    in al, dx
    mov ah, al
;second: get the low 8bit.
    mov dx, 0x03d4
    mov al, 0x0f   ;Cursor Location Low Register
    out dx, al
    mov dx, 0x03d5
    in al, dx
;store cursor's address to register of bx.
    mov bx, ax
;get the string we want to print.
    mov ecx, [esp + 36] ;pushad us 4 × 8 = 32byte,the return address use 4byte,so the string's address is under esp + 36.

    cmp cl, 0xd  ;Carriage Return's ASCLL is 0xd
    jz .is_carriage_return
    cmp cl, 0xa  ;Line feed's ASCLL is 0xa
    jz .is_line_feed
    cmp cl, 0x8  ;backspace's ASCLL is 0x8
    jz .is_backspace
    jmp .put_other

.is_backspace:
    dec bx     ;bx - 1,and the bx is cursor's value.
    shl bx, 1  ;make the cursor's value to real address in video memory.

    mov byte [gs:bx], 0x20  ;0x20:the ASCLL of space.
    inc bx     ;bx + 1  ;bx(real address) is point to the char's attribute
    mov byte [gs:bx], 0x07  ;black background and white font color.
    shr bx, 1  ;make the real address in video memory to cursor's value,and forget the remainder.
    jmp .set_cursor
.put_other:
    shl bx, 1
    mov byte [gs:bx], cl
    inc bx
    mov byte [gs:bx], 0x07
    shr bx, 1
    inc bx
    cmp bx, 2000  ;if cursor's val < 2000,it indicate that the address is not the end of video memory.
    jl .set_cursor
.is_line_feed:
.is_carriage_return:
    xor dx, dx
    mov ax, bx
    mov si, 80

    div si

    sub bx, dx
.is_carriage_return_end:
    add bx, 80
    cmp bx, 2000
.is_line_feed_end:
    jl .set_cursor
;_____________Pay attention : this code is not achieving Scroll!________________________________________
.set_cursor:
    mov dx, 0x03d4
    mov al, 0x0e
    out dx, al
    mov dx, 0x03d5
    mov al, bh
    out dx, al

    mov dx, 0x03d4
    mov al, 0x0f
    out dx, al
    mov dx, 0x03d5
    mov al, bl
    out dx, al
.put_char_done:
    popad  ;is the otherside of pushad
    ret    ;return from put_char function.

;--------------put_str----------
global put_str
put_str:
    push ebx
    push ecx
    xor ecx, ecx
    mov ebx, [esp + 12] ;why is 12?:one return address,two register.
.goon:
    mov cl, [ebx]
    cmp cl, 0
    jz .str_over
    push ecx
    call put_char
    add esp, 4
    inc ebx
    jmp .goon
.str_over:
    pop ecx
    pop ebx
    ret
;-----------------put_int-----------------
;Payattention!!
;The bochs's memory is Little Endian.
;we want to print char in screen by Big Endian(human's habit)like 15,0xfc0 and so on.
;so, we need two step;
;First step: make each int number to it's ASCLL.
;Second step: print each int number's ASCLL.
global put_int
put_int:
    pushad
    mov ebp, esp
    mov eax, [ebp+4*9] ;get the int number to eax,and 4*9 is means that one call's return address and eight register.
    mov edx, eax
    mov edi, 7
    mov ecx, 8
    mov ebx, put_int_buffer ;ebx is pointing to buffer address.
.16based_4bits:
    and edx, 0x0000000F ;only remian the int number's low 8bit.
    cmp edx, 9
    jg .is_A2F  ;"A2F" is "A to F".
    add edx, '0'  ;Make int number to ASCLL
    jmp .store
.is_A2F:
    sub edx, 10
    add edx, 'A'  ;Make int number to ASCLL(Hex)
.store:
    mov [ebx+edi], dl
    dec edi
    shr eax, 4
    mov edx, eax
    loop .16based_4bits
.ready_to_print:
    inc edi
.skip_prefix_0:
    cmp edi, 8
    je .full0
.go_on_skip:
    mov cl, [put_int_buffer+edi]
    inc edi
    cmp cl, '0'
    je .skip_prefix_0
    dec edi
    jmp .put_each_num
.full0:
    mov cl, '0'
;the program in below(on .full0) is created by myself,because i think, when all int number is '0',and at that time,the edi also is 8,we can not use this commend `mov cl,[put_int_buffer+edi]`,the cl register store some data that we don't know,it is so dangerous.
    push ecx
    call put_char
    add esp, 4
    popad
    ret
.put_each_num:
    push ecx
    call put_char
    add esp, 4
    inc edi
    mov cl, [put_int_buffer+edi]
    cmp edi, 8
    jl .put_each_num
    popad
    ret