#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "structures.h"
#include "functions.h"

int main(int argc, char **argv) {
    if (argc != 4) {
        printf("Usage: %s <mappers> <reducers> <file>\n", argv[0]);
        return 1;
    }

    Arguments args;
    Threads threads;
    Files files;

    // Inițializare structuri
    init_all(&args, argv);
    alloc_threads(&threads, &args);
    init_files(&files, &args);

    // Inițializare barrier pentru sincronizare
    pthread_barrier_init(&threads.barrier, NULL, args.mappers);

    // Inițializare mutex
    pthread_mutex_init(&threads.mutex, NULL);

    // Inițializare argumente pentru thread-urile Mapper
    MapperArgs mapper_args;
    mapper_args.args = &args;
    mapper_args.files = &files;
    mapper_args.threads = &threads;

    // Inițializare argumente pentru thread-urile Reducer
    ReducerArgs reducer_args;
    reducer_args.args = &args;
    reducer_args.files = &files;
    reducer_args.threads = &threads;

    // Inițializare pentru fiecare literă
    for (int i = 0; i < 26; i++) {
        reducer_args.letters[i].taken_letter = 0;
        reducer_args.letters[i].letter = 'a' + i;
    }

    // Asigură-te că pairs este inițializat dacă e necesar
    reducer_args.pairs = malloc(sizeof(ReducerPair) * LIST_SIZE);
    if (!reducer_args.pairs) {
        perror("Error allocating pairs in ReducerArgs");
        exit(1);
    }

    // Crearea thread-urilor Mapper
    for (int i = 0; i < args.mappers; i++) {
        if (pthread_create(&threads.mappers[i], NULL, mappers_function, &mapper_args) != 0) {
            perror("pthread_create mapper");
            exit(1);
        }
    }

    // Crearea thread-urilor Reducer
    for (int i = 0; i < args.reducers; i++) {
        if (pthread_create(&threads.reducers[i], NULL, reducers_function, &reducer_args) != 0) {
            perror("pthread_create reducer");
            exit(1);
        }
    }

    // Așteptarea thread-urilor Mapper
    for (int i = 0; i < args.mappers; i++) {
        if (pthread_join(threads.mappers[i], NULL) != 0) {
            perror("pthread_join mapper");
            exit(1);
        }
    }

    // Așteptarea thread-urilor Reducer
    for (int i = 0; i < args.reducers; i++) {
        if (pthread_join(threads.reducers[i], NULL) != 0) {
            perror("pthread_join reducer");
            exit(1);
        }
    }

    // Eliberare resurse
    for (int i = 0; i < files.size; i++) {
        for (int j = 0; j < files.lists[i].size; j++) {
            free(files.lists[i].pairs[j].word);
        }
        free(files.lists[i].pairs);
    }
    free(files.lists);
    free(args.files);
    free(threads.mappers);
    free(threads.reducers);
    free(args.status);
    pthread_mutex_destroy(&threads.mutex);
    pthread_barrier_destroy(&threads.barrier);

    return 0;
}
