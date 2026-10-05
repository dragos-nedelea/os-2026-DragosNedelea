# Operating Systems · Lab 2: Meet the OS You Will Build (xv6)
**Course:** Operating Systems (OS 2026-2027)  
**Institution:** FCIM / FAF · Technical University of Moldova (UTM)  
**Student:** Dragos Nedelea  
**Track:** Track B: Build your own small OS (xv6)

---

## Part 2. Use xv6 as the Unix It Is

### Observe Questions & Answers

1. **List three programs xv6 ships with.**  
   xv6 ships with standard UNIX user-space utilities located in `user/`, including:
   - `ls` (directory listing)
   - `cat` (concatenate and display files)
   - `grep` (pattern search in input text)
   *(Other notable utilities: `echo`, `wc`, `kill`, `rm`, `mkdir`, `sh`)*.

2. **You just used a pipe (`|`) inside xv6. Which two OS features must exist for a pipe to work?**  
   - **Process Creation & Concurrency (Multi-processing):** The operating system must be capable of creating separate, isolated process address spaces (`fork()`) and executing them concurrently or time-multiplexed on CPU cores (`scheduler`).
   - **Inter-Process Communication (IPC) & File Descriptor Redirection:** The kernel must provide an in-memory synchronized byte buffer (`pipe()`) with blocking read/write semantics, along with the ability to remap standard input/output file descriptors (`dup()` / `close()`) so that the stdout of the upstream process feeds directly into the stdin of the downstream process.

3. **In one sentence: how does the xv6 shell compare to the Linux shell from Lab 1?**  
   While the xv6 shell is a minimalistic ~500-line implementation providing only essential process execution, simple pipes, and basic redirection without scripting logic or autocompletion, the Linux Bash shell is a full-featured programming environment equipped with advanced expansion, job control, history, aliases, and comprehensive POSIX utilities.

---

## Part 3. Read the Source

### Observe Questions & Answers

1. **Which system calls does `user/cat.c` use, and what does each one ask the kernel to do?**  
   From inspecting `user/cat.c`:
   - `open(char *path, int mode)`: Asks the kernel to find the file corresponding to `path` in the file system, allocate an open file table entry, and return an integer file descriptor (`fd`) referencing it.
   - `read(int fd, void *buf, int n)`: Asks the kernel to read up to `n` bytes from the file or stream represented by `fd` into the process's user-space memory buffer `buf`.
   - `write(int fd, const void *buf, int n)`: Asks the kernel to write `n` bytes from user buffer `buf` to file descriptor `fd` (specifically stdout, `fd 1`).
   - `close(int fd)`: Asks the kernel to release the file descriptor `fd` and decrement the reference count of the associated open file structure.
   - `exit(int status)`: Asks the kernel to terminate the calling process, free its page tables and memory allocations, and record the exit status for its parent process.

2. **In which kernel file, and on which line, is `sys_read` implemented?**  
   `sys_read` is implemented in **`kernel/sysfile.c`**, starting at function definition:
   ```c
   uint64
   sys_read(void)
   ```
   (line 57–67 in standard `xv6-riscv`, where it retrieves the arguments using `argfd()` and `argaddr()` and delegates to `fileread()`).

3. **In one sentence: what is the difference between the code in `kernel/` and the code in `user/`?**  
   Code in `kernel/` runs in privileged supervisor mode with unrestricted access to physical memory, CPU control registers, and hardware devices, whereas code in `user/` runs in unprivileged user mode, restricted to its own virtual address space and relying on hardware-trapped system calls to request kernel operations.

---

## Part 4. First Program: `sleep.c`

The implementation is located in `user/sleep.c`.

### Registration in `Makefile`:
```make
UPROGS=\
	$U/_cat\
	$U/_echo\
	$U/_forktest\
	$U/_grep\
	$U/_init\
	$U/_kill\
	$U/_ln\
	$U/_ls\
	$U/_mkdir\
	$U/_rm\
	$U/_sh\
	$U/_stressfs\
	$U/_usertests\
	$U/_grind\
	$U/_wc\
	$U/_zombie\
	$U/_sleep\
```

### Verification inside xv6:
```
$ sleep 10
(pauses for 10 ticks and returns cleanly)

$ sleep
usage: sleep <ticks>
```
