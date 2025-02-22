#include "functions.h"
#include <fstream>
#include <iostream>
#include <cctype>
#include <algorithm>
#include <stdexcept>
#include <unordered_map>

using namespace std;

void alloc_threads(Arguments* arguments, Threads* threads) {
    threads->number_of_mappers = arguments->no_of_mappers;
    threads->number_of_reducers = arguments->no_of_reducers;
    threads->mappers = new pthread_t[threads->number_of_mappers];
    threads->reducers = new pthread_t[threads->number_of_reducers];
    pthread_mutex_init(&threads->mutex, NULL);
    pthread_barrier_init(&threads->barrier, NULL, threads->number_of_mappers + threads->number_of_reducers);
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
        arguments.files[i].second = i;
    }
}

void init_arguments(char** argv, Arguments* arguments) {
    arguments->no_of_mappers = atoi(argv[1]);
    arguments->no_of_reducers = atoi(argv[2]);
    arguments->main_file = argv[3];
    readMainFile(*arguments);
    for (int i = 0; i < arguments->number_of_files; i++) {
        arguments->remaining_files.push_back(arguments->files[i]);
    }
}

void* mapper_function(void* arg) {
    Mapper* mapper = (Mapper*)arg;
    Arguments* arguments = mapper->arguments;
    Threads* threads = mapper->threads;
    int file_id;
    
    while (true) {
        file_id = -1;
        pthread_mutex_lock(&threads->mutex);
        if (!arguments->remaining_files.empty()) {
            file_id = arguments->remaining_files.back().second;
            arguments->remaining_files.pop_back();
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
            pthread_mutex_lock(&threads->mutex);
            if (!processed_word.empty()) {
                mapper->files[file_id].push_back(make_pair(processed_word, file_id));
            }
            pthread_mutex_unlock(&threads->mutex);
        }
    }

    pthread_barrier_wait(&threads->barrier);
    return NULL;
}

// Inițializează literele alfabetului
void initialize_letters(vector<pair<char, int>>& letters)
{
    for (char c = 'a'; c <= 'z'; c++) {
        letters.push_back(pair<char, int>(c, 0));
    }
}

// Agregă cuvintele din mapper pentru o literă specifică
vector<pair<string, vector<int>>> aggregate_words_for_letter(
    char letter,
    const vector<vector<pair<string, int>>>& files
)
{
    vector<pair<string, vector<int>>> words_for_letter;
    unordered_map<string, vector<int>> word_map;

    for (const auto& file : files) {
        for (const auto& word_pair : file) {
            if (word_pair.first[0] == letter) {
                // Adăugăm word_pair.second doar dacă nu există deja
                if (word_map[word_pair.first].empty() || 
                    find(word_map[word_pair.first].begin(), word_map[word_pair.first].end(), word_pair.second) == word_map[word_pair.first].end()) {
                    word_map[word_pair.first].push_back(word_pair.second);
                }
            }
        }
    }

    // Convertim `unordered_map` în `vector<pair<string, vector<int>>>` pentru a păstra ieșirea compatibilă
    for (const auto& [word, indices] : word_map) {
        words_for_letter.emplace_back(word, indices);
    }
    return words_for_letter;
}

// Sortează cuvintele descrescător după numărul de fișiere și alfabetic
void sort_words(vector<pair<string, vector<int>>>& words)
{
    // Comparator optimizat
    auto comparator = [](const pair<string, vector<int>>& a, const pair<string, vector<int>>& b) {
        size_t size_a = a.second.size();
        size_t size_b = b.second.size();

        if (size_a != size_b) {
            return size_a > size_b; // Ordine descrescătoare după mărimea vectorului
        }
        return a.first < b.first; // Ordine lexicografică pentru egalități
    };

    // Sortare
    sort(words.begin(), words.end(), comparator);
}


// Scrie rezultatele într-un fișier
void write_to_file(char letter, const vector<pair<string, vector<int>>>& words_for_letter)
{
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

void* reducer_function(void* arg) {
    Reducer* reducer = (Reducer*)arg;
    Threads* threads = reducer->threads;

    // 26 de litere (a-z) în alfabet
    const int total_letters = 26;
    const int letters_per_thread = 4; // Fiecare thread va prelucra 4 litere

    vector<pair<char, vector<pair<string, vector<int>>>>> grouped_words(26); // 26 litere
    pthread_barrier_wait(&threads->barrier);
    initialize_letters(reducer->letters);

    // Începem procesarea literei pe intervale
    while (true) {
        char start_letter = '\0';
        int range_start = -1;

        pthread_mutex_lock(&threads->mutex);
        for (int i = 0; i < total_letters; ++i) {
            if (!reducer->letters[i].second) { // Căutăm litera neprocesată
                start_letter = reducer->letters[i].first;
                range_start = i;
                reducer->letters[i].second = 1; // Marcăm litera ca procesată
                break;
            }
        }
        pthread_mutex_unlock(&threads->mutex);

        if (start_letter == '\0') {
            break; // Toate literele au fost procesate
        }

        // Calculăm intervalul de litere care va fi procesat de acest thread
        int range_end = min(range_start + letters_per_thread - 1, total_letters - 1);

        // Agregăm cuvintele pentru fiecare literă din intervalul [range_start, range_end]
        for (int i = range_start; i <= range_end; ++i) {
            char letter = reducer->letters[i].first;
            auto words_for_letter = aggregate_words_for_letter(letter, reducer->mapper->files);
            sort_words(words_for_letter);
            write_to_file(letter, words_for_letter);
        }
    }

    return NULL;
}
