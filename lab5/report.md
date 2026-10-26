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

## Component 1. Hierarchical Directory Namespace and VFS File Typology

### Commands Run

```bash
whoami && id -u  # non-root execution check
test -d "$HOME/os-lab5" || mkdir -pv "$HOME/os-lab5" && cd "$HOME/os-lab5"

ls -1 /
ls -ld /etc /etc/passwd /bin /dev/null /tmp --time-style=iso
ls -la /dev | head -n 12
file -b /usr/bin/ls /etc/passwd /bin
du -sh /var/log 2>/dev/null ; du -sh /usr 2>/dev/null
```

### Outputs & Observations

```
dragos@utm-linux:~/os-lab5$ ls -ld /etc /etc/passwd /bin /dev/null /tmp
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

## Component 2. Inode Structure, Link Mechanics, and Storage Allocation

### Commands Run

```bash
printf "important notes\n" > notes.txt
ln -v notes.txt hard.txt
ln -sv notes.txt soft.txt
ls -l -i notes.txt hard.txt soft.txt
stat -c "File: %n, Inode: %i, Links: %h, Size: %s" notes.txt
stat -c "File: %n, Inode: %i, Links: %h, Size: %s" hard.txt
stat -c "File: %n, Inode: %i, Links: %h, Target: %N" soft.txt

rm -v notes.txt
cat hard.txt
cat soft.txt

# Directory link count experiment
mkdir -p dir_test && ls -ld dir_test
mkdir -p dir_test/sub1 dir_test/sub2 && ls -ld dir_test

# Sparse file experiment
truncate -s 1G sparse.img
du -sh sparse.img
ls -lh sparse.img
```

### Outputs & Observations

```
dragos@utm-linux:~/os-lab5$ ls -li notes.txt hard.txt soft.txt
 918234 -rw-r--r-- 2 student student 16 Oct 22 15:10 hard.txt
 918234 -rw-r--r-- 2 student student 16 Oct 22 15:10 notes.txt
 918235 lrwxrwxrwx 1 student student  9 Oct 22 15:10 soft.txt -> notes.txt

dragos@utm-linux:~/os-lab5$ rm -v notes.txt
removed 'notes.txt'
dragos@utm-linux:~/os-lab5$ cat hard.txt
important notes
dragos@utm-linux:~/os-lab5$ cat soft.txt
cat: soft.txt: No such file or directory

dragos@utm-linux:~/os-lab5$ ls -lh sparse.img ; du -sh sparse.img
-rw-r--r-- 1 student student 1.0G Oct 22 15:12 sparse.img
0       sparse.img
```
**Figure 5.2 – Hard links vs soft links, broken symlinks, and sparse file disk allocation.**

### Observe Questions & Answers

1. **Inode Identity and Link Equivalence: Do `notes.txt` and `hard.txt` reference an identical inode number? How does `soft.txt` compare in inode allocation?**  
   Yes, `notes.txt` and `hard.txt` share the exact same inode number (`918234`). `soft.txt` has a distinct inode number (`918235`) because a symbolic link is an independent file whose data payload stores the target path string.

2. **Reference Counting vs Symbolic Path Resolution: Why does `cat hard.txt` output successfully following `rm notes.txt`, whereas `cat soft.txt` yields an error?**  
   Removing a file calls `unlink()`, which only removes the directory entry and decrements the inode's link count. Because `hard.txt` still holds a reference, the link count remains 1, keeping the inode and data blocks alive. In contrast, `soft.txt` references the target by name (`notes.txt`); once that name is gone, the link is broken (dangling symlink).

3. **Directory Link Count Invariants: What explains why `dir_test` possessed a hard link count of 2 at creation, which expanded to 4 upon adding two subdirectories?**  
   Initially, link count 2 represents:
   1. The entry `dir_test` inside its parent folder.
   2. The self-reference entry `.` inside `dir_test`.  
   Each new subdirectory (`sub1`, `sub2`) contains a parent reference `..` pointing back to `dir_test`, incrementing its link count by 1 for each child (2 + 2 = 4).

4. **Sparse File Allocation Semantics: For what physical storage reason did `sparse.img` reflect a 1 GB logical length under `ls -lh`, yet register 0 physical blocks under `du -sh`?**  
   `truncate` extended the logical file length in the inode metadata without writing actual data blocks. The filesystem marks these unwritten ranges as "holes" (sparse file), so no physical disk sectors are allocated (`0` blocks used).

---

## Component 3. Discretionary Access Control and Administrative Boundaries

### Observe Questions & Answers

1. **Superuser Root Discretionary Bypass: Can the root superuser view the contents of an unreadable file? Explain the OS security mechanism.**  
   Yes, root can read files even if their permission bits are `000` (`---------`). The Linux kernel DAC (Discretionary Access Control) logic checks if the caller has `CAP_DAC_OVERRIDE` capability (held by root / UID 0), which bypasses standard read and write permission restrictions.

2. **Directory Execute Permission (`+x`) Role: What operational capability does execute permission (`+x`) grant on a directory, and what restriction occurs when it is revoked?**  
   Execute permission on a directory grants the "search / traverse" capability, allowing processes to pass through the directory to access files or subdirectories within it (`chdir`, `open`, `stat`). Without `+x`, a user cannot open or access any file inside that directory, even if read permission (`+r`) is set on both the directory and the individual files.

---

## Component 4. Synthetic In-Memory Filesystems: `/proc` and `/sys`

### Observe Questions & Answers

1. **Pseudo-Filesystem Architectural Purpose: What distinction separates `/proc` from standard persistent storage volumes on disk?**  
   `/proc` is not backed by physical disk blocks. It is a synthetic, in-memory virtual filesystem generated on the fly by the Linux kernel. Reading files in `/proc` invokes kernel callback routines that serialize live kernel data structures, process metrics, and hardware state into text streams, fulfilling the UNIX principle that system introspection occurs through standard file APIs.

---

## Conclusion (Concluzii proprii)

In this laboratory work, I investigated the core structural abstractions of the Linux filesystem:
1. **Inodes as Core Identifiers:** Confirmed that a filename in UNIX is merely an entry in a directory table pointing to an inode. Data blocks belong to the inode, not the filename, explaining why hard links share identical data and withstand target renames.
2. **Directory Invariant:** Verified how directory link counts reflect structural hierarchy: `link_count = 2 + number_of_subdirectories` due to `.` and `..` references.
3. **Storage Efficiency:** Observed sparse file mechanics, demonstrating how modern filesystems avoid allocating physical storage for unwritten zero blocks.
