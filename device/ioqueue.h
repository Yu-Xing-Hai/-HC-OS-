#ifndef __DEVICE_IOQUEUE_H
#define __DEVICE_IOQUEUE_H
#include "stdint.h"
#include "stdbool.h"
#include "thread.h"
#include "sync.h"

#define bufsize 64

struct ioqueue {
    /*the lock of buffer*/
    struct lock lock;

    /*if buffer is full, "producer" record which thread become sleeping.*/
    struct task_struct* producer;

    /*if buffer is empty, "consumer" record which thread become sleeping.*/
    struct task_struct* consumer;

    char buf[bufsize];  //buffer
    int32_t head;      //the head of queue
    int32_t tail;      //the tail of queue
};

void ioqueue_init(struct ioqueue* ioq);
bool ioq_full(struct ioqueue* ioq);
char ioq_getchar(struct ioqueue* ioq);
void ioq_putchar(struct ioqueue* ioq, char byte);

#endif