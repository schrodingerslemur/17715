#include "common.h"

/* Measure the time it takes to access a block with virtual address addr. */
CYCLES measure_one_block_access_time(ADDR_PTR addr) {
    CYCLES cycles;

    asm volatile(
        ".align 32\n\t"
        "lfence\n\t"
        "rdtsc\n\t"
        "shl $32, %%rdx\n\t"
        "or %%rdx, %%rax\n\t"
        "movq %%rax, %%r8\n\t"
        "mov (%1), %%rcx\n\t"
        "rdtscp\n\t"
        "shl $32, %%rdx\n\t"
        "or %%rdx, %%rax\n\t"
        "sub %%r8, %%rax\n\t"
        : "=&a"(cycles) /*output, early-clobber so addr never shares rax*/
        : "r"(addr)
        : "rcx", "rdx", "r8", "memory");

    return cycles;
}

/* CLFlushes the given address. */
void clflush(ADDR_PTR addr) { asm volatile("clflush (%0)" ::"r"(addr)); }
