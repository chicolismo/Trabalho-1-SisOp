#ifdef __APPLE__
#define _XOPEN_SOURCE 500 // Carlos: Gambiarra para não exibir avisos de "deprecated" na minha máquina.
#endif

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ucontext.h>
#include "../include/cthread.h"
#include "../include/cdata.h"
#include "../include/support.h"
#include "../include/queue.h"

//=============================================================================
// Tipos
//=============================================================================
typedef char byte;
typedef enum State { NEW, READY, RUNNING, BLOCKED, TERMINATED } State;


//=============================================================================
// Constantes
//=============================================================================

#define STACK_SIZE (sizeof(byte) * SIGSTKSZ)

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


//==============================================================================
// Globais
//==============================================================================
bool initialized_globals = false;

//------------------------------------------------------------------------------
// As filas de threads.
// Devem ser ponteiros para podermos usar CreateFila2.
//------------------------------------------------------------------------------
PFILA2 ready[4];          // Quatro filas de aptos
PFILA2 blocked_join;      // Bloqueados esperando outra thread terminar
PFILA2 blocked_semaphor;  // Bloqueados esperando semáforo
TCB_t *running_thread;    // Executando

//==============================================================================
// Funções
//==============================================================================


//------------------------------------------------------------------------------
//Funcoes de Semaforo
//------------------------------------------------------------------------------

int insert_semaphore_on_blocked_semaphor(csem_t *sem){
    //funcao que insere um semaforo na lista de semaforos criados.
    return 0;
}

TCB_t* get_first_of_semaphore_queue(csem_t *sem) {
    //funcao que remove e retorna o primeiro elemento da fila de um semaforo
    return 0;	
}

TCB_t* get_thread_from_blocked_semaphor(int tid) {
    //funcao que verifica a existencia de uma thread nas filas de bloqueados dos semaforos
    //NAO REMOVE nenhuma thread, somente retorna um ponteiro a ela
    return 0;	
}


//------------------------------------------------------------------------------
// Esta função inicializa todas as variáveis globais das quais as outras
// funções dependem.  Deve ser chamada nas outras funções, por garantia.
//------------------------------------------------------------------------------
void init() {
    if (initialized_globals) {
        return;
    }

    // TODO: Criar a thread da main e colocar em executando.

    // O tamanho em bytes da struct de fila. Não é possível obter o tamanho a
    // partir do tipo PFILA2, que é um ponteiro.

    // Inicializa as diversas filas de threads
    size_t queue_size = sizeof(struct sFila2);
    int i;
    for (i = 0; i < 4; ++i) {
        ready[i] = malloc(queue_size);
        CreateFila2(ready[i]);
    }
    blocked_join = malloc(queue_size);
    CreateFila2(blocked_join);

    blocked_semaphor = malloc(queue_size);
    CreateFila2(blocked_semaphor);

    initialized_globals = true;
}


//------------------------------------------------------------------------------
// Inicializa um semáforo
//------------------------------------------------------------------------------
int csem_init(csem_t *sem, int count) {
    // TODO: Descobrir como seria um erro.
    if (sem == NULL) {
        //Nao e possivel inicializar um ponteiro para um semaforo nulo.
        return CSEM_INIT_ERROR;
    }
    if (sem->fila != NULL){
        //nao e possivel inicializar um semaforo ja inicializado
        return CSEM_INIT_ERROR;
    }
    // Inicializa a contagem do semáforo
    sem->count = count;
    
    // Inicializa a fila referente ao semáforo
    sem->fila = (FILA2)malloc(sizeof(FILA2)); // PFILA2 é um tipo ponteiro
    return CreateFila2(sem->fila);
}


//------------------------------------------------------------------------------
// Gera uma nova tid
//------------------------------------------------------------------------------
int current_tid = 0;
int generate_tid() {
    return ++current_tid;
}

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
    // Será que esta pilha deveria ser compartilhada???
    byte *context_stack = malloc(STACK_SIZE);

    if (!initialized_globals) {
        init();
    }

    TCB_t *th = malloc(sizeof(TCB_t));
    if (th == NULL) {
        return CCREATE_ERROR;
    }
    th->state = NEW;
    th->tid = generate_tid();
    th->prio = priority;

    // Inicializa o contexto da thread
    getcontext(&th->context);
    // TODO: Verificar se é isso mesmo
    // Descobrir o que fazer com o resto do contexto.
    //
    // th->context.uc_link;
    // th->context.uc_sigmask;
    // th->context.uc_stack;
    // th->context.uc_mcontext;
    //
    // Associa uma função ao contexto
    makecontext(&th->context, VOID_FUNCTION(start), 1, arg);

    // Depois de ser criada, a thread entre para sua fila de aptos
    // correspondente
    FILA2 *queue = ready[priority];
    th->state = READY;
    push_ready(th);

    return th->tid;
}


