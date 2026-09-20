#include "util.h"
#include <sys/mman.h>
#include <unistd.h>

#define BUF_SIZE (1 << 20)
#define SET 32    // target L1d set index (bits [11:6])
#define KLINES 16 // > L1 associativity (8) so a full touch fills/evicts the set

// waits n cycles
static void wait(int n)
{
    while (n--)
        asm volatile("" ::: "memory");
}

int main(int argc, char **argv)
{
    char *buf = mmap(NULL, BUF_SIZE, PROT_READ | PROT_WRITE,
                     MAP_POPULATE | MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (buf == MAP_FAILED)
    {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    memset(buf, 1, BUF_SIZE);

    // KLINES addresses, all mapping to L1 set SET, on different pages (tags)
    ADDR_PTR lines[KLINES];
    for (int i = 0; i < KLINES; i++)
        lines[i] = (ADDR_PTR)buf + (ADDR_PTR)i * 4096 + (ADDR_PTR)SET * 64;

    srand(time(NULL) ^ getpid());

    printf("Sender hammering set %d (%d lines). Ctrl-C to stop.\n", SET, KLINES);
    fflush(stdout);

    // step 1: just pound the set forever so the receiver can see the signal
    while (1)
    {
        // shuffle order each pass to dodge the prefetcher
        for (int i = KLINES - 1; i > 0; i--)
        {
            int j = rand() % (i + 1);
            ADDR_PTR t = lines[i];
            lines[i] = lines[j];
            lines[j] = t;
        }
        for (int i = 0; i < KLINES; i++)
            *(volatile char *)lines[i];
        wait(50);
    }

    printf("Sender finished.\n");
    return 0;
}
