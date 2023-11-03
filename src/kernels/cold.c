/*
 * Copyright (c) 2022 - 2023 OSM Group @ HPI, University of Potsdam
 */

#include "kernels/cold.h"

#include <unistd.h>

#include "../smog.h"

void cold_run(struct smog_thread *thread) {
    (void) thread;

    while (1) {
        sleep(-1);
    }
}
