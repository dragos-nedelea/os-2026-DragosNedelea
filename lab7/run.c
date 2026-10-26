#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

int
main(int argc, char *argv[])
{
  printf("parent: active PID is %d\n", getpid());
  fflush(stdout);

  pid_t spawned_pid = fork();
  if (spawned_pid < 0) {
    perror("fork failure");
    exit(1);
  }

  if (spawned_pid == 0) {
    // Child sub-task execution
    printf("child: my PID is %d, my parent is %d\n", getpid(), getppid());
    execlp("ls", "ls", "-l", (char *)NULL);
    perror("exec invocation failed");
    exit(127);
  }

  int exit_info = 0;
  waitpid(spawned_pid, &exit_info, 0);
  printf("parent: child process %d terminated with exit code %d\n", 
         spawned_pid, WEXITSTATUS(exit_info));

  return 0;
}
