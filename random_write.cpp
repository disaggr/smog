#include <random_write.h>
#include <cstdlib>
#include <stdint.h>

void Random_Write::Execute_Kernel() {
        while (1) {
                for(uint64_t i = 0; i < m_elements; i++) {
                        // Here I am assuming the impact of skipping a few pages is not
                        // going to be a big issue
                        if (g_measuring) {
                                mem_fence();
                                continue;
                        }

                        m_buffer[ m_buffer[i].randoms[0] ].scratch = i;
		        m_buffer[ m_buffer[i].randoms[1] ].scratch = i;
	            	m_buffer[ m_buffer[i].randoms[2] ].scratch = i;
			m_buffer[ m_buffer[i].randoms[3] ].scratch = i;
                        g_thread_status[m_id].count += 4;

                        volatile uint64_t delay = 0;
                        for(size_t j = 0; j < g_smog_delay; j++) {
                                 delay += 1;
                        }
                }
        }
}
