/*
 * Copyright (c) 2022 - 2023 OSM Group @ HPI, University of Potsdam
 */

#include "./thread.h"

#include <stdio.h>
#include <pthread.h>

#include "./kernel.h"

static int thread_run_kernel(struct smog_thread *thread, struct smog_kernel *kernel) {
    // perform initializion on the slice, if necessary
    if (thread->is_principal && kernel->init) {
        int res = kernel->init(thread);
        if (res != 0) {
            fprintf(stderr, "error: kernel initialization failed\n");
            return 1;
        }
    }

    // ready to run, wait for the others
    pthread_barrier_wait(thread->init);

    // go
    if (thread->delay || thread->target_rate || !kernel->run_unhinged) {
        kernel->run(thread);
    } else {
        kernel->run_unhinged(thread);
    }

    // this should never be reached
    fprintf(stderr, "error: kernel ended prematurely\n");
    return 0;
}

// entry point of the kernel thread
void *thread_run(void *arg) {
    struct smog_thread *thread = arg;

    for (struct smog_kernel *k = smog_kernels; k->name != NULL; ++k)
        if (k->kernel == thread->kernel) {
            // this call only returns on error
            thread_run_kernel(thread, k);
            return NULL;
        }

    // error on fallthrough
    fprintf(stderr, "error: unrecognized kernel %s\n", kernel_to_string(thread->kernel));

    return NULL;
}
