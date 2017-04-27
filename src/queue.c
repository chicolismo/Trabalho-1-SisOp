#include <stdbool.h>
#include "../include/includes.h"

int queue_create(PFILA2 queue)
{
    return CreateFila2(queue);
}

// Coloca a thread "th" no final da fila "queue"
int push(PFILA2 queue, TCB_t *th)
{
    int ret = AppendFila2(queue, (void *) th);
    FirstFila2(queue);
    return ret;
}

// Coloca o primeiro elemento de "queue" em "th", e o remove da fila
TCB_t *shift(PFILA2 queue)
{
    FirstFila2(queue);
    TCB_t *th = (TCB_t*) GetAtIteratorFila2(queue);
    DeleteAtIteratorFila2(queue);
    return th;
}

bool empty(PFILA2 queue)
{
    // Quando não consegue "setar" para o primeiro elemento da fila,
    // então o resultado é diferente de zero e a fila está vazia.
    return FirstFila2(queue) != 0;
}
