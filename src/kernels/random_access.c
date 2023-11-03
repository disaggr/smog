/*
 * Copyright (c) 2022 - 2023 OSM Group @ HPI, University of Potsdam
 */

#include "kernels/random_access.h"

#include "../smog.h"
#include "../util.h"

int random_access_init(struct smog_thread *thread) {
    unsigned int seed = thread->seed;

    struct buffer_element *elements = thread->slice.elements;
    size_t nelements = thread->slice.nelements;

    for (size_t i = 0; i < nelements; i++)
        for (size_t j = 0; j < PARALLEL_ACCESSES; j++)
            elements[i].randoms[j] = rand_r(&seed);

    return 0;
}

void random_read_run(struct smog_thread *thread) {
    // cast to volatile pointer and dereference instructs the compiler to assume that
    // load and store accesses to that variable have side effects, which forces it to
    // emit a load instruction regardless of optimization levels.
    volatile struct buffer_element *elements = thread->slice.elements;
    size_t limit = thread->slice.nelements;

    struct thread_status *status = thread->status;

    while (1) {
        for (uint64_t i = 0; i < limit; i++) {
            *(elements + elements[i].randoms[0] % limit);
            *(elements + elements[i].randoms[1] % limit);
            *(elements + elements[i].randoms[2] % limit);
            *(elements + elements[i].randoms[3] % limit);

            status->count += 4;

            volatile uint64_t delay = 0;
            for (size_t j = 0; j < thread->delay; j++) {
                delay += 1;
            }
        }
    }
}

void random_read_run_unhinged(struct smog_thread *thread) {
    // cast to volatile pointer and dereference instructs the compiler to assume that
    // load and store accesses to that variable have side effects, which forces it to
    // emit a load instruction regardless of optimization levels.
    volatile struct buffer_element *elements = thread->slice.elements;
    size_t limit = thread->slice.nelements;

    struct thread_status *status = thread->status;

    while (1) {
        for (uint64_t i = 0; i < limit; i++) {
            *(elements + elements[i].randoms[0] % limit);
            *(elements + elements[i].randoms[1] % limit);
            *(elements + elements[i].randoms[2] % limit);
            *(elements + elements[i].randoms[3] % limit);

            status->count += 4;
        }
    }
}

void random_write_run(struct smog_thread *thread) {
    // cast to volatile pointer and dereference instructs the compiler to assume that
    // load and store accesses to that variable have side effects, which forces it to
    // emit a load instruction regardless of optimization levels.
    volatile struct buffer_element *elements = thread->slice.elements;
    size_t limit = thread->slice.nelements;

    struct thread_status *status = thread->status;

    while (1) {
        for (uint64_t i = 0; i < limit; i++) {
            (elements + elements[i].randoms[0] % limit)->scratch = i;
            (elements + elements[i].randoms[1] % limit)->scratch = i;
            (elements + elements[i].randoms[2] % limit)->scratch = i;
            (elements + elements[i].randoms[3] % limit)->scratch = i;

            status->count += 4;

            volatile uint64_t delay = 0;
            for (size_t j = 0; j < thread->delay; j++) {
                delay += 1;
            }
        }
    }
}

void random_write_run_unhinged(struct smog_thread *thread) {
    // cast to volatile pointer and dereference instructs the compiler to assume that
    // load and store accesses to that variable have side effects, which forces it to
    // emit a load instruction regardless of optimization levels.
    volatile struct buffer_element *elements = thread->slice.elements;
    size_t limit = thread->slice.nelements;

    struct thread_status *status = thread->status;

    while (1) {
        for (uint64_t i = 0; i < limit; i++) {
            (elements + elements[i].randoms[0] % limit)->scratch = i;
            (elements + elements[i].randoms[1] % limit)->scratch = i;
            (elements + elements[i].randoms[2] % limit)->scratch = i;
            (elements + elements[i].randoms[3] % limit)->scratch = i;

            status->count += 4;
        }
    }
}
