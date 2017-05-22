// vim: sw=4 ts=4 sts=4 expandtab foldenable foldmethod=syntax

#ifdef __APPLE__
// Necessário para não exibir avisos de "deprecated" no macOs
#define _XOPEN_SOURCE 500
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
// Macros
//=============================================================================

//-----------------------------------------------------------------------------
// Como funciona o debug:
// para ativar, deixe     #define DEBUG(X) printf X
// para desativar, deixe  #define DEBUG(X) //printf X
//
// ex: DEBUG(("HELLO WORLD!!"));
//
// se estiver ativado:    DEBUG(X) se transforma em printf("HELLO WORLD!!");
// se estiver desativado: DEBUG(X) se transforma em //printf("HELLO WORLD!!");
//-----------------------------------------------------------------------------
#define DEBUG(X) //printf X
#define SHOULD_DEBUG 0

// Faz "cast" de "arg" para um ponteiro de função void sem argumentos
#define VOID_FUNCTION(arg) (void (*)(void)) (arg)


//=============================================================================
// Códigos de erro
//=============================================================================

// ccreate
#define CCREATE_ERROR -1

// csem_init
#define CSEM_INIT_SUCCESS 0
#define CSEM_INIT_ERROR -1

// cidentify
#define CIDENTIFY_SUCCESS 0
#define CIDENTIFY_ERROR -1

// funcões genéricas
#define SUCCESS_CODE 0
#define ERROR_CODE -1

// init_ending_ctx
#define INIT_ENDING_CTX_ERROR -1
#define INIT_ENDING_CTX_SUCCESS 0


//==============================================================================
// Globais
//==============================================================================

// Flag para detectar a inicialização das variáveis.
bool initialized_globals = false;

// TID a ser usado nas novas threads
int current_tid = 0;

// As filas de threads.
// Devem ser ponteiros para podermos usar CreateFila2.
FILA2 *ready[4];               // Quatro filas de aptos
FILA2 *blocked_join;           // Bloqueados esperando outra thread terminar
FILA2 *blocked_semaphor;       // Bloqueados esperando semáforo
TCB_t *running_thread = NULL;  // Executando
ucontext_t *ending_ctx = NULL;


//==============================================================================
// Funções
//==============================================================================

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
// Funças da lista de bloqueados cjoin
//------------------------------------------------------------------------------

/*
 * Insere uma dupla (thread, tid) esperada na lista de bloq.cjoin.
 */
int blocked_join_insert(DUPLA_t *thread) {
    // A estrutura Duplacjoin está definida em queue.h.
    return AppendFila2(blocked_join, thread);
}


/*
 * Remove uma dupla (thread, tid) da fila de bloq.cjoin.
 * Será chamada após encontrar um tid esperado na fila e recuperar a thread bloqueada.
 */
int blocked_join_remove(DUPLA_t *toremove) {
    // A estrutura Duplacjoin está definida em queue.h.

    if (FirstFila2(blocked_join) == 0) {
        do {
            DUPLA_t *value = (DUPLA_t *)GetAtIteratorFila2(blocked_join);
            if (value != NULL) {
                if (value == toremove) {
                    return  DeleteAtIteratorFila2(blocked_join);
                }
            }
        } while (NextFila2(blocked_join) == 0);

        return ERROR_CODE;

    }
    else {
        // Fila vazia, não é possível remover.
        return ERROR_CODE;
    }
}

/*
 * Verifica a existência de uma thread na fila de bloquados por cjoin.
 * Caso a thread exista, retorna seu ponteiro, caso contrário, retorna NULL.
 */
TCB_t *blocked_join_get_thread(int tid) {
    if (FirstFila2(blocked_join) == 0) {
        do {
            DUPLA_t *value = (DUPLA_t *)GetAtIteratorFila2(blocked_join);
            if (value != NULL)
                if (value->blockedThread->tid == tid) {
                    return  value->blockedThread;
                }
        } while (NextFila2(blocked_join) == 0);
        return NULL;


    }
    else {
        // Fila vazia, não existe.
        return NULL;
    }
}

