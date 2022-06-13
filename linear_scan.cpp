#include <linear_scan.h>
#include <stdint.h>

void linear_scan(Thread_Options t_opts) {
        uint64_t *buffer = (uint64_t*) t_opts.page_buffer;
        uint64_t elements = t_opts.page_count * page_size / CACHE_LINE_SIZE;
	uint64_t sum = 0;

        while (1) {
		for(uint64_t i = 0; i < elements; i++) {
                        // Here I am assuming the impact of skipping a few pages is not
                        // going to be a big issue
                        if (measuring) {
                                continue;
                        }

                        sum += buffer[i];
                        thread_status[t_opts.tid].count += 1;

                        volatile uint64_t delay = 0;
                        for(size_t j = 0; j < smog_delay; j++) {
                                 delay += 1;
                        }
		}
        }
}
