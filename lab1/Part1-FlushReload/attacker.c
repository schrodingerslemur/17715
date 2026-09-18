#include "util.h"
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>


int main() {
    int flag = -1;

    // buf is shared between the attacker and the victim
    char *buf = allocate_shared_buffer();

    // TODO: Implement your attack here

    printf("Flag: %d\n", flag);

    deallocate_shared_buffer(buf);
    return 0;
}