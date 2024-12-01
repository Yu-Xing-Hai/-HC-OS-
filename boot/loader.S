%include "boot.inc"
section loader vstart=LOADER_BASE_ADDR
LOADER_STACK_TOP  equ LOADER_BASE_ADDR
jmp loader_start

;create the GDT and it's descriptor
GDT_BASE:  dd  0x00000000
		dd  0x00000000  ;The first(0) descriptor is not use.

CODE_DESC: dd  0x0000FFFF
		dd  DESC_CODE_HIGH4

DATA_STACK_DESC: dd  0x0000FFFF
			dd DESC_DATA_HIGH4

VIDEO_DESC: dd  0x80000007  ;limit = (0xbffff-0xb8000) / 4k = 0x7
		dd DESC_VIDEO_HIGH4
; you can check this one more time to understand this part.

GDT_SIZE  equ $ - GDT_BASE   ;The val is 0x,and it indicate the size of GDT in memory
GDT_LIMIT equ GDT_SIZE - 1   ;what is mean "-1"?sub one byte or...? Yes! it is one byte.
times 60 dq 0   ;"dq":define quad-word(4 word/8 byte),create sixty empty descriptors
SELECTOR_CODE equ (0x1 << 3) + TI_GDT + RPL0 ;Define the selector
SELECTOR_DATA equ (0x2 << 3) + TI_GDT + RPL0
SELECTOR_VIDEO equ (0x3 << 3) + TI_GDT + RPL0

gdt_ptr  dw  GDT_LIMIT
            dd GDT_BASE   ;We define a pointer and this pointer's address store GDT_LIMIT+GDT_BASE,we can use lgdt commend to copy this address's data to GDTR,it named initial GDTR.

loader_start:  ;running Function program
;------------------------GO to protect mode---------------------
;First:open the A20
;Second:load the GDT
;Third:make the PE of CR0 to "1".
;---------------------------------------------------------------------
;First:open the A20
in al,0x92 ;The port of A20Gate
or al,0000_0010B   ;(B:binary)To make the first bit to 1 to open A20.
out 0x92,al
;Second:load the GDT
lgdt [gdt_ptr]   ;Use lgdt instruction to initial GDTR,gdt_ptr:The parameter of GDTR's address point.GDTR:GDT register,it's 0~15 bit is GDT's boundary,16~47 bit is the GDT's base address in memory.
;Third:make the PE of CR0 to "1".
mov eax,cr0
or eax,0x00000001
mov cr0,eax

jmp dword SELECTOR_CODE:p_mode_start     ;[base address]:[offset value],refresh pipline + refresh "cache register of segement descripter",

[bits 32]
p_mode_start:
	mov ax,SELECTOR_DATA  ;This mode is protect mode ,so we can use selector to get data segement address.
	mov ds,ax
	mov es,ax
	mov ss,ax
	mov esp,LOADER_STACK_TOP  ;same to above
	mov ax,SELECTOR_VIDEO
	mov gs,ax

;------------------------------load kernel-----------------
mov eax, KERNEL_START_SECTOR ;the sector number of kernel.bin in disk.
mov ebx, KERNEL_BIN_BASE_ADDR ;After getting the kernel.bin from disk,writing kernel.bin to address in ebx in memory.
mov ecx, 200 ;The number of watting to be read from sector.
;--------------mistake-------------------
call rd_disk_m_32      ;This function have some problem,maybe can not correctly use 32 bit register.
;----------------------------------------

;Create PDT and PT and initial（Page directory table and Page table）
call setup_page

