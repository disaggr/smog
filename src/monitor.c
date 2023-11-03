/*
 * Copyright (c) 2022 - 2023 OSM Group @ HPI, University of Potsdam
 */

#include "./monitor.h"

#include <stddef.h>
#include <time.h>
#include <math.h>
#include <errno.h>

#include "./smog.h"
#include "./util.h"
#include "./thread.h"
#include "./kernel.h"

int monitor_run(size_t nthreads, struct smog_thread *threads) {
    FILE *csv_file = NULL;
    if (arguments.output_format == CSV) {
        csv_file = fopen(arguments.output_file, "w");
        if (!csv_file) {
            fprintf(stderr, "%s: fopen: %s\n", arguments.output_file, strerror(errno));
            return -1;
        }
        fprintf(csv_file, "thread,pages,elapsed\n");
    }

    struct timespec delay = TIMESPEC_FROM_MILLIS(arguments.monitor_interval);
    size_t monitor_ticks = 0;

    struct timespec start;
    struct timespec prev;
    struct timespec now;
    int res = clock_gettime(CLOCK_MONOTONIC, &prev);
    if (res != 0) {
        perror("clock_gettime");
        return res;
    }
    start = prev;

    while (1) {
        // delay
        res = nanosleep(&delay, NULL);
        if (res != 0) {
            perror("nanosleep");
            return res;
        }

        clock_gettime(CLOCK_MONOTONIC, &now);
        double elapsed = (now.tv_sec - prev.tv_sec) * 1000.0 +
                         (now.tv_nsec - prev.tv_nsec) / 1000000.0;  // ms
        prev = now;

        size_t sum = 0;
        for (size_t i = 0; i < nthreads; ++i) {
            struct smog_thread *thread = threads + i;
            struct thread_status *status = thread->status;

            // NOTE: for very fast kernels, this loses us a couple iterations
            size_t work_items = status->count - status->last_count;
            status->last_count = status->count;

            sum += work_items;
            printf("[%zu] %c %zu iterations",
                   i, kernel_to_char(thread->kernel), work_items);
            printf(" at %.2f iterations/s, elapsed: %.2f ms",
                   work_items / elapsed * 1000, elapsed);
            printf(", %.2f MiB/s",
                   work_items / elapsed * 1000 * CACHE_LINE_SIZE / 1024 / 1024);
            printf(", per iteration: %.2f ns",
                   elapsed * 1000000 / work_items);

            if (csv_file) {
                fprintf(csv_file, "%zu,%zu,%f\n",
                        i, work_items, elapsed);
            }

            double current_rate = work_items / elapsed * 1000;
            if (thread->target_rate) {
                printf(" (%f%%)",
                       current_rate / thread->target_rate * 100);
            }

            printf("\n");

            if (thread->target_rate) {
                // assuming a linear relationship between delay and dirty rate, it follows:
                //
                //  current rate      target delay
                //  ------------  =  -------------
                //   target rate     current delay
                //
                // we can compensate for a deviation from the target dirty rate by adjusting
                // the new delay value to:
                //
                //                 current rate
                //  target delay = ------------ * current delay
                //                  target rate
                //
                // assuming that all values are > 0.
                // for a smoother transition, we will adjust the new delay to:
                //
                //              current delay + target delay
                //  new delay = ----------------------------
                //                           2
                //
                // However, for a faster initial approximation, the ramp up phase is applied
                // without smoothing, until an epsilon of 0.05 tolerance is achieved.
                //
                size_t target_rate = thread->target_rate;
                size_t current_delay = thread->delay;

                double target_delay = current_rate * current_delay / target_rate;

                if (thread->adjust_phase < PHASE_CONSTANT_DELAY) {
                    if (thread->adjust_phase == PHASE_DYNAMIC_RAMP_UP) {
                        double epsilon = 0.05;
                        if (fabs(target_delay - current_delay) < target_delay * epsilon) {
                            printf("  reached target range after %zu ticks\n", monitor_ticks);
                            thread->adjust_phase = PHASE_STEADY_ADJUST;
                        }
                    }

                    size_t new_delay = 0;
                    if (thread->adjust_phase == PHASE_DYNAMIC_RAMP_UP) {
                        new_delay = target_delay;
                    } else {
                        new_delay = (target_delay + current_delay) / 2.0;
                    }
                    thread->delay = new_delay < 1 ? 1 : new_delay;

                    printf("  adjusting delay: %zu -> %zu (by %zi)\n",
                           current_delay, thread->delay, thread->delay - current_delay);

                    double total = (now.tv_sec - start.tv_sec) * 1.0 +
                                   (now.tv_nsec - start.tv_nsec) / 1000000000.0;  // s
                    if (thread->adjust_timeout > 0 && total >= thread->adjust_timeout) {
                        printf("  reached adjustment timeout after %zu ticks\n",
                               monitor_ticks);
                        thread->adjust_phase = PHASE_CONSTANT_DELAY;
                    }
                }
            }
        }

        double current_rate = sum / elapsed;
        printf("total: %zu cache lines at %.2f cache lines/s",
               sum, current_rate);
        printf(", %.2f MiB/s, per item: %.2f ns",
               sum / elapsed * 1000 * CACHE_LINE_SIZE / 1024 / 1024,
               elapsed * 1000000 / sum * nthreads);

        monitor_ticks++;
    }
}
