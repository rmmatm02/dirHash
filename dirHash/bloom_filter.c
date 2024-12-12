#include "bloom_filter.h"
#include "constants.h" 
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

BloomFilter *bloom_filter_init(size_t size) {
    BloomFilter *filter = (BloomFilter *)malloc(sizeof(BloomFilter));
    if (!filter) {
        perror("Failed to allocate Bloom filter");
        exit(EXIT_FAILURE);
    }

    filter->bit_array = (uint8_t *)calloc(size, sizeof(uint8_t));
    if (!filter->bit_array) {
        perror("Failed to allocate Bloom filter bit array");
        free(filter);
        exit(EXIT_FAILURE);
    }

    filter->size = size;
    filter->seed2 = HASH_SEED + 1;
    return filter;
}

void bloom_filter_free(BloomFilter *filter) {
    if (filter) {
        free(filter->bit_array);
        free(filter);
    }
}

void bloom_filter_add(BloomFilter *filter, uint64_t hash) {
    uint64_t hash1 = hash;
    uint64_t hash2 = hash ^ filter->seed2;

    size_t index1 = (size_t)(hash1 % filter->size);
    size_t index2 = (size_t)(hash2 % filter->size);

    uint8_t bit1 = (uint8_t)(1 << (index1 % 8));
    uint8_t bit2 = (uint8_t)(1 << (index2 % 8));

    size_t byte1 = index1 / 8;
    size_t byte2 = index2 / 8;

    uint8_t *addr1 = &filter->bit_array[byte1];
    uint8_t *addr2 = &filter->bit_array[byte2];

    __asm__ __volatile__ (
        "lock; orb %b1, (%0)"
        :
        : "r" (addr1), "iq" (bit1)
        : "memory"
    );

    __asm__ __volatile__ (
        "lock; orb %b1, (%0)"
        :
        : "r" (addr2), "iq" (bit2)
        : "memory"
    );
}

int bloom_filter_check(BloomFilter *filter, uint64_t hash) {
    uint64_t hash1 = hash;
    uint64_t hash2 = hash ^ filter->seed2;

    size_t index1 = (size_t)(hash1 % filter->size);
    size_t index2 = (size_t)(hash2 % filter->size);

    uint8_t bit1 = (uint8_t)(1 << (index1 % 8));
    uint8_t bit2 = (uint8_t)(1 << (index2 % 8));

    size_t byte1 = index1 / 8;
    size_t byte2 = index2 / 8;

    uint8_t val1, val2;

    __asm__ __volatile__ (
        "movb (%1), %0"
        : "=r" (val1)
        : "r" (&filter->bit_array[byte1])
        : "memory"
    );

    __asm__ __volatile__ (
        "movb (%1), %0"
        : "=r" (val2)
        : "r" (&filter->bit_array[byte2])
        : "memory"
    );

    return ((val1 & bit1) && (val2 & bit2)) ? 1 : 0;
}

