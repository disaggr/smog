/*
 * Copyright (c) 2022 - 2023 OSM Group @ HPI, University of Potsdam
 */

#include "kernels/dirty_pages.h"

#include "../smog.h"

void dirty_pages_run(struct smog_thread *thread) {
    struct buffer_element *elements = thread->slice.elements;

    size_t limit = thread->slice.nelements;
    size_t stride = arguments.pagesize / sizeof(*elements);

    struct thread_status *status = thread->status;

    while (1) {
        for (size_t i = 0; i < limit; i += stride) {
            elements[i].scratch += 1;
            status->count += 1;

            volatile uint64_t delay = 0;
            for (size_t j = 0; j < thread->delay; j++) {
                delay += 1;
            }
        }
    }
}

void dirty_pages_run_unhinged(struct smog_thread *thread) {
    struct buffer_element *elements = thread->slice.elements;

    size_t limit = thread->slice.nelements;
    size_t stride = arguments.pagesize / sizeof(*elements);

    struct thread_status *status = thread->status;

    while (1) {
        for (size_t i = 0; i < limit; i += stride) {
            elements[i].scratch += 1;
            status->count += 1;
        }
    }
}
