// This is a benchmarking file. Tests time for misaligned

// change this line for a specific implementation
// #define PQCLEAN_NAMESPACE PQCLEAN_KYBER512_CLEAN

#include "api.h"
#include "randombytes.h"

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

#if defined(__arm__) || defined(__thumb__) || defined(_M_ARM)
    #if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__) || defined(__ARM_ARCH_8M_MAIN__)
        // CORTEX-M (Teensy, STM32)
        #include "core_cm7.h" // Or equivalent for your specific chip

        // Manual DWT register addresses if headers aren't available
        #define K_DWT_CTRL      (*(volatile uint32_t*)0xE0001000)
        #define K_DWT_CYCCNT    (*(volatile uint32_t*)0xE0001004)
        #define K_DEMCR         (*(volatile uint32_t*)0xE000EDFC)
        #define K_TRCENA        (1UL << 24)
        #define K_CYCCNTENA     (1UL << 0)

        void where_am_i() {
            printf("Running on an stm32 or a teensy!");
        }

        void setup_timer() {
            K_DEMCR |= K_TRCENA;
            K_DWT_CYCCNT = 0;
            K_DWT_CTRL |= K_CYCCNTENA;
        }

        uint64_t get_cycles() {
            return (uint64_t)K_DWT_CYCCNT;
        }

    #else
        // Raspberry Pi / Linux ARM (Cortex-A)
        // Cycles are restricted in userspace; use clock_gettime for nanoseconds
        #include <time.h>

        void where_am_i() {
            printf("Running on a Pi!");
        }

        void setup_timer() {
            // Nothing to setup for POSIX clock
        }

        // uint64_t get_cycles() {
        //     struct timespec ts;
        //     clock_gettime(CLOCK_MONOTONIC, &ts);
        //     return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
        // }

        static inline uint32_t get_cycles() {
            uint32_t val;
            // Read Performance Monitor Count Register
            asm volatile("mrc p15, 0, %0, c15, c12, 1" : "=r"(val));
            return val;
        }
    #endif
#else
    // Generic fallback for x86_64 (Laptop/Desktop)
    #include <x86intrin.h>
    void where_am_i() {
        printf("Running on generic device!");
    }
    void setup_timer() {}
    uint64_t get_cycles() {
        return __rdtsc();
    }
#endif

static int test_keys(void) {
    /*
     * This is most likely going to be aligned by the compiler.
     */
    int res = 0;

    static uint8_t key_a  [CRYPTO_BYTES + 1]           __attribute__((aligned(32)));
    static uint8_t key_b  [CRYPTO_BYTES + 1]           __attribute__((aligned(32)));
    static uint8_t pk     [CRYPTO_PUBLICKEYBYTES + 1]  __attribute__((aligned(32)));
    static uint8_t sendb  [CRYPTO_CIPHERTEXTBYTES + 1] __attribute__((aligned(32)));
    static uint8_t sk_a   [CRYPTO_SECRETKEYBYTES + 1]  __attribute__((aligned(32)));

    uint8_t *key_a_mis  = key_a + 1;
    uint8_t *key_b_mis  = key_b + 1;
    uint8_t *pk_mis     = pk + 1;
    uint8_t *sendb_mis  = sendb + 1;
    uint8_t *sk_a_mis   = sk_a + 1;

    printf("Buffer Address: %p\n", (void*)pk_mis);
    if (((uintptr_t)pk_mis % 4) != 0) {
        printf("Confirmed: Buffer is NOT 4-byte aligned.\n");
    }

    int i;

    static uint32_t t0[NTESTS][3], t1[NTESTS][3];

    t0[0][0] = get_cycles();
    t1[0][0] = get_cycles();
    uint32_t overhead = t1[0][0] - t0[0][0];

    printf("overhead: %" PRIu32 "\n", overhead);

    for (i = 0; i < NTESTS; i++) {
        setup_timer();

        // Alice generates a public key
        t0[i][0] = get_cycles();
        RETURNS_ZERO(crypto_kem_keypair(pk_mis, sk_a_mis));
        t1[i][0] = get_cycles();

        t0[i][1] = get_cycles();
        // Bob derives a secret key and creates a response
        RETURNS_ZERO(crypto_kem_enc(sendb_mis, key_b_mis, pk_mis));
        t1[i][1] = get_cycles();

        // Alice uses Bobs response to get her secret key
        t0[i][2] = get_cycles();
        RETURNS_ZERO(crypto_kem_dec(key_a_mis, sendb_mis, sk_a_mis));
        t1[i][2] = get_cycles();

        if (memcmp(key_a_mis, key_b_mis, CRYPTO_BYTES) != 0) {
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

int run_benchmarks(void) {
    puts(CRYPTO_ALGNAME);

    where_am_i();

    setup_timer();

    // Check if CRYPTO_ALGNAME is printable
    printf("Starting Benchmarks for: %s\n", CRYPTO_ALGNAME);
    printf("Iteration, Keypair, Encaps, Decaps\n");

    int result = test_keys();

    if (result != 0) {
        printf("Errors occurred during benchmarking\n");
    }

    return result;
}

int main(void) {
    return run_benchmarks();
}
