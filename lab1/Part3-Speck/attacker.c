#include "common.h"
#include "speck.h"
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>


int main(void) {
    // Read the victim's (public) ciphertext from stdin first, then run the
    // attack. The recovered group's key (from speck.h) decrypts it below.
    unsigned int ct;
    int flag = -1;
    printf("Enter ciphertext > ");
    fflush(stdout);
    if (scanf("%x", &ct) != 1) {
        printf("Invalid ciphertext!\n");
        return 1;
    }

    // TODO: Implement your attack here
    printf("Flag: %d\n", flag);

    block pt = (block)ct;
    for (long i = 0; i < SPECK_ITERS; i++)
        pt = speck_decrypt(speck_keys[flag], pt);
    printf("ciphertext: %08x\n", ct);
    printf("plaintext: %04x %04x\n", (unsigned)((pt >> 16) & 0xffff),
           (unsigned)(pt & 0xffff));
    return 0;
}
