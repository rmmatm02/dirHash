#include <stdint.h>    // For uint64_t
#include <linux/limits.h> // For PATH_MAX
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>    // For scandir and alphasort
#include <xxhash.h>    // For XXH64

#include "file_list.h"
#include "directory_traversal.h"

uint64_t hash_file(const char *filepath); // Forward declaration

void traverse_directory(const char *path, FileList *fl, uint64_t *final_hash) {
    struct dirent **namelist;
    int n = scandir(path, &namelist, NULL, alphasort); // Ensure deterministic order
    if (n < 0) {
        perror("Failed to scan directory");
        return;
    }

    for (int i = 0; i < n; i++) {
        if (strcmp(namelist[i]->d_name, ".") == 0 || strcmp(namelist[i]->d_name, "..") == 0) {
            free(namelist[i]);
            continue;
        }

        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, namelist[i]->d_name);

        struct stat st;
        if (lstat(full_path, &st) == -1) {
            perror("Failed to stat file");
            free(namelist[i]);
            continue;
        }

        uint64_t name_hash = XXH64(namelist[i]->d_name, strlen(namelist[i]->d_name), 0);

        if (S_ISDIR(st.st_mode)) {
            traverse_directory(full_path, fl, final_hash);
        } else if (S_ISREG(st.st_mode)) {
            uint64_t file_hash = hash_file(full_path);
            *final_hash ^= name_hash ^ file_hash;
        } else if (S_ISLNK(st.st_mode)) {
            char link_target[PATH_MAX];
            ssize_t len = readlink(full_path, link_target, sizeof(link_target) - 1);
            if (len != -1) {
                link_target[len] = '\0';
                uint64_t link_hash = XXH64(link_target, strlen(link_target), 0);
                *final_hash ^= name_hash ^ link_hash;
            }
        }

        free(namelist[i]);
    }

    free(namelist);
}

