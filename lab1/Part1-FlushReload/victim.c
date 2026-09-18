#include "util.h"
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    char *buf = allocate_shared_buffer();

    int flag;
    if (argc > 1) {
        flag = atoi(argv[1]) % SEC_RANGE;
    } else {
        srand(time(NULL));
        flag = rand() % SEC_RANGE; // SEC_RANGE = 1024
    }
    printf("Flag: %d\n", flag);

    while (1) {
        int x = buf[flag * ALIGN]; // ALIGN = 128
        // flag = 0, 128, ..., 1024*128
    }

    deallocate_shared_buffer(buf);
    return 0;
}
