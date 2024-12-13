#ifndef BLOOM_FILTER_H
#define BLOOM_FILTER_H

#include <stdint.h>
#include <stdlib.h>

#define BLOOM_FILTER_SIZE (32ULL * 1024ULL * 1024ULL)
#define HASH_SEED 5381

typedef struct {
    uint8_t *bit_array;
    size_t size;
    uint64_t seed2;
} BloomFilter;

BloomFilter *bloom_filter_init(size_t size);
void bloom_filter_free(BloomFilter *filter);
void bloom_filter_add(BloomFilter *filter, uint64_t hash);
int bloom_filter_check(BloomFilter *filter, uint64_t hash);

#endif

