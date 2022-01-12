#include <thread>
#include <iostream>
#include <sys/mman.h>
#include <cerrno>
#include <boost/program_options.hpp>

namespace popts = boost::program_options;

class Thread_Options {
	public:
		Thread_Options(int from, int to, int page_size, int *buffer) :
			from(from), to(to), page_size(page_size), buffer(buffer)
		{}
		int from;
		int to;
		int page_size;
		int *buffer;
};

void dirty_pages(Thread_Options t_opts) {
	register int tmp = 0;
	for(int i = t_opts.from; i < t_opts.to * t_opts.page_size; i += t_opts.page_size) {
		tmp += t_opts.buffer[i];
		t_opts.buffer[i] = tmp;
		std::cout << "accessing " << i << std::endl;
	}
}

int main(int argc, char* argv[]) {
	int threads = 0;
	int pages = 0;
	size_t page_size = 0;
	void *buffer = 0;

	popts::options_description description("SMOG Usage");
	description.add_options()
		("help,h", "Display this help message")
		("threads,t", popts::value<int>()->default_value(1), "Number of threads to spawn")
		("pages,p", popts::value<int>()->default_value(1), "Number of pages to allocate");

	popts::variables_map vm;
	popts::store(popts::command_line_parser(argc, argv).options(description).run(), vm);
	popts::notify(vm);

	if(vm.count("help")) {
		std::cout << description << std::endl;
	}

	if(vm.count("threads")) {
		threads = vm["threads"].as<int>();
		std::cout << "Threads: " << threads << std::endl;
	}

	if(vm.count("pages")) {
		pages = vm["pages"].as<int>();
		std::cout << "Pages: " << pages << std::endl;
	}

	page_size = getpagesize();
	buffer = mmap(NULL, pages, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if(buffer == MAP_FAILED) {
		std::cout << "mmap failed: " << std::strerror(errno) << std::endl;
	}

	std::vector<std::thread> t_obj(threads);
	std::vector<Thread_Options> topts;
	Thread_Options topt(0, pages, page_size, (int *) buffer);
	for(int i = 0; i < threads; i++) {
		std::cout << "Creating Thread " << i << std::endl;
		topts.push_back(topt);
		t_obj[i] = std::thread(dirty_pages, topts[i]);
	}
	
	for(int i = 0; i < threads; i++) {
		t_obj[i].join();
	}
	
	return 0;
}
