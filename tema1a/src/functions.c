#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "structures.h"

void alloc_threads(Threads *threads, Arguments *args)
{
    threads->mappers = (pthread_t *)malloc(sizeof(pthread_t) * args->mappers);
    if (threads->mappers == NULL) {
        perror("malloc1");
        exit(1);
    }
    threads->mappers_count = args->mappers;

    threads->reducers = (pthread_t *)malloc(sizeof(pthread_t) * args->reducers);
    if (threads->reducers == NULL) {
        perror("malloc2");
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
        perror("fopen1");
        exit(1);
    }

    fscanf(file, "%d", &arguments->number_of_files);
    arguments->files = (char **)malloc(sizeof(char *) * arguments->number_of_files);
    if (arguments->files == NULL) {
        perror("malloc5");
        exit(1);
    }

    for (int i = 0; i < arguments->number_of_files; i++) {
        arguments->files[i] = (char *)malloc(sizeof(char) * 100);
        if (arguments->files[i] == NULL) {
            perror("malloc6");
            exit(1);
        }
        fscanf(file, "%s", arguments->files[i]);

        arguments->file_number = i + 1;
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
        perror("malloc7");
        exit(1);
    }

    read_main_file(arguments);
}

void init_files(Files *files, Arguments *args)
{
    files->size = args->number_of_files;
    files->lists = (List *)malloc(sizeof(List) * args->number_of_files);
    if (files->lists == NULL) {
        perror("malloc8");
        exit(1);
    }

    for (int i = 0; i < args->number_of_files; i++) {
        files->lists[i].size = 0;
        files->lists[i].file_id = args->file_number;
        files->lists[i].file_name = args->files[i];
        files->lists[i].pairs = (Pair *)malloc(sizeof(Pair) * LIST_SIZE);
        if (files->lists[i].pairs == NULL) {
            perror("malloc9");
            exit(1);
        }
    }
}

void* mappers_function(void* arguments) {
    MapperArgs *args = (MapperArgs *)arguments;
    Files *files = args->files;
    Arguments *main_args = args->args;
    Threads *threads = args->threads;
    
    for (int i = 0; i < main_args->number_of_files; i++) {
        // check if the file is processed
        if (main_args->status[i] == 1 || main_args->status[i] == 0) {
            continue;
        }
        
        // here, we know for sure that the file is not processed
        // so we lock the mutex
        if (pthread_mutex_lock(&threads->mutex) != 0) {
            perror("pthread_mutex_lock");
            exit(1);
        }

        // open the file
        printf("Processing %s\n", main_args->files[i]);
        // concatenate ../checker/" with the file name
        char file_name[100];
        strcpy(file_name, "../checker/");
        strcat(file_name, main_args->files[i]);
        FILE *file = fopen(file_name, "r");
        if (file == NULL) {
            perror("fopen2");
            exit(1);
        }

        // read the file and process it
        char word[100];
        
        // we take one word at a time, make it lowercase
        // for those like that's, we will use thats
        while (fscanf(file, "%s", word) != EOF) {
            for (int j = 0; j < strlen(word); j++) {
                if (word[j] == '.' || word[j] == ',' || word[j] == '?' || word[j] == '!' || word[j] == ';' || word[j] == ':') {
                    word[j] = '\0';
                    break;
                }
                word[j] = tolower(word[j]);
            }

            // add the word to the list of pairs
            files->lists[i].pairs[files->lists[i].size].file_id = i + 1;
            files->lists[i].pairs[files->lists[i].size].word = (char *)malloc(sizeof(char) * 100);
            if (files->lists[i].pairs[files->lists[i].size].word == NULL) {
                perror("malloc10");
                exit(1);
            }
            strcpy(files->lists[i].pairs[files->lists[i].size].word, word);
            files->lists[i].size++;
        }

        // close the file
        fclose(file);

        // mark the file as processed
        main_args->status[i] = 1;
        
        // unlock the mutex
        if (pthread_mutex_unlock(&threads->mutex) != 0) {
            perror("pthread_mutex_unlock");
            exit(1);
        }
    }

    // wait for all the threads to finish processing the files
    pthread_barrier_wait(&threads->barrier);
}
