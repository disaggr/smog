#include <dirty_pages.h>
#include <cstdint>

void Dirty_Pages::Execute_Kernel() {
        while (1) {
                for(size_t i = 0; i < m_page_count * g_smog_pagesize; i += g_smog_pagesize) {
                        uint64_t tmp = m_buffer[i].index;
                        m_buffer[i].scratch = tmp + 1;

                        g_thread_status[m_id].count += 1;

                        volatile uint64_t delay = 0;
                        for(size_t j = 0; j < g_smog_delay; j++) {
                                 delay += 1;
                        }
                }
        }
}
