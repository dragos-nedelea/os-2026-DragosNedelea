#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

int
main(int argc, char *argv[])
{
  int fds_upstream[2], fds_downstream[2];
  if (pipe(fds_upstream) < 0 || pipe(fds_downstream) < 0) {
    perror("pipe initialization error");
    exit(1);
  }

  // Stage 1: ls /usr/bin -> upstream pipe
  if (fork() == 0) {
    close(1);
    dup(fds_upstream[1]);
    close(fds_upstream[0]);
    close(fds_upstream[1]);
    close(fds_downstream[0]);
    close(fds_downstream[1]);
    execlp("ls", "ls", "/usr/bin", (char *)NULL);
    perror("exec ls failure");
    exit(1);
  }

  // Stage 2: grep x (reads upstream, writes downstream)
  if (fork() == 0) {
    close(0);
    dup(fds_upstream[0]);
    close(1);
    dup(fds_downstream[1]);
    close(fds_upstream[0]);
    close(fds_upstream[1]);
    close(fds_downstream[0]);
    close(fds_downstream[1]);
    execlp("grep", "grep", "x", (char *)NULL);
    perror("exec grep failure");
    exit(1);
  }

  // Stage 3: wc -l reads downstream
  if (fork() == 0) {
    close(0);
    dup(fds_downstream[0]);
    close(fds_upstream[0]);
    close(fds_upstream[1]);
    close(fds_downstream[0]);
    close(fds_downstream[1]);
    execlp("wc", "wc", "-l", (char *)NULL);
    perror("exec wc failure");
    exit(1);
  }

  // Master process closes all pipe descriptors
  close(fds_upstream[0]);
  close(fds_upstream[1]);
  close(fds_downstream[0]);
  close(fds_downstream[1]);

  wait((int *)0);
  wait((int *)0);
  wait((int *)0);
  return 0;
}
