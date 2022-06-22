#include <linear_scan.h>
#include <stdint.h>

void Linear_Scan::Initialize() {
        *m_buffer = (uint64_t*) m_page_buffer;
        m_elements = t_opts.page_count * page_size / CACHE_LINE_SIZE;
}

void Linear_Scan::Execute_Kernel() {
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
                        for(size_t j = 0; j < g_smog_delay; j++) {
                                 delay += 1;
                        }
		}
        }
}
