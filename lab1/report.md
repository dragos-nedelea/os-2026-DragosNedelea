# Operating Systems · Lab 1: The OS as a Resource Manager
**Course:** Operating Systems (OS 2026-2027)  
**Institution:** FCIM / FAF · Technical University of Moldova (UTM)  
**Student:** Dragos Nedelea (st. gr. FAF-241)  
**Checked by:** Patricia Reițman  
**Track:** Track A: The real OS, from the outside (Linux)  
**Chișinău 2026**

---

## Purpose of the Work (Scopul lucrării)

The objective of this laboratory is to observe and understand the operating system as a resource manager that stands between user programs and physical hardware. Through practical exploration on a Linux system, we analyze the four core pillars of OS resource management:
1. **File Management:** Exploring the virtual file system tree, file types, ownership, and POSIX permissions (`chmod`, `ls -l`).
2. **Process Management:** Observing process isolation, process identification (PID/PPID), process states, and scheduling (`ps`, `top`, `/proc`).
3. **Memory Management:** Analyzing physical RAM vs. virtual memory allocation, swap space, and per-process resident memory (`free`, `/proc/$PID/status`).
4. **I/O & Storage Management:** Investigating block devices, filesystem mounting, disk utilization, and special device nodes (`df`, `lsblk`, `/dev`).

---

## Before You Start: System Inspection

Initial system inspection to identify the running environment:

```bash
mkdir -p ~/os-lab1 && cd ~/os-lab1
whoami
uname -a
uptime
```

```
$ whoami
student

$ uname -a
Linux os-vm 6.5.0-28-generic #29~22.04.1-Ubuntu SMP PREEMPT_DYNAMIC Thu Mar 28 14:13:00 UTC 2024 x86_64 x86_64 x86_64 GNU/Linux

$ uptime
 14:22:10 up  1:45,  1 user,  load average: 0.08, 0.03, 0.01
```
**Figure 1.1 – Initial Linux environment inspection with whoami, uname, and uptime.**

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

### Command Outputs

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
**Figure 1.2 – Modifying and inspecting POSIX file permissions with chmod and ls -l.**

### Observe — Questions & Answers

1. **Who owns the files you just created, and which group?**  
   The created files (`note.txt`, `copy.txt`, etc.) are owned by user `student` and belong to the group `student` (the user and group under which the active shell process is executing).

2. **What do the ten characters at the start of `ls -l` mean for `note.txt` after `chmod 600`?**  
   The mode string is `-rw-------`:
   - Position 1 (`-`): File type indicator — regular file.
   - Positions 2–4 (`rw-`): **Owner permissions** — the owner has read (`r`) and write (`w`) permissions, but cannot execute (`-`).
   - Positions 5–7 (`---`): **Group permissions** — members of the group have zero permissions.
   - Positions 8–10 (`---`): **Others permissions** — all other users on the system have zero permissions.

3. **Name two directories directly under `/` and say, in one line each, what they hold.**  
   - `/etc`: Holds host-specific system configuration files and service startup configurations.
   - `/bin` (or `/usr/bin`): Holds essential binary executables and command-line utilities (such as `ls`, `cat`, `bash`, `cp`).

---

## Part 2. Processes (Process Management)

### Commands Run

```bash
ps aux | head
ps aux | wc -l
top # (interactive, exited with q)

sleep 300 &
PID=$!
echo "Background sleep PID: $PID"
ps aux | grep "$PID" | grep -v grep
kill $PID
```

### Command Outputs

```
$ sleep 300 &
[1] 14820

$ ps aux | grep 14820 | grep -v grep
student    14820  0.0  0.0   7224   816 pts/0    S    14:28   0:00 sleep 300

$ kill 14820
[1]+  Terminated              sleep 300
```
**Figure 1.3 – Process lifecycle management: background execution, process status inspection, and signal termination via kill.**

### Observe — Questions & Answers

1. **How many processes are currently running on your machine?**  
   Running `ps aux | wc -l` reported 104 processes (1 header line + 103 active processes). The vast majority are background daemon services and kernel threads (`kthreadd`, `systemd`).

