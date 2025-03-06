# 一、内容说明
1. 本项目依据书籍《操作系统真相还原》编写。
2. 项目内核使用C语言与汇编语言混合编程。
# 二、进度说明
## 2.1  当前已完成工作说明
1. 使用asm语言完成内存size获取、进入保护模式(打开A20、构建及加载GDT、打开CR0的PE位、创建页表)、加载kernel等工作
2. 使用C语言构建IDT中断描述符表并使用内联汇编(AT&T)完成IDT地址的加载(lidt指令)实现中断机制
3. 使用分页机制+bitmap+arena(多规格内存块)+MemoryBlockDescriptor+链表实现内存管理系统
4. 使用PCB+时钟中断+schedule+switch_to模块实现内核线程创建及多线程调度机制(RR轮询调度)
5. 使用Mutex+semphore+链表实现多线程同步机制
6. 使用TSS+process_activate+switch_to等模块实现进程用户态/内核态切换机制(仿Linux实现)
7. 使用IDT+syscall_handler等模块实现系统调用处理函数的填充
8. 使用fdisk创建分区表，通过编写硬盘驱动程序读取并打印硬盘分区信息
9. 使用GitHub完成软件版本控制，使用WinSCP完成软件跨平台迁移，使用Bochs运行内核代码
