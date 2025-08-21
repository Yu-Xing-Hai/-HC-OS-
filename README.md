# 一、内容说明
1. 本项目依据书籍《操作系统真相还原》编写
2. 项目内核使用C语言与汇编语言混合编程
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
# 三、各模块自我提问与回答
## 内核初始化与操作系统内核加载
### 你在内核初始化过程中都做了什么工作？
- MBR
	- 加载Bootloader到内存：`call rd_disk_m_16`
		- First step
			- setting the number of sectors watting to read to sector count register
		- Second step
			- Store the data address of LBA to 0x1f3~0x1f6
		- Third step
			- Write the reading commend(0x20) to port of 0x1f7
		- Fouth step
			- Analysize the state of disk
		- Firth step
			- Reading data from port of 0x1f0
	- 无条件跳转到Bootloader入口地址
		- `jmp LOADER_BASE_ADDR + 0x300`
			- just go to `loader_start` function, `0x300`is it's offset in file
- Bootloader
	- 初始化
		- GDT构建
			- 在文件首地址处构建三个GDT描述符，并预留一共六个GDT描述符空间
			- 使用`gdt_ptr`指针存储`GDT_LIMIT`，`GDT_BASE`，后续可使用`lgdt`指令将该指针内容加载到GDTR寄存器中存储，便于后续操作系统利用GDTR寄存器寻找到GDT
	- 构建`loader_start`函数，完成实模式到保护模式的转变
		- Zero
			- get total memory size
			- store the answer to the space of `total_mem_bytes` parameter
		- First
			- open the A20
		- Second
			- load the GDT
				- 使用`lgdt`指令将`gdt_ptr`加载到GDTR寄存器中
		- Third
			- make the PE(Protedted Enable) of CR0 to "1".
			- `jmp dword SELECTOR_CODE:p_mode_start`
				- 跳转到`p_mode_start`并刷新流水线(此时从16位指令模式转变为32位指令模式)
			- 将定义好的选择子结构加载到通用寄存器中
	- 加载内核：`call rd_disk_m_32`
		- 过程同MBR将Bootloader加载到内存
	- 初始化内核：`call kernel_init`
		- copy segment from kernel.bin(ELF) to compiled address
	- create PDT and PT：`call setup_page`
		- 宏定义，定义存放PDT的起始地址
			- `PAGE_DIR_TABLE_POS equ 0x100000`
		- First step
			- clean the space that will store PDE by using 0
		- Second stepe
			- Start to create PDE
				- 目标：将高1GB虚拟空间映射到内核空间，将低3GB虚拟空间映射到用户空间
				- 第一个页表项和第768个页表项(第一个指向高端1GB专供内核使用的虚拟空间的页目录项)存储相同的低端1MB物理地址空间所在物理页的起始地址
				- 最后一个页目录项内存储页目录表所在物理页的页基地址
	- 将PDT地址存入相关控制寄存器
		- Open flag bit which is relatived with PDT and PT
			- give PDT's address to CR3
				- `mov eax,PAGE_DIR_TABLE_POS`,`mov cr3,eax`
			- open "PG" flag(31th bit) of CR0
	- 无条件跳转到操作系统内核代码入口：`jmp KERNEL_ENTRY_POINT`
### 为什么需要保护模式？
- 原因
	- 安全问题【主要因素】
		- 实模式下的用户程序可以破坏存储代码的内存区域
		- 实模式下的用户程序和操作系统是同一级别，用户可任意修改操作系统配置或直接与硬件交互，操作系统安全性低
- 解决
	- 实模式下的用户程序可以破坏存储代码的内存区域，所以要添加个内存段类型属性来阻止这种行为
	- 实模式下的用户程序和操作系统是同一级别的，所以要添加个特权级属性来区分用户程序和操作系统的地位
	- 为了限制程序访问内存的范围，还要对段大小进行约束，所以要有段界限属性
	- **这些用来描述内存段的属性，被放到了一个称为段描述符的结构中，该结构是8字节大小**
### GDT中都装了什么？有什么作用？
- 引入
	- 到了保护模式下，内存段（如数据段、代码段等）不再是简单地用段寄存器加载一下段基址就能用，段的信息增加了很多，需要提前把段定义好才能使用
