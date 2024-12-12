#define _GNU_SOURCE // Enable GNU extensions like O_NOATIME // ADDED

#include "directory_traversal.h"
#include "file_list.h"
#include "constants.h" // Include shared constants
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h> // For close()
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h> // For syscall and SYS_getdents64
#include <linux/limits.h> // For PATH_MAX

struct linux_dirent64 {
    ino64_t d_ino;            // 64-bit inode number
    off64_t d_off;            // 64-bit offset to the next dirent
    unsigned short d_reclen;  // Length of this dirent
    unsigned char d_type;     // File type
    char d_name[];            // Null-terminated file name
};

#ifndef DT_UNKNOWN
#define DT_UNKNOWN 0
#endif
#ifndef DT_FIFO
#define DT_FIFO 1
#endif
#ifndef DT_CHR
#define DT_CHR 2
#endif
#ifndef DT_DIR
#define DT_DIR 4
#endif
#ifndef DT_BLK
#define DT_BLK 6
#endif
#ifndef DT_REG
#define DT_REG 8
#endif
#ifndef DT_LNK
#define DT_LNK 10
#endif
#ifndef DT_SOCK
#define DT_SOCK 12
#endif
#ifndef DT_WHT
#define DT_WHT 14
#endif

typedef struct {
    char path[MAX_PATH_LENGTH];
    unsigned char type;
} DirectoryEntry;

static int compare_entries(const void *a, const void *b) {
    const DirectoryEntry *entry_a = (const DirectoryEntry *)a;
    const DirectoryEntry *entry_b = (const DirectoryEntry *)b;
    return strcmp(entry_a->path, entry_b->path);
}

void traverse_directory(const char *path, FileList *fl) {
    int fd = open(path, O_RDONLY | O_NOATIME | O_DIRECTORY);
    if (fd < 0) {
        perror("Failed to open directory");
        return;
    }

    char buf[32768]; // Buffer for directory entries
    for (;;) {
        long nread = syscall(SYS_getdents64, fd, buf, sizeof(buf));
        if (nread == -1) {
            perror("Failed to read directory entries");
            break;
        }
        if (nread == 0) break; // End of directory

        for (long bpos = 0; bpos < nread; ) {
            struct linux_dirent64 *d = (struct linux_dirent64 *)(buf + bpos);
            bpos += d->d_reclen;

            // Skip "." and ".." entries
            if (strcmp(d->d_name, ".") == 0 || strcmp(d->d_name, "..") == 0) {
                continue;
            }

            char full_path[MAX_PATH_LENGTH];
            snprintf(full_path, sizeof(full_path), "%s/%s", path, d->d_name);

            unsigned char dtype = d->d_type;
            mode_t mode = 0;

            if (dtype == DT_UNKNOWN) {
                struct stat stbuf;
                if (lstat(full_path, &stbuf) == -1) {
                    perror("Failed to lstat");
                    continue;
                }
                mode = stbuf.st_mode;
            } else {
                switch (dtype) {
                    case DT_LNK: mode = S_IFLNK; break;
                    case DT_DIR: mode = S_IFDIR; break;
                    case DT_REG: mode = S_IFREG; break;
                    default: mode = 0; break;
                }
            }

            if (S_ISDIR(mode)) {
                traverse_directory(full_path, fl);
            } else if (S_ISREG(mode)) {
                file_list_add(fl, full_path);
            }
        }
    }

    close(fd);
}
