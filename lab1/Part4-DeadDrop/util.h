#ifndef UTIL_H_
#define UTIL_H_

#include "common.h"

#include <string.h>

// ---- shared covert-channel parameters (sender and receiver must agree) ----
#define BUF_SIZE (1 << 20)
#define SET 32          // target L1d set index (bits [11:6])
#define SENDER_LINES 24 // lines the sender hammers to evict the set
#define PRIME 8         // lines the receiver primes (= L1 associativity)
#define EVICT_CYCLES 40 // access slower than this == line was evicted (L2+)
#define WAITCYCLES 2000 // receiver gap between prime and probe
#define BIT_CYCLES 10000000ULL // sender holds each bit this many TSC cycles

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
