/*
 * Copyright (c) 2022 - 2023 OSM Group @ HPI, University of Potsdam
 */

#ifndef KERNELS_DIRTY_PAGES_H_
#define KERNELS_DIRTY_PAGES_H_

#include "../kernel.h"

void dirty_pages_run(struct smog_thread *thread);

void dirty_pages_run_unhinged(struct smog_thread *thread);

#endif  // KERNELS_DIRTY_PAGES_H_
