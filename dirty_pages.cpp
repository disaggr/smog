#include <dirty_pages.h>
#include <cstdint>

void dirty_pages(Thread_Options t_opts) {
        int work_items = t_opts.page_count;

        while (1) {
                for(size_t i = 0; i < work_items * page_size; i += page_size) {
                        // Here I am assuming the impact of skipping a few pages is not
                        // going to be a big issue
                        if (measuring) {
                                continue;
                        }
                        int tmp = *(int*)((uintptr_t)t_opts.page_buffer + i);
                        *(int*)((uintptr_t)t_opts.page_buffer + i) = tmp + 1;

                        thread_status[t_opts.tid].count += 1;

                        volatile uint64_t delay = 0;
                        for(size_t j = 0; j < smog_delay; j++) {
                                 delay += 1;
                        }
                }
        }
}
