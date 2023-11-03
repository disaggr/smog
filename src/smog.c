/*
 * Copyright (c) 2022 - 2023 OSM Group @ HPI, University of Potsdam
 */

#include "./smog.h"

#include <unistd.h>

#include "./util.h"
#include "./args.h"
#include "./parser.h"
#include "./monitor.h"
#include "./buffer.h"
#include "./thread.h"
#include "./kernel.h"

// defaults for cli arguments
struct arguments arguments = { 0, 1000, PLAIN, NULL, NULL, 0 };

int main(int argc, char* argv[]) {
    // determine system characteristics
    size_t system_pages = sysconf(_SC_PHYS_PAGES);
    size_t system_pagesize = sysconf(_SC_PAGE_SIZE);
    size_t cache_line_size = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
    size_t hardware_concurrency = sysconf(_SC_NPROCESSORS_ONLN);

    // parse CLI options
    arguments.pagesize = system_pagesize;
    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    printf("SMOG dirty-page benchmark\n");
    printf("  System page size:       %s\n", format_size_string(system_pagesize));
    printf("  Cache Line size:        %s\n", format_size_string(cache_line_size));
    printf("  System physical pages:  %zu (%s)\n",
           system_pages, format_size_string(system_pages * system_pagesize));
    printf("  Hardware Concurrency:   %zu\n", hardware_concurrency);
    printf("\n");
    printf("  SMOG page size:         %s\n", format_size_string(arguments.pagesize));
    printf("  Monitor interval:       %zu  ms\n", arguments.monitor_interval);

    // sanity check: assert for correct cache line size
    if (cache_line_size != CACHE_LINE_SIZE) {
        fprintf(stderr, "error: mismatched cache line size: %#x Bytes. expected: %#zx Bytes\n",
                CACHE_LINE_SIZE, cache_line_size);
        return 2;
    }
    // sanity check: assert for correct structure padding
    if (sizeof(struct thread_status) != CACHE_LINE_SIZE) {
        fprintf(stderr, "error: mismatched thread_status: %#zx Bytes. expected: %#x Bytes\n",
                sizeof(struct thread_status), CACHE_LINE_SIZE);
        return 2;
    }
    if (sizeof(struct buffer_element) != CACHE_LINE_SIZE) {
        fprintf(stderr, "error: mismatched buffer_element: %#zx Bytes. expected: %#x Bytes\n",
                sizeof(struct buffer_element), CACHE_LINE_SIZE);
        return 2;
    }

    // parse smogfile
    struct yaml_config config = { };
    int res = yaml_parse(arguments.smogfile, &config);
    if (res != 0) {
        fprintf(stderr, "error: failed to parse %s.\n", arguments.smogfile);
        return res;
    }

    // initialize buffers
    printf("\n");
    printf("Creating %zu SMOG buffers\n", config.nbuffers);

    struct smog_buffer *buffers = (struct smog_buffer*)malloc(sizeof(*buffers) * config.nbuffers);
    if (!buffers) {
        perror("malloc");
        return -1;
    }

    for (size_t i = 0; i < config.nbuffers; ++i) {
        res = buffer_init(buffers + i, config.buffers[i]);
        if (res != 0) {
            fprintf(stderr, "failed to create SMOG buffer\n");
            return res;
        }

        printf("  Allocated buffer #%zu with %s", i,
               format_size_string(config.buffers[i].size));
        if (config.buffers[i].file) {
            printf(" from %s\n", config.buffers[i].file);
        } else {
            printf(" from anonymous region\n");
        }

        if (arguments.verbose) {
            printf("    At %p ... %p\n", buffers[i].buffer,
                   buffers[i].buffer + config.buffers[i].size - 1);
        }
    }

    // initialize threads
    size_t nthreads = 0;
    for (size_t i = 0; i < config.nthreads; ++i) {
        nthreads += config.threads[i].group ? config.threads[i].group : 1;
    }

    printf("\n");
    printf("Creating %zu SMOG threads\n", nthreads);

    struct thread_status *thread_status = calloc(nthreads, sizeof(*thread_status));
    struct smog_thread *threads = calloc(nthreads, sizeof(*threads));
    if (!thread_status || !threads) {
        perror("malloc");
        return -1;
    }

    // initialize the shared barrier for buffer initialization
    pthread_barrier_t init;
    pthread_barrier_init(&init, NULL, nthreads + 1);

    // initialize the random number generator, we use rand() to generante RNG seeds for the
    // smog threads, which are passed through the smog_thread structure for use with rand_r
    srand(time(NULL));

    size_t tid = 0;
    for (size_t i = 0; i < config.nthreads; ++i) {
        struct yaml_slice *slice = config.threads[i].slice;

        const char *kernel = kernel_to_string(config.threads[i].kernel);

        size_t group = config.threads[i].group ? config.threads[i].group : 1;
        for (size_t j = 0; j < group; ++j) {
            struct smog_thread *thread = threads + tid;
            struct smog_buffer *buffer = buffers + config.threads[i].buffer_id;

            thread->tid = tid++;
            thread->status = thread_status + tid;
            thread->kernel = config.threads[i].kernel;
            thread->delay = config.threads[i].delay;
            thread->target_rate = config.threads[i].target_rate;
            thread->adjust_timeout = config.threads[i].adjust_timeout;
            thread->adjust_phase = PHASE_DYNAMIC_RAMP_UP;
            thread->init = &init;
            thread->seed = rand();

            off_t off_start = slice->start;
            off_t off_end = slice->end;

            if (slice->group) {
                size_t group_num = j % slice->group;
                size_t len = off_end - off_start + 1;
                off_t new_start = off_start + len * group_num / slice->group;
                off_t new_end = off_start + len * (group_num + 1) / slice->group;
                off_start = new_start;
                off_end = new_end - 1;
            }

            void *start = buffer->buffer + off_start;
            void *end = buffer->buffer + off_end;

            size_t slice_size = off_end - off_start + 1;

            thread->slice.buffer = start;
            thread->slice.length = slice_size;
            thread->slice.nelements = slice_size / sizeof(*thread->slice.elements);
            thread->slice.nwords = slice_size / sizeof(*thread->slice.words);

            if (thread->slice.length % sizeof(struct buffer_element)) {
                fprintf(stderr, "warning: slice size %s is not a multiple of "
                                "the cache line size %s\n",
                                format_size_string(slice_size),
                                format_size_string(CACHE_LINE_SIZE));
            }
            if (thread->slice.nelements <= 0) {
                fprintf(stderr, "warning: thread slice has zero elements\n");
            }

            printf("  Creating Smog kernel '%s' on thread #%zu\n", kernel, tid);
            printf("    With %s ranged %#zx ... %#zx on buffer #%zu\n",
                   format_size_string(thread->slice.length), off_start, off_end,
                   config.threads[i].buffer_id);
            if (arguments.verbose) {
                printf("    At %p ... %p\n", start, end);
                printf("    With delay %zu\n", config.threads[i].delay);
            }

            int res = pthread_create(&thread->thread, NULL, &thread_run, thread);
            if (res != 0) {
                fprintf(stderr, "failed to create thread: %s\n", strerror(errno));
                return 1;
            }
        }
    }

    // sync with worker threads
    pthread_barrier_wait(&init);

    printf("\n");
    printf("initialization finished, starting monitor\n");
    printf("\n");

    monitor_run(nthreads, threads);

    return 0;
}
