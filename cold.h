#pragma once
#include "smog.h"
#include "kernel.h"

class Cold : Smog_Kernel {
	public:
		Cold() {}
	protected:
		virtual void Initialize() {}
		virtual void Execute_Kernel() {}
};
