/*
 * time_pipe.c - CPSC 351 Programming Assignment 2 (Part B)
 *
 * Measures the elapsed time required to execute a command supplied on the
 * command line. The child records its starting timestamp with gettimeofday()
 * and writes it to the parent through an anonymous pipe, then replaces itself
 * with the requested command via execvp(). The parent waits for the child,
 * records the ending timestamp, reads the start time from the pipe, and
 * prints the difference.
 *
 * Build:  gcc time_pipe.c -o time
 * Usage:  ./time <command> [args...]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#define READ_END  0
#define WRITE_END 1

int main(int argc, char *argv[])
{
    int pipe_fd[2];                 /* pipe_fd[0] = read end, [1] = write end */
    struct timeval start_time;      /* starting timestamp (child -> parent)   */
    struct timeval end_time;        /* ending timestamp recorded by parent    */
    double elapsed;
    ssize_t nbytes;
    pid_t pid;
    int status;

    /* Step 1: make sure a command was supplied. */
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command> [args...]\n", argv[0]);
        return EXIT_FAILURE;
    }

    /* Step 2: create the pipe BEFORE fork() so the child inherits both ends. */
    if (pipe(pipe_fd) == -1) {
        perror("pipe");
        return EXIT_FAILURE;
    }

    /* Step 3: create the child process. */
    pid = fork();
    if (pid < 0) {
        perror("fork");
        close(pipe_fd[READ_END]);
        close(pipe_fd[WRITE_END]);
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        /* ---------------- Child process ---------------- */

        /* The child only writes, so close the unused read end. */
        close(pipe_fd[READ_END]);

        /* Step 4: record the starting time. */
        if (gettimeofday(&start_time, NULL) == -1) {
            perror("gettimeofday (child)");
            close(pipe_fd[WRITE_END]);
            _exit(EXIT_FAILURE);
        }

        /* Step 5: send the starting timestamp to the parent via the pipe.
         * A struct timeval is far smaller than PIPE_BUF, so the write is
         * atomic and will not block. */
        nbytes = write(pipe_fd[WRITE_END], &start_time, sizeof(start_time));
        if (nbytes != (ssize_t)sizeof(start_time)) {
            perror("write to pipe");
            close(pipe_fd[WRITE_END]);
            _exit(EXIT_FAILURE);
        }

        /* Done with the pipe; close it so the command we exec does not
         * inherit a stray descriptor. */
        close(pipe_fd[WRITE_END]);

        /* Step 6: replace this process image with the requested command.
         * argv[1] is the command, &argv[1] is its NULL-terminated argv. */
        execvp(argv[1], &argv[1]);

        /* execvp only returns on failure. */
        fprintf(stderr, "execvp failed for '%s': %s\n",
                argv[1], strerror(errno));
        _exit(EXIT_FAILURE);
    }

    /* ---------------- Parent process ---------------- */

    /* The parent only reads, so close the unused write end. */
    close(pipe_fd[WRITE_END]);

    /* Step 7: wait for the child to terminate. The timestamp stays buffered
     * in the pipe until we read it. */
    if (waitpid(pid, &status, 0) == -1) {
        perror("waitpid");
        close(pipe_fd[READ_END]);
        return EXIT_FAILURE;
    }

    /* Step 8: record the ending time. */
    if (gettimeofday(&end_time, NULL) == -1) {
        perror("gettimeofday (parent)");
        close(pipe_fd[READ_END]);
        return EXIT_FAILURE;
    }

    /* Step 9: read the starting time from the pipe. */
    nbytes = read(pipe_fd[READ_END], &start_time, sizeof(start_time));
    if (nbytes != (ssize_t)sizeof(start_time)) {
        if (nbytes == -1) {
            perror("read from pipe");
        } else {
            fprintf(stderr, "read from pipe: expected %zu bytes, got %zd\n",
                    sizeof(start_time), nbytes);
        }
        close(pipe_fd[READ_END]);
        return EXIT_FAILURE;
    }

    /* Step 10: compute elapsed = end - start, in seconds. */
    elapsed = (double)(end_time.tv_sec  - start_time.tv_sec) +
              (double)(end_time.tv_usec - start_time.tv_usec) / 1000000.0;

    printf("Elapsed time: %.6f seconds\n", elapsed);

    /* Step 11: close the remaining pipe descriptor. */
    close(pipe_fd[READ_END]);

    return EXIT_SUCCESS;
}
