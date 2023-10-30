/*
 * Copyright (c) 2022 - 2023 OSM Group @ HPI, University of Potsdam
 */

// TODO: enable locations

%define parse.error detailed
%define api.pure full
%locations
%define api.location.type { YYLTYPE }

%code requires {

#include <yaml.h>

#include "kernels/kernels.h"

struct parser_state {
    yaml_parser_t parser;
    int next_scalar_is_value;
};

typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;

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
    char *start_str;
    char *end_str;
    size_t start;
    size_t end;
    size_t group;
    struct yaml_buffer *buffer;
};

struct yaml_thread {
    enum kernel kernel;
    size_t buffer_id;
    size_t slice_id;
    size_t delay;
    size_t target_rate;
    size_t adjust_timeout;
    size_t group;
    struct yaml_slice *slice;
};

}

%code provides {

#ifdef __cplusplus
extern "C" {
#endif

int yaml_parse(const char *file, struct yaml_config *config);

#ifdef __cplusplus
}
#endif

}

%{

#include "./parser.h"

#include <stdio.h>
#include <stdarg.h>

#include "./lexer.h"
#include "./smog.h"
#include "./util.h"

static void thread_destroy(struct parsed_thread *t) {
    (void)t;  // noop;
}

static void slice_destroy(struct parsed_slice *s) {
    free(s->lower);
}

static void buffer_destroy(struct parsed_buffer *b) {
    for (size_t i = 0; i < b->slices.n; ++i)
        slice_destroy(b->slices.slices + i);
    free(b->slices.slices);
    free(b->file);
}

%}

%parse-param { struct parser_state *state }
%lex-param   { struct parser_state *state }
%parse-param { struct yaml_config *config }

%union {
    char *value;

    struct parsed_thread {
        enum kernel kernel;
        size_t buffer_id;
        size_t slice_id;
        YYLTYPE buffer_mark;
        size_t delay;
        size_t target_rate;
        size_t adjust_timeout;
        size_t group;
    } thread;

    struct parsed_threads {
        struct parsed_thread *threads;
        size_t n;
    } threads;

    struct parsed_slice {
        char *lower;
        char *upper;
        YYLTYPE range_mark;
        size_t group;
    } slice;

    struct parsed_slices {
        struct parsed_slice *slices;
        size_t n;
    } slices;

    struct parsed_buffer {
        size_t size;
        char *file;
        struct parsed_slices slices;
    } buffer;

    struct parsed_buffers {
        struct parsed_buffer *buffers;
        size_t n;
    } buffers;

    struct parsed_config {
        struct parsed_buffers buffers;
        struct parsed_threads threads;
    } config;
}

%token         YAML_STREAM_START
%token         YAML_STREAM_END
%token         YAML_DOCUMENT_START
%token         YAML_DOCUMENT_END
%token         YAML_MAPPING_START
%token         YAML_MAPPING_END
%token         YAML_SEQUENCE_START
%token         YAML_SEQUENCE_END
%token <value> YAML_SCALAR

%token         KEY_BUFFERS
%token         KEY_THREADS
%token         KEY_SIZE
%token         KEY_FILE
%token         KEY_SLICES
%token         KEY_RANGE
%token         KEY_GROUP
%token         KEY_KERNEL
%token         KEY_BUFFER
%token         KEY_DELAY
%token         KEY_TARGET_RATE
%token         KEY_ADJUST_TIMEOUT

%type <buffer>  Buffer
%type <slice>   Slice
%type <thread>  Thread

%type <buffers> Buffers
%type <slices>  Slices
%type <threads> Threads

%type <config>  Document

%% /* Grammar */

