#ifndef QUEUE_H_
#define QUEUE_H_

#include <stdbool.h>

typedef struct FindResult {
    NODE2 *node;
    int queue_number;
} FindResult;

int ready_push(TCB_t *th);
TCB_t *ready_shift();
TCB_t *ready_remove(int tid);
FindResult *ready_find(int tid);

#endif
