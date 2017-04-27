#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "../include/cthread.h"
#include "../include/cdata.h"
#include "../include/support.h"
#include "../include/queue.h"

//=============================================================================
// Constantes
//=============================================================================

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

typedef enum State { NEW, READY, RUNNING, BLOCKED, TERMINATED } State;

bool initialized_globals = false;

// A thread da "main"
TCB_t main_thread;

TCB_t *running_thread;              // Executando

// As filas de threads
// Existem 4 filas de cada tipo, uma para cada prioridade
PFILA2 ready_queues[4];      // Aptos
PFILA2 blocked_queues[4];    // Bloqueados
PFILA2 terminated_queues[4]; // Terminados


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

    // Inicializa as diversas filas de threads
    int i;
    size_t queue_size = sizeof(struct sFila2);
    for (i = 0; i < 4; ++i) {
        ready_queues[i] = malloc(queue_size);
        CreateFila2(ready_queues[i]);

        blocked_queues[i] = malloc(queue_size);
        CreateFila2(blocked_queues[i]);

        terminated_queues[i] = malloc(queue_size);
        CreateFila2(terminated_queues[i]);
    }

    getcontext(&(main_thread.context));

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


//------------------------------------------------------------------------------
// Cria uma nova thread.
//
// Parâmetros:
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

    TCB_t *th = malloc(sizeof(TCB_t));
    if (th == NULL) {
        return CCREATE_ERROR;
    }
    th->state = NEW;
    th->tid = generate_tid();
    th->prio = priority;

    // TODO: Verificar se é isso mesmo

    // Inicializa o contexto da thread
    getcontext(&th->context);

    // Associa uma função ao contexto
    makecontext(&th->context, (void *) start, (int) arg);

    // Depois de ser criada, a thread entre para sua fila de aptos
    // correspondente
    FILA2 *queue = ready_queues[priority];
    th->state = READY;
    push(queue, th);

    // TODO:
    // Colocar a thread na sua devida fila, dependendo da prioridade.
    // Também precisamos descobrir o quê fazer com "start" e "arg".
    // Não esquecer do contexto.

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


// Coloca o primeiro elemento de "queue" em "th", e o remove da fila
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

