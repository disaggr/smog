#include <dirty_pages.h>
#include <cstdint>

void Dirty_Pages::Initialize() {
	// prepare page preambles
        for(size_t i = 0; i < m_work_items; ++i) {
                *(int*)((uintptr_t)m_page_buffer + i * g_page_size) = 0;
        }	
}

void Dirty_Pages::Execute_Kernel() {
        while (1) {
                for(size_t i = 0; i < m_work_items * g_page_size; i += g_page_size) {
                        // Here I am assuming the impact of skipping a few pages is not
                        // going to be a big issue
                        if (g_measuring) {
                                continue;
                        }
                        int tmp = *(int*)((uintptr_t)m_page_buffer + i);
                        *(int*)((uintptr_t)m_page_buffer + i) = tmp + 1;

                        g_thread_status[m_id].count += 1;

                        volatile uint64_t delay = 0;
                        for(size_t j = 0; j < g_smog_delay; j++) {
                                 delay += 1;
                        }
                }
        }
}
