#include "util.h"
#include <sys/mman.h>
#include <unistd.h>

static ADDR_PTR lines[12];

static void send_bit(int bit)
{
    uint64_t end = rdtsc() + NCYCLES;

    if (bit)
    {
        while (rdtsc() < end)
        {
            // access the 12 lines
            for (int i = 0; i < NSENDER_LINES; i++)
            {
                *(char *)lines[i];
            }
        }
    }
    else
    {
        while (rdtsc() < end)
            ;
    }
}

// uart protocol
static void send_byte(unsigned char c)
{
    send_bit(1);
    for (int b = 7; b >= 0; b--)
    {
        send_bit((c >> b) & 1);
    }
    send_bit(0);
}

int main(int argc, char **argv)
{
    // TODO: setup code here
    char *buf = mmap(NULL, BUF_SIZE, PROT_READ | PROT_WRITE,
                     MAP_POPULATE | MAP_ANONYMOUS | MAP_PRIVATE | MAP_HUGETLB, -1, 0);
    if (buf == MAP_FAILED)
    {
        perror("mmap");
        exit(1);
    }
    memset(buf, 1, BUF_SIZE);

    for (int i = 0; i < NSENDER_LINES; i++)
        lines[i] = LINE(buf, i);

    printf("Please type a message.\n");

    char text_buf[128];
    while (fgets(text_buf, sizeof(text_buf), stdin))
    {
        // TODO: Put your covert channel code here
        // Send NLEADING_ZEROS
        for (int i = 0; i < NLEADING_ZEROS; i++)
        {
            send_bit(0);
        }
        send_byte(LEADING_BYTE);
        send_byte(LEADING_BYTE);

        // send byte one at a time
        for (char *p = text_buf; *p; p++)
        {
            send_byte((char)*p);
        }
    }

    printf("Sender finished.\n");
    return 0;
}
