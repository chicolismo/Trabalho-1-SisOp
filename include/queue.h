#ifndef QUEUE_H_
#define QUEUE_H_

int queue_create(PFILA2 queue);
int push(PFILA2 queue, TCB_t *th);
TCB_t *shift(PFILA2 queue);
bool empty(PFILA2 queue);

#endif
