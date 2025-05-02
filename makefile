BUILD_DIR = /home/yuxinghai/bochs/build
ENTRY_POINT = 0xc0001500
AS = nasm
CC = gcc
LD = ld
LIB = -I /home/yuxinghai/bochs/lib/ -I /home/yuxinghai/bochs/lib/kernel/ -I /home/yuxinghai/bochs/lib/user/ -I /home/yuxinghai/bochs/kernel/ -I /home/yuxinghai/bochs/device/ -I /home/yuxinghai/bochs/thread/ -I /home/yuxinghai/bochs/userprog/ -I /home/yuxinghai/bochs/fs/
ASFLAGS = -f elf
CCFLAGS = -m32 -Wall $(LIB) -c -fno-builtin -W -Wstrict-prototypes -Wmissing-prototypes
LDFLAGS = -m elf_i386 -Ttext $(ENTRY_POINT) -e main -Map $(BUILD_DIR)/kernel.map
OBJS = $(BUILD_DIR)/main.o $(BUILD_DIR)/init.o $(BUILD_DIR)/interrupt.o $(BUILD_DIR)/timer.o $(BUILD_DIR)/kernel.o $(BUILD_DIR)/print.o $(BUILD_DIR)/debug.o $(BUILD_DIR)/string.o $(BUILD_DIR)/stdio.o $(BUILD_DIR)/bitmap.o $(BUILD_DIR)/memory.o $(BUILD_DIR)/thread.o $(BUILD_DIR)/list.o $(BUILD_DIR)/switch.o $(BUILD_DIR)/sync.o $(BUILD_DIR)/console.o $(BUILD_DIR)/keyboard.o $(BUILD_DIR)/ioqueue.o $(BUILD_DIR)/tss.o $(BUILD_DIR)/process.o $(BUILD_DIR)/syscall.o $(BUILD_DIR)/syscall-init.o $(BUILD_DIR)/stdio-kernel.o $(BUILD_DIR)/ide.o $(BUILD_DIR)/fs.o

#################################       The compile of C program    ##############################
$(BUILD_DIR)/main.o : /home/yuxinghai/bochs/kernel/main.c /home/yuxinghai/bochs/lib/kernel/print.h /home/yuxinghai/bochs/kernel/init.h /home/yuxinghai/bochs/thread/thread.h /home/yuxinghai/bochs/kernel/interrupt.h /home/yuxinghai/bochs/device/console.h /home/yuxinghai/bochs/userprog/process.h  /home/yuxinghai/bochs/lib/user/syscall.h  /home/yuxinghai/bochs/userprog/syscall-init.h /home/yuxinghai/bochs/lib/stdio.h /home/yuxinghai/bochs/kernel/memory.h
	$(CC) $(CCFLAGS) $< -o $@

$(BUILD_DIR)/init.o : /home/yuxinghai/bochs/kernel/init.c /home/yuxinghai/bochs/kernel/init.h /home/yuxinghai/bochs/lib/kernel/print.h /home/yuxinghai/bochs/kernel/interrupt.h /home/yuxinghai/bochs/device/timer.h /home/yuxinghai/bochs/kernel/memory.h /home/yuxinghai/bochs/thread/thread.h /home/yuxinghai/bochs/device/console.h /home/yuxinghai/bochs/device/keyboard.h /home/yuxinghai/bochs/userprog/tss.h /home/yuxinghai/bochs/userprog/syscall-init.h /home/yuxinghai/bochs/fs/fs.h
	$(CC) $(CCFLAGS) $< -o $@

$(BUILD_DIR)/interrupt.o : /home/yuxinghai/bochs/kernel/interrupt.c /home/yuxinghai/bochs/kernel/interrupt.h /home/yuxinghai/bochs/lib/stdint.h /home/yuxinghai/bochs/kernel/global.h /home/yuxinghai/bochs/lib/kernel/io.h /home/yuxinghai/bochs/lib/kernel/print.h
	$(CC) $(CCFLAGS) $< -o $@

