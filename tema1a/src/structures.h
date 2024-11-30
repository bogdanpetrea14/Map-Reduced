#ifndef STRUCTURES_H
#define STRUCTURES_H

typedef struct {
    int mappers;
    int reducers;
    char *file;
    int number_of_files;
    char **files;
} Arguments;

typedef struct {
    pthread_t* mappers;
    pthread_t* reducers;
    int mappers_count;
    int reducers_count;
} Threads;

#endif // STRUCTURES_H
