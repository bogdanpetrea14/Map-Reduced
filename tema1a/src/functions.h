#include "structures.h"

void alloc_threads(Threads *threads, Arguments *args);
void read_main_file(Arguments *arguments);
void init_all(Arguments *arguments, char **argv);
void init_files(Files *files, Arguments *args);
void* mappers_function(void* arguments);
void* reducers_function(void* arguments);