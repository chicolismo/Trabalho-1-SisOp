// vim: sw=4 ts=4 sts=4 expandtab foldenable foldmethod=syntax

#ifdef __APPLE__
#define _XOPEN_SOURCE 500 // Carlos: Gambiarra para não exibir avisos de "deprecated" na minha máquina.
#endif

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ucontext.h>
#include <stdio.h>
#include "../include/cthread.h"
#include "../include/cdata.h"
#include "../include/support.h"
#include "../include/queue.h"


//=============================================================================
// Tipos
//=============================================================================
typedef char byte;


//=============================================================================
// Constantes
//=============================================================================

// Faz "cast" um arg para um ponteiro de função void sem argumentos
#define VOID_FUNCTION(arg) (void (*)(void)) (arg)

// ccreate
#define CCREATE_ERROR -1

// csem_init
#define CSEM_INIT_SUCCESS 0
#define CSEM_INIT_ERROR -1

// cidentify
#define CIDENTIFY_SUCCESS 0
#define CIDENTIFY_ERROR -1


//Public Functions use
#define SUCCESS_CODE 0
#define ERROR_CODE -1

// init_ending_ctx
#define INIT_ENDING_CTX_ERROR -1
#define INIT_ENDING_CTX_SUCCESS 0


//==============================================================================
// Globais
//==============================================================================
bool initialized_globals = false;

int current_tid = 0;

// As filas de threads.
// Devem ser ponteiros para podermos usar CreateFila2.
FILA2 *ready[4];          // Quatro filas de aptos
FILA2 *blocked_join;      // Bloqueados esperando outra thread terminar
FILA2 *blocked_semaphor;  // Bloqueados esperando semáforo
TCB_t *running_thread = NULL;    // Executando
ucontext_t *ending_ctx = NULL;


//==============================================================================
// Funções
//==============================================================================
//geracao e destruição de tid/tcb
// Gera uma nova tid
int generate_tid() {
    return ++current_tid;
}
// Destrói a thread.
void cdestroy(TCB_t *thread) {
    running_thread->state = PROCST_TERMINO;
	free(running_thread);  
	running_thread = NULL;
    return;
}


//------------------------------------------------------------------------------
//Funcoes da lista de bloqueados cjoin
//------------------------------------------------------------------------------
int blocked_join_insert(DUPLA_t *thread) { //essa estrutura Duplacjoin está definida em queue.h
    //funcao que insere uma dupla thread,tid esperado na lista de bloq. cjoin.
    //provavelmente a funcao seja so isso.
    return AppendFila2(blocked_join, thread);
}

int blocked_join_remove(DUPLA_t *toremove) { //essa estrutura Duplacjoin está definida em queue.h
    //funcao que remove uma dupla da fila. sera chamado apos encontrar um tid esperado na fila e recuperar a thread
    //bloqueada, pode ser implementada dentro da funcao de get_thread_waiting_for, mas isso eh escolha de quem implementar.
    if (FirstFila2(blocked_join) == 0) {
        do {
            DUPLA_t *value = (DUPLA_t *)GetAtIteratorFila2(blocked_join);
            if (value != NULL)
                if (value == toremove) {
                    return  DeleteAtIteratorFila2(blocked_join);
                }
        }
        while (NextFila2(blocked_join) == 0);

        return ERROR_CODE;


    } // Fila vazia, não É POSSÍVEL REMOVER.
    else {
        return ERROR_CODE;
    }
}

TCB_t *blocked_join_get_thread(int tid) {
    //funcao que verifica a existencia de uma thread na fila de bloqueados por cjoin.
    //retorna um ponteiro para a thread caso a encontre, e um ponteiro NULL caso a thread nao seja encontrada.
    if (FirstFila2(blocked_join) == 0) {
        do {
            DUPLA_t *value = (DUPLA_t *)GetAtIteratorFila2(blocked_join);
            if (value != NULL)
                if (value->blockedThread->tid == tid) {
                    return  value->blockedThread;
                }
        }
        while (NextFila2(blocked_join) == 0);
        return NULL;


    } // Fila vazia, não existe.
    else {
        return NULL;
    }
}

