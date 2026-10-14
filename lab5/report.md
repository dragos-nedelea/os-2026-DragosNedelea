# Operating Systems · Lab 5: The File System with Your Own Hands (Linux)
**Course:** Operating Systems (OS 2026-2027)  
**Institution:** FCIM / FAF · Technical University of Moldova (UTM)  
**Student:** Dragos Nedelea  
**Track:** Track A: Warm-up on Linux

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

### Observe Questions & Answers

1. **Which number in `stat` is the inode, and which tells how many names the file has?**  
   - The field labeled `Inode:` (e.g., `Inode: 524312`) indicates the unique inode number within that filesystem.  
   - The field labeled `Links:` (e.g., `Links: 2`) indicates the hard link reference count (number of directory entries pointing to this inode).

2. **After `rm notes.txt`, why could you still read `hard.txt`? What does `rm` really remove?**  
   A hard link is a direct directory entry pointing to an inode. When `notes.txt` was created and linked to `hard.txt`, both names pointed to the exact same inode with a link count of 2.  
   The `rm` command invokes the `unlink()` system call, which only removes the named directory entry (`notes.txt`) and decrements the inode's link count to 1. Since `hard.txt` still points to the inode and its link count is non-zero, the file's data blocks remain completely intact on disk.

3. **Why did `cat soft.txt` fail at the end, although `soft.txt` still exists? How big is `soft.txt` and why exactly that size?**  
   - `soft.txt` is a symbolic (soft) link storing the target string `"notes.txt"`. When `notes.txt` was unlinked, `soft.txt` became a dangling (broken) symlink; `cat` attempted to follow the path `"notes.txt"`, which no longer exists, failing with `No such file or directory`.  
   - `soft.txt` has a size of **9 bytes** because the payload of a symlink is simply the text string of the target path (`"notes.txt"` is 9 characters/bytes long).

4. **A new directory has 2 links, and 4 after you create two subdirectories. Explain where each link comes from:**  
   - Initial state (2 links):
     1. The entry `dir_test` in the parent directory (`..`).
     2. The self-referential entry `.` inside `dir_test`.
   - After creating `sub1` and `sub2` (4 links):
     3. The parent reference `..` inside `dir_test/sub1`.
     4. The parent reference `..` inside `dir_test/sub2`.

5. **Why does a 2-byte file use 4.0K, and a 1 GB file use 0?**  
   - File systems allocate storage in discrete fixed-size blocks (typically **4096 bytes / 4 KiB**). Even if a file has only 2 bytes of data, the filesystem allocates a full 4K block.
   - The 1 GB file created via `truncate -s 1G` is a **sparse file**. `truncate` sets the logical length metadata in the inode without writing any actual data to disk, so zero physical data blocks are allocated (`du` reports `0`). Blocks are only allocated on demand when non-zero bytes are written.

---

## Part 3. Permissions

### Commands Run

```bash
echo secret > s.txt
chmod 600 s.txt ; ls -l s.txt
chmod 000 s.txt ; cat s.txt
chmod u+r s.txt ; cat s.txt

umask
touch new.txt ; mkdir newdir ; ls -ld new.txt newdir

# Box experiment
mkdir box ; echo hi > box/f
chmod u=r,go= box ; ls box ; cat box/f
chmod u=x,go= box ; ls box ; cat box/f
chmod 755 box

# Special bits
ls -l /usr/bin/passwd ; ls -ld /tmp
```

### Observe Questions & Answers

1. **Write 600, 644, 755, and 700 as rwx strings, and give one real use for each:**  
   - `600` = `rw-------`: Private data (e.g., SSH private keys `~/.ssh/id_rsa`, user passwords).
   - `644` = `rw-r--r--`: Standard public documents/source files readable by all, editable only by owner (e.g., `/etc/passwd`, web server HTML files).
   - `755` = `rwxr-xr-x`: Executables and searchable directories (e.g., `/usr/bin/ls`, `/etc`).
   - `700` = `rwx------`: Private user directories (e.g., `~/.ssh/`, private user home directories).

2. **With `chmod 000` you own the file but cannot read it. Why does ownership not help?**  
   The Linux kernel performs permission checking using the most specific matching role: Owner, Group, Other. Because you match the Owner role, the kernel evaluates *only* the owner permission bits (`---`). Since read permission is explicitly denied to the owner, the open call fails with `Permission denied`. (Ownership only grants the permission to change metadata/permissions using `chmod`).

3. **What is your umask, and how does it explain the permissions of `new.txt` and `newdir`?**  
   The umask is `0022` (or `0002`).  
   - Files are created with base mode `0666` (`rw-rw-rw-`) minus umask `0022` = `0644` (`-rw-r--r--`).  
   - Directories are created with base mode `0777` (`rwxrwxrwx`) minus umask `0022` = `0755` (`drwxr-xr-x`).

4. **Explain the box experiment: what does `r` allow on a directory, and what does `x` allow?**  
   - **`r` (Read):** Allows reading the directory table (listing the names of files inside using `ls`). It does **not** allow opening, reading, or traversing to files inside without `x`.
   - **`x` (Execute/Traverse):** Allows traversing through the directory (resolving pathnames, using `cd`, accessing inodes and file contents inside like `cat box/f`). Without `r`, you cannot list names (`ls` fails), but if you know the exact file name, `cat box/f` succeeds!

5. **What do the `s` in `/usr/bin/passwd` and the `t` in `/tmp` mean, and why does `passwd` need the `s`?**  
   - `s` is the **SetUID (Set User ID)** bit (`-rwsr-xr-x`). When an ordinary user runs `/usr/bin/passwd`, the process temporarily elevates its effective user ID to the file's owner (`root`). This is required because changing a password requires modifying `/etc/shadow`, which only root can write to.  
   - `t` is the **Sticky bit** (`drwxrwxrwt`). In a world-writable shared directory like `/tmp`, the sticky bit ensures that users can only delete or rename files that they personally own, preventing malicious users from deleting other users' temporary files.

---

## Part 4. Disks and the Kernel as Files

### Observe Questions & Answers

1. **Disk layout & mount:**  
   Disk `sda` with partition `/dev/sda2` (LVM volume `/dev/mapper/ubuntu--vg-ubuntu--lv`) formatted with `ext4` and mounted at `/`.
2. **Root filesystem usage:**  
   Root filesystem is 31% full (6.4 GB used out of 22 GB).
3. **CPU cores and RAM in `/proc`:**  
   - Cores: `/proc/cpuinfo` shows 2 virtual processor cores.  
   - RAM: `/proc/meminfo` reports 3,994,528 kB (~4 GB).
4. **`/proc/self/status` program name:**  
   The `Name:` field shows `head` (or `cat`), because `/proc/self` is dynamically generated by the kernel to point to whichever process is currently querying the `/proc` filesystem.

---

## Conclusion
The UNIX file system seamlessly abstracts physical blocks, character devices, system statistics, and network endpoints into a uniform hierarchical tree. Understanding inodes, hard vs. soft links, and permissions is essential for low-level system design and security.
