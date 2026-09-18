#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define MAX_OFFSET 0x10000
#define BUFF_SIZE (1 << 21)
#define N 4

int get_flag(int offset) { return offset >> 6; }

int main() {
    void *buf =
        mmap(NULL, BUFF_SIZE, PROT_READ | PROT_WRITE,
             MAP_POPULATE | MAP_ANONYMOUS | MAP_PRIVATE | MAP_HUGETLB, -1, 0);

    if (buf == (void *)-1) {
        perror("mmap() error\n");
        exit(EXIT_FAILURE);
    }

    *((char *)buf) = 1; // dummy write to trigger page allocation

    printf("buf addr: %p\n", buf);

    srand(time(NULL));
    int offset = rand() % MAX_OFFSET;
    int flag = get_flag(offset);
    printf("Flag: %d\n", flag);

    buf += offset;

    while (1) {
        for (int i = 0; i < N; i++) {
            (*(char *)(buf + i * MAX_OFFSET))++;
        }
    }
}
