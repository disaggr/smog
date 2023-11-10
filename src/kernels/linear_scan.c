/*
 * Copyright (c) 2022 - 2023 OSM Group @ HPI, University of Potsdam
 */

#include "kernels/linear_scan.h"

#include "../smog.h"
#include "../util.h"

void linear_scan_run(struct smog_thread *thread) {
    uint64_t *words = thread->slice.words;
    size_t limit = thread->slice.nwords;

    struct thread_status *status = thread->status;

    while (1) {
        for (size_t i = 0; i < limit; ++i) {
            // cast to volatile pointer and dereference instructs the compiler to assume that
            // load and store accesses to that variable have side effects, which forces it to
            // emit a load instruction regardless of optimization levels.
            *(volatile uint64_t*)(words + i);
            status->count++;

            volatile uint64_t delay = 0;
            for (size_t j = 0; j < thread->delay; j++) {
                delay += 1;
            }
        }
    }
}

void linear_scan_run_unhinged(struct smog_thread *thread) {
    uint64_t *words = thread->slice.words;
    size_t limit = thread->slice.nwords;

    struct thread_status *status = thread->status;

    while (1) {
        for (size_t i = 0; i < limit;) {
            // manual loop unrolling, same logic as above
            REP(0, 5, 1, 2, *(volatile uint64_t*)(words + (i++ % limit));)
            status->count += 512;
        }
    }
}
