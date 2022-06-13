#include <random_access.h>
#include <cstdlib>
#include <stdint.h>

#define PARALLEL_ACCESSES 4

struct __attribute__((packed)) random_element {
	uint64_t randoms[PARALLEL_ACCESSES];
	uint64_t index;
	char padding[CACHE_LINE_SIZE - PARALLEL_ACCESSES + 1 * sizeof(uint64_t)];
};

void random_access_init(void *thread_buffer, size_t thread_pages) {
	struct random_element *buffer = (struct random_element*) thread_buffer;
	uint64_t elements = thread_pages * page_size / sizeof(struct random_element);
	std::srand(std::time(0));

	for(uint64_t i = 0; i < elements; i++) {
		buffer[i].index = i;
		for(uint64_t j = 0; j < PARALLEL_ACCESSES; j++)
			buffer[i].randoms[j] = std::rand() % (elements - 1);
	}
}

void random_access(Thread_Options t_opts) {
	struct random_element *buffer = (struct random_element*) t_opts.page_buffer;
        uint64_t elements = t_opts.page_count * page_size / sizeof(struct random_element);
        uint64_t sum = 0;

        while (1) {
                for(uint64_t i = 0; i < elements; i++) {
                        // Here I am assuming the impact of skipping a few pages is not
                        // going to be a big issue
                        if (measuring) {
                                continue;
                        }

                        sum += buffer[ buffer[i].randoms[0] ].index;
			sum += buffer[ buffer[i].randoms[1] ].index;
			sum += buffer[ buffer[i].randoms[2] ].index;
			sum += buffer[ buffer[i].randoms[3] ].index;
                        thread_status[t_opts.tid].count += 1;

                        volatile uint64_t delay = 0;
                        for(size_t j = 0; j < smog_delay; j++) {
                                 delay += 1;
                        }
                }
        }
}
