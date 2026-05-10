#include <stddef.h>

extern int store_printf(const char *format, ...);

// PQClean occasionally calls malloc/free for certain buffers.
// In bare-metal, we return NULL to signal failure, or you can
// implement a simple static allocator if the code absolutely requires it.
// void *malloc(size_t size) {
//     return NULL;
// }

// void free(void *ptr) {
//     // Nothing to do
// }

void exit(int status) {
    store_printf("exit() called with status %d - halting\n", status);
    while (1);
}

// Dummy printf/puts for benchy.c to satisfy the linker
// Circle uses its own logging, so these just prevent linker errors.
// int printf(const char *format, ...) { return 0; }
// int puts(const char *s) { return 0; }
