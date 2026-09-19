#include "util.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#define THRESHOLD 150
#define READY 0x1000
#define BIT 0x2000
#define ACK 0x3000

static char *chan;

static void wait(long n)
{
    while (n--)
        asm volatile("" ::: "memory");
}

static void touch(long off) { (void)*(volatile char *)(chan + off); }

static int probe(long off)
{
    ADDR_PTR a = (ADDR_PTR)(chan + off);
    clflush(a);
    wait(200);
    return measure_one_block_access_time(a) < THRESHOLD;
}

// wait for ready, sample the bit, then hold ack until ready drops
static int recv_bit(void)
{
    while (!probe(READY))
        ;
    int b = probe(BIT);
    do {
        touch(ACK);
    } while (probe(READY));
    return b;
}

int main(int argc, char **argv) {
    // TODO: setup code here
    int fd = open("./sender", O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }
    struct stat st;
    fstat(fd, &st);
    chan = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (chan == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    printf("Receiver now listening.\n");
    fflush(stdout);

    bool listening = true;
    while (listening) {
        // TODO: Put your covert channel code here
        char byte[9];
        for (int i = 0; i < 8; i++)
            byte[i] = recv_bit() ? '1' : '0';
        byte[8] = '\0';
        putchar((char)strtol(byte, 0, 2));
        fflush(stdout);
    }

    printf("Receiver finished.\n");

    return 0;
}
