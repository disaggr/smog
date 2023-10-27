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
#include "./kernel.h"

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
            std::cout << "[" << i << "] " << kernel_to_char(g_thread_options[i].kernel) << " " << work_items << " iterations";
            std::cout << " at " << (work_items * 1.0 / elapsed.count()) << " iterations/s, elapsed: " << elapsed.count() * 1000 << " ms";
            std::cout << ", " << (work_items * 1.0 / elapsed.count() * CACHE_LINE_SIZE / 1024 / 1024) << " MiB/s";
            std::cout << ", per iteration: " << elapsed.count() * 1000000000 / work_items << " nanoseconds";

            if (csv_file.is_open())
              csv_file << i << "," << work_items << ","<< elapsed.count() << std::endl;

            double current_rate = work_items * 1.0 / elapsed.count();
            if (g_thread_options[i].target_rate) {
                std::cout << " (" << (100.0 * current_rate / g_thread_options[i].target_rate) << "%)";

                // assuming a linear relationship between delay and dirty rate, the following equation holds:
                //
                //  current rate      target delay
                //  ------------  =  -------------
                //   target rate     current delay
                //
                // consequently, we can compensate for a deviation from the target dirty rate by adjusting
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
                // However, for a faster initial approximation, the ramp up phase is applied without smoothing,
                // until an epsilon of 0.05 tolerance is achieved.
                //
                size_t target_rate = g_thread_options[i].target_rate;
                size_t current_delay = g_thread_options[i].delay;

                double target_delay = current_rate * current_delay / target_rate;

                if (g_thread_options[i].adjust_phase < PHASE_CONSTANT_DELAY) {

                    if (g_thread_options[i].adjust_phase == PHASE_DYNAMIC_RAMP_UP) {
                        double epsilon = 0.05;
                        if (abs(target_delay - current_delay) < target_delay * epsilon) {
                            std::cout << "  reached target range after " << monitor_ticks << " ticks" << std::endl;
                            g_thread_options[i].adjust_phase = PHASE_STEADY_ADJUST;
                        }
                    }

                    if (g_thread_options[i].adjust_phase == PHASE_DYNAMIC_RAMP_UP) {
                        g_thread_options[i].delay = std::max(target_delay, 1.0);
                    } else {
                        g_thread_options[i].delay = std::max((target_delay + current_delay) / 2.0, 1.0);
                    }
                    g_kernels[i]->adjust_delay(g_thread_options[i].delay);

                    std::cout << "  adjusting delay: " << current_delay << " -> " << g_thread_options[i].delay << " (by " << (int)(g_thread_options[i].delay - current_delay) << ")" << std::endl;

                    std::chrono::duration<double> total_elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now - start);
                    if (g_thread_options[i].adjust_timeout > 0 && total_elapsed >= std::chrono::seconds(g_thread_options[i].adjust_timeout)) {
                        std::cout << "  reached adjustment timeout after " << monitor_ticks << " ticks" << std::endl;
                        g_thread_options[i].adjust_phase = PHASE_CONSTANT_DELAY;
                    }
                }
            }

            std::cout << std::endl;
        }
        double current_rate = sum * 1.0 / elapsed.count();
        std::cout << "total: " << sum << " cache lines";
        std::cout << " at " << current_rate << " cache lines/s";
        std::cout << ", " << (sum * 1.0 / elapsed.count() * CACHE_LINE_SIZE / 1024 / 1024) << " MiB/s";
        std::cout << ", per item: " << elapsed.count() * 1000000000 / sum * g_thread_count << " nanoseconds" << std::endl;

        monitor_ticks++;
    }

    for (size_t i = 0; i < g_thread_count; i++) {
        pthread_join(g_threads[i], NULL);
    }
    if (csv_file.is_open())
      csv_file.close();
}
