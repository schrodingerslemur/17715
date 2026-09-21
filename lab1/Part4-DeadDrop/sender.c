#include "util.h"
#include <sys/mman.h>
#include <unistd.h>

static ADDR_PTR lines[SENDER_LINES];

static void send_bit(int bit)
{
    uint64_t end = rdtsc() + BIT_CYCLES; 
    if (bit)
    {
        // hammer the shared L2 set for the whole bit period (100....0 some cycles)
        while (rdtsc() < end)
            for (int i = 0; i < SENDER_LINES; i++) // 12 different lines
                *(volatile char *)lines[i];
    }
    else
    {
        while (rdtsc() < end)
            ;
    }
}

// start bit, 8 data bits MSB first, stop bit (total 10 bits)
static void send_byte(unsigned char c)
{
    send_bit(1);
    for (int b = 7; b >= 0; b--)
        send_bit((c >> b) & 1);
    send_bit(0);
}

int main(int argc, char **argv)
{   
    // allocate page 2 MiB
    char *buf = mmap(NULL, BUF_SIZE, PROT_READ | PROT_WRITE,
                     MAP_POPULATE | MAP_ANONYMOUS | MAP_PRIVATE | MAP_HUGETLB,
                     -1, 0);
    if (buf == MAP_FAILED)
    {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    memset(buf, 1, BUF_SIZE);
    

    // addresses for send_bit
    for (int i = 0; i < SENDER_LINES; i++)
        lines[i] = LINE(buf, i); 

    printf("Please type a message.\n");

    char text_buf[128];
    while (fgets(text_buf, sizeof(text_buf), stdin))
    {
        // low hold preamble
        for (int i = 0; i < PREAMBLE_ZEROS; i++)
            send_bit(0);
        send_byte(MARKER);
        send_byte(MARKER);

        // fgets keeps the trailing '\n', which ends the message
        for (char *p = text_buf; *p; p++)
            send_byte((unsigned char)*p);
    }

    printf("Sender finished.\n");
    return 0;
}
