#pragma once
#include "smog.h"
#include "kernel.h"
#include <cstdint>

class Linear_Scan : public Smog_Kernel {
	public:
		Linear_Scan(bool initialize) : Smog_Kernel(initialize) {}
	protected:
		void Execute_Kernel() override;
};
