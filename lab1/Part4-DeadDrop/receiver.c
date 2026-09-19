#include "util.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#define THRESHOLD 150
#define BURST 500
#define DEBOUNCE 8

static char *chan;
static long R, B, A; // ready, bit, ack line offsets

static void touch(long off) { (void)*(volatile char *)(chan + off); }

static int probe(long off)
{
    ADDR_PTR a = (ADDR_PTR)(chan + off);
    clflush(a);
    return measure_one_block_access_time(a) < THRESHOLD;
}

static int recv_bit(void)
{
    // wait for the sender to raise ready
    for (int hi = 0; hi < DEBOUNCE;)
        hi = probe(R) ? hi + 1 : 0;

    // sample the bit, majority of a few reads
    int ones = 0;
    for (int i = 0; i < 5; i++)
        ones += probe(B);
    int b = ones >= 3;

    // hold ack until the sender drops ready
    for (int low = 0; low < DEBOUNCE;) {
        for (int i = 0; i < BURST; i++)
            touch(A);
        low = probe(R) ? 0 : low + 1;
    }
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
    // three well-separated, non-strided lines both sides agree on
    R = (st.st_size / 8) & ~63L;
    B = (st.st_size * 3 / 8) & ~63L;
    A = (st.st_size * 7 / 8) & ~63L;

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
