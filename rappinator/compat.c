#include <stddef.h>

extern int store_printf(const char *format, ...);

void exit(int status) {
    store_printf("exit() called with status %d - halting\n", status);
    while (1);
}
