#ifndef UTIL_H_
#define UTIL_H_

#include "common.h"
#include <string.h>

#define BUF_SIZE (1 << 21)
#define TAG_STRIDE (1 << 16)
#define TARGET_SET 512 // arbitrary as long as < 1024

#define NSENDER_LINES 12
#define NRECEIVER_LINES 6
#define NCYCLES 10000000
#define NLEADING_ZEROS 20
#define LEADING_BYTE 0x02

#define WAIT_CYCLES 800

#define LINE(buf, i) ((ADDR_PTR)(buf) + (ADDR_PTR)(i) * TAG_STRIDE + ((ADDR_PTR)TARGET_SET << 6))

static inline CYCLES rdtsc()
{
    CYCLES lo, hi;
    asm volatile("lfence\n\trdtsc" : "=a"(lo), "=d"(hi)::"memory");
    return ((CYCLES)hi << 32) | lo;
}

#endif // UTIL_H_
