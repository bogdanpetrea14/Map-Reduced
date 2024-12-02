#include "functions.h"
#include <fstream>
#include <iostream>
#include <cctype>
#include <algorithm>
#include <stdexcept>

using namespace std;

void alloc_threads(Arguments* arguments, Threads* threads) {
    threads->number_of_mappers = arguments->no_of_mappers;
    threads->number_of_reducers = arguments->no_of_reducers;
    threads->mappers = new pthread_t[threads->number_of_mappers];
    threads->reducers = new pthread_t[threads->number_of_reducers];
    pthread_mutex_init(&threads->mutex, NULL);
    pthread_barrier_init(&threads->barrier, NULL, threads->number_of_mappers);
}

void cleanup_threads(Threads* threads) {
    delete[] threads->mappers;
    delete[] threads->reducers;
    pthread_mutex_destroy(&threads->mutex);
    pthread_barrier_destroy(&threads->barrier);
}

void readMainFile(Arguments& arguments) {
    ifstream main_file(arguments.main_file);
    if (!main_file.is_open()) {
        throw runtime_error("Could not open file: " + arguments.main_file);
    }

    main_file >> arguments.number_of_files;
    arguments.files.resize(arguments.number_of_files);
    arguments.status.resize(arguments.number_of_files);

    for (int i = 0; i < arguments.number_of_files; i++) {
        main_file >> arguments.files[i].first;
        arguments.status[i] = false;
    }
}

void init_arguments(char** argv, Arguments* arguments) {
    arguments->no_of_mappers = atoi(argv[1]);
    arguments->no_of_reducers = atoi(argv[2]);
    arguments->main_file = argv[3];
    readMainFile(*arguments);
}

void* mapper_function(void* arg) {
    Mapper* mapper = (Mapper*)arg;
    Arguments* arguments = mapper->arguments;
    Threads* threads = mapper->threads;
    int file_id;

    while (true) {
        pthread_mutex_lock(&threads->mutex);
        file_id = -1;
        for (int i = 0; i < arguments->number_of_files; i++) {
            if (!arguments->status[i]) {
                file_id = i;
                arguments->status[i] = true;
                break;
            }
        }
        pthread_mutex_unlock(&threads->mutex);

        if (file_id == -1) {
            break;  // Toate fișierele au fost procesate
        }

        ifstream file(arguments->files[file_id].first);
        string word;
        while (file >> word) {
            string processed_word;
            for (char c : word) {
                if (isalpha(c)) {
                    processed_word += tolower(c);
                }
            }
            if (!processed_word.empty()) {
                pthread_mutex_lock(&threads->mutex);
                mapper->files[file_id].push_back(make_pair(processed_word, file_id));
                pthread_mutex_unlock(&threads->mutex);
            }
        }
    }

    pthread_barrier_wait(&threads->barrier);
    return NULL;
}

void* reducer_function(void* arg) {
    Reducer* reducer = (Reducer*)arg;
    Arguments* arguments = reducer->arguments;
    Threads* threads = reducer->threads;
    Mapper* mapper = reducer->mapper;

    // Inițializare reducer pentru litere
    for (char c = 'a'; c <= 'z'; c++) {
        reducer->letters.push_back(c);
    }

    for (char c : reducer->letters) {
        vector<pair<string, vector<int>>> words_for_letter;

        // Agregare cuvinte din mapper
        for (const auto& file : mapper->files) {
            for (const auto& word_pair : file) {
                if (word_pair.first[0] == c) {
                    auto it = find_if(words_for_letter.begin(), words_for_letter.end(),
                                      [&word_pair](const pair<string, vector<int>>& entry) {
                                          return entry.first == word_pair.first;
                                      });
                    if (it != words_for_letter.end()) {
                        if (find(it->second.begin(), it->second.end(), word_pair.second) == it->second.end()) {
                            it->second.push_back(word_pair.second);
                        }
                    } else {
                        words_for_letter.emplace_back(word_pair.first, vector<int>{word_pair.second});
                    }
                }
            }
        }

        // Sortare descrescătoare după numărul de fișiere și alfabetic
        sort(words_for_letter.begin(), words_for_letter.end(),
             [](const pair<string, vector<int>>& a, const pair<string, vector<int>>& b) {
                 if (a.second.size() != b.second.size()) {
                     return a.second.size() > b.second.size();
                 }
                 return a.first < b.first;
             });

        // Creare fișier pentru litera curentă
        ofstream out_file(string(1, c) + ".txt");

        // Scriere în fișier (chiar dacă este gol)
        if (!words_for_letter.empty()) {
            for (const auto& entry : words_for_letter) {
                out_file << entry.first << ":[";
                for (size_t i = 0; i < entry.second.size(); i++) {
                    out_file << entry.second[i] + 1; // ID-urile sunt incrementate cu 1
                    if (i < entry.second.size() - 1) {
                        out_file << " ";
                    }
                }
                out_file << "]" << endl;
            }
        }
    }

    return NULL;
}
