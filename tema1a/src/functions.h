#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "structures.h"
#include <string>
#include <vector>

// Funcții pentru inițializare
void init_arguments(char** argv, Arguments* arguments);
void alloc_threads(Arguments* arguments, Threads* threads);
void readMainFile(Arguments& arguments);

// Funcțiile mapper
void* mapper_function(void* arg);

// Funcțiile reducer
void* reducer_function(void* arg);

// Funcție pentru eliberarea resurselor
void cleanup_threads(Threads* threads);

#endif  // FUNCTIONS_H
