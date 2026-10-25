# Operating Systems · Lab 6: Inside the xv6 File System
**Course:** Operating Systems (OS 2026-2027)  
**Institution:** FCIM / FAF · Technical University of Moldova (UTM)  
**Student:** Dragos Nedelea (st. gr. FAF-241)  
**Checked by:** Patricia Reițman  
**Track:** Track B: Build your own small OS (xv6)  
**Chișinău 2026**

---

## Purpose of the Work (Scopul lucrării)

The objective of this laboratory is to inspect and extend the on-disk and in-memory filesystem of xv6 (`kernel/fs.h`, `kernel/stat.h`, `kernel/sysfile.c`). Specifically, the goals are:
1. Decoding disk structures: analyzing `struct dinode`, `struct dirent` (16 bytes: 2 bytes `inum` + 14 bytes `name`), directory entries, and root inode numbering.
2. Developing `user/mystat.c` to decode inode metadata (`st.type`, `st.ino`, `st.nlink`, `st.size`) and observing hard link creation and unlinking dynamics.
3. Implementing `user/find.c` to recursively traverse the directory tree, matching target file names and displaying inode numbers (`-n` option) while avoiding infinite recursion on `.` and `..`.

---

## Part 1. Read What xv6 Shows You

### Observe Questions & Answers

1. **What do types 1, 2, and 3 mean (`kernel/stat.h`)?**  
   From `kernel/stat.h`:
   - `#define T_DIR     1` -> Directory
   - `#define T_FILE    2` -> Regular data file
   - `#define T_DEVICE  3` -> Device file (e.g. console)

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

```c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

char *typename(int t){
  switch(t){
  case T_DIR:
    return "directory";
  case T_FILE:
    return "file";
  case T_DEVICE:
    return "device";
  default:
    return "?";
  }
}

int
main(int argc, char *argv[])
{
  struct stat st;
  if(argc < 2){
    fprintf(2, "usage: mystat <file1> [file2 ...]\n");
    exit(1);
  }

  for(int i = 1; i < argc; i++){
    if(stat(argv[i], &st) < 0){
      fprintf(2, "mystat: cannot stat %s\n", argv[i]);
      continue;
    }
    printf("%s: %s, inode %d, links %d, size %d bytes\n",
           argv[i], typename(st.type), st.ino, st.nlink, (int)st.size);
  }
  exit(0);
}
```

### Observe Questions & Answers

1. **Outputs after `ln` and after `rm`:**
   ```
   $ mystat README
   README: file, inode 2, links 1, size 2441 bytes

   $ ln README r2
   $ mystat README r2
   README: file, inode 2, links 2, size 2441 bytes
   r2: file, inode 2, links 2, size 2441 bytes

   $ rm README
   $ mystat r2
   r2: file, inode 2, links 1, size 2441 bytes
   ```
   **Figure 6.1 – Output of mystat demonstrating hard link creation and unlinking in xv6.**

   - **What changed and what did not:**  
     - The link count (`links`) increased from 1 to 2 when `ln` was called, and decreased from 2 to 1 when `rm` was called.  
     - The inode number (2), file type (`file`), and content size (2441 bytes) remained completely unchanged. `cat r2 | wc` verified that the full contents of `README` are still readable through `r2`.

2. **Line in `kernel/sysfile.c` (`sys_unlink`) that decrements link count; when are data blocks freed?**  
   In `kernel/sysfile.c`, inside `sys_unlink()`:
   ```c
   ip->nlink--;
   iupdate(ip);
   ```
   Data blocks are **not** immediately freed when `sys_unlink` runs. They are freed only inside `iput()` in `kernel/fs.c`:
   ```c
   if(ip->ref == 1 && ip->nlink == 0){
     itrunc(ip);
     ip->type = 0;
     iupdate(ip);
   }
   ```
   Data blocks are freed only when the on-disk link count drops to zero (`ip->nlink == 0`) AND no active process holds an open file descriptor referencing the inode in memory (`ip->ref == 1`).

---

## Part 3. Recursive `find` (`user/find.c`)

### Verification inside xv6:

```
$ mkdir a
$ mkdir a/b
$ echo hello > a/b/README
$ find . README
./README
./a/b/README

$ find . README -n
./README 2
./a/b/README 18
```
**Figure 6.2 – Recursive traversal and inode reporting with find in xv6.**

### Observe Questions & Answers

1. **What happens if you forget to check for `.` and `..`?**  
   If the recursion does not explicitly skip `.` (self) and `..` (parent), `find` enters an infinite recursion cycle (`./.` -> `././.` or `./..` -> `./../..`). It continuously consumes stack frames until user stack overflow triggers a fatal trap, causing xv6 to terminate the program.

2. **Which of the three `r2` files are the same file as `cat`?**  
   The file `./r2` has the exact same inode number as `cat` (inode 3). The files `./a/b/r2` and `./a/r2` have different inode numbers (18 and 17) because they were created with new content (`echo`), whereas `./r2` was created using `ln cat r2` (a hard link sharing inode 3).

---

## Conclusion (Concluzii proprii)

In this laboratory work, I investigated the low-level implementation of the xv6 file system:
1. **On-Disk Directory Architecture:** Understood that directory files in xv6 are simply sequences of fixed-size 16-byte `struct dirent` records mapping an in-line string name (max 14 chars) to a 16-bit inode number (`inum`).
2. **Hard Link Mechanics:** Proved through `mystat` that multiple directory entries point directly to the same underlying inode structure. The inode reference counter guarantees data persistence until all directory links are severed and all open file handles are closed.
3. **Directory Traversal:** Built a fully functional recursive file discovery utility (`find.c`) with depth traversal, dynamic path concatenation, and cycle prevention on `.` and `..` directory references.
