#include "common.h"
#include <string.h>
#include <sys/mman.h>

#define BUFF_SIZE (1 << 21)

int main() {
    void *buf =
        mmap(NULL, BUFF_SIZE, PROT_READ | PROT_WRITE,
             MAP_POPULATE | MAP_ANONYMOUS | MAP_PRIVATE | MAP_HUGETLB, -1, 0);
    if (buf == (void *)-1) {
        perror("mmap() error\n");
        exit(EXIT_FAILURE);
    }
    memset(buf, 0x1, BUFF_SIZE);

    // Warmup
    for (int i = 0; i < 1000000; i++)
        measure_one_block_access_time((uint64_t)buf);

    int flag = -1;

    // TODO: Implement your attack here

    printf("Flag: %d\n", flag);
    return 0;
}
