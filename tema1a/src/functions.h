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
void write_to_file(char letter, const vector<pair<string, vector<int>>>& words_for_letter);
void sort_words(vector<pair<string, vector<int>>>& words);
vector<pair<string, vector<int>>> aggregate_words_for_letter(
    char letter,
    const vector<vector<pair<string, int>>>& files
);
void initialize_letters(vector<char>& letters);

#endif  // FUNCTIONS_H
