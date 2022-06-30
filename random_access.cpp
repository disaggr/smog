#include <random_access.h>
#include <cstdlib>
#include <stdint.h>

void Random_Access::Execute_Kernel() {
        uint64_t sum = 0;

        while (1) {
                for(uint64_t i = 0; i < m_elements; i++) {
                        // Here I am assuming the impact of skipping a few pages is not
                        // going to be a big issue
                        if (g_measuring) {
                                continue;
                        }

                        sum += m_buffer[ m_buffer[i].randoms[0] ].index;
			sum += m_buffer[ m_buffer[i].randoms[1] ].index;
			sum += m_buffer[ m_buffer[i].randoms[2] ].index;
			sum += m_buffer[ m_buffer[i].randoms[3] ].index;
                        g_thread_status[m_id].count += 4;

                        volatile uint64_t delay = 0;
                        for(size_t j = 0; j < g_smog_delay; j++) {
                                 delay += 1;
                        }
                }
        }
}
