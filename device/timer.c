#include "timer.h"
#include "stdint.h"
#include "io.h"
#include "print.h"
#include "interrupt.h"
#include "thread.h"
#include "debug.h"

static void frequency_set(uint8_t counter_port, uint8_t counter_no, uint8_t rwl, uint8_t counter_mode, uint16_t counter_value) {
    outb(PIT_CONTROL_PORT, (uint8_t)(counter_no << 6 | rwl << 4 | counter_mode << 1));
    outb(counter_port, (uint8_t)counter_value); //low 8 bit
    outb(counter_port, (uint8_t)counter_value >> 8);
}

uint32_t ticks;
/*clock interrupt handler*/
static void intr_timer_handler(void) {
    struct task_struct* cur_thread = running_thread();

    ASSERT(cur_thread->stack_magic == 0x12345678);

    cur_thread->elapsed_ticks++;
    ticks++; //record all ticks from first clock interrupt to now.
    if(cur_thread->ticks == 0) {
        schedule();
    }
    else {
        cur_thread->ticks--;
    }
}

void timer_init() {
    put_str("timer_init start\n");
    frequency_set(COUNTER0_PORT, COUNTER0_NO, READ_WRITE_LATCH, COUNTER0_MODE, COUNTER0_VALUE);
    register_handler(0x20, intr_timer_handler);
    put_str("timer_init done\n");
}