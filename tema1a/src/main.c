#include <stdlib.h>
#include <stdio.h>
#include "structures.h"
#include "functions.h"
#include <pthread.h>

int main(int argc, char **argv)
{
    Arguments args;
    Threads threads;

    if (argc != 4)
    {
        printf("Usage: %s <mappers> <reducers> <file>\n", argv[0]);
        return 1;
    }

    init_all(&args, argv);
    alloc_threads(&threads, &args);

    int total_threads = args.mappers + args.reducers;

    // create a map, that has the following correspondence:
    // file_name -> list of words
    



    return 0;
}