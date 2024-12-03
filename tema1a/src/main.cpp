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

    int r;
    
    int total_threads = threads.number_of_mappers + threads.number_of_reducers;
    for (int i = 0; i < total_threads; i++) {
        if (i < threads.number_of_mappers) {
            r = pthread_create(&threads.mappers[i], NULL, mapper_function, &mapper);
        } else {
            r = pthread_create(&threads.reducers[i - threads.number_of_mappers], NULL, reducer_function, &reducer);
        }
        if (r) {
            cout << "Error creating thread " << i << "\n";
            return -1;
        }
    }
    
    for (int i = 0; i < total_threads; i++) {
        if (i < threads.number_of_mappers) {
            r = pthread_join(threads.mappers[i], NULL);
        } else {
            r = pthread_join(threads.reducers[i - threads.number_of_mappers], NULL);
        }
        if (r) {
            cout << "Error joining thread " << i << "\n";
            return -1;
        }
    }

    cleanup_threads(&threads);
    return 0;
}
