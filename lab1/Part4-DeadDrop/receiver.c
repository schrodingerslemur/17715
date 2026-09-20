#include "util.h"
#include <sys/mman.h>
#include <unistd.h>

#define WINDOW 4000 // probe rounds per printed sample

// waits n cycles
static void wait(int n)
{
    while (n--)
        asm volatile("" ::: "memory");
}

int main(int argc, char **argv)
{
    char *buf = mmap(NULL, BUF_SIZE, PROT_READ | PROT_WRITE,
                     MAP_POPULATE | MAP_ANONYMOUS | MAP_PRIVATE | MAP_HUGETLB,
                     -1, 0);
    if (buf == MAP_FAILED)
    {
        perror("mmap (need free huge pages: cat /proc/meminfo | grep HugePages)");
        exit(EXIT_FAILURE);
    }
    memset(buf, 1, BUF_SIZE);

    ADDR_PTR lines[PRIME];
    for (int i = 0; i < PRIME; i++)
        lines[i] = LINE(buf, i);

    srand(time(NULL) ^ getpid());

    printf("Receiver now listening. Watching L2 set %d, %d lines.\n",
           L2_SET, PRIME);
    printf("cols:  avg_latency   slow_lines(>%d)/probe\n", EVICT_CYCLES);
    fflush(stdout);

    while (1)
    {
        CYCLES sum = 0;
        long n = 0, slow = 0;

        for (int r = 0; r < WINDOW; r++)
        {
            for (int i = PRIME - 1; i > 0; i--)
            {
                int j = rand() % (i + 1);
                ADDR_PTR t = lines[i];
                lines[i] = lines[j];
                lines[j] = t;
            }

            for (int i = 0; i < PRIME; i++) // prime
                *(volatile char *)lines[i];

            wait(WAITCYCLES);

            for (int i = 0; i < PRIME; i++) // probe
            {
                CYCLES c = measure_one_block_access_time(lines[i]);
                if (c < 1000) // drop timer/context-switch outliers
                {
                    sum += c;
                    n++;
                    if (c > EVICT_CYCLES)
                        slow++;
                }
            }
        }

        printf("%6.2f      %5.2f\n",
               n ? (double)sum / n : 0.0, (double)slow / WINDOW);
        fflush(stdout);
    }

    printf("Receiver finished.\n");
    return 0;
}
