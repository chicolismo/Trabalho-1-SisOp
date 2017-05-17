#include <stdio.h>
#include <stdlib.h>

#include "../include/support.h"
#include "../include/cthread.h"

int soma = 0;
csem_t sem;

void func(void *arg) {

	int threadNum = *((int *)arg);

	printf("[T%d] Começado e tentando entrar na SC (sem.count = %d).\n", threadNum, sem.count);

	cwait(&sem);
	soma += 1;
	printf("\n[T%d] Entrei na SC e vou dar um cyield (sem.count = %d).\n", threadNum, sem.count);
	cyield();
	printf("\n[T%d] Vou liberar a SC.\n", threadNum);
	csignal(&sem);
	printf("[T%d] Semáforo count depois de liberar: %d.\n", threadNum, sem.count);
	printf("[T%d] Terminando.\n", threadNum);

	return;
}

int main	(int argc, char *argv[]) {

	int t1 = 1, t2 = 2, t3 = 3, t4 = 4;
	int	id1, id2, id3, id4;

    csem_init(&sem, 1);

    id1 = ccreate((void*(*)(void*))func, (void *)&t1);
    id2 = ccreate((void*(*)(void*))func, (void *)&t2);
    id3 = ccreate((void*(*)(void*))func, (void *)&t3);
    id4 = ccreate((void*(*)(void*))func, (void *)&t4);

	cjoin(id1); cjoin(id2); cjoin(id3); cjoin(id4);

    printf("[MAIN] voltando para terminar Resultado: soma = %d.\n", soma);

    return 0;
}