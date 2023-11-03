/*
 * Copyright (c) 2022 - 2023 OSM Group @ HPI, University of Potsdam
 */

#ifndef KERNELS_POINTER_CHASE_H_
#define KERNELS_POINTER_CHASE_H_

#include "../kernel.h"

int pointer_chase_init(struct smog_thread *thread);

void pointer_chase_run(struct smog_thread *thread);

void pointer_chase_run_unhinged(struct smog_thread *thread);

#endif  // KERNELS_POINTER_CHASE_H_
