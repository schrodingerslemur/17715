#include "util.h"
#include <sys/mman.h>
#include <unistd.h>

#define DETECT_WIN 100     // probes per level check while hunting a start edge
#define SAMPLE_WIN 150     // probes per level check when sampling a data bit
#define CALIB_ROUNDS 20    // baseline calibration samples
#define THRESH_MARGIN 9.0  // threshold = idle baseline + this (gap is ~20)

static ADDR_PTR lines[PRIME];

// waits n cycles
static void wait(int n)
{
    while (n--)
        asm volatile("" ::: "memory");
}

static void wait_until(uint64_t t)
{
    while (rdtsc() < t)
        ;
}

// average probe latency over `win` prime+probe rounds (the channel level)
static double measure_level(int win)
{
    CYCLES sum = 0;
    long n = 0;
    for (int r = 0; r < win; r++)
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
    return n ? (double)sum / n : 0.0;
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

    for (int i = 0; i < PRIME; i++)
        lines[i] = LINE(buf, i);

    srand(time(NULL) ^ getpid());

    // calibrate against the idle line (sender is blocked in fgets at startup)
    double base = 0;
    for (int k = 0; k < CALIB_ROUNDS; k++)
        base += measure_level(SAMPLE_WIN);
    base /= CALIB_ROUNDS;
    double thresh = base + THRESH_MARGIN;

    printf("Receiver now listening.\n");
    fprintf(stderr, "[dbg] base=%.2f thresh=%.2f\n", base, thresh);
    fflush(stdout);

    char line[1024];
    int len = 0;
    int in_msg = 0; // 0 = waiting for MARKER, 1 = accumulating a message

    while (1)
    {
        // hunt for a start bit: line goes high (evicted -> slow)
        while (measure_level(DETECT_WIN) < thresh)
            ;
        uint64_t t0 = rdtsc(); // a hair into the start bit

        // sample the 8 data bits at their centers, MSB first; each bit is
        // majority-voted over 3 points in the middle third to shrug off noise
        unsigned char byte = 0;
        for (int k = 0; k < 8; k++)
        {
            uint64_t center = t0 + ((uint64_t)(k + 1) * BIT_CYCLES) + BIT_CYCLES / 2;
            int votes = 0;
            for (int s = -1; s <= 1; s++)
            {
                wait_until(center + s * (BIT_CYCLES / 6));
                if (measure_level(SAMPLE_WIN) > thresh)
                    votes++;
            }
            byte = (byte << 1) | (votes >= 2);
        }

        // skip past the stop bit before hunting the next start edge
        wait_until(t0 + (uint64_t)10 * BIT_CYCLES);

        if (in_msg || byte == MARKER) // skip the idle-noise bytes in the log
            fprintf(stderr, "[dbg] 0x%02X %c\n", byte,
                    (byte >= 32 && byte < 127) ? byte : '.');

        if (!in_msg)
        {
            if (byte == MARKER) // real message starts now
            {
                in_msg = 1;
                len = 0;
            }
        }
        else if (byte == '\n')
        {
            line[len] = '\0';
            printf("%s\n", line);
            fflush(stdout);
            in_msg = 0;
        }
        else if (len < (int)sizeof(line) - 1)
        {
            line[len++] = byte;
        }
    }

    printf("Receiver finished.\n");
    return 0;
}
