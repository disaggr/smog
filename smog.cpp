#include <thread>
#include <iostream>
#include <sys/mman.h>
#include <cerrno>
#include <unistd.h>
#include <vector>
#include <cstring>
#include <boost/program_options.hpp>
#include <chrono>
#include <errno.h>
#include <time.h>

#include <chrono>
#include <sys/time.h>
#include <ctime>
#include <iostream>

using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::system_clock;


namespace popts = boost::program_options;

// globals
size_t page_size;
size_t smog_delay;
bool measuring = false;

struct thread_status_t {
	size_t count;
};

struct thread_status_t *thread_status;

class Thread_Options {
	public:
		Thread_Options(int tid, size_t page_count, void *page_buffer) :
			tid(tid), page_count(page_count), page_buffer(page_buffer)
		{}
		int tid;
		size_t page_count;
		void *page_buffer;
};

long double get_unixtime(){
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (long double)tv.tv_sec+(long double)(tv.tv_usec)/1e6;

}

void dirty_pages(Thread_Options t_opts) {
	int work_items = t_opts.page_count;

	while (1) {
		for(size_t i = 0; i < work_items * page_size; i += page_size) {
			// Here I am assuming the impact of skipping a few pages is not
			// going to be a big issue
			if (measuring) {
				continue;
			}
			int tmp = *(int*)((uintptr_t)t_opts.page_buffer + i);
			*(int*)((uintptr_t)t_opts.page_buffer + i) = tmp + 1;

			thread_status[t_opts.tid].count += 1;

			volatile uint64_t delay = 0;
			for(size_t j = 0; j < smog_delay; j++) {
				 delay += 1;
			}
		}
	}
}

