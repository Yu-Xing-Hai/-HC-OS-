#ifndef __USERPROG_TSS_H
#define __USERPROG_TSS_H
#include "thread.h"
#include "thread.h"

void update_tss_esp0(struct task_struct* pthread);
void tss_init(void);

#endif