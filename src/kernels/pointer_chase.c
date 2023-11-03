/*
 * Copyright (c) 2022 - 2023 OSM Group @ HPI, University of Potsdam
 */

#include "kernels/pointer_chase.h"

#include "../smog.h"
#include "../util.h"

int pointer_chase_init(struct smog_thread *thread) {
    unsigned int seed = thread->seed;

    struct buffer_element *elements = thread->slice.elements;
    size_t nelements = thread->slice.nelements;

    // initialize doubly-linked list of cache lines
    elements[0].prev = &elements[nelements - 1];
    elements[0].next = &elements[1];
    elements[nelements - 1].prev = &elements[nelements - 2];
    elements[nelements - 1].next = &elements[0];
    for (size_t i = 1; i < nelements - 1; i++) {
        elements[i].prev = &elements[i - 1];
        elements[i].next = &elements[i + 1];
    }

    // fisher-yates shuffle
    for (uint64_t i = 0; i < nelements - 2; ++i) {
        uint64_t j = i + rand_r(&seed) % (nelements - i);
        struct buffer_element *a = &elements[i];
        struct buffer_element *b = &elements[j];
        a->prev->next = b;
        a->next->prev = b;
        b->prev->next = a;
        b->next->prev = a;
        struct buffer_element *t = a->next;
        a->next = b->next;
        b->next = t;
        t = a->prev;
        a->prev = b->prev;
        b->prev = t;
    }

    return 0;
}

void pointer_chase_run(struct smog_thread *thread) {
    struct thread_status *status = thread->status;

    // cast to volatile pointer to instruct the compiler to assume that load and store accesses
    // to that variable have side effects, which forces it to emit a load instructions during the
    // pointer chasing regardless of optimization levels.
    volatile struct buffer_element *tmp = thread->slice.elements;

    while (1) {
        tmp = tmp->next;
        status->count++;

        volatile uint64_t delay = 0;
        for (size_t j = 0; j < thread->delay; j++) {
            delay += 1;
        }
    }
}

void pointer_chase_run_unhinged(struct smog_thread *thread) {
    struct thread_status *status = thread->status;

    // cast to volatile pointer to instruct the compiler to assume that load and store accesses
    // to that variable have side effects, which forces it to emit a load instructions during the
    // pointer chasing regardless of optimization levels.
    volatile struct buffer_element *tmp = thread->slice.elements;

    while (1) {
        // also, manual loop unrolling.
        REP(0, 5, 1, 2, tmp = tmp->next;)
        status->count += 512;
    }
}
