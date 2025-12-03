#include "kernel/types.h"
#include "user/user.h"

int main() {
    printf("CPU-bound process %d starting\n", getpid());
    while(1);   // infinite loop so demotion shows
}
