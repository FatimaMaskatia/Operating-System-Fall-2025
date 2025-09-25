#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int t = uptime();   // system call returns ticks since boot
  printf("uptime: %d ticks\n", t);
  exit(0);
}
