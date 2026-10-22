# Operating Systems · Lab 8: The Same Calls Inside Your Own OS (xv6)
**Course:** Operating Systems (OS 2026-2027)  
**Institution:** FCIM / FAF · Technical University of Moldova (UTM)  
**Student:** Dragos Nedelea  
**Track:** Track B: Build your own small OS (xv6)

---

## Part 1. `run`, Inside xv6 (`user/run.c`)

### Observe Questions & Answers

1. **Write the three calls that `run.c` uses in xv6 and the three calls `run.c` used on Linux in Lab 7. What is the same?**  
   - **xv6 calls:** `fork()`, `exec()`, `wait()`.  
   - **Linux calls:** `fork()`, `execlp()` (or `execve()`), `waitpid()`.  
   - **What is the same:** The conceptual design and sequence are 100% identical! In both operating systems:
     1. `fork()` duplicates the calling process to create a child.
     2. `exec()` replaces the memory image of the child with a newly loaded executable program.
     3. `wait()` blocks the parent process until the child terminates, reaping its exit code.  
     Unix system call semantics designed in the 1970s remain consistent between modern Linux and educational xv6.

2. **Who prints "hello from the child": the code of `run`, or `echo`?**  
   It is printed by **`echo`**! Once `exec("echo", args)` succeeds, the execution image of `run` in the child process is replaced by the `echo` binary. The `echo` utility executes `printf` to display its arguments and terminates via `exit(0)`.

3. **Why is the status 1 here and 127 on Linux? (Look at the exit line after `exec` in both programs.)**  
   When `exec` fails:
   - In xv6 (`user/run.c`), the line immediately following `exec` is:
     ```c
     printf("exec failed\n");
     exit(1);
     ```
     So the child explicitly exits with status `1`.
   - In Linux (`run.c` in Lab 7), the line was:
     ```c
     perror("exec");
     exit(127);
     ```
     So the child exited with status `127` (the standard POSIX shell exit code for command not found). The parent simply reads whatever value the child passed into `exit()`.

---

## Part 2. `pingpong` (`user/pingpong.c`)

### Observe Questions & Answers

1. **Why do you need TWO pipes, and not one for both directions?**  
   A standard Unix pipe is strictly **unidirectional** (one read end and one write end). If a single pipe were shared for bidirectional communication, both parent and child would read and write to the same buffer. A race condition would occur where the writer could immediately read back its own byte before the peer had a chance to read it, or both processes would deadlock waiting for each other. Two pipes (`p2c` and `c2p`) ensure independent, dedicated half-duplex communication channels.

2. **Nothing in your code says "wait until the child has replied". Which call makes the parent wait?**  
   The call **`read(c2p[0], &b, 1)`** makes the parent wait! Reading from an empty pipe in Unix is a **blocking system call**. The kernel puts the parent process into sleep state (`SLEEPING`) and deschedules it until data is written to the pipe by the child (or until all write descriptors to the pipe are closed).

---

## Optional: Concurrent Primes Sieve (`user/primes.c`)
Implemented the concurrent Sieve of Eratosthenes where each stage reads prime $p$, spawns a child process with a pipe, and filters multiples of $p$ for numbers up to 35. Output correctly prints:
```
prime 2
prime 3
prime 5
prime 7
prime 11
prime 13
prime 17
prime 19
prime 23
prime 29
prime 31
```
All unused file descriptors are closed to prevent file descriptor exhaustion.
