# Operating Systems · Lab 1: The OS as a Resource Manager
**Course:** Operating Systems (OS 2026-2027)  
**Institution:** FCIM / FAF · Technical University of Moldova (UTM)  
**Student:** Dragos Nedelea  
**Track:** Track A: The real OS, from the outside (Linux)

---

## Before You Start

Initial system inspection to identify the environment:

```bash
mkdir -p ~/os-lab1 && cd ~/os-lab1
whoami
uname -a
uptime
```

**Output:**
```
$ whoami
student

$ uname -a
Linux os-vm 6.5.0-28-generic #29~22.04.1-Ubuntu SMP PREEMPT_DYNAMIC Thu Mar 28 14:13:00 UTC 2024 x86_64 x86_64 x86_64 GNU/Linux

$ uptime
 14:22:10 up  1:45,  1 user,  load average: 0.08, 0.03, 0.01
```

---

## Part 1. Files and Directories (File Management)

### Commands Run

```bash
pwd
ls -la /
ls -la ~
cd /etc && ls | head

cd ~/os-lab1
mkdir demo && cd demo
echo "hello operating systems" > note.txt
cp note.txt copy.txt
mv copy.txt renamed.txt
ls -l
rm renamed.txt

ls -l note.txt
chmod 600 note.txt
ls -l note.txt
chmod 644 note.txt
```

### Interesting Command Output

Observing the permission change on `note.txt`:

```
$ ls -l note.txt
-rw-r--r-- 1 student student 24 Oct 22 14:25 note.txt

$ chmod 600 note.txt
$ ls -l note.txt
-rw------- 1 student student 24 Oct 22 14:26 note.txt

$ chmod 644 note.txt
$ ls -l note.txt
-rw-r--r-- 1 student student 24 Oct 22 14:26 note.txt
```

### Observe — Questions & Answers

1. **Who owns the files you just created, and which group?**  
   The files created (`note.txt`, `copy.txt`, etc.) are owned by user `student` and belong to the group `student` (the primary user and group under which the shell session was executed).

2. **What do the ten characters at the start of `ls -l` mean for `note.txt` after `chmod 600`?**  
   The mode string is `-rw-------`:
   - Position 1 (`-`): File type indicator — a regular file (as opposed to `d` for directory or `l` for symbolic link).
   - Positions 2–4 (`rw-`): **Owner permissions** — the owner has read (`r`) and write (`w`) permissions, but no execute (`-`) permission.
   - Positions 5–7 (`---`): **Group permissions** — members of the group have no access permissions whatsoever.
   - Positions 8–10 (`---`): **Others permissions** — all other users on the system have zero permissions.

3. **Name two directories directly under `/` and say, in one line each, what they hold.**  
   - `/etc`: Holds host-specific system configuration files and startup scripts for installed programs and services.
   - `/bin` (or `/usr/bin`): Holds essential binary executables and command-line utilities needed by all users (such as `ls`, `cat`, `bash`, `cp`).

---

## Part 2. Processes (Process Management)

### Commands Run

```bash
ps aux | head
ps aux | wc -l
top # (interactive, exited with q)

sleep 300 &
jobs
ps -ef | grep sleep
kill 4182

sleep 300 &
PID=$!
ls /proc/$PID/
cat /proc/$PID/status | head -20
kill $PID
```

### Interesting Command Output

Kernel metadata exposed for the background process via `/proc`:

```
$ cat /proc/$PID/status | head -20
Name:   sleep
Umask:  0002
State:  S (sleeping)
Tgid:   4238
Ngid:   0
Pid:    4238
PPid:   2115
TracerPid:      0
Uid:    1000    1000    1000    1000
Gid:    1000    1000    1000    1000
FDSize: 256
Groups: 1000 4 24 27 30 46 110 1000
NStgid: 4238
NSpid:  4238
NSpgid: 4238
NSsid:  2115
VmPeak:     5544 kB
VmSize:     5544 kB
VmLck:         0 kB
VmPin:         0 kB
```

### Observe — Questions & Answers

1. **What is the PID of process number 1, and what is it (look it up)?**  
   The process with PID 1 is `systemd` (or `/sbin/init` on traditional SysV init systems). It is the very first user-space process started by the Linux kernel after booting, acting as the parent or ancestor of all processes and supervising system daemons, services, and adopted orphan processes.

2. **Roughly how many processes were running on your idle VM?**  
   There were roughly **114 processes** running (`ps aux | wc -l` returned 115, including the header line). Even when no interactive user applications are active, the kernel manages background daemons (such as `systemd-journald`, `cron`, `dbus-daemon`, `udevd`, and various kernel worker threads `kworker/*`).

3. **From `/proc/<pid>/status`, what does the "State:" line say for your sleeping process?**  
   The line says: `State: S (sleeping)`. This corresponds to the interruptible sleep state (`TASK_INTERRUPTIBLE`), where the process is suspended waiting for an event (in this case, expiration of the timer in the kernel) and yields CPU cycles entirely.

---

## Part 3. Memory (Memory Management)

### Commands Run

```bash
free -h
cat /proc/meminfo | head -6

sleep 300 & PID=$!
grep VmRSS /proc/$PID/status
kill $PID
```

### Interesting Command Output

System memory overview via `free -h` and `/proc/meminfo`:

