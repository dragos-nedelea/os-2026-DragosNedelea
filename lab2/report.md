# Operating Systems · Lab 2: Meet the OS You Will Build (xv6)
**Course:** Operating Systems (OS 2026-2027)  
**Institution:** FCIM / FAF · Technical University of Moldova (UTM)  
**Student:** Dragos Nedelea (st. gr. FAF-241)  
**Checked by:** Patricia Reițman  
**Track:** Track B: Build your own small OS (xv6)  
**Chișinău 2026**

---

## Purpose of the Work (Scopul lucrării)

The objective of this laboratory is to build, boot, explore, and modify an educational operating system, **xv6-riscv** (a modern reimplementation of UNIX Version 6 in ~9,000 lines of C running on the RISC-V architecture under QEMU). Specifically, the goals are:
1. Compiling and booting the xv6 kernel and userland tools using the `riscv64` cross-compiler.
2. Exploring the essential UNIX shell environment and utilities provided by xv6 (`ls`, `cat`, `grep`, `wc`, pipelines).
3. Analyzing the fundamental architectural separation between privileged supervisor mode (`kernel/`) and unprivileged user mode (`user/`).
4. Writing, registering in the build system (`Makefile` with `UPROGS`), and testing custom user-space utility programs: `sleep.c` (using the `pause()` system call on current xv6) and `pingpong.c` (demonstrating two-way pipe IPC).

---

## Component 1. Cross-Compilation and System Initialization

Compiling the kernel and user programs, and launching xv6 in QEMU:

```bash
cd "$HOME/xv6-riscv"
make clean && make qemu
```

```
qemu-system-riscv64 -machine virt -bios none -kernel kernel/kernel -m 128M -smp 3 -nographic -global virtio-mmio.force-legacy=false -drive file=fs.img,if=none,format=raw,id=x0 -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0

xv6 kernel is booting

hart 2 starting
hart 1 starting
init: starting sh
xv6$ 
```
**Figure 2.1 – xv6-riscv compilation and boot sequence inside QEMU.**

---

## Component 2. Exercising xv6 Userland Capabilities

Testing built-in utilities and pipelines inside xv6:

```
xv6$ ls
.              1 1 1024
..             1 1 1024
README         2 2 2441
cat            2 3 34048
echo           2 4 32904
forktest       2 5 15824
grep           2 6 37488
init           2 7 33816
kill           2 8 32776
ln             2 9 32664
ls             2 10 36144
mkdir          2 11 32944
rm             2 12 32920
sh             2 13 54064
stressfs       2 14 34008
usertests      2 15 178520
grind          2 16 48000
wc             2 17 35080
zombie         2 18 32264
console        3 19 0

xv6$ cat README | grep xv6
xv6 is a re-implementation of Dennis Ritchie's and Ken Thompson's Unix
Version 6 (v6).  xv6 loosely follows the structure and style of v6,
```
**Figure 2.2 – Executing directory listings and inter-process pipelines inside xv6.**

### Observe Questions & Answers

1. **Bundled Executable Inventory: Identify three standard utility binaries pre-installed in the xv6 root environment.**  
   xv6 ships with standard UNIX user-space utilities located in `user/`, including:
   - `ls` (directory listing)
   - `cat` (concatenate and display files)
   - `grep` (regular expression pattern search in text streams)  
   *(Other notable utilities include `echo`, `wc`, `kill`, `rm`, `mkdir`, and `sh`)*.

2. **Pipe Subsystem Prerequisites: You chained utilities using a pipeline (`|`). Which two foundational kernel abstractions are strictly required to support this mechanism?**  
   - **Process Creation & Concurrency (Multi-processing):** The operating system kernel must be capable of allocating and managing separate, isolated process address spaces via `fork()` and scheduling them concurrently across CPU harts/cores.
   - **Inter-Process Communication (IPC) & File Descriptor Redirection:** The kernel must provide an in-memory synchronized ring-buffer (`pipe()`) with blocking read/write synchronization, together with descriptor duplication/remapping (`dup()` / `close()`) so that the stdout of the upstream process feeds directly into the stdin of the downstream process.

3. **Comparative Evaluation of Shell Architectures: In a single concise evaluation, contrast the minimal xv6 shell against the Linux Bash environment from Lab 1.**  
   While the xv6 shell is a minimalistic ~500-line implementation providing only essential command execution, simple pipes, and basic redirection without scripting logic, wildcards, or line-editing history, the Linux Bash shell is a complete programming environment equipped with advanced expansion, job control, history, aliases, and comprehensive POSIX utilities.

---

## Component 3. Codebase Structural Analysis

### Observe Questions & Answers

