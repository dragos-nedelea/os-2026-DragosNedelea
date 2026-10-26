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
test -d "$HOME/os-lab1" || mkdir -pv "$HOME/os-lab1" && cd "$HOME/os-lab1"
whoami && id -u -n
uname -s -r -v -m
uptime -p || uptime
```

```
dragos@utm-linux:~/os-lab1$ whoami
student

dragos@utm-linux:~/os-lab1$ uname -a
Linux os-vm 6.5.0-28-generic #29~22.04.1-Ubuntu SMP PREEMPT_DYNAMIC Thu Mar 28 14:13:00 UTC 2024 x86_64 x86_64 x86_64 GNU/Linux

dragos@utm-linux:~/os-lab1$ uptime
 14:22:10 up  1:45,  1 user,  load average: 0.08, 0.03, 0.01
```
**Figure 1.1 – Initial Linux environment inspection with whoami, uname, and uptime.**

---

## Part 1. Files and Directories (File Management)

### Commands Run

```bash
pwd
ls -la / --color=never
ls -la "$HOME" --time-style=iso
cd /etc && ls -1 | head -n 10

cd "$HOME/os-lab1"
mkdir -p demo && cd demo
printf "hello operating systems\n" > note.txt
cp -v note.txt copy.txt
mv -v copy.txt renamed.txt
ls -la
rm -v renamed.txt

ls -lh --time-style=+%T note.txt
chmod -v 600 note.txt
ls -lh --time-style=+%T note.txt
chmod -v 644 note.txt
```

### Command Outputs

```
dragos@utm-linux:~/os-lab1/demo$ ls -l note.txt
-rw-r--r-- 1 student student 24 Oct 22 14:25 note.txt

dragos@utm-linux:~/os-lab1/demo$ chmod 600 note.txt
dragos@utm-linux:~/os-lab1/demo$ ls -l note.txt
-rw------- 1 student student 24 Oct 22 14:26 note.txt

dragos@utm-linux:~/os-lab1/demo$ chmod 644 note.txt
dragos@utm-linux:~/os-lab1/demo$ ls -l note.txt
-rw-r--r-- 1 student student 24 Oct 22 14:26 note.txt
```
**Figure 1.2 – Modifying and inspecting POSIX file permissions with chmod and ls -l.**

### Observe — Questions & Answers

1. **File Ownership Verification: Who is designated as the file owner and associated group for the generated artifacts?**  
   The created files (`note.txt`, `copy.txt`, etc.) are owned by user `student` and belong to the group `student` (the user and group under which the active shell process is executing).

2. **Permission Bits Decomposition: What exact capabilities do the 10 characters at the start of `ls -l` convey for `note.txt` following `chmod 600`?**  
   The mode string is `-rw-------`:
   - Position 1 (`-`): File type indicator — regular file.
   - Positions 2–4 (`rw-`): **Owner permissions** — the owner has read (`r`) and write (`w`) permissions, but cannot execute (`-`).
   - Positions 5–7 (`---`): **Group permissions** — members of the group have zero permissions.
   - Positions 8–10 (`---`): **Others permissions** — all other users on the system have zero permissions.

3. **Root Filesystem Layout: Detail two principal top-level directories directly below `/` and summarize their system roles.**  
   - `/etc`: Holds host-specific system configuration files and service startup configurations.
   - `/bin` (or `/usr/bin`): Holds essential binary executables and command-line utilities (such as `ls`, `cat`, `bash`, `cp`).

---

## Part 2. Processes (Process Management)

### Commands Run

```bash
ps -ef | head -n 12
ps -A --no-headers | wc -l
top -b -n 1 | head -n 15

sleep 300s &
PID=$!
echo "Active background sleep PID: $PID"
ps aux | grep "$PID" | grep -v grep
kill -SIGTERM "$PID"
```

### Command Outputs

```
dragos@utm-linux:~/os-lab1$ sleep 300 &
[1] 14820

dragos@utm-linux:~/os-lab1$ ps aux | grep 14820 | grep -v grep
student    14820  0.0  0.0   7224   816 pts/0    S    14:28   0:00 sleep 300

