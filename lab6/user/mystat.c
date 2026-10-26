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
