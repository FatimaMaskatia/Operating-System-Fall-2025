#include "kernel/types.h"
#include "user/user.h"

int main() {
    int pid = getpid();
    printf("I/O-bound process %d starting\n", pid);

    // Yield frequently
    for(int i = 0; i < 10; i++){
        printf("I/O process %d iteration %d\n", pid, i);
        pause(1);   // simulate I/O / yielding
    }

    printf("I/O-bound process %d done\n", pid);
    exit(0);
}
