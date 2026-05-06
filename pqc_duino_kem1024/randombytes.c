#include <stdint.h>
#include "randombytes.h"

int randombytes(uint8_t *out, size_t outlen) {
    for (size_t i = 0; i < outlen; i++) {
        out[i] = (uint8_t)i; // Dummy deterministic "random" for benchmarking
    }
    return 0;
}