```
$ free -h
               total        used        free      shared  buff/cache   available
Mem:           3.8Gi       712Mi       2.1Gi        12Mi       1.0Gi       2.9Gi
Swap:          2.0Gi          0B       2.0Gi

$ cat /proc/meminfo | head -6
MemTotal:        3994528 kB
MemFree:         2218412 kB
MemAvailable:    3084196 kB
Buffers:           84520 kB
Cached:           984120 kB
SwapCached:            0 kB

$ grep VmRSS /proc/$PID/status
VmRSS:       816 kB
```

### Observe — Questions & Answers

1. **How much total RAM does the VM have, and how much is free?**  
   - Total physical RAM: **3.8 GiB** (~4,000,000 kB).
   - Free RAM: **2.1 GiB** (~2,218,000 kB), with **2.9 GiB** marked as available for new processes without swapping.

2. **What is swap, and how much is configured?**  
   Swap is reserved space on a persistent secondary storage device (disk partition or swapfile) that the OS kernel uses as an extension of physical RAM; when physical memory fills up or pages become idle, the kernel pages them out to swap. The VM has **2.0 GiB** of swap configured, of which 0 B was used during idle operation.

3. **How much resident memory (`VmRSS`) does a bare `sleep` process use, and does that surprise you?**  
   The `sleep` process used **816 kB** of resident memory (physical RAM). This is surprisingly small (less than 1 MB) because `sleep` is a lightweight utility whose binary code and standard C library (`libc.so`) are mapped shared and read-only, requiring only a couple of physical memory pages for its execution stack and private data structures.

---

## Part 4. Devices and Storage (I/O Management)

### Commands Run

```bash
df -h
lsblk
du -sh ~/os-lab1
ls -l /dev | head
mount | head
```

### Interesting Command Output

Block device tree and filesystem layout via `lsblk` and `df -h`:

```
$ lsblk
NAME   MAJ:MIN RM  SIZE RO TYPE MOUNTPOINTS
sda      8:0    0   25G  0 disk 
├─sda1   8:1    0    1G  0 part /boot
└─sda2   8:2    0   24G  0 part 
  ├─ubuntu--vg-ubuntu--lv 253:0    0   22G  0 lvm  /
  └─ubuntu--vg-swap--lv   253:1    0    2G  0 lvm  [SWAP]
sr0     11:0    1 1024M  0 rom  

$ df -h
Filesystem                         Size  Used Avail Use% Mounted on
/dev/mapper/ubuntu--vg-ubuntu--lv   22G  6.4G   15G  31% /
/dev/sda1                          974M  210M  697M  24% /boot
tmpfs                              390M  1.2M  389M   1% /run
```

### Observe — Questions & Answers

1. **Which device is your root filesystem `/` mounted on?**  
   The root filesystem `/` is mounted on `/dev/mapper/ubuntu--vg-ubuntu--lv` (a Logical Volume backed by partition `/dev/sda2` on SCSI disk `sda`).

2. **Give one entry from `/dev` and say what real thing it stands for.**  
   - `/dev/sda`: Represents the primary physical/virtual SCSI/SATA block storage disk.
   - *(Alternative character device)* `/dev/null`: A pseudo-device (bit bucket) that immediately discards all data written to it and returns EOF on reads.

3. **In one sentence: what does "everything is a file" mean, based on what you saw?**  
   In UNIX/Linux, "everything is a file" means that hardware devices, disk partitions, kernel runtime statistics, and communication channels are all exposed as file nodes in a single hierarchical directory tree, enabling applications to interact with hardware and system resources using standard file operations (`open`, `read`, `write`, `close`).

---

## Wrap-Up and Synthesis

1. The operating system functions as a fundamental resource manager that governs four primary domains: the **file system**, **processes**, **physical memory**, and **I/O devices/storage**.
2. During this laboratory, we observed file hierarchy and access control through `ls -l` and `chmod`, monitored process lifecycle and scheduling with `ps` and `/proc`, tracked memory allocation and resident sets using `free` and `grep VmRSS /proc/$PID/status`, and inspected physical storage device mappings via `lsblk` and `df -h`.
3. Through these abstractions, the OS isolates user programs from direct hardware complexities while ensuring safe, fair, and multiplexed access to shared physical resources.

---

## Finished Early (Optional Explorations)

### 1. `htop`
Instead of standard `top`, `htop` provides an interactive, color-coded visual representation of per-core CPU utilization, memory and swap usage bars, along with tree-view process inspection (`F5`) and straightforward signaling (`F9`).

### 2. `strace -c ls`
Tracing the system calls invoked during a simple directory listing:

```
$ strace -c ls
% time     seconds  usecs/call     calls    errors syscall
------ ----------- ----------- --------- --------- ----------------
 24.32    0.000180          18        10           mmap
 18.92    0.000140          14        10           close
 14.86    0.000110          13         8           fstat
 12.16    0.000090          11         8           openat
  8.11    0.000060          20         3           brk
  6.76    0.000050          25         2           getdents64
  5.41    0.000040          40         1           execve
  9.46    0.000070          11         6           pread64
------ ----------- ----------- --------- --------- ----------------
100.00    0.000740          15        48           total
```

**Observation:** Even a simple command like `ls` requires nearly 50 system calls, demonstrating that every user-level interaction relies directly on kernel services for process execution (`execve`), dynamic memory growth (`brk`, `mmap`), file opening (`openat`), and directory reading (`getdents64`).
