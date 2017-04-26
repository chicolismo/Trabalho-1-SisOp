/*
 * cdata.h: arquivo de inclus�o de uso apenas na gera��o da libpithread
 *
 * Esse arquivo pode ser modificado. ENTRETANTO, deve ser utilizada a TCB fornecida
 *
 * Vers�o de 25/04/2017
 *
 */
#ifndef __cdata__
#define __cdata__

// Necess�rio para usar "ucontext.h"
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

/* N�O ALTERAR ESSA struct */
typedef struct s_TCB {
    // Identificador da thread.
    int tid;

    // Estado em que a thread se encontra.
    //   0: Cria��o; 1: Apto; 2: Execu��o; 3: Bloqueado e 4: T�rmino
    int state;

    //***********************************************************************************
    // NOTE: A struct original se referia a um trabalho antigo, e n�o possuia
    // prioridade.
    //***********************************************************************************

    // Prioridade da thread, para uso do escalonador
    int prio;

    // contexto de execu��o da thread (SP, PC, GPRs e recursos)
    ucontext_t  context;
} TCB_t;

#endif