- 定义
	- 全局描述符表（Global Descriptor Table，GDT）是**保护模式下内存段的登记表**
- 构成
	- 一个段描述符只用来定义(描述)一个内存段，代码段要占用一个段描述符，数据段和栈段等多个内存段也要各自占用一个段描述符。因此这些段描述符均放在全局描述符表中
	- 一个段描述符指向一块内存区域
- 作用
	- 存储段描述符
	- CPU可利用段选择子中的索引来获取不同段描述符中的段基址及段界限
### CPU是如何知道何时更改段选择子的？
- 以`mov eax, [0x1200]`指令为例
	- 取址阶段
		- CPU通过`CS:EIP`（代码段选择子+指令指针）获取指令`mov eax, [0x1200]`
	- 解码阶段
		- 识别出指令需要访问内存数据
	- 段选择子切换
		- **默认使用`DS`​**​：因为指令未显式指定段（如`es:`），CPU自动切换到数据段选择子`DS`
			- CPU的段切换是硬件自动完成的，无需软件干预（除非显式使用段覆盖前缀）
		- ​**​段描述符加载​**​
			- 通过`DS`的值（段选择子）从GDT/LDT中查找对应的​**​段描述符​**​
			- 从段描述符中获取​**​数据段基址​**​（Base Address）和​**​段界限​**​（Limit）
	- **线性地址计算**
		- 线性地址 = ​**​数据段基址 + 偏移量0x1200​**​
		- CPU检查`0x1200`是否超出段界限（若超出则触发#GP异常）
	- **权限检查**
		- 确认当前CPL（`CS`的RPL） ≤ 数据段描述符的DPL（权限等级）
		- 若权限不足，触发#GP异常
	- 内存访问
		- 通过分页机制（若启用）将线性地址转换为物理地址，最终读取数据到`eax`
	- **执行完毕**
		- **自动恢复使用`CS`​**​（因为下一条指令的取指必须通过`CS:EIP=0x08:0x1005`）
		- ​**​回归`CS`的时机​**​：当前指令执行完毕，取下一条指令时自然回归
- 平坦模式
	- 在32/64位模式下，通常`CS/DS/ES/SS`的基址为0，界限为4GB，此时段机制近乎透明
	- 但CPU仍会按上述规则执行段检查（即使基址为0）
### 为什么需要分页机制？
- 在只分段的情况下，CPU认为线性地址等于物理地址。而线性地址是由编译器编译出来的，它本身是连续的，所以物理地址也必须要连续才行
- 有一块大段的程序，编译器将其编译为线性地址后，如果没有分页机制，当内存中可用空间都是断断续续的且最大的断续空间不够完整存放该程序，则会发生错误
- 采用分页机制后，尽管编译后的程序依然是线性地址(虚拟地址)，由于虚拟空间与物理空间采用页划分，因此该程序便可被分页为多段页并放置在不相邻的物理空间中，从而有效地利用断续空间
### 为什么使用二级页表而非一级或三级页表呢？
- 一级页表
	- 一级页表的地址转换过程如下
		- 用线性地址的高20位作为页表项的索引
		- 用CR3寄存器中的页表物理地址加上此偏移量便是该页表项的物理地址
		- 从该页表项中得到映射的物理页地址，然后用线性地址的低12位与该物理页地址相加，所得的地址之和便是最终要访问的物理地址。
	- 一级页表的结构
		- 线性地址的一页对应物理地址的一页。一页大小为4KB
		- 4GB虚拟地址空间被划分成4GB/4KB=1M个页
		- 4GB空间中可以容纳1048576个页，页表中自然也要有1048576个页表项，这就是我们要说的一级页表
- 不使用一级页表而使用二级页表的理由
	- 一级页表中最多可容纳1M（1048576）个页表项，每个页表项是4字节，如果页表项全满的话，便是4MB大小，占用物理内存空间过大
	- 一级页表中所有页表项必须要提前建好，原因是操作系统要占用4GB虚拟地址空间的高1GB，用户进程要占用低3GB（即使页表项中没有填充物理地址，也要构建该页表项）
	- 由于每个进程都有自己的页表，进程一多，所有页表占用的空间就很可观了
	- **总结**
		- **不要一次性地将全部页表项建好，而是需要时动态创建页表项**
