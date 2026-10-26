#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

int
main(int argc, char *argv[])
{
  int pipe_fds[2];
  if (pipe(pipe_fds) < 0) {
    perror("pipe creation failure");
    exit(1);
  }

  // First Stage: ls /etc writing to pipe
  if (fork() == 0) {
    close(1);
    dup(pipe_fds[1]);
    close(pipe_fds[0]);
    close(pipe_fds[1]);
    execlp("ls", "ls", "/etc", (char *)NULL);
    perror("failed executing ls");
    exit(1);
  }

  // Second Stage: wc -l reading from pipe
  if (fork() == 0) {
    close(0);
    dup(pipe_fds[0]);
    close(pipe_fds[0]);
    close(pipe_fds[1]);
    execlp("wc", "wc", "-l", (char *)NULL);
    perror("failed executing wc");
    exit(1);
  }

  close(pipe_fds[0]);
  close(pipe_fds[1]);

  wait((int *)0);
  wait((int *)0);
  return 0;
}
