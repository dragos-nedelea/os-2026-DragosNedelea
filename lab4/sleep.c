#include "kernel/types.h"
#include "user/user.h"

static void emit_err_and_bail(const char *msg) {
  fprintf(2, "sleep: %s\n", msg);
  exit(1);
}

int
main(int argc, char *argv[])
{
  if (argc <= 1 || argc > 2) {
    emit_err_and_bail("expected parameter: <ticks>");
  }

  int target_ticks = atoi(argv[1]);
  if (target_ticks < 0) {
    emit_err_and_bail("ticks interval must be positive");
  }

  // Invoke pause system call for tick-based waiting
  if (pause(target_ticks) < 0) {
    emit_err_and_bail("kernel pause call failed");
  }

  exit(0);
}