int main(int argc, char* argv[]) {
	size_t threads = 0;
	size_t system_pages = 0;
	size_t smog_pages = 0;
	size_t hardware_concurrency = 0;
	size_t target_rate = 0; // pages/s

	// determine system characteristics
	system_pages = sysconf(_SC_PHYS_PAGES);
	page_size = sysconf(_SC_PAGE_SIZE);
	hardware_concurrency = std::thread::hardware_concurrency();

	// per default, spawn one SMOG thread per core
	size_t default_threads = hardware_concurrency;
	// per default, allocate 2 GiB in memory pages for SMOG
	size_t default_pages = std::min( (2UL * 1024 * 1024 * 1024) / page_size, system_pages / 2);
	// per default, use a delay of 1000ns in the SMOG threads
	size_t default_delay = 1000; // ns

	std::cout << "System page size: " << page_size  << " Bytes" << std::endl;
	// parse CLI options
	popts::options_description description("SMOG Usage");
	description.add_options()
		("help,h", "Display this help message")
		("threads,t", popts::value<size_t>()->default_value(default_threads), "Number of threads to spawn")
		("pages,p", popts::value<size_t>()->default_value(default_pages), "Number of pages to allocate")
		("delay,d", popts::value<size_t>()->default_value(default_delay), "Delay in nanoseconds per thread per iteration")
		("rate,r", popts::value<size_t>()->default_value(0), "Target dirty rate to automatically adjust delay");

	popts::variables_map vm;
	popts::store(popts::command_line_parser(argc, argv).options(description).run(), vm);
	popts::notify(vm);

	if(vm.count("help")) {
		std::cout << description << std::endl;
		return 0;
	}

	std::cout << "SMOG dirty-page benchmark" << std::endl;

	if(vm.count("threads")) {
		threads = vm["threads"].as<size_t>();
	}

	if(vm.count("pages")) {
		smog_pages = vm["pages"].as<size_t>();
	}

	if(vm.count("delay")) {
		smog_delay = vm["delay"].as<size_t>();
	}

	if(vm.count("rate")) {
		target_rate = vm["rate"].as<size_t>();
	}

	// allocate global SMOG page buffer
	void *buffer = mmap(NULL, smog_pages * page_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if(buffer == MAP_FAILED) {
		std::cout << "mmap failed: " << std::strerror(errno) << std::endl;
		return 1;
	}
	else {
		std::cout << "  pagesize: " << (page_size >> 10) << "KiB" << std::endl;
		std::cout << "Allocated " << smog_pages << " pages (" << (smog_pages * (page_size >> 10)) << "KiB)" << std::endl;
	}

	// prepare page preambles
	for(size_t i = 0; i < smog_pages; ++i) {
		*(int*)((uintptr_t)buffer + i * page_size) = 0;
	}

	// launch SMOG threads
	std::cout << "Creating " << threads << " SMOG threads" << std::endl;

	thread_status = new struct thread_status_t[threads];

	std::vector<std::thread> t_obj(threads);
	std::vector<Thread_Options> topts;

	for(size_t i = 0; i < threads; i++) {
		size_t pages_begin = std::round(double(smog_pages) / threads * i);
		size_t pages_end = std::round(double(smog_pages) / threads * (i + 1)) - 1;
		size_t thread_pages = pages_end - pages_begin + 1;
		void *thread_buffer = (void*)((uintptr_t)buffer + pages_begin * page_size);
		thread_status[i].count = 0;

		std::cout << "  creating SMOG thread #" << i << " with " << thread_pages << " pages (" << pages_begin << "--" << pages_end << ")" << std::endl;

		topts.push_back(Thread_Options(i, thread_pages, thread_buffer));
		t_obj[i] = std::thread(dirty_pages, topts[i]);
	}

	size_t monitor_delay = 1000; // ms
	size_t monitor_ticks = 0;
	int phase = 0; // dynamic ramp up

	std::chrono::steady_clock::time_point prev = std::chrono::steady_clock::now();

	while (1) {
		measuring = false;
		std::this_thread::sleep_for(std::chrono::milliseconds(monitor_delay));
		measuring = true;
		std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
		std::chrono::duration<double> elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now - prev);
		prev = now;

		size_t sum = 0;

		for (size_t i = 0; i < threads; ++i) {
			size_t work_items = thread_status[i].count;
			sum += work_items;
			thread_status[i].count = 0;
			std::cout << "[" << i << "] touched " << work_items << " pages";
			std::cout << " at " << (work_items * 1.0 / elapsed.count()) << " pages/s, elapsed: " << elapsed.count();
			std::cout << ", " << (work_items * 1.0 / elapsed.count() * page_size / 1024 / 1024) << " MiB/s";
			std::cout << ", per item: " << elapsed.count() * 1000000000 / work_items << " nanoseconds" << std::endl;
		}
		double current_rate = sum * 1.0 / elapsed.count();
		std::cout << "total: " << sum << " pages";
		std::cout << " at " << current_rate << " pages/s";
		std::cout << ", " << (sum * 1.0 / elapsed.count() * page_size / 1024 / 1024) << " MiB/s";
		if (target_rate) {
			std::cout << " (" << (100.0 * current_rate / target_rate) << "%)";
		}
		std::cout << ", per item: " << elapsed.count() * 1000000000 / sum * threads << " nanoseconds" << std::endl;

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
			size_t current_delay = smog_delay;
			double target_delay = current_rate * current_delay / target_rate;

			if (phase == 0) {
				double epsilon = 0.05;
				if (abs(target_delay - current_delay) < target_delay * epsilon) {
					std::cout << "  reached target range after " << monitor_ticks << " ticks" << std::endl;
					phase = 1;
				}
			}

			if (phase == 0) {
				smog_delay = target_delay;
			} else {
				smog_delay = (target_delay + current_delay) / 2;
			}

			std::cout << "  adjusting delay: " << current_delay << " -> " << smog_delay << " (by " << (int)(smog_delay - current_delay) << ")" << std::endl;
		}

		monitor_ticks++;
	}

	for(size_t i = 0; i < threads; i++) {
		t_obj[i].join();
	}

	return 0;
}
