#include "util.h"
#include <sys/mman.h>
#include <unistd.h>

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

    ADDR_PTR lines[SENDER_LINES];
    for (int i = 0; i < SENDER_LINES; i++)
        lines[i] = LINE(buf, i);

    srand(time(NULL) ^ getpid());

    printf("Sender hammering L2 set %d (%d lines). Ctrl-C to stop.\n",
           L2_SET, SENDER_LINES);
    fflush(stdout);

    // constant hammer so the receiver can be checked baseline-vs-active
    while (1)
    {
        for (int i = SENDER_LINES - 1; i > 0; i--)
        {
            int j = rand() % (i + 1);
            ADDR_PTR t = lines[i];
            lines[i] = lines[j];
            lines[j] = t;
        }
        for (int i = 0; i < SENDER_LINES; i++)
            *(volatile char *)lines[i];
    }

    printf("Sender finished.\n");
    return 0;
}
