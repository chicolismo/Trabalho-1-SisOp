/*
 * cdata.h: arquivo de inclusão de uso apenas na geração da libpithread
 *
 * Esse arquivo pode ser modificado. ENTRETANTO, deve ser utilizada a TCB fornecida
 *
 * Versão de 25/04/2017
 *
 */
#ifndef __cdata__
#define __cdata__

// Necessário para usar "ucontext.h"
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 500
#endif

#include <ucontext.h>

#define PROCST_CRIACAO  0
#define PROCST_APTO 1
#define PROCST_EXEC 2
#define PROCST_BLOQ 3
#define PROCST_TERMINO  4

/* como funciona o debug:
para ativar, deixe 	#define DEBUG(X) printf X
para desativar, deixe 	#define DEBUG(X) //printf X

ex: DEBUG(("HELLO WORLD!!");

se estiver ativado: 	DEBUG(X) se transforma em printf("HELLO WORLD!!");
se estiver desativado: 	DEBUG(X) se transforma em //printf("HELLO WORLD!!");
*/
#define DEBUG(X) //printf X


//AVISO DO CECHIN NO DIA 9/5
/*
Pessoal,

Conforme avisei em aula, seguem as informações de orientação sobre o trabalho:

O arquivo CTHREAD.H não deverá ser alterado. Mantenham o mesmo arquivo distribuído e utilizem o campo TICKET como a PRIORIDADE atribuída à thread
Os exemplo devem ser alterados para que a "ccreate" tenha um terceiro parâmetro com a prioridade. Sugere-se que seja prioridade "0" (zero)
A thread MAIN deverá ter prioridade "0" (zero)
Sérgio Cechin
*/
//==============================================================================
// Thread Control Block
//==============================================================================

/* NÃO ALTERAR ESSA struct */
typedef struct s_TCB {
    // Identificador da thread.
    int tid;

    // Estado em que a thread se encontra.
    //   0: Criação; 1: Apto; 2: Execução; 3: Bloqueado e 4: Término
    int state;

    //***********************************************************************************
    // NOTE: A struct original se referia a um trabalho antigo, e não possuia
    // prioridade.
    //***********************************************************************************

    // Prioridade da thread, para uso do escalonador
    int prio;

    // contexto de execução da thread (SP, PC, GPRs e recursos)
    ucontext_t  context;
} TCB_t;

#endif
