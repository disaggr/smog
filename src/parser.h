/*
 * Copyright (c) 2022 - 2023 OSM Group @ HPI, University of Potsdam
 */

#ifndef PARSER_H_
#define PARSER_H_

#include <stddef.h>

#include "kernels/kernels.h"

struct yaml_config {
    struct yaml_buffer *buffers;
    size_t nbuffers;
    struct yaml_thread *threads;
    size_t nthreads;
};

struct yaml_buffer {
    size_t size;
    char *file;
    struct yaml_slice *slices;
    size_t nslices;
};

struct yaml_slice {
    size_t start;
    char *start_str;
    size_t end;
    char *end_str;
    size_t group;
    struct yaml_buffer *buffer;
};

struct yaml_thread {
    enum kernel kernel;
    size_t buffer_id;
    size_t slice_id;
    size_t delay;
    size_t group;
    struct yaml_slice *slice;
};

#ifdef __cplusplus
extern "C" {
#endif

int yaml_parse(const char *file, struct yaml_config *config);

#ifdef __cplusplus
}
#endif

#endif  // PARSER_H_
