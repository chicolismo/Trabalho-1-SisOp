/*

	TESTE PARA AS FUNÇÕES E ESTRUTURAS QUE POSSIBILITAM A FILA DE APTOS.

*/

#include <stdio.h>
#include <stdlib.h>

#include "../include/support.h"
#include "../include/cdata.h"
#include "../include/cthread.h"



int main(int argc, char const *argv[])
{	 
	init();
	
	TCB_t* tcb[100];

	int i;
	for(i = 0; i<100; i++) {
		tcb[i] = (TCB_t *)malloc(sizeof(TCB_t));
		tcb[i]->tid = i;

		tcb[i]->prio = i%4;
		
	}

	DEBUG(("ready_push(tcb[10]): %d\n", ready_push(tcb[0]))); 
	DEBUG(("ready_push(tcb[15]) %d\n", ready_push(tcb[1])));
	DEBUG(("ready_push(tcb[5]): %d\n", ready_push(tcb[5])));
	DEBUG(("ready_push(tcb[5]): %d\n", ready_push(tcb[5])));
	DEBUG(("ready_push(tcb[8]): %d\n", ready_push(tcb[8])));
	DEBUG(("ready_push(tcb[8]): %d\n", ready_push(tcb[8])));
	DEBUG(("ready_push(tcb[51]): %d\n", ready_push(tcb[51])));
	DEBUG(("ready_push(tcb[52]): %d\n", ready_push(tcb[52])));
	DEBUG(("ready_push(tcb[53]): %d\n", ready_push(tcb[53])));
	DEBUG(("ready_push(tcb[54]): %d\n", ready_push(tcb[54])));


	ready_shift();
	ready_shift();
	ready_shift();
	ready_shift();
	ready_shift();
	ready_shift();
	ready_shift();
	ready_shift();
	ready_shift();
	ready_shift();
	ready_shift();
	ready_shift();
	ready_shift();

	return 0;
}