;write GDT's address and offset value to memory which is pointed by gdt_ptr
sgdt [gdt_ptr]   ;dump the data from GDRT to space which is pointed by gdt_ptr. 
mov ebx,[gdt_ptr + 2]  ;make ebx store GDT's base address
or dword [ebx + 0x18 + 4],0xc0000000 ;ebx + 0x18(0x18:24Byte,this is third segement descriptor(Video segement descriptor),ebx + 0x18 + 4:this is video segement descriptor's high 32bit,and we want to rechange it's segement base address to 3G~4G virtual space address.(you can see segement descriptor's structure).
add dword [gdt_ptr + 2], 0xc0000000

add esp,0xc0000000

;give PDT's address to cr3
mov eax,PAGE_DIR_TABLE_POS
mov cr3,eax

;open "PG" flag(31th bit) of cr0
mov eax,cr0
or eax,0x80000000
mov cr0,eax

;When we open page mode,use new GDT's address to load GDT
lgdt [gdt_ptr]  ;reload GDT

;------------------refresh assembly line----------
jmp SELECTOR_CODE:enter_kernel
enter_kernel:
	call kernel_init
	mov esp, 0xc009f000
	jmp KERNEL_ENTRY_POINT ;use address 0x1500 to test

;------------Create the PDE and PTE-------------------
setup_page:
;First step,clean the space that will store PDE by using 0.
	mov ecx,4096    ;ecx:cycle number register
	mov esi,0
.clear_page_dir:
	mov byte [PAGE_DIR_TABLE_POS + esi],0 ;clean 1 byte in one times,(address + 1)mains that include one byte! 
	inc esi         ;esi = esi + 1(Have some question!,now no have question!)
	loop .clear_page_dir
	
;Start to create PDE
.create_pde:  ;Create Page Directory Entry
	mov eax,PAGE_DIR_TABLE_POS  ;page directory table position
	add eax,0x1000   ;Now,eax point to the first PT's address.
	mov ebx,eax      ;Prepare for .create_pte and ebx is PT's base address.

	or eax,PG_US_U | PG_RW_W | PG_P ;The logic-or's answer is 0x7 and store the answer to eax.
	mov [PAGE_DIR_TABLE_POS + 0x0],eax   ;write the 0x7 to frist PDE(set flags bit)
	mov [PAGE_DIR_TABLE_POS + 0xc00],eax  ;The auther said the first and the 768th(0xc00/4=768) PDE store one same address of page_table(physics address is 0x101000)
;0xc00 indicate that this is 768th PDE,because one PDE use 4 Bytes space.
;Why we use two PDE to store same PT's address? The PDT indicate a virtual address space,as we have said,in virtual address space,the operate system must use high 1G space,and actually in real,operate system is in low 1M physic space.The 768th PDE is the first PDE in high 1G space,so we use 768th PDE to store first PT and it point low 4M physic address(include low 1M space),these steps finished that translate the operate system's virtual space to physic space.  
	sub eax,0x1000	;0x101007 - 0x1000 = 0x100007
	mov [PAGE_DIR_TABLE_POS + 4092],eax ;The auther said that let the last PDE point to PDT's address.(4092/4=1024),eax point to PDT's physic address.

;now,begin to create PTE!!!
	mov ecx,256   ;(1M low memory / 4k(one page size) = 256,this register using to cycle,because of we just use low 1M physic address to store operate system now,so we can just create 256 PTE now.(remember,one PT can include 1024 PTE and all 1024 PTE can include 4M physic address.) 
	mov esi,0
	mov edx,PG_US_U | PG_RW_W | PG_P  ;same
.create_pte:   ;create page table entry~~
	mov [ebx+esi*4],edx  ;ebx register already have value(0x101000:the first page table's address).
	add edx,4096  ;4096: 0001_0000_0000_0000b,so it indicate that we make PTE's high 20bit from 00000000....1 to fffffff...f point to real physic page's head address,and we use (this value + line address's low 12bit to visit real physic address,you can say that in low 1M space,virtual address is same to physic address.
	inc esi  ;one PTE use 4Byte space,so we use (ebx+esi*4) to visit each PTE.
	loop .create_pte ;loop's times is exc=256

;Create other PT's PDE about kernel(virtual address in high 3G~4G)
	mov eax,PAGE_DIR_TABLE_POS  ;0x100000
	add eax,0x2000   ;PDT and PT's size are all 1024*4Byte = 4096Byte = 0x1000,so,the second PT's address = 0x100000+0x2000 = 0x102000
	or eax,PG_US_U | PG_RW_W | PG_P  ;eax now is the next PDE that point to second PT.
	mov ebx,PAGE_DIR_TABLE_POS
	mov ecx,254 ;from 769 to 1023
	mov esi,769  ;768th PDE has already point to first PT.
;------------------------------Question-----------------------
.create_kernel_pde:  ;But why we use total 254 PDE to point PT? if we just want to visit kernel,we just use one PDE point to one PT point to 4M physic address,it absulutaly is enough,so why? 
;Ans:you can see the book page of 202~~
;------------------------------------------------------------------------------------
	mov [ebx+esi*4],eax
	inc esi
	add eax,0x1000
	loop .create_kernel_pde
	ret
;Conclusion: we make 0 and 768th PDE to point to first PT,we have explained that we use 768th and above PDEs to finish translation about kernel(virtual address to physic address),but we also explain why we use 0th PDE to point to first PT.
;We must know that our loader is stored in low 1M physic address.
;When we in real mode,we use (segement_base_address:offset_address) to visit memory,and we already writted these address in loader.S and boot.inc,so,we can use low 1M physic space to run loader,
;When we use loader program to open A20 and set cr0,cr3,we finished to enter protect mode,at that time, our visiting memory's methors become to page mode.
;However,in loader.S,we don't have address about high 3G~4G in virtual address.So,if we don't solve this problem,at the time of we launching page mode,the loader suddently can't find it's program,because as we designing,it's line address is not point to low 1M physic space(Just 3G~4G virtual adderss can point to kernel address.).
;As you can see,the way we solve this problem is make the first PDE(0th) to store first PT,and the first PT point to low 4M physic address,as this way,loader use it's line address to find it's program is successful!!

;----------------------------------------------------rd_disk_m_32---------Have some mistake---------
rd_disk_m_32:
	mov esi,eax
	mov edi,ecx
	
;first step
	mov dx,0x1f2
	mov ax,cx
	out dx,ax
	
	mov eax,esi
;second step
	mov dx,0x1f3
	out dx,ax

	mov cl,8
	shr eax,cl
	mov dx,0x1f4
	out dx,al

	shr eax,cl
	mov dx,0x1f5
	out dx,al

	shr eax,cl
	and al,0x0f
	or al,0xe0
	mov dx,0x1f6
	out dx,al
;third step
	mov dx,0x1f7
	mov al,0x20
	out dx,al
;forth step
  .not_ready:
	nop
	in al,dx
	and al,0x88
	cmp al,0x08
	jnz .not_ready
;fifth step
	mov eax,edi
	mov edx,256   ;"in" operation's one step can read 2 byte data,so 512/2=256
	mul edx
	mov ecx,eax

	mov dx,0x1f0
  .go_on_read:
	in ax,dx
	mov [ebx],ax
	add ebx,2
	loop .go_on_read
	ret
	
;---------------------------copy segment from kernel.bin to compiled address-----------
kernel_init:
	xor eax,eax ;xor:yi_huo caculate,this caculate can reset the register because eax = eax.
	xor ebx,ebx ;ebx is used to store address of program header table.
	xor ecx,ecx ;cx is used to store the number of program header in PHT.
	xor edx,edx ;dx is used to store the size of program header.
	
	mov dx,[KERNEL_BIN_BASE_ADDR + 42]
; offset 42 bytes from file is attribute of e_phentsize,it indicate the size of this program header.
	mov ebx,[KERNEL_BIN_BASE_ADDR + 28]
;offset 28 bytes in file is attribute of e_phoff,it indicate the offset about program header table also about first program header(you can also call it by segement header) in file.(the value is 0x34)
	add ebx,KERNEL_BIN_BASE_ADDR ;what do this step? offset in file + address is physic addrsss~
	mov cx,[KERNEL_BIN_BASE_ADDR + 44]
;offset 44 bytes in file is attribute of e_phnum,it indicate the numbers of program header. 
.each_segment:  ; start to iterate through each segment.
	cmp byte [ebx + 0],PT_NULL ;PT_NULL is a macro and indicate a NULL segment,it defined in ./include/boot.inc
;if p_type == PT_NULL,it indicate that program header is not used.
	je .PTNULL ;if this segment is NULL,jump to .PTNULL function

	push dword [ebx + 16] ;use stack to store p_filesz(memcpy's third parameter: size)
	mov eax, [ebx + 4] ;p_offset
	add eax, KERNEL_BIN_BASE_ADDR ;become physic address
	push eax ;use stack to store segment's physic address(memcpy's second parameter: src)
	push dword [ebx + 8] ;use stack to store p_vaddr(memcpy's first parameter: dest),p_vaddr store the segment's virtual address in memory.
	
	call mem_cpy ;use mem_cpy(function) to copy segment from src(physic address) to dest(virtual address) 
	add esp, 12 ;cleaning three parameters which pushed to stack

.PTNULL:
	add ebx, edx ;edx is indicate size of program header(e_phentsize),so ebx is point to next program header.

	loop .each_segment ;visit the next program header in program header table.
	ret
;----------------Using "Byte-by-byte" method to copy segments---------
; input : three parameters in stack [dst, src, size]
;output : no output
;_______________________________________________________
mem_cpy:
	cld ;clean direction,make eflags register's DF = 0,it means that [e]si and [e]di register add the data's size automatically.
	push ebp ;backup ebp's value by push it's value to stack,when we finish function,we can recover the ebp's value.
	mov ebp, esp ;in this function,we use ebp to visit stack.
	push ecx ;backup ecx
	mov edi,[ebp + 8] ;dst
	mov esi,[ebp + 12] ;src
	mov ecx,[ebp + 16] ;size,Be used to iterate(cycle count)
	rep movsb ;copy use "Byte-by-byte".rep : while(ecx != 0){repeat;ecx--;}

	pop ecx ;recover ecx's value
	pop ebp ;same
	ret
