# Assignment 2: Processes, Timing, and Interprocess Communication (IPC)

## Overview
This project measures the precise elapsed time required to execute any system command supplied via the command-line interface. To observe and compare different Operating System primitives, the exact same core timing logic is implemented twice using two distinct Interprocess Communication (IPC) mechanisms:
1. **Shared Memory (`time_shm.c`)**: Uses POSIX shared memory (`shm_open`, `ftruncate`, and `mmap`) to create a shared region where the child process stores its starting timestamp before running the command.
2. **Pipes (`time_pipe.c`)**: Uses a unidirectional pipe channel (`pipe`) where the child writes its starting timestamp directly into the pipe data stream.

In both implementations, a child process is spawned via `fork()`, records the time right before using `execvp()` to run the targeted command, and the parent calculates the microsecond-accurate delta after the child process successfully terminates via `wait()`.

---

## Learning Objectives
- Creating and managing lifecycle stages of child processes using `fork()`.
- Overriding address spaces using the `execvp()` system call.
- Accessing structural time primitives using `gettimeofday()` and `struct timeval`.
- Implementing and synchronizing IPC using POSIX shared memory segments.
- Implementing and cleaning up IPC channels via standard pipes.

---

## Files Included
- `time_shm.c` — Source code for the Shared Memory IPC implementation.
- `time_pipe.c` — Source code for the Pipe IPC implementation.
- `time_shm_output.txt` — Captured terminal log showing successful compilation and verification tests for the shared memory program.
- `time_pipe_output.txt` — Captured terminal log showing successful compilation and verification tests for the pipe program.

---

## Requirements & Environment
- **Language:** C (C99 standard or higher recommended)
- **Operating System:** Linux / Unix / macOS
- **Compiler:** `gcc` or any standard POSIX C compiler

---

## Compilation

Both files compile independently using standard flags. The shared memory version requires the Real-Time extensions library (`-lrt`) on standard Linux environments.

```bash
# Compile the Shared Memory implementation
gcc time_shm.c -o time_shm -lrt

# Compile the Pipe implementation
gcc time_pipe.c -o time_pipe
```

---

## Usage

Both binaries adhere to the standard POSIX command-line format where `<command>` is any valid system command, followed by optional parameters:

```bash
./<binary_name> <command> [args...]
```

### Examples:

```bash
# Test a basic command
./time_shm ls

# Test a command requiring argument parsing
./time_pipe ls -l

# Test current working directory
./time_shm pwd

# Test a time-delayed payload (should return ~1.00 seconds)
./time_pipe sleep 1
```

---

## Verification and Output Capture

To generate the required compliance logs (`.txt` files) alongside visual validation on your machine, use the standard system pipeline tool `tee`:

```bash
# Capture execution outputs to compliance files
./time_shm ls -l | tee time_shm_output.txt
./time_pipe ls -l | tee time_pipe_output.txt
```

---

## Expected Output Format
The program prints standard command outputs directly onto the console, trailing with a microsecond-accurate summary string containing exactly six fractional digits:

```text
total 16
-rw-r--r-- 1 user group 3120 Oct  5 08:35 time_pipe.c
-rw-r--r-- 1 user group 4251 Oct  5 08:35 time_shm.c
Elapsed time: 0.002481 seconds
```
*(Note: Small variances in timing values are expected between runs due to OS scheduling shifts and active kernel load constraints).*
