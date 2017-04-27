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

// As filas de threads
// Existem 4 filas de cada tipo, uma para cada prioridade
PFILA2 ready_queues[4];      // Aptos
PFILA2 blocked_queues[4];    // Bloqueados
PFILA2 terminated_queues[4]; // Terminados
TCB_t *running_thread;       // Executando

TCB_t main_thread; // A thread da "main"

ucontext_t *current_context; // O contexto atual

//==============================================================================
// Funções
//==============================================================================

//------------------------------------------------------------------------------
// Esta função inicializa todas as variáveis globais das quais as outras
// funções dependem.  Deve ser chamada nas outras funções, por garantia.
//------------------------------------------------------------------------------
void init() {
    if (initialized_globals) {
        return;
    }

    // O tamanho em bytes da struct de fila. Não é possível obter o tamanho a
    // partir do tipo PFILA2, que é um ponteiro.
    size_t queue_size = sizeof(struct sFila2);

    // Inicializa as diversas filas de threads
    int i;
    for (i = 0; i < 4; ++i) {
        ready_queues[i] = malloc(queue_size);
        CreateFila2(ready_queues[i]);

        blocked_queues[i] = malloc(queue_size);
        CreateFila2(blocked_queues[i]);

        terminated_queues[i] = malloc(queue_size);
        CreateFila2(terminated_queues[i]);
    }


    initialized_globals = true;
}


//------------------------------------------------------------------------------
// Inicializa um semáforo
//------------------------------------------------------------------------------
int csem_init(csem_t *sem, int count) {
    // TODO: Descobrir como seria um erro.
    if (sem == NULL) {
        return CSEM_INIT_ERROR;
    }

    // Inicializa a fila referente ao semáforo
    sem->fila = malloc(sizeof(struct sFila2)); // PFILA2 é um tipo ponteiro
    if (queue_create(sem->fila) != 0) {
        return CSEM_INIT_ERROR;
    }

    // Inicializa a contagem do semáforo
    sem->count = count;

    return CSEM_INIT_SUCCESS;
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
    byte *stack = malloc(STACK_SIZE);

    if (!initialized_globals) {
        init();

        getcontext(&main_thread.context);
        // TODO: É preciso salvar mais coisas do contexto... Descobrir o quê, exatamente.
        // main_thread.context.uc_link;
        // main_thread.context.uc_sigmask;
         main_thread.context.uc_stack.ss_sp = stack;
         main_thread.context.uc_stack.ss_size = STACK_SIZE;
        // main_thread.context.uc_mcontext;
        makecontext(&main_thread.context, VOID_FUNCTION(start), 1, arg);
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
    makecontext(&(main_thread.context), VOID_FUNCTION(start), 1, arg);

    // Associa uma função ao contexto
    makecontext(&th->context, VOID_FUNCTION(start), 1, arg);

    // Depois de ser criada, a thread entre para sua fila de aptos
    // correspondente
    FILA2 *queue = ready_queues[priority];
    th->state = READY;
    push(queue, th);

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


//------------------------------------------------------------------------------
// Copia o nome e número do cartão dos alunos para um endereço fornecido
//
// Params:
//  char *name :: Endereço aonde serão copiados os nomes dos alunos
//  int size :: Limite de caracteres que serão copiados para o endereço
//------------------------------------------------------------------------------
int cidentify(char *name, int size) {
    // TODO: Incluir os dados do último integrante.
    char *names =
        "Carlos Pinheiro -- 109910\n"
        "Bruno Feil      -- 216631";

    return strncpy(name, names, size) == 0 ? CIDENTIFY_SUCCESS : CIDENTIFY_ERROR;
}


//------------------------------------------------------------------------------
// Funções da fila
//------------------------------------------------------------------------------
int queue_create(PFILA2 queue) {
    return CreateFila2(queue);
}


// Coloca a thread "th" no final da fila "queue"
int push(PFILA2 queue, TCB_t *th) {
    int ret = AppendFila2(queue, (void *) th);
    FirstFila2(queue);
    return ret;
}


// Retorna o primeiro elemento de "queue" e o remove da fila.
TCB_t *shift(PFILA2 queue) {
    FirstFila2(queue);
    TCB_t *th = (TCB_t *) GetAtIteratorFila2(queue);
    DeleteAtIteratorFila2(queue);
    return th;
}


bool empty(PFILA2 queue) {
    // Quando não consegue "setar" para o primeiro elemento da fila,
    // então o resultado é diferente de zero e a fila está vazia.
    return FirstFila2(queue) != 0;
}


