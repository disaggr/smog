/*
 * Copyright (c) 2022 - 2023 OSM Group @ HPI, University of Potsdam
 */

#include <sys/time.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

#include "./util.h"

long double get_unixtime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long double)tv.tv_sec+(long double)(tv.tv_usec)/1e6;
}

size_t parse_size_string(const char *s) {
    size_t len = strlen(s);

    switch (s[len - 1]) {
        case '0' ... '9':
            // return as literal number
            return strtoll(s, NULL, 0);
        case 'B':
            // we have some sort of bytes suffix to deal with, continue.
            len--;
            break;
        default:
            errno = EINVAL;
            return 0;
    }

    size_t multiplier = 1000;
    if (s[len - 1] == 'i') {
        multiplier = 1024;
        len--;
    }

    size_t unit = 1;

    switch (s[len -1]) {
        // NOTE THAT THE MISSING BREAK STATEMENTS ARE INTENTIONAL!
        case 'T':
            unit *= multiplier;
            // fall through
        case 'G':
            unit *= multiplier;
            // fall through
        case 'M':
            unit *= multiplier;
            // fall through
        case 'K':
            unit *= multiplier;
            len--;
            break;
        default:
            errno = EINVAL;
            return 0;
    }

    size_t base = strtoll(s, NULL, 0);

    return base * unit;
}

const char *format_size_string(size_t s) {
    static char *buffer = NULL;
    static size_t buflen = 0;
    static const char *units[] = { "Bytes", "KiB", "MiB", "GiB" };

    int unit = 0;
    while (unit < 4 && !(s % 1024)) {
        unit++;
        s /= 1024;
    }

    size_t n = snprintf(buffer, buflen, "%zu %s", s, units[unit]);
    if (n >= buflen) {
        buflen = n + 1;
        buffer = realloc(buffer, buflen);
        if (!buffer) {
            return NULL;
        }

        snprintf(buffer, buflen, "%zu %s", s, units[unit]);
    }

    return buffer;
}

