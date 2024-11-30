#include <stdlib.h>
#include <stdio.h>
#include "structures.h"
#include "functions.h"
#include <pthread.h>

int main(int argc, char **argv)
{
    Arguments args;

    if (argc != 4)
    {
        printf("Usage: %s <mappers> <reducers> <file>\n", argv[0]);
        return 1;
    }

    init_all(&args, argv);


    return 0;
}