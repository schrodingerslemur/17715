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
// 10_0000_0000_0000_0000_0000
// 21 bits - 5 bit tag | 10 bit set | 6 bit offset
#define N 4

int get_flag(int offset) { return offset >> 6; }

int main()
{
    void *buf =
        mmap(NULL, BUFF_SIZE, PROT_READ | PROT_WRITE,
             MAP_POPULATE | MAP_ANONYMOUS | MAP_PRIVATE | MAP_HUGETLB, -1, 0);

    if (buf == (void *)-1)
    {
        perror("mmap() error\n");
        exit(EXIT_FAILURE);
    }

    *((char *)buf) = 1; // dummy write to trigger page allocation

    printf("buf addr: %p\n", buf);

    srand(time(NULL));
    int offset = rand() % MAX_OFFSET; // 0 to 65535
    int flag = get_flag(offset);      // 0 to 1023
    // flag = (offset & 0xFFFF) >> 6
    // flag is bit 6 - 15
    // flag is 10 bits, 2 ^ 10 - 1 = 1023
    // Flag is the set index in L2
    printf("Flag: %d\n", flag);

    buf += offset;

    while (1)
    {
        for (int i = 0; i < N; i++) // 4 times
        {                           
            // Max offset = 0x1_0000
            // It changes bit 16
            // tag bits:
            // 0b0_0000, 0b0_0001, 0b0_0010, 0b0_0011

            // All same set accesses
            (*(char *)(buf + i * MAX_OFFSET))++;
        }
    }
}
