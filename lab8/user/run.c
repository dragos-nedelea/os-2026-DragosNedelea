#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  printf("parent: active pid is %d\n", getpid());

  int subproc_pid = fork();
  if (subproc_pid < 0) {
    fprintf(2, "run: fork failed\n");
    exit(1);
  }

  if (subproc_pid == 0) {
    printf("child: my pid is %d\n", getpid());
    char *exec_params[] = { "echo", "hello", "from", "the", "child", 0 };
    exec("echo", exec_params);
    fprintf(2, "run: execve substitution failed\n");
    exit(1);
  }

  int termination_status = 0;
  wait(&termination_status);
  printf("parent: child %d ended with status %d\n", subproc_pid, termination_status);
  exit(0);
}
