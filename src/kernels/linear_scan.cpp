/*
 * Copyright (c) 2022 - 2023 OSM Group @ HPI, University of Potsdam
 */

#include "kernels/linear_scan.h"

#include <stdint.h>

#include "../util.h"

void Linear_Scan::Execute_Kernel() {
    // mark this volatile so that gcc is strict about load and stores
    // (as if they had side effects) so usage is not optimized away below.
    uint64_t sum = 0;

    uint64_t bytes = m_elements * sizeof(m_elements);
    uint64_t elements = bytes / sizeof(uint64_t);

    while (1) {
        for (uint64_t i = 0; i < elements / 10; i += 10) {
            // for(uint64_t i = 0; i < elements; i++) {
            // sum += *(((uint64_t *)m_buffer) + i);
            uint64_t *p = reinterpret_cast<uint64_t*>(m_buffer);
            sum += *(p + i + 1);
            sum += *(p + i + 2);
            sum += *(p + i + 3);
            sum += *(p + i + 4);
            sum += *(p + i + 5);
            sum += *(p + i + 6);
            sum += *(p + i + 7);
            sum += *(p + i + 8);
            sum += *(p + i + 9);
            g_thread_status[m_id].count += 10;
            // g_thread_status[m_id].count += 1;

            volatile uint64_t delay = 0;
            for (size_t j = 0; j < g_smog_delay; j++) {
                delay += 1;
            }
        }
    ((volatile uint64_t *)m_buffer)[0] = sum;
    }
}

void Linear_Scan::Execute_Kernel_Unhinged() {
    // mark this volatile so that gcc is strict about load and stores
    // (as if they had side effects) so usage is not optimized away below.
    volatile uint64_t sum[512] = {0};
    // uint64_t sum = 0;

    uint64_t bytes = m_elements * sizeof(m_elements);
    uint64_t elements = bytes / sizeof(uint64_t);

    while (1) {
        for (uint64_t i = 0; i < elements;) {
            REP(0, 5, 1, 2, *(((volatile uint64_t *)m_buffer) + i++);)
            g_thread_status[m_id].count += 512;
        }
        // ((uint64_t *)m_buffer)[0] = sum;
    }
}
