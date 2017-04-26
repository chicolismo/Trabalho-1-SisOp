#include <stdlib.h>
#include <stdbool.h>
#include "../include/includes.h"


//------------------------------------------------------------------------------
// Esta função inicializa todas as variáveis globais das quais as outras
// funções dependem.  Deve ser chamada nas outras funções, por garantia.
//------------------------------------------------------------------------------
void init()
{
    if (initialized_globals) {
        return;
    }

    // Inicializa as filas de threads
    int i;
    for (i = 0; i < 4; ++i) {
        thread_queues[i] = malloc(sizeof(PFILA2));
        thread_queues[i]->it = NULL;
        thread_queues[i]->first = NULL;
        thread_queues[i]->last = NULL;
    }

    initialized_globals = true;
}

