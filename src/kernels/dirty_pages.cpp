/*
 * Copyright (c) 2022 - 2023 OSM Group @ HPI, University of Potsdam
 */

#include "kernels/dirty_pages.h"

#include <cstdint>

void Dirty_Pages::Execute_Kernel() {
    size_t limit = m_slice_length / sizeof(struct element);
    size_t step = arguments.pagesize / sizeof(struct element);

    while (1) {
        for (size_t i = 0; i < limit; i += step) {
            uint64_t tmp = m_buffer[i].index;
            m_buffer[i].scratch = tmp + 1;

            g_thread_status[m_id].count += 1;

            volatile uint64_t delay = 0;
            for (size_t j = 0; j < m_delay; j++) {
                delay += 1;
            }
        }
    }
}
