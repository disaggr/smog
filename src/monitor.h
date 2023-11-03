/*
 * Copyright (c) 2022 - 2023 OSM Group @ HPI, University of Potsdam
 */

#ifndef MONITOR_H_
#define MONITOR_H_

#include "./thread.h"

enum adjust_phase {
    PHASE_DYNAMIC_RAMP_UP = 0,
    PHASE_STEADY_ADJUST   = 1,
    PHASE_CONSTANT_DELAY  = 2,
};

int monitor_run(size_t nthreads, struct smog_thread *threads);

#endif  // MONITOR_H_
