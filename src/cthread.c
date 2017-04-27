#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "../include/cthread.h"
#include "../include/cdata.h"
#include "../include/support.h"
#include "../include/queue.h"

//=============================================================================
// As constantes usadas no programa devem ser definidas aqui.
//=============================================================================

//-----------------------------------------------------------------------------
// ccreate
//-----------------------------------------------------------------------------
#define CCREATE_ERROR -1

//-----------------------------------------------------------------------------
// csem_init
//-----------------------------------------------------------------------------
#define CSEM_INIT_SUCCESS 0
#define CSEM_INIT_ERROR -1

//-----------------------------------------------------------------------------
// cidentify
//-----------------------------------------------------------------------------
#define CIDENTIFY_SUCCESS 0
#define CIDENTIFY_ERROR -1


//==============================================================================
// Aqui ficarão todas as variáveis globais usadas pelo programa
//==============================================================================

static bool initialized_globals = false;

// As filas de threads
static PFILA2 thread_queues[4];


//------------------------------------------------------------------------------
// Esta função inicializa todas as variáveis globais das quais as outras
// funções dependem.  Deve ser chamada nas outras funções, por garantia.
//------------------------------------------------------------------------------
void init()
{
    if (initialized_globals) {
        return;
    }

    // Inicializa as filas de threads
    int i;
    for (i = 0; i < 4; ++i) {
        CreateFila2(thread_queues[i]);
    }

    initialized_globals = true;
}


//------------------------------------------------------------------------------
// Gera uma nova tid
//------------------------------------------------------------------------------
int current_tid = 0;
int get_tid() {
    ++current_tid;
    if (current_tid < 0) {
        current_tid = 0;
    }
    return current_tid;
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
int ccreate(void *(*start) (void *), void *arg, int priority)
{
    init();

    TCB_t *th = malloc(sizeof(TCB_t));
    if (th == NULL) {
        return CCREATE_ERROR;
    }
    th->tid = get_tid();
    th->prio = priority;

    // TODO: Verificar se é isso mesmo

    // Inicializa o contexto da thread
    getcontext(&th->context);

    // Associa uma função ao contexto
    makecontext(&th->context, (void *) start, (int) arg);

    // Coloca a thread na fila de aptos correspondente
    FILA2 *queue = thread_queues[priority];
    push(queue, th);

    // TODO:
    // Colocar a thread na sua devida fila, dependendo da prioridade.
    // Também precisamos descobrir o quê fazer com "start" e "arg".
    // Não esquecer do contexto.

    return th->tid;
}


//------------------------------------------------------------------------------
// Destrói a thread
//------------------------------------------------------------------------------
TCB_t *cdestroy(TCB_t *thread)
{
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
int cidentify(char *name, int size)
{
    // TODO: Incluir os dados do último integrante.
    char *names =
        "Carlos Pinheiro -- 109910\n"
        "Bruno Feil      -- 216631";

    return strncpy(name, names, size) == 0 ? CIDENTIFY_SUCCESS : CIDENTIFY_ERROR;
}


//------------------------------------------------------------------------------
// Inicializa um semáforo
//------------------------------------------------------------------------------
int csem_init(csem_t *sem, int count)
{
    init();
    // TODO: Descobrir como seria um erro.
    if (sem == NULL) {
        return CSEM_INIT_ERROR;
    }

    sem->fila = NULL;
    sem->count = count;
    return CSEM_INIT_SUCCESS;
}


//------------------------------------------------------------------------------
// Funções da fila
//------------------------------------------------------------------------------
int queue_create(PFILA2 queue)
{
    return CreateFila2(queue);
}

// Coloca a thread "th" no final da fila "queue"
int push(PFILA2 queue, TCB_t *th)
{
    int ret = AppendFila2(queue, (void *) th);
    FirstFila2(queue);
    return ret;
}

// Coloca o primeiro elemento de "queue" em "th", e o remove da fila
TCB_t *shift(PFILA2 queue)
{
    FirstFila2(queue);
    TCB_t *th = (TCB_t*) GetAtIteratorFila2(queue);
    DeleteAtIteratorFila2(queue);
    return th;
}

bool empty(PFILA2 queue)
{
    // Quando não consegue "setar" para o primeiro elemento da fila,
    // então o resultado é diferente de zero e a fila está vazia.
    return FirstFila2(queue) != 0;
}
