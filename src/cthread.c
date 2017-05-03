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


//Public Functions use
#define SUCCESS_CODE 0
#define ERROR_CODE -1


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
//Funcoes de inicialização
//------------------------------------------------------------------------------

int init_main_thread() {
	//TODO: implementar a inicializacao da thread main.
	
	return SUCCESS_CODE;
}

int init_end_context() {
	//TODO: implementar a inicializacao do contexto de finalizacao de thread.
	return SUCCESS_CODE;
}

//------------------------------------------------------------------------------
// Esta função inicializa todas as variáveis globais das quais as outras
// funções dependem.  Deve ser chamada nas outras funções, por garantia.
//------------------------------------------------------------------------------
int init() {
	if (!initialized_globals) {

		initialized_globals = true;

		// inicializar a thread main e colocar em executando.
		if(init_main_thread() != SUCCESS_CODE)
			return ERROR_CODE;

		// inicializar o contexto de finalizacao de thread.
		if(init_end_context() != SUCCESS_CODE)
			return ERROR_CODE;

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
	
		if(CreateFila2(blocked_join)!=SUCCESS_CODE)
			return ERROR_CODE;

		blocked_semaphor = (FILA2 *)malloc(sizeof(FILA2));
		if(CreateFila2(blocked_semaphor)!=SUCCESS_CODE)
			return ERROR_CODE;

		return SUCCESS_CODE;
	}
	return SUCESS_CODE;
}



//------------------------------------------------------------------------------
//Funcoes da lista de bloqueados cjoin
//------------------------------------------------------------------------------
int blocked_join_insert(struct Duplacjoin *thread){//essa estrutura Duplacjoin deve ainda ser definida
	//funcao que insere uma dupla thread,tid esperado na lista de bloq. cjoin.
	//provavelmente a funcao seja so isso.
	return AppendFila2(blocked_join, thread);
}

int blocked_join_remove(struct Duplacjoin *thread) { //essa estrutura duplacjoin deve ser definida
	//funcao que remove uma dupla da fila. sera chamado apos encontrar um tid esperado na fila e recuperar a thread
	//bloqueada, pode ser implementada dentro da funcao de get_thread_waiting_for, mas isso eh escolha de quem implementar.
	return 0;
}

TCB_t* blocked_join_get_thread(int tid) {
	//funcao que verifica a existencia de uma thread na fila de bloqueados por cjoin.
	//retorna um ponteiro para a thread caso a encontre, e um ponteiro NULL caso a thread nao seja encontrada.
	return NULL;
}

struct Duplacjoin* blocked_join_get_thread_waiting_for(int tid) { //essa estrutura duplacjoin ainda deve ser definida.
	//funcao que procura por um tid esperado na lista de duplas da fila cjoin,
	//pode retornar a thread ou a dupla, pensei na dupla so para ser mais direto a busca. Mas de novo, decisao de implementacao.
	//retorna um ponteiro para a dupla/thread caso exista uma thread bloqueada pelo tid, e um ponteiro NULL caso nao exista
	return NULL;
}
//------------------------------------------------------------------------------
//Funcoes de Semaforo
//------------------------------------------------------------------------------

int insert_semaphore_on_blocked_semaphor(csem_t *sem){
    	//funcao que insere um semaforo na lista de semaforos criados.
	//retorna 0 quando a insercao e bem sucedida e -1 quando ha erros.
	
	if (FirstFila2(blocked_semaphor) == 0) {
		do {	
		csem_t *value = (csem_t *)GetAtIteratorFila2(semaphore_list);
		if (value != NULL && value == sem) {
			//semaforo ja esta na fila, portanto nao ha insercao e retorna codigo de sucesso.
			return SUCCESS_CODE;
			}
		} while (NextFila2(blocked_semaphor) == 0);
		//caso o semaforo nao esteja inserido, ele eh entao inserido na fila.
		return AppendFila2(blocked_semaphor, sem);
		
		
	} else {
		//caso seja o primeiro elemento, o semaforo sem eh simplesmente inserido.
		return AppendFila2(blocked_semaphor, sem);
}

TCB_t* get_first_of_semaphore_queue(csem_t *sem) {
    	//funcao que remove e retorna o primeiro elemento da fila de um semaforo
	//retorna um ponteiro para a thread se a funcao for bem sucedida
	//e um ponteiro NULL em caso de thread nao existente.
	
	if (FirstFila2(sem->fila) == 0) {
		TCB_t *value = (TCB_t *)GetAtIteratorFila2(sem->fila);
		DeleteAtIteratorFila2(sem->fila);
		return value;
		
	}else return NULL;	
}

TCB_t* get_thread_from_blocked_semaphor(int tid) {
    	//funcao que verifica a existencia de uma thread nas filas de bloqueados dos semaforos
    	//NAO REMOVE nenhuma thread, Retorna um ponteiro para a thread se for bem sucedida
	//e um ponteiro NULL em caso de erro
	
	if (FirstFila2(blocked_semaphor) == 0) {
		do { //iteracao para varrer a lista de semaforos.
			csem_t *value = (csem_t *)GetAtIteratorFila2(semaphore_list);
			if(value == NULL) break; //precisa desse teste pois caso value seja igual a NULL, o ponteiro de
			//value->fila apontara para um endereco nao conhecido e acontecera segmentation fault.
			if(FirstFila2(value->fila)==0){
				do{//iteracao para varrer a fila de bloqueados do semaforo
					TCB_t *value2 = (TCB_t *)GetAtIteratorFila2(value->fila);
					if(value2 == NULL) break;//teste necessario, pois caso value2 seja igual a NULL,
					//acontecera segmentation fault no ponteiro value2->tid
					if (value2->tid == tid)//encontrou a thread procurada.
						return value2;
				}while (NextFila2(value->fila) == 0);
			}
		} while (NextFila2(blocked_semaphor) == 0);
		//Nao encontrou a thread, retornara um ponteiro NULL
		return NULL;
	} else { 
		// Fila de semaforos nao existe, retorna ponteiro nulo.
		return NULL;
	}
}





//------------------------------------------------------------------------------
// Gera uma nova tid
//------------------------------------------------------------------------------
int current_tid = 0;
int generate_tid() {
    return ++current_tid;
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

int csetprio(int tid, int prio){
	return SUCCESS_CODE;
}

int cyield(void){
	return SUCCESS_CODE;
}

int cjoin(int tid){
	return SUCCESS_CODE;
}

//------------------------------------------------------------------------------
// Inicializa um semáforo
//------------------------------------------------------------------------------
int csem_init(csem_t *sem, int count) {
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
	sem->fila = (FILA2)malloc(sizeof(FILA2));
	
	//insere o semáforo na lista de semáforos
	if(insert_semaphore_on_blocked_semaphor(csem_t *sem)==0){
		return CreateFila2(sem->fila);
	}
	else
		return ERROR_CODE;
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
	TCB_t *thread = (TCB_t *)get_first_of_semaphore_queue(sem);	
	if (thread != NULL) {//existia uma thread bloqueada pelo semaforo.
		//estado da thread e modificado para APTO
		thread->state = PROCST_APTO;
		return;//deve retornar funcao_de_inserir_na_fila_de_aptos(thread);
	} else {
		//O semaforo esta livre. Segue execucao.
		return SUCCESS_CODE;
	}
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
    char *names =
        "Carlos Pinheiro -- 109910\n"
        "Bruno Feil      -- 216631"
	"Hugo Constantinopolos -- 208897;

    return strncpy(name, names, size) == 0 ? CIDENTIFY_SUCCESS : CIDENTIFY_ERROR;
}