- 二级页表
	- 二级页表结构
		- 无论是几级页表，标准页的尺寸都是4KB。所以4GB线性地址空间最多有1M个标准页，因此对应1M个页表项
		- 一级页表是将这1M个标准页(对应的页表项)放置到一张页表中，二级页表是将这1M个标准页对应的页表项**平均放置在1K个页表中**
		- 每个页表中包含有1K个页表项。一个页表项是4字节，一个页表包含1K个页表项，故一个页表大小为4KB，这恰恰是一个标准页的大小，专门有个页目录表来存储这些页表
		- 每个页表的物理地址(头地址)在页目录表中都以页目录项（Page Directory Entry，PDE）的形式存储
		- 页目录项大小同页表项一样，都用来描述一个物理页的物理地址，故其大小都是4字节，而且最多有1024(1K)个页表，所以页目录表也是4KB大小，同样也是标准页的大小
## 中断
### 你是如何完成中断控制的？
- 构建初始化函数`idt_init()`
	- `idt_desc_init();`
		- 填充中断处理程序地址到IDT中
	- `pic_init();`
		- initialize 8259A——可编程中断控制器Programmable Interrupt Controller
		- 8259A 用于管理和控制可屏蔽中断，它表现在：屏蔽外设中断、对它们实行优先级判决、向 CPU 提供中断向量号等功能。而它称为可编程的原因，就是可以通过编程的方式来设置以上功能
		- 不同引脚代表不同的可屏蔽中断，如 IR0 引脚代表时钟中断
			- 屏蔽某个外部设备中断信号可以通过设置位于 8259A 中的中断屏蔽寄存器 IMR 来实现，只要将相应位置 1 就达到了屏蔽相应中断源信号的目的
- 加载 IDT 到 IDTR 寄存器
	- `lidt 48位操作数`
- ``iret``和`ret`指令的区别
	- `iret(interrupt ret)`
		- 从中断返回的指令是 `iret(interrupt ret)`，**它从栈中弹出数据到寄存器 cs、eip、eflags 等**，根据特权级是否改变，判断是否要恢复旧栈，也就是说是否将栈中位于 SS_old 和 ESP_old 位置的值弹出到寄存 ss 和 esp。当中断处理程序执行完成返回后，通过 `iret` 指令从栈中恢复 eflags 的内容。
		- `iret` 指令有两个功能，一是从中断返回，另外一个就是返回到调用自己执行的那个旧任务
	- `ret(return)`
		- `ret(return)`指令的功能是在栈顶（寄存器 `ss：sp` 所指向的地址）弹出 2 字节的内容来替换 IP 寄存器，常与`call`指令搭配使用
## 内存管理
### 描述一下你的内存管理系统是如何实现的？
- 第一步：使用bitmap管理内存页
	- 使用bitmap中的每一bit来映射内存中的每一页(4KB)
	- bit为1代表该页被使用；bit为0代表该页未被使用
- 第二步：创建内存池
	- 扫描完物理内存后，将物理内存划分为**用户物理内存**及**内核物理内存**，分别使用用户物理内存池和内核物理内存池进行管理
		- 用户物理内存池：此内存池中的内存只用来分配给用户进程
		- 内核物理内存池：此内存池中的内存只给操作系统使用
	- 由于我们使用了分页机制，因此需要对每个任务的虚拟内存维护一个虚拟内存池
```
struct pool {   //used to manage physic memory.

    struct bitmap pool_bitmap;  //用于描述和管理可用内存的位图；

    uint32_t phy_addr_start;  //内存池结构体管理的可用内存池的起始内存地址(物理/虚拟)；

    uint32_t pool_size;  //内存池中可用内存的总体大小(虚拟内存池结构体不需要，因为4GB与32MB相比足够大，因此不对其大小做约束)

};
```
- 第三步：进程使用malloc申请内存
	- 情况一：申请内存 size >1024 Byte
		`struct arena* a;`
		`a = malloc_page(PF, page_cnt);`
		`return (void*)(a + 1);  //skip the size of arena's information return the remainder memory.`
	- 情况二：申请内存 size <1024 Byte
		- 第一步
			- 用想申请的内存size遍历memory-block-descriptor数组，找出合适的block规格
			- 内存规格：内存块大小按 2 的幂次方对齐的块
		- 第二步
			- 判断该memory-block-descriptor里是否还有空闲内存块
				- 如果memory-block-descriptor的free_list为空
					- 申请一块页内存
					- 初始化该内存页的arena元信息
					`Begin to split the arena to little mem_block, and add them to free_list in mem_block_desc`
				- 如果memory-block-descriptor的free_list不为空
					- 执行第三步
		- 第三步
			- 分配内存块
				- 从memory-block-descriptor的free_list中取出一个内存块，返回其地址
				- 删去memory-block-descriptor的free_list中的该内存块链节点
				- 该小内存块对应的物理页中的arena元信息中的可用内存块数量减1
	- 注意
		- `mem_block_desc`分为两类
			- `k_block_descs[DESC_CNT];`
				- 定义在内核中
			- `u_block_descs`
				- 每个用户进程都拥有一个该数组
