#ifndef STRUCTURES_H
#define STRUCTURES_H

#define LIST_SIZE 1024

typedef struct {
    int mappers;
    int reducers;
    char *file;
    int number_of_files;
    char **files;
    int* status; // -1 - not processed, 0 - processing, 1 - processed
} Arguments;

typedef struct {
    pthread_t* mappers;
    pthread_t* reducers;
    pthread_mutex_t* mutex;
    pthread_barrier_t* barrier;
    int mappers_count;
    int reducers_count;
} Threads;

typedef struct {
    char *word;
    char *file;
} Word;

#endif // STRUCTURES_H
