#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char*
fmtname(char *path)
{
  char *p;
  // Find first character after last slash.
  for(p = path + strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;
  return p;
}

void
find(char *path, char *target, int show_inode)
{
  char buf[512];
  char *p;
  int fd;
  struct dirent de;
  struct stat st;

  if((fd = open(path, 0)) < 0){
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch(st.type){
  case T_DEVICE:
  case T_FILE:
    if(strcmp(fmtname(path), target) == 0){
      if(show_inode)
        printf("%s %d\n", path, st.ino);
      else
        printf("%s\n", path);
    }
    break;

  case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)){
      fprintf(2, "find: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf + strlen(buf);
    *p++ = '/';

    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;
      if(strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
        continue;

      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;

      if(stat(buf, &st) < 0){
        fprintf(2, "find: cannot stat %s\n", buf);
        continue;
      }

      if(st.type == T_DIR){
        find(buf, target, show_inode);
      } else if(strcmp(de.name, target) == 0){
        if(show_inode)
          printf("%s %d\n", buf, st.ino);
        else
          printf("%s\n", buf);
      }
    }
    break;
  }
  close(fd);
}

int
main(int argc, char *argv[])
{
  int show_inode = 0;
  if(argc == 4 && strcmp(argv[3], "-n") == 0){
    show_inode = 1;
  } else if(argc != 3){
    fprintf(2, "usage: find <path> <filename> [-n]\n");
    exit(1);
  }

  find(argv[1], argv[2], show_inode);
  exit(0);
}
