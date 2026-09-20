#include "util.h"
#include <sys/mman.h>
#include <unistd.h>

#define TEST_BYTE 0xA5 // 10100101, easy to recognize when oversampled

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

    ADDR_PTR lines[SENDER_LINES];
    for (int i = 0; i < SENDER_LINES; i++)
        lines[i] = (ADDR_PTR)buf + (ADDR_PTR)i * 4096 + (ADDR_PTR)SET * 64;

    srand(time(NULL) ^ getpid());

    printf("Sender transmitting 0x%02X repeatedly on set %d. Ctrl-C to stop.\n",
           TEST_BYTE, SET);
    fflush(stdout);

    // step 2: keep sending a known byte so the receiver can be eyeballed
    while (1)
    {
        for (int b = 7; b >= 0; b--) // MSB first
        {
            int bit = (TEST_BYTE >> b) & 1;
            uint64_t end = rdtsc() + BIT_CYCLES;

            if (bit)
            {
                // hold the set busy for the whole bit period
                while (rdtsc() < end)
                    for (int i = 0; i < SENDER_LINES; i++)
                        *(volatile char *)lines[i];
            }
            else
            {
                // leave the set alone for the whole bit period
                while (rdtsc() < end)
                    ;
            }
        }
    }

    printf("Sender finished.\n");
    return 0;
}
