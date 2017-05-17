/*

teste de semaforos, cyield e cjoin com ordem bem aleatória de execução.
	

*/

#include "../include/support.h"
#include "../include/cthread.h"
#include <stdio.h>

csem_t semaforo;
int thread1,thread2,thread3, thread4;
int chama_func4 = 0;

void* func4(void *i)
{
	while(1){
		chama_func4++;
		cyield();
		if(chama_func4 == 100)
		{
			printf("Provavelmente não há thread apta. O semáforo sera desbloqueado.\n");
			csignal(&semaforo);
			chama_func4 = 0;
		}
	}
return NULL;
}

void* func1(void *i){
printf("A thread 1 vai pedir acesso ao recurso\n");
cwait(&semaforo);
printf("A thread 1 tem acesso ao recurso\n");
printf("A thread 1 vai esperar o termino da thread 2\n");
if(cjoin(thread2) == 0){
	printf("A thread 2 acabou e a 1 voltou a executar.\n");
	printf("A thread 1 vai liberar o recurso e acabar.\n");
	csignal(&semaforo);
	return NULL;
	}
else{
	printf("A thread 2 já acabou ou já é esperada.\n");
	printf("A thread 1 liberar o recurso.\n");
	csignal(&semaforo);
	printf("A thread 1 vai esperar o termino da thread 2.\n");
	if(cjoin(thread2) == 0){
		printf("A thread 2 acabou e a 1 voltou a executar.\n");
		printf("A thread 1 vai acabar.\n");
		return NULL;
		}
	else{
		printf("A thread 2 já acabou ou já é esperada.\n");
		printf("A thread 1 vai ceder seu espaço de execução (yield)\n");
		cyield();
		printf("A thread 1 vai acabar.\n");
		return NULL;
		}
	}	
return NULL;	
}
void* func2(void *i){
printf("A thread 2 vai pedir acesso ao recurso\n");
cwait(&semaforo);
printf("A thread 2 tem acesso ao recurso\n");
printf("A thread 2 vai liberar o recurso\n");
csignal(&semaforo);
printf("A thread 2 vai ceder seu espaço de execução (yield)\n");
cyield();
printf("A thread 2 vai pedir acesso ao recurso\n");
cwait(&semaforo);
printf("A thread 2 tem acesso ao recurso\n");
printf("A thread 2 vai liberar o recurso\n");
csignal(&semaforo);
printf("A thread 2 vai ceder seu espaço de execução (yield)\n");
cyield();
cwait(&semaforo);
printf("A thread 2 vai acabar\n");
csignal(&semaforo);
return NULL;
}


void* func3(void *i){
printf("A thread 3 vai pedir acesso ao recurso\n");
cwait(&semaforo);
printf("A thread 3 tem acesso ao recurso\n");
printf("A thread 3 vai esperar o termino da thread 2.\n");
if(cjoin(thread2)==0){
	printf("A thread 2 acabou e a 3 voltou a executar.\n");
	printf("A thread 3 vai esperar o termino da thread 1.\n");
	if(cjoin(thread1)==0){
		printf("A thread 1 acabou e a 3 voltou a executar.\n");
		printf("A thread 3 vai liberar o recurso\n");
		csignal(&semaforo);
		printf("A thread 3 vai acabar.\n");
		return NULL;
		}
	else{
		printf("A thread 1 já acabou ou já é esperada.\n");
		printf("A thread 3 vai ceder seu espaço de execução (yield)\n");
		cyield();
		printf("A thread 3 vai esperar o termino da thread 2.\n");
		if(cjoin(thread1)==0){
			printf("A thread 2 acabou e a 3 voltou a executar.\n");
			printf("A thread 3 vai liberar o recurso e acabar.\n");
			csignal(&semaforo);
			return NULL;}	
		printf("A thread 3 vai ceder seu espaço de execução (yield)\n");
		cyield();	
		printf("A thread 3 vai liberar o recurso e acabar.\n");
		csignal(&semaforo);
		return NULL;
		}
	}
else{
	printf("A thread 2 já acabou ou já é esperada.\n");
	printf("A thread 3 vai liberar o recurso\n");
	csignal(&semaforo);	
	printf("A thread 3 vai ceder seu espaço de execução (yield)\n");
	cyield();
	printf("A thread 3 vai pedir acesso ao recurso\n");
	cwait(&semaforo);
	printf("A thread 3 vai ceder seu espaço de execução (yield)\n");
	cyield();
	printf("A thread 3 vai liberar o recurso e acabar.\n");
	csignal(&semaforo);
	return NULL;
	}
	
		
return NULL;	
}



int main()
{
	int i = 42;
	printf("comecou\n");
	thread1 = ccreate(func1, (void *)&i);
	thread2 = ccreate(func2, (void *)&i);
	thread3 = ccreate(func3, (void *)&i);
	thread4 = ccreate(func4, (void *)&i);
	printf("criou thread\n");
	csem_init(&semaforo, 1);
	printf("iniciou semaforo\n");
	
	printf("thread MAIN dará join na thread 3\n");
	if(cjoin(thread3) == 0)
	{
		printf("FIM DA THREAD MAIN\n");
		return 1;
	}
	return 1;
}
