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
