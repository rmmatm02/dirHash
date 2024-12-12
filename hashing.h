#ifndef HASHING_H
#define HASHING_H

#include <stdint.h>

#define QUEUE_DEPTH 64

uint64_t hash_file_contents_aio(const char *filepath);

#endif

