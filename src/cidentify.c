#include <string.h>
#include "../include/includes.h"

//------------------------------------------------------------------------------
// Copia o nome e número do cartão dos alunos para um endereço fornecido
//
// Params:
//  char *name :: Endereço aonde serão copiados os nomes dos alunos
//  int size :: Limite de caracteres que serão copiados para o endereço
//------------------------------------------------------------------------------
int cidentify(char *name, int size)
{
    // TODO: Incluir o resto dos dados.
    char *names =
        "Carlos F. Pinheiro -- 109910\n"
        "Bruno Feil -- 216631";

    return strncpy(name, names, size) == 0 ? CIDENTIFY_SUCCESS : CIDENTIFY_ERROR;
}
