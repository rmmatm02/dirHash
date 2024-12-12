#include "file_list.h"
#include "constants.h" 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

FileList* file_list_init(size_t initial_capacity) {
    FileList *fl = (FileList*)malloc(sizeof(FileList));
    if (!fl) {
        perror("Failed to allocate FileList");
        exit(EXIT_FAILURE);
    }
    fl->filepaths = (char**)malloc(initial_capacity * sizeof(char *));
    if (!fl->filepaths) {
        perror("Failed to allocate filepaths array");
        free(fl);
        exit(EXIT_FAILURE);
    }
    fl->capacity = initial_capacity;
    fl->size = 0;
    return fl;
}

static int compare_filepaths(const void *a, const void *b) {
    const char *path_a = *(const char **)a;
    const char *path_b = *(const char **)b;
    return strcmp(path_a, path_b);
}

void file_list_sort(FileList *fl) {
    qsort(fl->filepaths, fl->size, sizeof(char *), compare_filepaths);
}

void file_list_add(FileList *fl, const char *filepath) {
    if (fl->size == fl->capacity) {
        fl->capacity *= 2;
        char **new_array = (char**)realloc(fl->filepaths, fl->capacity * sizeof(char *));
        if (!new_array) {
            perror("Failed to resize filepaths array");
            exit(EXIT_FAILURE);
        }
        fl->filepaths = new_array;
    }
    char *copy = strdup(filepath);
    if (!copy) {
        perror("Failed to duplicate filepath");
        exit(EXIT_FAILURE);
    }
    fl->filepaths[fl->size++] = copy;
}

void file_list_free(FileList *fl) {
    if (fl) {
        for (size_t i = 0; i < fl->size; ++i) {
            free(fl->filepaths[i]);
        }
        free(fl->filepaths);
        free(fl);
    }
}

