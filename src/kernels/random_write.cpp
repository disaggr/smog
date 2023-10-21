/*
 * Copyright (c) 2022 - 2023 OSM Group @ HPI, University of Potsdam
 */

#include "kernels/random_write.h"

#include <cstdlib>
#include <cstdint>

void Random_Write::Execute_Kernel() {
    while (1) {
        for (uint64_t i = 0; i < m_elements; i++) {
            m_buffer[ m_buffer[i].randoms[0] ].scratch = i;
            m_buffer[ m_buffer[i].randoms[1] ].scratch = i;
            m_buffer[ m_buffer[i].randoms[2] ].scratch = i;
            m_buffer[ m_buffer[i].randoms[3] ].scratch = i;
            g_thread_status[m_id].count += 4;

            volatile uint64_t delay = 0;
            for (size_t j = 0; j < g_smog_delay; j++) {
                delay += 1;
            }
        }
    }
}
