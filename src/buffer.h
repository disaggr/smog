/*
 * Copyright (c) 2022 - 2023 OSM Group @ HPI, University of Potsdam
 */

#ifndef BUFFER_H_
#define BUFFER_H_

#include <stdint.h>

#include "./smog.h"
#include "./parser.h"

#define PARALLEL_ACCESSES 4

struct buffer_element {
    union {
        struct {
            uint64_t index;
            struct buffer_element *next;
            struct buffer_element *prev;
            uint64_t randoms[PARALLEL_ACCESSES];
            uint64_t scratch;
        };
        char padding[CACHE_LINE_SIZE];
    };
};

struct smog_buffer {
    union {
        unsigned char *buffer;
        struct buffer_element *elements;
        uint64_t *words;
    };
    size_t length;
    size_t nelements;
    size_t nwords;
};

int buffer_init(struct smog_buffer *buffer, struct yaml_buffer config);

#endif  // BUFFER_H_
