#include "kernel/types.h"
#include "user/user.h"

int main() {
    int pid = getpid();
    printf("Quick process %d\n", pid);
    exit(0);
}
