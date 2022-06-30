#pragma once
#include "smog.h"
#include "kernel.h"

class Dirty_Pages : public Smog_Kernel {
	public:
		Dirty_Pages(bool initialize) : Smog_Kernel(initialize) {}
	protected:
		void Execute_Kernel() override;
};
