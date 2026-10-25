# Operating Systems · Lab 5: The File System with Your Own Hands (Linux)
**Course:** Operating Systems (OS 2026-2027)  
**Institution:** FCIM / FAF · Technical University of Moldova (UTM)  
**Student:** Dragos Nedelea (st. gr. FAF-241)  
**Checked by:** Patricia Reițman  
**Track:** Track A: Warm-up on Linux  
**Chișinău 2026**

---

## Purpose of the Work (Scopul lucrării)

The objective of this laboratory is to explore the Linux virtual file system (VFS) architecture through low-level shell commands and system calls. Specifically, the goals are:
1. Identifying UNIX file types (regular files, directories, symbolic links, character/block devices, sockets, FIFOs).
2. Investigating inodes, hard links vs. soft (symbolic) links, reference counts, directory link counting (`.` and `..`), and sparse file disk allocation (`truncate`, `du`, `stat`).
3. Analyzing POSIX permission bits (rwx for user, group, others) and evaluating superuser (root) privilege boundaries.
4. Exploring virtual and pseudo-filesystems (`/proc`, `/sys`, `/dev`) to understand how kernel data structures and hardware devices are projected into user space.

---

## Part 1. The Tree and File Types

### Commands Run

```bash
whoami  # non-root user
mkdir -p ~/os-lab5 && cd ~/os-lab5

ls /
ls -ld /etc /etc/passwd /bin /dev/null /tmp
ls -l /dev | head
file /usr/bin/ls /etc/passwd /bin
du -sh /var/log 2>/dev/null ; du -sh /usr 2>/dev/null
```

### Outputs & Observations

```
$ ls -ld /etc /etc/passwd /bin /dev/null /tmp
lrwxrwxrwx  1 root root          7 Apr  8  2024 /bin -> usr/bin
crw-rw-rw-  1 root root       1, 3 Oct 22 14:20 /dev/null
drwxr-xr-x 98 root root       4096 Oct 22 14:20 /etc
-rw-r--r--  1 root root       2656 Oct 22 14:20 /etc/passwd
drwxrwxrwt 14 root root       4096 Oct 22 14:20 /tmp
```
**Figure 5.1 – Identifying UNIX file types from ls -ld mode characters.**

- File type indicator (first character of `ls -l`):
  - `-`: Regular file (e.g., `/etc/passwd`)
  - `d`: Directory (e.g., `/etc`, `/tmp`)
  - `l`: Symbolic link (e.g., `/bin -> usr/bin`)
  - `c`: Character device (e.g., `/dev/null`, `/dev/tty`)
  - `b`: Block device (e.g., `/dev/sda`, `/dev/vda`)
  - `s`: Unix domain socket (e.g., `/run/systemd/journal/socket`)
  - `p`: Named pipe / FIFO

---

## Part 2. Inodes, Links, and Blocks

### Commands Run

```bash
echo "important notes" > notes.txt
ln notes.txt hard.txt
ln -s notes.txt soft.txt
ls -li notes.txt hard.txt soft.txt
stat notes.txt
stat hard.txt
stat soft.txt

rm notes.txt
cat hard.txt
cat soft.txt

# Directory link count experiment
mkdir dir_test && ls -ld dir_test
mkdir dir_test/sub1 dir_test/sub2 && ls -ld dir_test

# Sparse file experiment
truncate -s 1G sparse.img
du -sh sparse.img
ls -lh sparse.img
```

### Outputs & Observations

```
$ ls -li notes.txt hard.txt soft.txt
 918234 -rw-r--r-- 2 student student 16 Oct 22 15:10 hard.txt
 918234 -rw-r--r-- 2 student student 16 Oct 22 15:10 notes.txt
 918235 lrwxrwxrwx 1 student student  9 Oct 22 15:10 soft.txt -> notes.txt

$ rm notes.txt
$ cat hard.txt
important notes
$ cat soft.txt
cat: soft.txt: No such file or directory

$ ls -lh sparse.img ; du -sh sparse.img
-rw-r--r-- 1 student student 1.0G Oct 22 15:12 sparse.img
0       sparse.img
```
**Figure 5.2 – Hard links vs soft links, broken symlinks, and sparse file disk allocation.**

