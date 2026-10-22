#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int p2c[2], c2p[2];     // parent-to-child, child-to-parent
  char b = 'x';

  if(pipe(p2c) < 0 || pipe(c2p) < 0){
    fprintf(2, "pingpong: pipe failed\n");
    exit(1);
  }

  int pid = fork();
  if(pid < 0){
    fprintf(2, "pingpong: fork failed\n");
    exit(1);
  }

  if(pid == 0){
    // Child process
    close(p2c[1]);
    close(c2p[0]);

    if(read(p2c[0], &b, 1) != 1){
      fprintf(2, "child read failed\n");
      exit(1);
    }
    printf("%d: received ping\n", getpid());

    if(write(c2p[1], &b, 1) != 1){
      fprintf(2, "child write failed\n");
      exit(1);
    }

    close(p2c[0]);
    close(c2p[1]);
    exit(0);
  }

  // Parent process
  close(p2c[0]);
  close(c2p[1]);

  if(write(p2c[1], &b, 1) != 1){
    fprintf(2, "parent write failed\n");
    exit(1);
  }

  if(read(c2p[0], &b, 1) != 1){
    fprintf(2, "parent read failed\n");
    exit(1);
  }
  printf("%d: received pong\n", getpid());

  close(p2c[1]);
  close(c2p[0]);
  wait(0);
  exit(0);
}