/*
 * Procura por um tid esperado na lista de duplas da fila cjoin.  Retorna o
 * ponteiro para a dupla, caso exista uma thread bloqueada com o tid fornecido.
 * Caso contrário, retorna NULL.
 */
DUPLA_t *blocked_join_get_thread_waiting_for(int tid) {
    // A estrutura Duplacjoin esta definida em queue.h.

    if (FirstFila2(blocked_join) == 0) {
        do {
            DUPLA_t *value = (DUPLA_t *)GetAtIteratorFila2(blocked_join);
            if (value != NULL)
                if (value->waitedTid == tid) {
                    return  value;
                }
        } while (NextFila2(blocked_join) == 0);
        return NULL;


    }
    else {
        // Fila vazia, não existe.
        return NULL;
    }
}


void debug_print_blocked_list() {
    DUPLA_t *print;
    if (FirstFila2(blocked_join) == 0) {
        DEBUG(("##################################################\n"));
        DEBUG(("############### FILA DE BLOQUEADOS ###############\n"));
        DEBUG(("##################################################\n"));
        do {
            print = (DUPLA_t *)GetAtIteratorFila2(blocked_join);
            if (print == NULL) {
                break;
            }
            if (print->blockedThread != NULL) {
                DEBUG(("# tid bloqueado: %d   /   esperando pelo tid: %d  # \n", print->blockedThread->tid,
                       print->waitedTid));
            }
            else {
                DEBUG(("# tid bloqueado: atual  / esperando pelo tid: %d # \n", print->waitedTid));
            }
        } while (NextFila2(blocked_join) == 0);
        DEBUG(("##################################################\n"));
        return;
    }
    return;
}

//------------------------------------------------------------------------------
// Funções de Semáforo
//------------------------------------------------------------------------------

int semaphore_queue_insert_thread(csem_t *sem, TCB_t *thread) {
    //função que insere uma thread na fila de bloqueados de um semáforo sem.
    return AppendFila2(sem->fila, thread);
}

int insert_semaphore_on_blocked_semaphor(csem_t *sem) {
    //funcao que insere um semáforo na lista de semáforos criados.
    //retorna 0 quando a insercao e bem sucedida e -1 quando ha erros.

    if (FirstFila2(blocked_semaphor) == 0) {
        do {
            csem_t *value = (csem_t *)GetAtIteratorFila2(blocked_semaphor);
            if (value != NULL && value == sem) {
                // Semáforo já está na fila, portanto não há inserção e retorna código de sucesso.
                return SUCCESS_CODE;
            }
        } while (NextFila2(blocked_semaphor) == 0);
        // Caso o semáforo não esteja inserido, ele é então inserido na fila.
        return AppendFila2(blocked_semaphor, sem);


    }
    else {
        // Caso seja o primeiro elemento, o semáforo "sem" é simplesmente
        // inserido.
        return AppendFila2(blocked_semaphor, sem);
    }
}

/*
 * Remove e retorna o primeiro elemento da fila de um semáforo.
 * Retorna um ponteiro para a thread se a função for bem sucedida, caso
 * contrário, retorna NULL
 */
TCB_t *get_first_of_semaphore_queue(csem_t *sem) {
    if (FirstFila2(sem->fila) == 0) {
        TCB_t *value = (TCB_t *)GetAtIteratorFila2(sem->fila);
        DeleteAtIteratorFila2(sem->fila);
        return value;

    }
    else {
        return NULL;
    }
}

/*
 * Verifica a existência de uma thread nas filas de bloqueados dos semáforos.
 * NÃO remove nenhuma thread.
 * Retorna um ponteiro para a thread se for bem sucedida ou NULL caso contrário.
 */
