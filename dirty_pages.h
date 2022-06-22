#pragma once
#include "smog.h"
#include "kernel.h"

class Dirty_Pages : Smog_Kernel {
	public:
		Dirty_Pages() {}
	protected:
		virtual void Initialize() {}
		virtual void Execute_Kernel() {}
};
