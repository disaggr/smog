#pragma once

#include <cstddef>
#include <time.h>
#include <sys/time.h>
#include <ctime>
#include <pthread.h>

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

// assert for correct padding
static_assert (sizeof(thread_status_t) == CACHE_LINE_SIZE, "thread_status_t padded incorrectly");

extern struct thread_status_t *g_thread_status;

class Thread_Options {
    public:
        Thread_Options(int tid, size_t page_count, void *page_buffer) :
            tid(tid), page_count(page_count), page_buffer(page_buffer)
        {}
        int tid;
        size_t page_count;
        void *page_buffer;
};

long double get_unixtime();