DUPLA_t *blocked_join_get_thread_waiting_for(int tid) { //essa estrutura Duplacjoin está definida em queue.h
    //funcao que procura por um tid esperado na lista de duplas da fila cjoin,
    //pode retornar a thread ou a dupla, pensei na dupla so para ser mais direto a busca. Mas de novo, decisao de implementacao.
    //retorna um ponteiro para a dupla/thread caso exista uma thread bloqueada pelo tid, e um ponteiro NULL caso nao exista

    if (FirstFila2(blocked_join) == 0) {
        do {
            DUPLA_t *value = (DUPLA_t *)GetAtIteratorFila2(blocked_join);
            if (value != NULL)
                if (value->waitedTid == tid) {
                    return  value;
                }
        }
        while (NextFila2(blocked_join) == 0);
        return NULL;


    } // Fila vazia, não existe.
    else {
        return NULL;
    }
}

void debug_print_blocked_list()
{
	DUPLA_t *print;	
	if (FirstFila2(blocked_join) == 0) {
		printf("##################################################\n");
		printf("############### FILA DE BLOQUEADOS ###############\n");
		printf("##################################################\n");
		do
		{
			print = (DUPLA_t *)GetAtIteratorFila2(blocked_join);
			if(print == NULL) break;
			if(print->blockedThread != NULL){
				printf("# tid bloqueado: %d   /   esperando pelo tid: %d  # \n", print->blockedThread->tid, print->waitedTid);
			}
			else {
				printf("# tid bloqueado: atual  / esperando pelo tid: %d # \n",print->waitedTid);
			}
		} while (NextFila2(blocked_join) == 0);
		printf("##################################################\n");
		return;
	}
	return;
}

//------------------------------------------------------------------------------
//Funcoes de Semaforo
//------------------------------------------------------------------------------

int semaphore_queue_insert_thread(csem_t *sem, TCB_t *thread) {
    //função que insere uma thread na fila de bloqueados de um semáforo sem.
    return AppendFila2(sem->fila, thread);
}

int insert_semaphore_on_blocked_semaphor(csem_t *sem) {
    //funcao que insere um semaforo na lista de semaforos criados.
    //retorna 0 quando a insercao e bem sucedida e -1 quando ha erros.

    if (FirstFila2(blocked_semaphor) == 0) {
        do {
            csem_t *value = (csem_t *)GetAtIteratorFila2(blocked_semaphor);
            if (value != NULL && value == sem) {
                //semaforo ja esta na fila, portanto nao ha insercao e retorna codigo de sucesso.
                return SUCCESS_CODE;
            }
        }
        while (NextFila2(blocked_semaphor) == 0);
        //caso o semaforo nao esteja inserido, ele eh entao inserido na fila.
        return AppendFila2(blocked_semaphor, sem);


    }
    else {
        //caso seja o primeiro elemento, o semaforo sem eh simplesmente inserido.
        return AppendFila2(blocked_semaphor, sem);
    }
}

TCB_t *get_first_of_semaphore_queue(csem_t *sem) {
    //funcao que remove e retorna o primeiro elemento da fila de um semaforo
    //retorna um ponteiro para a thread se a funcao for bem sucedida
    //e um ponteiro NULL em caso de thread nao existente.

    if (FirstFila2(sem->fila) == 0) {
        TCB_t *value = (TCB_t *)GetAtIteratorFila2(sem->fila);
        DeleteAtIteratorFila2(sem->fila);
        return value;

    }
    else {
        return NULL;
    }
}

