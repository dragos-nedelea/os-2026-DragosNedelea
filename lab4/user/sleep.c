#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  if(argc != 2){
    fprintf(2, "usage: sleep <ticks>\n");
    exit(1);
  }

  // Compatible with both classic sleep() and post-August 2025 pause()
#if defined(SYS_pause) || !defined(SYS_sleep)
  // If user.h declared pause
  pause(atoi(argv[1]));
#else
  sleep(atoi(argv[1]));
#endif

  exit(0);
}
