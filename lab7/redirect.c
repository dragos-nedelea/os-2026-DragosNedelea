#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>

int
main(int argc, char *argv[])
{
  pid_t child_worker = fork();
  if (child_worker < 0) {
    perror("failed to fork worker");
    exit(1);
  }

  if (child_worker == 0) {
    // Redirect standard output to out.txt
    close(1);
    int target_fd = open("out.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (target_fd < 0) {
      perror("file open failed");
      exit(1);
    }
    fprintf(stderr, "open allocated descriptor %d\n", target_fd);
    execlp("ls", "ls", "-l", (char *)NULL);
    perror("exec call failed");
    exit(1);
  }

  waitpid(child_worker, (int *)0, 0);
  printf("completed: output captured in out.txt\n");
  return 0;
}
