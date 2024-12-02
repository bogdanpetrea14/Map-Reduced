#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <string>
#include <vector>
#include <utility>
#include <pthread.h>  // Pentru pthread_t, pthread_mutex_t, pthread_barrier_t

using namespace std;

typedef struct {
    string main_file;
    int number_of_files;
    int no_of_mappers;
    int no_of_reducers;
    vector<pair<string, int>> files;
    vector<bool> status; // 0 - not processed, 1 - processed
} Arguments;

typedef struct {
    pthread_t* mappers;
    pthread_t* reducers;
    pthread_mutex_t mutex;
    pthread_barrier_t barrier;
    int number_of_mappers;
    int number_of_reducers;
} Threads;

typedef struct {
    vector<vector<pair<string, int>>> files;
    Threads* threads;
    Arguments* arguments;
} Mapper;

typedef struct {
    vector<pair<char, int>> letters; // literele de la a la z
    vector<pair<string, vector<int>>> words;
    Threads* threads;
    Arguments* arguments;
    Mapper* mapper;
} Reducer;

#endif  // STRUCTURES_H
