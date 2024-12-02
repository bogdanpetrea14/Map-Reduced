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

// Inițializează literele alfabetului
void initialize_letters(vector<char>& letters) {
    for (char c = 'a'; c <= 'z'; c++) {
        letters.push_back(c);
    }
}

// Agregă cuvintele din mapper pentru o literă specifică
vector<pair<string, vector<int>>> aggregate_words_for_letter(
    char letter,
    const vector<vector<pair<string, int>>>& files
) {
    vector<pair<string, vector<int>>> words_for_letter;

    for (const auto& file : files) {
        for (const auto& word_pair : file) {
            if (word_pair.first[0] == letter) {
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
    return words_for_letter;
}

// Sortează cuvintele descrescător după numărul de fișiere și alfabetic
void sort_words(vector<pair<string, vector<int>>>& words) {
    sort(words.begin(), words.end(),
         [](const pair<string, vector<int>>& a, const pair<string, vector<int>>& b) {
             if (a.second.size() != b.second.size()) {
                 return a.second.size() > b.second.size();
             }
             return a.first < b.first;
         });
}

// Scrie rezultatele într-un fișier
void write_to_file(char letter, const vector<pair<string, vector<int>>>& words_for_letter) {
    ofstream out_file(string(1, letter) + ".txt");

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

// Funcția principală a reducerului
void* reducer_function(void* arg) {
    Reducer* reducer = (Reducer*)arg;

    // Inițializează literele alfabetului
    initialize_letters(reducer->letters);

    // Parcurge fiecare literă și procesează cuvintele asociate
    for (char letter : reducer->letters) {
        // Agregare cuvinte pentru litera curentă
        vector<pair<string, vector<int>>> words_for_letter =
            aggregate_words_for_letter(letter, reducer->mapper->files);

        // Sortare cuvinte
        sort_words(words_for_letter);

        // Scriere în fișier
        write_to_file(letter, words_for_letter);
    }

    return NULL;
}
