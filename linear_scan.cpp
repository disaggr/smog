#include <linear_scan.h>
#include <stdint.h>

void Linear_Scan::Execute_Kernel() {
	uint64_t sum = 0;
        uint64_t bytes = m_elements * sizeof(m_elements);
        uint64_t elements = bytes / sizeof(uint64_t);


        while (1) {
		//for(uint64_t i = 0; i < elements / 10; i += 10) {
		for(uint64_t i = 0; i < elements; i++) {
                        // Here I am assuming the impact of skipping a few pages is not
                        // going to be a big issue
                        if (g_measuring) {
                                mem_fence();
                                continue;
                        }

                        sum += *(((uint64_t *)m_buffer) + i);
			/*sum += *(((uint64_t *)m_buffer) + i + 1);
			sum += *(((uint64_t *)m_buffer) + i + 2);
			sum += *(((uint64_t *)m_buffer) + i + 3);
			sum += *(((uint64_t *)m_buffer) + i + 4);
			sum += *(((uint64_t *)m_buffer) + i + 5);
			sum += *(((uint64_t *)m_buffer) + i + 6);
			sum += *(((uint64_t *)m_buffer) + i + 7);
			sum += *(((uint64_t *)m_buffer) + i + 8);
			sum += *(((uint64_t *)m_buffer) + i + 9);
                        g_thread_status[m_id].count += 10;*/
			g_thread_status[m_id].count += 1;
			mem_fence();

                        volatile uint64_t delay = 0;
                        for(size_t j = 0; j < g_smog_delay; j++) {
                                 delay += 1;
                        }
		}
        }
}
