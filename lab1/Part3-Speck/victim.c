#include "speck.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char **argv) {
    srand(time(NULL));

    // Random plaintext (two 16-bit words)
    uint16_t x = (uint16_t)(rand() & 0xffff);
    uint16_t y = (uint16_t)(rand() & 0xffff);
    int group = (argc > 1) ? atoi(argv[1]) : rand() % SPECK_NUM_GROUPS;
    if (group < 0 || group >= SPECK_NUM_GROUPS)
        group = 0;

    // Correct answer
    printf("plaintext: %04x %04x\n", x, y);

    // Hidden info: uncomment these to test your attack
    // printf("Flag: %d\n", group);
    // printf("group key: %04x %04x %04x %04x\n", speck_keys[group][3],
    //        speck_keys[group][2], speck_keys[group][1], speck_keys[group][0]);
    // fflush(stdout);

    block (*const encrypt)(block) = speck_group_heads[group];
    block init = ((block)x << 16) | y;

    // Encrypt the plaintext 1M times to make it *extra* secure!
    block ref = init;
    for (long i = 0; i < SPECK_ITERS; i++)
        ref = encrypt(ref);
    printf("Ciphertext: %08x\n", ref);
    fflush(stdout);

    // Double check the encryption
    while (1) {
        block b = init;
        for (long i = 0; i < SPECK_ITERS; i++)
            b = encrypt(b);
        if (b != ref) {
            printf("UNEXPECTED: ciphertext %08x != %08x\n", b, ref);
            fflush(stdout);
        }
    }
}
