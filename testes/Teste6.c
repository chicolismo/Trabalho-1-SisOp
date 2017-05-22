/*

Teste6.c
É possivel criar qualquer situação dentro da biblioteca e testar suas funcionalidades.


*/


#include "../include/support.h"
#include "../include/cthread.h"
#include <stdio.h>

csem_t *semaforo[256];
int tcb1;
int tcb[256];
int t = 2;
int s = 0;

void* func1(void *i){
    int c;
    int n = (int)i;
    int run = 1;
    int prio;
    int tid;
    int count;
    int sem;
    while(run){
        printf("#############################################\n");
        printf("#                                           #\n");
        printf("# Thread %d em execução                     #\n",n);
        printf("# Selecione uma das opções:                 #\n");
        printf("# Pressione a tecla correspondente          #\n");
        printf("# 1 - Criar nova thread.                    #\n");
        printf("# 2 - Cyield                                #\n");
        printf("# 3 - Alterar a prioridade de uma thread    #\n");
        printf("# 4 - Cjoin                                 #\n");
        printf("# 5 - criar um semáforo                     #\n");
        printf("# 6 - Solicitar recurso de um semáforo      #\n");
        printf("# 7 - Liberar recurso de um semáforo        #\n");
	printf("# 10 - Exibir fila de aptos                 #\n");
	printf("# 20 - Exibir bloqueados cjoin              #\n");
	printf("# 30 - Exibir bloqueados semaforo           #\n");
	printf("#                                           #\n");
        printf("# 0 - finalizar thread                      #\n");
        printf("# ");
        scanf("%d", &c);
        switch(c){
            case 1:   printf("# Digite a prioridade da thread:");
                        scanf("%d", &prio);
                        tcb[t] = ccreate(func1, (void *)t,prio);
                        t++;
                        break;
            case 2:   cyield();
                        break;
            case 3:   printf("# Digite o tid da thread:");
                        scanf("%d", &tid);
                        printf("# Digite a nova prioridade:");
                        scanf("%d", &prio);
                        csetprio(tid,prio);
                        break;
            case 4:   printf("# Digite o tid da thread:");
                        scanf("%d", &tid);
                        cjoin(tid);
                        break;
            case 5:   printf("# Digite o count do semaforo:");
                        scanf("%d", &count);
                        semaforo[s] = (csem_t *) malloc(sizeof(csem_t));
                        csem_init(semaforo[s], count);
                        printf("# Criado semáforo %d                        #\n", s);
                        s++;
                        break;
            case 6:   printf("# Número do semaforo:");
                        scanf("%d", &sem);
                        cwait(semaforo[sem]);
                        break;
            case 7:   printf("# Número do semaforo:");
                        scanf("%d", &sem);
                        csignal(semaforo[sem]);
                        break;
	    case 10:	debug_ready();
			break;
	    case 20:	debug_print_blocked_list();
			break;
	    case 30:	debug_blocked_semaphor();
			break;
			
            case 0:   run = 0;
                        break;
            default:    printf("# Opção invalida.                           #\n");
                        break;
	}
	do{
		c = getchar();
    }while(c != '\n' && c != EOF);
    
	printf("Aperte ENTER para continuar. \n");
    getchar();
	
    }
    return NULL;
}


int main()
{
	tcb1 = ccreate(func1, (int *) 1, 0);

	printf("#############################################\n");
	printf("#                                           #\n");
	printf("#   Teste cthread.c                         #\n");
	printf("#                                           #\n");
	printf("#   Selecione as opções e teste             #\n");
	printf("#   a biblioteca.                           #\n");
	printf("#   Obs: A thread main só terminará após    #\n");
	printf("#   o final da thread 1.                    #\n");
	printf("#                                           #\n");
	printf("#   O número máximo de threads é 256.       #\n");
	printf("#   O número máximo de semaforos é 256.     #\n");
	printf("#############################################\n");
	if(cjoin(tcb1)==0)
	{
		printf("Eu sou a thread Main após o termino da thread 1.\n");
		printf("Eu sou a thread Main e vou terminar o programa.\n");
	}


	return 1;
}
