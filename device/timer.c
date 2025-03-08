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
    ticks++; //record ticks's happen times from first clock interrupt to now.
    if(cur_thread->ticks == 0) {
        schedule();
    }
    else {
        cur_thread->ticks--;
    }
}

/*Make task sleep, purppose is to delay*/
static void ticks_to_sleep(uint32_t sleep_ticks) {
    uint32_t start_tick = ticks;  //ticks: global parameter

    while(ticks - start_tick < sleep_ticks) {
        thread_yield();  //Yield the CPU to other threads
    }
}

/*
function: delay the process for m_seconds
1s = 1000ms, ms : millisecond
*/
void mtime_sleep(uint32_t m_seconds) {
    uint32_t sleep_ticks = DIV_ROUND_UP(m_seconds, mil_seconds_per_intr);  //convert time to the number of tick.
    ASSERT(sleep_ticks > 0);
    ticks_to_sleep(sleep_ticks);
}

void timer_init() {
    put_str("timer_init start\n");
    frequency_set(COUNTER0_PORT, COUNTER0_NO, READ_WRITE_LATCH, COUNTER0_MODE, COUNTER0_VALUE);
    register_handler(0x20, intr_timer_handler);
    put_str("timer_init done\n");
}