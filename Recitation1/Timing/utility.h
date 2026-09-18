#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>

#ifndef __UTILITY_H__
#define __UTILITY_H__

#define SAMPLES 100

// Supporting functions for printing results in different formats
// Function "compare" is used in the priting functions and you do not need it
int compare(const void *p1, const void *p2) {
    uint64_t u1 = *(uint64_t *)p1;
    uint64_t u2 = *(uint64_t *)p2;

    return (int)u1 - (int)u2;
}

// Print out the latencies you measured
void print_results_plaintext(uint64_t* dram, uint64_t* l1, uint64_t* l2, uint64_t* l3) {
    qsort(dram, SAMPLES, sizeof(uint64_t), compare);
    qsort(l1, SAMPLES, sizeof(uint64_t), compare);
    qsort(l2, SAMPLES, sizeof(uint64_t), compare);
    qsort(l3, SAMPLES, sizeof(uint64_t), compare);
    printf("             :  L1   L2   L3   Mem   \n");
    printf("Minimum      : %5ld %5ld %5ld %5ld\n", l1[0], l2[0], l3[0], dram[0]);

    printf("Bottom decile: %5ld %5ld %5ld %5ld\n", l1[SAMPLES/10], l2[SAMPLES/10],
                                                l3[SAMPLES/10], dram[SAMPLES/10]);

    printf("Median       : %5ld %5ld %5ld %5ld\n", l1[SAMPLES/2], l2[SAMPLES/2],
                                                l3[SAMPLES/2], dram[SAMPLES/2]);

    printf("Top decile   : %5ld %5ld %5ld %5ld\n", l1[(SAMPLES * 9)/10], l2[(SAMPLES * 9)/10],
                                                   l3[(SAMPLES * 9)/10], dram[(SAMPLES * 9)/10]);

    printf("Maximum      : %5ld %5ld %5ld %5ld\n", l1[SAMPLES-1], l2[SAMPLES-1],
                                                    l3[SAMPLES-1], dram[SAMPLES-1]);
}

#endif // _UTILITY_H__ 
