/*
 * Copyright (c) 2022 - 2023 OSM Group @ HPI, University of Potsdam
 */

#ifndef KERNELS_LINEAR_SCAN_H_
#define KERNELS_LINEAR_SCAN_H_

#include "../kernel.h"

void linear_scan_run(struct smog_thread *thread);

void linear_scan_run_unhinged(struct smog_thread *thread);

#endif  // KERNELS_LINEAR_SCAN_H_
