#include "sync.h"
#include "stdint.h"
#include "list.h"
#include "interrupt.h"
#include "debug.h"

/*initialized semaphore*/
void sema_init(struct semaphore* psema, uint8_t value) {
    psema->value = value;
    list_init(&psema->waiters);
}

/*initialized lock*/
void lock_init(struct lock* plock) {
    plock->holder = NULL;
    plock->holder_repeat_nr = 0;
    sema_init(&plock->semaphore, 1);
}

/*"down" opention of semaphore*/
void sema_down(struct semaphore* psema) {
    enum intr_status old_status = intr_disable();
    if(psema->value == 0) {  //the lock has not be released
        ASSERT(!elem_find(&psema->waiters, &running_thread()->general_tag));
        if(elem_find(&psema->waiters, &running_thread()->general_tag)) {
            PANIC("sema_down: thread blocked has been in waiters_list\n");
        }
        list_append(&psema->waiters, &running_thread()->general_tag);
        thread_block(TASK_BLOCKED);
    }
    psema->value--;  //the lock has be released
    ASSERT(psema->value == 0);
    
    intr_set_status(old_status);
}

/*"up" opention of semaphore*/
void sema_up(struct semaphore* psema) {
    enum intr_status old_status = intr_disable();
    ASSERT(psema->value == 0);
    if(!list_empty(&psema->waiters)) {
        struct task_struct* thread_blocked = elem2entry(struct task_struct, general_tag, list_pop(&psema->waiters));
        thread_unblock(thread_blocked);
    }
    psema->value++;
    ASSERT(psema->value == 1);

    intr_set_status(old_status);
}

/*acquire the plock*/
void lock_acquire(struct lock* plock) {
    if(plock->holder != running_thread()) {
        sema_down(&plock->semaphore);
        plock->holder = running_thread();
        ASSERT(plock->holder_repeat_nr == 0);
        plock->holder_repeat_nr = 1;
    }
    else {    //avoid deadlock
        plock->holder_repeat_nr++;
    }
}

/*release the plock*/
void lock_release(struct lock* plock) {
    ASSERT(plock->holder == running_thread());
    if(plock->holder_repeat_nr > 1) {
        plock->holder_repeat_nr--;
        return;
    }
    ASSERT(plock->holder_repeat_nr == 1);

    plock->holder = NULL;
    plock->holder_repeat_nr = 0;
    sema_up(&plock->semaphore);
}