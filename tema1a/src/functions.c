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

    threads->mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t) * args->number_of_files);
    if (threads->mutex == NULL) {
        perror("malloc");
        exit(1);
    }

    threads->barrier = (pthread_barrier_t *)malloc(sizeof(pthread_barrier_t) * args->number_of_files);
    if (threads->barrier == NULL) {
        perror("malloc");
        exit(1);
    }

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

        arguments->status[i] = -1;
    }

    fclose(file);
    return;
}

void init_all(Arguments *arguments, char **argv)
{
    arguments->mappers = atoi(argv[1]);
    arguments->reducers = atoi(argv[2]);
    arguments->file = argv[3];
    arguments->status = (int *)malloc(sizeof(int) * arguments->number_of_files);
    if (arguments->status == NULL) {
        perror("malloc");
        exit(1);
    }

    read_main_file(arguments);
}


// function given for the mapper threads
// read the file and for each new word found, we create a pair
// {word, file_ID} and we add it to the list
// after finishing the file, the thread will close it
