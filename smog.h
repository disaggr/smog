#pragma once

#include <cstddef>
#include <time.h>
#include <sys/time.h>
#include <ctime>
#include <pthread.h>

// globals
#define CACHE_LINE_SIZE 128
extern size_t g_page_size;
extern size_t g_smog_delay;
extern size_t g_smog_timeout;
extern bool g_measuring;
extern pthread_barrier_t g_initalization_finished;

#if defined(__ppc64__) || defined(__PPC64__)
#  define mem_fence()  __asm__ __volatile__ ("lwsync" : : : "memory")
#else
#  define mem_fence()
#endif

struct __attribute__((packed)) thread_status_t {
        size_t count;
	char padding[CACHE_LINE_SIZE - sizeof(size_t)];
};

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
