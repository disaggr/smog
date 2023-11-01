/*
 * Copyright (c) 2022 - 2023 OSM Group @ HPI, University of Potsdam
 */

#include "./lexer.h"

#include <stdarg.h>

#include "./parser.h"
#include "./smog.h"

#define CASE(s) case s: return #s
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

int yylex(YYSTYPE *yylval, YYLTYPE *yyloc, struct parser_state *state) {
    yaml_event_t event;
    int res = yaml_parser_parse(&state->parser, &event);
    if (!res) {
        yyloc->first_line = state->parser.problem_mark.line;
        yyloc->first_column = state->parser.problem_mark.column;
        yyloc->last_line = state->parser.problem_mark.line;
        yyloc->last_column = state->parser.problem_mark.column;

        yyerror(yyloc, state, "..TODO..");
        return YYerror;
    }

    yyloc->first_line = event.start_mark.line;
    yyloc->first_column = event.start_mark.column;
    yyloc->last_line = event.end_mark.line;
    yyloc->last_column = event.end_mark.column;

    if (arguments.verbose) {
        printf("event=%s",
               event_str(event.type));
        if (event.type == YAML_SCALAR_EVENT) {
            printf(" value=\"%s\"", event.data.scalar.value);
            printf(" is_key=%d", !state->next_scalar_is_value);
        }
        printf("\n");
    }

    yytoken_kind_t ret;

    int next_scalar_is_value = state->next_scalar_is_value;
    state->next_scalar_is_value = 0;

    switch (event.type) {
        case YAML_STREAM_START_EVENT:
            ret = YAML_STREAM_START;
            break;
        case YAML_STREAM_END_EVENT:
            ret = YAML_STREAM_END;
            break;
        case YAML_DOCUMENT_START_EVENT:
            ret = YAML_DOCUMENT_START;
            break;
        case YAML_DOCUMENT_END_EVENT:
            ret = YAML_DOCUMENT_END;
            break;
        case YAML_MAPPING_START_EVENT:
            ret = YAML_MAPPING_START;
            break;
        case YAML_MAPPING_END_EVENT:
            ret = YAML_MAPPING_END;
            break;
        case YAML_SEQUENCE_START_EVENT:
            ret = YAML_SEQUENCE_START;
            break;
        case YAML_SEQUENCE_END_EVENT:
            ret = YAML_SEQUENCE_END;
            break;
        case YAML_SCALAR_EVENT:
            char *value = (char*)event.data.scalar.value;

            if (next_scalar_is_value) {
                ret = YAML_SCALAR;
                yylval->value = strdup((char*)event.data.scalar.value);
            } else {
                if (!strcmp(value, "buffers")) {
                    ret = KEY_BUFFERS;
                } else if (!strcmp(value, "threads")) {
                    ret = KEY_THREADS;
                } else if (!strcmp(value, "size")) {
                    ret = KEY_SIZE;
                } else if (!strcmp(value, "file")) {
                    ret = KEY_FILE;
                } else if (!strcmp(value, "slices")) {
                    ret = KEY_SLICES;
                } else if (!strcmp(value, "range")) {
                    ret = KEY_RANGE;
                } else if (!strcmp(value, "group")) {
                    ret = KEY_GROUP;
                } else if (!strcmp(value, "kernel")) {
                    ret = KEY_KERNEL;
                } else if (!strcmp(value, "buffer")) {
                    ret = KEY_BUFFER;
                } else if (!strcmp(value, "delay")) {
                    ret = KEY_DELAY;
                } else if (!strcmp(value, "target_rate")) {
                    ret = KEY_TARGET_RATE;
                } else if (!strcmp(value, "adjust_timeout")) {
                    ret = KEY_ADJUST_TIMEOUT;
                } else {
                    ret = YYUNDEF;
                }
                state->next_scalar_is_value = 1;
            }
            break;
        case YAML_NO_EVENT:
            ret = YYEOF;
            break;
        default:
            ret = YYUNDEF;
            break;
    }

    yaml_event_delete(&event);
    return ret;
}

void yyerror(YYLTYPE *loc, struct parser_state *state, char const *s) {
    (void)state;

    fprintf(stderr, "%s:%u:%u: Error: %s\n",
            arguments.smogfile,
            loc->first_line,
            loc->first_column,
            s);
}

void yyferror(YYLTYPE *loc, struct parser_state *state, const char *f, ...) {
    char *buf = NULL;
    size_t size = 0;

    va_list ap;
    va_start(ap, f);
    int n = vsnprintf(buf, size, f, ap);
    va_end(ap);

    if (n < 0) {
        yyerror(loc, state, "invalid error format");
        return;
    }

    size = (size_t)n + 1;
    buf = malloc(size);
    if (!buf) {
        yyerror(loc, state, "unable to allocate memory");
        return;
    }

    va_start(ap, f);
    n = vsnprintf(buf, size, f, ap);
    va_end(ap);

    if (n < 0) {
        free(buf);
        yyerror(loc, state, "invalid error format");
        return;
    }

    yyerror(loc, state, buf);
}

