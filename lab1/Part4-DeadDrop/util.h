#ifndef UTIL_H_
#define UTIL_H_

#include "common.h"

#include <string.h>

// ---- shared covert-channel parameters (sender and receiver must agree) ----
// Channel: contention on one shared L2 set (siblings share L2). A 2MB huge
// page lets us fix physical bits [20:0], so bits [15:6] pick the L2 set and
// striding the tag by 1<<16 puts many lines in that same set.
#define BUF_SIZE (1 << 21)     // 2MB huge page
#define L2_SET 512             // target L2 set index (bits [15:6])
#define TAG_STRIDE (1 << 16)   // step tag bits, keep the L2 set fixed
#define SENDER_LINES 12        // aggressor lines (> L2 4-way, fills the set)
#define PRIME 6                // lines the receiver primes/probes (small: keep baseline fast)
#define EVICT_CYCLES 30        // L2(~22) vs L3(~38) boundary (from Part 2)
#define WAITCYCLES 800         // receiver gap between prime and probe
#define BIT_CYCLES 40000000ULL // sender holds each bit this many TSC cycles

// address of the i-th line in the target L2 set
#define LINE(buf, i) ((ADDR_PTR)(buf) + (ADDR_PTR)(i) * TAG_STRIDE + ((ADDR_PTR)L2_SET << 6))

// read the timestamp counter
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
