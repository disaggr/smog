#pragma once
#include "smog.h"
#include "kernel.h"
#include <cstdint>

class Random_Access : public Smog_Kernel {
	public:
		Random_Access(bool initialize) : Smog_Kernel(initialize) {}
	protected:
		void Execute_Kernel() override;
};