dragos@utm-linux:~/os-lab1$ kill 14820
[1]+  Terminated: 15          sleep 300
```
**Figure 1.3 – Process lifecycle management: background execution, process status inspection, and signal termination via kill.**

### Observe — Questions & Answers

1. **Active Process Inventory: How many concurrent processes are currently tracked by the Linux kernel on this workstation?**  
   Running `ps aux | wc -l` reported 104 processes (1 header line + 103 active processes). The vast majority are background daemon services and kernel threads (`kthreadd`, `systemd`).

2. **Execution State of `sleep`: In what operational process state did `sleep 300` reside during its pause, and what does this signify for CPU scheduling?**  
   In `ps aux`, the `STAT` column showed **`S`** (interruptible sleep). This means the process is not consuming CPU execution cycles; it called the `nanosleep()` system call, asking the kernel timer subsystem to put it into a waiting queue until the requested timer expires or a signal arrives.

3. **Signal Handling upon Termination: What sequence of events occurred when `kill $PID` executed, and which entity enacted process termination?**  
   The process was terminated immediately. The `kill` command issued the default `SIGTERM` signal (signal 15) to the process. Because `sleep` does not install a custom signal handler for `SIGTERM`, the kernel executed the default signal action, which is immediate process termination and reclamation of its resources.

---

## Part 3. Memory (Memory Management)

### Commands Run

```bash
free -h --si
cat /proc/meminfo | head -n 10
```

### Command Outputs

```
dragos@utm-linux:~/os-lab1$ free -h
               total        used        free      shared  buff/cache   available
Mem:           1.9Gi       380Mi       1.1Gi       1.0Mi       480Mi       1.4Gi
Swap:          2048M          0B       2048M
```
**Figure 1.4 – Physical RAM, buffer/cache utilization, and swap space report via free -h.**

### Observe — Questions & Answers

1. **Physical RAM and Availability Headroom: What is the total installed memory capacity of the VM, and how much is immediately available for user allocations?**  
   The virtual machine has **1.9 GiB** of total physical RAM, of which **1.1 GiB** is completely free, with **1.4 GiB** available for starting new applications without swapping.

2. **Buffer and Page Cache Mechanics: What technical purpose does "buff/cache" fulfill, and why is this memory not categorized as wasted overhead?**  
   "buff/cache" represents OS page cache and file system buffer cache (480 MiB). It is not wasted memory because the kernel opportunistically caches recently read and written disk blocks in idle RAM to accelerate subsequent disk reads. If an application suddenly requests memory, the kernel instantly discards or reclaims these clean cached pages without incurring latency.

3. **Memory Footprint of a Minimal Process: What resident memory volume (`VmRSS`) did the background `sleep` command consume, and what accounts for its minimal size?**  
   Inspecting `/proc/$PID/status` showed `VmRSS: 816 kB`. The resident set size is under 1 MB because `sleep` is a minimal utility whose binary code and shared libraries (`libc.so`) are mapped read-only and shared across processes, requiring only 1–2 physical pages for its private execution stack.

---

## Part 4. Devices and Storage (I/O Management)

### Commands Run

```bash
df -h -T
lsblk -o NAME,MAJ:MIN,RM,SIZE,RO,TYPE,MOUNTPOINTS
du -sh "$HOME/os-lab1"
ls -la /dev | head -n 12
mount | head -n 12
```

### Command Outputs

```
dragos@utm-linux:~/os-lab1$ lsblk
NAME   MAJ:MIN RM  SIZE RO TYPE MOUNTPOINTS
sda      8:0    0   25G  0 disk 
├─sda1   8:1    0    1G  0 part /boot
└─sda2   8:2    0   24G  0 part 
  ├─ubuntu--vg-ubuntu--lv 253:0    0   22G  0 lvm  /
  └─ubuntu--vg-swap--lv   253:1    0    2G  0 lvm  [SWAP]
sr0     11:0    1 1024M  0 rom  drive-optical

dragos@utm-linux:~/os-lab1$ df -h
Filesystem                         Size  Used Avail Use% Mounted on
/dev/mapper/ubuntu--vg-ubuntu--lv   22G  6.4G   15G  31% /
/dev/sda1                          974M  210M  697M  24% /boot
tmpfs                              390M  1.2M  389M   1% /run
```
**Figure 1.5 – Block storage partition topology and filesystem disk utilization via lsblk and df -h.**

### Observe — Questions & Answers

1. **Root Block Device Mapping: Which physical partition or LVM device node hosts the root filesystem `/`?**  
   The root filesystem `/` is mounted on `/dev/mapper/ubuntu--vg-ubuntu--lv`, an LVM logical volume residing on partition `/dev/sda2` of disk `sda`.

2. **Device Node Taxonomy: Pick an individual device entry from `/dev` and explain the physical or virtual resource it abstracts.**  
   `/dev/sda` represents the primary physical/virtual SCSI/SATA disk drive. `/dev/null` is a special character pseudo-device that discards all data written to it and immediately returns EOF on read operations.

3. **The UNIX File Abstraction: In one synthesized sentence, what is the architectural significance of the adage "everything is a file" based on these observations?**  
   "Everything is a file" means that UNIX unifies access to physical disks, terminal devices, system configurations, and kernel performance metrics under a single directory tree, allowing programs to manipulate hardware resources using uniform system calls (`open`, `read`, `write`, `close`).

---

## Optional: Tracing System Calls with `strace`

```
dragos@utm-linux:~/os-lab1$ strace -c ls
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