//------------------------------------------------------------------------------
// Destrói a thread.
// NOTE: Talvez não seja necessária
//------------------------------------------------------------------------------
TCB_t *cdestroy(TCB_t *thread) {
    free(thread);
    thread = NULL;
    return thread;
}

/*
	Tranca o semaforo se o mesmo ainda nao esta trancado, se ja estiver trancado
	coloca a thread em uma fila de bloqueados, aguardando a liberacao do recurso
*/
int cwait(csem_t *sem) {
	
	if ((sem == NULL) || (sem->fila == NULL)) {
		// Não é possivel dar wait em um ponteiro para um semaforo nulo ou cuja fila não esteja inicializada.
		return ERROR_CODE;
	}
	
	if (sem->count > 0) {
		// O recurso NÃO ESTÁ sendo usado, então a thread vai usá-lo.
		sem->count -= 1;
		return SUCCESS_CODE;
	} else {
		// O recurso JÁ ESTÁ sendo usado, então precisamos bloquear a thread.
		sem->count -= 1;
        //altera o estado da thread ativa para bloqueado.
		//insere na fila de bloqueados do semaforo a thread ativa.

    	//executa o escalonador.
	}
}
/*
	Destrava o semaforo, e libera as threads bloqueadas esperando pelo recurso
*/
int csignal(csem_t *sem) {
	if ((sem == NULL) || (sem->fila == NULL)) {
		//Não é possivel dar signal em um ponteiro para um semaforo nulo ou cuja fila não esteja inicializada.
		return ERROR_CODE;
	
	}

	sem->count += 1;
	//TCB_t *thread = (TCB_t *)primeira_thread_bloqueada_no_semaforo(sem);	
	/*if (ha thread bloqueada / sem->fila != NULL) {
		estado da thread e modificado para APTO. --- thread->state = PROCST_APTO;
		return funcao_de_inserir_na_fila_de_aptos(thread);
	} else {
		O semaforo esta livre. Segue execucao.
		return SUCCESS_CODE;
	}*/
}


//------------------------------------------------------------------------------
// Copia o nome e número do cartão dos alunos para um endereço fornecido
//
// Params:
//  char *name :: Endereço aonde serão copiados os nomes dos alunos
//  int size :: Limite de caracteres que serão copiados para o endereço
//------------------------------------------------------------------------------
int cidentify(char *name, int size)
{
    // TODO: Incluir os dados do último integrante.
    char *names =
        "Carlos Pinheiro -- 109910\n"
        "Bruno Feil      -- 216631";

    return strncpy(name, names, size) == 0 ? CIDENTIFY_SUCCESS : CIDENTIFY_ERROR;
}




//==============================================================================
// Filas de Aptos
//==============================================================================
int push_ready(TCB_t *thread)
{
    PFILA2 queue = ready[thread->prio];
    return AppendFila2(queue, (void *) thread);
}

//------------------------------------------------------------------------------
// Retorna o primeiro elemento de menor prioridade das filas de aptos e remove
// esse elemento das filas.
//
// Caso não haja mais nenhum elemento nas filas, retorna NULL.
//------------------------------------------------------------------------------
TCB_t *shift_ready()
{
    TCB_t *th = NULL;
    int i;
    for (i = 0; i < 4; ++i) {
        if (FirstFila2(ready[i])) {
            th = GetAtIteratorFila2(ready[i]);
            DeleteAtIteratorFila2(ready[i]);
            break;
        }
    }
    return th;
}

//------------------------------------------------------------------------------
// Remove o elemento com a "tid" fornecida das filas de aptos e retorna esse
// elemento.
// 
// Caso ele não seja encontrado, nada acontece e NULL é retornado.
//------------------------------------------------------------------------------
TCB_t *remove_ready(int tid)
{
    TCB_t *th = NULL;
    int i;
    for (i = 0; i < 4; ++i) {
        FirstFila2(ready[i]);
        do {
            th = GetAtIteratorFila2(ready[i]);
            if (th->tid == tid) {
                DeleteAtIteratorFila2(ready[i]);
                return th;
            }
        } while (NextFila2(ready[i]));
    }
    return th;
}