TCB_t *get_thread_from_blocked_semaphor(int tid) {
    if (FirstFila2(blocked_semaphor) == 0) {
        // Varre a lista de semáforos.
        do {
            csem_t *value = (csem_t *)GetAtIteratorFila2(blocked_semaphor);
            if (value == NULL) {
                // Precisa desse teste, pois caso "value" seja NULL, "value->fila" será um segfault
                break;
            }
            if (FirstFila2(value->fila) == 0) {
                // Varre a lista de bloqueados do semáforo
                do {
                    TCB_t *value2 = (TCB_t *)GetAtIteratorFila2(value->fila);
                    if (value2 == NULL) {
                        // Caso "value2" seja null, "value2->tid" será segfault também.
                        break;
                    }
                    if (value2->tid == tid) {
                        // Thread procurada foi encontrada.
                        return value2;
                    }
                } while (NextFila2(value->fila) == 0);
            }
        } while (NextFila2(blocked_semaphor) == 0);

        // Thread não encontrada.
        return NULL;
    }
    else {
        // Fila de semaforos nao existe, retorna ponteiro nulo.
        return NULL;
    }
}

/*
 * Imprime a lista de semáforos em detalhes.
 */
int debug_blocked_semaphor() {
    int i = 1;
    DEBUG(("========== DEBUG SEMAPHORE LIST ==========\n"));
    if (FirstFila2(blocked_semaphor) == 0) {
        do {
            DEBUG(("==                                      ==\n"));
            csem_t *value = (csem_t *)GetAtIteratorFila2(blocked_semaphor);
            if (value != NULL) {
                DEBUG(("== Semaforo %2i. Count = %2i               ==\n", i, value->count));
                if (FirstFila2(value->fila) == 0) {
                    DEBUG(("Tids bloqueados:\n"));
                    do {
                        TCB_t *value2 = (TCB_t *)GetAtIteratorFila2(value->fila);
                        if (value2 != NULL) {
                            DEBUG(("%d\n", value2->tid));
                        }
                    } while (NextFila2(value->fila) == 0);
                }
                i++;
            }
        } while (NextFila2(blocked_semaphor) == 0);
    }
    else {
        DEBUG(("========== NÃO HÁ LISTA SEMÁFORO =========\n"));

    }
    DEBUG(("========== DEBUG SEMAPHORE LIST ==========\n"));
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
        if (FirstFila2(ready[i]) == SUCCESS_CODE) {
            th = (TCB_t *) GetAtIteratorFila2(ready[i]);
            DeleteAtIteratorFila2(ready[i]);
            break;
        }
    }
    return th;
}

// /*
//  * Retorna um resultado de busca nas filas de aptos.  Caso seja encontrada a
//  * thread com o tid correspondente, é retornada uma estrutura contendo o
//  * iterador da fila em que a thread se encontra, bem como o número da fila.
//  *
//  * Caso nenhuma thread com o tid fornecido seja encontrada, a função retorna
//  * NULL.
//  */
// FindResult *ready_find(int tid) {
//     FindResult *result = NULL;
//     int i;
//     for (i = 0; i < 4; ++i) {
//         FirstFila2(ready[i]);
//         do {
//             if (((TCB_t *)GetAtIteratorFila2(ready[i]))->tid == tid) {
//                 result->node = ready[i]->it;
//                 result->queue_number = i;
//             }
//         } while (NextFila2(ready[i]) == 0);
//     }
//     return result;
// }

/*
 * Retorna uma thread das filas de aptos a partir de sua tid.
 * Não remove a thread, apenas retorna seu ponteiro, caso exista.
 * Se não existir, retorna NULL
 */
