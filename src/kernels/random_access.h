/*
 * Copyright (c) 2022 - 2023 OSM Group @ HPI, University of Potsdam
 */

#ifndef KERNELS_RANDOM_ACCESS_H_
#define KERNELS_RANDOM_ACCESS_H_

#include "../kernel.h"

int random_access_init(struct smog_thread *thread);

void random_read_run(struct smog_thread *thread);

void random_read_run_unhinged(struct smog_thread *thread);

void random_write_run(struct smog_thread *thread);

void random_write_run_unhinged(struct smog_thread *thread);

#endif  // KERNELS_RANDOM_ACCESS_H_
