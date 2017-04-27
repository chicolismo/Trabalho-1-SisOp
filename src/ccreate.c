#include <stdlib.h>
#include "../include/includes.h"

// 
// typedef struct s_TCB {
//     // Identificador da thread.
//     int tid;
// 
//     // Estado em que a thread se encontra.
//     //   0: Criação; 1: Apto; 2: Execução; 3: Bloqueado e 4: Término
//     int state;
// 
//     // "bilhete" de loteria da thread, para uso do escalonador
//     int ticket;
// 
//     // contexto de execução da thread (SP, PC, GPRs e recursos)
//     ucontext_t  context;
// } TCB_t;
// 

// TODO: Descobrir se é possível decrementar o valor em algum momento,
// ou seja, se é possível destruir uma thread.
int current_tid = 0;
int get_tid() {
    ++current_tid;
    if (current_tid < 0) {
        current_tid = 0;
    }
    return current_tid;
}

TCB_t *cdestroy(TCB_t *thread)
{
    free(thread);
    thread = NULL;
    return thread;
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
/*int ccreate(void *(*start) (void *), void *arg, int priority)*/
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
