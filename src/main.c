#include <stdlib.h>
#include <stdio.h>
#include "../include/includes.h"

int main()
{
    char *names = malloc(300);
    cidentify(names, 300);

    printf("Fuck off\n");
    printf("%s\n", names);

    return EXIT_SUCCESS;
}