$(BUILD_DIR)/timer.o : /home/yuxinghai/bochs/device/timer.c /home/yuxinghai/bochs/device/timer.h /home/yuxinghai/bochs/lib/stdint.h /home/yuxinghai/bochs/lib/kernel/io.h /home/yuxinghai/bochs/lib/kernel/print.h /home/yuxinghai/bochs/kernel/interrupt.h /home/yuxinghai/bochs/thread/thread.h /home/yuxinghai/bochs/kernel/debug.h
	$(CC) $(CCFLAGS) $< -o $@

$(BUILD_DIR)/debug.o : /home/yuxinghai/bochs/kernel/debug.c /home/yuxinghai/bochs/kernel/debug.h /home/yuxinghai/bochs/lib/kernel/print.h /home/yuxinghai/bochs/kernel/interrupt.h
	$(CC) $(CCFLAGS) $< -o $@

$(BUILD_DIR)/string.o : /home/yuxinghai/bochs/lib/string.c /home/yuxinghai/bochs/lib/string.h /home/yuxinghai/bochs/lib/stdint.h /home/yuxinghai/bochs/lib/stdbool.h /home/yuxinghai/bochs/kernel/global.h /home/yuxinghai/bochs/kernel/debug.h
	$(CC) $(CCFLAGS) $< -o $@

$(BUILD_DIR)/stdio.o : /home/yuxinghai/bochs/lib/stdio.c /home/yuxinghai/bochs/lib/stdio.h /home/yuxinghai/bochs/lib/stdint.h /home/yuxinghai/bochs/lib/string.h /home/yuxinghai/bochs/lib/user/syscall.h
	$(CC) $(CCFLAGS) $< -o $@

$(BUILD_DIR)/bitmap.o : /home/yuxinghai/bochs/lib/kernel/bitmap.c /home/yuxinghai/bochs/lib/kernel/bitmap.h /home/yuxinghai/bochs/lib/stdint.h /home/yuxinghai/bochs/lib/kernel/print.h /home/yuxinghai/bochs/kernel/interrupt.h /home/yuxinghai/bochs/kernel/debug.h /home/yuxinghai/bochs/kernel/global.h /home/yuxinghai/bochs/lib/string.h
	$(CC) $(CCFLAGS) $< -o $@

$(BUILD_DIR)/memory.o : /home/yuxinghai/bochs/kernel/memory.c /home/yuxinghai/bochs/kernel/memory.h /home/yuxinghai/bochs/lib/stdint.h /home/yuxinghai/bochs/lib/kernel/print.h /home/yuxinghai/bochs/lib/kernel/bitmap.h /home/yuxinghai/bochs/kernel/debug.h /home/yuxinghai/bochs/lib/string.h /home/yuxinghai/bochs/lib/kernel/bitmap.h /home/yuxinghai/bochs/thread/sync.h /home/yuxinghai/bochs/thread/thread.h /home/yuxinghai/bochs/kernel/interrupt.h /home/yuxinghai/bochs/kernel/list.h /home/yuxinghai/bochs/kernel/interrupt.h /home/yuxinghai/bochs/kernel/list.h
	$(CC) $(CCFLAGS) $< -o $@

$(BUILD_DIR)/thread.o : /home/yuxinghai/bochs/thread/thread.c /home/yuxinghai/bochs/thread/thread.h /home/yuxinghai/bochs/lib/stdint.h /home/yuxinghai/bochs/lib/string.h /home/yuxinghai/bochs/kernel/global.h /home/yuxinghai/bochs/kernel/memory.h /home/yuxinghai/bochs/lib/kernel/print.h /home/yuxinghai/bochs/kernel/debug.h /home/yuxinghai/bochs/userprog/process.h /home/yuxinghai/bochs/thread/sync.h
	$(CC) $(CCFLAGS) $< -o $@

