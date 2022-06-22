#pragma once
#include "smog.h"
#include "kernel.h"

class Linear_Scan : Smog_Kernel {
	public:
		Linear_Scan() {}
	protected:
		virtual void Initialize() {}
		virtual void Execute_Kernel() {}
    private:
        uint64_t *m_buffer;
        uint64_t m_elements;
};
