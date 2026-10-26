#include "kernel/types.h"
#include "user/user.h"

static inline void panic_msg(const char *err) {
  fprintf(2, "pingpong fault: %s\n", err);
  exit(1);
}

int
main(int argc, char *argv[])
{
  // Channel 1: parent writes, child reads
  // Channel 2: child writes, parent reads
  int fds_downstream[2], fds_upstream[2];
  char token_byte = 'Z';

  if (pipe(fds_downstream) != 0 || pipe(fds_upstream) != 0) {
    panic_msg("unable to allocate IPC pipe pair");
  }

  int child_pid = fork();
  if (child_pid < 0) {
    panic_msg("forking failed during setup");
  }

  if (child_pid == 0) {
    // === Child Task ===
    close(fds_downstream[1]);
    close(fds_upstream[0]);

    if (read(fds_downstream[0], &token_byte, sizeof(char)) != 1) {
      panic_msg("child could not receive token from parent");
    }
    printf("%d: received ping\n", getpid());

    if (write(fds_upstream[1], &token_byte, sizeof(char)) != 1) {
      panic_msg("child could not return token to parent");
    }

    close(fds_downstream[0]);
    close(fds_upstream[1]);
    exit(0);
  }

  // === Parent Task ===
  close(fds_downstream[0]);
  close(fds_upstream[1]);

  if (write(fds_downstream[1], &token_byte, sizeof(char)) != 1) {
    panic_msg("parent failed to dispatch initial ping token");
  }

  if (read(fds_upstream[0], &token_byte, sizeof(char)) != 1) {
    panic_msg("parent failed to receive pong token from child");
  }
  printf("%d: received pong\n", getpid());

  close(fds_downstream[1]);
  close(fds_upstream[0]);

  wait((int *)0);
  exit(0);
}
