

//#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
    int fd = open("time.txt", O_CREATE | O_WRONLY);
    if(fd < 0){
        printf("cannot open time.txt\n");
        exit();
    }

    int t = uptime();  // your syscall
    char buf[32];
    int n = sprintf(buf, "%d\n", t);
    write(fd, buf, n);
    close(fd);
    exit();
}