### Observe Questions & Answers

1. **Do `notes.txt` and `hard.txt` share the same inode number? What about `soft.txt`?**  
   Yes, `notes.txt` and `hard.txt` share the exact same inode number (`918234`). `soft.txt` has a distinct inode number (`918235`) because a symbolic link is an independent file whose data payload stores the target path string.

2. **Why does `cat hard.txt` still work after `rm notes.txt`, but `cat soft.txt` fails?**  
   Removing a file calls `unlink()`, which only removes the directory entry and decrements the inode's link count. Because `hard.txt` still holds a reference, the link count remains 1, keeping the inode and data blocks alive. In contrast, `soft.txt` references the target by name (`notes.txt`); once that name is gone, the link is broken (dangling symlink).

3. **Why did `dir_test` have link count 2 initially, and 4 after adding two subdirectories?**  
   Initially, link count 2 represents:
   1. The entry `dir_test` inside its parent folder.
   2. The self-reference entry `.` inside `dir_test`.  
   Each new subdirectory (`sub1`, `sub2`) contains a parent reference `..` pointing back to `dir_test`, incrementing its link count by 1 for each child (2 + 2 = 4).

4. **Why did `sparse.img` report 1 GB with `ls -lh` but 0 with `du -sh`?**  
   `ls -lh` reports the logical file size recorded in the inode metadata. `du -sh` reports actual physical disk blocks allocated. Because `truncate` created holes without writing data blocks, the filesystem allocates zero blocks until data is written (sparse file).

---

## Part 3. Permissions and the Superuser

### Observe Questions & Answers

1. **Translate `chmod 754 script.sh` into permissions for user, group, and others.**  
   - 7 (`rwx`): Owner has Read, Write, and Execute permissions.
   - 5 (`r-x`): Group members have Read and Execute permissions.
   - 4 (`r--`): Others have Read-only permission.

2. **Can a user delete a read-only file in their own directory? Can root be stopped by `chmod 000`?**  
   - Yes, a user can delete a read-only file in a directory they own because directory write and execute permissions (`wx`) control the creation and unlinking of directory entries.
   - No, root bypasses standard discretionary access control (DAC) checks in the kernel and can read and write files with mode `000`.

---

## Part 4. Special Filesystems (`/proc`, `/sys`, `/dev`)

### Commands Run

```bash
ls -l /dev/null /dev/zero /dev/urandom
cat /proc/uptime
head -5 /proc/cpuinfo
cat /proc/sys/fs/file-max
```

```
$ ls -l /dev/null /dev/zero /dev/urandom
crw-rw-rw- 1 root root 1, 3 Oct 22 14:20 /dev/null
crw-rw-rw- 1 root root 1, 5 Oct 22 14:20 /dev/zero
crw-rw-rw- 1 root root 1, 9 Oct 22 14:20 /dev/urandom
```
**Figure 5.3 – Character pseudo-devices and kernel interfaces in /dev and /proc.**

---

## Conclusion (Concluzii proprii)

In this laboratory work, I investigated the core structural abstractions of the Linux filesystem:
1. **Inodes as Core Identifiers:** Confirmed that a filename in UNIX is merely an entry in a directory table pointing to an inode. Data blocks belong to the inode, not the filename, explaining why hard links share identical data and withstand target renames.
2. **Directory Invariant:** Verified how directory link counts reflect structural hierarchy: `link_count = 2 + number_of_subdirectories` due to `.` and `..` references.
3. **Storage Efficiency:** Observed sparse file mechanics, demonstrating how modern filesystems avoid allocating physical storage for unwritten zero blocks.
4. **VFS Power:** Explored how `/dev`, `/proc`, and `/sys` virtualize hardware devices, processes, and kernel tunables behind standard POSIX filesystem interfaces.
