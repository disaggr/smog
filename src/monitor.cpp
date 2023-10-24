/*
 * Copyright (c) 2022 - 2023 OSM Group @ HPI, University of Potsdam
 */

#include "./monitor.h"

#include <cstddef>
#include <chrono>
#include <thread>
#include <iostream>
#include <fstream>

#include "./smog.h"

int monitor_run() {
    std::ofstream csv_file;
    if (arguments.output_format == CSV) {
        csv_file.open(arguments.output_file);
        csv_file << "thread,pages,elapsed" << std::endl;
    }

    const int PHASE_DYNAMIC_RAMP_UP = 0;
    const int PHASE_STEADY_ADJUST   = 1;
    const int PHASE_CONSTANT_DELAY  = 2;

    size_t monitor_interval = arguments.monitor_interval;  // ms

    size_t monitor_ticks = 0;
    int phase = PHASE_DYNAMIC_RAMP_UP;

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point prev = start;

    while (1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(monitor_interval));
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now - prev);
        prev = now;

        size_t sum = 0;

        for (size_t i = 0; i < g_thread_count; ++i) {
            // NOTE: for very fast kernels, this loses us a couple iterations between this line and the next
            size_t work_items = g_thread_status[i].count - g_thread_status[i].last_count;
            g_thread_status[i].last_count = g_thread_status[i].count;

            sum += work_items;
            // FIXME
            //std::cout << "[" << i << "] " << kernels[i] << " " << work_items << " iterations";
            std::cout << " at " << (work_items * 1.0 / elapsed.count()) << " iterations/s, elapsed: " << elapsed.count() * 1000 << " ms";
            std::cout << ", " << (work_items * 1.0 / elapsed.count() * CACHE_LINE_SIZE / 1024 / 1024) << " MiB/s";
            std::cout << ", per iteration: " << elapsed.count() * 1000000000 / work_items << " nanoseconds" << std::endl;

            if (csv_file.is_open())
              csv_file << i << "," << work_items << ","<< elapsed.count() << std::endl;
        }
        double current_rate = sum * 1.0 / elapsed.count();
        std::cout << "total: " << sum << " cache lines";
        std::cout << " at " << current_rate << " cache lines/s";
        std::cout << ", " << (sum * 1.0 / elapsed.count() * CACHE_LINE_SIZE / 1024 / 1024) << " MiB/s";
        // FIXME: auto adjustment per thread
        //if (target_rate) {
        //    std::cout << " (" << (100.0 * current_rate / target_rate) << "%)";
        //}
        std::cout << ", per item: " << elapsed.count() * 1000000000 / sum * g_thread_count << " nanoseconds";
        // FIXME: overpaging
        //if (current_rate > smog_pages) {
        //
        //    // If the rate of pages/s exceeds the amount of available pages, then pages are touched
        //    // multiple times per second. this changes the effectiveness of the smog dirty rate and
        //    // is reported as a metric of 'overpaging', if present.
        //    //
        //    // Overpaging in this context is a percentage of pages touched per second in the last
        //    // measuring interval that exceed the number of allocated pages for dirtying.
        //    //
        //    // For example, a smog run on 10000 allocated pages that touched 20000 pages per second
        //    // would be reported as 100% overpaging by the code below.
        //    std::cout << ", Overpaging: " << (100.0 * (current_rate / smog_pages - 1)) << "%";
        //}
        std::cout << std::endl;

        // FIXME: auto adjust
        // if (target_rate) {
        //     // assuming a linear relationship between delay and dirty rate, the following equation holds:
        //     //
        //     //  current rate      target delay
        //     //  ------------  =  -------------
        //     //   target rate     current delay
        //     //
        //     // consequently, we can compensate for a deviation from the target dirty rate by adjusting
        //     // the new delay value to:
        //     //
        //     //                 current rate
        //     //  target delay = ------------ * current delay
        //     //                  target rate
        //     //
        //     // assuming that all values are > 0.
        //     // for a smoother transition, we will adjust the new delay to:
        //     //
        //     //              current delay + target delay
        //     //  new delay = ----------------------------
        //     //                           2
        //     //
        //     // However, for a faster initial approximation, the ramp up phase is applied without smoothing,
        //     // until an epsilon of 0.05 tolerance is achieved.
        //     //
        //     size_t current_delay = g_smog_delay;
        //     double target_delay = current_rate * current_delay / target_rate;

        //     if (phase < PHASE_CONSTANT_DELAY) {

        //         if (phase == PHASE_DYNAMIC_RAMP_UP) {
        //             double epsilon = 0.05;
        //             if (abs(target_delay - current_delay) < target_delay * epsilon) {
        //                 std::cout << "  reached target range after " << monitor_ticks << " ticks" << std::endl;
        //                 phase = PHASE_STEADY_ADJUST;
        //             }
        //         }

        //         if (phase == PHASE_DYNAMIC_RAMP_UP) {
        //             g_smog_delay = std::max(target_delay, 1.0);
        //         } else {
        //             g_smog_delay = std::max((target_delay + current_delay) / 2.0, 1.0);
        //         }

        //         std::cout << "  adjusting delay: " << current_delay << " -> " << g_smog_delay << " (by " << (int)(g_smog_delay - current_delay) << ")" << std::endl;

        //         std::chrono::duration<double> total_elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now - start);
        //         if (g_smog_timeout > 0 && total_elapsed >= std::chrono::seconds(g_smog_timeout)) {
        //             std::cout << "  reached adjustment timeout after " << monitor_ticks << " ticks" << std::endl;
        //             phase = PHASE_CONSTANT_DELAY;
        //         }
        //     }
        // }

        monitor_ticks++;
    }

    for (size_t i = 0; i < g_thread_count; i++) {
        pthread_join(g_threads[i], NULL);
    }
    if (csv_file.is_open())
      csv_file.close();
}
