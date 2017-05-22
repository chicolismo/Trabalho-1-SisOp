//
// Programa de teste para primitivas de criação e sincronização
//
// Disclamer: este programa foi desenvolvido para auxiliar no desenvolvimento
//            de testes para o micronúcleo. NÃO HÁ garantias de estar correto.
#include "../include/support.h"
#include "../include/cthread.h"
#include <stdio.h>


void *fatorial(void *i) {
    int fat, n, counter;

    fat = 1;
    n = *(int *) i;

    printf("O valor de i no fatorial é: %d\n", n);

    counter = 1;
    while (counter < n) {
        fat = counter * fat;
        ++counter;
    }

    printf("Fatorial de %d: %d\n", n, fat);
    return NULL;
}

void *fibonnaci(void *i) {
    int n, a, b, temp;

    n = *(int *) i;
    a = 0;
    b = 1;
    while (n > 0) {
        temp = a + b;
        a = b;
        b = temp;
        printf("%d", a);
        if (n > 1) {
            printf(" ");
        }
        --n;
    }
    printf("\n");
    return NULL;
}

int main(int argc, char **argv) {
    int id0, id1;
    int i = 10;

    id0 = ccreate(fatorial, (void *)&i, 3);
    id1 = ccreate(fibonnaci, (void *)&i, 0);

    printf("Threads fatorial e Fibonnaci criadas...\n");

    cjoin(id1);
    cjoin(id0);

    printf("Main retornando para terminar o programa\n");
    return 0;
}

