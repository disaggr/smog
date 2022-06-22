#pragma once
#include "smog.h"
#include "kernel.h"

#define PARALLEL_ACCESSES 4

struct __attribute__((packed)) random_element {
	uint64_t randoms[PARALLEL_ACCESSES];
	uint64_t index;
	char padding[CACHE_LINE_SIZE - PARALLEL_ACCESSES + 1 * sizeof(uint64_t)];
};

class Random_Access : Smog_Kernel {
	public:
		Random_Access() {}
	protected:
		void Initialize();
		void Execute_Kernel();
    private:
        struct random_element *m_buffer;
        uint64_t m_elements;
};