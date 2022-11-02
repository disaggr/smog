#pragma once
#include "smog.h"
#include "kernel.h"
#include <cstdint>

class Random_Write : public Smog_Kernel {
	public:
		Random_Write(bool initialize) : Smog_Kernel(initialize, false) {}
	protected:
		void Execute_Kernel() override;
};