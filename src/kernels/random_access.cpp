/*
 * Copyright (c) 2022 - 2023 OSM Group @ HPI, University of Potsdam
 */

#include "kernels/random_access.h"

#include <cstdlib>
#include <cstdint>

void Random_Access::Execute_Kernel() {
    // mark this volatile so that gcc is strict about load and stores
    // (as if they had side effects) so usage is not optimized away below.
    volatile uint64_t sum = 0;

    while (1) {
        for (uint64_t i = 0; i < m_elements; i++) {
            sum += m_buffer[ m_buffer[i].randoms[0] ].index;
            sum += m_buffer[ m_buffer[i].randoms[1] ].index;
            sum += m_buffer[ m_buffer[i].randoms[2] ].index;
            sum += m_buffer[ m_buffer[i].randoms[3] ].index;
            g_thread_status[m_id].count += 4;

            volatile uint64_t delay = 0;
            for (size_t j = 0; j < m_delay; j++) {
                delay += 1;
            }
        }
    }
}
