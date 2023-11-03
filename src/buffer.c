/*
 * Copyright (c) 2022 - 2023 OSM Group @ HPI, University of Potsdam
 */

#include "./buffer.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "./util.h"

int buffer_init(struct smog_buffer *buffer, struct yaml_buffer config) {
    static size_t index_offset = 0;

    // determine the length of the buffer
    if (config.size % sizeof(*buffer->elements)) {
        fprintf(stderr, "warning: buffer size %s is not a multiple of the cache line size %s\n",
                        format_size_string(config.size), format_size_string(CACHE_LINE_SIZE));
    }

    buffer->length = config.size;
    buffer->nelements = config.size / sizeof(*buffer->elements);
    buffer->nwords = config.size / sizeof(*buffer->words);

    // map the actual memory from the backing file, or anonymously
    if (config.file) {
        int fd = open(config.file, O_RDWR | O_SYNC);
        if (fd < 0) {
            fprintf(stderr, "%s: open failed: %s\n", config.file,
                    strerror(errno));
            return 1;
        }

        // attempt to truncate the mapping file if too small
        struct stat st;
        int res = fstat(fd, &st);
        if (res != 0) {
            fprintf(stderr, "%s: fstat failed: %s\n", config.file,
                    strerror(errno));
            return 1;
        }

        if ((size_t)st.st_size < config.size) {
            res = ftruncate(fd, config.size);
            if (res != 0) {
                fprintf(stderr, "%s: ftruncate failed: %s\n", config.file,
                        strerror(errno));
                return 1;
            }
        }

        buffer->buffer = mmap(NULL, config.size,
                              PROT_READ | PROT_WRITE,
                              MAP_SHARED,
                              fd, 0);
        close(fd);
    } else {
        buffer->buffer = mmap(NULL, config.size,
                              PROT_READ | PROT_WRITE,
                              MAP_PRIVATE | MAP_ANONYMOUS,
                              -1, 0);
    }

    if (buffer->buffer == MAP_FAILED) {
        fprintf(stderr, "mmap failed: %s\n", strerror(errno));
        return 1;
    }

    // init phase I -- fill with zeroes and deduplicate cache lines
    memset(buffer->buffer, 0, buffer->length);
    for (size_t i = 0; i < buffer->nelements; ++i) {
        buffer->elements[i].index = index_offset + i;
    }
    index_offset += buffer->nelements;

    return 0;
}
