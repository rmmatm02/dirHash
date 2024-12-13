#ifndef FILE_LIST_H
#define FILE_LIST_H

#include <stddef.h>

typedef struct {
    char **filepaths;
    size_t capacity;
    size_t size;
} FileList;

FileList* file_list_init(size_t initial_capacity);
void file_list_add(FileList *fl, const char *filepath);
void file_list_free(FileList *fl);

#endif

