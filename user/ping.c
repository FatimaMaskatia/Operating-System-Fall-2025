#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  int p1[2];  // pipe from parent to child
  int p2[2];  // pipe from child to parent
  char buf[1];
  int n = 100; // number of ping-pong exchanges

  pipe(p1);
  pipe(p2);

  int pid = fork();
  if (pid < 0) {
    printf("fork failed\n");
    exit(1);
  }

  if (pid == 0) {
    // child process
    for (int i = 0; i < n; i++) {
      read(p1[0], buf, 1);     // read from parent
      // printf("child received: %c\n", buf[0]);
      write(p2[1], buf, 1);    // send back to parent
    }
    exit(0);
  } else {
    // parent process
    char msg = 'p';
    for (int i = 0; i < n; i++) {
      write(p1[1], &msg, 1);   // send to child
      read(p2[0], buf, 1);     // read reply
      // printf("parent got reply %c\n", buf[0]);
    }
    wait(0);
    printf("Ping-pong %d times done!\n", n);
  }

  exit(0);
}