TCB_t *ready_get_thread(int tid) {
    TCB_t *thread = NULL;
    int i;
    for (i = 0; i < 4; ++i) {
        if (FirstFila2(ready[i]) == SUCCESS_CODE) {
            do {
                thread = (TCB_t *)GetAtIteratorFila2(ready[i]);
                if (thread != NULL) {
                    if (thread->tid == tid) {
                        return thread;
                    }
                }
            } while (NextFila2(ready[i]) == 0);
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
    TCB_t *thread = NULL;
    TCB_t *result = ready_get_thread(tid);
    if (result != NULL) {
        if (FirstFila2(ready[result->prio]) == SUCCESS_CODE) {
            do {
                thread = (TCB_t *)GetAtIteratorFila2(ready[result->prio]);
                if (thread != NULL) {
                    if (thread->tid == tid) {
                        DeleteAtIteratorFila2(ready[result->prio]);
                    }
                }
            } while (NextFila2(ready[result->prio]) == 0);
            return result;
        }
    }
    return NULL;
}

void debug_ready() {
    int i;
    TCB_t *print;
    DEBUG(("##################################################\n"));
    DEBUG(("###############   FILA DE APTOS    ###############\n"));
    DEBUG(("##################################################\n"));
    for (i = 0; i < 4; ++i) {
        if (FirstFila2(ready[i]) == SUCCESS_CODE) {
            DEBUG(("# Prioridade %d\n", i));
            DEBUG(("# Tids na fila: \n"));
            do {
                print = (TCB_t *)GetAtIteratorFila2(ready[i]);
                if (print != NULL) {
                    DEBUG(("# %d\n", print->tid));
                }
            }
            while (NextFila2(ready[i]) == 0);
            DEBUG(("##################################################\n"));
        }
    }
    return;
}


//------------------------------------------------------------------------------
//Funcoes de inicialização
//------------------------------------------------------------------------------

int init_main_thread() {
    // Inicializacao da thread main.
    TCB_t *thread = (TCB_t *) malloc(sizeof(TCB_t));
    thread->tid = current_tid;
    thread->prio = 0; // Thread main terá a maior prioridade.
    thread->state = PROCST_EXEC;

    // Criação da pilha da main.
    if (((thread->context).uc_stack.ss_sp = malloc(SIGSTKSZ)) == NULL) {
        // Sem espaço na memória para criar a TCB
        return ERROR_CODE;
    }

    (thread->context).uc_stack.ss_size = SIGSTKSZ;

    // Quando a thread main acaba o programa acaba.
    (thread->context).uc_link = NULL;

    // Não é inserido em nenhuma fila, a thread simplesmente está em execução.
    running_thread = thread;

    // Não é necessário usar um makecontext, pois o contexto da thread é a
    // propria main, que está em execução.

    return SUCCESS_CODE;
}

void end_thread();

int init_ending_ctx() {
    ending_ctx = malloc(sizeof(ucontext_t));

    if ((getcontext(ending_ctx) != 0) || ending_ctx == NULL) {
        // Erro na inicialização do contexto de finalização
        return INIT_ENDING_CTX_ERROR;
    }

    ending_ctx->uc_stack.ss_sp = malloc(SIGSTKSZ);
    ending_ctx->uc_stack.ss_size = SIGSTKSZ;
    ending_ctx->uc_link = NULL; // Nada a fazer quando acabar

    makecontext(ending_ctx, end_thread, 0);

    return INIT_ENDING_CTX_SUCCESS;
}

/*
 * Inicializa todas as variáveis globais das quais as outras funções dependem.
 * Deve ser chamada nas outras funções da biblioteca, por garantia.
 */
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

        // Tamanho de cada struct de fila
        size_t queue_size = sizeof(struct sFila2);

        // Inicializa as diversas filas de threads
        int i;
        for (i = 0; i < 4; ++i) {
            ready[i] = malloc(queue_size);
            CreateFila2(ready[i]);
        }
        blocked_join = (FILA2 *) malloc(sizeof(FILA2));

        if (CreateFila2(blocked_join) != SUCCESS_CODE) {
            return ERROR_CODE;
        }

        blocked_semaphor = (FILA2 *) malloc(sizeof(FILA2));
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

int main(void) {

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

/*
 * Escalonador.
 */
int dispatch() {
    bool swapped_context = false;

    DEBUG(("===Dispatch=== \n"));

    if (running_thread != NULL) {
        // se uma thread acabar o running thread é NULL, o teste evita segmentation fault.

        DEBUG(("Thread %d perdendo a execução. ", running_thread->tid));
        getcontext(&(running_thread->context));
    }

    if (!swapped_context) {
        // Somente executará no contexto original, quando a thread está
        // voltando de execução não é executado.

        swapped_context = true;
        running_thread = ready_shift();

        if (running_thread == NULL || running_thread->tid < 0) {
            //Não existe thread a ser executada ou possui erro em sua geração.
            return ERROR_CODE;
        }
        DEBUG(("Thread %d está em execução.\n", running_thread->tid));
        running_thread->state = PROCST_EXEC;
        setcontext(&(running_thread->context));
    }
    return SUCCESS_CODE;
}

/*
 * Função a ser executada ao final de cada thread.
 */
void end_thread() {
    if (running_thread == NULL) {
        // Não há thread em execução
        return;
    }

    int tid = running_thread->tid;
    DEBUG(("Thread %d Acabando\n", tid));

    cdestroy(running_thread);

    // Procura pela thread que estava esperando esta aqui terminar.
    DUPLA_t *waiting_thread = blocked_join_get_thread_waiting_for(tid);

    if (waiting_thread == NULL) {
        DEBUG(("Não há threads esperando pela tid. \n"));
        dispatch();
    }
    else {
        DEBUG(("Há uma thread esperando pelo fim dessa tid.\n"));
        blocked_join_remove(waiting_thread);
        waiting_thread->blockedThread->state = PROCST_APTO;
        DEBUG(("Thread %d estava esperando e foi inserida na fila de aptos.\n", waiting_thread->blockedThread->tid));
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
    DEBUG(("Ccreate> Criando thread %d\n", thread->tid));

    thread->context.uc_stack.ss_size = SIGSTKSZ;
    thread->context.uc_link = ending_ctx;
    makecontext(&(thread->context), VOID_FUNCTION(start), 1, arg);

    ready_push(thread);

    return new_tid;
}

int csetprio(int tid, int prio) {
    init();
    bool thread_in_ready = false;
    DEBUG(("csetprio>\n"));
    DEBUG(("Alterando prioridade da thread %i para %i\n", tid, prio));

    if (prio < 0 || prio > 3) {
        // Prioridade não está no intervalo [0, 3]
        return ERROR_CODE;
    }

    TCB_t *thread = NULL;
    if ((thread = blocked_join_get_thread(tid)) == NULL) {
        if ((thread = get_thread_from_blocked_semaphor(tid)) == NULL) {
            if ((thread = ready_get_thread(tid)) == NULL) {
                if (running_thread->tid == tid) {
                    running_thread->prio = prio;
                    return SUCCESS_CODE;
                }
                else {
                    DEBUG(("Thread a ser modificada não existe.\n"));
                }
                return ERROR_CODE;
            }
            else {
                thread_in_ready = true;
            }
        }
    }
    // A thread existe e está apontada pelo ponteiro "thread".

    if (thread_in_ready) {
        // Caso a thread esteja na fila de aptos, temos que removê-la da fila
        // com prioridade atual, e inseri-la na sua nova fila com a prioridade
        // certa.
        thread = ready_remove(thread->tid);
        if (thread != NULL) {
            thread->prio = prio;  // Altera a prioridade da thread.
            ready_push(thread);   // Coloca na fila certa.
        }
    }
    else {
        // Neste caso, a thread não está nos aptos, e podemos simplesmente
        // alterar sua prioridade.
        thread->prio = prio;
    }

    return SUCCESS_CODE;
}

int cyield() {
    init();
    DEBUG(("cyield>\n Thread %d cedendo a execução voluntariamente.\n", running_thread->tid));

    // Muda-se o estado da thread em execução para "apto", e ela é colocada de volta pra fila de aptos.
    running_thread->state = PROCST_APTO;
    ready_push(running_thread);

    // O escalonador é acionado.
    return dispatch();
}

int cjoin(int tid) {
    init();
    DEBUG(("Cjoin> join na thread %d pela thread %d\n", tid, running_thread->tid));

    if (blocked_join_get_thread_waiting_for(tid) != NULL) {
        DEBUG(("A thread ja esta sendo esperada.\n"));
        return ERROR_CODE;
    }

    TCB_t *thread = NULL;
    if ((thread = blocked_join_get_thread(tid)) == NULL) {
        if ((thread = get_thread_from_blocked_semaphor(tid)) == NULL) {
            if ((thread = ready_get_thread(tid)) == NULL) {
                DEBUG(("Thread não existe.\n"));
                // Thread não existe, retorna erro.
                return ERROR_CODE;
            }
        }
    }

    DEBUG(("Thread existe e não é esperada.\n"));
    // Thread que se deseja esperar o término existe, não é esperada e está
    // apontada pelo ponteiro "thread".
    DUPLA_t *new_cjoin = (DUPLA_t *) malloc(sizeof(DUPLA_t));
    new_cjoin->waitedTid = tid;
    new_cjoin->blockedThread = running_thread;
    running_thread->state = PROCST_BLOQ;
    blocked_join_insert(new_cjoin);

#if SHOULD_DEBUG
    debug_print_blocked_list();
#endif

    return dispatch();
}

/*
 * Inicializa um semáforo
 */
int csem_init(csem_t *sem, int count) {
    init();
    DEBUG(("csem_init>\n"));
    if (sem == NULL) {
        DEBUG(("Nao e possivel inicializar um ponteiro para um semaforo nulo.\n"));
        return CSEM_INIT_ERROR;
    }
    if (sem->fila != NULL) {
        DEBUG(("Nao e possivel inicializar um semaforo ja inicializado.\n"));
        return CSEM_INIT_ERROR;
    }
    sem->count = count;

    sem->fila = (FILA2 *) malloc(sizeof(FILA2));

    // Insere o semáforo na lista de semáforos
    if (insert_semaphore_on_blocked_semaphor(sem) == 0) {
#if SHOULD_DEBUG
        debug_blocked_semaphor();
#endif
        return CreateFila2(sem->fila);
    }
    else {
        return ERROR_CODE;
    }
}

/*
 * Tranca o semáforo se o mesmo ainda não está trancado, se já estiver trancado
 * coloca a thread em uma fila de bloqueados, aguardando a liberação do
 * recurso.
 */
int cwait(csem_t *sem) {
    init();
    DEBUG(("cwait>\n"));
    if ((sem == NULL) || (sem->fila == NULL)) {
        DEBUG(("Não é possivel dar wait em um ponteiro para um semaforo nulo ou cuja fila não esteja inicializada.\n"));
        return ERROR_CODE;
    }

    if (sem->count > 0) {
        DEBUG(("O recurso NÃO ESTÁ sendo usado, então a thread vai usá-lo..\n"));
        sem->count -= 1;
        return SUCCESS_CODE;
    }
    else {
        DEBUG(("O recurso JÁ ESTÁ sendo usado, então precisamos bloquear a thread.\n"));
        sem->count -= 1;
        running_thread->state = PROCST_BLOQ;
        semaphore_queue_insert_thread(sem, running_thread);
        DEBUG(("Thread bloqueada e inserida na fila do semáforo.\n"));
        dispatch();
    }
    return SUCCESS_CODE;
}

/*
 * Destrava o semáforo, e libera as threads bloqueadas esperando pelo recurso.
 */
int csignal(csem_t *sem) {
    init();
    DEBUG(("csignal>\n"));
    if ((sem == NULL) || (sem->fila == NULL)) {
        // Não é possivel dar signal em um ponteiro para um semáforo nulo ou
        // cuja fila não esteja inicializada.
        return ERROR_CODE;
    }

    sem->count += 1;
    TCB_t *thread = (TCB_t *)get_first_of_semaphore_queue(sem);
    if (thread != NULL) {
        // Existia uma thread bloqueada pelo semáforo.
        // Estado da thread e modificado para APTO
        thread->state = PROCST_APTO;
        return ready_push(thread);
    }
    else {
        //O semáforo esta livre. Segue execucao.
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

