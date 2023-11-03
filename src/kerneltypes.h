/*
 * Copyright (c) 2022 - 2023 OSM Group @ HPI, University of Potsdam
 */

#ifndef KERNELTYPES_H_
#define KERNELTYPES_H_

enum kernel {
    KERNEL_UNKNOWN,
    KERNEL_COLD,
    KERNEL_DIRTYPAGES,
    KERNEL_LINEARSCAN,
    KERNEL_POINTERCHASE,
    KERNEL_RANDOMREAD,
    KERNEL_RANDOMWRITE,
};

#endif  // KERNELTYPES_H_
