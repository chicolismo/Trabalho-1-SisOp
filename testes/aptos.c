/*

	TESTE PARA AS FUNÇÕES E ESTRUTURAS QUE POSSIBILITAM A FILA DE APTOS.

*/

#include <stdio.h>
#include <stdlib.h>

#include "../include/support.h"
#include "../include/cdata.h"
#include "../include/cthread.h"

// Retorna um numero aleatório entre [0, limit]
int rand_lim(int limit) {

    int divisor = RAND_MAX/(limit+1);
    int retval;

    do { 
        retval = rand() / divisor;
    } while (retval > limit);

    return retval;
}

PFILA2 ready_active;
PFILA2 test;

int main(int argc, char const *argv[])
{	 
	ready_active = (FILA2 *)malloc(sizeof(FILA2));
	DEBUGPRINT(("[Test] Criando fila de APTOS: %d\n", CreateFila2(ready_active)));

	TCB_t* tcb[100];

	int i;
	for(i = 0; i<100; i++) {
		tcb[i] = (TCB_t *)malloc(sizeof(TCB_t));
		tcb[i]->tid = rand_lim(9999);

		if(i >= 50)
			tcb[i]->ticket = rand_lim(255);
		else
			tcb[i]->ticket = 10;
	}

	DEBUGPRINT(("ready_queue_insert(tcb[10]): %d\n", ready_queue_insert(tcb[0]))); 
	DEBUGPRINT(("ready_queue_insert(tcb[15]) %d\n", ready_queue_insert(tcb[1])));
	DEBUGPRINT(("ready_queue_insert(tcb[5]): %d\n", ready_queue_insert(tcb[2])));
	DEBUGPRINT(("ready_queue_insert(tcb[5]): %d\n", ready_queue_insert(tcb[3])));
	DEBUGPRINT(("ready_queue_insert(tcb[8]): %d\n", ready_queue_insert(tcb[4])));
	DEBUGPRINT(("ready_queue_insert(tcb[8]): %d\n", ready_queue_insert(tcb[5])));
	DEBUGPRINT(("ready_queue_insert(tcb[51]): %d\n", ready_queue_insert(tcb[51])));
	DEBUGPRINT(("ready_queue_insert(tcb[52]): %d\n", ready_queue_insert(tcb[52])));
	DEBUGPRINT(("ready_queue_insert(tcb[53]): %d\n", ready_queue_insert(tcb[53])));
	DEBUGPRINT(("ready_queue_insert(tcb[54]): %d\n", ready_queue_insert(tcb[54])));

	debug_print_ready_active();

	choose_next_thread_to_run();
	choose_next_thread_to_run();
	choose_next_thread_to_run();
	choose_next_thread_to_run();
	choose_next_thread_to_run();
	choose_next_thread_to_run();
	choose_next_thread_to_run();
	choose_next_thread_to_run();
	choose_next_thread_to_run();
	choose_next_thread_to_run();
	choose_next_thread_to_run();
	choose_next_thread_to_run();
	choose_next_thread_to_run();

	return 0;
}
