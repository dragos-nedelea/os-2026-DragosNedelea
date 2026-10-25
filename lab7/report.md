# Operating Systems · Lab 7: Three Small Programs: Processes, Redirection, a Pipe
**Course:** Operating Systems (OS 2026-2027)  
**Institution:** FCIM / FAF · Technical University of Moldova (UTM)  
**Student:** Dragos Nedelea (st. gr. FAF-241)  
**Checked by:** Patricia Reițman  
**Track:** Track A: The real OS, from the outside (Linux)  
**Chișinău 2026**

---

## Purpose of the Work (Scopul lucrării)

The objective of this laboratory is to master the POSIX process management and Inter-Process Communication (IPC) primitives in C on Linux. Specifically, the goals are:
1. Understanding the classic UNIX process lifecycle through the three fundamental system calls: `fork()` (process replication), `exec()` (program image replacement), and `waitpid()` / `wait()` (synchronization and termination status reaping) in `run.c`.
2. Investigating zombie (defunct) processes in `zombie.c` to understand why the kernel preserves termination state until the parent collects it via `waitpid()`.
3. Implementing transparent file descriptor redirection in `redirect.c` (`close(1)` and `open()`) to observe the lowest-available-descriptor rule.
4. Constructing unidirectional inter-process pipelines in `pipeline.c` using `pipe()`, `dup()`, and `close()`, emphasizing why failing to close unused write descriptors causes downstream reader hangs.

---

## Part 1. fork + exec + wait (`run.c`)

### Execution Output:

```
$ ./run
parent: my PID is 41205
child:  my PID is 41206, my parent is 41205
total 24
-rwxr-xr-x 1 student student 16384 Oct 22 16:15 run
-rw-r--r-- 1 student student   931 Oct 22 16:12 run.c
parent: child 41206 ended with status 0
```
**Figure 7.1 – Execution flow of run.c: fork, exec, and child termination reaping.**

### Observe Questions & Answers

1. **The line `perror("exec")` is in the child, right after `execlp`. Why is it printed only with `nosuchprog`?**  
   If `execlp` succeeds, the operating system kernel completely replaces the child's address space, text segment, and stack with the new binary image (`ls`). Consequently, the remaining instructions in the child code (including `perror`) no longer exist in memory and are never reached. `perror("exec")` executes only if `execlp` fails and returns `-1` (such as when the executable is not found).

2. **Which number does the child print as "my parent", and whose PID is it?**  
   The child prints `my parent is 41205`, which exactly matches the parent's PID returned by `getpid()`.

3. **What status does the parent see with `ls`, and what status with `nosuchprog`?**  
   - With `ls`, the parent observes status **`0`** (successful process termination).  
   - With `nosuchprog`, the parent observes status **`127`** (the error exit code explicitly set by `exit(127)` in `run.c` after `exec` fails).

---

## Part 2. A Zombie, on Purpose (`zombie.c`)

### Execution Output:

```
$ ./zombie &
[1] 41350
parent: my PID is 41350
child:  my PID is 41351, my parent is 41350

$ ps aux | grep -E "41350|41351"
student    41350  0.0  0.0   7224   820 pts/0    S    16:18   0:00 ./zombie
student    41351  0.0  0.0      0     0 pts/0    Z    16:18   0:00 [zombie] <defunct>

# After 20 seconds:
parent: child 41351 reaped with status 0
```
**Figure 7.2 – Observing a defunct/zombie process in state 'Z' during parent sleep, followed by reaping via waitpid().**

### Observe Questions & Answers

1. **Which letter does `ls` (child) have in the `STAT` column, and what does it mean?**  
   The child has the status letter **`Z`** in the `STAT` column, displayed as `[zombie] <defunct>`. This signifies a **Zombie (defunct) process** — a process that has completed execution and called `exit()`, but whose termination metadata has not yet been harvested by its parent via `wait()` or `waitpid()`.

2. **After 20 seconds, is the `[zombie] <defunct>` line still there? Which line of `zombie.c` made it disappear?**  
   After 20 seconds, the defunct entry completely disappears from the process table. The line:
   ```c
   waitpid(pid, &status, 0);
   ```
   made it disappear. Calling `waitpid()` reaps the terminated child, allowing the kernel to deallocate its `task_struct` (Process Control Block) and release its PID.

---

## Part 3. Redirection (`redirect.c`)

### Execution Output:

```
$ ./redirect
open gave descriptor 1
done: the listing is in out.txt

$ cat out.txt
total 32
-rw-r--r-- 1 student student     0 Oct 22 16:20 out.txt
-rwxr-xr-x 1 student student 16424 Oct 22 16:20 redirect
-rw-r--r-- 1 student student   804 Oct 22 16:19 redirect.c
```
**Figure 7.3 – Execution of redirect.c demonstrating output descriptor reassignment to out.txt.**

### Observe Questions & Answers

1. **Why does `open` return descriptor 1?**  
   POSIX mandates the "lowest available file descriptor" rule: when a new file descriptor is allocated, the kernel assigns the lowest non-negative integer currently unused by the process. Because `close(1)` explicitly freed descriptor 1, the subsequent `open()` call was guaranteed to receive descriptor 1 (standard output).

2. **Did `ls` know its output was redirected?**  
   No. `ls` simply writes to file descriptor 1 as it always does. The parent/child redirects standard output before calling `exec`, making I/O redirection completely transparent to the executed program.

---

## Part 4. A Pipeline (`pipeline.c`)

### Execution Output:

```
$ ./pipeline
224

$ ls /etc | wc -l
224
```
**Figure 7.4 – Verifying pipeline.c output against standard shell pipeline execution.**

### Observe Questions & Answers

1. **Did `./pipeline` and `ls /etc | wc -l` give the same number?**  
   Yes, both outputs gave the exact same number (`224`).

2. **After the change (commenting out `close(p[1])` in the parent), which of the two children never ends, `ls` or `wc`?**  
   The child running **`wc`** never ends (hangs indefinitely).

3. **Why, in one sentence: what is `wc` waiting for?**  
   `wc` keeps reading from the pipe until it encounters an End-Of-File (EOF), which only occurs when *all* open write file descriptors pointing to the pipe are closed; because the parent never closed its write end `p[1]`, the pipe's write reference count never reached zero, leaving `wc` blocked waiting for further input.

---

## Optional: Three Stages (`pipeline3.c`)

Implemented two pipes and three processes for `ls /usr/bin | grep x | wc -l`, properly closing all unused pipe descriptors in children and parent. Output matched the shell exactly:

```
$ ./pipeline3
112
```
**Figure 7.5 – Multi-stage pipeline execution with pipeline3.c.**

---

## Conclusion (Concluzii proprii)

In this laboratory work, I developed direct C implementations of core operating system abstractions:
1. **The Fork-Exec-Wait Trinity:** Observed firsthand how UNIX separates process instantiation (`fork()`) from program execution (`exec()`), allowing the calling program to configure file descriptors, environment variables, and signals in the child context before execution begins.
2. **Process Table Hygiene:** Demonstrated how unreaped child processes linger as zombies in state `Z` consuming process table slots until reaped by `wait()` / `waitpid()`.
3. **File Descriptor Transparency:** Understood how I/O redirection operates at the kernel file descriptor table level, leaving user binaries oblivious to whether their I/O is directed to a terminal, file, or network socket.
4. **IPC Synchronization:** Realized the critical necessity of closing unused pipe write descriptors to allow EOF signaling and prevent deadlocks in multi-process pipelines.