start
    : YAML_STREAM_START
        YAML_DOCUMENT_START
          YAML_MAPPING_START
            Document
          YAML_MAPPING_END
        YAML_DOCUMENT_END
      YAML_STREAM_END
      {
        config->nbuffers = $4.buffers.n;
        config->nthreads = $4.threads.n;

        config->buffers = calloc($4.buffers.n, sizeof(*config->buffers));
        config->threads = calloc($4.threads.n, sizeof(*config->threads));

        if (!config->buffers || !config->threads) {
            yyerror(&@4, state, config, "unable to allocate memory");
            return -1;
        }

        // convert parsed_buffers to yaml_buffers
        for (size_t i = 0; i < $4.buffers.n; ++i) {
            struct yaml_buffer *b = config->buffers + i;
            struct parsed_buffer *pb = $4.buffers.buffers + i;

            b->size = pb->size;
            b->file = pb->file; pb->file = NULL;

            b->nslices = pb->slices.n;
            b->slices = calloc(pb->slices.n, sizeof(*b->slices));
            if (!b->slices) {
                yyerror(&@4, state, config, "unable to allocate memory");
                return -1;
            }
 
            // convert parsed_slices to yaml_slices
            for (size_t j = 0; j < pb->slices.n; ++j) {
                struct yaml_slice *s = b->slices + j;
                struct parsed_slice *ps = pb->slices.slices + j;

                s->start_str = ps->lower; ps->lower = NULL;
                s->end_str = ps->upper;   ps->upper = NULL;

                size_t len = strlen(s->start_str);
                if (s->start_str[len - 1] == '%') {
                    errno = 0;
                    size_t percent = strtoll(s->start_str, NULL, 0);
                    if (errno != 0 || percent > 100) {
                        yyferror(&ps->range_mark, state, config, "invalid percentage: %s",
                                 s->start_str);
                        return 0;
                    }

                    s->start = percent * b->size / 100;
                } else {
                    errno = 0;
                    s->start = parse_size_string(s->start_str);
                    if (errno != 0) {
                        yyferror(&ps->range_mark, state, config, "invalid size: %s",
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
                        yyferror(&ps->range_mark, state, config, "invalid percentage: %s",
                                 end_str);
                        return 0;
                    }

                    s->end = percent * b->size / 100 + offset;
                } else {
                    errno = 0;
                    s->end = parse_size_string(end_str) + offset;
                    if (errno != 0) {
                        yyferror(&ps->range_mark, state, config, "invalid size: %s",
                                 end_str);
                        return 0;
                    }
                }

                s->group = ps->group;
                s->buffer = b;
            }
        }

        for (size_t i = 0; i < $4.threads.n; ++i) {
            struct yaml_thread *t = config->threads + i;
            struct parsed_thread *pt = $4.threads.threads + i;

            t->kernel = pt->kernel;
            t->buffer_id = pt->buffer_id;
            t->slice_id = pt->slice_id;
            t->delay = pt->delay;
            t->target_rate = pt->target_rate;
            t->adjust_timeout = pt->adjust_timeout;
            t->group = pt->group;
            
            if (pt->buffer_id >= config->nbuffers) {
                yyferror(&pt->buffer_mark, state, config, "Buffer id out of range: %zu",
                         pt->buffer_id);
                return -1;
            }

            struct yaml_buffer *b = config->buffers + pt->buffer_id;
            
            if (pt->slice_id >= b->nslices) {
                yyferror(&pt->buffer_mark, state, config, "Slice id out of range: %zu",
                         pt->slice_id);
                return -1;
            }

            t->slice = b->slices + pt->slice_id;
        }
      }
    ;

Document
    : Document KEY_BUFFERS YAML_SEQUENCE_START Buffers YAML_SEQUENCE_END
      {
        $$ = $1;

        // the buffers are a list of parsed_buffer structs
        // free previous value now to avoid memory leaks on double assignment
        for (size_t i = 0; i < $$.buffers.n; ++i)
            buffer_destroy($$.buffers.buffers + i);
        free($$.buffers.buffers);
        $$.buffers = $4;
      }
    | Document KEY_THREADS YAML_SEQUENCE_START Threads YAML_SEQUENCE_END
      {
        $$ = $1;

        // the threads are a list of parsed_thread structs
        // free previous value now to avoid memory leaks on double assignment
        for (size_t i = 0; i < $$.threads.n; ++i)
            thread_destroy($$.threads.threads + i);
        free($$.threads.threads);
        $$.threads = $4;
      }
    | /* empty */
      {
        struct parsed_config c = { 0 };
        $$ = c;
      }
    ;

Buffers
    : Buffers YAML_MAPPING_START Buffer YAML_MAPPING_END
      {
        $$ = $1;

        struct parsed_buffer *b = realloc($$.buffers, sizeof(*b) * ($$.n + 1));
        if (!b) {
            yyerror(&@3, state, config, "unable to allocate memory");
            return -1;
        }
        $$.buffers = b;
        $$.buffers[$$.n++] = $3;
      }
    | /* empty */
      {
        struct parsed_buffers b = { 0 };
        $$ = b;
      }
    ;

Buffer
    : Buffer KEY_SIZE YAML_SCALAR
      {
        $$ = $1;

        // the size is parsed from a size string
        errno = 0;
        size_t size = parse_size_string($3);
        if (errno != 0) {
            yyferror(&@3, state, config, "invalid size: %s", $3);
            return -1;
        }
        free($3);
        $$.size = size;
      }
    | Buffer KEY_FILE YAML_SCALAR
      {
        $$ = $1;

        // the file is a string
        // free previous value now to avoid memory leaks on double assignment
        free($$.file);

        $$.file = $3;
      }
    | Buffer KEY_SLICES YAML_SEQUENCE_START Slices YAML_SEQUENCE_END
      {
        $$ = $1;

        // the slices are a list of parsed_slice structs
        // free previous value now to avoid memory leaks on double assignment
        for (size_t i = 0; i < $$.slices.n; ++i)
            slice_destroy($$.slices.slices + i);
        free($$.slices.slices);

        $$.slices = $4;
      }
    | /* empty */
      {
        struct parsed_buffer b = { 0 };
        $$ = b;
      }
    ;

Slices
    : Slices YAML_MAPPING_START Slice YAML_MAPPING_END
      {
        $$ = $1;

        struct parsed_slice *s = realloc($$.slices, sizeof(*s) * ($$.n + 1));
        if (!s) {
            yyerror(&@3, state, config, "unable to allocate memory");
            return -1;
        }
        $$.slices = s;
        $$.slices[$$.n++] = $3;
      }
    | /* empty */
      {
        struct parsed_slices s = { 0 };
        $$ = s;
      }
    ;

Slice
    : Slice KEY_RANGE YAML_SCALAR
      {
        $$ = $1;

        // the range is in the form [lower]...[upper], where upper may be
        // prefixed with < to mean an exclusive range.
        //
        // we store this as string now and convert later when the buffer is parsed
        char *p = strstr($3, "...");
        if (!p) {
            yyferror(&@3, state, config, "invalid range: %s", $3);
            return -1;
        }

        // free previous value now to avoid memory leaks on double assignment
        free($$.lower);

        $$.lower = $3;
        $$.upper = p + 3;
        $$.range_mark = @3;
        *p = 0;
      }
    | Slice KEY_GROUP YAML_SCALAR
      {
        $$ = $1;

        // the group is a single integer
        errno = 0;
        size_t group = strtoll($3, NULL, 0);
        if (errno != 0) {
            yyferror(&@3, state, config, "invalid group: %s", $3);
            return -1;
        }
        free($3);
        $$.group = group;
      }
    | /* empty */
      {
        struct parsed_slice s = { 0 };
        $$ = s;
      }
    ;

Threads
    : Threads YAML_MAPPING_START Thread YAML_MAPPING_END
      {
        $$ = $1;

        struct parsed_thread *t = realloc($$.threads, sizeof(*t) * ($$.n + 1));
        if (!t) {
            yyerror(&@3, state, config, "unable to allocate memory");
            return -1;
        }
        $$.threads = t;
        $$.threads[$$.n++] = $3;
      }
    | /* empty */
      {
        struct parsed_threads t = { 0 };
        $$ = t;
      }
    ;

Thread
    : Thread KEY_KERNEL YAML_SCALAR
      {
        $$ = $1;

        // the kernel is a string literal we can map to an enum
        enum kernel kernel = kernel_from_string($3);
        if (kernel == KERNEL_UNKNOWN) {
            yyferror(&@3, state, config, "Unrecognized kernel: %s", $3);
            return -1;
        }
        free($3);
        $$.kernel = kernel;
      }
    | Thread KEY_BUFFER YAML_SCALAR
      {
        $$ = $1;

        // the buffer is a tuple of buffer_id and slice_id
        char *p = strchr($3, ':');
        if (!p) {
            yyferror(&@3, state, config, "invalid buffer: %s", $3);
            return -1;
        }
        char *s_buffer_id = $3;
        char *s_slice_id = p + 1;
        *p = 0;

        errno = 0;
        size_t buffer_id = strtoll(s_buffer_id, NULL, 0);
        if (errno != 0) {
            yyferror(&@3, state, config, "invalid buffer id: %s", s_buffer_id);
            return -1;
        }
        errno = 0;
        size_t slice_id = strtoll(s_slice_id, NULL, 0);
        if (errno != 0) {
            yyferror(&@3, state, config, "invalid slice id: %s", s_slice_id);
            return -1;
        }
        free($3);
        $$.buffer_id = buffer_id;
        $$.slice_id = slice_id;
      }
    | Thread KEY_DELAY YAML_SCALAR
      {
        $$ = $1;

        // the delay is a single integer
        errno = 0;
        size_t delay = strtoll($3, NULL, 0);
        if (errno != 0) {
            yyferror(&@3, state, config, "invalid delay: %s", $3);
            return -1;
        }
        free($3);
        $$.delay = delay;
      }
    | Thread KEY_GROUP YAML_SCALAR
      {
        $$ = $1;

        // the group is a single integer
        errno = 0;
        size_t group = strtoll($3, NULL, 0);
        if (errno != 0) {
            yyferror(&@3, state, config, "invalid group: %s", $3);
            return -1;
        }
        free($3);
        $$.group = group;
      }
    | Thread KEY_TARGET_RATE YAML_SCALAR
      {
        $$ = $1;

        // the target_rate is a single integer
        errno = 0;
        size_t target_rate = strtoll($3, NULL, 0);
        if (errno != 0) {
            yyferror(&@3, state, config, "invalid target_rate: %s", $3);
            return -1;
        }
        free($3);
        $$.target_rate = target_rate;
      }
    | Thread KEY_ADJUST_TIMEOUT YAML_SCALAR
      {
        $$ = $1;

        // the adjust_timeout is a single integer
        errno = 0;
        size_t adjust_timeout = strtoll($3, NULL, 0);
        if (errno != 0) {
            yyferror(&@3, state, config, "invalid adjust_timeout: %s", $3);
            return -1;
        }
        free($3);
        $$.adjust_timeout = adjust_timeout;
      }
    | /* empty */
      {
        struct parsed_thread t = { 0 };
        $$ = t;
      }
    ;

%% /* Footer */

int yaml_parse(const char *file, struct yaml_config *config) {
    FILE *f = fopen(file, "rb");
    if (!f) {
        perror(arguments.smogfile);
        return 1;
    }

    yaml_parser_t parser;
    yaml_parser_initialize(&parser);
    yaml_parser_set_input_file(&parser, f);

    struct parser_state state = { parser, 0 };
    int res = yyparse(&state, config);

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
        printf("      delay : %zu\n", t->delay);
        printf("      group : %zu\n", t->group);
    }

    return res;
}
