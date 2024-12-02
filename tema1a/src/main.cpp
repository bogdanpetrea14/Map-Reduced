#include <iostream>
#include "functions.h"
#include "structures.h"

using namespace std;

int main(int argc, char* argv[]) {
    if (argc < 4) {
        cout << "Usage: ./tema2 <no_of_mappers> <no_of_reducers> <main_file>\n";
        return -1;
    }

    Arguments arguments;
    init_arguments(argv, &arguments);
    Threads threads;
    alloc_threads(&arguments, &threads);

    Mapper mapper;
    mapper.arguments = &arguments;
    mapper.threads = &threads;
    mapper.files.resize(arguments.number_of_files);

    Reducer reducer;
    reducer.arguments = &arguments;
    reducer.threads = &threads;
    reducer.mapper = &mapper;

    for (int i = 0; i < threads.number_of_mappers; i++) {
        pthread_create(&threads.mappers[i], NULL, mapper_function, &mapper);
    }

    for (int i = 0; i < threads.number_of_reducers; i++) {
        pthread_create(&threads.reducers[i], NULL, reducer_function, &reducer);
    }

    for (int i = 0; i < threads.number_of_mappers; i++) {
        pthread_join(threads.mappers[i], NULL);
    }

    for (int i = 0; i < threads.number_of_reducers; i++) {
        pthread_join(threads.reducers[i], NULL);
    }

    cleanup_threads(&threads);
    return 0;
}