$(BUILD_DIR)/list.o : /home/yuxinghai/bochs/kernel/list.c /home/yuxinghai/bochs/kernel/list.h /home/yuxinghai/bochs/kernel/interrupt.h /home/yuxinghai/bochs/lib/stdbool.h /home/yuxinghai/bochs/lib/stdint.h
	$(CC) $(CCFLAGS) $< -o $@

$(BUILD_DIR)/sync.o : /home/yuxinghai/bochs/thread/sync.c /home/yuxinghai/bochs/thread/sync.h /home/yuxinghai/bochs/lib/stdint.h /home/yuxinghai/bochs/kernel/list.h /home/yuxinghai/bochs/kernel/interrupt.h /home/yuxinghai/bochs/kernel/debug.h
	$(CC) $(CCFLAGS) $< -o $@

$(BUILD_DIR)/console.o : /home/yuxinghai/bochs/device/console.c /home/yuxinghai/bochs/device/console.h /home/yuxinghai/bochs/lib/kernel/print.h /home/yuxinghai/bochs/lib/stdint.h /home/yuxinghai/bochs/thread/sync.h /home/yuxinghai/bochs/thread/thread.h
	$(CC) $(CCFLAGS) $< -o $@

$(BUILD_DIR)/keyboard.o : /home/yuxinghai/bochs/device/keyboard.c /home/yuxinghai/bochs/device/keyboard.h /home/yuxinghai/bochs/lib/kernel/print.h /home/yuxinghai/bochs/kernel/interrupt.h /home/yuxinghai/bochs/lib/kernel/io.h /home/yuxinghai/bochs/kernel/global.h /home/yuxinghai/bochs/lib/stdint.h /home/yuxinghai/bochs/lib/stdbool.h /home/yuxinghai/bochs/device/ioqueue.h
	$(CC) $(CCFLAGS) $< -o $@

$(BUILD_DIR)/ioqueue.o : /home/yuxinghai/bochs/device/ioqueue.c /home/yuxinghai/bochs/device/ioqueue.h /home/yuxinghai/bochs/kernel/interrupt.h /home/yuxinghai/bochs/kernel/global.h /home/yuxinghai/bochs/kernel/debug.h /home/yuxinghai/bochs/lib/stdbool.h
	$(CC) $(CCFLAGS) $< -o $@

$(BUILD_DIR)/tss.o : /home/yuxinghai/bochs/userprog/tss.c /home/yuxinghai/bochs/userprog/tss.h /home/yuxinghai/bochs/lib/stdint.h /home/yuxinghai/bochs/lib/kernel/print.h /home/yuxinghai/bochs/kernel/global.h /home/yuxinghai/bochs/thread/thread.h /home/yuxinghai/bochs/lib/string.h /home/yuxinghai/bochs/kernel/memory.h
	$(CC) $(CCFLAGS) $< -o $@

$(BUILD_DIR)/process.o : /home/yuxinghai/bochs/userprog/process.c /home/yuxinghai/bochs/userprog/process.h /home/yuxinghai/bochs/thread/thread.h /home/yuxinghai/bochs/kernel/global.h /home/yuxinghai/bochs/lib/stdint.h /home/yuxinghai/bochs/kernel/memory.h /home/yuxinghai/bochs/kernel/kernel.h /home/yuxinghai/bochs/userprog/tss.h /home/yuxinghai/bochs/device/console.h /home/yuxinghai/bochs/lib/string.h /home/yuxinghai/bochs/kernel/interrupt.h /home/yuxinghai/bochs/kernel/debug.h /home/yuxinghai/bochs/kernel/list.h
	$(CC) $(CCFLAGS) $< -o $@

$(BUILD_DIR)/syscall.o : /home/yuxinghai/bochs/lib/user/syscall.c /home/yuxinghai/bochs/lib/user/syscall.h /home/yuxinghai/bochs/userprog/syscall-init.h /home/yuxinghai/bochs/lib/stdint.h
	$(CC) $(CCFLAGS) $< -o $@

