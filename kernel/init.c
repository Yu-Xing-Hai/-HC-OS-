#include "init.h"
#include "print.h"
#include "interrupt.h"
#include "timer.h"
#include "memory.h"
#include "thread.h"
#include "console.h"
#include "keyboard.h"
#include "tss.h"

void init_all() {
   put_str("init_all\n");
   idt_init(); //now, IF bit is 0, we must use "sti" to receive clock interrupt.
   timer_init();  //change clock frequency
   mem_init();
   thread_init();
   console_init();
   keyboard_init();
   tss_init();
}