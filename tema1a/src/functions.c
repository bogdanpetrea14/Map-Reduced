#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "structures.h"

void alloc_threads(Threads *threads, Arguments *args)
{
    threads->mappers = (pthread_t *)malloc(sizeof(pthread_t) * args->mappers);
    if (threads->mappers == NULL) {
        perror("malloc1");
        exit(1);
    }
    threads->mappers_count = args->mappers;

    threads->reducers = (pthread_t *)malloc(sizeof(pthread_t) * args->reducers);
    if (threads->reducers == NULL) {
        perror("malloc2");
        exit(1);
    }
    threads->reducers_count = args->reducers;

    return;
}

void read_main_file(Arguments *arguments)
{
    // firs line contains the number of files to be read
    FILE *file = fopen(arguments->file, "r");
    if (file == NULL) {
        perror("fopen1");
        exit(1);
    }

    fscanf(file, "%d", &arguments->number_of_files);
    arguments->files = (char **)malloc(sizeof(char *) * arguments->number_of_files);
    if (arguments->files == NULL) {
        perror("malloc5");
        exit(1);
    }

    arguments->status = (int *)malloc(sizeof(int) * arguments->number_of_files);
    if (arguments->status == NULL) {
        perror("malloc7");
        exit(1);
    }

    for (int i = 0; i < arguments->number_of_files; i++) {
        arguments->files[i] = (char *)malloc(sizeof(char) * 100);
        if (arguments->files[i] == NULL) {
            perror("malloc6");
            exit(1);
        }
        fscanf(file, "%s", arguments->files[i]);

        arguments->file_number = i + 1;
        arguments->status[i] = -1;
    }

    fclose(file);
    return;
}

void init_all(Arguments *arguments, char **argv)
{
    arguments->mappers = atoi(argv[1]);
    arguments->reducers = atoi(argv[2]);
    arguments->file = argv[3];
    read_main_file(arguments);
}

void init_files(Files *files, Arguments *args)
{
    files->size = args->number_of_files;
    files->lists = (List *)malloc(sizeof(List) * args->number_of_files);
    if (files->lists == NULL) {
        perror("malloc8");
        exit(1);
    }

    for (int i = 0; i < args->number_of_files; i++) {
        files->lists[i].size = 0;
        files->lists[i].file_id = args->file_number;
        files->lists[i].file_name = args->files[i];
        files->lists[i].pairs = (Pair *)malloc(sizeof(Pair) * LIST_SIZE);
        if (files->lists[i].pairs == NULL) {
            perror("malloc9");
            exit(1);
        }
    }
}

void* mappers_function(void* arguments) {
    MapperArgs *args = (MapperArgs *)arguments;
    Files *files = args->files;
    Arguments *main_args = args->args;
    Threads *threads = args->threads;

    while (1) {
        int file_to_process = -1;

        pthread_mutex_lock(&threads->mutex);
        for (int i = 0; i < main_args->number_of_files; i++) {
            if (main_args->status[i] == -1) {
                file_to_process = i;
                main_args->status[i] = 0; // Mark as processing
                break;
            }
        }
        pthread_mutex_unlock(&threads->mutex);

        if (file_to_process == -1) {
            break; // No more files to process
        }

        FILE *file = fopen(main_args->files[file_to_process], "r");
        if (file == NULL) {
            perror("fopen");
            continue; // Skip file on error
        }

        char word[100];
        while (fscanf(file, "%s", word) != EOF) {
            for (int j = 0; j < strlen(word); j++) {
                if (!isalpha(word[j])) {
                    word[j] = '\0';
                    break;
                }
                word[j] = tolower(word[j]);
            }

            pthread_mutex_lock(&threads->mutex);
            // Add word to the list, realloc if necessary
            List *list = &files->lists[file_to_process];
            if (list->size == LIST_SIZE) {
                list->pairs = realloc(list->pairs, sizeof(Pair) * (list->size + LIST_SIZE));
                if (!list->pairs) {
                    perror("realloc");
                    exit(1);
                }
            }

            list->pairs[list->size].word = strdup(word);
            list->pairs[list->size].file_id = file_to_process + 1;
            list->size++;
            pthread_mutex_unlock(&threads->mutex);
        }

        fclose(file);
        pthread_mutex_lock(&threads->mutex);
        main_args->status[file_to_process] = 1; // Mark as processed
        pthread_mutex_unlock(&threads->mutex);
    }

    pthread_barrier_wait(&threads->barrier);
    pthread_exit(NULL);
}

