#include "common.h"
#include <stdint.h>
#include <sys/mman.h>

#define THRESHOLD 100
#define BUFF_SIZE (1 << 20)
#define SAMPLES 100

int main() {
    int line_size = -1;
    uint8_t *buffer =
        (uint8_t *)mmap(NULL, BUFF_SIZE, PROT_READ | PROT_WRITE,
                        MAP_POPULATE | MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (NULL == buffer) {
        perror("Unable to malloc");
        return EXIT_FAILURE;
    }

    // write to the buffer
    for (int i = 0; i < BUFF_SIZE; i++) {
        buffer[i] = rand();
    }

    // TODO: solution here

    printf("Line size is %d\n", line_size);
    munmap(buffer, BUFF_SIZE);
    return 0;
}