$(BUILD_DIR)/syscall-init.o : /home/yuxinghai/bochs/userprog/syscall-init.c /home/yuxinghai/bochs/userprog/syscall-init.h /home/yuxinghai/bochs/lib/user/syscall.h /home/yuxinghai/bochs/thread/thread.h /home/yuxinghai/bochs/lib/stdint.h /home/yuxinghai/bochs/lib/kernel/print.h /home/yuxinghai/bochs/device/console.h /home/yuxinghai/bochs/lib/string.h /home/yuxinghai/bochs/kernel/memory.h
	$(CC) $(CCFLAGS) $< -o $@

$(BUILD_DIR)/stdio-kernel.o : /home/yuxinghai/bochs/lib/kernel/stdio-kernel.c /home/yuxinghai/bochs/lib/stdio.h /home/yuxinghai/bochs/device/console.h
	$(CC) $(CCFLAGS) $< -o $@

$(BUILD_DIR)/ide.o : /home/yuxinghai/bochs/device/ide.c /home/yuxinghai/bochs/device/ide.h /home/yuxinghai/bochs/lib/stdint.h /home/yuxinghai/bochs/kernel/debug.h /home/yuxinghai/bochs/kernel/global.h /home/yuxinghai/bochs/lib/stdio.h /home/yuxinghai/bochs/lib/kernel/stdio-kernel.h /home/yuxinghai/bochs/thread/sync.h /home/yuxinghai/bochs/lib/kernel/io.h /home/yuxinghai/bochs/device/timer.h /home/yuxinghai/bochs/kernel/interrupt.h /home/yuxinghai/bochs/kernel/list.h /home/yuxinghai/bochs/lib/string.h /home/yuxinghai/bochs/kernel/memory.h
	$(CC) $(CCFLAGS) $< -o $@

$(BUILD_DIR)/fs.o : /home/yuxinghai/bochs/fs/fs.c /home/yuxinghai/bochs/fs/fs.h /home/yuxinghai/bochs/lib/stdint.h /home/yuxinghai/bochs/kernel/debug.h /home/yuxinghai/bochs/kernel/global.h /home/yuxinghai/bochs/fs/inode.h /home/yuxinghai/bochs/fs/super_block.h /home/yuxinghai/bochs/fs/dir.h /home/yuxinghai/bochs/lib/kernel/stdio-kernel.h //home/yuxinghai/bochs/device/ide.h /home/yuxinghai/bochs/lib/string.h /home/yuxinghai/bochs/kernel/memory.h /home/yuxinghai/bochs/lib/stdbool.h
	$(CC) $(CCFLAGS) $< -o $@
################################   The compile of assembly program    ##############################
$(BUILD_DIR)/kernel.o : /home/yuxinghai/bochs/kernel/kernel.S
	$(AS) $(ASFLAGS) $< -o $@
$(BUILD_DIR)/print.o : /home/yuxinghai/bochs/lib/kernel/print.S
	$(AS) $(ASFLAGS) $< -o $@
$(BUILD_DIR)/switch.o : /home/yuxinghai/bochs/thread/switch.S
	$(AS) $(ASFLAGS) $< -o $@
######  link all object document  #########
$(BUILD_DIR)/kernel.bin : $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

.PHONY : mk_dir hd clean build all

#fi: the end of if,and "if"must add a " " + other cmmmend#
mk_dir:
	if [[ ! -d $(BUILD_DIR) ]];then mkdir $(BUILD_DIR);fi

hd:
	dd if=$(BUILD_DIR)/kernel.bin \
	of=/home/yuxinghai/bochs/hd60M.img \
	bs=512 count=200 seek=9 conv=notrunc

clean:
	cd $(BUILD_DIR) && rm -f ./*

build: $(BUILD_DIR)/kernel.bin

all: mk_dir build hd

