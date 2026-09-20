#ifndef UTIL_H_
#define UTIL_H_

#include "common.h"

#include <string.h>

// The sender and receiver run on SMT siblings, so they share the L2 cache.
// A 2MB huge page fixes physical bits [20:0], letting both agree on one L2
// set (bits [15:6]); striding the tag by 1<<16 puts several lines in it.
#define BUF_SIZE (1 << 21)
#define L2_SET 512
#define TAG_STRIDE (1 << 16)
#define SENDER_LINES 12        // enough lines to fill the 4-way set
#define PRIME 6                // lines the receiver watches
#define WAITCYCLES 800         // receiver gap between prime and probe
#define BIT_CYCLES 10000000ULL // TSC cycles the sender holds each bit
#define MARKER 0x02            // start-of-message byte
#define PREAMBLE_ZEROS 20      // low run that parks the receiver before MARKER

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
