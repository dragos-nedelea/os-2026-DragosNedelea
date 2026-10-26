#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

int
main(int argc, char *argv[])
{
  printf("parent: active PID is %d\n", getpid());
  fflush(stdout);

  pid_t child_proc = fork();
  if (child_proc < 0) {
    perror("fork failure");
    exit(1);
  }

  if (child_proc == 0) {
    // Child immediately exits to create zombie state
    printf("child: my PID is %d, my parent is %d\n", getpid(), getppid());
    execlp("ls", "ls", "-l", (char *)NULL);
    perror("exec failed");
    exit(1);
  }

  // Parent intentionally sleeps without wait(), preserving child zombie entry
  printf("parent sleeping 60 seconds (check 'ps aux | grep zombie' in another terminal)...\n");
  sleep(60);

  return 0;
}
