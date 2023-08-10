#include <sys/time.h>
#include <stdio.h>
#include "util.h"

int64_t micros() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000000 + tv.tv_usec;
}