TCB_t *get_thread_from_blocked_semaphor(int tid) {
    //funcao que verifica a existencia de uma thread nas filas de bloqueados dos semaforos
    //NAO REMOVE nenhuma thread, Retorna um ponteiro para a thread se for bem sucedida
    //e um ponteiro NULL em caso de erro

    if (FirstFila2(blocked_semaphor) == 0) {
        do { //iteracao para varrer a lista de semaforos.
            csem_t *value = (csem_t *)GetAtIteratorFila2(blocked_semaphor);
            if (value == NULL) {
                break;    //precisa desse teste pois caso value seja igual a NULL, o ponteiro de
            }
            //value->fila apontara para um endereco nao conhecido e acontecera segmentation fault.
            if (FirstFila2(value->fila) == 0) {
                do { //iteracao para varrer a fila de bloqueados do semaforo
                    TCB_t *value2 = (TCB_t *)GetAtIteratorFila2(value->fila);
                    if (value2 == NULL) {
                        break;    //teste necessario, pois caso value2 seja igual a NULL,
                    }
                    //acontecera segmentation fault no ponteiro value2->tid
                    if (value2->tid == tid) { //encontrou a thread procurada.
                        return value2;
                    }
                }
                while (NextFila2(value->fila) == 0);
            }
        }
        while (NextFila2(blocked_semaphor) == 0);
        //Nao encontrou a thread, retornara um ponteiro NULL
        return NULL;
    }
    else {
        // Fila de semaforos nao existe, retorna ponteiro nulo.
        return NULL;
    }
}

int debug_blocked_semaphor() {
    int i = 1;
    int t;
    //função que imprime a lista de semaforos em detalhes.
    printf("========== DEBUG SEMAPHORE LIST ==========\n");
    if (FirstFila2(blocked_semaphor) == 0) {
        do {
            printf("==                                      ==\n");
            csem_t *value = (csem_t *)GetAtIteratorFila2(blocked_semaphor);
            if (value != NULL) {
                t = 0;
                do {
                    TCB_t *value2 = (TCB_t *)GetAtIteratorFila2(value->fila);
                    if (value2 != NULL) {
                        t++;
                    }
                }
                while (NextFila2(value->fila) == 0);
                printf("== Semaforo %2i. Count = %2i b.threads= %2i==\n", i, value->count, t);
                i++;
            }
        }
        while (NextFila2(blocked_semaphor) == 0);


    }
    else {
        printf("========== NÃO HÁ LISTA SEMÁFORO =========\n");

    }
    printf("========== DEBUG SEMAPHORE LIST ==========\n");
    return 0;
}

//==============================================================================
// Filas de Aptos
//==============================================================================

/*
 * Apende nova thread nas filas de aptos
 */
int ready_push(TCB_t *thread) {
    return AppendFila2(ready[thread->prio], thread);
}

/*
 * Retorna o primeiro elemento de menor prioridade das filas de aptos e remove
 * esse elemento das filas.
 *
 * Caso não haja mais nenhum elemento nas filas, retorna NULL.
 */
TCB_t *ready_shift() {
    TCB_t *th = NULL;
    int i;
    for (i = 0; i < 4; ++i) {
        if (FirstFila2(ready[i])==SUCCESS_CODE) {
            th = (TCB_t *) GetAtIteratorFila2(ready[i]);
            DeleteAtIteratorFila2(ready[i]);
            break;
        }
    }
    return th;
}

/*
 * Retorna um resultado de busca nas filas de aptos.  Caso seja encontrada a
 * thread com o tid correspondente, é retornada uma estrutura contendo o
 * iterador da fila em que a thread se encontra, bem como o número da fila.
 *
 * Caso nenhuma thread com o tid fornecido seja encontrada, a função retorna
 * NULL
 */
FindResult *ready_find(int tid) {
    FindResult *result = NULL;
    int i;
    for (i = 0; i < 4; ++i) {
        FirstFila2(ready[i]);
        do {
            if (((TCB_t *)GetAtIteratorFila2(ready[i]))->tid == tid) {
                result->node = ready[i]->it;
                result->queue_number = i;
            }
        }
        while (NextFila2(ready[i])==0);
    }
    return result;
}

TCB_t *ready_get_thread(int tid) {
    //funçao que retorna uma thread da fila de aptos a partir de uma tid.
    //Nao remove a thread, apenas retorna um ponteiro para ela caso exista, ou NULL caso nao exista
    TCB_t *thread = NULL;
    int i;
    for (i = 0; i < 4; ++i) {
        if(FirstFila2(ready[i])==SUCCESS_CODE){
            do {
                thread = (TCB_t *)GetAtIteratorFila2(ready[i]);
	            if(thread != NULL){	
                	if (thread->tid == tid) {
                        return thread;
		            }
                }
            }
        while (NextFila2(ready[i])==0);
        }
    }
    return NULL;
}


