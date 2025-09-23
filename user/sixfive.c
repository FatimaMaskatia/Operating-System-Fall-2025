#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

char *seps = " -\r\t\n./,";

void
process_file(char *filename)
{
  int fd = open(filename, 0);
  if(fd < 0){
    fprintf(2, "sixfive: cannot open %s\n", filename);
    return;
  }

  char buf[32];
  int idx = 0;
  char c;

  while(read(fd, &c, 1) == 1){
    if(c >= '0' && c <= '9'){
      if(idx < sizeof(buf)-1)
        buf[idx++] = c;
    } else if(strchr(seps, c)){
      if(idx > 0){
        buf[idx] = '\0';
        int n = atoi(buf);
        if(n % 5 == 0 || n % 6 == 0)
          printf("%d\n", n);
        idx = 0;
      }
    }
  }

  // Handle last number if file ended with digits
  if(idx > 0){
    buf[idx] = '\0';
    int n = atoi(buf);
    if(n % 5 == 0 || n % 6 == 0)
      printf("%d\n", n);
  }

  close(fd);
}

int
main(int argc, char *argv[])
{
  if(argc < 2){
    fprintf(2, "Usage: sixfive <file>...\n");
    exit(1);
  }

  for(int i = 1; i < argc; i++){
    process_file(argv[i]);
  }

  exit(0);
}
