/*
 * Copyright (c) 2022 - 2023 OSM Group @ HPI, University of Potsdam
 */

#include "./kernel.h"

#include <string.h>

#include "./kernels/cold.h"
#include "./kernels/dirty_pages.h"
#include "./kernels/linear_scan.h"
#include "./kernels/pointer_chase.h"
#include "./kernels/random_access.h"

struct smog_kernel smog_kernels[] = {
    { KERNEL_COLD, "cold", 'c', &cold_run, NULL, NULL },
    { KERNEL_DIRTYPAGES, "dirty pages", 'd',
      &dirty_pages_run, &dirty_pages_run_unhinged, NULL },
    { KERNEL_LINEARSCAN, "linear scan", 'l',
      &linear_scan_run, &linear_scan_run_unhinged, NULL },
    { KERNEL_POINTERCHASE, "pointer chase", 'p',
      &pointer_chase_run, &pointer_chase_run_unhinged, &pointer_chase_init },
    { KERNEL_RANDOMREAD, "random read", 'r',
      &random_read_run, &random_read_run_unhinged, &random_access_init },
    { KERNEL_RANDOMWRITE, "random write", 'w',
      &random_write_run, &random_write_run_unhinged, &random_access_init },
    { 0 },
};

enum kernel kerneltype_from_string(const char *s) {
    for (struct smog_kernel *k = smog_kernels; k->name != NULL; ++k)
        if (!strcmp(s, k->name))
            return k->kernel;

    return KERNEL_UNKNOWN;
}

const char* kernel_to_string(enum kernel kernel) {
    for (struct smog_kernel *k = smog_kernels; k->name != NULL; ++k)
        if (k->kernel == kernel)
            return k->name;

    return "unknown";
}

char kernel_to_char(enum kernel kernel) {
    for (struct smog_kernel *k = smog_kernels; k->name != NULL; ++k)
        if (k->kernel == kernel)
            return k->short_name;

    return '?';
}
