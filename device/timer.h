#ifndef __DEVICE_TIMER_H
#define __DEVICE_TIMER_H
#include "stdint.h"

#define IRQ0_FREQUENCY     100
#define INPUT_FREQUENCY    1193180
#define COUNTER0_VALUE     INPUT_FREQUENCY / IRQ0_FREQUENCY
#define COUNTER0_PORT      0x40
#define COUNTER0_NO        0
#define COUNTER0_MODE      2
#define READ_WRITE_LATCH   3
#define PIT_CONTROL_PORT   0x43
#define mil_seconds_per_intr (1000 / IRQ0_FREQUENCY)  //the time between two clock interrupt(uint is ms),the cycle of interrupt(uint is ms) is 10ms.

void mtime_sleep(uint32_t m_seconds);
void timer_init(void);

#endif