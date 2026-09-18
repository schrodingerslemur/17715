#ifndef COMMON_H_
#define COMMON_H_

// You may only use fgets() to pull input from stdin
// You may use any print function to stdout to print
// out chat messages
#include <stdio.h>

// You may use memory allocators and helper functions
// (e.g., rand()).  You may not use system().
#include <stdlib.h>

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#define ADDR_PTR uint64_t
#define CYCLES uint64_t

/* Measure the time it takes to access a block with virtual address addr. */
CYCLES measure_one_block_access_time(ADDR_PTR addr);

/* CLFlushes the given address. */
void clflush(ADDR_PTR addr);

#endif // COMMON_H_
