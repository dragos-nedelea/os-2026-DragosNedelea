#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

// Extract final component filename from a full path
static const char*
extract_leaf_name(const char *raw_path)
{
  const char *leaf = raw_path;
  for (const char *curr = raw_path; *curr; ++curr) {
    if (*curr == '/') {
      leaf = curr + 1;
    }
  }
  return leaf;
}

// Assemble parent path and child entry name into destination buffer
static int
combine_subpath(char *dest_buf, int dest_cap, const char *dir_prefix, const char *entry_leaf)
{
  int plen = strlen(dir_prefix);
  int nlen = strlen(entry_leaf);
  if (plen + 1 + nlen + 1 > dest_cap) {
    return -1;
  }
  strcpy(dest_buf, dir_prefix);
  dest_buf[plen] = '/';
  strcpy(dest_buf + plen + 1, entry_leaf);
  return 0;
}

// Recursive directory scanning routine
static void
traverse_and_match(const char *current_dir, const char *query_pattern, int show_inode)
{
  int dir_fd = open(current_dir, 0);
  if (dir_fd < 0) {
    fprintf(2, "find: open failed on path '%s'\n", current_dir);
    return;
  }

  struct stat node_st;
  if (fstat(dir_fd, &node_st) < 0) {
    fprintf(2, "find: stat failed on path '%s'\n", current_dir);
    close(dir_fd);
    return;
  }

  // Handle direct file argument
  if (node_st.type != T_DIR) {
    const char *bname = extract_leaf_name(current_dir);
    if (strcmp(bname, query_pattern) == 0) {
      if (show_inode) {
        printf("%s %d\n", current_dir, node_st.ino);
      } else {
        printf("%s\n", current_dir);
      }
    }
    close(dir_fd);
    return;
  }

  char nested_path[512];
  struct dirent dir_record;

  while (read(dir_fd, &dir_record, sizeof(dir_record)) == sizeof(dir_record)) {
    if (dir_record.inum == 0) continue;

    // Filter out '.' and '..' directory self/parent links
    if (dir_record.name[0] == '.' && (dir_record.name[1] == '\0' || 
       (dir_record.name[1] == '.' && dir_record.name[2] == '\0'))) {
      continue;
    }

    char record_name[DIRSIZ + 1];
    memmove(record_name, dir_record.name, DIRSIZ);
    record_name[DIRSIZ] = '\0';

    if (combine_subpath(nested_path, sizeof(nested_path), current_dir, record_name) < 0) {
      fprintf(2, "find: buffer limit exceeded traversing '%s'\n", current_dir);
      continue;
    }

    struct stat child_st;
    if (stat(nested_path, &child_st) < 0) {
      fprintf(2, "find: unable to stat '%s'\n", nested_path);
      continue;
    }

    if (strcmp(record_name, query_pattern) == 0) {
      if (show_inode) {
        printf("%s %d\n", nested_path, child_st.ino);
      } else {
        printf("%s\n", nested_path);
      }
    }

    if (child_st.type == T_DIR) {
      traverse_and_match(nested_path, query_pattern, show_inode);
    }
  }

  close(dir_fd);
}

int
main(int argc, char *argv[])
{
  if (argc < 3) {
    fprintf(2, "Usage: find <dir> <pattern> [-inum]\n");
    exit(1);
  }

  int display_inum = 0;
  if (argc >= 4 && strcmp(argv[3], "-inum") == 0) {
    display_inum = 1;
  }

  traverse_and_match(argv[1], argv[2], display_inum);
  exit(0);
}
