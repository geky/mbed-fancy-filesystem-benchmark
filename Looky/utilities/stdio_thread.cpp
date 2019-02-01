#include <stdarg.h>
#include <stdio.h>
#include "mbed.h"

#include "stdio_thread.h"

Mutex printf_mutex;

int safe_printf(const char *format, ...) {

    printf_mutex.lock();

    va_list args;
    va_start(args, format);
    int num_bytes = vprintf(format, args);

    va_end(args);
    printf_mutex.unlock();

    return num_bytes;
}
