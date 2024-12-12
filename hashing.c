#define _GNU_SOURCE

#include "hashing.h"
#include "constants.h" 
#include "xxhash.h"
#include <linux/io_uring.h>
#include <liburing.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

uint64_t hash_file_contents_aio(const char *filepath) {
    struct io_uring ring;
    struct io_uring_sqe *sqe;
    struct io_uring_cqe *cqe;
    struct iovec iov;
    struct stat st;
    uint64_t hash = 0;
    int fd;

    if (io_uring_queue_init(QUEUE_DEPTH, &ring, 0) < 0) {
        perror("Failed to initialize io_uring");
        return 0;
    }

    fd = open(filepath, O_RDONLY | O_NOATIME);
    if (fd < 0) {
        perror("Failed to open file");
        io_uring_queue_exit(&ring);
        return 0;
    }

    if (fstat(fd, &st) < 0) {
        perror("Failed to stat file");
        close(fd);
        io_uring_queue_exit(&ring);
        return 0;
    }

    if (st.st_size == 0) {
        close(fd);
        io_uring_queue_exit(&ring);
        return XXH64("", 0, HASH_SEED);
    }

    iov.iov_base = malloc(st.st_size);
    if (!iov.iov_base) {
        perror("Failed to allocate buffer");
        close(fd);
        io_uring_queue_exit(&ring);
        return 0;
    }
    iov.iov_len = st.st_size;

    sqe = io_uring_get_sqe(&ring);
    if (!sqe) {
        perror("Failed to get submission queue entry");
        free(iov.iov_base);
        close(fd);
        io_uring_queue_exit(&ring);
        return 0;
    }

    io_uring_prep_readv(sqe, fd, &iov, 1, 0);
    io_uring_sqe_set_data(sqe, iov.iov_base);

    if (io_uring_submit(&ring) < 0) {
        perror("Failed to submit request");
        free(iov.iov_base);
        close(fd);
        io_uring_queue_exit(&ring);
        return 0;
    }

    if (io_uring_wait_cqe(&ring, &cqe) < 0) {
        perror("Failed to wait for completion");
        free(iov.iov_base);
        close(fd);
        io_uring_queue_exit(&ring);
        return 0;
    }

    if (cqe->res < 0) {
        fprintf(stderr, "Async read failed: %s\n", strerror(-cqe->res));
    } else {
        hash = XXH64(iov.iov_base, st.st_size, HASH_SEED);
    }

    io_uring_cqe_seen(&ring, cqe);
    free(iov.iov_base);
    close(fd);
    io_uring_queue_exit(&ring);

    return hash;
}

