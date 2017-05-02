#ifndef QUEUE_H_
#define QUEUE_H_

#include <stdbool.h>

int push_ready(TCB_t *th);
TCB_t *shift_ready();
TCB_t *remove_ready(int tid);

#endif
