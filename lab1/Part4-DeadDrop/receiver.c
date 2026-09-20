#include "util.h"
#include <sys/mman.h>
#include <unistd.h>

#define BUF_SIZE (1 << 20)
#define SET 32        // must match sender
#define KLINES 16     // must match sender
#define THRESHOLD 100 // tentative L1-hit vs miss cutoff (cycles); tune from output
#define WAITCYCLES 800
#define WINDOW 2000 // probe rounds per printed sample

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

    ADDR_PTR lines[KLINES];
    for (int i = 0; i < KLINES; i++)
        lines[i] = (ADDR_PTR)buf + (ADDR_PTR)i * 4096 + (ADDR_PTR)SET * 64;

    srand(time(NULL) ^ getpid());

    printf("Receiver now listening.\n");
    printf("Watching set %d. avg_slow = # of %d lines slower than %d cycles.\n",
           SET, KLINES, THRESHOLD);
    fflush(stdout);

    while (1)
    {
        long slow_sum = 0;   // total lines seen slow this window
        CYCLES cyc_sum = 0;  // total probe latency this window
        long probes = 0;

        for (int r = 0; r < WINDOW; r++)
        {
            for (int i = KLINES - 1; i > 0; i--)
            {
                int j = rand() % (i + 1);
                ADDR_PTR t = lines[i];
                lines[i] = lines[j];
                lines[j] = t;
            }

            // prime
            for (int i = 0; i < KLINES; i++)
                *(volatile char *)lines[i];

            wait(WAITCYCLES);

            // probe
            for (int i = 0; i < KLINES; i++)
            {
                CYCLES c = measure_one_block_access_time(lines[i]);
                cyc_sum += c;
                probes++;
                if (c > THRESHOLD)
                    slow_sum++;
            }
        }

        double avg_slow = (double)slow_sum / WINDOW;
        double avg_cyc = (double)cyc_sum / probes;
        printf("avg_slow = %5.2f / %d   avg_latency = %6.2f cyc\n",
               avg_slow, KLINES, avg_cyc);
        fflush(stdout);
    }

    printf("Receiver finished.\n");
    return 0;
}
