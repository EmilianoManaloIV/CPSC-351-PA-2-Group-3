/*
 * time_shm.c - Measure the elapsed time of a command using shared memory IPC.
 *
 * Usage:   ./time <command> [args...]
 * Compile: gcc time_shm.c -o time -lrt
 *
 * The child records the start time, stores it in a POSIX shared-memory
 * region (created before fork), then execs the command. The parent waits,
 * records the end time, reads the start time from shared memory, and prints
 * the elapsed time.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
    char shm_name[64];
    int shm_fd;
    struct timeval *start_shared;   /* points into shared memory */
    struct timeval end_time, start_time;
    pid_t pid;
    int status;

    /* Step 1: a command must be supplied */
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command> [args...]\n", argv[0]);
        return EXIT_FAILURE;
    }

    /* Step 2: create the shared-memory object BEFORE fork() */
    snprintf(shm_name, sizeof(shm_name), "/time_shm_%d", (int)getpid());
    shm_fd = shm_open(shm_name, O_CREAT | O_EXCL | O_RDWR, 0600);
    if (shm_fd == -1) {
        perror("shm_open");
        return EXIT_FAILURE;
    }
    if (ftruncate(shm_fd, sizeof(struct timeval)) == -1) {
        perror("ftruncate");
        close(shm_fd);
        shm_unlink(shm_name);
        return EXIT_FAILURE;
    }
    start_shared = mmap(NULL, sizeof(struct timeval), PROT_READ | PROT_WRITE,
                        MAP_SHARED, shm_fd, 0);
    if (start_shared == MAP_FAILED) {
        perror("mmap");
        close(shm_fd);
        shm_unlink(shm_name);
        return EXIT_FAILURE;
    }
    close(shm_fd);   /* mapping remains valid after the descriptor is closed */

    /* Step 3: create the child */
    pid = fork();
    if (pid < 0) {
        perror("fork");
        munmap(start_shared, sizeof(struct timeval));
        shm_unlink(shm_name);
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        /* ---- Child ---- */
        /* Step 4: record the starting time (shared memory is written directly) */
        if (gettimeofday(start_shared, NULL) == -1) {
            perror("gettimeofday (child)");
            _exit(EXIT_FAILURE);
        }

        /* Step 6: execute the requested command */
        execvp(argv[1], &argv[1]);

        /* Only reached if execvp failed */
        fprintf(stderr, "execvp failed for '%s': %s\n", argv[1], strerror(errno));
        _exit(EXIT_FAILURE);
    }

    /* ---- Parent ---- */
    /* Step 7: wait for the child to terminate */
    if (waitpid(pid, &status, 0) == -1) {
        perror("waitpid");
        munmap(start_shared, sizeof(struct timeval));
        shm_unlink(shm_name);
        return EXIT_FAILURE;
    }

    /* Step 8: record the ending time */
    if (gettimeofday(&end_time, NULL) == -1) {
        perror("gettimeofday (parent)");
        munmap(start_shared, sizeof(struct timeval));
        shm_unlink(shm_name);
        return EXIT_FAILURE;
    }

    /* Step 9: read the starting time from shared memory */
    start_time = *start_shared;

    /* Step 10: compute and print elapsed time (end - start) */
    double elapsed = (double)(end_time.tv_sec - start_time.tv_sec) +
                     (double)(end_time.tv_usec - start_time.tv_usec) / 1000000.0;
    printf("Elapsed time: %.6f seconds\n", elapsed);

    /* Step 11: release the shared-memory resources */
    if (munmap(start_shared, sizeof(struct timeval)) == -1)
        perror("munmap");
    if (shm_unlink(shm_name) == -1)
        perror("shm_unlink");

    return EXIT_SUCCESS;
}
