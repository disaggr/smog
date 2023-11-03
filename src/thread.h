/*
 * Copyright (c) 2022 - 2023 OSM Group @ HPI, University of Potsdam
 */

#ifndef THREAD_H_
#define THREAD_H_

#include <stdint.h>
#include <pthread.h>

#include "./smog.h"
#include "./buffer.h"
#include "./kerneltypes.h"

struct thread_status {
    union {
        struct {
            volatile size_t count;
            size_t last_count;
        };
        char padding[CACHE_LINE_SIZE];
    };
};

struct smog_thread {
    size_t tid;
    pthread_t thread;

    struct thread_status *status;

    enum kernel kernel;
    struct smog_buffer slice;

    volatile size_t delay;
    size_t target_rate;
    size_t adjust_timeout;
    int adjust_phase;

    pthread_barrier_t *init;
    unsigned int seed;

    int is_principal;
};

void* thread_run(void *arg);

#endif  // THREAD_H_
