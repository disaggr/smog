/*
 * Copyright (c) 2022 - 2023 OSM Group @ HPI, University of Potsdam
 */

#include "./parser.h"

#include <yaml.h>

#include <stdarg.h>

#include "./smog.h"
#include "./util.h"

struct yaml_config yaml_config = { };

#define LIST_PUSH(O, E) \
  (O).E = realloc((O).E, ((O).n##E + 1) * sizeof(*(O).E)); (O).n##E++; \

enum key {
    KEY_NONE,
    KEY_SIZE,
    KEY_FILE,
    KEY_RANGE,
    KEY_GROUP,
    KEY_KERNEL,
    KEY_BUFFER,
};

enum context {
    CONTEXT_ROOT,
    CONTEXT_BUFFER,
    CONTEXT_SLICE,
    CONTEXT_THREAD,
};

enum state {
    STATE_START,
    STATE_STREAM,
    STATE_DOCUMENT,
    STATE_MAPPING,
    STATE_EXPECT_LIST,
    STATE_LIST,
    STATE_EXPECT_SCALAR,
    STATE_STOP,
};

#define CASE(s) case s: return #s
static const char *key_str(enum key k) {
    switch (k) {
        CASE(KEY_SIZE);
        CASE(KEY_FILE);
        CASE(KEY_RANGE);
        CASE(KEY_GROUP);
        CASE(KEY_KERNEL);
        CASE(KEY_BUFFER);
        default:
            return "Unknown Key";
    }
}

static const char *context_str(enum context c) {
    switch (c) {
        CASE(CONTEXT_ROOT);
        CASE(CONTEXT_BUFFER);
        CASE(CONTEXT_SLICE);
        CASE(CONTEXT_THREAD);
        default:
            return "Unknown State";
    }
}

static const char *state_str(enum state s) {
    switch (s) {
        CASE(STATE_START);
        CASE(STATE_STREAM);
        CASE(STATE_DOCUMENT);
        CASE(STATE_MAPPING);
        CASE(STATE_EXPECT_LIST);
        CASE(STATE_LIST);
        CASE(STATE_EXPECT_SCALAR);
        CASE(STATE_STOP);
        default:
            return "Unknown State";
    }
}

static const char *event_str(yaml_event_type_t e) {
    switch (e) {
        CASE(YAML_NO_EVENT);
        CASE(YAML_STREAM_START_EVENT);
        CASE(YAML_STREAM_END_EVENT);
        CASE(YAML_DOCUMENT_START_EVENT);
        CASE(YAML_DOCUMENT_END_EVENT);
        CASE(YAML_ALIAS_EVENT);
        CASE(YAML_SCALAR_EVENT);
        CASE(YAML_SEQUENCE_START_EVENT);
        CASE(YAML_SEQUENCE_END_EVENT);
        CASE(YAML_MAPPING_START_EVENT);
        CASE(YAML_MAPPING_END_EVENT);
        default:
            return "Unknown State";
    }
}
#undef CASE

struct parsed_buffer {
    size_t size;
    char *file;
    struct parsed_slice *slices;
    size_t nslices;
};

struct parsed_slice {
    char *lower;
    char *upper;
    size_t group;
};

struct parsed_thread {
    enum kernel kernel;
    size_t buffer_id;
    size_t slice_id;
    size_t group;
};

struct parser_state {
    enum state state;
    enum key key;
    enum context context;

    struct yaml_config *config;

    struct parsed_buffer buffer;
    struct parsed_slice slice;
    struct parsed_thread thread;
};

static int parser_error(yaml_parser_t *parser, yaml_event_t *event, const char *format, ...) {
    char *buf = NULL;
    size_t size = 0;

    va_list ap;
    va_start(ap, format);
    int n = vsnprintf(buf, size, format, ap);
    va_end(ap);

    if (n < 0)
        return -1;

    size = (size_t)n + 1;
    buf = malloc(size);
    if (!buf)
        return -1;

    va_start(ap, format);
    n = vsnprintf(buf, size, format, ap);
    va_end(ap);

    if (n < 0) {
        free(buf);
        return -1;
    }

    parser->problem = buf;
    parser->problem_mark = event->start_mark;
    return n;
}

static int consume_event(yaml_parser_t *parser, struct parser_state *state, yaml_event_t *event) {
    char *value;

    if (arguments.verbose) {
        printf("state=%s context=%s event=%s",
               state_str(state->state),
               context_str(state->context),
               event_str(event->type));
        if (event->type == YAML_SCALAR_EVENT) {
            printf(" value=\"%s\"", event->data.scalar.value);
        }
        printf("\n");
    }

    switch (state->state) {
        case STATE_START:
            switch (event->type) {
                case YAML_STREAM_START_EVENT:
                    state->state = STATE_STREAM;
                    break;
                default:
                    parser_error(parser, event, "Unexpected event %d in state %s",
                                 event->type, state_str(state->state));
                    return 0;
            }
            break;

        case STATE_STREAM:
            switch (event->type) {
                case YAML_DOCUMENT_START_EVENT:
                    state->state = STATE_DOCUMENT;
                    break;
                case YAML_STREAM_END_EVENT:
                    // establish internal references
                    for (size_t i = 0; i < state->config->nbuffers; ++i) {
                        struct yaml_buffer *b = state->config->buffers + i;
                        for (size_t j = 0; j < b->nslices; ++j) {
                            b->slices[j].buffer = b;
                        }
                    }

                    for (size_t i = 0; i < state->config->nthreads; ++i) {
                        struct yaml_thread *t = state->config->threads + i;

                        if (t->buffer_id >= state->config->nbuffers) {
                            parser_error(parser, event, "Buffer id out of range: %zu",
                                         t->buffer_id);
                            return 0;
                        }

                        struct yaml_buffer *b = state->config->buffers + t->buffer_id;

                        if (t->slice_id >= b->nslices) {
                            parser_error(parser, event, "Slice id out of range: %zu",
                                         t->slice_id);
                            return 0;
                        }

                        t->slice = b->slices + t->slice_id;
                    }
                    state->state = STATE_STOP;
                    break;
                default:
                    parser_error(parser, event, "Unexpected event %d in state %s",
                                 event->type, state_str(state->state));
                    return 0;
            }
            break;

        case STATE_DOCUMENT:
            switch (event->type) {
                case YAML_MAPPING_START_EVENT:
                    state->state = STATE_MAPPING;
                    break;
                case YAML_DOCUMENT_END_EVENT:
                    state->state = STATE_STREAM;
                    break;
                default:
                    parser_error(parser, event, "Unexpected event %d in state %s",
                                 event->type, state_str(state->state));
                    return 0;
            }
            break;

        case STATE_MAPPING:
            switch (event->type) {
                case YAML_SCALAR_EVENT:
                    value = (char *)event->data.scalar.value;

                    switch (state->context) {
                        case CONTEXT_ROOT:
                            if (!strcmp(value, "buffers")) {
                                state->context = CONTEXT_BUFFER;
                                state->state = STATE_EXPECT_LIST;
                            } else if (!strcmp(value, "threads")) {
                                state->context = CONTEXT_THREAD;
                                state->state = STATE_EXPECT_LIST;
                            } else {
                                parser_error(parser, event, "Unrecognized key: %s", value);
                                return 0;
                            }
                            break;

                        case CONTEXT_BUFFER:
                            if (!strcmp(value, "size")) {
                                state->key = KEY_SIZE;
                                state->state = STATE_EXPECT_SCALAR;
                            } else if (!strcmp(value, "file")) {
                                state->key = KEY_FILE;
                                state->state = STATE_EXPECT_SCALAR;
                            } else if (!strcmp(value, "slices")) {
                                state->context = CONTEXT_SLICE;
                                state->state = STATE_EXPECT_LIST;
                            } else {
                                parser_error(parser, event, "Unrecognized key: %s", value);
                                return 0;
                            }
                            break;

                        case CONTEXT_SLICE:
                            if (!strcmp(value, "range")) {
                                state->key = KEY_RANGE;
                                state->state = STATE_EXPECT_SCALAR;
                            } else if (!strcmp(value, "group")) {
                                state->key = KEY_GROUP;
                                state->state = STATE_EXPECT_SCALAR;
                            } else {
                                parser_error(parser, event, "Unrecognized key: %s", value);
                                return 0;
                            }
                            break;

                        case CONTEXT_THREAD:
                            if (!strcmp(value, "kernel")) {
                                state->key = KEY_KERNEL;
                                state->state = STATE_EXPECT_SCALAR;
                            } else if (!strcmp(value, "buffer")) {
                                state->key = KEY_BUFFER;
                                state->state = STATE_EXPECT_SCALAR;
                            } else if (!strcmp(value, "group")) {
                                state->key = KEY_GROUP;
                                state->state = STATE_EXPECT_SCALAR;
                            } else {
                                parser_error(parser, event, "Unrecognized key: %s", value);
                                return 0;
                            }
                            break;

                        default:
                            parser_error(parser, event, "Unrecognized key: %s", value);
                            return 0;
                    }
                    break;
                case YAML_MAPPING_END_EVENT:
                    switch (state->context) {
                        case CONTEXT_ROOT:
                            state->state = STATE_DOCUMENT;
                            break;
                        case CONTEXT_BUFFER:
                            struct yaml_buffer *b = state->config->buffers +
                                                    state->config->nbuffers - 1;
                            b->size = state->buffer.size;
                            b->file = state->buffer.file;
                            // translate slice ranges into offsets
                            for (size_t i = 0; i < b->nslices; ++i) {
                                struct yaml_slice *s = b->slices + i;

                                size_t len = strlen(s->start_str);
                                if (s->start_str[len - 1] == '%') {
                                    errno = 0;
                                    size_t percent = strtoll(s->start_str, NULL, 0);
                                    if (errno != 0 || percent > 100) {
                                        parser_error(parser, event, "invalid percentage: %s",
                                                     s->start_str);
                                        return 0;
                                    }

                                    s->start = percent * b->size / 100;
                                } else {
                                    errno = 0;
                                    s->start = parse_size_string(s->start_str);
                                    if (errno != 0) {
                                        parser_error(parser, event, "invalid size: %s",
                                                     s->start_str);
                                        return 0;
                                    }
                                }

                                int offset = 0;
                                char *end_str = s->end_str;
                                if (s->end_str[0] == '<') {
                                    offset = -1;
                                    end_str++;
                                }

                                len = strlen(end_str);
                                if (end_str[len - 1] == '%') {
                                    errno = 0;
                                    size_t percent = strtoll(end_str, NULL, 0);
                                    if (errno != 0 || percent > 100) {
                                        parser_error(parser, event, "invalid percentage: %s",
                                                     end_str);
                                        return 0;
                                    }

                                    s->end = percent * b->size / 100 + offset;
                                } else {
                                    errno = 0;
                                    s->end = parse_size_string(end_str) + offset;
                                    if (errno != 0) {
                                        parser_error(parser, event, "invalid size: %s",
                                                     end_str);
                                        return 0;
                                    }
                                }
                            }

                            state->state = STATE_LIST;
                            break;
                        case CONTEXT_THREAD:
                            struct yaml_thread *t = state->config->threads +
                                                    state->config->nthreads - 1;
                            t->kernel = state->thread.kernel;
                            t->buffer_id = state->thread.buffer_id;
                            t->slice_id = state->thread.slice_id;
                            t->group = state->thread.group;
                            state->state = STATE_LIST;
                            break;
                        case CONTEXT_SLICE:
                            state->state = STATE_LIST;
                            b = state->config->buffers + state->config->nbuffers - 1;
                            struct yaml_slice *s = b->slices + b->nslices - 1;
                            s->start_str = state->slice.lower;
                            s->end_str = state->slice.upper;
                            s->group = state->slice.group;
                            break;
                        default:
                            parser_error(parser, event, "Unrecognized context: %d", state->context);
                            return 0;
                    }
                    break;
                case YAML_DOCUMENT_END_EVENT:
                    state->state = STATE_STREAM;
                    break;
                default:
                    parser_error(parser, event, "Unexpected event %d in state %s",
                                 event->type, state_str(state->state));
                    return 0;
            }
            break;

        case STATE_EXPECT_LIST:
            switch (event->type) {
                case YAML_SEQUENCE_START_EVENT:
                    state->state = STATE_LIST;
                    break;
                default:
                    parser_error(parser, event, "Unexpected event %d in state %s",
                                 event->type, state_str(state->state));
                    return 0;
            }
            break;

        case STATE_LIST:
            switch (event->type) {
                case YAML_MAPPING_START_EVENT:
                    state->state = STATE_MAPPING;
                    switch (state->context) {
                        case CONTEXT_BUFFER:
                            // clear the context's buffer
                            memset(&state->buffer, 0, sizeof(state->buffer));
                            // add a new buffer object to the config
                            LIST_PUSH(*state->config, buffers);
                            if (!state->config->buffers) {
                                parser_error(parser, event, strerror(errno));
                                return 0;
                            }
                            struct yaml_buffer *b = state->config->buffers +
                                                    state->config->nbuffers - 1;
                            memset(b, 0, sizeof(*b));
                            break;
                        case CONTEXT_THREAD:
                            // clear the context's thread
                            memset(&state->thread, 0, sizeof(state->thread));
                            // add a new thread object to the config
                            LIST_PUSH(*state->config, threads);
                            if (!state->config->threads) {
                                parser_error(parser, event, strerror(errno));
                                return 0;
                            }
                            struct yaml_thread *t = state->config->threads +
                                                    state->config->nthreads - 1;
                            memset(t, 0, sizeof(*t));
                            break;
                        case CONTEXT_SLICE:
                            // clear the context's slice
                            memset(&state->slice, 0, sizeof(state->thread));
                            // add a new slice object to the buffer
                            b = state->config->buffers + state->config->nbuffers - 1;
                            LIST_PUSH(*b, slices);
                            if (!b->slices) {
                                parser_error(parser, event, strerror(errno));
                                return 0;
                            }
                            struct yaml_slice *s = b->slices + b->nslices - 1;
                            memset(s, 0, sizeof(*s));
                            break;
                        default:
                            break;
                    }
                    break;
                case YAML_SEQUENCE_END_EVENT:
                    switch (state->context) {
                        case CONTEXT_BUFFER:
                        case CONTEXT_THREAD:
                            state->context = CONTEXT_ROOT;
                            state->state = STATE_MAPPING;
                            break;
                        case CONTEXT_SLICE:
                            state->context = CONTEXT_BUFFER;
                            state->state = STATE_MAPPING;
                            break;
                        default:
                            parser_error(parser, event, "Unrecognized context: %d", state->context);
                            return 0;
                    }
                    break;
                default:
                    parser_error(parser, event, "Unexpected event %d in state %s",
                                 event->type, state_str(state->state));
                    return 0;
            }
            break;

        case STATE_EXPECT_SCALAR:
            switch (event->type) {
                case YAML_SCALAR_EVENT:
                    value = (char *)event->data.scalar.value;

                    switch (state->key) {
                        case KEY_SIZE:
                            // the size is parsed from a size string
                            errno = 0;
                            size_t size = parse_size_string(value);
                            if (errno != 0) {
                                parser_error(parser, event, "invalid size: %s", value);
                                return 0;
                            }

                            switch (state->context) {
                                case CONTEXT_BUFFER:
                                    state->buffer.size = size;
                                    break;
                                default:
                                    parser_error(parser, event, "%s does not apply to %s",
                                                 key_str(state->key),
                                                 context_str(state->context));
                                    return 0;
                            }
                            break;

                        case KEY_FILE:
                            // the file is a string
                            switch (state->context) {
                                case CONTEXT_BUFFER:
                                    state->buffer.file = strdup(value);
                                    break;
                                default:
                                    parser_error(parser, event, "%s does not apply to %s",
                                                 key_str(state->key),
                                                 context_str(state->context));
                                    return 0;
                            }
                            break;

                        case KEY_RANGE:
                            // the range is in the form [lower]...[upper], where upper may be
                            // prefixed with < to mean an exclusive range.
                            char *p = strstr(value, "...");
                            if (!p) {
                                parser_error(parser, event, "invalid range: %s", value);
                                return 0;
                            }
                            char *lower = value;
                            char *upper = p + 3;
                            *p = 0;

                            switch (state->context) {
                                case CONTEXT_SLICE:
                                    state->slice.lower = strdup(lower);
                                    state->slice.upper = strdup(upper);
                                    break;
                                default:
                                    parser_error(parser, event, "%s does not apply to %s",
                                                 key_str(state->key),
                                                 context_str(state->context));
                                    return 0;
                            }
                            break;

                        case KEY_GROUP:
                            // the group is a single integer
                            errno = 0;
                            size_t group = strtoll(value, NULL, 0);
                            if (errno != 0) {
                                parser_error(parser, event, "invalid group: %s", value);
                                return 0;
                            }

                            switch (state->context) {
                                case CONTEXT_SLICE:
                                    state->slice.group = group;
                                    break;
                                case CONTEXT_THREAD:
                                    state->thread.group = group;
                                    break;
                                default:
                                    parser_error(parser, event, "%s does not apply to %s",
                                                 key_str(state->key),
                                                 context_str(state->context));
                                    return 0;
                            }
                            break;

                        case KEY_KERNEL:
                            // the kernel is a string literal
                            enum kernel kernel = kernel_from_string(value);
                            if (kernel == KERNEL_UNKNOWN) {
                                parser_error(parser, event, "Unrecognized kernel: %s", value);
                                return 0;
                            }

                            switch (state->context) {
                                case CONTEXT_THREAD:
                                    state->thread.kernel = kernel;
                                    break;
                                default:
                                    parser_error(parser, event, "%s does not apply to %s",
                                                 key_str(state->key),
                                                 context_str(state->context));
                                    return 0;
                            }
                            break;

                        case KEY_BUFFER:
                            // the buffer is a tuple of buffer_id and slice_id
                            p = strchr(value, ':');
                            if (!p) {
                                parser_error(parser, event, "invalid buffer: %s", value);
                                return 0;
                            }
                            char *s_buffer_id = value;
                            char *s_slice_id = p + 1;
                            *p = 0;

                            errno = 0;
                            size_t buffer_id = strtoll(s_buffer_id, NULL, 0);
                            if (errno != 0) {
                                parser_error(parser, event, "invalid buffer id: %s", s_buffer_id);
                                return 0;
                            }

                            errno = 0;
                            size_t slice_id = strtoll(s_slice_id, NULL, 0);
                            if (errno != 0) {
                                parser_error(parser, event, "invalid slice id: %s", s_slice_id);
                                return 0;
                            }

                            switch (state->context) {
                                case CONTEXT_THREAD:
                                    state->thread.buffer_id = buffer_id;
                                    state->thread.slice_id = slice_id;
                                    break;
                                default:
                                    parser_error(parser, event, "%s does not apply to %s",
                                                 key_str(state->key),
                                                 context_str(state->context));
                                    return 0;
                            }
                            break;

                        default:
                            parser_error(parser, event, "Unrecognized key: %d", state->key);
                            return 0;
                    }

                    state->state = STATE_MAPPING;
                    break;
                default:
                    parser_error(parser, event, "Unexpected event %d in state %s",
                                 event->type, state_str(state->state));
                    return 0;
            }
            break;

        default:
            parser_error(parser, event, "Reached invalid state: %d", state->state);
            return 0;
    }

    return 1;
}

int yaml_parse(const char *file, struct yaml_config *config) {
    FILE *f = fopen(file, "rb");
    if (!f) {
        perror(arguments.smogfile);
        return 1;
    }

    yaml_parser_t parser;
    yaml_event_t event;

    yaml_parser_initialize(&parser);
    yaml_parser_set_input_file(&parser, f);

    struct parser_state parser_state = {
        STATE_START,
        KEY_NONE,
        CONTEXT_ROOT,
        config,
        { 0 }, { 0 }, { 0 }
    };

    int res = 1;
    while (1) {
        res = yaml_parser_parse(&parser, &event);
        if (!res)
            break;

        res = consume_event(&parser, &parser_state, &event);
        if (!res)
            break;

        if (event.type == YAML_STREAM_END_EVENT) {
            yaml_event_delete(&event);
            break;
        }

        yaml_event_delete(&event);
    }

    if (!res) {
        fprintf(stderr, "%s:%zu:%zu: Error: %s\n",
                arguments.smogfile,
                parser.problem_mark.line,
                parser.problem_mark.column,
                parser.problem);
        return 1;
    }

    yaml_parser_delete(&parser);

    if (!arguments.verbose)
        return 0;

    printf("\n");
    printf("parsed smogfile:\n");
    printf("\n");
    printf("  buffers: %zu\n", config->nbuffers);

    for (size_t i = 0; i < config->nbuffers; ++i) {
        struct yaml_buffer *b = config->buffers + i;

        printf("\n");
        printf("    - buffer[%zu], %p\n", i, b);
        printf("      size  : %s\n", format_size_string(b->size));
        printf("      file  : %s\n", b->file);
        printf("      slices: %zu\n", b->nslices);

        for (size_t j = 0; j < b->nslices; ++j) {
            struct yaml_slice *s = b->slices + j;

            printf("\n");
            printf("        - slice[%zu], %p\n", j, s);
            printf("         *range : %s...%s\n", s->start_str, s->end_str);
            printf("          range : %#zx...%#zx\n", s->start, s->end);
            printf("          size  : %s\n", format_size_string(s->end - s->start + 1));
            printf("          group : %zu\n", s->group);
            printf("         *buffer: %p\n", s->buffer);
        }
    }

    printf("\n");
    printf("  threads: %zu\n", config->nthreads);

    for (size_t i = 0; i < config->nthreads; ++i) {
        struct yaml_thread *t = config->threads + i;

        printf("\n");
        printf("    - thread[%zu], %p\n", i, t);
        printf("      kernel: %s\n", kernel_to_string(t->kernel));
        printf("      buffer: %zu:%zu\n", t->buffer_id, t->slice_id);
        printf("     *slice : %p\n", t->slice);
        printf("      group : %zu\n", t->group);
    }

    return 0;
}
