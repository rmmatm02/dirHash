#define _GNU_SOURCE

#include <stdint.h>
#include <xxhash.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h> // For fstat
#include <liburing.h> // For io_uring

// Define a hash seed for consistent hashing
#define HASH_SEED 0x12345678

// Hashes the contents of a file
uint64_t hash_file(const char *filepath) {
    // Open the file in read-only mode without altering metadata
    int fd = open(filepath, O_RDONLY | O_NOATIME);
    if (fd < 0) {
        fprintf(stderr, "Failed to open file '%s': %s\n", filepath, strerror(errno));
        return 0;
    }

    // Initialize hash with the file path hash
    uint64_t hash = XXH64(filepath, strlen(filepath), HASH_SEED);

    // Read the file in chunks and update the hash
    char buffer[4096]; // 4KB buffer
    ssize_t bytes_read;

    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
        hash = XXH64(buffer, bytes_read, hash);
    }

    // Handle read errors
    if (bytes_read < 0) {
        fprintf(stderr, "Failed to read file '%s': %s\n", filepath, strerror(errno));
    }

    // Close the file
    close(fd);

    return hash;
}

// Hashes the contents of a file using asynchronous I/O (io_uring)
uint64_t hash_file_contents_aio(const char *filepath) {
    struct io_uring ring;
    struct io_uring_sqe *sqe;
    struct io_uring_cqe *cqe;
    struct iovec iov;
    struct stat st;
    uint64_t hash = HASH_SEED;
    int fd;

    // Initialize io_uring
    if (io_uring_queue_init(64, &ring, 0) < 0) {
        perror("Failed to initialize io_uring");
        return 0;
    }

    // Open the file in read-only mode without altering metadata
    fd = open(filepath, O_RDONLY | O_NOATIME);
    if (fd < 0) {
        perror("Failed to open file");
        io_uring_queue_exit(&ring);
        return 0;
    }

    // Get file statistics
    if (fstat(fd, &st) < 0) {
        perror("Failed to stat file");
        close(fd);
        io_uring_queue_exit(&ring);
        return 0;
    }

    // Handle empty files
    if (st.st_size == 0) {
        close(fd);
        io_uring_queue_exit(&ring);
        return XXH64("", 0, HASH_SEED);
    }

    // Allocate memory for the file buffer
    iov.iov_base = malloc(st.st_size);
    if (!iov.iov_base) {
        perror("Failed to allocate buffer");
        close(fd);
        io_uring_queue_exit(&ring);
        return 0;
    }
    iov.iov_len = st.st_size;

    // Prepare the read request
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

    // Submit the read request
    if (io_uring_submit(&ring) < 0) {
        perror("Failed to submit io_uring request");
        free(iov.iov_base);
        close(fd);
        io_uring_queue_exit(&ring);
        return 0;
    }

    // Wait for the request to complete
    if (io_uring_wait_cqe(&ring, &cqe) < 0) {
        perror("Failed to wait for io_uring completion");
        free(iov.iov_base);
        close(fd);
        io_uring_queue_exit(&ring);
        return 0;
    }

    // Process the result
    if (cqe->res < 0) {
        fprintf(stderr, "Async read failed: %s\n", strerror(-cqe->res));
    } else {
        hash = XXH64(iov.iov_base, st.st_size, HASH_SEED);
    }

    io_uring_cqe_seen(&ring, cqe);

    // Cleanup
    free(iov.iov_base);
    close(fd);
    io_uring_queue_exit(&ring);

    return hash;
}

