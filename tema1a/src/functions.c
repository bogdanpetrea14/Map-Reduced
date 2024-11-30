#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include "structures.h"

void alloc_threads(Threads *threads, Arguments *args)
{
    threads->mappers = (pthread_t *)malloc(sizeof(pthread_t) * args->mappers);
    if (threads->mappers == NULL) {
        perror("malloc");
        exit(1);
    }
    threads->mappers_count = args->mappers;

    threads->reducers = (pthread_t *)malloc(sizeof(pthread_t) * args->reducers);
    if (threads->reducers == NULL) {
        perror("malloc");
        exit(1);
    }
    threads->reducers_count = args->reducers;

    return;
}

void read_main_file(Arguments *arguments)
{
    // firs line contains the number of files to be read
    FILE *file = fopen(arguments->file, "r");
    if (file == NULL) {
        perror("fopen");
        exit(1);
    }

    fscanf(file, "%d", &arguments->number_of_files);
    arguments->files = (char **)malloc(sizeof(char *) * arguments->number_of_files);
    if (arguments->files == NULL) {
        perror("malloc");
        exit(1);
    }

    for (int i = 0; i < arguments->number_of_files; i++) {
        arguments->files[i] = (char *)malloc(sizeof(char) * 100);
        if (arguments->files[i] == NULL) {
            perror("malloc");
            exit(1);
        }
        fscanf(file, "%s", arguments->files[i]);
    }

    fclose(file);
    return;
}

void init_all(Arguments *arguments, char **argv)
{
    arguments->mappers = atoi(argv[1]);
    arguments->reducers = atoi(argv[2]);
    arguments->file = argv[3];
    read_main_file(arguments);
}