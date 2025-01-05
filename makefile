BUILD_DIR = /home/yuxinghai/bochs/build
ENTRY_POINT = 0xc0001500
AS = nasm
CC = gcc
LD = ld
LIB = -I /home/yuxinghai/bochs/lib/ -I /home/yuxinghai/bochs/lib/kernel/ -I /home/yuxinghai/bochs/lib/user/ -I /home/yuxinghai/bochs/kernel/ -I /home/yuxinghai/bochs/device/
ASFLAGS = -f elf
CCFLAGS = -m32 -Wall $(LIB) -c -fno-builtin -W -Wstrict-prototypes -Wmissing-prototypes
LDFLAGS = -m elf_i386 -Ttext $(ENTRY_POINT) -e main -Map $(BUILD_DIR)/kernel.map
OBJS = $(BUILD_DIR)/main.o $(BUILD_DIR)/init.o $(BUILD_DIR)/interrupt.o $(BUILD_DIR)/timer.o $(BUILD_DIR)/kernel.o $(BUILD_DIR)/print.o $(BUILD_DIR)/debug.o

##########       The compile of C program    #############
$(BUILD_DIR)/main.o : /home/yuxinghai/bochs/kernel/main.c /home/yuxinghai/bochs/lib/kernel/print.h /home/yuxinghai/bochs/lib/stdint.h /home/yuxinghai/bochs/kernel/init.h
	$(CC) $(CCFLAGS) $< -o $@

$(BUILD_DIR)/init.o : /home/yuxinghai/bochs/kernel/init.c /home/yuxinghai/bochs/kernel/init.h /home/yuxinghai/bochs/lib/kernel/print.h /home/yuxinghai/bochs/lib/stdint.h /home/yuxinghai/bochs/kernel/interrupt.h /home/yuxinghai/bochs/device/timer.h
	$(CC) $(CCFLAGS) $< -o $@

$(BUILD_DIR)/interrupt.o : /home/yuxinghai/bochs/kernel/interrupt.c /home/yuxinghai/bochs/kernel/interrupt.h /home/yuxinghai/bochs/lib/stdint.h /home/yuxinghai/bochs/kernel/global.h /home/yuxinghai/bochs/lib/kernel/io.h /home/yuxinghai/bochs/lib/kernel/print.h
	$(CC) $(CCFLAGS) $< -o $@

$(BUILD_DIR)/timer.o : /home/yuxinghai/bochs/device/timer.c /home/yuxinghai/bochs/device/timer.h /home/yuxinghai/bochs/lib/stdint.h /home/yuxinghai/bochs/lib/kernel/io.h /home/yuxinghai/bochs/lib/kernel/print.h
	$(CC) $(CCFLAGS) $< -o $@

$(BUILD_DIR)/debug.o : /home/yuxinghai/bochs/kernel/debug.c /home/yuxinghai/bochs/kernel/debug.h /home/yuxinghai/bochs/lib/kernel/print.h /home/yuxinghai/bochs/lib/stdint.h /home/yuxinghai/bochs/kernel/interrupt.h
	$(CC) $(CCFLAGS) $< -o $@

################   The compile of assembly program    #############
$(BUILD_DIR)/kernel.o : /home/yuxinghai/bochs/kernel/kernel.S
	$(AS) $(ASFLAGS) $< -o $@
$(BUILD_DIR)/print.o : /home/yuxinghai/bochs/lib/kernel/print.S
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

