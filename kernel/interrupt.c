#include "interrupt.h"
#include "stdint.h"
#include "global.h"
#include "io.h"

#define IDT_DESC_CNT 0x21  //the quantity of interrupt which we support at this time.
#define PIC_M_CTRL 0X20
#define PIC_M_DATA 0x21
#define PIC_S_CTRL 0xa0
#define PIC_S_DATA 0xa1


struct gate_desc {  //the struct of interrupt gate descriptor.
   uint16_t  func_offset_low_word;
   uint16_t  selector;
   uint8_t   dcount;  //fixed value,don't think about it.
   uint8_t   attribute;
   uint16_t  func_offset_high_word;
};

//static function declaration
static void make_idt_desc(struct gate_desc* p_gdesc, uint8_t attr, intr_handler function);
static struct gate_desc idt[IDT_DESC_CNT]; //it is an array of interrupt descriptor(IDT).

extern intr_handler intr_entry_table[IDT_DESC_CNT];  //it's also an array of "address of each interrupt handler",and the type of "intr_handle" is defined in document which is named "interrupt.h".

char* intr_name[IDT_DESC_CNT];
intr_handler idt_table[IDT_DESC_CNT];

static void general_intr_handler(uint8_t vec_nr) {
   if(vec_nr == 0x27 || vec_nr == 0x2f) {
      return;
   }
   put_str("int vector : 0x");
   put_int(vec_nr);
   put_char(' ');
   put_str(intr_name[vec_nr]);
   put_char('\n');
}

/*Create interrupt gate descriptor*/
static void make_idt_desc(struct gate_desc* p_gdesc, uint8_t attr, intr_handler function) {
   p_gdesc->func_offset_low_word = (uint32_t)function & 0x0000FFFF; //function: address of each interrupt handler.
   p_gdesc->selector = SELECTOR_K_CODE; //it is defined in document which is named "global.h" and it indicates a selector which is point to code segement in kernel.
   p_gdesc->dcount = 0;
   p_gdesc->attribute = attr;
   p_gdesc->func_offset_high_word = ((uint32_t)function & 0xFFFF0000) >> 16;
}

static void exception_init(void) {
   int i;
   for(i = 0; i < IDT_DESC_CNT; i++) {
      idt_table[i] = general_intr_handler;
      intr_name[i] = "unknow";
   }
   intr_name[0] = "#DE Divide Error";
   intr_name[1] = "#DB Debug Exception";
   intr_name[2] = "NMI Interrupt";
   intr_name[3] = "BP Breakpoint Exception";
   intr_name[4] = "OF Overflow Exception";
   intr_name[5] = "BR BOUND Range Exceeded Exception";
   intr_name[6] = "UD Invalid Opcode Exception";
   intr_name[7] = "NM Device Not Available Exception";
   intr_name[8] = "DF Double Fault Exception";
   intr_name[9] = "Coprocessor Segement Overrun";
   intr_name[10] = "TS Invalid TSS Exception";
   intr_name[11] = "NP Segement Not Present";
   intr_name[12] = "SS Stack Fault Exception";
   intr_name[13] = "GP General Protection Exception";
   intr_name[14] = "PF Page-Faule Exception";
   //intr_name[15] this is holds.
   intr_name[16] = "MF x87 fpu Floating-Point Error";
   intr_name[17] = "AC Alignment Check Exception";
   intr_name[18] = "MC Machine-Check Exception";
   intr_name[19] = "XF SIMD Floation-Point Exception";
}

/*Initialize IDT*/
static void idt_desc_init(void) {
   int i;
   for(i = 0; i < IDT_DESC_CNT; i++) {
      make_idt_desc(&idt[i], IDT_DESC_ATTR_DPL0, intr_entry_table[i]);
   }
   put_str("idt_desc_init done\n");
}

static void pic_init(void) {
   //initialize master slice.
   outb(PIC_M_CTRL, 0x11);
   outb(PIC_M_DATA, 0x20);

   outb(PIC_M_DATA, 0x04);
   outb(PIC_M_DATA, 0x01);

   //initialize slave slice.
   outb(PIC_S_CTRL, 0x11);
   outb(PIC_S_DATA, 0x28);

   outb(PIC_S_DATA, 0x02);
   outb(PIC_S_DATA, 0x01);

   //only open IR0(the interrupt of click)
   outb(PIC_M_DATA, 0xfe);
   outb(PIC_S_DATA, 0xff);

   put_str("pic_init done\n");
}

/*Finish all initial work about interrupt*/
void idt_init() {
   put_str("idt_init start\n");
   exception_init();
   idt_desc_init();
   pic_init();

   /*load IDT*/
   uint64_t idt_operand = ((sizeof(idt) - 1) | ((uint64_t)(uint32_t)idt << 16)); //iperand: the number on which an operation is to be done.
   asm volatile("lidt %0": : "m"(idt_operand));
   put_str("idt_init done\n");
}