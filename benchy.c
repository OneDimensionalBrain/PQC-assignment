#include "api.h"
#include "randombytes.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

inline static void *malloc_s(size_t size) {
    void *ptr = malloc(size);
    if (ptr == NULL) {
        perror("Malloc failed!");
        exit(1);
    }
    return ptr;
}

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

static void test_keys(void) {
    /*
     * This is most likely going to be aligned by the compiler.
     * 16 extra bytes for canary
     * 1 extra byte for unalignment
     */

    uint8_t res = 0;

    uint8_t *key_a = malloc_s(CRYPTO_BYTES + 16 + 1);
    uint8_t *key_b = malloc_s(CRYPTO_BYTES + 16 + 1);
    uint8_t *pk    = malloc_s(CRYPTO_PUBLICKEYBYTES + 16 + 1);
    uint8_t *sendb = malloc_s(CRYPTO_CIPHERTEXTBYTES + 16 + 1);
    uint8_t *sk_a  = malloc_s(CRYPTO_SECRETKEYBYTES + 16 + 1);

    // Alice generates a public key
    RETURNS_ZERO(crypto_kem_keypair(pk + 8, sk_a + 8));

    // Bob derives a secret key and creates a response
    RETURNS_ZERO(crypto_kem_enc(sendb + 8, key_b + 8, pk + 8));

    // Alice uses Bobs response to get her secret key
    RETURNS_ZERO(crypto_kem_dec(key_a + 8, sendb + 8, sk_a + 8));
end:
    free(key_a);
    free(key_b);
    free(pk);
    free(sendb);
    free(sk_a);
}

int main(void) {
    // Check if CRYPTO_ALGNAME is printable
    puts(CRYPTO_ALGNAME);

    //int result = 0;
    test_keys();

    return 0;
}
