/*
 * time_shm.c - CPSC 351 Programming Assignment 2 (Part A)
 *
 * Measures the elapsed time required to execute a command supplied on the
 * command line. The child records its starting timestamp with gettimeofday()
 * and passes it to the parent through a POSIX shared-memory region, then
 * replaces itself with the requested command via execvp(). The parent waits
 * for the child, records the ending timestamp, and prints the difference.
 *
 * Build:  gcc time_shm.c -o time -lrt
 * Usage:  ./time <command> [args...]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

/* Name of the shared-memory object. POSIX requires it to begin with a slash. */
#define SHM_NAME "/pa2_time_shm"

/* Size of the shared region: just enough to hold one struct timeval. */
#define SHM_SIZE sizeof(struct timeval)

int main(int argc, char *argv[])
{
    struct timeval *shared_start;   /* pointer into the shared-memory region */
    struct timeval end_time;        /* ending timestamp recorded by parent  */
    double elapsed;
    int shm_fd;
    pid_t pid;
    int status;

    /* Step 1: make sure a command was supplied. */
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command> [args...]\n", argv[0]);
        return EXIT_FAILURE;
    }

    /* Step 2: set up shared memory BEFORE fork() so both processes share it. */
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return EXIT_FAILURE;
    }

    if (ftruncate(shm_fd, SHM_SIZE) == -1) {
        perror("ftruncate");
        close(shm_fd);
        shm_unlink(SHM_NAME);
        return EXIT_FAILURE;
    }

    shared_start = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE,
                        MAP_SHARED, shm_fd, 0);
    if (shared_start == MAP_FAILED) {
        perror("mmap");
        close(shm_fd);
        shm_unlink(SHM_NAME);
        return EXIT_FAILURE;
    }

    /* The descriptor is no longer needed once the region is mapped. */
    close(shm_fd);

    /* Step 3: create the child process. */
    pid = fork();
    if (pid < 0) {
        perror("fork");
        munmap(shared_start, SHM_SIZE);
        shm_unlink(SHM_NAME);
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        /* ---------------- Child process ---------------- */

        /* Step 4: record the starting time directly into shared memory. */
        if (gettimeofday(shared_start, NULL) == -1) {
            perror("gettimeofday (child)");
            _exit(EXIT_FAILURE);
        }

        /* Step 5: the write above IS the IPC transfer; the parent reads the
         * same physical memory after the child terminates. */

        /* Step 6: replace this process image with the requested command.
         * argv[1] is the command, &argv[1] is its NULL-terminated argv. */
        execvp(argv[1], &argv[1]);

        /* execvp only returns on failure. */
        fprintf(stderr, "execvp failed for '%s': %s\n",
                argv[1], strerror(errno));
        _exit(EXIT_FAILURE);
    }

    /* ---------------- Parent process ---------------- */

    /* Step 7: wait for the child to terminate. */
    if (waitpid(pid, &status, 0) == -1) {
        perror("waitpid");
        munmap(shared_start, SHM_SIZE);
        shm_unlink(SHM_NAME);
        return EXIT_FAILURE;
    }

    /* Step 8: record the ending time. */
    if (gettimeofday(&end_time, NULL) == -1) {
        perror("gettimeofday (parent)");
        munmap(shared_start, SHM_SIZE);
        shm_unlink(SHM_NAME);
        return EXIT_FAILURE;
    }

    /* Step 9/10: read the starting time from shared memory and compute
     * elapsed = end - start, in seconds. */
    elapsed = (double)(end_time.tv_sec  - shared_start->tv_sec) +
              (double)(end_time.tv_usec - shared_start->tv_usec) / 1000000.0;

    printf("Elapsed time: %.6f seconds\n", elapsed);

    /* Step 11: release shared-memory resources. */
    if (munmap(shared_start, SHM_SIZE) == -1) {
        perror("munmap");
    }
    if (shm_unlink(SHM_NAME) == -1) {
        perror("shm_unlink");
    }

    return EXIT_SUCCESS;
}
