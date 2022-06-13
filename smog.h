#pragma once

#include <cstddef>
#include <time.h>
#include <sys/time.h>
#include <ctime>

// globals
extern size_t page_size;
extern size_t smog_delay;
extern size_t smog_timeout;
extern bool measuring;

struct thread_status_t {
        size_t count;
};

extern struct thread_status_t *thread_status;

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
