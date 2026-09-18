#include "common.h"
#include "utility.h"
#include <stdint.h>

#define LINE_SIZE 64
#define L1_SIZE 32 * 1024
#define L2_SIZE 256 * 1024
#define L3_SIZE 16 * 1024 * 1024
#define BUFF_SIZE 8 * L2_SIZE

int main() {

    // create 4 arrays to store the latency numbers
    // the arrays are initialized to 0
    uint64_t dram_latency[SAMPLES] = {0};
    uint64_t l1_latency[SAMPLES] = {0};
    uint64_t l2_latency[SAMPLES] = {0};
    uint64_t l3_latency[SAMPLES] = {0};

    // A temporary variable we can use to load addresses
    uint8_t tmp;

    // Allocate a buffer of LINE_SIZE Bytes
    // The volatile keyword tells the compiler to not put this variable into a
    // register -- it should always try to be loaded from memory / cache.
    volatile uint8_t *target_buffer = (uint8_t *)malloc(LINE_SIZE);

    if (NULL == target_buffer) {
        perror("Unable to malloc");
        return EXIT_FAILURE;
    }

    target_buffer[0] = 123;

    volatile uint8_t *eviction_buffer = (uint8_t *)malloc(BUFF_SIZE);

    // Measure L1 access latency, store results in l1_latency array
    for (int i = 0; i < SAMPLES; i++) {
        // bring the target cache line into L1
        tmp = target_buffer[0];

        l1_latency[i] = measure_one_block_access_time((uint64_t)target_buffer);
    }

    // Measure DRAM Latency, store results in dram_latency array
    for (int i = 0; i < SAMPLES; i++) {
        // flush the target cache line
        clflush((uint64_t)target_buffer);

        dram_latency[i] =
            measure_one_block_access_time((uint64_t)target_buffer);
    }

    // Measure L2 Latency, store results in l2_latency array
    for (int i = 0; i < SAMPLES; i++) {
        // bring the target cache line into L1
        tmp = target_buffer[0];

        // evict the target cache line to L2
        for (int j = 0; j < 8; j++) {
            for (int l = 0; l < 4 * L1_SIZE; l += LINE_SIZE) {
                tmp += eviction_buffer[l]++;
            }
        }

        l2_latency[i] = measure_one_block_access_time((uint64_t)target_buffer);
    }

    // Measure L3 Latency, store results in l3_latency array
    for (int i = 0; i < SAMPLES; i++) {
        // bring the target cache line into L1
        tmp = target_buffer[0];

        // evict the target cache line into L3
        for (int j = 0; j < 8; j++) {
            for (int l = 0; l < BUFF_SIZE; l += LINE_SIZE) {
                tmp += eviction_buffer[l]++;
            }
        }

        l3_latency[i] = measure_one_block_access_time((uint64_t)target_buffer);
    }

    print_results_plaintext(dram_latency, l1_latency, l2_latency, l3_latency);

    free((uint8_t *)target_buffer);
    free((uint8_t *)eviction_buffer);
    return 0;
}
