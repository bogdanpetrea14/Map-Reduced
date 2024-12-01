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
    arguments->status = (int *)malloc(sizeof(int) * arguments->number_of_files);
    if (arguments->status == NULL) {
        perror("malloc7");
        exit(1);
    }

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
    
    for (int i = 0; i < main_args->number_of_files; i++) {
        // check if the file is processed
        if (main_args->status[i] == 1 || main_args->status[i] == 0) {
            continue;
        }
        
        // here, we know for sure that the file is not processed
        // so we lock the mutex
        if (pthread_mutex_lock(&threads->mutex) != 0) {
            perror("pthread_mutex_lock");
            exit(1);
        }

        // open the file
        
        FILE *file = fopen(main_args->files[i], "r");
        if (file == NULL) {
            perror("fopen2");
            exit(1);
        }

        // read the file and process it
        char word[100];
        
        // we take one word at a time, make it lowercase
        // for those like that's, we will use thats
        while (fscanf(file, "%s", word) != EOF) {
            for (int j = 0; j < strlen(word); j++) {
                if (word[j] == '.' || 
                    word[j] == ',' || 
                    word[j] == '?' || 
                    word[j] == '!' || 
                    word[j] == ';' || 
                    word[j] == ':') {
                    word[j] = '\0';
                    break;
                }
                word[j] = tolower(word[j]);
            }

            // check if the word is already in the list of pairs
            int found = 0;
            for (int j = 0; j < files->lists[i].size; j++) {
                if (strcmp(files->lists[i].pairs[j].word, word) == 0) {
                    found = 1;
                    break;
                }
            }

            if (found == 1) {
                continue;
            }

            // realloc if needed
            if (files->lists[i].size == LIST_SIZE) {
                files->lists[i].pairs = (Pair *)realloc(files->lists[i].pairs, sizeof(Pair) * (files->lists[i].size + LIST_SIZE));
                if (files->lists[i].pairs == NULL) {
                    perror("realloc1");
                    exit(1);
                }
            }

            // add the word to the list of pairs
            files->lists[i].pairs[files->lists[i].size].file_id = i + 1;
            files->lists[i].pairs[files->lists[i].size].word = (char *)malloc(sizeof(char) * 100);
            if (files->lists[i].pairs[files->lists[i].size].word == NULL) {
                perror("malloc10");
                exit(1);
            }
            strcpy(files->lists[i].pairs[files->lists[i].size].word, word);
            files->lists[i].size++;
        }

        // close the file
        fclose(file);

        // mark the file as processed
        main_args->status[i] = 1;
        
        // unlock the mutex
        if (pthread_mutex_unlock(&threads->mutex) != 0) {
            perror("pthread_mutex_unlock");
            exit(1);
        }
    }

    // wait for all the threads to finish processing the files
    pthread_barrier_wait(&threads->barrier);
}

void* reducers_function(void* arguments) {
    ReducerArgs *args = (ReducerArgs *)arguments;
    Files *files = args->files;
    Threads *threads = args->threads;
    Letter *letters = args->letters;

    // Iterăm prin literele alfabetului pentru reducere
    for (int i = 0; i < 26; i++) {
        // Verificăm dacă litera a fost deja procesată
        pthread_mutex_lock(&threads->mutex);
        if (letters[i].taken_letter) {
            pthread_mutex_unlock(&threads->mutex);
            continue;
        }
        letters[i].taken_letter = 1; // Marcăm litera ca fiind procesată
        char current_letter = letters[i].letter;
        pthread_mutex_unlock(&threads->mutex);

        // Creăm un fișier de ieșire pentru litera curentă
        char filename[10];
        snprintf(filename, sizeof(filename), "%c.txt", current_letter);
        FILE *output_file = fopen(filename, "w");
        if (!output_file) {
            perror("fopen reducer output");
            exit(1);
        }

        // Hash map pentru agregare {word -> {file_id1, file_id2, ...}}
        ReducerPair aggregated_words[LIST_SIZE];
        int aggregated_count = 0;

        // Iterăm prin toate listele parțiale și agregăm cuvintele care încep cu litera curentă
        for (int j = 0; j < files->size; j++) {
            List *list = &files->lists[j];
            for (int k = 0; k < list->size; k++) {
                char *word = list->pairs[k].word;
                int file_id = list->pairs[k].file_id;

                // Verificăm dacă cuvântul începe cu litera curentă
                if (tolower(word[0]) != current_letter) {
                    continue;
                }

                // Căutăm cuvântul în lista agregată
                int found = 0;
                for (int l = 0; l < aggregated_count; l++) {
                    if (strcmp(aggregated_words[l].word, word) == 0) {
                        // Adăugăm ID-ul fișierului dacă nu există deja
                        int duplicate = 0;
                        for (int m = 0; m < aggregated_words[l].size; m++) {
                            if (aggregated_words[l].in_files[m] == file_id) {
                                duplicate = 1;
                                break;
                            }
                        }
                        if (!duplicate) {
                            aggregated_words[l].in_files[aggregated_words[l].size++] = file_id;
                        }
                        found = 1;
                        break;
                    }
                }

                // Dacă cuvântul nu există în lista agregată, îl adăugăm
                if (!found) {
                    aggregated_words[aggregated_count].word = strdup(word);
                    aggregated_words[aggregated_count].in_files = (int *)malloc(sizeof(int) * LIST_SIZE);
                    aggregated_words[aggregated_count].in_files[0] = file_id;
                    aggregated_words[aggregated_count].size = 1;
                    aggregated_count++;
                }
            }
        }

        // Sortăm cuvintele în ordine descrescătoare după numărul de fișiere în care apar
        for (int l = 0; l < aggregated_count - 1; l++) {
            for (int m = l + 1; m < aggregated_count; m++) {
                if (aggregated_words[l].size < aggregated_words[m].size ||
                    (aggregated_words[l].size == aggregated_words[m].size &&
                     strcmp(aggregated_words[l].word, aggregated_words[m].word) > 0)) {
                    ReducerPair temp = aggregated_words[l];
                    aggregated_words[l] = aggregated_words[m];
                    aggregated_words[m] = temp;
                }
            }
        }

        // Scriem rezultatele în fișierul de ieșire
        for (int l = 0; l < aggregated_count; l++) {
            fprintf(output_file, "%s:[", aggregated_words[l].word);
            for (int m = 0; m < aggregated_words[l].size; m++) {
                fprintf(output_file, "%d", aggregated_words[l].in_files[m]);
                if (m < aggregated_words[l].size - 1) {
                    fprintf(output_file, " ");
                }
            }
            fprintf(output_file, "]\n");

            // Eliberăm memoria alocată pentru cuvânt și lista de fișiere
            free(aggregated_words[l].word);
            free(aggregated_words[l].in_files);
        }

        fclose(output_file);
    }

    pthread_exit(NULL);
}
