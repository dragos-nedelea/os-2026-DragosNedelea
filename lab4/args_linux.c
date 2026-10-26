#include <stdio.h>

int
main(int argc, char *argv[])
{
  printf("argc = %d\n", argc);
  for (int idx = 0; idx < argc; idx++) {
    printf("argv[%d] = [%s]\n", idx, argv[idx]);
  }
  return 0;
}
