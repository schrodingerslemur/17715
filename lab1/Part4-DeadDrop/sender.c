#include "util.h"
#include <sys/mman.h>
#include <unistd.h>

static ADDR_PTR lines[SENDER_LINES];

// hold one bit for a full bit period: 1 = hammer the L2 set, 0 = leave it idle
static void send_bit(int bit)
{
    uint64_t end = rdtsc() + BIT_CYCLES;
    if (bit)
    {
        while (rdtsc() < end)
            for (int i = 0; i < SENDER_LINES; i++)
                *(volatile char *)lines[i];
    }
    else
    {
        while (rdtsc() < end)
            ;
    }
}

// one framed byte: start(1), 8 data bits MSB-first, stop(0)
static void send_byte(unsigned char c)
{
    send_bit(1);
    for (int b = 7; b >= 0; b--)
        send_bit((c >> b) & 1);
    send_bit(0);
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

    for (int i = 0; i < SENDER_LINES; i++)
        lines[i] = LINE(buf, i);

    printf("Please type a message.\n");

    char text_buf[128];
    while (fgets(text_buf, sizeof(text_buf), stdin))
    {
        // fgets keeps the trailing '\n', which the receiver uses as end marker
        for (char *p = text_buf; *p; p++)
            send_byte((unsigned char)*p);
    }

    printf("Sender finished.\n");
    return 0;
}
