# Operating Systems · Lab 8: The Same Calls Inside Your Own OS (xv6)
**Course:** Operating Systems (OS 2026-2027)  
**Institution:** FCIM / FAF · Technical University of Moldova (UTM)  
**Student:** Dragos Nedelea (st. gr. FAF-241)  
**Checked by:** Patricia Reițman  
**Track:** Track B: Build your own small OS (xv6)  
**Chișinău 2026**

---

## Purpose of the Work (Scopul lucrării)

The objective of this laboratory is to replicate and verify the classic UNIX process and IPC system calls inside our own teaching operating system (**xv6-riscv**). Specifically, the goals are:
1. Implementing process execution using the `fork()`, `exec()`, and `wait()` syscall interface in `user/run.c` and comparing the mechanics with POSIX Linux from Lab 7.
2. Developing bidirectional inter-process communication using pairs of unidirectional pipes in `user/pingpong.c` and analyzing pipe blocking semantics on `read()`.
3. Constructing a concurrent, pipeline-based prime number sieve (Sieve of Eratosthenes) in `user/primes.c` using dynamic process generation and pipe cascading.

---

## Component 1. Process Lifecycle Mechanics inside xv6: `user/run.c`

### Execution Output:

```
xv6$ run
parent: active pid is 3
child: my pid is 4
hello from the child
parent: child 4 ended with status 0
```
**Figure 8.1 – Execution of user/run.c inside xv6 demonstrating process spawning, exec, and parent reaping.**

### Observe Questions & Answers

1. **Cross-Platform System Call Equivalence: Itemize the tripartite sequence of system calls executed by `run.c` inside xv6 versus its Linux counterpart in Lab 7. What fundamental design elements are unified across both environments?**  
   - **xv6 calls:** `fork()`, `exec()`, `wait()`.  
   - **Linux calls:** `fork()`, `execlp()` (or `execve()`), `waitpid()`.  
   - **What is the same:** The conceptual design and sequence are 100% identical! In both operating systems:
     1. `fork()` duplicates the calling process address space to create a child.
     2. `exec()` replaces the memory image of the child with a newly loaded executable program.
     3. `wait()` blocks the parent process until the child terminates, reaping its exit code.  
     The Unix system call semantics designed in the 1970s remain consistent between modern production Linux and educational xv6.

2. **Instruction Stream Execution Source: Exactly which binary image generates the terminal string `"hello from the child"`: the initial instructions of `run`, or the subsequent `echo` binary?**  
   It is printed by **`echo`**! Once `exec("echo", args)` succeeds, the execution image of `run` in the child process is replaced by the `echo` binary. The `echo` utility executes `printf` to display its arguments and terminates via `exit(0)`.

3. **Exit Status Code Divergence: Account for the difference between status code 1 in xv6 versus status 127 on Linux upon binary execution failure.**  
   When `exec` fails:
   - In xv6 (`user/run.c`), the line immediately following `exec` is:
     ```c
     fprintf(2, "run: execve substitution failed\n");
     exit(1);
     ```
     So the child explicitly exits with status `1`.
   - In Linux (`run.c` in Lab 7), the line was:
     ```c
     perror("exec invocation failed");
     exit(127);
     ```
     So the child exited with status `127` (the standard POSIX shell exit code for command not found). The parent simply reads whatever value the child passed into `exit()`.

---

## Component 2. Bidirectional Pipe IPC: `user/pingpong.c`

### Execution Output:

```
xv6$ pingpong
4: received ping
3: received pong
```
**Figure 8.2 – Inter-process communication between parent and child processes using pipes inside xv6.**

### Observe Questions & Answers

1. **Half-Duplex Channel Separation: What structural limitation of POSIX pipes necessitates allocating two distinct pipe descriptors for bidirectional communication?**  
   A standard Unix pipe is strictly **unidirectional** (one read end and one write end). If a single pipe were shared for bidirectional communication, both parent and child would read and write to the same buffer. A race condition would occur where the writer could immediately read back its own byte before the peer had a chance to read it, or both processes would deadlock waiting for each other. Two pipes (`fds_downstream` and `fds_upstream`) ensure independent, dedicated half-duplex communication channels.

2. **Implicit Synchronization via Blocking I/O: No explicit synchronization primitive (e.g. `sleep` or mutex) was programmed to delay the parent. Which specific system call induces parent blocking until the child dispatches its response?**  
   The call **`read(fds_upstream[0], &token_byte, sizeof(char))`** makes the parent wait! Reading from an empty pipe in Unix is a **blocking system call**. The kernel puts the parent process into sleep state (`SLEEPING`) and deschedules it until data is written to the pipe by the child (or until all write descriptors to the pipe are closed).

---

## Component 3. Dynamic Process Pipeline: Concurrent Prime Sieve (`user/primes.c`)

Implemented the concurrent Sieve of Eratosthenes where each stage reads prime $p$, spawns a child process with a pipe, and filters multiples of $p$ for numbers up to 35:

```
xv6$ primes
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
**Figure 8.3 – Output of concurrent Sieve of Eratosthenes inside xv6.**

All unused file descriptors are carefully closed at every pipeline stage to avoid file descriptor table exhaustion.

---

## Conclusion (Concluzii proprii)

In this laboratory work, I completed the full circle of systems programming by implementing core process and IPC primitives inside xv6:
1. **Universality of Unix Primitives:** Direct comparison between Linux (Lab 7) and xv6 (Lab 8) confirmed that the fundamental abstractions (`fork`, `exec`, `wait`, `pipe`) transcend specific kernels and hardware architectures (x86_64 vs RISC-V).
2. **Pipe Concurrency & Synchronization:** Implementing `pingpong.c` and `primes.c` deepened my understanding of blocking I/O synchronization and descriptor lifecycle management across cascaded child processes.
3. **Systems Mindset:** Writing code that runs directly on bare metal within a teaching kernel demystified how operating systems transition between user and supervisor mode, manage process address spaces, and orchestrate cooperative multi-processing.
