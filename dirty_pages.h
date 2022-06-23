#pragma once
#include "smog.h"
#include "kernel.h"

class Dirty_Pages : public Smog_Kernel {
	public:
		Dirty_Pages() {}
	protected:
		void Initialize() override;
		void Execute_Kernel() override;
};
