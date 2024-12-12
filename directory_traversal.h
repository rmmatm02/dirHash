#ifndef DIRECTORY_TRAVERSAL_H
#define DIRECTORY_TRAVERSAL_H

#include "file_list.h"

#define MAX_PATH_LENGTH 4096

void concatenate_path(const char *base, const char *name, char *dest, size_t dest_size);
void traverse_directory(const char *path, FileList *fl);

#endif

