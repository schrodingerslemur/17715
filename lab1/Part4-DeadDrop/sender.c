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

static void send_bit(int b)
{
    // hold data + ready until the receiver acks
    while (1) {
        for (int i = 0; i < BURST; i++) {
            if (b)
                touch(B);
            touch(R);
        }
        if (probe(A))
            break;
    }
    // drop them, wait for the ack to clear
    for (int low = 0; low < DEBOUNCE;)
        low = probe(A) ? 0 : low + 1;
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

    printf("Please type a message.\n");

    bool sending = true;
    while (sending) {
        char text_buf[128];
        fgets(text_buf, sizeof(text_buf), stdin);

        // TODO: Put your covert channel code here
        char *bits = string_to_binary(text_buf);
        for (int i = 0; bits[i]; i++)
            send_bit(bits[i] == '1');
        free(bits);
    }

    printf("Sender finished.\n");
    return 0;
}
