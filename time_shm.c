// uses POSIX shared memory APIs (shm_open, mmap) to share a struct timeval containing the child process's starting timestamp.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>

#define SHM_NAME "/time_shm_memory"
#define SHM_SIZE sizeof(struct timeval)

int main(int argc, char *argv[]) {
    // Step 1: Ensure a command has been provided
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command> [args...]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Step 2: Set up shared memory region before fork
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed");
        exit(EXIT_FAILURE);
    }

    // Configure the size of the shared memory segment
    if (ftruncate(shm_fd, SHM_SIZE) == -1) {
        perror("ftruncate failed");
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }

    // Map the shared memory segment into the address space
    struct timeval *shared_time = (struct timeval *)mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_time == MAP_FAILED) {
        perror("mmap failed");
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }

    // Step 3: Call fork()
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        munmap(shared_time, SHM_SIZE);
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    } 
    else if (pid == 0) {
        // Child Process
        struct timeval start_time;
        
        // Step 4: Record starting time
        if (gettimeofday(&start_time, NULL) == -1) {
            perror("child gettimeofday failed");
            exit(EXIT_FAILURE);
        }

        // Step 5: Copy timestamp to shared memory region
        *shared_time = start_time;

        // Step 6: Execute requested command
        // &argv[1] passes the command name and trailing arguments up to the NULL terminator
        execvp(argv[1], &argv[1]);

        // If execvp returns, an error occurred
        perror("execvp failed");
        exit(EXIT_FAILURE);
    } 
    else {
        // Parent Process
        int status;
        struct timeval end_time;

        // Step 7: Wait for child process to finish
        if (wait(&status) == -1) {
            perror("wait failed");
        }

        // Step 8: Record ending time immediately after termination
        if (gettimeofday(&end_time, NULL) == -1) {
            perror("parent gettimeofday failed");
            munmap(shared_time, SHM_SIZE);
            shm_unlink(SHM_NAME);
            exit(EXIT_FAILURE);
        }

        // Step 9 & 10: Retrieve start time and calculate elapsed time
        double start_secs = shared_time->tv_sec + (shared_time->tv_usec / 1000000.0);
        double end_secs = end_time.tv_sec + (end_time.tv_usec / 1000000.0);
        double elapsed_time = end_secs - start_secs;

        printf("Elapsed time: %.6f seconds\n", elapsed_time);

        // Step 11: Clean up shared memory resources
        if (munmap(shared_time, SHM_SIZE) == -1) {
            perror("munmap failed");
        }
        if (shm_unlink(SHM_NAME) == -1) {
            perror("shm_unlink failed");
        }
    }

    return 0;
}
