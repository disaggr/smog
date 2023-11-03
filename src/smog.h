/*
 * Copyright (c) 2022 - 2023 OSM Group @ HPI, University of Potsdam
 */

#ifndef SMOG_H_
#define SMOG_H_

#include <stddef.h>

#if defined(__ppc64__) || defined(__PPC64__) || defined(__powerpc64__)
#  define mem_fence()  __asm__ __volatile__ ("lwsync" : : : "memory")
#  define CACHE_LINE_SIZE 128
#elif defined(__x86_64__)
#  define mem_fence()  __asm__ __volatile__ ("mfence" : : : "memory")
#  define CACHE_LINE_SIZE 64
#else
#  define mem_fence()
#  define CACHE_LINE_SIZE 64
#endif

enum output_format {
    PLAIN,
    CSV,
};

struct arguments {
    size_t pagesize;
    size_t monitor_interval;
    enum output_format output_format;
    const char *output_file;
    const char *smogfile;
    int verbose;
};

extern struct arguments arguments;

#endif  // SMOG_H_