/*
 * Remove o elemento com a "tid" fornecida das filas de aptos e retorna esse
 * elemento.
 *
 * Caso ele não seja encontrado, nada acontece e NULL é retornado.
 */
TCB_t *ready_remove(int tid) {
    FindResult *result = ready_find(tid);
    if (result != NULL) {
        DeleteAtIteratorFila2(ready[result->queue_number]);
        return (TCB_t *)(result->node->node);
    }
    return NULL;
}

//------------------------------------------------------------------------------
//Funcoes de inicialização
//------------------------------------------------------------------------------

int init_main_thread() {
    //TODO: implementar a inicializacao da thread main.

    TCB_t *thread = (TCB_t *)malloc(sizeof(TCB_t));

	thread->tid = current_tid;
	thread->prio = 0; //?????? nao sei qual a prioridade da thread main.
	thread->state = PROCST_EXEC;

	if (((thread->context).uc_stack.ss_sp = malloc(SIGSTKSZ)) == NULL) {
		//Sem espaço na memória para criar a TCB
		return ERROR_CODE;
	}

	(thread->context).uc_stack.ss_size = SIGSTKSZ;
	(thread->context).uc_link = NULL;  //Quando a thread main acaba o programa acaba.
	running_thread = thread;  //Não é inserido em nenhuma fila, a thread simplesmente está em execução.
    
    //Não é necessário usar um makecontext, pois o contexto da thread é a propria main, que está em execução.
	
	return SUCCESS_CODE;
}
void end_thread();
int init_ending_ctx() {
    ending_ctx = malloc(sizeof(ucontext_t));

    if ((getcontext(ending_ctx) != 0) || ending_ctx == NULL) {
        // Deu merda
        return INIT_ENDING_CTX_ERROR;
    }

    ending_ctx->uc_stack.ss_sp = malloc(SIGSTKSZ);
    ending_ctx->uc_stack.ss_size = SIGSTKSZ;
    ending_ctx->uc_link = NULL; // Nada a fazer quando acabar

    makecontext(ending_ctx, end_thread, 0);

    return INIT_ENDING_CTX_SUCCESS;
}

//------------------------------------------------------------------------------
// Esta função inicializa todas as variáveis globais das quais as outras
// funções dependem.  Deve ser chamada nas outras funções, por garantia.
//------------------------------------------------------------------------------
int init() {
    if (!initialized_globals) {

        initialized_globals = true;

        // inicializar a thread main e colocar em executando.
        if (init_main_thread() != SUCCESS_CODE) {
            return ERROR_CODE;
        }

        // inicializar o contexto de finalizacao de thread.
        if (init_ending_ctx() != SUCCESS_CODE) {
            return ERROR_CODE;
        }

        // O tamanho em bytes da struct de fila. Não é possível obter o tamanho a
        // partir do tipo PFILA2, que é um ponteiro. -> é só utilizar sizeof(FILA2).

        // Inicializa as diversas filas de threads
        size_t queue_size = sizeof(struct sFila2);
        int i;
        for (i = 0; i < 4; ++i) {
            ready[i] = malloc(queue_size);
            CreateFila2(ready[i]);
        }
        blocked_join = (FILA2 *)malloc(sizeof(FILA2));

        if (CreateFila2(blocked_join) != SUCCESS_CODE) {
            return ERROR_CODE;
        }

        blocked_semaphor = (FILA2 *)malloc(sizeof(FILA2));
        if (CreateFila2(blocked_semaphor) != SUCCESS_CODE) {
            return ERROR_CODE;
        }

        return SUCCESS_CODE;
    }
    return SUCCESS_CODE;
}


