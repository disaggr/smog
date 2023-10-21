/*
 * Copyright (c) 2022 - 2023 OSM Group @ HPI, University of Potsdam
 */

#ifndef KERNELS_KERNELS_H_
#define KERNELS_KERNELS_H_

enum kernel {
    KERNEL_UNKNOWN,
    KERNEL_COLD,
    KERNEL_DIRTYPAGES,
    KERNEL_LINEARSCAN,
    KERNEL_POINTERCHASE,
    KERNEL_RANDOMREAD,
    KERNEL_RANDOMWRITE,
};

#ifdef __cplusplus
extern "C" {
#endif

enum kernel kernel_from_string(const char *s);

const char* kernel_to_string(enum kernel k);

#ifdef __cplusplus
}
#endif

#endif  // KERNELS_KERNELS_H_
