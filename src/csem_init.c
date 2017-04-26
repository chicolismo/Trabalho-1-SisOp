#include <stdlib.h>
#include "../include/includes.h"

// 
// typedef struct s_sem {
//     // indica se recurso está ocupado ou não (livre > 0, ocupado = 0)
// 	int	count;
// 
//     // ponteiro para uma fila de threads bloqueadas no semáforo
// 	PFILA2 fila;
// } csem_t;
// 

int csem_init(csem_t *sem, int count)
{
    init();
    // TODO: Descobrir como seria um erro.
    if (sem == NULL) {
        return CSEM_INIT_ERROR;
    }

    sem->fila = NULL;
    sem->count = count;
    return CSEM_INIT_SUCCESS;
}
