%include "boot.inc"
SECTION MBR vstart=0x7c00
	mov ax,cs,
	mov ds,ax
	mov es,ax
	mov ss,ax
	mov fs,ax
	mov sp,0x7c00
	mov ax,0xb800
	mov gs,ax
	
	mov	ax,0600h
	mov	bx,0700h
	mov	cx,0
	mov	dx,184fh


	int	10h

	mov byte [gs:0x00],'1'
	mov byte [gs:0x01],0xA4
	
	mov byte [gs:0x02],''
	mov byte [gs:0x03],0xA4

	mov byte [gs:0x04],'M'
	mov byte [gs:0x05],0xA4

	mov byte [gs:0x06],'B'
	mov byte [gs:0x07],0xA4

	mov byte [gs:0x08],'R'
	mov byte [gs:0x09],0xA4

	mov eax,LOADER_START_SECTOR  ;The first LBA sector address.
	mov bx,LOADER_BASE_ADDR      ;The address of writting to memory.(bx:base)
	mov cx,4                     ;The number of sector of watting to read.
	call rd_disk_m_16            ;call the function

	jmp LOADER_BASE_ADDR         ;Move to read bootloader.

;----------------------------------------------------------------------------------------------------------
;function:reading sector of disk and the sector number is n;
rd_disk_m_16:
;----------------------------------------------------------------------------------------------------------
	mov esi,eax        ;backup the eax(bei_feng)
	mov di,cx          ;backup the cx(bei_feng)
;reading the disk:
;First step:setting the number of sectors watting to read to sector count register.
	mov dx,0x1f2          ;0x1f2 is the register of sector count
	mov al,cl
	out dx,al             ;write to register

	mov eax,esi           ;recover eax

;Second step:store the address of LBA to 0x1f3~0x1f6
	mov dx,0x1f3        ;The 0x1f3 is LBA low register, it's size is 8 bit.
	out dx,al           ;Only can write 8 bit.
	
	mov cl,8            ;cx register was already finished to use,so it can use to store 8.
	shr eax,cl          ;logic move to right by 8 bit.
	mov dx,0x1f4        ;The 0x1f4 is LBA mid register, it's size is 8 bit.
	out dx,al

	shr eax,cl
	mov dx,0x1f5        ;The 0x1f4 is LBA high register, it's size is 8 bit.
	out dx,al

	shr eax,cl           ;The LBA address is have 28 bit,so there have 4 bit need to store.
	and al,0x0f          ;0x0f:0000,1111,al store the last 4 bit LBA address,"and"operation can make sure the data is right.
	or al,0xe0           ;0xe0:1110,0000, create the high bit of device register,it's fifth and seventh bit is always be 1,and it's sixth bit set 1 is tell CPU that this address use LBA mode.
	mov dx,0x1f6         ;set device register,this register have 8 bit.
	out dx,al
	
;Third step:Write the reading commend(0x20) to port of 0x1f7
	mov dx,0x1f7         ;0x1f7:when in writting mode, this register's function is commend,and 0x20 commend indicate reading sector.
	mov al,0x20          ;The register of al is free now,because the address of LBA is already stored.
	out dx,al

;Fouth step:Analysize the state of disk
   .not_ready:
	nop            ;empty operation,make a relax.
	in al,dx       ;when in reading mode,0x1f7 function is status,"in" operation is reading the 0x1f7's value to al register,one step can read 2 byte.
	and al,0x88    ;0x88:1000,1000,retain the seventh bit and third bit,if third bit is "1",it indicate that value is ready,if seventh bit is "1",it indicate that disk is busy now,maybe we only need to judge third byte.
	cmp al,0x08    ;0x08:00001000,use "cmp" operation to "-",to judge whether the third bit is "1",and cmp can set the flag by above caculation.We can use ZF flag to judge the outcome of cmp.
	jnz .not_ready ;jnz can inquire the ZF flag,and if ZF is 0,it indicate that the third bit of status is "1",it indicate that CPU can read the value,if ZF is 1,it indicate the third bit of status is "0",it indicate that disk is busy now(the seventh bit is absualutaly "1"),so go back to .not_ready.

;Firth step:reading data from port of 0x1f0
	mov ax,di         ;di store the number of sector of watting to read 
	mov dx,256        ;"in" operation's one step can read 2 byte data,so 512/2=256
	mul dx            ;mul is operation "*",it's value is dx*ax(default).
	mov cx,ax         ;store the answer of mul,and answer is "in" operation's frequency.

	mov dx,0x1f0      ;0x1f0:data register
  .go_on_read:
	in ax,dx          ;get data from dx(0x1f0) and store it to ax.
	mov [bx],ax       ;store data to address of [bx](LOADER_BASE_ADDR)
	add bx,2          ;one step read 2 byte.
	loop .go_on_read
	ret               ;return

	times 510-($-$$) db 0     ;padding the free space by 0
	db 0x55,0xaa      ;flag of MBR
