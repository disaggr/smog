#pragma once
#include "smog.h"
#include "kernel.h"
#include <cstdint>

class Linear_Scan : public Smog_Kernel {
	public:
		Linear_Scan() {}
	protected:
		void Initialize() override;
		void Execute_Kernel() override;
    private:
        uint64_t *m_buffer;
        uint64_t m_elements;
};
