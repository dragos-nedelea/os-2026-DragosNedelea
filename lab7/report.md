# Operating Systems · Lab 7: Three Small Programs: Processes, Redirection, a Pipe
**Course:** Operating Systems (OS 2026-2027)  
**Institution:** FCIM / FAF · Technical University of Moldova (UTM)  
**Student:** Dragos Nedelea  
**Track:** Track A: The real OS, from the outside (Linux)

---

## Part 1. fork + exec + wait (`run.c`)

### Observe Questions & Answers

1. **The line `perror("exec")` is in the child, right after `execlp`. Why is it printed only with `nosuchprog`?**  
   If `execlp` succeeds, the operating system kernel completely replaces the child's virtual address space, text, and stack with the new binary image (`ls` or `date`). Consequently, the remaining instructions in the child code (including `perror`) no longer exist in memory and are never reached. `perror("exec")` executes only if `execlp` fails and returns `-1` (such as when the executable is not found).

2. **Which number does the child print as "my parent", and whose PID is it?**  
   The child prints `my parent is <PID>`, which matches the PID printed by the parent process (`getpid()`).

3. **What status does the parent see with `ls`, and what status with `nosuchprog`?**  
   - With `ls`, the parent observes status **`0`** (successful termination).  
   - With `nosuchprog`, the parent observes status **`127`** (the error exit code set by `exit(127)` in `run.c` after `exec` fails).

---

## Part 2. A Zombie, on Purpose (`zombie.c`)

### Observe Questions & Answers

1. **Which letter does `ls` have in the `STAT` column, and what does it mean?**  
   `ls` has the status letter **`Z`** (or `Z+`) in the `STAT` column, displayed as `[ls] <defunct>`. This signifies a **Zombie (defunct) process** — a process that has completed execution and called `exit()`, but whose exit status has not yet been harvested by its parent via `wait()` or `waitpid()`.

2. **After 20 seconds, is the `[ls] <defunct>` line still there? Which line of `zombie.c` made it disappear?**  
   After 20 seconds, the defunct entry completely disappears from the process table. The line:
   ```c
   waitpid(pid, &status, 0);
   ```
   made it disappear. Calling `waitpid()` reaps the terminated child, allowing the kernel to finally deallocate its Process Control Block (task_struct) and release its PID.

---

## Part 3. Redirection (`redirect.c`)

### Observe Questions & Answers

1. **Why does `open` return descriptor 1?**  
   POSIX mandates the "lowest available file descriptor" rule: when a new file descriptor is allocated, the kernel assigns the lowest non-negative integer currently unused by the process. Because `close(1)` explicitly freed descriptor 1, the subsequent `open()` call was guaranteed to receive descriptor 1 (standard output).

2. **Did `ls` know its output was redirected?**  
   No. `ls` simply writes to file descriptor 1 as it always does. The shell/program redirects standard output before calling `exec`, making I/O redirection completely transparent to the executed program.

---

## Part 4. A Pipeline (`pipeline.c`)

### Observe Questions & Answers

1. **Did `./pipeline` and `ls /etc | wc -l` give the same number?**  
   Yes, both outputs gave the exact same number (e.g., `224`).

2. **After the change (commenting out `close(p[1])` in the parent), which of the two children never ends, `ls` or `wc`?**  
   The child running **`wc`** never ends (hangs indefinitely).

3. **Why, in one sentence: what is `wc` waiting for?**  
   `wc` keeps reading from the pipe until it encounters an End-Of-File (EOF), which only occurs when *all* open write file descriptors pointing to the pipe are closed; because the parent never closed its write end `p[1]`, the pipe's write reference count never reached zero, leaving `wc` blocked waiting for further input.

---

## Optional: Three Stages (`pipeline3.c`)
Implemented two pipes and three processes for `ls /usr/bin | grep x | wc -l`, properly closing all unused pipe descriptors in children and parent. Output matched the shell exactly.
