/*
 * Copyright (c) 2022 - 2023 OSM Group @ HPI, University of Potsdam
 */

#include "kernels/kernels.h"

#include <string.h>

enum kernel kernel_from_string(const char *s) {
    if (!strcmp(s, "cold"))
        return KERNEL_COLD;
    if (!strcmp(s, "dirty pages"))
        return KERNEL_DIRTYPAGES;
    if (!strcmp(s, "linear scan"))
        return KERNEL_LINEARSCAN;
    if (!strcmp(s, "pointer chase"))
        return KERNEL_POINTERCHASE;
    if (!strcmp(s, "random read"))
        return KERNEL_RANDOMREAD;
    if (!strcmp(s, "random write"))
        return KERNEL_RANDOMWRITE;

    return KERNEL_UNKNOWN;
}

const char* kernel_to_string(enum kernel k) {
    switch (k) {
        case KERNEL_COLD:
            return "cold";
        case KERNEL_DIRTYPAGES:
            return "dirty pages";
        case KERNEL_LINEARSCAN:
            return "linear scan";
        case KERNEL_POINTERCHASE:
            return "pointer chase";
        case KERNEL_RANDOMREAD:
            return "random read";
        case KERNEL_RANDOMWRITE:
            return "random write";
        default:
            return "unkwnown";
    }
}