/* 
Função Dispatcher :
*************************************************************
par.c do estudo dirigido: forte inspiração para o dispatcher.
void even(void) {
    [..]  
} 

int main(void)
{

    ucontext_t main_context, even_context;
[...]
 
     * É necessario criar uma estrutura contexto a partir de um molde.
     * O contexto da propria main serve como esse molde. 

    getcontext(&even_context);
    even_context.uc_link          = &main_context;    
    even_context.uc_stack.ss_sp   = even_stack;
    even_context.uc_stack.ss_size = sizeof(even_stack);
    makecontext(&even_context, (void (*)(void)) even, 0); 
    /------------------- A PARTIR DAQUI É UM MINI DISPATCHER ------------------------/
    ret_code = 0;
    getcontext(&main_context);

    if (!ret_code) {
        * Testa a variavel even_finished para diferenciar se a funcaoo getcontext
          anterior retornou via uc_link (se ret_code==1) depois do termino de 
          even ou se ela retornou apos a sua chamada simples 

        ret_code = 1;             
        setcontext(&even_context);  * posiciona o contexto para even
        printf("NUNCA sera executado!\n");
        return(-1);    * nunca sera executado! 
    }
 
    printf("\n\n Terminando a main...\n");
    return 0;
}
*/

int dispatch() {
		
	int swapped_context = 0;

	if (running_thread != NULL) { // se uma thread acabar o running thread é NULL, o teste evita segmentation fault.	
		getcontext(&(running_thread->context));
	}
    
	if (swapped_context == 0) {
        // Somente executará no contexto original, quando a thread está voltando de execução não é executado.
		swapped_context = 1;
		running_thread = ready_shift();

		if (running_thread == NULL || running_thread->tid < 0) {
			//Não existe thread a ser executada ou possui erro em sua geração.
			return ERROR_CODE;
		}
        
		running_thread->state = PROCST_EXEC;
    	setcontext(&(running_thread->context));
	}
	return SUCCESS_CODE;
}
//*********************************
// Função fim de thread
//*********************************
void end_thread() {
    if (running_thread == NULL) {
		//nao há thread em execução
        return;
    }
    
    int tid = running_thread->tid;
    cdestroy(running_thread);
    // Procura pela thread que estava esperando esta aqui terminar.		
    DUPLA_t *waiting_thread = blocked_join_get_thread_waiting_for(tid);

    if (waiting_thread == NULL) {
        //não há threads esperando pela tid.
        dispatch();
    } else {
        //Há uma thread esperando pelo fim dessa tid.

        blocked_join_remove(waiting_thread);
        waiting_thread->blockedThread->state = PROCST_APTO;

        ready_push(waiting_thread->blockedThread);
        dispatch();
	}
}


//==============================================================================
// Funções da biblioteca cthread
//==============================================================================

/*
 * =============================================================================
 * Sobre o ucontext (segundo a man page)
 * =============================================================================
 *
 * UCONTEXT(3)    BSD Library Functions Manual        UCONTEXT(3)
 *
 * NAME
 *     ucontext -- user thread context
 *
 * LIBRARY
 *     Standard C Library (libc, -lc)
 *
 * SYNOPSIS
 *     #include <ucontext.h>
 *
 * DESCRIPTION
 *
 *     The ucontext_t type is a structure type suitable for holding the
 *     context for a user thread of execution.  A thread's context includes its stack,
 *     saved registers, and list of blocked signals.
 *
 *     The ucontext_t structure contains at least these fields:
 *
 *     ucontext_t *uc_link      context to assume when this one returns
 *     sigset_t uc_sigmask      signals being blocked
 *     stack_t uc_stack         stack area
 *     mcontext_t uc_mcontext   saved registers
 *
 *     The uc_link field points to the context to resume when this context's entry
 *     point function returns.  If uc_link is equal to NULL, then the process exits
 *     when this context returns.
 *
 *     The uc_mcontext field is machine-dependent and should be treated as opaque by
 *     portable applications.
 *
 *     The following functions are defined to manipulate ucontext_t structures:
 *
 *     int getcontext(ucontext_t *);
 *     int setcontext(const ucontext_t *);
 *     void makecontext(ucontext_t *, void (*)(void), int, ...);
 *     int swapcontext(ucontext_t *, const ucontext_t *);
 *
 * SEE ALSO
 *     sigaltstack(2), getcontext(3), makecontext(3)
 *
 * BSD             September 10, 2002                BSD
 *
 */

