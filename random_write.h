#pragma once
#include "smog.h"
#include "kernel.h"
#include <cstdint>

#define PARALLEL_ACCESSES 4

struct __attribute__((packed)) random_element_write {
	uint64_t randoms[PARALLEL_ACCESSES];
	uint64_t index;
    uint64_t random_write;
	char padding[CACHE_LINE_SIZE - PARALLEL_ACCESSES + 2 * sizeof(uint64_t)];
};

class Random_Write : public Smog_Kernel {
	public:
		Random_Write() {}
	protected:
		void Initialize() override;
		void Execute_Kernel() override;
    private:
        struct random_element_write *m_buffer;
        uint64_t m_elements;
};