### 为什么使用链表而不是顺序表来存储空闲内存块？
- 第零步
	- 在memory-block-descriptor中创建一个空list
	- 该list包含一个头节点和一个尾节点
	- 头尾节点均为双向链表节点
- 第一步
	- 我们将一个物理页划分为不同规格的小内存块
- 第二步
	- 小内存块的头地址处留有一个双向链表节点的空间
- 第三步
	- 当该小内存块被加入到memory-block-descriptor的free_list中时，实际上仅仅是在小内存块的双向链表节点空间中填充其他链表节点的指针信息
	- memory-block-descriptor的free_list中并不添加额外信息，它只负责维护空闲内存块链表的一个头指针和一个尾指针即可
### 你是如何处理内存碎片的？【重点问题】
- 外部内存碎片
	- 使用页表
- 内部碎片
	- memory-block-descriptor数组中的规格从小到大排列
	- 检索内存块时，找到恰好第一个满足申请内存块大小的空闲内存块进行分配，将更大的内存块留给大作业使用，提高了内存块的利用率，降低了内部碎片
- 其他系统常见的处理方式
	- Slab分配器
		- 设计哲学
			- ​**​以空间换时间​**​
		- 定义
			- Slab 分配器（Slab Allocator）是一种​**​针对小对象(如进程描述符、文件对象)高频分配/释放场景优化的内存管理机制**
		- 核心设计思想
			- 消除内存碎片
				- 每个 Slab 仅存储​**​同类型、同大小​**​的对象，对象大小固定，无内存浪费
			- 避免频繁分配物理页
				- 通过预分配多个 Slab（即内存块），减少向内存管理机制申请页面的次数
			- 缓存热对象​
				- 释放的对象不立即归还系统，而是标记为空闲，供后续快速复用，减少初始化开销
		- 三级结构： **Cache → Slab → Object​**
			-  Cache（缓存）
				- 定义
					- 一个 Cache 对应一种内核对象（如进程描述符、 `task_struct`, `inode`）
				- 管理单位
					- 每个 Cache 包含多个 Slab，Slab分为以下三种状态
						- ​**​Full Slab​**​：所有对象已分配
						- **Partial Slab​**​：部分对象空闲
						- **Empty Slab​**​：所有对象未分配
			- Slab（内存块）​
				- **物理结构​**​
					- 一个或多个连续物理页划分为多个等大小的对象槽（Object Slot）
				- **元数据​**​
					- 每个 Slab 维护一个空闲对象链表（Free List），记录可用对象位置
			- 对象（Object）
				- **存储实体​**​
					- 实际分配的内存单元，大小由 Cache 定义（如进程描述符、 `task_struct`, `inode`）
				- **复用机制​**​
					- 释放的对象被标记为空闲，插入 Slab 的空闲链表，供下次快速分配
		- 优缺点分析
			- 优点
				- 极低的分配/释放延迟（接近 O(1) 时间复杂度）
				- 几乎消除碎片问题
				- 减少内存初始化开销（批量预初始化）
			- 缺点
				- 内存预分配可能导致浪费（Empty Slab 占用未使用的内存）。
				- 仅适合固定大小的对象，通用性较差。
				- 实现复杂度较高（需维护多级状态和缓存结构）
	- 内存压缩（Memory Compaction）​
		- 原理
			- 移动已分配的内存块，将空闲内存合并为连续的大块
		- 优缺点
			- 优点
				- ​通过移动内存使空闲区域连续，进而消除外部碎片
			- 缺点
				- 需暂停进程运行，开销较大
		- 应用
			- Java 等垃圾回收语言的堆内存整理
	- 动态重定位（Dynamic Relocation）
		- 原理
			- 通过基址寄存器（Base Register）和界限寄存器（Limit Register）实现进程内存的动态映射
		- 解决碎片
			- 逻辑地址连续即可​，​物理地址无需连续，进程可动态加载到任意空闲区域
		- 应用
			- 结合分页/分段技术，现代操作系统普遍采用