/*
 * Cria uma nova thread.
 *
 * Parâmetros:
 *  start: ponteiro para a função que a thread executará.
 *
 *  arg: um parâmetro que pode ser passado para a thread na sua criação.
 *  (Obs.: é um único parâmetro. Se for necessário passar mais de um valor
 *  deve-se empregar um ponteiro para uma struct)
 *
 *  prio: prioridade com que deve ser criada a thread.
 *
 * Retorno:
 *  Quando executada corretamente: retorna um valor positivo, que representa o
 *  identificador da thread criada, caso contrário, retorna um valor negativo.
 */
//------------------------------------------------------------------------------
// Cria uma nova thread.
//
// Parmetros:
//  start: ponteiro para a função que a thread executará.
//
//  arg: um parâmetro que pode ser passado para a thread na sua criação.
//  (Obs.: é um único parâmetro. Se for necessário passar mais de um valor
//  deve-se empregar um ponteiro para uma struct)
//
//  prio: prioridade com que deve ser criada a thread.
//
// Retorno:
//  Quando executada corretamente: retorna um valor positivo, que representa o
//  identificador da thread criada, caso contrário, retorna um valor negativo.
//------------------------------------------------------------------------------
int ccreate(void *(*start)(void *), void *arg, int priority) {
    init();

    int new_tid = generate_tid();

    TCB_t *thread = (TCB_t *) malloc(sizeof(TCB_t));

    thread->tid = new_tid;
    thread->prio = priority;
    thread->state = PROCST_CRIACAO;

    getcontext(&(thread->context));

    if ((thread->context.uc_stack.ss_sp = malloc(SIGSTKSZ)) == NULL) {
        return CCREATE_ERROR;
    }

    // NOTE: Descobrir o que fazer com o resto do contexto.
    // th->context.uc_link  ->> contexto de finalização
    // th->context.uc_sigmask ->> man: uc_sigmask is the set of signals blocked in this context, acho que nao precisa setar nada.
    // th->context.uc_stack;  ->> pilha usada pelo contexto, precisa um set em uc_stack.ss_size
    // th->context.uc_mcontext ->> man: uc_mcontext is the machine-specific representation of the saved
    // context, that includes the calling thread's machine registers. Acho que a propria funcao de getcontext ja faz o set.
    thread->context.uc_stack.ss_size = SIGSTKSZ;
    thread->context.uc_link = ending_ctx;
    makecontext(&(thread->context), VOID_FUNCTION(start), 1, arg);

    ready_push(thread);

    return new_tid;
}

int csetprio(int tid, int prio) {
    init();
    bool thread_in_ready = false;

    if (prio < 0 || prio > 3) {
        //prioridade nao esta no intervalo [0,3]
        return ERROR_CODE;
    }
    TCB_t *thread = NULL;
    if ((thread = blocked_join_get_thread(tid)) == NULL) {
        if ((thread = get_thread_from_blocked_semaphor(tid)) == NULL) {
            if ((thread = ready_get_thread(tid)) == NULL) {
                //Thread a ser modificada não existe.
                return ERROR_CODE;
            }
            else {//thread encontrada na fila de aptos.
                thread_in_ready = true;
            }
        }
    }
    // A thread existe e est[a apontada pelo ponteiro thread.

    if (thread_in_ready) {
        //TODO: remoção da fila de aptos, troca de prioridade e reinserçao na fila de aptos.
	thread = ready_remove(thread->tid);
	if(thread != NULL){
		thread->prio = prio;
		ready_push(thread);
	}
    }
    else {
        thread->prio = prio;
    }
    return SUCCESS_CODE;
}

int cyield() {
    init();

    running_thread->state = PROCST_APTO;
    ready_push(running_thread);
    return dispatch();
}

