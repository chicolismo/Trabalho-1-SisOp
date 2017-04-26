#ifndef GLOBALS_H_
#define GLOBALS_H_

#include <stdbool.h>
#include "../include/cdata.h"
#include "../include/support.h"

//==============================================================================
// Aqui ficarão todas as variáveis globais usadas pelo programa
//==============================================================================

static bool initialized_globals = false;

// As filas de threads
static PFILA2 thread_queues[4];

#endif
