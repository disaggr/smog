#pragma once
#include "smog.h"
#include "kernel.h"

class Cold : public Smog_Kernel {
	public:
		Cold(bool initialize) : Smog_Kernel(initialize, false) {}
	protected:
		void Execute_Kernel() override;
};
