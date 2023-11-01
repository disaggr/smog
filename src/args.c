/*
 * Copyright (c) 2022 - 2023 OSM Group @ HPI, University of Potsdam
 */

#include "./args.h"

#include <stdlib.h>

#include "./util.h"
#include "./smog.h"

static const char doc[] = "A configurable memory workload generator and benchmark";
static const char args_doc[] = "SMOGFILE";

static struct argp_option options[] = {
    { "pagesize", 'S', "SIZE", 0, "pagesize to use for operations", 0 },
    { "monitor-interval", 'M', "INTERVAL", 0, "monitor and reporting interval in milliseconds", 0 },
    { "csv", 'c', "FILE", 0, "output measurements in csv format to FILE", 0 },
    { "verbose", 'v', 0, 0, "show additional output", 0 },
    { 0 }
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    struct arguments *arguments = (struct arguments*)state->input;

    switch (key) {
        case 'S':
            errno = 0;
            arguments->pagesize = parse_size_string(arg);
            if (errno != 0)
                argp_failure(state, 1, errno, "invalid pagesize: %s", arg);
            break;
        case 'M':
            errno = 0;
            arguments->monitor_interval = strtoll(arg, NULL, 10);
            if (errno != 0)
                argp_failure(state, 1, errno, "invalid interval: %s", arg);
            break;
        case 'c':
            arguments->output_format = CSV;
            arguments->output_file = arg;
            break;
        case 'v':
            arguments->verbose = 1;
            break;

        case ARGP_KEY_ARG:
            if (state->arg_num >= 1)
                argp_usage(state);
            arguments->smogfile = arg;
            break;

        case ARGP_KEY_END:
            if (state->arg_num < 1)
                argp_usage(state);
            break;

        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

struct argp argp = { options, parse_opt, args_doc, doc, NULL, NULL, NULL };
