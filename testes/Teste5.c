/*

Teste Join. 
	

*/


#include "../include/support.h"
#include "../include/cthread.h"
#include <stdio.h>

csem_t semaforo;
int tcb1,tcb2,tcb3,tcb4;
int fim_semaforo = 0;
int tcb4_rodou = 0;
int tcb2_rodou = 0;


void* func1(void *i){
while(fim_semaforo == 0)
	cyield();
printf("Eu sou a thread 1 e vou dar join na thread 3.\n");
tcb2_rodou = 0;
if(cjoin(tcb3)==0){
	printf("Eu sou a thread 1 depois que a thread 3 acabou. Vou acabar também.\n");
	return NULL;
	}
else{
	printf("ERRO NO JOIN\n");
	}
return NULL;
}
void* func2(void *i){
printf("Eu sou a thread 2 Solicitando acesso ao recurso\n");
cwait(&semaforo);
printf("Eu sou a thread 2 com o recurso e vou dar yield()\n");
tcb2_rodou = 1;
cyield();
while(tcb2_rodou == 1)
	cyield();

printf("Eu sou a thread 2 e vou liberar o recurso e acabar.\n");
csignal(&semaforo);
return NULL;
}
void* func3(void *i){
while(tcb4_rodou == 0)
	cyield();
printf("Eu sou a thread 3 depois da 4 ser bloqueada por semaforo e vou dar join nela.\n");
fim_semaforo = 1;
if(cjoin(tcb4)==0){
	printf("Eu sou a thread 3 depois que a thread 4 acabou. Vou acabar também.\n");
	return NULL;
	}
else{
	printf("ERRO NO JOIN\n");
	}
return NULL;
}
void* func4(void *i){
while(tcb2_rodou == 0)
	cyield();
printf("Eu sou a thread 4 pedindo acesso ao recurso e sendo bloqueada.\n");
tcb4_rodou = 1;
cwait(&semaforo);
printf("Eu sou a thread 4 finalmente ganhando o recurso, vou libera-lo e acabar.\n");
csignal(&semaforo);
return NULL;
}

int main()
{
	int i;
	
	if((csem_init(&semaforo,1))!=0)
		return 1;
	
	tcb1 = ccreate(func1, (void *)&i);
	tcb2 = ccreate(func2, (void *)&i);
	tcb3 = ccreate(func3, (void *)&i);
	tcb4 = ccreate(func4, (void *)&i);
	printf("Eu sou a thread Main e vou dar join na thread 1.\n");
	if(cjoin(tcb1)==0)
	{
		printf("Eu sou a thread Main após o termino da thread 1.\n");
		printf("Eu sou a thread Main e vou terminar o programa.\n");
	}
	

	return 1;
}
