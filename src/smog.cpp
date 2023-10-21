/*
 * Copyright (c) 2022 - 2023 OSM Group @ HPI, University of Potsdam
 */

#include "./smog.h"

#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

#include <thread>
#include <iostream>
#include <cerrno>
#include <vector>
#include <cstring>

#include "./util.h"
#include "./parser.h"
#include "./monitor.h"
#include "kernels/linear_scan.h"
#include "kernels/random_access.h"
#include "kernels/random_write.h"
#include "kernels/pointer_chase.h"
#include "kernels/cold.h"
#include "kernels/dirty_pages.h"

// assert for correct padding
static_assert (sizeof(thread_status_t) == CACHE_LINE_SIZE, "thread_status_t padded incorrectly");

using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::system_clock;

// globals
size_t g_smog_pagesize;
size_t g_smog_delay;
size_t g_smog_timeout;

// threads
size_t g_thread_count;
struct thread_status_t *g_thread_status;
struct thread_options *g_thread_options;
pthread_t *g_threads;

pthread_barrier_t g_initalization_finished;

// defaults
struct arguments arguments = { 0, 1000, PLAIN, NULL, NULL, 0 };

extern struct argp argp;

int main(int argc, char* argv[]) {
    // determine system characteristics
    size_t system_pages = sysconf(_SC_PHYS_PAGES);
    size_t system_pagesize = sysconf(_SC_PAGE_SIZE);
    size_t cache_line_size = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
    size_t hardware_concurrency = std::thread::hardware_concurrency();

    arguments.pagesize = system_pagesize;

    // parse CLI options
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

    // assert for correct cache line size
    if (cache_line_size != CACHE_LINE_SIZE) {
        fprintf(stderr, "error: mismatched cache line size: %#x Bytes. expected: %#zx Bytes\n",
                CACHE_LINE_SIZE, cache_line_size);
        return 2;
    }

    // parse smogfile
    struct yaml_config config = { NULL, 0, NULL, 0 };
    int res = yaml_parse(arguments.smogfile, &config);
    if (res) {
        fprintf(stderr, "error: failed to parse %s.\n", arguments.smogfile);
        return res;
    }

    // initialize buffers
    std::cout << std::endl;
    std::cout << "Creating " << config.nbuffers << " SMOG buffers" << std::endl;

    void **buffers = (void**)malloc(sizeof(*buffers) * config.nbuffers);
    if (!buffers) {
        perror("malloc");
        return -1;
    }

    for (size_t i = 0; i < config.nbuffers; ++i) {
        if (config.buffers[i].file) {
            int fd = open(config.buffers[i].file, O_RDWR | O_SYNC);
            if (fd < 0) {
                std::cout << config.buffers[i].file << ": open failed: "
                          << std::strerror(errno) << std::endl;
                return 1;
            }
            buffers[i] = mmap(NULL, config.buffers[i].size,
                              PROT_READ | PROT_WRITE,
                              MAP_SHARED,
                              fd, 0);
            close(fd);
        } else {
            buffers[i] = mmap(NULL, config.buffers[i].size,
                              PROT_READ | PROT_WRITE,
                              MAP_PRIVATE | MAP_ANONYMOUS,
                              -1, 0);
        }
        if (buffers[i] == MAP_FAILED) {
            std::cout << "mmap failed: " << std::strerror(errno) << std::endl;
            return 1;
        }
        std::cout << "  Allocated buffer #" << i 
                  << " with " << format_size_string(config.buffers[i].size);
        if (config.buffers[i].file) {
            std::cout << " from " << config.buffers[i].file << std::endl;
        } else {
            std::cout << " from anonymous region" << std::endl;
        }
        std::cout << "    At " << buffers[i] << " ... "
                  << buffers[i] + config.buffers[i].size - 1 << std::endl;
    }

    // initialize threads
    g_thread_count = 0;
    for (size_t i = 0; i < config.nthreads; ++i) {
        g_thread_count += config.threads[i].group ? config.threads[i].group : 1;
    }

    // launch SMOG threads
    std::cout << std::endl;
    std::cout << "Creating " << g_thread_count << " SMOG threads" << std::endl;

    g_thread_status = (thread_status_t*)malloc(sizeof(*g_thread_status) * g_thread_count);
    g_thread_options = (thread_options*)malloc(sizeof(*g_thread_options) * g_thread_count);
    g_threads = (pthread_t*)malloc(sizeof(*g_threads) * g_thread_count);
    if (!g_thread_status || !g_thread_options || !g_threads) {
        perror("malloc");
        return -1;
    }

    pthread_barrier_init(&g_initalization_finished, NULL, g_thread_count + 1);

    size_t tid = 0;
    for (size_t i = 0; i < config.nthreads; ++i) {
        const char *kernel = kernel_to_string(config.threads[i].kernel);
        size_t group = config.threads[i].group ? config.threads[i].group : 1;
        struct yaml_slice *s = config.threads[i].slice;

        for (size_t j = 0; j < group; ++j, ++tid) {
            struct thread_status_t *status = g_thread_status + tid;
            struct thread_options *options = g_thread_options + tid;
            pthread_t *thread = g_threads + tid;

            memset(status, 0, sizeof(*status));
            memset(options, 0, sizeof(*options));

            off_t off_start = s->start;
            off_t off_end = s->end;

            if (s->group) {
                size_t group_num = j % s->group;
                size_t len = off_end - off_start + 1;
                off_t new_start = off_start + len * group_num / s->group; 
                off_t new_end = off_start + len * (group_num + 1) / s->group;
                off_start = new_start;
                off_end = new_end - 1;
            }

            void *start = buffers[config.threads[i].buffer_id] + off_start;
            void *end = buffers[config.threads[i].buffer_id] + off_end; 

            size_t slice_size = off_end - off_start + 1;

            std::cout << "  Creating SMOG kernel '" << kernel
                      << "' on thread #" << tid << std::endl;
            std::cout << "    With " << format_size_string(slice_size)
                      << " ranged " << off_start << " ... " << off_end
                      << " on buffer #" << config.threads[i].buffer_id << std::endl
                      << "    At " << start << " ... " << end << std::endl; 

            options->tid = tid;
            options->slice_start = start;
            options->slice_length = slice_size;

            Smog_Kernel *kernel;
            bool first = (tid == 0);
            switch(config.threads[i].kernel) {
              case KERNEL_LINEARSCAN:
                kernel = new Linear_Scan(first);
                break;
              case KERNEL_RANDOMREAD:
                kernel = new Random_Access(first);
                break;
              case KERNEL_RANDOMWRITE:
                kernel = new Random_Write(first);
                break;
              case KERNEL_POINTERCHASE:
                kernel = new Pointer_Chase(first);
                break;
              case KERNEL_COLD:
                kernel = new Cold(first);
                break;
              case KERNEL_DIRTYPAGES:
                kernel = new Dirty_Pages(first);
                break;
              default:
                std::cerr << "Unknown kernel" << std::endl;
                return -1;
            }

            kernel->Configure(*options);

            if (g_smog_delay > 0)
                pthread_create(thread, NULL, (void*(*)(void*))&Smog_Kernel::Run, kernel);
            else
                pthread_create(thread, NULL, (void*(*)(void*))&Smog_Kernel::Run_Unhinged, kernel);
        }
    }

    // sync with worker threads
    pthread_barrier_wait(&g_initalization_finished);

    std::cout << "initialization finished, starting monitor" << std::endl;
    monitor_run();

    return 0;
}
