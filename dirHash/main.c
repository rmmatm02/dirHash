#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <omp.h>
#include "bloom_filter.h"
#include "file_list.h"
#include "directory_traversal.h"
#include "progress.h"
#include "hashing.h"
#include "constants.h"

int compare_filepaths(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <directory>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *directory = argv[1];
    long num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    if (num_cores < 1) {
        num_cores = 4;
    }
    size_t NUM_THREADS = (size_t)num_cores;
    printf("Number of threads: %zu\n", NUM_THREADS);

    BloomFilter *filter = bloom_filter_init(BLOOM_FILTER_SIZE);
    FileList *fl = file_list_init(INITIAL_FILE_LIST_CAPACITY);

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    uint64_t final_hash = HASH_SEED; // Initialize hash
    traverse_directory(directory, fl, &final_hash); // Updated signature

    qsort(fl->filepaths, fl->size, sizeof(char *), compare_filepaths);

    struct timespec traversal_end;
    clock_gettime(CLOCK_MONOTONIC, &traversal_end);
    double traversal_time = (traversal_end.tv_sec - start.tv_sec) +
                             (double)(traversal_end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Directory traversal completed in %.6f seconds.\n", traversal_time);

    struct timespec loop_start;
    clock_gettime(CLOCK_MONOTONIC, &loop_start);
    double last_progress_update = 0.0;

    #pragma omp parallel for num_threads(NUM_THREADS) schedule(guided) reduction(^:final_hash)
    for (size_t i = 0; i < fl->size; ++i) {
        uint64_t hash = hash_file_contents_aio(fl->filepaths[i]);
        if (__builtin_expect(hash == 0, 0)) {
            continue;
        }
        if (!bloom_filter_check(filter, hash)) {
            bloom_filter_add(filter, hash);
            final_hash = (final_hash * PRIME_MULTIPLIER) ^ hash;
        }

        if (omp_get_thread_num() == 0) {
            struct timespec now;
            clock_gettime(CLOCK_MONOTONIC, &now);
            double elapsed_time = (now.tv_sec - loop_start.tv_sec) + (double)(now.tv_nsec - loop_start.tv_nsec) / 1e9;

            if (elapsed_time - last_progress_update >= 0.1 || i == fl->size - 1) {
                display_progress(i + 1, fl->size, elapsed_time, 0);
                last_progress_update = elapsed_time;
            }
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double total_time = (end.tv_sec - start.tv_sec) + (double)(end.tv_nsec - start.tv_nsec) / 1e9;

    int hours, minutes, seconds, milliseconds;
    format_time(total_time, &hours, &minutes, &seconds, &milliseconds);

    printf("\nFinal directory hash: %lx\n", final_hash);
    printf("Total time taken: %02d:%02d:%02d.%03d\n", hours, minutes, seconds, milliseconds);

    file_list_free(fl);
    bloom_filter_free(filter);

    return EXIT_SUCCESS;
}

