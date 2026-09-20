#include "util.h"
#include <sys/mman.h>
#include <unistd.h>

#define SAMPLE_PROBES 1024   // probes averaged into one printed sample
#define SLOW_THRESH 0.20     // slow-lines/probe above this == bit 1

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

    ADDR_PTR lines[PRIME];
    for (int i = 0; i < PRIME; i++)
        lines[i] = (ADDR_PTR)buf + (ADDR_PTR)i * 4096 + (ADDR_PTR)SET * 64;

    srand(time(NULL) ^ getpid());

    printf("Receiver now listening. Oversampling set %d.\n", SET);
    printf("Each char = one sample; look for the 10100101 pattern.\n");
    fflush(stdout);

    int col = 0;
    while (1)
    {
        CYCLES sum = 0;
        long n = 0;
        for (int r = 0; r < SAMPLE_PROBES; r++)
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
                }
            }
        }

        double avg = n ? (double)sum / n : 0.0;
        printf("%.2f ", avg);
        if (++col % 20 == 0)
            putchar('\n');
        fflush(stdout);
    }

    printf("Receiver finished.\n");
    return 0;
}