1. **System Call Inventory in `user/cat.c`: Enumerate each distinct system call invoked within `user/cat.c` and specify the exact kernel operation requested.**  
   Inspecting `user/cat.c`:
   - `open(char *path, int mode)`: Asks the kernel to look up the file at `path`, allocate an open file table entry, and return a file descriptor (`fd`).
   - `read(int fd, void *buf, int n)`: Asks the kernel to read up to `n` bytes from the file/stream referenced by `fd` into the process's user-space memory buffer `buf`.
   - `write(int fd, const void *buf, int n)`: Asks the kernel to write `n` bytes from user buffer `buf` to file descriptor `fd` (specifically stdout, `fd 1`).
   - `close(int fd)`: Releases the file descriptor `fd` and decrements the open file structure's reference count.
   - `exit(int status)`: Terminates the calling process, reclaims its page tables and memory allocations, and records its exit status for the parent process.

2. **Kernel Dispatch Location for `sys_read`: Point out the exact source file and function implementation line where `sys_read` executes within the kernel layer.**  
   `sys_read` is implemented in **`kernel/sysfile.c`**, starting at function definition:
   ```c
   uint64
   sys_read(void)
   ```
   (line 57–67 in standard `xv6-riscv`, where it retrieves system call arguments via `argfd()` and `argaddr()`, then delegates to `fileread()`).

3. **Privilege Boundary Distinction: In a single synthesized sentence, what is the architectural difference between instructions executing under `kernel/` versus `user/`?**  
   Code in `kernel/` runs in privileged supervisor mode with unrestricted access to physical memory, CPU control registers, and hardware devices, whereas code in `user/` runs in unprivileged user mode, restricted to its own virtual address space and relying on hardware-trapped system calls to request kernel operations.

---

## Component 4. Utility Implementation: `sleep.c`

### Implementation (`user/sleep.c`)

```c
#include "kernel/types.h"
#include "user/user.h"

static void emit_err_and_bail(const char *msg) {
  fprintf(2, "sleep: %s\n", msg);
  exit(1);
}

int
main(int argc, char *argv[])
{
  if (argc <= 1 || argc > 2) {
    emit_err_and_bail("expected parameter: <ticks>");
  }

  int target_ticks = atoi(argv[1]);
  if (target_ticks < 0) {
    emit_err_and_bail("ticks interval must be positive");
  }

  // Invoke pause system call for tick-based waiting
  if (pause(target_ticks) < 0) {
    emit_err_and_bail("kernel pause call failed");
  }

  exit(0);
}
```

*Note on xv6 syscall naming:* The modern xv6-riscv kernel names the underlying timer pause system call `pause()` (declared as `int pause(int);` in `user/user.h`), rather than the classic name `sleep()`. Our program explicitly invokes `pause(target_ticks)` to compile and execute cleanly on the current xv6 branch.

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
	$U/_pingpong\
```

### Verification inside xv6:

```
xv6$ sleep 10
(pauses for 10 ticks and returns cleanly to shell prompt)

xv6$ sleep
sleep: expected parameter: <ticks>
```
**Figure 2.3 – Testing sleep command execution and argument verification inside xv6.**

---

## Component 5. Inter-Process Communication: `pingpong.c`

### Verification inside xv6:

```
xv6$ pingpong
4: received ping
3: received pong
```
**Figure 2.4 – Inter-process communication between parent and child processes using pipes.**

The parent writes a 1-byte message to pipe `fds_downstream`, which wakes the child blocked on `read(fds_downstream[0])`. The child prints its PID with `received ping` and forwards the byte to `fds_upstream`. The parent, blocked on `read(fds_upstream[0])`, unblocks and prints its PID with `received pong`.

---

## Conclusion (Concluzii proprii)

In this laboratory work, I gained hands-on experience building and exploring the xv6 operating system from source code. Key conclusions include:
1. **Architectural Clarity:** The strict segregation between `kernel/` and `user/` illustrates the hardware privilege boundary; user processes cannot access hardware devices directly, requiring hardware trap entry points into supervisor mode.
2. **System Call Lifecycle:** Tracing `user/cat.c` into `kernel/sysfile.c` clarified how system calls validate parameters in user memory before delegating to internal kernel subsystems (such as the virtual file system).
3. **Build Integration:** Successfully adding `sleep.c` and `pingpong.c` to `UPROGS` in `Makefile` demonstrated how user-space programs are cross-compiled into ELF binaries, packaged into the initial filesystem disk image (`fs.img`), and invoked by the shell.
4. **Current xv6 Compatibility:** Adapting `sleep.c` to use `pause()` aligns with the modern xv6 kernel API specifications, ensuring clean compilation and predictable timer semantics.
