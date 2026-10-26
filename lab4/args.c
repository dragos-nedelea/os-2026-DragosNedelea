#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  printf("argc = %d\n", argc);
  for (int idx = 0; idx < argc; idx++) {
    printf("argv[%d] = [%s]\n", idx, argv[idx]);
  }
  exit(0);
}
