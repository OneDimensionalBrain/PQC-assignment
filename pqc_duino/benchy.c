// This is for verification that nothing breaks. canaries add
// a little overhead, since each canary is 8 bytes and you have
// to add these 8 bytes to each array you made, so I guess each
// array has two canaries. This file tests for aligned data.
// The code that ran on the embedded system does not use this
// method.
#define PQCLEAN_NAMESPACE PQCLEAN_MLKEM512_CLEAN

# include <Arduino.h>

#include "api.h"
// #include "randombytes.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#ifndef NTESTS
#define NTESTS 100
#endif

// https://stackoverflow.com/a/1489985/1711232
#define PASTER(x, y) x##_##y
#define EVALUATOR(x, y) PASTER(x, y)
#define NAMESPACE(fun) EVALUATOR(PQCLEAN_NAMESPACE, fun)

#define CRYPTO_BYTES           NAMESPACE(CRYPTO_BYTES)
#define CRYPTO_PUBLICKEYBYTES  NAMESPACE(CRYPTO_PUBLICKEYBYTES)
#define CRYPTO_SECRETKEYBYTES  NAMESPACE(CRYPTO_SECRETKEYBYTES)
#define CRYPTO_CIPHERTEXTBYTES NAMESPACE(CRYPTO_CIPHERTEXTBYTES)
#define CRYPTO_ALGNAME NAMESPACE(CRYPTO_ALGNAME)

#define crypto_kem_keypair NAMESPACE(crypto_kem_keypair)
#define crypto_kem_enc NAMESPACE(crypto_kem_enc)
#define crypto_kem_dec NAMESPACE(crypto_kem_dec)

#define RETURNS_ZERO(f)                           \
    if ((f) != 0) {                               \
        puts(#f " returned non-zero returncode"); \
        res = 1;                                  \
        goto end;                                 \
    }

// https://stackoverflow.com/a/55243651/248065
#define MY_TRUTHY_VALUE_X 1
#define CAT(x,y) CAT_(x,y)
#define CAT_(x,y) x##y
#define HAS_NAMESPACE(x) CAT(CAT(MY_TRUTHY_VALUE_,CAT(PQCLEAN_NAMESPACE,CAT(_,x))),X)

#if !HAS_NAMESPACE(API_H)
#error "namespace not properly defined for header guard"
#endif

void setup_teensy_timer() {
    // Enable the Cycle Counter
    ARM_DEMCR |= ARM_DEMCR_TRCENA;
    ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA;
}

uint32_t get_cycles() {
    return ARM_DWT_CYCCNT;
}

static int test_keys(void) {
    /*
     * This is most likely going to be aligned by the compiler.
     */
    int res = 0;

    static uint8_t key_a  [CRYPTO_BYTES]           __attribute__((aligned(32)));
    static uint8_t key_b  [CRYPTO_BYTES]           __attribute__((aligned(32)));
    static uint8_t pk     [CRYPTO_PUBLICKEYBYTES]  __attribute__((aligned(32)));
    static uint8_t sendb  [CRYPTO_CIPHERTEXTBYTES] __attribute__((aligned(32)));
    static uint8_t sk_a   [CRYPTO_SECRETKEYBYTES]  __attribute__((aligned(32)));

    int i;

    static uint32_t t0[NTESTS][3], t1[NTESTS][3];

    t0[0][0] = get_cycles();
    t1[0][0] = get_cycles();
    uint32_t overhead = t1[0][0] - t0[0][0];

    printf("overhead: %" PRIu32 "\n", overhead);

    for (i = 0; i < NTESTS; i++) {
        setup_teensy_timer();

        // Alice generates a public key
        t0[i][0] = get_cycles();
        RETURNS_ZERO(crypto_kem_keypair(pk, sk_a));
        t1[i][0] = get_cycles();

        t0[i][1] = get_cycles();
        // Bob derives a secret key and creates a response
        RETURNS_ZERO(crypto_kem_enc(sendb, key_b, pk));
        t1[i][1] = get_cycles();

        // Alice uses Bobs response to get her secret key
        t0[i][2] = get_cycles();
        RETURNS_ZERO(crypto_kem_dec(key_a, sendb, sk_a));
        t1[i][2] = get_cycles();

        if (memcmp(key_a, key_b, CRYPTO_BYTES) != 0) {
            printf("ERROR KEYS\n");
            res = 1;
            goto end;
        }
    }

    for (i = 0; i < NTESTS; i++) {
        printf("%d, \t%" PRIu32 ", \t%" PRIu32 ", \t%" PRIu32 "\n",
               i,
               (t1[i][0] - t0[i][0]), // Keypair
               (t1[i][1] - t0[i][1]), // Encaps
               (t1[i][2] - t0[i][2])  // Decaps
        );
    }

end:
    return res;
}

void run_benchmarks(void) {
    puts(CRYPTO_ALGNAME);

    setup_teensy_timer();

    // Check if CRYPTO_ALGNAME is printable
    printf("Starting Benchmarks for: %s\n", CRYPTO_ALGNAME);
    printf("Iteration, Keypair, Encaps, Decaps\n");

    int result = test_keys();

    if (result != 0) {
        printf("Errors occurred during benchmarking\n");
    }
}
