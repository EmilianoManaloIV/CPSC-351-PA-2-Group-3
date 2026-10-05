// sets up a unidirectional pipe() channel to pass the initialization timestamp from the child to the parent.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>

int main(int argc, char *argv[]) {
    // Step 1: Ensure a command has been provided
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command> [args...]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Step 2: Create pipe infrastructure before fork
    // fd[0] is read end, fd[1] is write end
    int fd[2];
    if (pipe(fd) == -1) {
        perror("pipe failed");
        exit(EXIT_FAILURE);
    }

    // Step 3: Call fork()
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    } 
    else if (pid == 0) {
        // Child Process
        struct timeval start_time;

        // Step 11: Close unused read descriptor in child
        close(fd[0]);

        // Step 4: Record starting time
        if (gettimeofday(&start_time, NULL) == -1) {
            perror("child gettimeofday failed");
            close(fd[1]);
            exit(EXIT_FAILURE);
        }

        // Step 5: Write the struct data structure to the pipe
        if (write(fd[1], &start_time, sizeof(struct timeval)) == -1) {
            perror("pipe write failed");
            close(fd[1]);
            exit(EXIT_FAILURE);
        }

        // Close write descriptor before moving to exec boundary
        close(fd[1]);

        // Step 6: Execute requested command
        execvp(argv[1], &argv[1]);

        // If execvp returns, an error occurred
        perror("execvp failed");
        exit(EXIT_FAILURE);
    } 
    else {
        // Parent Process
        int status;
        struct timeval start_time;
        struct timeval end_time;

        // Step 11: Close unused write descriptor in parent
        close(fd[1]);

        // Step 7: Wait for child process to finish execution
        if (wait(&status) == -1) {
            perror("wait failed");
        }

        // Step 8: Record ending time immediately after termination
        if (gettimeofday(&end_time, NULL) == -1) {
            perror("parent gettimeofday failed");
            close(fd[0]);
            exit(EXIT_FAILURE);
        }

        // Step 9: Retrieve starting timestamp from the pipe channel
        if (read(fd[0], &start_time, sizeof(struct timeval)) == -1) {
            perror("pipe read failed");
            close(fd[0]);
            exit(EXIT_FAILURE);
        }

        // Close reading descriptor
        close(fd[0]);

        // Step 10: Calculate and print exact elapsed timing frame
        double start_secs = start_time.tv_sec + (start_time.tv_usec / 1000000.0);
        double end_secs = end_time.tv_sec + (end_time.tv_usec / 1000000.0);
        double elapsed_time = end_secs - start_secs;

        printf("Elapsed time: %.6f seconds\n", elapsed_time);
    }

    return 0;
}
