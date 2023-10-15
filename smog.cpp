#include <thread>
#include <iostream>
#include <fstream>
#include <sys/mman.h>
#include <cerrno>
#include <unistd.h>
#include <vector>
#include <cstring>
#include <boost/program_options.hpp>
#include <chrono>
#include <errno.h>
#include <iostream>
#include <fcntl.h>

#include "smog.h"
#include "linear_scan.h"
#include "random_access.h"
#include "random_write.h"
#include "pointer_chase.h"
#include "cold.h"
#include "dirty_pages.h"

using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::system_clock;


namespace popts = boost::program_options;

// globals
size_t g_smog_pagesize;
size_t g_smog_delay;
size_t g_smog_timeout;

struct thread_status_t *g_thread_status;
pthread_barrier_t g_initalization_finished;

long double get_unixtime() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (long double)tv.tv_sec+(long double)(tv.tv_usec)/1e6;
}

int main(int argc, char* argv[]) {
	size_t threads = 0;
	std::vector<std::string> kernel_groups = {};
	std::vector<char> kernels = {};
	
	size_t system_pages = 0;
	size_t system_pagesize = 0;
	size_t smog_pages = 0;
	size_t hardware_concurrency = 0;
	size_t target_rate = 0; // pages/s
	size_t monitor_interval = 0; // ms

	// determine system characteristics
	system_pages = sysconf(_SC_PHYS_PAGES);
	system_pagesize = sysconf(_SC_PAGE_SIZE);
	size_t cache_line_size = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
	hardware_concurrency = std::thread::hardware_concurrency();

	// per default, spawn one SMOG thread per core
	size_t default_threads = hardware_concurrency;
	// and one dirty page kernel per thread in a shared group
	kernel_groups.push_back(std::string(default_threads, 'd'));
	for(std::string ks : kernel_groups) {
		for(uint64_t i = 0; i < ks.size(); i++) {
			if(i > 0 && ks[i] == 'p') {
				std::cout << "If the pointerchase kernel is used, put it at the beginning because. Just this kernel is initialized and pointer chase needs shuffling." << std::endl;
				return 1;
			}
			kernels.push_back(ks[i]);
		}
	}
	// per default, allocate 2 GiB in memory pages for SMOG
	size_t default_pages = std::min( (2UL * 1024 * 1024 * 1024) / system_pagesize, system_pages / 2);
	// per default, use a delay of 1000ns in the SMOG threads
	size_t default_delay = 1000; // ns
	// per default, use a monitor interval of 1000ms in the monitor
	size_t default_interval = 1000; // ms
	// per default, keep self-adjusting
	size_t default_timeout = 0; // s

	// parse CLI options
	popts::options_description description("SMOG Usage");
	description.add_options()
		("help,h", "Display this help message")
		("threads,t", popts::value<size_t>()->default_value(default_threads), "Number of threads to spawn")
		("kernels,k", popts::value<std::vector<std::string>>()->multitoken(), "For each thread you can specifiy a kernel to execute: (l)inear, (r)andom, random (w)rite, (p)ointerchase, (c)old, (d)irty pages")
		("pages,p", popts::value<size_t>()->default_value(default_pages), "Number of pages to allocate")
		("page-size,S", popts::value<size_t>(), "Page size to use for operations and reporting")
		("allocation-type,a", popts::value<char>(), "Specify if the allocation happens (g)lobally using mmap or thread-(l)ocal using malloc")
		("delay,d", popts::value<size_t>()->default_value(default_delay), "Delay in nanoseconds per thread per iteration")
		("rate,r", popts::value<std::string>(), "Target dirty rate to automatically adjust delay")
		("adjust-timeout,R", popts::value<size_t>()->default_value(default_timeout), "Timeout in seconds for automatic adjustment")
		("monitor-interval,M", popts::value<size_t>()->default_value(default_interval), "Monitor interval in milliseconds")
		("file-backing,f", popts::value<std::string>(), "Location of file used for mmap")
		("csv,c", popts::value<std::string>(), "Output measurements in csv format to");

	popts::variables_map arguments;
	popts::store(popts::command_line_parser(argc, argv).options(description).run(), arguments);
	popts::notify(arguments);

	if(arguments.count("help")) {
		std::cout << description << std::endl;
		return 0;
	}

	std::cout << "SMOG dirty-page benchmark" << std::endl;
	std::cout << "System page size:       " << (system_pagesize << 10) << " KiB" << std::endl;

	g_smog_pagesize = system_pagesize;
	if(arguments.count("page-size")) {
		g_smog_pagesize = arguments["page-size"].as<size_t>();
		std::cout << "Logical page size:      " << (g_smog_pagesize << 10) << " KiB" << std::endl;
	}

	std::cout << "System cache line size: " << cache_line_size << " Bytes" << std::endl;

	// assert for correct cache line size
	if (cache_line_size != CACHE_LINE_SIZE) {
		std::cerr << "error: built with incorrect cache line size: " << CACHE_LINE_SIZE << " Bytes. expected: " << cache_line_size << " Bytes" << std::endl;
		return 2;
	}

	if(arguments.count("threads")) {
		threads = arguments["threads"].as<size_t>();

		kernel_groups.clear();
		kernel_groups.push_back(std::string(threads, 'd'));

		kernels.clear();
		for(std::string ks : kernel_groups) {
			for(uint64_t i = 0; i < ks.size(); i++) {
				if(i > 0 && ks[i] == 'p') {
					std::cout << "If the pointerchase kernel is used, put it at the beginning because. Just this kernel is initialized and pointer chase needs shuffling." << std::endl;
					return 1;
				}
				kernels.push_back(ks[i]);
			}
		}
	}

	if(arguments.count("kernels")) {
		size_t size = 0;
		kernel_groups.clear();
		kernel_groups = arguments["kernels"].as<std::vector<std::string>>();
		for(std::string ks : kernel_groups) {
			size += ks.size();
			for(uint64_t i = 0; i < ks.size(); i++) {
				if(i > 0 && ks[i] == 'p') {
					std::cout << "If the pointerchase kernel is used, put it at the beginning because. Just this kernel is initialized and pointer chase needs shuffling." << std::endl;
					return 1;
				}
				kernels.push_back(ks[i]);
			}
		}
		if(size != threads) {
			std::cout << "Number of kernels and threads must match." << std::endl;
			return 0;
		}
	}

	if(arguments.count("pages")) {
		smog_pages = arguments["pages"].as<size_t>();
	}

	if(arguments.count("delay")) {
		g_smog_delay = arguments["delay"].as<size_t>();
	}

	std::ofstream csv_file;
	if (arguments.count("csv")){
		csv_file.open(arguments["csv"].as<std::string>());
		csv_file << "thread,pages,elapsed" << std::endl;
	}

	if(arguments.count("rate")) {
		std::string target_rate_str = arguments["rate"].as<std::string>();

		if (target_rate_str.back() == 'B') {
			// determine possible size suffix
			target_rate_str.pop_back();
			size_t multiplier = 1000;
			if (target_rate_str.back() == 'i') {
				// SI units
				multiplier = 1024;
				target_rate_str.pop_back();
			}
			// determine magnitude
			size_t size_unit = 1;
			switch (target_rate_str.back()) {
			case 'K':
				size_unit = multiplier;
				break;
			case 'M':
				size_unit = multiplier * multiplier;
				break;
			case 'G':
				size_unit = multiplier * multiplier * multiplier;
				break;
			case 'T':
				size_unit = multiplier * multiplier * multiplier * multiplier;
				break;
			default:
				std::cerr << "error: " << arguments["rate"].as<std::string>() << ": unrecognized unit specifier: " << target_rate_str.back() << std::endl;
				return 1;
			}
			target_rate = strtoll(target_rate_str.c_str(), NULL, 0) * size_unit / g_smog_pagesize;
			std::cout << "  target dirty rate: " << arguments["rate"].as<std::string>() << " (" << target_rate << " pages/s)" << std::endl;
		} else {
			// otherwise, interpret as pages/s
			target_rate = strtoll(target_rate_str.c_str(), NULL, 0);
			if (g_smog_pagesize != system_pagesize) {
				target_rate *= (system_pagesize / g_smog_pagesize);
			}
			std::cout << "  target dirty rate: " << target_rate << " pages/s" << std::endl;
		}
	}

	if(arguments.count("adjust-timeout")) {
		g_smog_timeout = arguments["adjust-timeout"].as<size_t>();
	}

	if (arguments.count("monitor-interval")) {
		monitor_interval = arguments["monitor-interval"].as<size_t>();
	}

	// allocate global SMOG page buffer
	void *buffer;
	if(arguments.count("file-backing")) {
		std::string file_backing = std::string(arguments["file-backing"].as<std::string>());
		int fd;
		if ((fd = open(file_backing.c_str(), O_RDWR|O_SYNC)) < 0 ) {
			std::cout << "open failed: " << std::strerror(errno) << std::endl;
			close(fd);
			return 1;
		}
		buffer = mmap(NULL, smog_pages * g_smog_pagesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		close(fd);
	}
	else {
		buffer = mmap(NULL, smog_pages * g_smog_pagesize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	}

	if(buffer == MAP_FAILED) {
		std::cout << "mmap failed: " << std::strerror(errno) << std::endl;
		return 1;
	}
	else {
		std::cout << "Allocated " << smog_pages << " pages (" << (smog_pages * (g_smog_pagesize >> 10)) << "KiB)" << std::endl;
	}

	// launch SMOG threads
	std::cout << "Creating " << threads << " SMOG threads" << std::endl;

	g_thread_status = new struct thread_status_t[threads];
	pthread_barrier_init(&g_initalization_finished, NULL, threads + 1);

	std::vector<std::thread> t_obj(threads);
	std::vector<Thread_Options> topts;

	size_t kernel_group_size = kernel_groups.size();
	int tid = 0;
	for(size_t i = 0; i < kernel_group_size; i++) {
		size_t pages_begin = std::round(double(smog_pages) / kernel_group_size * i);
		size_t pages_end = std::round(double(smog_pages) / kernel_group_size * (i + 1)) - 1;
		size_t thread_pages = pages_end - pages_begin + 1;
		void *thread_buffer = (void*)((uintptr_t)buffer + pages_begin * g_smog_pagesize);
		for(size_t k = 0; k < kernel_groups[i].size(); k++) {
			g_thread_status[i].count = 0;
			Smog_Kernel *kernel;
			bool first = (k == 0);
			switch(kernel_groups[i][k]) {
				case 'l':
					kernel = new Linear_Scan(first);
					break;
				case 'r':
					kernel = new Random_Access(first);
					break;
				case 'w':
					kernel = new Random_Write(first);
					break;
				case 'p':
					kernel = new Pointer_Chase(first);
					break;
				case 'c':
					kernel = new Cold(first);
					break;
				case 'd':
					kernel = new Dirty_Pages(first);
					break;
				default:
					std::cout << "Unknown kernel, must be one of: l, r, w, p, c, d" << std::endl;
					return 0;
			}

			std::cout << "  creating SMOG thread #" << tid << " with " << thread_pages << " pages (" << pages_begin << "--" << pages_end << ")" << std::endl;

			topts.push_back(Thread_Options(tid, thread_pages, thread_buffer));
			kernel->Configure(topts[tid]);
			if (g_smog_delay > 0)
				t_obj[tid] = std::thread(&Smog_Kernel::Run, kernel);
			else
				t_obj[tid] = std::thread(&Smog_Kernel::Run_Unhinged, kernel);
			tid++;
		}
	}

	const int PHASE_DYNAMIC_RAMP_UP = 0;
	const int PHASE_STEADY_ADJUST   = 1;
	const int PHASE_CONSTANT_DELAY  = 2;

	size_t monitor_ticks = 0;
	int phase = PHASE_DYNAMIC_RAMP_UP;

	// sync with worker threads
	pthread_barrier_wait(&g_initalization_finished);

	std::cout << "initialization finished, starting monitor" << std::endl;

	std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
	std::chrono::steady_clock::time_point prev = start;

	while (1) {
		std::this_thread::sleep_for(std::chrono::milliseconds(monitor_interval));
		std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
		std::chrono::duration<double> elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now - prev);
		prev = now;

		size_t sum = 0;

		for (size_t i = 0; i < threads; ++i) {
			// NOTE: for very fast kernels, this loses us a couple iterations between this line and the next
			size_t work_items = g_thread_status[i].count - g_thread_status[i].last_count;
			g_thread_status[i].last_count = g_thread_status[i].count;

			sum += work_items;
			std::cout << "[" << i << "] " << kernels[i] << " " << work_items << " iterations";
			std::cout << " at " << (work_items * 1.0 / elapsed.count()) << " iterations/s, elapsed: " << elapsed.count() * 1000 << " ms";
			std::cout << ", " << (work_items * 1.0 / elapsed.count() * CACHE_LINE_SIZE / 1024 / 1024) << " MiB/s";
			std::cout << ", per iteration: " << elapsed.count() * 1000000000 / work_items << " nanoseconds" << std::endl;

			if(csv_file.is_open())
				csv_file << i << "," << work_items << ","<< elapsed.count() << std::endl;
		}
		double current_rate = sum * 1.0 / elapsed.count();
		std::cout << "total: " << sum << " cache lines";
		std::cout << " at " << current_rate << " cache lines/s";
		std::cout << ", " << (sum * 1.0 / elapsed.count() * CACHE_LINE_SIZE / 1024 / 1024) << " MiB/s";
		if (target_rate) {
			std::cout << " (" << (100.0 * current_rate / target_rate) << "%)";
		}
		std::cout << ", per item: " << elapsed.count() * 1000000000 / sum * threads << " nanoseconds";
		if (current_rate > smog_pages) {
			// If the rate of pages/s exceeds the amount of available pages, then pages are touched
			// multiple times per second. this changes the effectiveness of the smog dirty rate and
			// is reported as a metric of 'overpaging', if present.
			//
			// Overpaging in this context is a percentage of pages touched per second in the last
			// measuring interval that exceed the number of allocated pages for dirtying.
			//
			// For example, a smog run on 10000 allocated pages that touched 20000 pages per second
			// would be reported as 100% overpaging by the code below.
			std::cout << ", Overpaging: " << (100.0 * (current_rate / smog_pages - 1)) << "%";
		}
		std::cout << std::endl;

		if (target_rate) {
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
			size_t current_delay = g_smog_delay;
			double target_delay = current_rate * current_delay / target_rate;

			if (phase < PHASE_CONSTANT_DELAY) {

				if (phase == PHASE_DYNAMIC_RAMP_UP) {
					double epsilon = 0.05;
					if (abs(target_delay - current_delay) < target_delay * epsilon) {
						std::cout << "  reached target range after " << monitor_ticks << " ticks" << std::endl;
						phase = PHASE_STEADY_ADJUST;
					}
				}

				if (phase == PHASE_DYNAMIC_RAMP_UP) {
					g_smog_delay = std::max(target_delay, 1.0);
				} else {
					g_smog_delay = std::max((target_delay + current_delay) / 2.0, 1.0);
				}

				std::cout << "  adjusting delay: " << current_delay << " -> " << g_smog_delay << " (by " << (int)(g_smog_delay - current_delay) << ")" << std::endl;

				std::chrono::duration<double> total_elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now - start);
				if (g_smog_timeout > 0 && total_elapsed >= std::chrono::seconds(g_smog_timeout)) {
					std::cout << "  reached adjustment timeout after " << monitor_ticks << " ticks" << std::endl;
					phase = PHASE_CONSTANT_DELAY;
				}
			}
		}

		monitor_ticks++;
	}

	for(size_t i = 0; i < threads; i++) {
		t_obj[i].join();
	}
	if(csv_file.is_open())
		csv_file.close();
	return 0;
}
