#include "util.h"
#include <sys/mman.h>
#include <unistd.h>

#define DETECT_WIN 100 // probes used to check for a start bit
#define SAMPLE_WIN 150 // probes used to read one data bit
#define CALIB_ROUNDS 20
#define THRESH_MARGIN 9.0

static ADDR_PTR lines[PRIME];

static void wait(int n)
{
    while (n--)
        asm volatile("" ::: "memory");
}

static void wait_until(uint64_t deadline)
{
    while (rdtsc() < deadline)
        ;
}

// Prime our lines, pause, then time them. If the sender is hammering the same
// set our lines get pushed to L3, so a busy channel reads as a higher latency.
static double measure_level(int rounds)
{
    CYCLES sum = 0;
    long count = 0;

    for (int r = 0; r < rounds; r++)
    {
        for (int i = PRIME - 1; i > 0; i--)
        {
            int j = rand() % (i + 1);
            ADDR_PTR tmp = lines[i];
            lines[i] = lines[j];
            lines[j] = tmp;
        }

        for (int i = 0; i < PRIME; i++)
            *(volatile char *)lines[i];

        wait(WAITCYCLES);

        for (int i = 0; i < PRIME; i++)
        {
            CYCLES c = measure_one_block_access_time(lines[i]);
            if (c < 1000) // skip context-switch spikes
            {
                sum += c;
                count++;
            }
        }
    }

    return count ? (double)sum / count : 0.0;
}

// Read one bit: sample five times around its center and take the majority.
static int read_bit(uint64_t center, double thresh)
{
    int highs = 0;
    for (int s = -2; s <= 2; s++)
    {
        wait_until(center + s * (BIT_CYCLES / 8));
        if (measure_level(SAMPLE_WIN) > thresh)
            highs++;
    }
    return highs >= 3;
}

// Read a framed byte: 8 data bits (MSB first) starting one bit after t0.
static unsigned char read_byte(uint64_t t0, double thresh)
{
    unsigned char byte = 0;
    for (int k = 0; k < 8; k++)
    {
        uint64_t center = t0 + (uint64_t)(k + 1) * BIT_CYCLES + BIT_CYCLES / 2;
        byte = (byte << 1) | read_bit(center, thresh);
    }

    // Stop halfway through the stop bit, while the line is low, so the next
    // start bit is caught on its rising edge rather than latched late.
    wait_until(t0 + 9 * BIT_CYCLES + BIT_CYCLES / 2);
    return byte;
}

int main(int argc, char **argv)
{
    char *buf = mmap(NULL, BUF_SIZE, PROT_READ | PROT_WRITE,
                     MAP_POPULATE | MAP_ANONYMOUS | MAP_PRIVATE | MAP_HUGETLB,
                     -1, 0);
    if (buf == MAP_FAILED)
    {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    memset(buf, 1, BUF_SIZE);

    for (int i = 0; i < PRIME; i++)
        lines[i] = LINE(buf, i);

    srand(time(NULL) ^ getpid());

    // The sender is still blocked on input, so this reads the idle latency.
    double base = 0;
    for (int k = 0; k < CALIB_ROUNDS; k++)
        base += measure_level(SAMPLE_WIN);
    double thresh = base / CALIB_ROUNDS + THRESH_MARGIN;

    printf("Receiver now listening.\n");
    fflush(stdout);

    char line[1024];
    int len = 0;
    int in_msg = 0;

    while (1)
    {
        while (measure_level(DETECT_WIN) < thresh) // wait for a start bit
            ;
        unsigned char byte = read_byte(rdtsc(), thresh);

        // MARKER always starts a fresh message (\n)
        if (byte == MARKER)
        {
            in_msg = 1;
            len = 0;
        }
        else if (in_msg && byte == '\n')
        {
            line[len] = '\0';
            printf("%s\n", line);
            fflush(stdout);
            in_msg = 0;
        }
        else if (in_msg && len < (int)sizeof(line) - 1)
        {
            line[len++] = byte;
        }
    }

    return 0;
}
