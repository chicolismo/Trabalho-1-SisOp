#include<stdlib.h>
#include <stdio.h>

#include "../include/support.h"
#include "../include/cthread.h"

//TESTE DA INICIALIZAÇÂO DE SEMAFOROS. A INICIALIZAÇÃO TAMBÉM INSERE O SEMÁFORO NA LISTA DE SEMÁFORO

csem_t sem1;
csem_t sem2;
csem_t sem3;


int main(){

	if(csem_init(&sem1, 1)==0){
		printf("semaforo 1 inicializado.\n");
	}else{
		printf("erro inicializacao semaforo 1\n");
		return 0;
	}

	if(csem_init(&sem2, 2)==0){
		printf("semaforo 2 inicializado.\n");
	}else{
		printf("erro inicializacao semaforo 2\n");
		return 0;
	}

	if(csem_init(&sem3, 3)==0){
		printf("semaforo 3 inicializado.\n");
	}else{
		printf("erro inicializaca semaforo 3\n");
		return 0;
	}
	debug_blocked_semaphor();
return 0;
}

