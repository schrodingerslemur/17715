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

// hold data+ready until the receiver acks, then wait for the ack to clear
static void send_bit(int b)
{
    do {
        if (b)
            touch(BIT);
        touch(READY);
    } while (!probe(ACK));
    while (probe(ACK))
        ;
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
