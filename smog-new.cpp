#include <thread>
#include <iostream>
#include <sys/mman.h>
#include <cerrno>
#include <unistd.h>
#include <vector>
#include <cstring>
//#include <boost/program_options.hpp>

//namespace popts = boost::program_options;

class Thread_Options {
	public:
		Thread_Options(int tid, size_t from, size_t to, size_t page_size, int *buffer) :
			tid(tid), from(from), to(to), page_size(page_size), buffer(buffer)
		{}
		int tid;
		size_t from;
		size_t to;
		size_t page_size;
		int *buffer;
};

void dirty_pages(Thread_Options t_opts) {
	register int tmp = 0;
	size_t page_elements = t_opts.page_size / sizeof(*t_opts.buffer);
	for(size_t i = t_opts.from; i < t_opts.to * page_elements; i += page_elements) {
		tmp += t_opts.buffer[i];
		t_opts.buffer[i] = tmp;
		printf("[%i] accessing 0x%08zx  (page no. %zu)\n", t_opts.tid, i, i / page_elements);
		fflush(stdout);
	}
}

int main(int argc, char* argv[]) {
	int threads = 2;
	int pages = 1000;
	size_t page_size = 0;
	void *buffer = 0;

//	popts::options_description description("SMOG Usage");
//	description.add_options()
//		("help,h", "Display this help message")
//		("threads,t", popts::value<int>()->default_value(1), "Number of threads to spawn")
//		("pages,p", popts::value<int>()->default_value(1), "Number of pages to allocate");
//
//	popts::variables_map vm;
//	popts::store(popts::command_line_parser(argc, argv).options(description).run(), vm);
//	popts::notify(vm);
//
//	if(vm.count("help")) {
//		std::cout << description << std::endl;
//	}
//
//	if(vm.count("threads")) {
//		threads = vm["threads"].as<int>();
//		std::cout << "Threads: " << threads << std::endl;
//	}
//
//	if(vm.count("pages")) {
//		pages = vm["pages"].as<int>();
//		std::cout << "Pages: " << pages << std::endl;
//	}

	page_size = getpagesize();
	buffer = mmap(NULL, pages * page_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if(buffer == MAP_FAILED) {
		std::cout << "mmap failed: " << std::strerror(errno) << std::endl;
	}

	std::vector<std::thread> t_obj(threads);
	std::vector<Thread_Options> topts;
	
	for(int i = 0; i < threads; i++) {
		std::cout << "Creating Thread " << i << std::endl;
		topts.push_back(Thread_Options(i, 0, pages, page_size, (int *) buffer));
		t_obj[i] = std::thread(dirty_pages, topts[i]);
	}
	
	for(int i = 0; i < threads; i++) {
		t_obj[i].join();
	}
	
	return 0;
}
