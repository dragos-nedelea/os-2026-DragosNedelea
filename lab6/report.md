# Operating Systems · Lab 6: Inside the xv6 File System
**Course:** Operating Systems (OS 2026-2027)  
**Institution:** FCIM / FAF · Technical University of Moldova (UTM)  
**Student:** Dragos Nedelea  
**Track:** Track B: Build your own small OS (xv6)

---

## Part 1. Read What xv6 Shows You

### Observe Questions & Answers

1. **What do types 1, 2, and 3 mean (`kernel/stat.h`)?**  
   From `kernel/stat.h`:
   - `#define T_DIR     1` -> Directory
   - `#define T_FILE    2` -> Regular data file
   - `#define T_DEVICE  3` -> Device file (such as console)

2. **Which inode number does the root directory have, and why are `.` and `..` the same there?**  
   - The root directory has inode number **1** (`ROOTINO` defined as 1 in `kernel/fs.h`).  
   - `.` refers to the directory itself (root `/`), and `..` refers to the parent directory. Because the root directory has no parent above it in the filesystem tree, its parent pointer `..` is set to point to itself (inode 1).

3. **An empty directory `a` has size 32. A directory entry has a fixed size. How big is one entry, and which two entries does `a` contain?**  
   - In xv6, each directory entry (`struct dirent` in `kernel/fs.h`) consists of:
     - `ushort inum` (2 bytes)
     - `char name[DIRSIZ]` (14 bytes, where `DIRSIZ` = 14)
     - Total size: **16 bytes**.
   - An empty directory has a size of **32 bytes** because it contains exactly two 16-byte entries: `.` (pointing to `a` itself) and `..` (pointing to the parent directory where `a` was created).

---

## Part 2. Write `mystat` and Observe Hard Links

### Implementation (`user/mystat.c`)
Decodes `st.type` into `"directory"`, `"file"`, or `"device"` and prints:
```
NAME: TYPE, inode N, links N, size N bytes
```

### Observe Questions & Answers

1. **Outputs after `ln` and after `rm`:**
   - After `ln README r2`:
     ```
     README: file, inode 2, links 2, size 2441 bytes
     r2: file, inode 2, links 2, size 2441 bytes
     ```
   - After `rm README`:
     ```
     r2: file, inode 2, links 1, size 2441 bytes
     ```
   - **What changed and what did not:**  
     - The link count (`links`) increased from 1 to 2 when `ln` was called, and decreased from 2 to 1 when `rm` was called.  
     - The inode number (2), file type (`file`), and content size (2441 bytes) remained completely unchanged. `cat r2 | wc` verified that the full contents of `README` are still readable through `r2`.

2. **Line in `kernel/sysfile.c` (`sys_unlink`) that decrements link count; when are data blocks freed?**  
   In `kernel/sysfile.c`, inside `sys_unlink()`:
   ```c
   ip->nlink--;
   iupdate(ip);
   ```
   Data blocks are **not** immediately freed when `sys_unlink` runs if `ip->nlink > 0` or if another process currently has the inode open (`ip->ref > 1`). The data blocks are only freed inside `itrunc(ip)` when the last directory entry has been unlinked (`ip->nlink == 0`) **and** the last process holding an open reference calls `iput(ip)`.

3. **xv6 has no symbolic links and no permissions. Look at `struct dinode` in `kernel/fs.h`: which fields does an xv6 inode have?**  
   From `kernel/fs.h`:
   ```c
   struct dinode {
     short type;           // File type (T_DIR, T_FILE, T_DEVICE)
     short major;          // Major device number (T_DEVICE only)
     short minor;          // Minor device number (T_DEVICE only)
     short nlink;          // Number of hard links to inode in table
     uint size;            // Size of file in bytes
     uint addrs[NDIRECT+1];// Data block addresses (12 direct + 1 indirect)
   };
   ```

---

## Part 3. Measure the Limits

### Observe Questions & Answers

1. **What does `ls` show for `averyveryverylongname`, and which constant explains it?**  
   `ls` truncates the name to `averyveryveryl` (14 characters). This is governed by `#define DIRSIZ 14` in `kernel/fs.h`. Any excess characters beyond 14 bytes are discarded by the directory entry structure.

2. **Compute the largest file xv6 can hold:**  
   From `kernel/fs.h`:
   - `BSIZE = 1024` bytes (1 KiB per block).
   - `NDIRECT = 12` direct block addresses.
   - 1 indirect block points to a block containing 4-byte block addresses:  
     `NINDIRECT = BSIZE / sizeof(uint) = 1024 / 4 = 256` block addresses.
   - Total blocks per file = `NDIRECT + NINDIRECT = 12 + 256 = 268` blocks.
   - **Maximum file size:**  
     $$268 \text{ blocks} \times 1024 \text{ bytes/block} = 274,432 \text{ bytes} \approx 268 \text{ KiB}.$$

3. **From the `nmeta` line during `make fs.img`: disk layout:**  
   ```
   nmeta 46 (boot, super, log blocks 30, inode blocks 13, bitmap blocks 1) blocks 954 total 1000
   ```
   - Total disk size: **1000 blocks** (`FSSIZE` in `kernel/param.h`).
   - Metadata (`nmeta` = 46 blocks):
     - 1 boot block (block 0)
     - 1 superblock (block 1)
     - 30 log blocks (for crash recovery logging)
     - 13 inode blocks (holding 13 * (1024 / 64) = 200 inodes)
     - 1 free bitmap block
   - Data blocks: `1000 - 46 = 954` data blocks available for file and directory contents.

---

## Part 4. Write `find` (`user/find.c`)

### Testing on xv6
```
$ mkdir a
$ mkdir a/b
$ echo x > a/b/r2
$ echo y > a/r2
$ ln cat r2

$ find . r2
./a/b/r2
./a/r2
./r2

$ find a b
a/b

$ find . r2 -n
./a/b/r2 18
./a/r2 17
./r2 3
```

### Stretch Question
**Which of the three `r2` files are the same file as `cat`?**  
The file `./r2` has the exact same inode number as `cat` (inode 3). The files `./a/b/r2` and `./a/r2` have different inode numbers (18 and 17) because they were created with new content (`echo`), whereas `./r2` was created using `ln cat r2` (a hard link).
