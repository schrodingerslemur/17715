#ifndef UTIL_H_
#define UTIL_H_

#include "common.h"

#include <string.h>

#define BUF_SIZE (1 << 21) // same as part 2, 2 MB
#define L2_SET 512         // target set (not number of sets lol)
// only hammer set 512
#define TAG_STRIDE (1 << 16)   // tag index
#define SENDER_LINES 12        // L2 is 4-way, L1 is 1-way
#define PRIME 6                // lines the receiver watches
#define WAITCYCLES 800         // receiver gap between prime and probe
#define BIT_CYCLES 10000000ULL // length of each bit
#define MARKER 0x02            // byte for start of message
#define PREAMBLE_ZEROS 20      // receiver stall

// calculate address of cache line i
// remmebr that buf is base address
#define LINE(buf, i) ((ADDR_PTR)(buf) + (ADDR_PTR)(i) * TAG_STRIDE + ((ADDR_PTR)L2_SET << 6))

static inline uint64_t rdtsc(void)
{
    uint32_t lo, hi;
    asm volatile("lfence\n\trdtsc" : "=a"(lo), "=d"(hi)::"memory");
    return ((uint64_t)hi << 32) | lo;
}

char *string_to_binary(char *s);
char *binary_to_string(char *data);

int string_to_int(char *s);

#endif // UTIL_H_