void* reducers_function(void* arguments) {
    ReducerArgs *args = (ReducerArgs *)arguments;
    Files *files = args->files;
    Threads *threads = args->threads;
    Letter *letters = args->letters;

    while (1) {
        int letter_to_process = -1;
        char current_letter;

        // Căutăm următoarea literă neprocesată
        pthread_mutex_lock(&threads->mutex);
        for (int i = 0; i < 26; i++) {
            if (letters[i].taken_letter == 0) {
                letter_to_process = i;
                letters[i].taken_letter = 1; // Marcăm litera ca fiind procesată
                current_letter = letters[i].letter;
                break;
            }
        }
        pthread_mutex_unlock(&threads->mutex);

        if (letter_to_process == -1) {
            break; // Toate literele au fost procesate
        }

        // Creăm un fișier de ieșire pentru litera curentă
        char filename[10];
        snprintf(filename, sizeof(filename), "%c.txt", current_letter);
        FILE *output_file = fopen(filename, "w");
        if (!output_file) {
            perror("fopen reducer output");
            continue; // Continuăm cu următoarea literă
        }

        // Hash map pentru agregare {word -> {file_id1, file_id2, ...}}
        ReducerPair *aggregated_words = (ReducerPair *)malloc(sizeof(ReducerPair) * LIST_SIZE);
        if (!aggregated_words) {
            perror("Error allocating aggregated_words in ReducerArgs");
            fclose(output_file);
            continue;
        }
        int aggregated_count = 0;

        // Iterăm prin toate listele de fișiere pentru a agrega cuvintele care încep cu litera curentă
        for (int i = 0; i < files->size; i++) {
            List *list = &files->lists[i];
            for (int j = 0; j < list->size; j++) {
                char *word = list->pairs[j].word;
                int file_id = list->pairs[j].file_id;

                if (tolower(word[0]) != current_letter) {
                    continue; // Dacă cuvântul nu începe cu litera curentă, trecem mai departe
                }

                // Căutăm cuvântul în lista agregată
                int found = 0;
                for (int k = 0; k < aggregated_count; k++) {
                    if (strcmp(aggregated_words[k].word, word) == 0) {
                        // Adăugăm ID-ul fișierului dacă nu există deja
                        int duplicate = 0;
                        for (int l = 0; l < aggregated_words[k].size; l++) {
                            if (aggregated_words[k].in_files[l] == file_id) {
                                duplicate = 1;
                                break;
                            }
                        }
                        if (!duplicate) {
                            aggregated_words[k].in_files[aggregated_words[k].size++] = file_id;
                        }
                        found = 1;
                        break;
                    }
                }

                // Dacă cuvântul nu există, îl adăugăm în lista agregată
                if (!found) {
                    aggregated_words[aggregated_count].word = strdup(word);
                    aggregated_words[aggregated_count].in_files = (int *)malloc(sizeof(int) * LIST_SIZE);
                    if (!aggregated_words[aggregated_count].in_files) {
                        perror("malloc in_files");
                        exit(1);
                    }
                    aggregated_words[aggregated_count].in_files[0] = file_id;
                    aggregated_words[aggregated_count].size = 1;
                    aggregated_count++;
                }
            }
        }

        // Sortăm lista agregată după dimensiune și ordine lexicografică
        for (int i = 0; i < aggregated_count - 1; i++) {
            for (int j = i + 1; j < aggregated_count; j++) {
                if (aggregated_words[i].size < aggregated_words[j].size ||
                    (aggregated_words[i].size == aggregated_words[j].size &&
                     strcmp(aggregated_words[i].word, aggregated_words[j].word) > 0)) {
                    ReducerPair temp = aggregated_words[i];
                    aggregated_words[i] = aggregated_words[j];
                    aggregated_words[j] = temp;
                }
            }
        }

        // Scriem rezultatele în fișierul de ieșire
        for (int i = 0; i < aggregated_count; i++) {
            fprintf(output_file, "%s:[", aggregated_words[i].word);
            for (int j = 0; j < aggregated_words[i].size; j++) {
                fprintf(output_file, "%d", aggregated_words[i].in_files[j]);
                if (j < aggregated_words[i].size - 1) {
                    fprintf(output_file, " ");
                }
            }
            fprintf(output_file, "]\n");

            free(aggregated_words[i].word);
            free(aggregated_words[i].in_files);
        }

        fclose(output_file);
        free(aggregated_words);
    }

    pthread_barrier_wait(&threads->barrier);
    pthread_exit(NULL);
}
