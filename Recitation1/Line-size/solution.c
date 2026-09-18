#include "common.h"
#include <math.h>
#include <stdint.h>
#include <sys/mman.h>

#define THRESHOLD 100
#define BUFF_SIZE (1 << 20)
#define SAMPLES 100

int compare(const void *p1, const void *p2) {
    CYCLES u1 = *(CYCLES *)p1;
    CYCLES u2 = *(CYCLES *)p2;

    return (int)(u1 > u2);
}

bool is_flushed(uint8_t *buffer, int offset) {
    CYCLES latency[SAMPLES] = {0};
    int tmp = 0;

    for (int s = 0; s < SAMPLES; s++) {
        // ensure the address at offset is in the cache
        for (int i = 0; i < 10000; i++) {
            tmp += buffer[offset];
        }

        // flush at offset 0
        clflush((ADDR_PTR)buffer);

        latency[s] = measure_one_block_access_time((ADDR_PTR)(buffer + offset));
    }

    qsort(latency, SAMPLES, sizeof(CYCLES), compare);
    printf("Latency at offset %d is %ld\n", offset, latency[SAMPLES / 2]);
    return latency[SAMPLES / 2] > THRESHOLD;
}

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

    int high = log2(BUFF_SIZE), low = 0;
    while (high != low) {
        int mid = (high + low) / 2;
        if (is_flushed(buffer, 1 << mid)) {
            low = (low == mid) ? low + 1 : mid;
        } else {
            high = mid;
        }
    }
    line_size = 1 << high;

    printf("Line size is %d\n", line_size);
    munmap(buffer, BUFF_SIZE);
    return 0;
}
