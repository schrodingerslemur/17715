#include "util.h"
#include <sys/mman.h>
#include <unistd.h>

#define BUF_SIZE (1 << 20)
#define SET 32       // must match sender
#define PRIME 8      // prime exactly the L1 associativity so baseline is all-L1
#define WAITCYCLES 800
#define WINDOW 2000  // probe rounds per printed sample

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

    printf("Receiver now listening. Watching set %d, priming %d lines.\n",
           SET, PRIME);
    printf("cols: avg  max   >40   >60   >90  >150  (counts are lines per probe)\n");
    fflush(stdout);

    while (1)
    {
        CYCLES cyc_sum = 0, cyc_max = 0;
        long n40 = 0, n60 = 0, n90 = 0, n150 = 0;
        long probes = 0;

        for (int r = 0; r < WINDOW; r++)
        {
            for (int i = PRIME - 1; i > 0; i--)
            {
                int j = rand() % (i + 1);
                ADDR_PTR t = lines[i];
                lines[i] = lines[j];
                lines[j] = t;
            }

            // prime
            for (int i = 0; i < PRIME; i++)
                *(volatile char *)lines[i];

            wait(WAITCYCLES);

            // probe
            for (int i = 0; i < PRIME; i++)
            {
                CYCLES c = measure_one_block_access_time(lines[i]);
                cyc_sum += c;
                if (c > cyc_max)
                    cyc_max = c;
                if (c > 40) n40++;
                if (c > 60) n60++;
                if (c > 90) n90++;
                if (c > 150) n150++;
                probes++;
            }
        }

        printf("%5.1f %5llu  %4.2f  %4.2f  %4.2f  %4.2f\n",
               (double)cyc_sum / probes, (unsigned long long)cyc_max,
               (double)n40 / WINDOW, (double)n60 / WINDOW,
               (double)n90 / WINDOW, (double)n150 / WINDOW);
        fflush(stdout);
    }

    printf("Receiver finished.\n");
    return 0;
}
