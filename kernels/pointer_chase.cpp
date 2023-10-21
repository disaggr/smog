/*
 * Copyright (c) 2022 - 2023 OSM Group @ HPI, University of Potsdam
 */

#include "kernels/pointer_chase.h"

#include <stdint.h>
#include <cstdlib>
#include <iostream>

#include "../util.h"

void Pointer_Chase::Execute_Kernel() {
    // mark this volatile so that gcc is strict about load and stores
    // (as if they had side effects) so usage is not optimized away below.
    volatile struct element *tmp = &m_buffer[0];

    while (1) {
        // tmp = tmp->next;
        // tmp = tmp->next;
        // tmp = tmp->next;
        tmp = tmp->next;
        g_thread_status[m_id].count += 1;

        volatile uint64_t delay = 0;
        for (size_t j = 0; j < g_smog_delay; j++) {
            delay += 1;
        }
    }
}

void Pointer_Chase::Execute_Kernel_Unhinged() {
    volatile struct element *tmp = &m_buffer[0];

    while (1) {
        REP(0, 9, 9, 9, tmp = tmp->next;)
        g_thread_status[m_id].count += 999;
    }
}
