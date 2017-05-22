#ifndef QUEUE_H_
#define QUEUE_H_

#include "cdata.h"
#include <stdbool.h>

int ready_push(TCB_t *th);
TCB_t *ready_shift();
TCB_t *ready_remove(int tid);

typedef struct dupla {
	int waitedTid;
	TCB_t* blockedThread;
} DUPLA_t;

void debug_print_blocked_list();
void debug_blocked_semaphor();
void debug_ready();

#endif
