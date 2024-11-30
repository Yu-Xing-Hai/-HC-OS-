;------------------------loader_and_kernel----------------
LOADER_BASE_ADDR equ 0x900
LOADER_START_SECTOR equ 0x2

PAGE_DIR_TABLE_POS equ 0x100000 ;This is PDT's address and this address is the first byte out of low 1MB space(loader and MBR is stored here.).

KERNEL_START_SECTOR equ 0x9
KERNEL_BIN_BASE_ADDR equ 0x70000
KERNEL_ENTRY_POINT equ 0xc0001500
PT_NULL equ 0
;-----------------------the character of gdt discriptor---------
DESC_G_4K  equ  1_00000000000000000000000b   ;The unit of segement'limit is 4K
DESC_D_32  equ   1_0000000000000000000000b   ;Indicate that we use 32 patten of address and operater number.
DESC_L     equ    0_000000000000000000000b   ;Just make it to 0.

DESC_AVL   equ     0_00000000000000000000b
		;CPU didn't use this bit,so we make it to 0
DESC_LIMIT_CODE2  equ  1111_0000000000000000b   ;Make high segement's limit to 1111. 
DESC_LIMIT_DATA2  equ  DESC_LIMIT_CODE2
DESC_LIMIT_VIDEO2  equ 0000_0000000000000000b
DESC_P     equ    1_000000000000000b        ;Make sure that the segement is exist.
DESC_DPL_0 equ     00_0000000000000b        ;DPL:descriptor privilege level
DESC_DPL_1 equ     01_0000000000000b
DESC_DPL_2 equ     10_0000000000000b
DESC_DPL_3 equ     11_0000000000000b
DESC_S_CODE equ      1_000000000000b        ;Descript that this segement is not system'segement.
DESC_S_DATA equ    DESC_S_CODE
DESC_S_sys  equ      0_000000000000b
;-------------------------------------mistake----------------------------------
DESC_TYPE_CODE equ    1000_00000000b        ;x=1,W=0,E=0,A=0
DESC_TYPE_DATA equ    0010_00000000b        ;x=1,W=0,E=1,A=0(Maybe have some mistake!!)
;___________________________________________________________

DESC_CODE_HIGH4 equ (0x00 << 24) + DESC_G_4K + DESC_D_32 + DESC_L + \
DESC_AVL + DESC_LIMIT_CODE2 + DESC_P + DESC_DPL_0 + DESC_S_CODE + \
DESC_TYPE_CODE + 0x00

DESC_DATA_HIGH4 equ (0x00 << 24) + DESC_G_4K + DESC_D_32 + DESC_L + \
DESC_AVL + DESC_LIMIT_DATA2 + DESC_P + DESC_DPL_0 + \
DESC_S_DATA + DESC_TYPE_DATA + 0x00

DESC_VIDEO_HIGH4 equ (0x00 << 24) + DESC_G_4K + DESC_D_32 + DESC_L + \
DESC_AVL + DESC_LIMIT_VIDEO2 + DESC_P + \
DESC_DPL_0 + DESC_S_DATA + DESC_TYPE_DATA + 0x0b

;------------------------chose the sub character-----------
RPL0  equ  00b
RPL1  equ  01b
RPL2  equ  10b
RPL3  equ  11b
TI_GDT  equ  000b
TI_LDT  equ  100b

;-----------------The character of PDE and PTE---------------
PG_P equ 1b
PG_RW_R equ 00b
PG_RW_W equ 10b
PG_US_S equ 000b
PG_US_U equ 100b

