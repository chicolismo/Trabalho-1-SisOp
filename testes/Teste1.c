/*

	Teste de troca de threads sem entrar no estado de bloqueados, só dando yield.

*/

#include "../include/support.h"
#include "../include/cthread.h"
#include <stdio.h>

int livre1 = 0;
int livre2 = 0;
int livre1rodou = 0;
int livre2rodou = 0;

int thread1,thread2,thread3;

void* func3(void *i){
while(livre1rodou < 5 && livre2rodou < 5){
	printf("eu sou a thread3 resetando a resetando a prioridade da thread 1 e 2\n");
	csetprio(thread1,0);
	csetprio(thread2,0);
	cyield();
	}
return NULL;
}

void* func1(void *i){
while(1){
	
	printf("eu sou a thread 1 reduzindo a prioridade da thread 2\n");
	csetprio(thread2,1);
	livre1rodou++;
	if(livre1rodou == 5){
		printf("eu sou a thread 1 e já rodei o bastante\n");
		return NULL;
		}
	else{
		cyield();
		}
	
	
	}

return NULL;
}

void* func2(void *i){
while(1 + 1 == 2){
	int prio = 0;
	printf("eu sou a thread 2 alterando minha prioridade\n");
	csetprio(thread2, prio);
	prio++;
	if(prio>3)prio = 0;

	livre2rodou++;
	
	if(livre2rodou == 5){
		printf("eu sou a thread 2 e já rodei o bastante\n");
		return NULL;
		}

	else{
		cyield();
		}
	}
	return NULL;
}

int main()
{

	thread1 = ccreate(func1,(int*) 2,0);
	thread2 = ccreate(func2,(int*) 1,0);
	thread3 = ccreate(func3,(int*) 0,0);
	
	if(cjoin(thread3) == 0)
	{
		printf("fim\n");
		return 1;
	}
	return 1;
}