2. **What state was `sleep 300` in while running? What does that mean?**  
   In `ps aux`, the `STAT` column showed **`S`** (interruptible sleep). This means the process is not consuming CPU execution cycles; it called the `nanosleep()` system call, asking the kernel timer subsystem to put it into a waiting queue until the requested timer expires or a signal arrives.

3. **What happened to the process when you ran `kill $PID`? Who decided to terminate it?**  
   The process was terminated immediately. The `kill` command issued the default `SIGTERM` signal (signal 15) to the process. Because `sleep` does not install a custom signal handler for `SIGTERM`, the kernel executed the default signal action, which is immediate process termination and reclamation of its resources.

---

## Part 3. Memory (Memory Management)

### Commands Run

```bash
free -h
cat /proc/meminfo | head -10
```

### Command Outputs

```
$ free -h
               total        used        free      shared  buff/cache   available
Mem:           1.9Gi       380Mi       1.1Gi       1.0Mi       480Mi       1.4Gi
Swap:          2.0Gi          0B       2.0Gi
```
**Figure 1.4 – Physical RAM, buffer/cache utilization, and swap space report via free -h.**

### Observe — Questions & Answers

1. **How much total RAM does your VM have, and how much is currently free?**  
   The virtual machine has **1.9 GiB** of total physical RAM, of which **1.1 GiB** is completely free, with **1.4 GiB** available for starting new applications without swapping.

2. **What is "buff/cache", and why is it not considered wasted memory?**  
   "buff/cache" represents OS page cache and file system buffer cache (480 MiB). It is not wasted memory because the kernel opportunistically caches recently read and written disk blocks in idle RAM to accelerate subsequent disk reads. If an application suddenly requests memory, the kernel instantly discards or reclaims these clean cached pages without incurring latency.

3. **How much memory did your `sleep` process use? Why is it so small?**  
   Inspecting `/proc/$PID/status` showed `VmRSS: 816 kB`. The resident set size is under 1 MB because `sleep` is a minimal utility whose binary code and shared libraries (`libc.so`) are mapped read-only and shared across processes, requiring only 1–2 physical pages for its private execution stack.

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

### Command Outputs

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
**Figure 1.5 – Block storage partition topology and filesystem disk utilization via lsblk and df -h.**

### Observe — Questions & Answers

1. **Which device is your root filesystem `/` mounted on?**  
   The root filesystem `/` is mounted on `/dev/mapper/ubuntu--vg-ubuntu--lv`, an LVM logical volume residing on partition `/dev/sda2` of disk `sda`.

2. **Give one entry from `/dev` and say what real thing it stands for.**  
   `/dev/sda` represents the primary physical/virtual SCSI/SATA disk drive. `/dev/null` is a special character pseudo-device that discards all data written to it and immediately returns EOF on read operations.

3. **In one sentence: what does "everything is a file" mean, based on what you saw?**  
   "Everything is a file" means that UNIX unifies access to physical disks, terminal devices, system configurations, and kernel performance metrics under a single directory tree, allowing programs to manipulate hardware resources using uniform system calls (`open`, `read`, `write`, `close`).

---

## Optional: Tracing System Calls with `strace`

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
**Figure 1.6 – Profiling user-to-kernel context switches in ls using strace -c.**

---

## Conclusion (Concluzii proprii)

In this laboratory work, I investigated how the Linux operating system mediates between user-level software and physical hardware. Through practical experimentation with shell utilities and the `/proc` virtual filesystem, I observed how:
1. File permissions and ownership enforce access control boundaries across user accounts.
2. The process scheduler isolates concurrent processes and transitions them across execution states (Running, Sleeping, Terminated).
3. The virtual memory manager dynamically coordinates resident memory sets (`VmRSS`) and intelligently utilizes unused physical RAM as buffer/cache to enhance I/O performance.
4. The UNIX abstraction principle "everything is a file" presents hardware devices, virtual disks, and IPC channels as uniform filesystem nodes, greatly simplifying application development.
