rm -f /home/yuxinghai/bochs/build/* &&
gcc -m32 -I /home/yuxinghai/bochs/lib/kernel/ -I /home/yuxinghai/bochs/lib/ -I /home/yuxinghai/bochs/kernel/ -c -fno-builtin -o /home/yuxinghai/bochs/build/main.o /home/yuxinghai/bochs/kernel/main.c &&
nasm -f elf -o /home/yuxinghai/bochs/build/print.o /home/yuxinghai/bochs/lib/kernel/print.S &&
nasm -f elf -o /home/yuxinghai/bochs/build/kernel.o /home/yuxinghai/bochs/kernel/kernel.S &&
gcc -m32 -I /home/yuxinghai/bochs/lib/kernel/ -I /home/yuxinghai/bochs/lib/ -I /home/yuxinghai/bochs/kernel/ -c -fno-builtin -o /home/yuxinghai/bochs/build/interrupt.o /home/yuxinghai/bochs/kernel/interrupt.c &&
gcc -m32 -I /home/yuxinghai/bochs/lib/kernel/ -I /home/yuxinghai/bochs/lib/ -I /home/yuxinghai/bochs/kernel/ -c -fno-builtin -o /home/yuxinghai/bochs/build/init.o /home/yuxinghai/bochs/kernel/init.c &&
ld -m elf_i386 -Ttext 0xc0001500 -e main -o /home/yuxinghai/bochs/build/kernel.bin /home/yuxinghai/bochs/build/main.o /home/yuxinghai/bochs/build/init.o /home/yuxinghai/bochs/build/interrupt.o /home/yuxinghai/bochs/build/print.o /home/yuxinghai/bochs/build/kernel.o &&
dd if=/home/yuxinghai/bochs/build/kernel.bin of=/home/yuxinghai/bochs/hd60M.img bs=512 count=200 seek=9 conv=notrunc