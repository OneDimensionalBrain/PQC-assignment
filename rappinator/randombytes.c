#include "randombytes.h"
#include <stdint.h>
#include <stddef.h>

/**
 * For a university research project on PQC benchmarking,
 * we often want deterministic "randomness" to ensure
 * cycle counts are stable across runs.
 */
int randombytes(uint8_t *output, size_t n) {
    // This is a simple Xorshift PRNG or a static fill.
    // On the Pi Zero, you could also read from the hardware
    // TRNG at address 0x20104000, but for benchmarks,
    // a deterministic fill is often preferred.

    for (size_t i = 0; i < n; i++) {
        // 0x42 is the classic "answer to everything"
        // and keeps your ML-KEM math predictable for cycle counting.
        output[i] = 0x42;
    }

    return 0; // Success
}
