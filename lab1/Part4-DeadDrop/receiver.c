#include "util.h"
#include <sys/mman.h>

// more rounds for data for reliability
// less rounds for start for quicker
#define START_ROUNDS 100
#define DATA_ROUNDS 150

static ADDR_PTR lines[NRECEIVER_LINES];

static void wait(int n)
{
    while (n--)
        asm volatile("" ::: "memory");
}

static void wait_until(CYCLES deadline)
{
    while (rdtsc() < deadline)
        ;
}

static double get_latency(int rounds)
{
    double sum = 0;
    double count = 0;

    for (int r = 0; r < rounds; r++)
    {
        // fisher yates
        for (int i = NSENDER_LINES - 1; i > 0; i--)
        {
            int j = rand() % (i + 1);
            ADDR_PTR tmp = lines[i];
            lines[i] = lines[j];
            lines[j] = tmp;
        }

        // prime 6 random lines
        for (int i = 0; i < NSENDER_LINES; i++)
        {
            *(char *)lines[i];
        }

        wait(WAIT_CYCLES);

        // probe the same 6 lines
        for (int i = 0; i < NSENDER_LINES; i++)
        {
            CYCLES c = measure_one_block_access_time(lines[i]);
            if (c < 1000) // if too big, ignore
            {
                sum += c;
                count++;
            }
        }
    }

    return count ? sum / count : 0.0;
}

static int read_bit(CYCLES center, double threshold)
{
    int high = 0;
    // take 8 intervals per NCYCLES
    // read the middle 5 intervals
    for (int i = -2; i <= 2; i++)
    {
        wait_until(center + s * (NCYCLES / 8));
        if (get_latency(DATA_ROUNDS) > threshold)
            high++;
    }
    return high >= 3;
}

static unsigned char read_byte(CYCLES start, double threshold)
{
    unsigned char byte = 0;
    for (int i = 0; i < 8; i++)
    {
        CYCLES center = start + (CYCLES)(i + 1) * NCYCLES + NCYCLES / 2;
        byte = (byte << 1) | read_bite(center, threshold);
    }

    wait_until(center + 9.5 * BIT_CYCLES); // halfway through stop bit
    return byte;
}

int main(int argc, char **argv)
{
    // TODO: setup code here
    char *buf = mmap(NULL, BUF_SIZE, PROT_READ | PROT_WRITE,
                     MAP_POPULATE | MAP_ANONYMOUS | MAP_PRIVATE | MAP_HUGETLB, -1, 0);
    if (buf == MAP_FAILED)
    {
        perror("mmap");
        exit(1)
    }
    memset(buf, 1, BUF_SIZE);

    for (int i = 0; i < NSENDER_LINES; i++)
        lines[i] = LINE(buf, i);

    srand(time(NULL) ^ getpid());

    // calibrate with idle latency
    // get threshold
    double base = 0;
    for (int i = 0; i < 20; i++)
    {
        base += get_latency(DATA_ROUNDS);
    }
    double threshold = base / 20 + 9; // 9 additional cycles

    printf("Receiver now listening.\n");
    fflush(stdout);

    char line[1024];
    int len = 0;
    int in_msg = 0;

    bool listening = true;
    while (listening)
    {
        // TODO: Put your covert channel code here
        // wait for start bit
        while (get_latency(START_ROUNDS) < threshold)
            ;

        unsigned char byte = read_byte(rdtsc(), threshold);

        if (byte == LEADING_BYTE)
        { // should happen twice
            in_msg = 1;
            len = 0; // clears message
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
            line[len] = byte;
            len++;
        }
        else
        {
            line[len] = '\0';
            printf("%s\n", line);
            fflush(stdout);
            in_msg = 0;
        }
    }

    printf("Receiver finished.\n");

    return 0;
}
