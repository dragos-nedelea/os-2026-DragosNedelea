#include "kernel/types.h"
#include "user/user.h"

// Concurrent prime sieve using pipelines
void
sieve(int pleft[2])
{
  close(pleft[1]); // Close write end of left pipe

  int prime;
  if(read(pleft[0], &prime, sizeof(int)) != sizeof(int)){
    close(pleft[0]);
    exit(0);
  }

  printf("prime %d\n", prime);

  int pright[2];
  if(pipe(pright) < 0){
    fprintf(2, "pipe failed\n");
    exit(1);
  }

  int pid = fork();
  if(pid < 0){
    fprintf(2, "fork failed\n");
    exit(1);
  }

  if(pid == 0){
    // Child stage
    close(pleft[0]);
    sieve(pright);
  } else {
    // Current stage
    close(pright[0]); // Close read end of right pipe
    int n;
    while(read(pleft[0], &n, sizeof(int)) == sizeof(int)){
      if(n % prime != 0){
        write(pright[1], &n, sizeof(int));
      }
    }
    close(pleft[0]);
    close(pright[1]);
    wait(0);
    exit(0);
  }
}

int
main(int argc, char *argv[])
{
  int p[2];
  if(pipe(p) < 0){
    fprintf(2, "pipe failed\n");
    exit(1);
  }

  int pid = fork();
  if(pid < 0){
    fprintf(2, "fork failed\n");
    exit(1);
  }

  if(pid == 0){
    sieve(p);
  } else {
    close(p[0]);
    for(int i = 2; i <= 35; i++){
      write(p[1], &i, sizeof(int));
    }
    close(p[1]);
    wait(0);
    exit(0);
  }
}
