// This is a benchmarking file. For aligned tests

// #define PQCLEAN_NAMESPACE PQCLEAN_KYBER512_CLEAN

// # include <Arduino.h>

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
        printf(#f " returned non-zero returncode"); \
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
        #include "core_cm7.h" // Or equivalent for your specific chip, I'll replace it later with the relevant one when it is ready

        // Manual DWT register addresses if headers aren't available
        #define K_DWT_CTRL      (*(volatile uint32_t*)0xE0001000)
        #define K_DWT_CYCCNT    (*(volatile uint32_t*)0xE0001004)
        #define K_DEMCR         (*(volatile uint32_t*)0xE000EDFC)
        #define K_TRCENA        (1UL << 24)
        #define K_CYCCNTENA     (1UL << 0)

        #define SCOPE_HIGH()  do {  } while (0)
        #define SCOPE_LOW()   do {  } while (0)

        void paint_stack() {}
        uint32_t measure_stack() {}
        static inline void set_pmu_event_pmn0(uint32_t event) {}
        static inline void set_pmu_event_pmn1(uint32_t event) {}
        static inline uint32_t read_pmn0(void) {}
        static inline uint32_t read_pmn1(void) {}

        void where_am_i() {
            printf("Running on an stm32 or a teensy!\n");
        }

        void setup_timer() {
            K_DEMCR |= K_TRCENA;
            K_DWT_CYCCNT = 0;
            K_DWT_CTRL |= K_CYCCNTENA;
        }

        uint32_t get_cycles() {
            return (uint64_t)K_DWT_CYCCNT;
        }

    #else
        // Raspberry Pi / Linux ARM (Cortex-A)
        // Cycles are restricted in userspace; use clock_gettime for nanoseconds
        #include <time.h>
        // static CGPIOPin *g_pScopePin = 0;

        extern void scope_high(void);
        extern void scope_low(void);
        extern uint64_t get_system_micros(void);
        extern int store_printf(const char *format, ...);

        #define STACK_PAINT 0xDEADBEEFU
        #define STACK_CHECK_SIZE 0x8000  // 32KB — matches KERNEL_STACK_SIZE

        void paint_stack(void) {
            uint32_t sp;
            __asm__ volatile ("mov %0, sp" : "=r"(sp));

            // Paint below current SP
            uint32_t *p = (uint32_t *)(sp - STACK_CHECK_SIZE);
            uint32_t *end = (uint32_t *)sp;
            while (p < end) *p++ = STACK_PAINT;

            // Verify painting worked
            // p = (uint32_t *)(sp - STACK_CHECK_SIZE);
            // uint32_t failures = 0;
            // while (p < end) {
            //     if (*p != STACK_PAINT) failures++;
            //     p++;
            // }

            // if (failures == 0)
            //     store_printf("Stack painted OK (%u bytes)\n", STACK_CHECK_SIZE);
            // else
            //     store_printf("Stack paint FAILED: %u words corrupted\n", failures);
        }

        uint32_t measure_stack(void) {
            uint32_t sp;
            __asm__ volatile ("mov %0, sp" : "=r"(sp));

            uint32_t *p = (uint32_t *)(sp - STACK_CHECK_SIZE);
            while (*p == STACK_PAINT) p++;

            uint32_t used = (uint32_t)((sp - STACK_CHECK_SIZE) +
                            STACK_CHECK_SIZE - (uint32_t)p);
            // store_printf("Peak stack usage: %u bytes\n", used);
            return used;
        }


        #define SCOPE_HIGH()  scope_high()
        #define SCOPE_LOW()   scope_low()
        #define printf(...)   store_printf(__VA_ARGS__)

        void where_am_i() {
            printf("Running on a Pi!\n");
        }

        // yeah yeah I know it is supposed to be start_pmu but I'm not in the mood to
        // make code syntax specific and keep it compatible at the same time.
        void setup_timer() {
            // ARM1176JZF-S PMU - c15 based, NOT c9 like Cortex-A
            uint32_t val;
            __asm__ volatile (
                "mrc p15, 0, %0, c15, c12, 0\n\t"  // read PMNC
                "orr %0, %0, #7\n\t"               // E|P|C: enable + reset both counters
                "mcr p15, 0, %0, c15, c12, 0\n\t"  // write PMNC
                : "=r"(val)
            );
        }

        static inline uint32_t get_cycles() {
            uint32_t cc;
            __asm__ volatile (
                "mrc p15, 0, %0, c15, c12, 1\n\t"  // read CCNT
                : "=r"(cc)
            );
            return (uint64_t)cc;
        }

        static inline void set_pmu_event_pmn0(uint32_t event) {
            __asm__ volatile ("mcr p15, 0, %0, c15, c12, 5" : : "r"(event));
        }
        static inline void set_pmu_event_pmn1(uint32_t event) {
            __asm__ volatile ("mcr p15, 0, %0, c15, c12, 6" : : "r"(event));
        }
        static inline uint32_t read_pmn0(void) {
            uint32_t v;
            __asm__ volatile ("mrc p15, 0, %0, c15, c12, 2" : "=r"(v));
            return v;
        }
        static inline uint32_t read_pmn1(void) {
            uint32_t v;
            __asm__ volatile ("mrc p15, 0, %0, c15, c12, 3" : "=r"(v));
            return v;
        }

        // static inline uint64_t get_cycles() {
        //     return get_system_micros(); // Now returning microseconds
        // }

    #endif
#else
    // Generic fallback for x86_64 (Laptop/Desktop)
    #include <x86intrin.h>
    #define SCOPE_HIGH()  do {  } while (0)
    #define SCOPE_LOW()   do {  } while (0)

    void paint_stack() {}
    uint32_t measure_stack() {}
    static inline void set_pmu_event_pmn0(uint32_t event) {}
    static inline void set_pmu_event_pmn1(uint32_t event) {}
    static inline uint32_t read_pmn0(void) {}
    static inline uint32_t read_pmn1(void) {}

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

    static uint8_t key_a  [CRYPTO_BYTES]           __attribute__((aligned(32)));
    static uint8_t key_b  [CRYPTO_BYTES]           __attribute__((aligned(32)));
    static uint8_t pk     [CRYPTO_PUBLICKEYBYTES]  __attribute__((aligned(32)));
    static uint8_t sendb  [CRYPTO_CIPHERTEXTBYTES] __attribute__((aligned(32)));
    static uint8_t sk_a   [CRYPTO_SECRETKEYBYTES]  __attribute__((aligned(32)));

    int i;

    static uint32_t t0[NTESTS][3], t1[NTESTS][3];
    static uint32_t keypair_stack[NTESTS], encaps_stack[NTESTS], decaps_stack[NTESTS];

    // these store cache misses. first field for iteration, the run itself. Second field says it stores
    // an element for each cryptographic function. Third field specifies if it is to keep data centered
    // so cache_misses_read[i][func][1] - cache_misses_read[i][func][0] will be executed.
    static uint32_t cache_misses_read[NTESTS][3][2];
    static uint32_t cache_misses_write[NTESTS][3][2];

    t0[0][0] = get_cycles();
    t1[0][0] = get_cycles();
    uint32_t overhead = t1[0][0] - t0[0][0];

    printf("overhead: %" PRIu32 "\n", overhead);

    set_pmu_event_pmn0(0x0B); // PMN0 counts data cache read misses
    set_pmu_event_pmn1(0x0D); // PMN1 counts data cache write misses
    setup_timer();
    for (i = 0; i < NTESTS; i++) {
        // Alice generates a public key
        SCOPE_HIGH();
        paint_stack();
        cache_misses_read[i][0][0] = read_pmn0();
        cache_misses_write[i][0][0] = read_pmn1();
        t0[i][0] = get_cycles();
        RETURNS_ZERO(crypto_kem_keypair(pk, sk_a));
        t1[i][0] = get_cycles();
        keypair_stack[i] = measure_stack();
        cache_misses_read[i][0][1] = read_pmn0();
        cache_misses_write[i][0][1] = read_pmn1();
        SCOPE_LOW();

        // Bob derives a secret key and creates a response
        SCOPE_HIGH();
        paint_stack();
        cache_misses_read[i][1][0] = read_pmn0();
        cache_misses_write[i][1][0] = read_pmn1();
        t0[i][1] = get_cycles();
        RETURNS_ZERO(crypto_kem_enc(sendb, key_b, pk));
        t1[i][1] = get_cycles();
        encaps_stack[i] = measure_stack();
        cache_misses_read[i][1][1] = read_pmn0();
        cache_misses_write[i][1][1] = read_pmn1();
        SCOPE_LOW();

        // Alice uses Bobs response to get her secret key
        SCOPE_HIGH();
        paint_stack();
        cache_misses_read[i][2][0] = read_pmn0();
        cache_misses_write[i][2][0] = read_pmn1();
        t0[i][2] = get_cycles();
        RETURNS_ZERO(crypto_kem_dec(key_a, sendb, sk_a));
        // printf("successful run 3\n");
        t1[i][2] = get_cycles();
        decaps_stack[i] = measure_stack();
        cache_misses_read[i][2][1] = read_pmn0();
        cache_misses_write[i][2][1] = read_pmn1();
        SCOPE_LOW();

        // printf("compare stage\n");
        if (memcmp(key_a, key_b, CRYPTO_BYTES) != 0) {
            printf("ERROR KEYS\n");
            res = 1;
            goto end;
        }
        // printf("nothing ever happens\n");
    }

    printf("Iteration, Keypair cycles, Keypair stack, Keypair read cache miss, Keypair write cache miss, \
Encaps cycles, Encaps stack, Encaps read cache miss, Encaps write cache miss, \
Decaps cycles, Decaps stack, Decaps read cache miss, Decaps write cache miss\n");
    for (i = 0; i < NTESTS; i++) {
        printf("%d, \t%" PRIu32 ", \t%" PRIu32 ", \t%" PRIu32
                ", \t%" PRIu32 ", \t%" PRIu32 ", \t%" PRIu32
                ", \t%" PRIu32 ", \t%" PRIu32 ", \t%" PRIu32
                ", \t%" PRIu32 ", \t%" PRIu32 ", \t%" PRIu32 "\n",
               i,
               (t1[i][0] - t0[i][0]), keypair_stack[i], (cache_misses_read[i][0][1] - cache_misses_read[i][0][0]), (cache_misses_write[i][0][1] - cache_misses_write[i][0][0]),// Keypair
               (t1[i][1] - t0[i][1]), encaps_stack[i], (cache_misses_read[i][1][1] - cache_misses_read[i][1][0]), (cache_misses_write[i][1][1] - cache_misses_write[i][1][0]),// Encaps
               (t1[i][2] - t0[i][2]), decaps_stack[i], (cache_misses_read[i][2][1] - cache_misses_read[i][2][0]), (cache_misses_write[i][2][1] - cache_misses_write[i][2][0])// Decaps

        );
    }

end:
    return res;
}

int run_benchmarks(void) {
    printf("%s\n", CRYPTO_ALGNAME);

    where_am_i();

    setup_timer();

    // Check if CRYPTO_ALGNAME is printable
    printf("Starting Benchmarks for: %s\n", CRYPTO_ALGNAME);
    // I moved it up

    int result = test_keys();

    if (result != 0) {
        printf("Errors occurred during benchmarking\n");
    }

    return result;
}

// int main() {
//     return run_benchmarks();
// }
