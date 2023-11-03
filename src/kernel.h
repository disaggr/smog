/*
 * Copyright (c) 2022 - 2023 OSM Group @ HPI, University of Potsdam
 */

#ifndef KERNEL_H_
#define KERNEL_H_

#include "./thread.h"
#include "./kerneltypes.h"

typedef void(*kernel_run_f)(struct smog_thread*);
typedef int(*kernel_init_f)(struct smog_thread*);

struct smog_kernel {
    enum kernel kernel;
    const char *name;
    char short_name;

    kernel_run_f run;
    kernel_run_f run_unhinged;
    kernel_init_f init;
};

extern struct smog_kernel smog_kernels[];

enum kernel kerneltype_from_string(const char *s);

const char* kernel_to_string(enum kernel k);

char kernel_to_char(enum kernel k);

#endif  // KERNEL_H_