- 内存碎片解决规划
	- 当小规格内存块利用率低，剩余空间无法被使用，如何解决？
	- 当申请连续多页物理页且最后一页的空间利用率低，如何解决？
	- 当空闲页足够，但空闲页不连续，如何解决？
## 虚拟内存空间
### 线性地址是如何与物理地址建立映射的？
- 虚拟地址与物理地址的映射建立过程【用户申请内存空间】
	- 调用`malloc_page()`函数，函数内包含以下三类子函数
		- `vaddr_get()`
			- 获取虚拟内存页地址
		- `palloc()`
			- 获取物理内存页地址
		- `page_table_add()`
			- `uint32_t* pde = pde_ptr(vaddr);`
				- `pde_ptr()`
					- `uint32_t* pde = (uint32_t*)((0xfffff000) + PDE_IDX(vaddr) * 4);`
					- 宏：`#define PDE_IDX(addr) ((addr & 0xffc00000) >> 22)  //get high 10 bit of 32 address`
			- `uint32_t* pte = pte_ptr(vaddr);`
				- `pte_ptr()`
					- `uint32_t* pte = (uint32_t*)(0xffc00000 + ((vaddr & 0xffc00000) >> 10) + PTE_IDX(vaddr) * 4);`
					- 宏：`#define PTE_IDX(addr) ((addr & 0x003ff000) >> 12)  //get mid 10 bit of 32 address`
- 线性地址转为物理地址的转换过程
	- 从32位线性地址到物理地址的转换过程
		- 高12位
		- 中12位
		- 低10位
### 你的系统是如何处理Page-Fault异常的？
- 当前还未实现文件系统，因此暂未实现Page-Fault异常的处理函数【但直到常见Page-Fault异常处理方式】
### 为什么你的虚拟内存池大小是4GB？
- 虚拟地址的范围取决于地址总线的宽度
- 本项目是32位环境，因此最大可访问的虚拟内存空间为4GB
## 进程与线程
### 你是如何创建进程的？
### 你是如何创建线程的？
### 你是如何实现进程切换的？

### 你是如何实现线程切换的？
## 系统调用
### 描述一下你的系统调用是如何实现的？【以getpid()系统调用为例】
- 第一步
	- 用户进程调用`getpid()`系统调用函数
- 第二步
	- 系统调用函数内封装系统调用宏指令
- 第三步
	- 系统调用宏指令利用汇编语言发出`int &0x80`中断指令，同时利用寄存器传入参数(80中断的子功能号)，同时明确函数返回值使用eax寄存器存储
		`asm volatile("int $0x80" : "=a"(retval) : "a"(NUMBER) : "memory");`
	- 调用结果将被eax寄存器存储返回
- 第四步
	- 操作系统从TSS数据结构中获取0级SS段选择子、0级ESP栈指针
	- 操作系统将用户进程上下文信息压入0级栈
- 第五步
	- CPU访问IDT，依据中断向量号80索引访问到系统调用处理程序地址
- 第六步
	- 系统调用处理程序保存寄存器信息
	- 系统调用处理程序使用call命令，依据保存在eax寄存器中的系统调用子功能号跳转到系统调用子功能处理程序数组中对应的处理程序地址【如本例的getpid()系统调用子功能号为0】
- 第七步
	- `uint32_t sys_getpid(void) {return running_thread()->pid;}`
- 第八步
	- eax寄存器保存返回信息
	- 系统调用处理函数调用`jmp intr_exit`返回用户态
		- `iret`
## 输入输出系统
## 硬盘驱动
## 文件系统
## 系统交互
# 四、个人技术博客
- [知乎](https://www.zhihu.com/people/Yu-Xing-Hai)
- [博客园](https://www.cnblogs.com/Yu-Xing-Hai)