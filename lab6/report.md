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

## Component 1. On-Disk Filesystem Topology and Node Representation

### Observe Questions & Answers

1. **Filesystem Node Typing Semantics: What distinct inode categories are designated by constants 1, 2, and 3 defined within `kernel/stat.h`?**  
   From `kernel/stat.h`:
   - `#define T_DIR     1` -> Directory
   - `#define T_FILE    2` -> Regular data file
   - `#define T_DEVICE  3` -> Device file (e.g. console)

2. **Root Inode Allocation and Self-Reference: Which specific numerical index is allocated to the root filesystem node, and what topological reason explains why `.` and `..` share an identical inode index at root?**  
   - The root directory has inode number **1** (`ROOTINO` defined as 1 in `kernel/fs.h`).  
   - `.` refers to the directory itself (root `/`), and `..` refers to the parent directory. Because the root directory has no parent above it in the filesystem tree, its parent pointer `..` is set to point to itself (inode 1).

3. **Directory Block Sizing and Record Structure: An unpopulated directory `a` exhibits a size of 32 bytes on disk. What byte dimension defines a single `struct dirent` record, and which two entries reside in `a`?**  
   - In xv6, each directory entry (`struct dirent` in `kernel/fs.h`) consists of:
     - `ushort inum` (2 bytes)
     - `char name[DIRSIZ]` (14 bytes, where `DIRSIZ` = 14)
     - Total size: **16 bytes**.
   - An empty directory has a size of **32 bytes** because it contains exactly two 16-byte entries: `.` (pointing to `a` itself) and `..` (pointing to the parent directory where `a` was created).

---

## Component 2. Inode Inspection Utility and Link Count Dynamics: `mystat`

### Implementation (`user/mystat.c`)

```c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

static const char*
resolve_type_label(short ftype)
{
  if (ftype == T_DIR) return "dir";
  if (ftype == T_FILE) return "file";
  if (ftype == T_DEVICE) return "device";
  return "unknown";
}

int
main(int argc, char *argv[])
{
  if (argc <= 1) {
    fprintf(2, "Usage: mystat <file_or_dir_path> ...\n");
    exit(1);
  }

  struct stat meta;
  for (int path_idx = 1; path_idx < argc; ++path_idx) {
    char *target_path = argv[path_idx];
    if (stat(target_path, &meta) < 0) {
      fprintf(2, "mystat error: cannot stat target '%s'\n", target_path);
      continue;
    }
    printf("%s: %s, inode %d, links %d, size %d bytes\n",
           target_path,
           resolve_type_label(meta.type),
           meta.ino,
           meta.nlink,
           (int)meta.size);
  }

  exit(0);
}
```

### Observe Questions & Answers

1. **Structural Invariant Analysis Across Link/Unlink Operations: Compare the output of `mystat` before and after invoking `ln` and `rm`. Which attributes underwent mutation, and which remained immutable?**
   ```
   xv6$ mystat README
   README: file, inode 2, links 1, size 2441 bytes

   xv6$ ln README r2
   xv6$ mystat README r2
   README: file, inode 2, links 2, size 2441 bytes
   r2: file, inode 2, links 2, size 2441 bytes

   xv6$ rm README
   xv6$ mystat r2
   r2: file, inode 2, links 1, size 2441 bytes
   ```
   **Figure 6.1 – Output of mystat demonstrating hard link creation and unlinking in xv6.**

   - **What changed and what did not:**  
     - The link count (`links`) increased from 1 to 2 when `ln` was called, and decreased from 2 to 1 when `rm` was called.  
     - The inode number (2), file type (`file`), and content size (2441 bytes) remained completely unchanged. `cat r2 | wc` verified that the full contents of `README` are still readable through `r2`.

2. **Reference Counter Decrement and Storage Deallocation: Point out the kernel instructions in `kernel/sysfile.c` where the inode link count is decremented. Under what exact conditions are data blocks reclaimed to the free list?**  
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

## Component 3. Recursive Directory Traversal Engine: `find`

### Verification inside xv6:

```
xv6$ mkdir -p a/b
xv6$ echo hello > a/b/README
xv6$ find . README
./README
./a/b/README

xv6$ find . README -inum
./README 2
./a/b/README 18
```
**Figure 6.2 – Recursive traversal and inode reporting with find in xv6.**

### Observe Questions & Answers

1. **Recursion Hazard Prevention: What operational failure occurs if the directory scanner fails to filter out self (`.`) and parent (`..`) links?**  
   If the recursion does not explicitly skip `.` (self) and `..` (parent), `find` enters an infinite recursion cycle (`./.` -> `././.` or `./..` -> `./../..`). It continuously consumes stack frames until user stack overflow triggers a fatal trap, causing xv6 to terminate the program.

2. **Hard Link Inode Identification: When comparing multiple `r2` files across subdirectories, which instance shares physical file identity with `cat`?**  
   The file `./r2` has the exact same inode number as `cat` (inode 3). The files `./a/b/r2` and `./a/r2` have different inode numbers (18 and 17) because they were created with new content (`echo`), whereas `./r2` was created using `ln cat r2` (a hard link sharing inode 3).

---

## Conclusion (Concluzii proprii)

In this laboratory work, I investigated the low-level implementation of the xv6 file system:
1. **On-Disk Directory Architecture:** Understood that directory files in xv6 are simply sequences of fixed-size 16-byte `struct dirent` records mapping an in-line string name (max 14 chars) to a 16-bit inode number (`inum`).
2. **Hard Link Mechanics:** Proved through `mystat` that multiple directory entries point directly to the same underlying inode structure. The inode reference counter guarantees data persistence until all directory links are severed and all open file handles are closed.
3. **Directory Traversal:** Built a fully functional recursive file discovery utility (`find.c`) with depth traversal, dynamic path concatenation, and cycle prevention on `.` and `..` directory references.