int cjoin(int tid) {
    init();
    if (blocked_join_get_thread_waiting_for(tid) != NULL){
	//A thread ja esta sendo esperada, retorna erro
	return ERROR_CODE;
    }
    TCB_t *thread = NULL;
    if ((thread = blocked_join_get_thread(tid)) == NULL) {
        if ((thread = get_thread_from_blocked_semaphor(tid)) == NULL) {
            if ((thread = ready_get_thread(tid)) == NULL) {
               	//Thread nao existe, retorna erro.
		return ERROR_CODE;
	    }
        }
    }
    //Thread que se deseja esperar o termino existe, nao e esperada e esta apontada pelo ponteiro thread.
    DUPLA_t *new_cjoin = (DUPLA_t *) malloc(sizeof(DUPLA_t));
    new_cjoin->waitedTid = tid;
    new_cjoin->blockedThread = running_thread;
    running_thread->state = PROCST_BLOQ;
	
    blocked_join_insert(new_cjoin);

    return dispatch();
}

//------------------------------------------------------------------------------
// Inicializa um semáforo
//------------------------------------------------------------------------------
int csem_init(csem_t *sem, int count) {
    init();
    if (sem == NULL) {
        //printf("Nao e possivel inicializar um ponteiro para um semaforo nulo.\n");
        return CSEM_INIT_ERROR;
    }
    if (sem->fila != NULL) {
        //printf("Nao e possivel inicializar um semaforo ja inicializado.\n");
        return CSEM_INIT_ERROR;
    }
    //printf("Inicializando a contagem do semáforo\n");
    sem->count = count;

    //printf("Inicializando a fila referente ao semáforo.\n");
    sem->fila = (FILA2 *)malloc(sizeof(FILA2));

    //insere o semáforo na lista de semáforos
    //printf("inserindo semaforo na fila de semaforos.\n");
    if (insert_semaphore_on_blocked_semaphor(sem) == 0) {
        return CreateFila2(sem->fila);
    }
    else {
        return ERROR_CODE;
    }
}

/*
    Tranca o semaforo se o mesmo ainda nao esta trancado, se ja estiver trancado
    coloca a thread em uma fila de bloqueados, aguardando a liberacao do recurso
*/
int cwait(csem_t *sem) {
    init();

    if ((sem == NULL) || (sem->fila == NULL)) {
        // Não é possivel dar wait em um ponteiro para um semaforo nulo ou cuja fila não esteja inicializada.
        return ERROR_CODE;
    }

    if (sem->count > 0) {
        // O recurso NÃO ESTÁ sendo usado, então a thread vai usá-lo.
        sem->count -= 1;
        return SUCCESS_CODE;
    }
    else {
        // O recurso JÁ ESTÁ sendo usado, então precisamos bloquear a thread.
        sem->count -= 1;
        running_thread->state = PROCST_BLOQ;
        semaphore_queue_insert_thread(sem, running_thread);
        dispatch();
    }
    return SUCCESS_CODE;
}

/*
 * Destrava o semaforo, e libera as threads bloqueadas esperando pelo recurso
 */
int csignal(csem_t *sem) {
    init();
    if ((sem == NULL) || (sem->fila == NULL)) {
        //Não é possivel dar signal em um ponteiro para um semaforo nulo ou cuja fila não esteja inicializada.
        return ERROR_CODE;
    }

    sem->count += 1;
    TCB_t *thread = (TCB_t *)get_first_of_semaphore_queue(sem);
    if (thread != NULL) {//existia uma thread bloqueada pelo semaforo.
        //estado da thread e modificado para APTO
        thread->state = PROCST_APTO;
        return ready_push(thread);
    }
    else {
        //O semaforo esta livre. Segue execucao.
        return SUCCESS_CODE;
    }
}

/*
 * Copia o nome e número do cartão dos alunos para um endereço fornecido
 *
 * Params:
 *  char *name :: Endereço aonde serão copiados os nomes dos alunos
 *  int size :: Limite de caracteres que serão copiados para o endereço
 */
int cidentify(char *name, int size) {
    char *names =
        "Carlos Pinheiro       -- 109910\n"
        "Bruno Feil            -- 216631\n"
        "Hugo Constantinopolos -- 208897";

    return strncpy(name, names, size) == 0 ? CIDENTIFY_SUCCESS : CIDENTIFY_ERROR;
}

