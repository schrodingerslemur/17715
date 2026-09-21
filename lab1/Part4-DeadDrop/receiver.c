#include "util.h"
#include <sys/mman.h>
#include <unistd.h>

#define DETECT_WIN 100
#define SAMPLE_WIN 150
#define CALIB_ROUNDS 20
#define THRESH_MARGIN 9.0

#define DEBUG 0

static ADDR_PTR lines[PRIME];

#if DEBUG
static int dbg_votes[8];
static double dbg_lvl[8];
#endif

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

// Prime the watched set, pause, then time each line. A sender hammering the
// same set evicts our lines to L3, which shows up as a higher average latency.
static double measure_level(int rounds)
{
    CYCLES sum = 0;
    long n = 0;
    for (int r = 0; r < rounds; r++)
    {
        for (int i = PRIME - 1; i > 0; i--)
        {
            int j = rand() % (i + 1);
            ADDR_PTR t = lines[i];
            lines[i] = lines[j];
            lines[j] = t;
        }

        for (int i = 0; i < PRIME; i++)
            *(volatile char *)lines[i];

        wait(WAITCYCLES);

        for (int i = 0; i < PRIME; i++)
        {
            CYCLES c = measure_one_block_access_time(lines[i]);
            if (c < 1000) // ignore context-switch spikes
            {
                sum += c;
                n++;
            }
        }
    }
    return n ? (double)sum / n : 0.0;
}

// Read one framed byte, sampling each data bit at its center. Every bit is
// voted over five samples so a stray reading can't flip it.
static unsigned char read_byte(uint64_t t0, double thresh)
{
    unsigned char byte = 0;
    for (int k = 0; k < 8; k++)
    {
        uint64_t center = t0 + (uint64_t)(k + 1) * BIT_CYCLES + BIT_CYCLES / 2;
        int votes = 0;
        double lsum = 0;
        for (int s = -2; s <= 2; s++)
        {
            wait_until(center + s * (BIT_CYCLES / 8));
            double lvl = measure_level(SAMPLE_WIN);
            lsum += lvl;
            if (lvl > thresh)
                votes++;
        }
#if DEBUG
        dbg_votes[k] = votes;
        dbg_lvl[k] = lsum / 5;
#endif
        byte = (byte << 1) | (votes >= 3);
    }
    // stop mid stop-bit (line low) so the caller catches the real rising edge
    // of the next start bit instead of latching late and drifting
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

    // The sender is still blocked on input, so this measures the quiet channel.
    double base = 0;
    for (int k = 0; k < CALIB_ROUNDS; k++)
        base += measure_level(SAMPLE_WIN);
    double thresh = base / CALIB_ROUNDS + THRESH_MARGIN;

    printf("Receiver now listening.\n");
    fflush(stdout);
#if DEBUG
    fprintf(stderr, "base=%.1f thresh=%.1f\n", base / CALIB_ROUNDS, thresh);
#endif

    char line[1024];
    int len = 0;
    int in_msg = 0;

    while (1)
    {
        // wait for a start bit (channel goes high)
        while (measure_level(DETECT_WIN) < thresh)
            ;
        unsigned char byte = read_byte(rdtsc(), thresh);

#if DEBUG
        if (in_msg || byte == MARKER)
        {
            fprintf(stderr, "0x%02X %c  v=", byte,
                    (byte >= 32 && byte < 127) ? byte : '.');
            for (int k = 0; k < 8; k++)
                fprintf(stderr, "%d", dbg_votes[k]);
            fprintf(stderr, "  L=");
            for (int k = 0; k < 8; k++)
                fprintf(stderr, " %.0f", dbg_lvl[k]);
            fprintf(stderr, "\n");
        }
#endif

        // MARKER always (re)starts a message, so a dropped '\n' can't swallow
        // the next one; 0x02 never shows up in real text.
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
