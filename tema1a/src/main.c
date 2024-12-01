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

    // init the barrier
    pthread_barrier_init(&threads.barrier, NULL, args.mappers);

    // init mutex
    pthread_mutex_init(&threads.mutex, NULL);

    // Inițializare argumente pentru fiecare thread Mapper
    MapperArgs mapper_args;
    mapper_args.args = &args;
    mapper_args.files = &files;
    mapper_args.threads = &threads;

    // Crearea thread-urilor Mapper
    for (int i = 0; i < args.mappers; i++) {
        if (pthread_create(&threads.mappers[i], NULL, mappers_function, &mapper_args) != 0) {
            perror("pthread_create");
            exit(1);
        }
    }

    // Așteptarea thread-urilor Mapper
    for (int i = 0; i < args.mappers; i++) {
        if (pthread_join(threads.mappers[i], NULL) != 0) {
            perror("pthread_join");
            exit(1);
        }
    }

    // Aici trebuie implementată partea pentru Reducers (Reduce phase)
    // print the pairs
    // for (int i = 0; i < files.size; i++) {
    //     for (int j = 0; j < files.lists[i].size; j++) {
    //         printf("%s %d\n", files.lists[i].pairs[j].word, files.lists[i].pairs[j].file_id);
    //     }
    // }

    // Eliberare resurse
    for (int i = 0; i < files.size; i++) {
        free(files.lists[i].pairs);
    }
    free(files.lists);
    free(args.files);
    free(threads.mappers);
    free(threads.reducers);
    free(args.status);

    return 0;
}
