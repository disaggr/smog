/*
 * Copyright (c) 2022 - 2023 OSM Group @ HPI, University of Potsdam
 */

#pragma once

#include <pthread.h>
#include <argp.h>

// globals
extern size_t g_smog_pagesize;
extern size_t g_smog_delay;
extern size_t g_smog_timeout;
extern pthread_barrier_t g_initalization_finished;

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

struct thread_status_t {
    union {
        struct {
            size_t count;
            size_t last_count;
        };
        char padding[CACHE_LINE_SIZE];
    };
};

extern size_t g_thread_count;
extern struct thread_status_t *g_thread_status;
extern struct thread_options *g_thread_options;
extern pthread_t *g_threads;

struct thread_options {
    int tid;
    void *slice_start;
    size_t slice_length;
};

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
